#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>
#include <drivers/keyboard_events.h>

void keyboard_init(void);
void keyboard_handler(struct registers *regs);
bool keyboard_poll(keyboard_event_t *out_event);

#endif //KEYBOARD_H