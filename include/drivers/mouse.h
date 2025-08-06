#ifndef DRIVERS_MOUSE_H
#define DRIVERS_MOUSE_H

#include <arch/i386/interrupts.h>

// Initializes the mouse driver.
void mouse_init(void);

// The interrupt handler for the mouse.
void mouse_handler(struct registers *regs);

#endif // DRIVERS_MOUSE_H
