#include <kernel.h>
#include <multiboot.h>
#include <idt.h>
#include <gdt.h>
#include <pic.h>
#include <terminal.h>
#include <screen.h>
#include <keyboard.h>
#include <interrupts.h>

void debug_gdt();
void debug_pic();
void debug_idt();
void debug_multiboot_header(multiboot_info_t *mbi);

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    multiboot_info_t *mbi;

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) multiboot_addr;

    /* Init screen so we can show any info to the user */
    screen_init(mbi);

    /* Am I booted by a Multiboot-compliant boot loader? */
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        terminal_writestring("Invalid magic number: 0x");
        terminal_writestring("\n");
        return;
    }

    // TODO: Figure out boot order:
    //       Right now I have:
    //          1) Bootloader call main
    //          2) Init screen for debugging
    //          3) Init GDT
    //          4) Remap PIC
    //          5) Init IDT
    //          6) Enable Interrupts
    //
    //       Somewhere in here I need to add the ability to initialize my memory manager, probably after 2 before 3...
    //       The physical memory allocator should parse the mmap from mbi and identify RAM/Memory regions.
    //       Virtual memory manager then fills page directories and tables, maps code and data of the kernel and then enables paging
    //       Then, allocate a large hunk of memory for the Kernel heap. Have the heap allocator use this region (kmalloc/kfree will use this)


    /* Initialization sequence */
    terminal_writestring("Kernel Initializing...\n");
    // terminal_writestring("\n");

    /* Initialize the GDT */
    gdt_init();
    // debug_gdt(); // Temporarily commented out for debugging 0x00 interrupt
    // terminal_writestring("\n");

    /* Remap the PIC */
    pic_remap(0x20, 0x28);
    // debug_pic(); // Temporarily commented out for debugging 0x00 interrupt
    // terminal_writestring("\n");

    /* Initialize the IDT */
    idt_init();
    debug_idt(); // Temporarily commented out for debugging 0x00 interrupt

    /* Initialize the keyboard */
    keyboard_init();
    register_interrupt_handler(0x21, keyboard_handler);

    /* Debug multiboot headers */
    // debug_multiboot_header(mbi); // Temporarily commented out for debugging 0x00 interrupt
    // terminal_writestring("\n");

    // TODO: Debug the GDT/PIC/IDT to figure out why interrupts for keyboards aren't working correctly yet....
    // terminal_writestring("Enabling interrupts...\n");
    enable_interrupts();
    // terminal_writestring("Interrupts enabled.\n");

    //terminal_writestring("Hello World from the terminal!\n");
    // terminal_writestring("\n");

}

void debug_pic() {
    char buf[33];
    terminal_writestring("PIC Remapped.\n");
    terminal_writestring("  PIC Master IMR: 0x");
    itoa(buf, 'x', pic_read_data_port(0x21));
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  PIC Slave IMR: 0x");
    itoa(buf, 'x', pic_read_data_port(0xA1));
    terminal_writestring(buf);
    terminal_writestring("\n");
}

void debug_gdt() {
    char buf[33];

    terminal_writestring("GDT initialized.\n");
    terminal_writestring("  gdt_ptr.limit: 0x");
    itoa(buf, 'x', gdt_ptr.limit);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  gdt_ptr.address: 0x");
    itoa(buf, 'x', gdt_ptr.address);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  sizeof(gdt_entries): ");
    itoa(buf, 'd', sizeof(gdt_entries));
    terminal_writestring(buf);
    terminal_writestring(" bytes");

    terminal_writestring("  Address of gdt_ptr: 0x");
    itoa(buf, 'x', (kuint32_t)&gdt_ptr);
    terminal_writestring(buf);
    terminal_writestring("\n");

    for (kint16_t i = 0; i < 5; i++) {
        struct gdt_entry *entry = &gdt_entries[i];
        kuint32_t base = (entry->base_high << 24) | (entry->base_middle << 16) | entry->base_low;
        kuint32_t limit = ((entry->granularity & 0xF0) << 12) | entry->limit_low;

        terminal_writestringf("  GDT Entry %d:\n", i);
        terminal_writestringf("    Base: 0x%x, Limit: 0x%x\n", base, limit);
        terminal_writestringf("    Access Byte: 0x%x (Present: %d, DPL: %d, Type: %d, Executable: %d, DC: %d, RW: %d, Accessed: %d)\n",
                entry->access,
                (entry->access >> 7) & 0x1, // Present bit
                (entry->access >> 5) & 0x3, // DPL (Descriptor Privilege Level)
                (entry->access >> 4) & 0x1, // Type (0 for data/code, 1 for system)
                (entry->access >> 3) & 0x1, // Executable (1 for code, 0 for data)
                (entry->access >> 2) & 0x1, // Direction/Conforming (data: expand-down/normal, code: conforming/non-conforming)
                (entry->access >> 1) & 0x1, // Readable/Writable (code: readable, data: writable)
                entry->access & 0x1);       // Accessed bit
        terminal_writestringf("    Granularity Byte: 0x%x (Granularity: %d, Size: %d, Long Mode: %d)\n",
                entry->granularity,
                (entry->granularity >> 7) & 0x1, // Granularity (0=byte, 1=4KB page)
                (entry->granularity >> 6) & 0x1, // Size (0=16-bit, 1=32-bit)
                (entry->granularity >> 5) & 0x1); // Long Mode (1=64-bit code segment)
    }
    terminal_writestring("\n");
}

void debug_idt() {
    char buf[33];

    terminal_writestring("IDT initialized.\n");
    terminal_writestring("  idt_ptr.limit: 0x");
    itoa(buf, 'x', idt_ptr.limit);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  idt_ptr.base: 0x");
    itoa(buf, 'x', idt_ptr.base);
    terminal_writestring(buf);
    terminal_writestring("\n");

    terminal_writestring("  sizeof(idt_entries): ");
    itoa(buf, 'd', sizeof(idt_entries));
    terminal_writestring(buf);
    terminal_writestring(" bytes");

    terminal_writestring("  Address of idt_ptr: 0x");
    itoa(buf, 'x', (kuint32_t)&idt_ptr);
    terminal_writestring(buf);
    terminal_writestring("\n");
}

void debug_multiboot_header(multiboot_info_t *mbi) {
    char buf[33];

    /* Print out the flags. */
    terminal_writestring("MultiBoot Header.\n");
    terminal_writestring("flags = 0x");
    itoa(buf, 'x', mbi->flags);
    terminal_writestring(buf);
    terminal_writestring("\n");

    /* Are mem_* valid? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 0)) {
        terminal_writestring("mem_lower = ");
        itoa(buf, 'd', mbi->mem_lower);
        terminal_writestring(buf);
        terminal_writestring("KB, mem_upper = ");
        itoa(buf, 'd', mbi->mem_upper);
        terminal_writestring(buf);
        terminal_writestring("KB\n");
    }

    /* Is boot_device valid? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 1)) {
        terminal_writestring("boot_device = 0x");
        itoa(buf, 'x', mbi->boot_device);
        terminal_writestring(buf);
        terminal_writestring("\n");
    }

    /* Is the command line passed? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 2)) {
        terminal_writestring("cmdline = ");
        terminal_writestring((char*)mbi->cmdline);
        terminal_writestring("\n");
    }

    /* Are mods_* valid? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 3)) {
        multiboot_module_t *mod;
        kuint32_t i;

        terminal_writestring("mods_count = ");
        itoa(buf, 'd', mbi->mods_count);
        terminal_writestring(buf);
        terminal_writestring(", mods_addr = 0x");
        itoa(buf, 'x', mbi->mods_addr);
        terminal_writestring(buf);
        terminal_writestring("\n");

        for(i = 0, mod =(multiboot_module_t *) mbi->mods_addr; i < mbi->mods_count; i++, mod++) {
            terminal_writestring(" mod_start = 0x");
            itoa(buf, 'x', mod->mod_start);
            terminal_writestring(buf);
            terminal_writestring(", mod_end = 0x");
            itoa(buf, 'x', mod->mod_end);
            terminal_writestring(buf);
            terminal_writestring(", cmdline = ");
            terminal_writestring((char*)mod->cmdline);
            terminal_writestring("\n");
        }
    }

    /* Bits 4 and 5 are mutually exclusive! */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 4) && CHECK_MULTIBOOT_FLAG(mbi->flags, 5)) {
        terminal_writestring("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the symbol table of a.out valid? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 4)) {
        multiboot_aout_symbol_table_t *multiboot_aout_sym = &(mbi->u.aout_sym);
        terminal_writestring("multiboot_aout_symbol_table: tabsize = 0x");
        itoa(buf, 'x', multiboot_aout_sym->tabsize);
        terminal_writestring(buf);
        terminal_writestring(", strsize = 0x");
        itoa(buf, 'x', multiboot_aout_sym->strsize);
        terminal_writestring(buf);
        terminal_writestring(", addr = 0x");
        itoa(buf, 'x', multiboot_aout_sym->addr);
        terminal_writestring(buf);
        terminal_writestring("\n");
    }

    /* Is the section header table of ELF valid? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 5)) {
        multiboot_elf_section_header_table_t *multiboot_elf_sec = &(mbi->u.elf_sec);
        terminal_writestring("multiboot_elf_sec: num = ");
        itoa(buf, 'd', multiboot_elf_sec->num);
        terminal_writestring(buf);
        terminal_writestring(", size = 0x");
        itoa(buf, 'x', multiboot_elf_sec->size);
        terminal_writestring(buf);
        terminal_writestring(", addr = 0x");
        itoa(buf, 'x', multiboot_elf_sec->addr);
        terminal_writestring(buf);
        terminal_writestring(", shndx = 0x");
        itoa(buf, 'x', multiboot_elf_sec->shndx);
        terminal_writestring(buf);
        terminal_writestring("\n");
    }

    /* Are mmap_* valid? */
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 6)) {
        multiboot_memory_map_t *mmap;
        terminal_writestring("mmap_addr = 0x");
        itoa(buf, 'x', mbi->mmap_addr);
        terminal_writestring(buf);
        terminal_writestring(", mmap_length = 0x");
        itoa(buf, 'x', mbi->mmap_length);
        terminal_writestring(buf);
        terminal_writestring("\n");

        for(mmap =(multiboot_memory_map_t *) mbi->mmap_addr;
          (unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
           mmap =(multiboot_memory_map_t *)((unsigned long) mmap
                                    + mmap->size + sizeof(mmap->size))) {
            terminal_writestring(" size = 0x");
            itoa(buf, 'x', mmap->size);
            terminal_writestring(buf);
            terminal_writestring(", base_addr = 0x");
            itoa(buf, 'x', (mmap->addr >> 32));
            terminal_writestring(buf);
            itoa(buf, 'x', (mmap->addr & 0xffffffff));
            terminal_writestring(buf);
            terminal_writestring(", length = 0x");
            itoa(buf, 'x', (mmap->len >> 32));
            terminal_writestring(buf);
            itoa(buf, 'x', (mmap->len & 0xffffffff));
            terminal_writestring(buf);
            terminal_writestring(", type = 0x");
            itoa(buf, 'x', mmap->type);
            terminal_writestring(buf);
            terminal_writestring("\n");
        }
    }
}