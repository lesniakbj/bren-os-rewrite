#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/acpi.h>
#include <kernel/heap.h>
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

void test_heap_allocations();
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
    heap_init(HEAP_VIRTUAL_START, HEAP_SIZE); // Initialize kernel heap

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

    // Test heap allocation
    test_heap_allocations();

    // vmm_identity_map_page(0x1000000);
    // terminal_writestring("Attempting to force a page fault...\n");
    // kuint32_t *ptr = (kuint32_t*)0x1000000;
    // *ptr = 0; // This should cause a page fault
    // terminal_writestring("This should not be printed.\n");
    // terminal_writestring("\n");

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

void test_heap_allocations() {
    terminal_writestring("Testing heap allocation...\n");
    
    // Print initial heap statistics
    terminal_writestringf("Initial heap stats - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    generic_ptr test_alloc = kmalloc(128);
    if (test_alloc) {
        terminal_writestringf("Heap allocation successful! Address: 0x%x, Size: %d\n", test_alloc, 128);
    }
    
    // Print heap statistics after first allocation
    terminal_writestringf("After first allocation - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    generic_ptr null_realloc = krealloc(NULL, 256);
    if (null_realloc) {
        terminal_writestringf("krealloc(NULL, 256) successful! Address: 0x%x\n", null_realloc);
        char* data_ptr = (char*)null_realloc;
        for(int i = 0; i < 10; i++) {
            data_ptr[i] = 'A' + i;
        }
        terminal_writestring("Data written to null_realloc block\n");
    }
    
    // Print heap statistics after second allocation
    terminal_writestringf("After second allocation - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    if (test_alloc) {
        generic_ptr zero_realloc = krealloc(test_alloc, 0);
        if (zero_realloc == NULL) {
            terminal_writestring("krealloc(ptr, 0) correctly returned NULL and freed memory\n");
        }
    }
    
    // Print heap statistics after freeing
    terminal_writestringf("After freeing - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    generic_ptr expand_test = kmalloc(64);
    if (expand_test) {
        terminal_writestringf("Initial block for expansion test: 0x%x, Size: %d\n", expand_test, 64);

        char* data_ptr = (char*)expand_test;
        for(int i = 0; i < 32; i++) {
            data_ptr[i] = 'X' + (i % 26);
        }
        terminal_writestring("Data written to expand_test block\n");

        generic_ptr expanded = krealloc(expand_test, 512);
        if (expanded && expanded != expand_test) {
            terminal_writestringf("krealloc expanded block! Old: 0x%x, New: 0x%x, Size: %d\n", expand_test, expanded, 512);

            char* new_data_ptr = (char*)expanded;
            bool data_ok = true;
            for(int i = 0; i < 32; i++) {
                if (new_data_ptr[i] != ('X' + (i % 26))) {
                    data_ok = false;
                    break;
                }
            }

            if (data_ok) {
                terminal_writestring("Data successfully copied to new block!\n");
            } else {
                terminal_writestring("Data corruption detected!\n");
            }

            expand_test = expanded;
        } else if (expanded == expand_test) {
            terminal_writestring("Block was shrunk in place (no reallocation needed)\n");
        } else {
            terminal_writestring("Expansion failed!\n");
        }
    }
    
    // Print heap statistics after expansion test
    terminal_writestringf("After expansion test - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    if (expand_test) {
        generic_ptr shrunk = krealloc(expand_test, 32);
        if (shrunk) {
            terminal_writestringf("krealloc shrunk block! Result: 0x%x, Size: %d\n", shrunk, 32);
            expand_test = shrunk;
        } else {
            terminal_writestring("Shrinking failed!\n");
        }
    }
    
    // Print heap statistics after shrinking
    terminal_writestringf("After shrinking - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());
    
    // Test heap expansion explicitly
    terminal_writestring("Testing explicit heap expansion...\n");
    size_t size_before_expansion = heap_get_total_size();
    if (heap_expand(0x20000)) { // Expand by 128KB
        terminal_writestringf("Heap expansion successful! Size increased from %d to %d bytes\n", 
                              size_before_expansion, heap_get_total_size());
    } else {
        terminal_writestring("Heap expansion failed!\n");
    }
    
    // Print final heap statistics
    terminal_writestringf("Final heap stats - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    if (null_realloc) kfree(null_realloc);
    if (expand_test) kfree(expand_test);
    
    // Print heap statistics after all freeing
    terminal_writestringf("After all freeing - Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                          heap_get_total_size(), heap_get_used_size(), heap_get_free_size());

    terminal_writestring("Heap testing completed!\n");
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