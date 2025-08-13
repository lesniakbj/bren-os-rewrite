#include <drivers/serial.h>
#include <arch/i386/io.h>

void serial_init(kuint16_t port) {
    // Disable interrupts
    outb(port + SERIAL_INTERRUPT_REG, 0x00);
    
    // Enable DLAB (set baud rate divisor)
    outb(port + SERIAL_LINE_REG, 0x80);
    
    // Set divisor to 1 (115200 baud)
    outb(port + SERIAL_DATA_REG, 0x01);        // Low byte
    outb(port + SERIAL_INTERRUPT_REG, 0x00);   // High byte
    
    // 8 bits, no parity, one stop bit
    outb(port + SERIAL_LINE_REG, 0x03);
    
    // Enable FIFO, clear them, with 14-byte threshold
    outb(port + SERIAL_FIFO_REG, 0xC7);
    
    // IRQs enabled, RTS/DSR set
    outb(port + SERIAL_MODEM_REG, 0x0B);
    
    // Enable interrupts
    outb(port + SERIAL_INTERRUPT_REG, 0x01);
}

kuint8_t serial_received(kuint16_t port) {
    return inb(port + SERIAL_LINE_STATUS_REG) & SERIAL_LINE_STATUS_DATA_READY;
}

kuint8_t serial_read_char(kuint16_t port) {
    while (serial_received(port) == 0);
    return inb(port);
}

kuint8_t serial_is_transmit_empty(kuint16_t port) {
    return inb(port + SERIAL_LINE_STATUS_REG) & SERIAL_LINE_STATUS_TRANSMIT_EMPTY;
}

void serial_write_char(kuint16_t port, char c) {
    while (serial_is_transmit_empty(port) == 0);
    outb(port, c);
}

void serial_write_string(kuint16_t port, const char* str) {
    while (*str) {
        serial_write_char(port, *str++);
    }
}