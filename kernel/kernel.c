#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <arch/i386/idt.h>
#include <arch/i386/gdt.h>
#include <arch/i386/pic.h>
#include <arch/i386/interrupts.h>
#include <arch/i386/time.h>
#include <drivers/terminal.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/keyboard_events.h>

#include <arch/i386/memory.h>

#ifdef DEBUG
void debug_gdt();
void debug_pic();
void debug_idt();
void debug_multiboot_header(multiboot_info_t *mbi);
#endif

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    multiboot_info_t *mbi;

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) multiboot_addr;

    /* Initialize our system clock based off of the RTC */
    CMOS_Time current_time = time_init();

    /* Init screen so we can show any info to the user */
    screen_init(mbi);

    /* Am I booted by a Multiboot-compliant boot loader? */
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        terminal_writestring("Invalid magic number: 0x");
        terminal_writestring("\n");
        return;
    }

    /* Initialization sequence */
    terminal_writestringf("Current Time: %d/%d/%d %d:%d:%d\n",
                            current_time.month,
                            current_time.day,
                            current_time.year,
                            current_time.hours,
                            current_time.minutes,
                            current_time.seconds);
    terminal_writestringf("Unix timestamp: %d\n", time_to_unix_seconds(&current_time));
    terminal_writestring("Kernel Initializing...\n");
    terminal_writestring("\n");

    /* Initialize the GDT */
    gdt_init();
#ifdef DEBUG
    debug_gdt();
    terminal_writestring("\n");
#endif

    /* Remap the PIC */
    pic_remap(0x20, 0x28);
#ifdef DEBUG
    debug_pic();
    terminal_writestring("\n");
#endif

    /* Initialize the IDT */
    idt_init();
#ifdef DEBUG
    debug_idt();
    terminal_writestring("\n");
#endif

    /* Initialize the Physical Memory Manager */
    pmm_init_status_t status = pmm_init(mbi);
#ifdef DEBUG
    debug_pmm(&status);
    terminal_writestring("\n");
#endif
    // TODO: Heap allocation and managment
    // Test allocation
    // void *block = pmm_alloc_block();
    // if (block) {
    //     terminal_writestringf("Allocated a block at: 0x%x\n", block);
    //     // And free it
    //     pmm_free_block(block);
    //     terminal_writestringf("Freed the block.\n");
    // } else {
    //     terminal_writestring("Could not allocate a block.\n");
    // }
    // terminal_writestring("\n");


    /* Initialize the keyboard */
    keyboard_init();
    register_interrupt_handler(0x21, keyboard_handler);
    // Drain keyboard buffer of any stale data before enabling interrupts
    // This prevents old scancodes (like 0xFA ACK) from being processed.
    inb(0x60); // Read and discard
    inb(0x60); // Read and discard again, just in case
    terminal_writestring("\n");

    /* Initialize the mouse */
    mouse_init();
    register_interrupt_handler(0x2C, mouse_handler);
    terminal_writestring("\n");

#ifdef DEBUG
    /* Debug multiboot headers */
    debug_multiboot_header(mbi);
    terminal_writestring("\n");
#endif

    terminal_writestring("Enabling interrupts...\n");
    enable_interrupts();
    terminal_writestring("Interrupts enabled.\n");

    // MAIN EVENT LOOP
    mouse_event_t mouse_event;
    keyboard_event_t keyboard_event;
    while(1) {
        if (keyboard_poll(&keyboard_event)) {
            if (keyboard_event.type == KEY_PRESS) {
                if (keyboard_event.special_key == KEY_UP_ARROW) {
                    terminal_scroll(1);
                } else if (keyboard_event.special_key == KEY_DOWN_ARROW) {
                    terminal_scroll(-1);
                } else if (keyboard_event.ascii) {
                    terminal_writestringf("ASCII: %d, Scancode: %x\n", keyboard_event.ascii, keyboard_event.scancode);
                    // terminal_putchar(keyboard_event.ascii);
                }
            }
        }

        if(mouse_poll(&mouse_event)) {
            terminal_writestringf("Mouse Event: buttons=%x, x=%d, y=%d\n", mouse_event.buttons_pressed, mouse_event.x_delta, mouse_event.y_delta);
        }
        asm volatile ("hlt");
    }
}

#ifdef DEBUG
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

void debug_pmm(pmm_init_status_t *pmm_status) {
    if (pmm_status->error) {
        terminal_writestring("Error: Could not find a suitable location for the memory map!\n");
        return;
    }

    terminal_writestringf("Total memory: %d KB (%d blocks)\n", pmm_status->total_memory_kb, pmm_status->max_blocks);
    terminal_writestringf("Required bitmap size: %d bytes\n", pmm_status->bitmap_size);
    terminal_writestringf("Kernel ends at: 0x%x\n", pmm_status->kernel_end);
    terminal_writestringf("Placing bitmap at: 0x%x\n", pmm_status->placement_address);
    terminal_writestringf("%d blocks used initially.\n", pmm_status->used_blocks);
}


#endif