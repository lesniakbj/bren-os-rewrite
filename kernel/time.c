#include <kernel/time.h>
#include <drivers/pit.h>
#include <drivers/text_mode_console.h>
#include <drivers/screen.h>
#include <drivers/pit.h>
#include <arch/i386/time.h>
#include <libc/strings.h>
#include <libc/stdlib.h>

// Seconds counter, updated by the RTC interrupt
volatile kuint64_t g_unix_seconds = 0;
static kint32_t system_time_initialized = 0;

// Used to display time information on the terminal
char console_time_buffer[32] = "Initializing...";
kuint8_t console_clock_color = 0x0F;

static void unix_time_to_components(kuint64_t unix_time, time_components_t *tc) {
    if (tc == NULL) return;

    // Days in each month (non-leap year)
    static const kint32_t days_in_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // 86400s == 1 Day
    kuint64_t days_since_epoch = unix_time / 86400ULL;
    kuint64_t remaining_seconds = unix_time % 86400ULL;

    // Calculate the hour/min/sec portion
    tc->hour = remaining_seconds / 3600;
    remaining_seconds %= 3600;
    tc->minute = remaining_seconds / 60;
    tc->second = remaining_seconds % 60;

    // Calculate year, month, day from days since epoch
    tc->year = 1970;
    tc->month = 1;
    tc->day = 1;

    // Calculate the year
    kuint64_t tmp_days = days_since_epoch;
    while (1) {
        kint32_t days_in_year = IS_LEAP_YEAR(tc->year) ? 366 : 365;
        if (tmp_days >= (kuint64_t)days_in_year) {
            tmp_days -= days_in_year;
            tc->year++;
        } else {
            break;
        }
    }

    // Now tmp_days is the day of the year (0-based)
    // Calculate the month and day
    kint32_t day_of_year = tmp_days;
    for (tc->month = 1; tc->month <= 12; tc->month++) {
        kint32_t dim = days_in_month[tc->month];
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
}

static void format_and_pad(char* buffer, kint32_t value, kint32_t width) {
    char temp_str[32];
    itoa(temp_str, 'd', value);

    int len = strlen(temp_str);
    int padding = width - len;

    // Add left-padding with zeros if the number is shorter than the width
    if (padding > 0) {
        for (int i = 0; i < padding; i++) {
            *buffer++ = '0';
        }
    }

    // Copy the number string itself
    strcpy(buffer, temp_str);
}

void system_time_init(cmos_time_t *initial_cmos_time) {
    if (initial_cmos_time == NULL) {
        return;
    }
    g_unix_seconds = time_to_unix_seconds(initial_cmos_time);
    system_time_initialized = 1;
}

void get_current_time(kuint64_t *seconds, kuint64_t *nanoseconds) {
    if (!system_time_initialized || seconds == NULL || nanoseconds == NULL) {
        if (seconds) *seconds = 0;
        if (nanoseconds) *nanoseconds = 0;
        return;
    }

    // The second count comes from our RTC-updated global variable
    *seconds = g_unix_seconds;

    // The nanosecond part is derived from the PIT's progress through the current second
    kuint32_t tick_count = pit_get_tick_count();
    kuint32_t frequency = pit_get_frequency();
    if (frequency == 0) {
         *nanoseconds = 0;
         return;
    }

    // Calculate total nanoseconds since boot according to the PIT, he remainder is the nanoseconds within the current second.
    kuint64_t total_ns_from_pit = ((kuint64_t)tick_count * 1000000000ULL) / (kuint64_t)frequency;
    *nanoseconds = total_ns_from_pit % 1000000000ULL;
}

void update_console_clock() {
    if (!system_time_initialized) {
        return;
    }

    // Get the current seconds and nanoseconds from our system timers
    kuint64_t current_seconds, current_nanoseconds;
    get_current_time(&current_seconds, &current_nanoseconds);

    // Convert those seconds/ns from Unix Time to components (Day/Month/Year)
    time_components_t tc;
    unix_time_to_components(current_seconds, &tc);
    tc.millisecond = current_nanoseconds / 1000000; // Convert nanoseconds to milliseconds

    // --- Format the time string --- 
    char time_buffer[32];
    char* p = time_buffer;
    format_and_pad(p, tc.year, 4); p += 4;
    *p++ = '-';
    format_and_pad(p, tc.month, 2); p += 2;
    *p++ = '-';
    format_and_pad(p, tc.day, 2); p += 2;
    *p++ = ' ';
    format_and_pad(p, tc.hour, 2); p += 2;
    *p++ = ':';
    format_and_pad(p, tc.minute, 2); p += 2;
    *p++ = ':';
    format_and_pad(p, tc.second, 2); p += 2;
    *p++ = '.';
    format_and_pad(p, tc.millisecond, 3); p += 3;
    *p = '\0';

    // Update the shared buffer
    strcpy(console_time_buffer, time_buffer);
    console_clock_color = 11 | (0 << 4);
}
