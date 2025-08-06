#ifndef ARCH_I386_TIME_H
#define ARCH_I386_TIME_H

#include <libc/stdint.h>

#define CMOS_CMD_PORT 0x70
#define CMOS_DATA_PORT 0x71

#define SECONDS_DATA_PORT 0x00
#define MINUTES_DATA_PORT 0x02
#define HOURS_DATA_PORT 0x04
#define DAY_OF_MONTH_DATA_PORT 0x07
#define MONTH_DATA_PORT 0x08
#define YEAR_DATA_PORT 0x09
#define CENTURY_DATA_PORT 0x32
#define STATUS_REGISTER_A 0x0A
#define STATUS_REGISTER_B 0x0B

// Structure to hold all CMOS time fields
typedef struct {
    kuint8_t seconds;
    kuint8_t minutes;
    kuint8_t hours;
    kuint8_t day;
    kuint8_t month;
    kuint16_t year;
    kuint8_t century;
} CMOS_Time;

CMOS_Time time_init();
kuint64_t time_to_unix_seconds(CMOS_Time* t);

#endif