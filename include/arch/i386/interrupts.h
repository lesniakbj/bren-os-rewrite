#ifndef ARCH_I386_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_H

#include <libc/stdint.h>

// TODO: Verify the common ISR handler is being called with these parameters in this order
struct registers {
    kuint32_t gs, fs, es, ds;
    kuint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    kuint32_t interrupt_number, error_code;
    kuint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

typedef struct registers registers_t;

extern void isr_common_stub();

// Extern to be called from asm
typedef void (*interrupt_handler_t)(struct registers*);
void register_interrupt_handler(kuint8_t n, interrupt_handler_t handler);

// Extern to be called from asm
extern void isr_handler_c(struct registers *regs);

#endif