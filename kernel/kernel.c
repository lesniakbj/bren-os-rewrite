#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/acpi.h>
#include <kernel/debug.h>
#include <arch/i386/idt.h>
#include <arch/i386/gdt.h>
#include <arch/i386/pic.h>
#include <arch/i386/interrupts.h>
#include <arch/i386/memory.h>
#include <arch/i386/time.h>
#include <drivers/pci.h>
#include <drivers/hpet.h>
#include <drivers/terminal.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/keyboard_events.h>


void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    // Set MBI to the address of the Multiboot information structure
    multiboot_info_t *mbi = (multiboot_info_t *) multiboot_addr;

    // Init screen early so we can show debug information
    screen_init(mbi);

#ifdef DEBUG
    // Debug multiboot headers
    debug_multiboot_header(mbi);
    terminal_writestring("\n");
#endif

    // Am I booted by a Multiboot-compliant boot loader?
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        terminal_writestring("Invalid magic number: 0x");
        terminal_writestring("\n");
        return;
    }

    // Main Initialization sequence
    terminal_writestring("Kernel Initializing...\n");

    // Init wall time as early as possible, we will add updates to this later/use a high res timer for sys time
    terminal_writestring("Retrieving wall clock time from CMOS...\n");
    CMOS_Time current_time = time_init();
#ifdef DEBUG
    debug_time(current_time);
    terminal_writestring("\n");
#endif

    // Initialize the GDT
    terminal_writestring("Installing GDT...\n");
    gdt_init();
#ifdef DEBUG
    debug_gdt();
    terminal_writestring("\n");
#endif

   // Remap the PIC
    terminal_writestring("Remapping PICs for PS/2 devices...\n");
    pic_remap(0x20, 0x28);
#ifdef DEBUG
    debug_pic();
    terminal_writestring("\n");
#endif

   // Initialize the IDT
    terminal_writestring("Installing IDT and ISRs...\n");
    idt_init();
#ifdef DEBUG
    debug_idt();
    terminal_writestring("\n");
#endif

    // TODO: Here is where I want to initialize ACPI I believe so I can get sys timers (HPET or PIT for "high res")
    pci_init();
    acpi_init();
    hpet_init();

   // Initialize the Physical Memory Manager
    terminal_writestring("Initializing physical memory manager...\n");
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


    // Initialize the keyboard
    terminal_writestring("Initializing keyboard PS/2 device...\n");
    keyboard_init();
    register_interrupt_handler(0x21, keyboard_handler);
    // Drain keyboard buffer of any stale data before enabling interrupts
    // This prevents old scancodes (like 0xFA ACK) from being processed.
    inb(0x60); // Read and discard
    inb(0x60); // Read and discard again, just in case
    terminal_writestring("\n");

   // Initialize the mouse
    terminal_writestring("Initializing mouse PS/2 device...\n");
    mouse_init();
    register_interrupt_handler(0x2C, mouse_handler);
    terminal_writestring("\n");

    terminal_writestring("Enabling interrupts...\n");
    enable_interrupts();

    // TODO:
    //  1) Virtual Mem
    //  2) Heap
    //  3) Process model/scheduling
    //  4) User Mode
    //  5) User Mode drivers
    //  6) Window Manager

    // MAIN EVENT REACTION LOOP
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
                    terminal_putchar(keyboard_event.ascii);
                }
            } else if (keyboard_event.type == KEY_RELEASE) {
                if (keyboard_event.ascii) {
                    terminal_putchar(keyboard_event.ascii);
                }
            }
        }

        if(mouse_poll(&mouse_event)) {
            terminal_writestringf("Mouse Event: buttons=%x, x=%d, y=%d\n", mouse_event.buttons_pressed, mouse_event.x_delta, mouse_event.y_delta);
        }
        asm volatile ("hlt");
    }
}