#include <arch/i386/gdt.h>

extern void tss_flush();
extern kuint8_t stack_bottom[];
extern kuint8_t stack_top[];

gdt_entry_t gdt_entries[GDT_ENTRIES];
gdt_ptr_t gdt_ptr;
tss_entry_t tss_entry;

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_ENTRIES - 1;
    gdt_ptr.address = (physical_addr_t)&gdt_entries;

    gdt_populate_gdt_entries(0, 0, 0, 0, 0);                // Null segment
    gdt_populate_gdt_entries(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdt_populate_gdt_entries(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_populate_gdt_entries(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_populate_gdt_entries(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    // TSS Entry
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0  = 0x10; // kernel data segment
    tss_entry.esp0 = (physical_addr_t)stack_top; // physical address for TSS stack
    physical_addr_t tss_base = (physical_addr_t)&tss_entry;
    size_t tss_limit = sizeof(tss_entry);
    gdt_populate_gdt_entries(5, tss_base, tss_limit, 0x89, 0x00); // TSS descriptor

    gdt_load(&gdt_ptr);
    tss_flush();
}

void gdt_populate_gdt_entries(kuint32_t idx, physical_addr_t segment_address, size_t limit, kuint8_t access, kuint8_t granularity) {
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
    tss_entry.ss0  = 0x10; // kernel data segment
    tss_entry.esp0 = (physical_addr_t)stack_top;
    tss_flush();
}

// This function will be called by the scheduler on a context switch
void tss_set_stack(kuint32_t kernel_ss, kuint32_t kernel_esp) {
    tss_entry.ss0  = kernel_ss;
    tss_entry.esp0 = kernel_esp;
}