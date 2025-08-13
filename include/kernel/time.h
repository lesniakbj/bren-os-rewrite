#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <libc/stdint.h>
#include <arch/i386/time.h> // For CMOS_Time

// Shared buffer for the console clock string
extern char console_time_buffer[32];
// Shared color for the console clock
extern kuint8_t console_clock_color;
// Authoritative seconds counter updated by the RTC
extern volatile kuint64_t g_unix_seconds;


/**
 * @brief Initializes the system time based on the initial CMOS read.
 *
 * This should be called once during kernel initialization after time_init().
 *
 * @param initial_cmos_time Pointer to the CMOS_Time struct obtained from time_init().
 */
void system_time_init(CMOS_Time *initial_cmos_time);

/**
 * @brief Gets the current high-resolution system time.
 *
 * Calculates the current time based on the initial CMOS time and
 * elapsed PIT ticks.
 *
 * @param seconds Pointer to store the current Unix timestamp (seconds).
 * @param nanoseconds Pointer to store the nanosecond remainder.
 */
void get_current_time(kuint64_t *seconds, kuint64_t *nanoseconds);

/**
 * @brief Updates the console clock display.
 *
 * Uses ANSI escape sequences to update the clock in place on the terminal.
 * This function calculates the current time and formats it for display.
 */
void update_console_clock();

#endif // KERNEL_TIME_H