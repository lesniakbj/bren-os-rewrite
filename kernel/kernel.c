#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/acpi.h>
#include <kernel/debug.h>
#include <arch/i386/idt.h>
#include <arch/i386/gdt.h>
#include <arch/i386/pic.h>
#include <arch/i386/interrupts.h>
#include <arch/i386/vmm.h>
#include <arch/i386/pmm.h>
#include <arch/i386/time.h>
#include <drivers/pci.h>
#include <drivers/hpet.h>
#include <drivers/terminal.h>
#include <drivers/screen.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/keyboard_events.h>

void kernel_event_loop();

void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    // Set MBI to the address of the Multiboot information structure
    multiboot_info_t *mbi = (multiboot_info_t *) multiboot_addr;

    // ---- Phase 1 ----
    // Init master systems, things needed to continue further initialization
    screen_init(mbi);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        terminal_writestring("Invalid magic number: 0x");
        terminal_writestring("\n");
        return;
    }
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);
    pmm_init_status_t status = pmm_init(mbi);

    // ---- Phase 2 ----
    // Virtual memory management, relies on the GDT/PMM to be initialized
    vmm_init(mbi);
    register_interrupt_handler(0x0E, page_fault_handler);

    // ---- Phase 3 ----
    // Non-priority subsystems (device discovery, time, etc)
    CMOS_Time current_time = time_init();
    pci_init();
    acpi_init();
    hpet_init();
    keyboard_init();
    register_interrupt_handler(0x21, keyboard_handler);
    // Drain keyboard buffer of any stale data before enabling interrupts
    // This prevents old scancodes (like 0xFA ACK) from being processed.
    inb(0x60); // Read and discard
    inb(0x60); // Read and discard again, just in case
    mouse_init();
    register_interrupt_handler(0x2C, mouse_handler);

    // ---- Phase 3 ----
    // Now we're all set up, lets enable interrupts
    enable_interrupts();

    // --- Force a page fault ---
    vmm_identity_map_page(0x1000000);
    terminal_writestring("Attempting to force a page fault...\n");
    kuint32_t *ptr = (kuint32_t*)0x1000000;
    *ptr = 0; // This should cause a page fault
    terminal_writestring("This should not be printed.\n");
    terminal_writestring("\n");

    // ---- Phase 4 ----
    // Setup Ring 3 and make the jump to User Mode

    // TODO:
    //  1) Virtual Mem
    //  2) Heap
    //  3) Process model/scheduling
    //  4) User Mode
    //  5) User Mode drivers
    //  6) Window Manager

    // TODO: Heap allocation and managment

#ifdef DEBUG
    // Debug multiboot headers
    debug_multiboot_header(mbi);
    terminal_writestring("\n");
    debug_gdt();
    terminal_writestring("\n");
    debug_idt();
    terminal_writestring("\n");
    debug_pic();
    terminal_writestring("\n");
    debug_pmm(&status);
    terminal_writestring("\n");
    debug_time(current_time);
    terminal_writestring("\n");
#endif

    // ---- Phase 5 ----
    // Begin main event loop
    kernel_event_loop();
}

void kernel_event_loop() {
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