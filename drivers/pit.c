// drivers/pit.c

#include <drivers/pit.h>
#include <arch/i386/io.h> // For outb, inb
#include <kernel/log.h>       // For LOG_INFO, LOG_ERR
#include <libc/strings.h>      // For memset, memcpy if needed
#include <kernel/time.h>      // For update_console_clock

// PIT I/O ports
#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_COMMAND_PORT       0x43

// PIT Mode/Command Flags
#define PIT_MODE_3             0x06 // Square Wave Mode
#define PIT_CHANNEL_0          0x00
#define PIT_ACCESS_LOBYTE_HIBYTE 0x30 // Access mode: Low byte / High byte

// PIT Oscillator Frequency (Hz)
#define PIT_BASE_FREQUENCY     1193182

// Global variables to track PIT state
static unsigned int pit_tick_count = 0;
static unsigned int pit_frequency = 0;
static unsigned int pit_divisor = 0;

// Counter for console clock updates
static unsigned int schedule_counter = 0;
static unsigned int console_clock_counter = 0;

#define CONSOLE_CLOCK_UPDATE_INTERVAL 100

int pit_init(unsigned int frequency_hz) {
    if (frequency_hz == 0 || frequency_hz > PIT_BASE_FREQUENCY) {
        LOG_ERR("Invalid frequency requested: %u Hz\n", frequency_hz);
        return -1; // Failure: Invalid frequency
    }

    pit_frequency = frequency_hz;
    pit_divisor = PIT_BASE_FREQUENCY / frequency_hz;

    // Check for potential precision loss
    if (PIT_BASE_FREQUENCY % frequency_hz != 0) {
        // The actual frequency will be slightly different
        unsigned int actual_freq = PIT_BASE_FREQUENCY / pit_divisor;
        LOG_WARN("Requested freq %d Hz, actual freq approx %d Hz due to integer division.\n",
                 frequency_hz, actual_freq);
    }

    // Prepare the command byte: Channel 0, Access Mode Low/High, Mode 3 (Square Wave)
    kuint8_t command = PIT_CHANNEL_0 | PIT_ACCESS_LOBYTE_HIBYTE | PIT_MODE_3;

    // Send the command byte to the PIT command port
    outb(PIT_COMMAND_PORT, command);

    // Send the divisor (low byte, then high byte) to Channel 0 data port
    outb(PIT_CHANNEL_0_DATA_PORT, (kuint8_t)(pit_divisor & 0xFF));         // Low byte
    outb(PIT_CHANNEL_0_DATA_PORT, (kuint8_t)((pit_divisor >> 8) & 0xFF)); // High byte

    // Reset tick counter
    pit_tick_count = 0;

    LOG_INFO("PIT initialized: Frequency=%d Hz, Divisor=%d\n", pit_frequency, pit_divisor);
    return 0; // Success
}

void pit_handler(registers_t *regs) {
    // Increment the global tick counter
    pit_tick_count++;

    // --- Console Clock Update Logic ---
    console_clock_counter++;
    if (console_clock_counter >= CONSOLE_CLOCK_UPDATE_INTERVAL) {
        console_clock_counter = 0;
        update_console_clock(); // Update the clock display every N ticks
    }

    // --- Process Scheduler Logic ---
    schedule_counter++;
    if (schedule_counter >= 10) {
        schedule_counter = 0;
        proc_scheduler_run(regs);
    }
}

unsigned int pit_get_tick_count() {
    return pit_tick_count;
}

unsigned int pit_get_frequency() {
    return pit_frequency;
}