#include <interrupts.h>
#include <io.h>

interrupt_handler_t interrupt_handlers[256];

void register_interrupt_handler(kuint8_t n, interrupt_handler_t handler) {
    interrupt_handlers[n] = handler;
}

// Generic C handler for all interrupts and exceptions
void isr_handler_c(struct registers *regs) {
    if (interrupt_handlers[regs->interrupt_number] != 0) {
        interrupt_handlers[regs->interrupt_number](regs);
    } else {
        // Default handler if no specific handler is registered
    }

    // For IRQs, we need to send an End-of-Interrupt (EOI) to the PIC
    if (regs->interrupt_number >= 32 && regs->interrupt_number <= 47) {
        outb(0x20, 0x20); // EOI for master PIC
        if (regs->interrupt_number >= 40) {
            outb(0xA0, 0x20); // EOI for slave PIC
        }
    }
}
