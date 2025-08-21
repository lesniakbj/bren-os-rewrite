#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/heap.h>
#include <kernel/log.h>
#include <kernel/debug.h>
#include <kernel/syscall.h>
#include <kernel/time.h>
#include <kernel/vfs.h>
#include <kernel/proc.h>
#include <arch/i386/idt.h>
#include <arch/i386/gdt.h>
#include <arch/i386/pic.h>
#include <arch/i386/interrupts.h>
#include <arch/i386/vmm.h>
#include <arch/i386/pmm.h>
#include <arch/i386/time.h>
#include <arch/i386/fault.h>
#include <drivers/pit.h>
#include <drivers/screen.h>
#include <drivers/serial.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/keyboard_events.h>
#include <drivers/text_mode_console.h>
#include <libc/sysstd.h>

void keyboard_proc();
void mouse_proc();

void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    multiboot_info_t *mbi = (multiboot_info_t *) multiboot_addr;

    // Phase 1: Core system initialization (no dependencies other than the MBI)
    log_init(mbi);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        LOG_ERR("Invalid magic number: 0x%x", magic);
        return;
    }
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);

    // Phase 2: Memory management
    pmm_init_status_t pmm_status = pmm_init(mbi);
    vmm_init_status_t vmm_status = vmm_init(mbi);
    heap_init(HEAP_VIRTUAL_START, HEAP_SIZE);

    //TODO: remove
    //(void)pmm_status;
    //(void)vmm_status;

    // Phase 3: Subsystems and drivers and timers
    cmos_time_t current_time = time_init();
    if (pit_init(1000) == 0) {
        register_interrupt_handler(0x28, rtc_handler);
        register_interrupt_handler(0x20, pit_handler);
        system_time_init(&current_time);
    } else {
        LOG_ERR("FATAL: No timer available!");
        return; // Halt if no timer
    }

    // Register interrupt handlers BEFORE enabling interrupts.
    // The device initialization will happen inside their respective processes.
    register_interrupt_handler(0x0E, page_fault_handler);
    register_interrupt_handler(0x0D, general_protection_fault_handler);
    register_interrupt_handler(0x21, keyboard_handler);
    register_interrupt_handler(0x2C, mouse_handler);
    register_interrupt_handler(0x80, syscall_handler);

    // Initialize the vfs so it can be used by Procs
    vfs_init();

    // Initialize the process scheduler, proc 0, etc.
    proc_init();

    // Create the driver processes
    proc_create(keyboard_proc, false);
    proc_create(mouse_proc, false);

    // Create a test user mode program here
    // create_user_process();
    // create_user_process_syscall_exit();

    // Enable interrupts now that all handlers are registered.
    asm volatile("sti");
    LOG_DEBUG("Interrupts are now enabled, processes starting...");

    // The kernel's main thread now becomes the idle task.
    // All other work is done by scheduled processes or interrupt handlers.
    while(1) {
        text_mode_console_refresh();
        // vfs_write(1, "Hello from proc 0", 19);
        asm volatile("hlt");
    }
}

void keyboard_proc() {
    // This is a critical section. Disable interrupts to ensure that the
    // keyboard controller initialization sequence is not interrupted.
    // The are already disabled... but lets be really sure...
    asm volatile("cli");
    keyboard_init();
    // Drain keyboard buffer of any stale data.
    inb(0x60);
    inb(0x60);
    asm volatile("sti");

    LOG_INFO("Keyboard process started.");

    // --- Main Loop ---
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
            }
        }

        // Voluntarily give up the CPU to the scheduler... by exiting...
        kuint32_t id = proc_pid();
        LOG_INFO("Voluntarily exiting process ID: %d", id);
        vfs_write(1, "hello before exit\n", sizeof("hello before exit\n") - 1);
        vfs_write(3, "hello before exit\n", sizeof("hello before exit\n") - 1);
        proc_exit(-1);
    }
}

void mouse_proc() {
    // This is a critical section. Disable interrupts to ensure that the
    // mouse controller initialization sequence is not interrupted.
    // The are already disabled... but lets be really sure...
    asm volatile("cli");
    mouse_init();
    asm volatile("sti");

    LOG_INFO("Mouse process started.");

    // --- Main Loop ---
    mouse_event_t mouse_event;
    while(1) {
        if(mouse_poll(&mouse_event)) {
            kint8_t scroll_z = 0;
            bool b4 = false;
            bool b5 = false;

            // If it is an extended event or scroll we need to decompose the extra buttons
            if(mouse_event.z_delta != 0) {
                b4 = mouse_event.z_delta & 0x10;
                b5 = mouse_event.z_delta & 0x20;
                scroll_z = mouse_event.z_delta & 0x0F;
                if (scroll_z > 7) {
                    scroll_z -=16;
                }
            }

            if(scroll_z != 0) {
                terminal_scroll(-scroll_z);
            }

            if(b4) {
                LOG_INFO("Mouse button 4 was pressed!");
            }

            if(b5) {
                LOG_INFO("Mouse button 5 was pressed!");
            }

            if(mouse_event.buttons_pressed & 0x01) {
                LOG_INFO("Mouse button left was pressed!");
            }
        }
        // Voluntarily give up the CPU to the scheduler.
        proc_yield();
    }
}
