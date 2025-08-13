#ifndef DRIVERS_PIT_H
#define DRIVERS_PIT_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h> // For registers_t

/**
 * @brief Initializes the Programmable Interval Timer (PIT).
 *
 * Configures the PIT Channel 0 to operate in Mode 3 (Square Wave)
 * and sets it to generate interrupts at the specified frequency.
 *
 * @param frequency_hz The desired interrupt frequency in Hertz.
 *                     Common values are 100 (100Hz), 1000 (1kHz).
 * @return 0 on success, -1 on failure (e.g., invalid frequency).
 */
int pit_init(unsigned int frequency_hz);

/**
 * @brief Interrupt Service Routine (ISR) for the PIT.
 *
 * This function is called by the interrupt controller when
 * the PIT generates an interrupt (typically IRQ 0).
 * It handles the timer tick, e.g., updating system time,
 * calling the scheduler.
 *
 * @param regs Pointer to the CPU registers at the time of the interrupt.
 */
void pit_handler(registers_t *regs);

/**
 * @brief Gets the current system tick count.
 *
 * Returns the number of timer ticks that have elapsed since
 * the PIT was initialized.
 *
 * @return The current tick count.
 */
unsigned int pit_get_tick_count();

#endif // DRIVERS_PIT_H