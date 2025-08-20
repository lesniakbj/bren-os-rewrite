#include <arch/i386/idt.h>
#include <arch/i386/io.h>
#include <arch/i386/interrupts.h>

// ISR stubs defined in isr_handlers.s
extern void isr0(), isr1(), isr2(), isr3(), isr4(), isr5(), isr6(), isr7();
extern void isr8(), isr9(), isr10(), isr11(), isr12(), isr13(), isr14(), isr15();
extern void isr16(), isr17(), isr18(), isr19(), isr20(), isr21(), isr22(), isr23();
extern void isr24(), isr25(), isr26(), isr27(), isr28(), isr29(), isr30(), isr31();
extern void isr32(), isr33(), isr34(), isr35(), isr36(), isr37(), isr38(), isr39();
extern void isr40(), isr41(), isr42(), isr43(), isr44(), isr45(), isr46(), isr47();
extern void isr128(); // Syscall

idt_gate_descriptor_t idt_entries[IDT_ENTRIES];
idt_ptr_entry_t idt_ptr;

void idt_init(void) {
    idt_ptr.limit = (sizeof(struct idt_gate_descriptor) * IDT_ENTRIES) - 1;
    idt_ptr.base = (kuint32_t)&idt_entries;

    // Create a table of our ISR stubs.
    void* isr_stub_table[] = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
        isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
        isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
    };

    // Loop through the table to populate the IDT for exceptions and IRQs.
    kuint16_t num_stubs = sizeof(isr_stub_table) / sizeof(void*);
    for (kuint16_t i = 0; i < num_stubs; i++) {
        idt_populate_idt_entries(i, (kuint32_t)isr_stub_table[i], 0x08, 0x8E);
    }

    // -- Syscall --
    idt_populate_idt_entries(128, (kuint32_t)isr128, 0x08, IDT_PRESENT | IDT_DPL3 | IDT_INT32);

    idt_load(&idt_ptr);
}

void idt_populate_idt_entries(kuint16_t idx, kuint32_t isr_address, kuint16_t segment_selector, kuint8_t flags) {
    idt_entries[idx].address_low        = isr_address & 0xFFFF;
    idt_entries[idx].segment_selector   = segment_selector;
    idt_entries[idx].always_zero        = 0;
    idt_entries[idx].flags              = flags;
    idt_entries[idx].address_hi         = (isr_address >> 16) & 0xFFFF;
}
