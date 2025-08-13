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

void keyboard_proc();
void mouse_proc();

void kernel_main(kuint32_t magic, kuint32_t multiboot_addr) {
    multiboot_info_t *mbi = (multiboot_info_t *) multiboot_addr;

    // Phase 1: Core system initialization (no dependencies)
    log_init(mbi);
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        LOG_ERR("Invalid magic number: 0x%x\n", magic);
        return;
    }
    gdt_init();
    idt_init();
    pic_remap(0x20, 0x28);

    // Phase 2: Memory management
    pmm_init_status_t pmm_status = pmm_init(mbi);
    vmm_init_status_t vmm_status = vmm_init(mbi);
    heap_init(HEAP_VIRTUAL_START, HEAP_SIZE);

    // Phase 3: Subsystems and drivers
    CMOS_Time current_time = time_init();
    pci_init();
    acpi_init();

    // Initialize a timer (HPET or PIT)
    bool timer_initialized = false;
    if (hpet_init() == 0) {
        timer_initialized = true;
    } else {
        if (pit_init(1000) == 0) {
            timer_initialized = true;
            register_interrupt_handler(0x20, pit_handler);
        } else {
            LOG_ERR("FATAL: No timer available!\n");
            return; // Halt if no timer
        }
    }
    if (timer_initialized) {
        system_time_init(&current_time);
    }

    // Register interrupt handlers BEFORE enabling interrupts.
    // The device initialization will happen inside their respective processes.
    register_interrupt_handler(0x0E, page_fault_handler);
    register_interrupt_handler(0x21, keyboard_handler);
    register_interrupt_handler(0x2C, mouse_handler);
    register_interrupt_handler(0x28, rtc_handler);

    // Initialize the process scheduler
    proc_init();

    // Create the driver processes
    proc_create(keyboard_proc);
    proc_create(mouse_proc);

    // Enable interrupts now that all handlers are registered.
    enable_interrupts();

    // The kernel's main thread now becomes the idle task.
    // It will halt until the next interrupt, saving CPU.
    // All other work is done by scheduled processes or interrupt handlers.
    while(1) {
        asm volatile("hlt");
    }
}

void keyboard_proc() {
    // --- Initialization ---
    // This is a critical section. Disable interrupts to ensure that the
    // keyboard controller initialization sequence is not interrupted.
    asm volatile("cli");
    keyboard_init();
    // Drain keyboard buffer of any stale data.
    inb(0x60);
    inb(0x60);
    asm volatile("sti");

    LOG_INFO("Keyboard process started.\n");

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
        // Let the scheduler run other processes.
        // A proper yield() syscall would be better here in the future.
        asm volatile ("hlt");
    }
}

void mouse_proc() {
    // --- Initialization ---
    // This is a critical section. Disable interrupts to ensure that the
    // mouse controller initialization sequence is not interrupted.
    asm volatile("cli");
    mouse_init();
    asm volatile("sti");

    LOG_INFO("Mouse process started.\n");

    // --- Main Loop ---
    mouse_event_t mouse_event;
    while(1) {
        if(mouse_poll(&mouse_event)) {
            LOG_INFO("Mouse Event: buttons=%x, x=%d, y=%d\n", mouse_event.buttons_pressed, mouse_event.x_delta, mouse_event.y_delta);
        }
        // Let the scheduler run other processes.
        asm volatile ("hlt");
    }
}