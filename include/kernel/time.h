#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#define IS_LEAP_YEAR(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))

#define CURRENT_YEAR 2025

#include <libc/stdint.h>
#include <arch/i386/time.h>

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
} time_components_t;

extern char console_time_buffer[32];
extern kuint8_t console_clock_color;
extern volatile kuint64_t g_unix_seconds;

void system_time_init(cmos_time_t *initial_cmos_time);
void get_current_time(kuint64_t *seconds, kuint64_t *nanoseconds);
void update_console_clock();

#endif