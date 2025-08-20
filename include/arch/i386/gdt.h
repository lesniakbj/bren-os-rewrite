#ifndef ARCH_I386_GDT_H
#define ARCH_I386_GDT_H

#define GDT_ENTRIES 6

#include <libc/stdint.h>

typedef struct gdt_entry {
    kuint16_t limit_low;        // The lower 16 bits of the limit.
    kuint16_t base_low;         // The lower 16 bits of the base.
    kuint8_t base_middle;       // The next 8 bits of the base
    kuint8_t access;            // Access flags, determine ring level, type, etc.
    kuint8_t granularity;       // Upper 4 bits of limit, plus granularity and size flags.
    kuint8_t base_high;         // The last 8 bits of the base.
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_ptr {
    kuint16_t limit;
    kuint32_t address;
} __attribute__((packed)) gdt_ptr_t;

// Defines the structure of a Task State Segment
typedef struct tss_entry {
    kuint32_t prev_tss;   // The previous TSS - if we used hardware task switching
    kuint32_t esp0;       // The stack pointer to load when we change to kernel mode.
    kuint32_t ss0;        // The stack segment to load when we change to kernel mode.
    // All below here are unused when we use Software Multitasking
    kuint32_t esp1;
    kuint32_t ss1;
    kuint32_t esp2;
    kuint32_t ss2;
    kuint32_t cr3;
    kuint32_t eip;
    kuint32_t eflags;
    kuint32_t eax;
    kuint32_t ecx;
    kuint32_t edx;
    kuint32_t ebx;
    kuint32_t esp;
    kuint32_t ebp;
    kuint32_t esi;
    kuint32_t edi;
    kuint32_t es;
    kuint32_t cs;
    kuint32_t ss;
    kuint32_t ds;
    kuint32_t fs;
    kuint32_t gs;
    kuint32_t ldt;
    kuint16_t trap;
    kuint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init();
void gdt_populate_gdt_entries(kuint32_t idx, physical_addr_t segment_address, size_t limit, kuint8_t access, kuint8_t granularity);
extern void gdt_load(gdt_ptr_t* gdt_ptr);

void tss_init();
void tss_set_stack(kuint32_t kernel_ss, kuint32_t kernel_esp);
extern void tss_flush();

extern gdt_entry_t gdt_entries[GDT_ENTRIES];
extern gdt_ptr_t gdt_ptr;
extern tss_entry_t tss_entry;

#endif