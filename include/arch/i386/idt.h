#ifndef ARCH_I386_IDT_H
#define ARCH_I386_IDT_H

#include <libc/stdint.h>
#include <libc/stdlib.h>

struct idt_gate_descriptor {
    kuint16_t address_low;      // Bits 0-15 of the 32 bit address to the ISR
    kuint16_t segment_selector; // Points to a GDT entry, tells CPU which memory segment the ISR is in
    kuint8_t  always_zero;      // Always 0
    kuint8_t  flags;            // GateType: 0-3, S: 4, DPL: 6-5, P: 7
    kuint16_t address_hi;       // Bits 16-32 of the 32 bit address to the ISR
} __attribute__((packed));

struct idt_ptr_entry {
    kuint16_t limit;            // Size of IDT in bytes, minus 1
    kuint32_t base;             // Linear address where IDT starts
} __attribute__((packed));

#define IDT_ENTRIES 256
extern struct idt_gate_descriptor idt_entries[IDT_ENTRIES];
extern struct idt_ptr_entry idt_ptr;

void idt_init(void);
void idt_populate_idt_entries(kuint16_t idx, kuint32_t isr_address, kuint16_t segment_selector, kuint8_t flags);

extern void idt_load(struct idt_ptr_entry* idt_ptr);

#endif