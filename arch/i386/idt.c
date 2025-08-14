#include <arch/i386/idt.h>
#include <arch/i386/io.h>
#include <arch/i386/interrupts.h>

// ISR stubs defined in isr_handlers.s
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// IRQ stubs
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();

// Syscall stub
extern void isr128();

struct idt_gate_descriptor idt_entries[IDT_ENTRIES];
struct idt_ptr_entry idt_ptr;

void idt_init(void) {
    idt_ptr.limit = (sizeof(struct idt_gate_descriptor) * IDT_ENTRIES) - 1;
    idt_ptr.base = (kuint32_t)&idt_entries;

    // --- Exceptions ---
    idt_populate_idt_entries(0, (kuint32_t)isr0, 0x08, 0x8E);
    idt_populate_idt_entries(1, (kuint32_t)isr1, 0x08, 0x8E);
    idt_populate_idt_entries(2, (kuint32_t)isr2, 0x08, 0x8E);
    idt_populate_idt_entries(3, (kuint32_t)isr3, 0x08, 0x8E);
    idt_populate_idt_entries(4, (kuint32_t)isr4, 0x08, 0x8E);
    idt_populate_idt_entries(5, (kuint32_t)isr5, 0x08, 0x8E);
    idt_populate_idt_entries(6, (kuint32_t)isr6, 0x08, 0x8E);
    idt_populate_idt_entries(7, (kuint32_t)isr7, 0x08, 0x8E);
    idt_populate_idt_entries(8, (kuint32_t)isr8, 0x08, 0x8E);
    idt_populate_idt_entries(9, (kuint32_t)isr9, 0x08, 0x8E);
    idt_populate_idt_entries(10, (kuint32_t)isr10, 0x08, 0x8E);
    idt_populate_idt_entries(11, (kuint32_t)isr11, 0x08, 0x8E);
    idt_populate_idt_entries(12, (kuint32_t)isr12, 0x08, 0x8E);
    idt_populate_idt_entries(13, (kuint32_t)isr13, 0x08, 0x8E);
    idt_populate_idt_entries(14, (kuint32_t)isr14, 0x08, 0x8E);
    idt_populate_idt_entries(15, (kuint32_t)isr15, 0x08, 0x8E);
    idt_populate_idt_entries(16, (kuint32_t)isr16, 0x08, 0x8E);
    idt_populate_idt_entries(17, (kuint32_t)isr17, 0x08, 0x8E);
    idt_populate_idt_entries(18, (kuint32_t)isr18, 0x08, 0x8E);
    idt_populate_idt_entries(19, (kuint32_t)isr19, 0x08, 0x8E);
    idt_populate_idt_entries(20, (kuint32_t)isr20, 0x08, 0x8E);
    idt_populate_idt_entries(21, (kuint32_t)isr21, 0x08, 0x8E);
    idt_populate_idt_entries(22, (kuint32_t)isr22, 0x08, 0x8E);
    idt_populate_idt_entries(23, (kuint32_t)isr23, 0x08, 0x8E);
    idt_populate_idt_entries(24, (kuint32_t)isr24, 0x08, 0x8E);
    idt_populate_idt_entries(25, (kuint32_t)isr25, 0x08, 0x8E);
    idt_populate_idt_entries(26, (kuint32_t)isr26, 0x08, 0x8E);
    idt_populate_idt_entries(27, (kuint32_t)isr27, 0x08, 0x8E);
    idt_populate_idt_entries(28, (kuint32_t)isr28, 0x08, 0x8E);
    idt_populate_idt_entries(29, (kuint32_t)isr29, 0x08, 0x8E);
    idt_populate_idt_entries(30, (kuint32_t)isr30, 0x08, 0x8E);
    idt_populate_idt_entries(31, (kuint32_t)isr31, 0x08, 0x8E);

    // --- IRQs ---
    idt_populate_idt_entries(32, (kuint32_t)isr32, 0x08, 0x8E); // IRQ0
    idt_populate_idt_entries(33, (kuint32_t)isr33, 0x08, 0x8E); // IRQ1 (Keyboard)
    idt_populate_idt_entries(34, (kuint32_t)isr34, 0x08, 0x8E); // IRQ2
    idt_populate_idt_entries(35, (kuint32_t)isr35, 0x08, 0x8E); // IRQ3
    idt_populate_idt_entries(36, (kuint32_t)isr36, 0x08, 0x8E); // IRQ4
    idt_populate_idt_entries(37, (kuint32_t)isr37, 0x08, 0x8E); // IRQ5
    idt_populate_idt_entries(38, (kuint32_t)isr38, 0x08, 0x8E); // IRQ6
    idt_populate_idt_entries(39, (kuint32_t)isr39, 0x08, 0x8E); // IRQ7
    idt_populate_idt_entries(40, (kuint32_t)isr40, 0x08, 0x8E); // IRQ8
    idt_populate_idt_entries(41, (kuint32_t)isr41, 0x08, 0x8E); // IRQ9
    idt_populate_idt_entries(42, (kuint32_t)isr42, 0x08, 0x8E); // IRQ10
    idt_populate_idt_entries(43, (kuint32_t)isr43, 0x08, 0x8E); // IRQ11
    idt_populate_idt_entries(44, (kuint32_t)isr44, 0x08, 0x8E); // IRQ12
    idt_populate_idt_entries(45, (kuint32_t)isr45, 0x08, 0x8E); // IRQ13
    idt_populate_idt_entries(46, (kuint32_t)isr46, 0x08, 0x8E); // IRQ14
    idt_populate_idt_entries(47, (kuint32_t)isr47, 0x08, 0x8E); // IRQ15

    // -- Syscall --
    idt_populate_idt_entries(128, (kuint32_t)isr128, 0x08, 0xEF); // Syscalls


    idt_load(&idt_ptr);
}

void idt_populate_idt_entries(kuint16_t idx, kuint32_t isr_address, kuint16_t segment_selector, kuint8_t flags) {
    idt_entries[idx].address_low        = isr_address & 0xFFFF;
    idt_entries[idx].segment_selector   = segment_selector;
    idt_entries[idx].always_zero        = 0;
    idt_entries[idx].flags              = flags;
    idt_entries[idx].address_hi         = (isr_address >> 16) & 0xFFFF;
}