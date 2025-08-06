#include <arch/i386/time.h>
#include <arch/i386/io.h>

#define CURRENT_YEAR 2025

int century_register = CENTURY_DATA_PORT;

CMOS_Time current_time;

static int update_in_progress_flag() {
      outb(CMOS_CMD_PORT, 0x0A);
      return (inb(CMOS_DATA_PORT) & 0x80);
}

CMOS_Time time_init() {
    static CMOS_Time last_time;

    while (update_in_progress_flag());

    outb(CMOS_CMD_PORT, SECONDS_DATA_PORT);
    current_time.seconds = inb(CMOS_DATA_PORT);
    outb(CMOS_CMD_PORT, MINUTES_DATA_PORT);
    current_time.minutes = inb(CMOS_DATA_PORT);
    outb(CMOS_CMD_PORT, HOURS_DATA_PORT);
    current_time.hours = inb(CMOS_DATA_PORT);
    outb(CMOS_CMD_PORT, DAY_OF_MONTH_DATA_PORT);
    current_time.day = inb(CMOS_DATA_PORT);
    outb(CMOS_CMD_PORT, MONTH_DATA_PORT);
    current_time.month = inb(CMOS_DATA_PORT);
    outb(CMOS_CMD_PORT, YEAR_DATA_PORT);
    current_time.year = inb(CMOS_DATA_PORT);

    if(century_register != 0) {
        outb(CMOS_CMD_PORT, century_register);
        current_time.century = inb(CMOS_DATA_PORT);
    }

    do {
        last_time.seconds = current_time.seconds;
        last_time.minutes = current_time.minutes;
        last_time.hours = current_time.hours;
        last_time.day = current_time.day;
        last_time.month = current_time.month;
        last_time.year = current_time.year;
        last_time.century = current_time.century;

        while (update_in_progress_flag());

        outb(CMOS_CMD_PORT, SECONDS_DATA_PORT);
        current_time.seconds = inb(CMOS_DATA_PORT);
        outb(CMOS_CMD_PORT, MINUTES_DATA_PORT);
        current_time.minutes = inb(CMOS_DATA_PORT);
        outb(CMOS_CMD_PORT, HOURS_DATA_PORT);
        current_time.hours = inb(CMOS_DATA_PORT);
        outb(CMOS_CMD_PORT, DAY_OF_MONTH_DATA_PORT);
        current_time.day = inb(CMOS_DATA_PORT);
        outb(CMOS_CMD_PORT, MONTH_DATA_PORT);
        current_time.month = inb(CMOS_DATA_PORT);
        outb(CMOS_CMD_PORT, YEAR_DATA_PORT);
        current_time.year = inb(CMOS_DATA_PORT);
        if(century_register != 0) {
            outb(CMOS_CMD_PORT, century_register);
            current_time.century = inb(CMOS_DATA_PORT);
        }
    } while((last_time.seconds != current_time.seconds) || (last_time.minutes != current_time.minutes) || (last_time.hours != current_time.hours) ||
                           (last_time.day != current_time.day) || (last_time.month != current_time.month) || (last_time.year != current_time.year) ||
                           (last_time.century != current_time.century));

    outb(CMOS_CMD_PORT, STATUS_REGISTER_B);
    kuint8_t registerB = inb(CMOS_DATA_PORT);
    if (!(registerB & 0x04)) {
        current_time.seconds = (current_time.seconds & 0x0F) + ((current_time.seconds / 16) * 10);
        current_time.minutes = (current_time.minutes & 0x0F) + ((current_time.minutes / 16) * 10);
        current_time.hours = ( (current_time.hours & 0x0F) + (((current_time.hours & 0x70) / 16) * 10) );
        current_time.day = (current_time.day & 0x0F) + ((current_time.day / 16) * 10);
        current_time.month = (current_time.month & 0x0F) + ((current_time.month / 16) * 10);
        current_time.year = (current_time.year & 0x0F) + ((current_time.year / 16) * 10);
        if(century_register != 0) {
              current_time.century = (current_time.century & 0x0F) + ((current_time.century / 16) * 10);
        }
    }

    // Convert 12 hour clock to 24 hour clock if necessary
    if (!(registerB & 0x02) && (current_time.hours & 0x80)) {
        current_time.hours = ((current_time.hours & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year
    if(century_register != 0) {
        current_time.year += current_time.century * 100;
    } else {
        current_time.year += (CURRENT_YEAR / 100) * 100;
        if(current_time.year < CURRENT_YEAR) current_time.year += 100;
    }

    return current_time;
}

// Days in each month (non-leap year)
static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Function to check if a year is a leap year
static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

kuint64_t time_to_unix_seconds(CMOS_Time* t) {
    kuint64_t total_days = 0;
    int year = t->year;
    int month = t->month;
    int day = t->day;
    int hours = t->hours;
    int minutes = t->minutes;
    int seconds = t->seconds;

    // Calculate days from years since 1970
    for (int y = 1970; y < year; y++) {
        total_days += is_leap_year(y) ? 366 : 365;
    }

    // Calculate days from months in the current year
    for (int m = 1; m < month; m++) {
        total_days += days_in_month[m];
        if (m == 2 && is_leap_year(year)) { // Add an extra day for February in a leap year
            total_days++;
        }
    }

    // Add days for the current month
    total_days += (day - 1); // Subtract 1 because day is 1-indexed

    // Convert total days to seconds and add time components
    kuint64_t timestamp = total_days * 86400; // Seconds in a day
    timestamp += hours * 3600; // Seconds in an hour
    timestamp += minutes * 60; // Seconds in a minute
    timestamp += seconds;

    return timestamp;
}