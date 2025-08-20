#ifndef ARCH_I386_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_H

#include <libc/stdint.h>

typedef struct registers {
    // Pushed by pusha
    kuint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // Pushed manually in isr_common_stub
    kuint32_t ds, es, fs, gs;
    // Pushed by the ISR stub
    kuint32_t interrupt_number, error_code;
    // Pushed by the CPU on interrupt
    kuint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) registers_t;

typedef void (*interrupt_handler_t)(registers_t*);

// Function pointers to interrupt handlers, register handler, etc
void register_interrupt_handler(kuint8_t n, interrupt_handler_t handler);

// Interrupt flow isr_common -> isr_handler
extern void isr_common_stub();
extern void isr_handler_c(registers_t *regs);

// Proc context switching is handled via interrupts
extern void context_switch(kuint32_t new_esp);

#endif