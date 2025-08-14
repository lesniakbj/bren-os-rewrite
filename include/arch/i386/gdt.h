#ifndef ARCH_I386_GDT_H
#define ARCH_I386_GDT_H

#include <libc/stdint.h>

struct gdt_entry {
    kuint16_t limit_low;        // The lower 16 bits of the limit.
    kuint16_t base_low;         // The lower 16 bits of the base.
    kuint8_t base_middle;       // The next 8 bits of the base
    kuint8_t access;            // Access flags, determine ring level, type, etc.
    kuint8_t granularity;       // Upper 4 bits of limit, plus granularity and size flags.
    kuint8_t base_high;         // The last 8 bits of the base.
} __attribute__((packed));

struct gdt_ptr {
    kuint16_t limit;
    kuint32_t address;
} __attribute__((packed));

// Defines the structure of a Task State Segment
struct tss_entry {
    kuint32_t prev_tss;   // The previous TSS - if we used hardware task switching
    kuint32_t esp0;       // The stack pointer to load when we change to kernel mode.
    kuint32_t ss0;        // The stack segment to load when we change to kernel mode.
    kuint32_t esp1;       // Unused
    kuint32_t ss1;        // Unused
    kuint32_t esp2;       // Unused
    kuint32_t ss2;        // Unused
    kuint32_t cr3;        // Unused
    kuint32_t eip;        // Unused
    kuint32_t eflags;     // Unused
    kuint32_t eax;        // Unused
    kuint32_t ecx;        // Unused
    kuint32_t edx;        // Unused
    kuint32_t ebx;        // Unused
    kuint32_t esp;        // Unused
    kuint32_t ebp;        // Unused
    kuint32_t esi;        // Unused
    kuint32_t edi;        // Unused
    kuint32_t es;         // Unused
    kuint32_t cs;         // Unused
    kuint32_t ss;         // Unused
    kuint32_t ds;         // Unused
    kuint32_t fs;         // Unused
    kuint32_t gs;         // Unused
    kuint32_t ldt;        // Unused
    kuint16_t trap;       // Unused
    kuint16_t iomap_base; // Unused
} __attribute__((packed));

typedef struct tss_entry tss_entry_t;

#define GDT_ENTRIES 6

extern struct gdt_entry gdt_entries[GDT_ENTRIES];
extern struct gdt_ptr gdt_ptr;

void gdt_init(void);
void gdt_populate_gdt_entries(kuint16_t idx, kuint32_t segment_address, kuint32_t limit, kuint8_t access, kuint8_t granularity);

extern void gdt_load(struct gdt_ptr* gdt_ptr);

void tss_init();
void tss_set_stack(kuint32_t kernel_ss, kuint32_t kernel_esp);

#endif