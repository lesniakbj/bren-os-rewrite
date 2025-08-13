#ifndef SERIAL_H
#define SERIAL_H

#include <libc/stdint.h>

// Serial port addresses
#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8

// Serial port offsets
#define SERIAL_DATA_REG         0    // Data register (read/write)
#define SERIAL_INTERRUPT_REG    1    // Interrupt enable register (write)
#define SERIAL_FIFO_REG         2    // FIFO control register (write)
#define SERIAL_LINE_REG         3    // Line control register (write)
#define SERIAL_MODEM_REG        4    // Modem control register (write)
#define SERIAL_LINE_STATUS_REG  5    // Line status register (read)
#define SERIAL_MODEM_STATUS_REG 6    // Modem status register (read)

// Line status flags
#define SERIAL_LINE_STATUS_DATA_READY     0x01
#define SERIAL_LINE_STATUS_OVERRUN_ERROR  0x02
#define SERIAL_LINE_STATUS_PARITY_ERROR   0x04
#define SERIAL_LINE_STATUS_FRAMING_ERROR  0x08
#define SERIAL_LINE_STATUS_BREAK_SIGNAL   0x10
#define SERIAL_LINE_STATUS_TRANSMIT_EMPTY 0x20
#define SERIAL_LINE_STATUS_TRANSMIT_IDLE  0x40
#define SERIAL_LINE_STATUS_FIFO_ERROR     0x80

// Baud rate divisors
#define SERIAL_BAUD_115200 1
#define SERIAL_BAUD_57600  2
#define SERIAL_BAUD_38400  3
#define SERIAL_BAUD_19200  6
#define SERIAL_BAUD_9600   12

// Function prototypes
void serial_init(kuint16_t port);
void serial_write_char(kuint16_t port, char c);
void serial_write_string(kuint16_t port, const char* str);
kuint8_t serial_received(kuint16_t port);
kuint8_t serial_read_char(kuint16_t port);
kuint8_t serial_is_transmit_empty(kuint16_t port);

#endif // SERIAL_H