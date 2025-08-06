#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>

void keyboard_init(void);
void keyboard_handler(struct registers *regs);

#endif //KEYBOARD_H