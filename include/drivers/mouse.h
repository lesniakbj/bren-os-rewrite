#ifndef DRIVERS_MOUSE_H
#define DRIVERS_MOUSE_H

#include <libc/stdint.h>
#include <arch/i386/interrupts.h>

#define ACK_SUCCESS 0xFA

#define WRITE_TO_AUX_DEV 0xD4
#define ENABLE_AUX_DEV 0xA8
#define COMPAQ_STATUS_BYTE 0x20
#define IRQ12_ENABLE_BIT 0x02
#define MOUSE_CLOCK_ENABLE_BIT 0x20
#define WRITE_STATUS_BYTE 0x60

#define MOUSE_DATA_PORT   0x60
#define MOUSE_CMD_PORT    0x64

#define MOUSE_EVENT_QUEUE_SIZE 16

#define MOUSE_OUT_BUFFER_STATUS 0x01
#define MOUSE_IN_BUFFER_STATUS 0x02
#define MOUSE_DATA_SYNC_BIT 0x08

#define MOUSE_GET_ID 0xF2
#define MOUSE_DEFAULT_SETTINGS 0xF6
#define MOUSE_ENABLE_PACKET_STREAM 0xF4
#define MOUSE_SET_SAMPLE_RATE 0xF3

typedef struct {
    kint8_t buttons_pressed;
    kint8_t x_delta;
    kint8_t y_delta;
    kint8_t z_delta;
} mouse_event_t;

void mouse_init();
void mouse_handler(registers_t *regs);
bool mouse_poll(mouse_event_t* event_out);

#endif // DRIVERS_MOUSE_H
