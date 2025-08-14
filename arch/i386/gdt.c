#include <arch/i386/gdt.h>

extern void tss_flush();

struct gdt_entry gdt_entries[GDT_ENTRIES];
struct gdt_ptr gdt_ptr;
tss_entry_t tss_entry;

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_ptr.address = (kuint32_t)&gdt_entries;

    gdt_populate_gdt_entries(0, 0, 0, 0, 0);                // Null segment
    gdt_populate_gdt_entries(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_populate_gdt_entries(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_populate_gdt_entries(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_populate_gdt_entries(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    // TSS Entry
    kuint32_t tss_base = (kuint32_t)&tss_entry;
    kuint32_t tss_limit = sizeof(tss_entry);
    gdt_populate_gdt_entries(5, tss_base, tss_limit, 0x89, 0x00); // TSS

    gdt_load(&gdt_ptr);
}

void gdt_populate_gdt_entries(kuint16_t idx, kuint32_t segment_address, kuint32_t limit, kuint8_t access, kuint8_t granularity) {
    gdt_entries[idx].base_low = segment_address & 0xFFFF;
    gdt_entries[idx].base_middle = (segment_address >> 16) & 0xFF;
    gdt_entries[idx].base_high = (segment_address >> 24) & 0xFF;

    gdt_entries[idx].limit_low = (limit & 0xFFFF);
    gdt_entries[idx].granularity = (limit >> 16) & 0x0F;

    gdt_entries[idx].granularity |= granularity & 0xF0;
    gdt_entries[idx].access = access;
}

void tss_init() {
    memset(&tss_entry, 0, sizeof(tss_entry));

    // Set the kernel stack segment selector
    tss_entry.ss0 = 0x10; // Kernel Data Segment selector
    // esp0 will be updated on each context switch
    tss_entry.esp0 = 0;

    // Load the TSS selector (0x28) into the Task Register
    tss_flush();
}

// This function will be called by the scheduler on a context switch
void tss_set_stack(kuint32_t kernel_ss, kuint32_t kernel_esp) {
    tss_entry.ss0 = kernel_ss;
    tss_entry.esp0 = kernel_esp;
}