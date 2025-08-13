#include <kernel/debug.h>
#include <kernel/proc.h>
#include <drivers/terminal.h>

#ifdef DEBUG

void test_proc_1() {
    while(1) {
        LOG_DEBUG("A\n");
        for(volatile int i = 0; i < 10000000; i++); // Delay loop
        // Yield to scheduler
        asm volatile("int $0x20"); // Trigger timer interrupt to yield
    }
}

void test_proc_2() {
    while(1) {
        LOG_DEBUG("B\n");
        for(volatile int i = 0; i < 10000000; i++); // Delay loop
        // Yield to scheduler
        asm volatile("int $0x20"); // Trigger timer interrupt to yield
    }
}

void debug_proc_test() {
    LOG_DEBUG("Initializing scheduler test...\n");
    if(proc_init() == 0) {
        proc_create(test_proc_1, true);
        proc_create(test_proc_2, true);
    }
    LOG_DEBUG("Test processes created.\n");
}

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

void test_heap_allocations() {
    LOG_INFO("Testing heap allocation...\n");

    // Print initial heap statistics
    LOG_INFO("Initial heap stats - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    generic_ptr test_alloc = kmalloc(128);
    if (test_alloc) {
        LOG_INFO("Heap allocation successful! Address: 0x%x, Size: %d\n", test_alloc, 128);
    }

    // Print heap statistics after first allocation
    LOG_INFO("After first allocation - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    generic_ptr null_realloc = krealloc(NULL, 256);
    if (null_realloc) {
        LOG_INFO("krealloc(NULL, 256) successful! Address: 0x%x\n", null_realloc);
        char* data_ptr = (char*)null_realloc;
        for(int i = 0; i < 10; i++) {
            data_ptr[i] = 'A' + i;
        }
        LOG_INFO("Data written to null_realloc block\n");
    }

    // Print heap statistics after second allocation
    LOG_INFO("After second allocation - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    if (test_alloc) {
        generic_ptr zero_realloc = krealloc(test_alloc, 0);
        if (zero_realloc == NULL) {
            LOG_INFO("krealloc(ptr, 0) correctly returned NULL and freed memory\n");
        }
    }

    // Print heap statistics after freeing
    LOG_INFO("After freeing - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    generic_ptr expand_test = kmalloc(64);
    if (expand_test) {
        LOG_INFO("Initial block for expansion test: 0x%x, Size: %d\n", expand_test, 64);

        char* data_ptr = (char*)expand_test;
        for(int i = 0; i < 32; i++) {
            data_ptr[i] = 'X' + (i % 26);
        }
        LOG_INFO("Data written to expand_test block\n");

        generic_ptr expanded = krealloc(expand_test, 512);
        if (expanded && expanded != expand_test) {
            LOG_INFO("krealloc expanded block! Old: 0x%x, New: 0x%x, Size: %d\n", expand_test, expanded, 512);

            char* new_data_ptr = (char*)expanded;
            bool data_ok = true;
            for(int i = 0; i < 32; i++) {
                if (new_data_ptr[i] != ('X' + (i % 26))) {
                    data_ok = false;
                    break;
                }
            }

            if (data_ok) {
                LOG_INFO("Data successfully copied to new block!\n");
            } else {
                LOG_ERR("Data corruption detected!\n");
            }

            expand_test = expanded;
        } else if (expanded == expand_test) {
            LOG_INFO("Block was shrunk in place (no reallocation needed)\n");
        } else {
            LOG_ERR("Expansion failed!\n");
        }
    }

    // Print heap statistics after expansion test
    LOG_INFO("After expansion test - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    if (expand_test) {
        generic_ptr shrunk = krealloc(expand_test, 32);
        if (shrunk) {
            LOG_INFO("krealloc shrunk block! Result: 0x%x, Size: %d\n", shrunk, 32);
            expand_test = shrunk;
        } else {
            LOG_ERR("Shrinking failed!\n");
        }
    }

    // Print heap statistics after shrinking
    LOG_INFO("After shrinking - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    // Test heap expansion explicitly
    LOG_INFO("Testing explicit heap expansion...\n");
    size_t size_before_expansion = heap_get_total_size();
    if (heap_expand(0x20000)) { // Expand by 128KB
        LOG_INFO("Heap expansion successful! Size increased from %d to %d bytes\n",
                              size_before_expansion, heap_get_total_size());
    } else {
        LOG_ERR("Heap expansion failed!\n");
    }

    // Print final heap statistics
    LOG_INFO("Final heap stats - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    if (null_realloc) kfree(null_realloc);
    if (expand_test) kfree(expand_test);

    // Print heap statistics after all freeing
    LOG_INFO("After all freeing - Total: %d bytes, Used: %d bytes, Free: %d bytes\n",
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    LOG_INFO("Heap testing completed!\n");
}
#endif