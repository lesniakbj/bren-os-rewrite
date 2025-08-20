#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_BUFFER_SIZE 256

#define KEYBOARD_READ_PORT 0x60

#define KEYBOARD_CMD_ACK_CODE 0xFA
#define KEYBOARD_EXTENDED_CODE 0xE0

#define KEYBOARD_RELEASE_MASK 0x80

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>
#include <drivers/keyboard_events.h>

void keyboard_init();
void keyboard_handler(registers_t *regs);
bool keyboard_poll(keyboard_event_t *out_event);

#endif //KEYBOARD_H