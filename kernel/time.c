#include <kernel/time.h>
#include <drivers/pit.h>              // For pit_get_tick_count, pit_get_frequency
#include <arch/i386/time.h>           // For time_to_unix_seconds
#include <drivers/text_mode_console.h> // For text_mode_write_at
#include <drivers/screen.h>           // For VGA color constants
#include <libc/strings.h>              // For snprintf (might be needed for formatting, or we can do it manually)

// --- Global state for system time ---

// Base Unix timestamp from CMOS initialization
static kuint64_t base_unix_timestamp = 0;

// Flag to check if system time is initialized
static int system_time_initialized = 0;

/**
 * @brief Initializes the system time based on the initial CMOS read.
 *
 * This should be called once during kernel initialization after time_init().
 *
 * @param initial_cmos_time Pointer to the CMOS_Time struct obtained from time_init().
 */
void system_time_init(CMOS_Time *initial_cmos_time) {
    if (initial_cmos_time == NULL) {
        // Handle error or log
        return;
    }
    base_unix_timestamp = time_to_unix_seconds(initial_cmos_time);
    system_time_initialized = 1;
}

/**
 * @brief Gets the current high-resolution system time.
 *
 * Calculates the current time based on the initial CMOS time and
 * elapsed PIT ticks. Calculates elapsed time in microseconds to
 * avoid 64-bit division issues.
 *
 * @param seconds Pointer to store the current Unix timestamp (seconds).
 * @param nanoseconds Pointer to store the nanosecond remainder.
 */
void get_current_time(kuint64_t *seconds, kuint64_t *nanoseconds) {
    if (!system_time_initialized || seconds == NULL || nanoseconds == NULL) {
        if (seconds) *seconds = 0;
        if (nanoseconds) *nanoseconds = 0;
        return;
    }

    unsigned int tick_count = pit_get_tick_count();
    unsigned int frequency = pit_get_frequency(); // Declared in pit.h

    if (frequency == 0) {
         // Avoid division by zero
         *seconds = base_unix_timestamp;
         *nanoseconds = 0;
         return;
    }

    // --- Calculate elapsed time in microseconds ---
    // elapsed_us = (tick_count * 1000000) / frequency;
    // To avoid 64-bit division, we break it down.
    // elapsed_us = (tick_count / frequency) * 1000000 + ((tick_count % frequency) * 1000000) / frequency;

    kuint64_t elapsed_us_final = 0;
    if (frequency <= 1000000) {
        unsigned int whole_seconds_in_ticks = tick_count / frequency;
        unsigned int remainder_ticks = tick_count % frequency;
        
        unsigned int remainder_us_part = 0;
        if (frequency <= 4294) {
            // Safe for 32-bit intermediate calculation
            remainder_us_part = (remainder_ticks * 1000000) / frequency;
        } else {
            // For higher frequencies, use 64-bit intermediate.
            // This relies on our custom __udivdi3 implementation.
            kuint64_t remainder_us_64 = ((kuint64_t)remainder_ticks * 1000000ULL) / (kuint64_t)frequency;
            remainder_us_part = (unsigned int)remainder_us_64;
        }

        kuint64_t whole_seconds_us = (kuint64_t)whole_seconds_in_ticks * 1000000ULL;
        elapsed_us_final = whole_seconds_us + (kuint64_t)remainder_us_part;
    } else {
        // Fallback for very high frequencies (unlikely for PIT)
        elapsed_us_final = ((kuint64_t)tick_count * 1000000ULL) / (kuint64_t)frequency;
    }

    // Convert elapsed microseconds to seconds and nanoseconds
    *seconds = base_unix_timestamp + (elapsed_us_final / 1000000ULL);
    *nanoseconds = (elapsed_us_final % 1000000ULL) * 1000ULL; // Convert us remainder to ns
}


/**
 * @brief A simple structure to hold broken-down time components.
 */
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond; // Derived from nanoseconds
} time_components_t;

/**
 * @brief Converts Unix time (seconds) to calendar components.
 *
 * @param unix_time The Unix timestamp in seconds.
 * @param tc Pointer to the time_components_t struct to fill.
 */
static void unix_time_to_components(kuint64_t unix_time, time_components_t *tc) {
    if (tc == NULL) return;

    // Days in each month (non-leap year)
    static const int days_in_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // Function to check if a year is a leap year
    // Using a macro for simplicity and compatibility.
    #define IS_LEAP_YEAR(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))

    kuint64_t days_since_epoch = unix_time / 86400ULL;
    kuint64_t remaining_seconds = unix_time % 86400ULL;

    tc->hour = remaining_seconds / 3600;
    remaining_seconds %= 3600;
    tc->minute = remaining_seconds / 60;
    tc->second = remaining_seconds % 60;

    // Calculate year, month, day from days since epoch
    tc->year = 1970;
    tc->month = 1;
    tc->day = 1;

    kuint64_t tmp_days = days_since_epoch;
    // Calculate the year
    while (1) {
        int days_in_year = IS_LEAP_YEAR(tc->year) ? 366 : 365;
        if (tmp_days >= (kuint64_t)days_in_year) {
            tmp_days -= days_in_year;
            tc->year++;
        } else {
            break;
        }
    }

    // Now tmp_days is the day of the year (0-based)
    int day_of_year = tmp_days;

    // Calculate the month and day
    for (tc->month = 1; tc->month <= 12; tc->month++) {
        int dim = days_in_month[tc->month];
        if (tc->month == 2 && IS_LEAP_YEAR(tc->year)) {
            dim = 29;
        }
        if (day_of_year < dim) {
            tc->day = day_of_year + 1; // Convert to 1-based
            break;
        }
        day_of_year -= dim;
    }

    // Ensure basic validity (should be guaranteed by the loop logic)
    if (tc->month < 1) tc->month = 1;
    if (tc->month > 12) tc->month = 12;
    if (tc->day < 1) tc->day = 1;
    
    #undef IS_LEAP_YEAR // Clean up the macro
}

/**
 * @brief A simple integer to string conversion function.
 * 
 * @param buffer The buffer to write the string to.
 * @param value The integer value to convert.
 * @param width The minimum width of the string (padded with leading zeros).
 */
static void itoa_pad(char *buffer, int value, int width) {
    // Handle negative numbers if needed, but for time components, they should be positive.
    // For simplicity, assume positive.
    char temp[32]; // Enough for 32-bit int
    int i = 0;
    
    if (value == 0) {
        temp[i++] = '0';
    } else {
        while (value > 0) {
            temp[i++] = '0' + (value % 10);
            value /= 10;
        }
    }
    
    // Pad with zeros if necessary
    while (i < width) {
        temp[i++] = '0';
    }
    
    // Reverse the string
    int len = i;
    for (int j = 0; j < len; j++) {
        buffer[j] = temp[len - 1 - j];
    }
    buffer[len] = '\0';
}


/**
 * @brief Updates the console clock display.
 *
 * Displays time in YYYY-MM-DD HH:MM:SS.nnn format at a fixed terminal position.
 */
void update_console_clock() {
    if (!system_time_initialized) {
        return; // Not initialized yet
    }

    kuint64_t current_seconds, current_nanoseconds;
    get_current_time(&current_seconds, &current_nanoseconds);

    time_components_t tc;
    unix_time_to_components(current_seconds, &tc);
    tc.millisecond = current_nanoseconds / 1000000; // Convert nanoseconds to milliseconds

    // --- Format the time string ---
    // Using manual formatting to avoid snprintf dependency
    char time_buffer[32]; // Should be sufficient for YYYY-MM-DD HH:MM:SS.nnn
    int idx = 0;
    
    // Year (4 digits)
    itoa_pad(&time_buffer[idx], tc.year, 4);
    idx += 4;
    time_buffer[idx++] = '-';
    
    // Month (2 digits)
    itoa_pad(&time_buffer[idx], tc.month, 2);
    idx += 2;
    time_buffer[idx++] = '-';
    
    // Day (2 digits)
    itoa_pad(&time_buffer[idx], tc.day, 2);
    idx += 2;
    time_buffer[idx++] = ' ';
    
    // Hour (2 digits)
    itoa_pad(&time_buffer[idx], tc.hour, 2);
    idx += 2;
    time_buffer[idx++] = ':';
    
    // Minute (2 digits)
    itoa_pad(&time_buffer[idx], tc.minute, 2);
    idx += 2;
    time_buffer[idx++] = ':';
    
    // Second (2 digits)
    itoa_pad(&time_buffer[idx], tc.second, 2);
    idx += 2;
    time_buffer[idx++] = '.';
    
    // Millisecond (3 digits)
    itoa_pad(&time_buffer[idx], tc.millisecond, 3);
    idx += 3;
    
    time_buffer[idx] = '\0'; // Null terminate

    // --- Write the formatted time string directly to the screen ---
    // Target position: Row 0 (top line), Column 60
    // Use a distinct color for the clock, e.g., Light Cyan on Black
    kuint8_t clock_color = 11 | (0 << 4);
    
    // Use the new function to write directly to the VGA buffer
    text_mode_write_at(0, 57, time_buffer, -1, clock_color);
    // The -1 tells it to write the entire null-terminated string.
}