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

#define GDT_ENTRIES 5
extern struct gdt_entry gdt_entries[GDT_ENTRIES];
extern struct gdt_ptr gdt_ptr;

void gdt_init(void);
void gdt_populate_gdt_entries(kuint16_t idx, kuint32_t segment_address, kuint32_t limit, kuint8_t access, kuint8_t granularity);

extern void gdt_load(struct gdt_ptr* gdt_ptr);

#endif