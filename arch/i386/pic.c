#include <arch/i386/pic.h>

void pic_remap(kuint32_t offset1, kuint32_t offset2) {
    // Mask all interrupts initially
    outb(PIC1_DATA, 0xFF);
    io_wait();
    outb(PIC2_DATA, 0xFF);
    io_wait();

    // Start initialization sequence
    // This command makes the PIC wait for 3 extra "initialisation words" on the data port.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Command Word 1 (ICW2): Remap offsets
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    // Command Word 2 (ICW3): Cascade identity
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    // Command Word 3 (ICW4): Mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Send EOI to clear any pending interrupts after remapping
    outb(PIC1_COMMAND, PIC_EOI);
    io_wait();
    outb(PIC2_COMMAND, PIC_EOI);
    io_wait();

    // Unmask IRQ0 (timer), IRQ1 (keyboard), and IRQ2 (cascade from slave)
    outb(PIC1_DATA, 0xF8);
    io_wait();
    // Unmask IRQ8 (RTC) and IRQ12 (mouse)
    outb(PIC2_DATA, 0xEE);
    io_wait();
}

kuint8_t pic_read_data_port(kuint16_t port) {
    return inb(port);
}

kuint16_t pic_read_irr() {
    outb(PIC1_COMMAND, 0x0A);   // OCW3 to read Interrupt Request Register
    outb(PIC2_COMMAND, 0x0A);   // OCW3 to read Interrupt Request Register
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

kuint16_t pic_read_isr() {
    outb(PIC1_COMMAND, 0x0B);   // OCW3 to read In-Service Register
    outb(PIC2_COMMAND, 0x0B);   // OCW3 to read In-Service Register
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}