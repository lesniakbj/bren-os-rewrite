#ifndef ARCH_I386_FAULT_H
#define ARCH_I386_FAULT_H

#include <arch/i386/interrupts.h>

void general_protection_fault_handler(registers_t* regs);

#endif