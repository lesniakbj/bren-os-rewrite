#include <kernel/debug.h>

#ifdef DEBUG
void debug_time(CMOS_Time current_time) {
    LOG_DEBUG("Current Time: %d/%d/%d %d:%d:%d\n",
                            current_time.month,
                            current_time.day,
                            current_time.year,
                            current_time.hours,
                            current_time.minutes,
                            current_time.seconds);
    LOG_DEBUG("Unix timestamp: %d\n", time_to_unix_seconds(&current_time));
}

void debug_pic() {
    LOG_DEBUG("PIC Remapped.\n");
    LOG_DEBUG("\tPIC Master IMR: 0x%x\n", pic_read_data_port(0x21));
    LOG_DEBUG("\tPIC Slave IMR: 0x%x\n", pic_read_data_port(0xA1));
}

void debug_gdt() {
    LOG_DEBUG("GDT initialized.\n");
    LOG_DEBUG("\tgdt_ptr.limit: 0x%x\n", gdt_ptr.limit);
    LOG_DEBUG("\tgdt_ptr.address: 0x%x\n", gdt_ptr.address);
    LOG_DEBUG("\tsizeof(gdt_entries): %d bytes\n", sizeof(gdt_entries));
    LOG_DEBUG("\tAddress of gdt_ptr: 0x%x\n", (kuint32_t)&gdt_ptr);

    for (kint16_t i = 0; i < 5; i++) {
        struct gdt_entry *entry = &gdt_entries[i];
        kuint32_t base = (entry->base_high << 24) | (entry->base_middle << 16) | entry->base_low;
        kuint32_t limit = ((entry->granularity & 0xF0) << 12) | entry->limit_low;

        LOG_DEBUG("\tGDT Entry %d:\n", i);
        LOG_DEBUG("\t\tBase: 0x%x, Limit: 0x%x\n", base, limit);
        LOG_DEBUG("\t\tAccess Byte: 0x%x (Present: %d, DPL: %d, Type: %d, Executable: %d, DC: %d, RW: %d, Accessed: %d)\n",
                entry->access,
                (entry->access >> 7) & 0x1, // Present bit
                (entry->access >> 5) & 0x3, // DPL (Descriptor Privilege Level)
                (entry->access >> 4) & 0x1, // Type (0 for data/code, 1 for system)
                (entry->access >> 3) & 0x1, // Executable (1 for code, 0 for data)
                (entry->access >> 2) & 0x1, // Direction/Conforming (data: expand-down/normal, code: conforming/non-conforming)
                (entry->access >> 1) & 0x1, // Readable/Writable (code: readable, data: writable)
                entry->access & 0x1);       // Accessed bit
        LOG_DEBUG("\t\tGranularity Byte: 0x%x (Granularity: %d, Size: %d, Long Mode: %d)\n",
                entry->granularity,
                (entry->granularity >> 7) & 0x1, // Granularity (0=byte, 1=4KB page)
                (entry->granularity >> 6) & 0x1, // Size (0=16-bit, 1=32-bit)
                (entry->granularity >> 5) & 0x1); // Long Mode (1=64-bit code segment)
    }
}

void debug_idt() {
    LOG_DEBUG("IDT initialized.\n");
    LOG_DEBUG("\tidt_ptr.limit: 0x%x\n", idt_ptr.limit);
    LOG_DEBUG("\tidt_ptr.base: 0x%x\n", idt_ptr.base);
    LOG_DEBUG("\tsizeof(idt_entries): %d bytes\n", sizeof(idt_entries));
    LOG_DEBUG("\tAddress of idt_ptr: 0x%x\n", (kuint32_t)&idt_ptr);
}

void debug_multiboot_header(multiboot_info_t *mbi) {
    // Print out the flags.
    LOG_DEBUG("MultiBoot Header.\n");
    LOG_DEBUG("flags = 0x%x\n", mbi->flags);

   // Are mem_* valid?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 0)) {
        LOG_DEBUG("mem_lower = %dKB, mem_upper = %dKB\n", mbi->mem_lower, mbi->mem_upper);
    }

    // Is boot_device valid?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 1)) {
        LOG_DEBUG("boot_device = 0x%x\n", mbi->boot_device);
    }

    // Is the command line passed?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 2)) {
        LOG_DEBUG("cmdline = %s\n", (char*)mbi->cmdline);
    }

    // Are mods_* valid?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 3)) {
        multiboot_module_t *mod;
        kuint32_t i;

        LOG_DEBUG("mods_count = %d, mods_addr = 0x%x\n", mbi->mods_count, mbi->mods_addr);

        for(i = 0, mod =(multiboot_module_t *) mbi->mods_addr; i < mbi->mods_count; i++, mod++) {
            LOG_DEBUG("\tmod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n",
                      mod->mod_start, mod->mod_end, (char*)mod->cmdline);
        }
    }

    // Bits 4 and 5 are mutually exclusive!
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 4) && CHECK_MULTIBOOT_FLAG(mbi->flags, 5)) {
        LOG_DEBUG("Both bits 4 and 5 are set.\n");
        return;
    }

    // Is the symbol table of a.out valid?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 4)) {
        multiboot_aout_symbol_table_t *multiboot_aout_sym = &(mbi->u.aout_sym);
        LOG_DEBUG("multiboot_aout_symbol_table: tabsize = 0x%x, strsize = 0x%x, addr = 0x%x\n",
                  multiboot_aout_sym->tabsize, multiboot_aout_sym->strsize, multiboot_aout_sym->addr);
    }

    // Is the section header table of ELF valid?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 5)) {
        multiboot_elf_section_header_table_t *multiboot_elf_sec = &(mbi->u.elf_sec);
        LOG_DEBUG("multiboot_elf_sec: num = %d, size = 0x%x, addr = 0x%x, shndx = 0x%x\n",
                  multiboot_elf_sec->num, multiboot_elf_sec->size, multiboot_elf_sec->addr, multiboot_elf_sec->shndx);
    }

    // Are mmap_* valid?
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 6)) {
        multiboot_memory_map_t *mmap;
        LOG_DEBUG("mmap_addr = 0x%x, mmap_length = 0x%x\n", mbi->mmap_addr, mbi->mmap_length);

        for(mmap =(multiboot_memory_map_t *) mbi->mmap_addr;
          (unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
           mmap =(multiboot_memory_map_t *)((unsigned long) mmap
                                    + mmap->size + sizeof(mmap->size))) {
            LOG_DEBUG("\tsize = 0x%x, base_addr = 0x%x, length = 0x%x, type = 0x%x\n",
                      mmap->size, mmap->addr, mmap->len, mmap->type);
        }
    }
}

void debug_pmm(pmm_init_status_t *pmm_status) {
    if (pmm_status->error) {
        LOG_DEBUG("Error: Could not find a suitable location for the memory map!\n");
        return;
    }

    LOG_DEBUG("Total memory: %d KB (%d blocks)\n", pmm_status->total_memory_kb, pmm_status->max_blocks);
    LOG_DEBUG("Required bitmap size: %d bytes\n", pmm_status->bitmap_size);
    LOG_DEBUG("Kernel ends at: 0x%x\n", pmm_status->kernel_end);
    LOG_DEBUG("Placing bitmap at: 0x%x\n", pmm_status->placement_address);
    LOG_DEBUG("%d blocks used initially.\n", pmm_status->used_blocks);
}
#endif