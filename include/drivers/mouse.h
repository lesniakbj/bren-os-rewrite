#ifndef DRIVERS_MOUSE_H
#define DRIVERS_MOUSE_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>

typedef struct {
    kint8_t buttons_pressed;
    kint8_t x_delta;
    kint8_t y_delta;
} mouse_event_t;

// Initializes the mouse driver.
void mouse_init(void);

// The interrupt handler for the mouse.
void mouse_handler(struct registers *regs);

// Poll for the next event in the queue
int mouse_poll(mouse_event_t* event_out);

#endif // DRIVERS_MOUSE_H
