#include <arch/i386/pic.h>

void pic_remap(kuint32_t offset1, kuint32_t offset2) {
    // Mask all interrupts initially
    outb(PIC1_DATA, 0xFF);
    io_wait();
    outb(PIC2_DATA, 0xFF);
    io_wait();

    // Start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Remap offsets
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    // Cascade identity
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    // Mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Send EOI to clear any pending interrupts after remapping
    outb(PIC1_COMMAND, 0x20);
    io_wait();
    outb(PIC2_COMMAND, 0x20);
    io_wait();

    // Unmask IRQ1 (keyboard) and IRQ12 (mouse)
    outb(PIC1_DATA, 0xF8); // 1111 1101b: Unmask IRQ1 (timer/keyboard/slave)
    io_wait();
    outb(PIC2_DATA, 0xEF); // 1110 1111b: Unmask IRQ12 (mouse)
    io_wait();
}

kuint8_t pic_read_data_port(kuint16_t port) {
    return inb(port);
}

kuint8_t pic_read_irr(void) {
    outb(PIC1_COMMAND, 0x0A); // OCW3 to read IRR
    return inb(PIC1_COMMAND);
}

kuint8_t pic_read_isr(void) {
    outb(PIC1_COMMAND, 0x0B); // OCW3 to read ISR
    return inb(PIC1_COMMAND);
}