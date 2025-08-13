#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/acpi.h>
#include <kernel/heap.h>
#include <kernel/log.h>
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
#include <drivers/pit.h>
#include <drivers/screen.h>
#include <drivers/serial.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/keyboard_events.h>
#include <drivers/text_mode_console.h>

void test_heap_allocations();
void kernel_event_loop();

void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    // Set MBI to the address of the Multiboot information structure
    multiboot_info_t *mbi = (multiboot_info_t *) multiboot_addr;

    // ---- Phase 1 ----
    // Init master systems, things needed to continue further initialization
    log_init(mbi);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        LOG_ERR("Invalid magic number: 0x%x\n", magic);
        return;
    }
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);
    pmm_init_status_t pmm_status = pmm_init(mbi);

    // ---- Phase 2 ----
    // Virtual memory management, relies on the GDT/PMM to be initialized
    vmm_init_status_t vmm_status = vmm_init(mbi);
    register_interrupt_handler(0x0E, page_fault_handler);
    heap_init(HEAP_VIRTUAL_START, HEAP_SIZE); // Initialize kernel heap

    // ---- Phase 3 ----
    // Non-priority subsystems (device discovery, time, etc)
    CMOS_Time current_time = time_init();
    pci_init();
    acpi_init();

    bool timer_initialized = false;
    if (hpet_init() == 0) {
        LOG_INFO("HPET initialized successfully.\n");
        timer_initialized = true;
    } else {
        LOG_WARN("HPET initialization failed, falling back to PIT.\n");
        // Fallback to PIT initialization
        if (pit_init(1000) == 0) { // Initialize PIT to fire interrupt every 1ms (1000 Hz)
                                   // You can adjust the frequency as needed (e.g., 100 for 100Hz)
            LOG_INFO("PIT initialized successfully.\n");
            timer_initialized = true;
            // Register the PIT interrupt handler (usually IRQ 0, mapped to IDT entry 0x20)
            // Make sure pit_handler is defined in drivers/pit.c and declared in drivers/pit.h
            register_interrupt_handler(0x20, pit_handler);
        } else {
            LOG_ERR("PIT initialization also failed! System timer unavailable.\n");
            // Handle the critical error - maybe loop or panic?
            // For now, we'll log and continue, but scheduling/timing won't work.
        }
    }
    if (timer_initialized) {
        system_time_init(&current_time);
        LOG_INFO("System time tracking initialized.\n");
    }

    keyboard_init();
    register_interrupt_handler(0x21, keyboard_handler);
    // Drain keyboard buffer of any stale data before enabling interrupts
    // This prevents old scancodes (like 0xFA ACK) from being processed.
    inb(0x60); // Read and discard
    inb(0x60); // Read and discard again, just in case
    mouse_init();
    register_interrupt_handler(0x2C, mouse_handler);
    register_interrupt_handler(0x28, rtc_handler); // Register RTC handler for IRQ8

    // ---- Phase 3 ----
    // Now we're all set up, lets enable interrupts
    enable_interrupts();

    // Test heap allocation
    test_heap_allocations();

    // vmm_identity_map_page(0x1000000);
    // terminal_write_string("Attempting to force a page fault...\n");
    // kuint32_t *ptr = (kuint32_t*)0x1000000;
    // *ptr = 0; // This should cause a page fault
    // terminal_write_string("This should not be printed.\n");
    // terminal_write_string("\n");

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
    debug_gdt();
    debug_idt();
    debug_pic();
    debug_pmm(&pmm_status);
    debug_time(current_time);
#endif

    // ---- Phase 5 ----
    // Begin main event loop
    kernel_event_loop();
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
                    // terminal_putchar(keyboard_event.ascii);
                }
            }
        }

        if(mouse_poll(&mouse_event)) {
            LOG_INFO("Mouse Event: buttons=%x, x=%d, y=%d\n", mouse_event.buttons_pressed, mouse_event.x_delta, mouse_event.y_delta);
        }
        
        // Refresh the screen to show clock updates and reduce flicker
        text_mode_console_refresh();
        
        asm volatile ("hlt");
    }
}
