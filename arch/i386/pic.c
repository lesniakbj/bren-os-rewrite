#include <arch/i386/pic.h>

void pic_remap(kuint32_t offset1, kuint32_t offset2) {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);

    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Set masks: Unmask IRQ1 (keyboard) and IRQ2 (for slave PIC) on Master.
    // Unmask IRQ12 (mouse) on Slave.
    outb(PIC1_DATA, 0xF9); // 1111 1001b: Unmask IRQ1 and IRQ2.
    outb(PIC2_DATA, 0xEF); // 1110 1111b: Unmask IRQ12.
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