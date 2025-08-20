#include <drivers/mouse.h>
#include <arch/i386/io.h>
#include <arch/i386/pic.h>
#include <kernel/log.h>

// State for reading mouse packets
static kuint8_t mouse_cycle = 0;
static kint8_t  mouse_packet[4];

// State for managing the read queue
static mouse_event_t mouse_event_queue[MOUSE_EVENT_QUEUE_SIZE];
static kuint8_t mouse_queue_head = 0;
static kuint8_t mouse_queue_tail = 0;

static bool mouse_wheel_present = false;
static bool mouse_extended_buttons = false;

// Waits for the PS/2 controller's input buffer to be empty.
static void mouse_wait_signal() {
    kuint32_t timeout = 100000;
    while (timeout--) {
        // 0 means empty, 1 means full. We wait until it's empty.
        if ((inb(MOUSE_CMD_PORT) & MOUSE_IN_BUFFER_STATUS) == 0) {
            return;
        }
        io_wait();
    }
    LOG_WARN("mouse_wait_signal timeout!");
}

// Waits for the PS/2 controller's output buffer to be full.
static void mouse_wait_data() {
    kuint32_t timeout = 100000;
    while (timeout--) {
        // 1 means full, 0 means empty. We wait until it's full.
        if ((inb(MOUSE_CMD_PORT) & MOUSE_OUT_BUFFER_STATUS) == 1) {
            return;
        }
        io_wait();
    }
    LOG_WARN("mouse_wait_data timeout!");
}

// Sends a command byte to the mouse device.
static void mouse_write(kuint8_t cmd) {
    // Tell the controller we want to send a byte to the mouse. Then send the command byte.
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, WRITE_TO_AUX_DEV);
    mouse_wait_signal();
    outb(MOUSE_DATA_PORT, cmd);
}

// Reads a data byte from the mouse device.
static kuint8_t mouse_read() {
    mouse_wait_data();
    return inb(MOUSE_DATA_PORT);
}

// Add an event to the mouse event queue
static void mouse_queue_add(mouse_event_t event) {
    kuint8_t next_head = (mouse_queue_head + 1) % MOUSE_EVENT_QUEUE_SIZE;

    // If queue is full, drop the new event
    if(next_head != mouse_queue_tail) {
        mouse_event_queue[mouse_queue_head] = event;
        mouse_queue_head = next_head;
    }
}

// Sets the sampling rate for the mouse
static void set_mouse_rate(kuint16_t rate) {
    mouse_write(MOUSE_SET_SAMPLE_RATE);
    mouse_read();
    mouse_write(rate);
    mouse_read();
}

static kuint8_t get_mouse_id() {
    mouse_write(MOUSE_GET_ID);
    mouse_read();               // ACK
    return mouse_read();        // the ID
}

// Polls to see if the mouse has events in the buffer or not
bool mouse_poll(mouse_event_t* event_out) {
    if(mouse_queue_head == mouse_queue_tail) {
        return false; // Empty
    }

    *event_out = mouse_event_queue[mouse_queue_tail];
    mouse_queue_tail = (mouse_queue_tail + 1) % MOUSE_EVENT_QUEUE_SIZE;
    return true;
}

// The interrupt handler for the mouse.
void mouse_handler(registers_t *regs) {
    // TODO: Remove
    (void)regs;

    // Read the byte from the mouse
    kuint8_t data = inb(MOUSE_DATA_PORT);

    // Decode the mouse packet, this may come as 3 or 4 byte packet (if we have a scroll wheel)
    switch(mouse_cycle) {
        case 0:
            // The first byte should have bit 3 set. This is a sync check.
            if ((data & MOUSE_DATA_SYNC_BIT) != 0) {
                mouse_packet[0] = data;
                mouse_cycle++;
            } else {
                LOG_INFO("Mouse sync error, byte: 0x%x", data);
            }
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_packet[2] = data;
            if (mouse_wheel_present) {
                mouse_cycle++;
            } else {
                // Process 3-byte packet
                mouse_event_t event = {
                    .buttons_pressed = mouse_packet[0],
                    .x_delta = mouse_packet[1],
                    .y_delta = mouse_packet[2],
                    .z_delta = 0
                };
                mouse_queue_add(event);
                mouse_cycle = 0; // Reset for the next packet.
            }
            break;
        case 3:
            mouse_packet[3] = data;
            // Process 4-byte packet
            mouse_event_t event = {
                .buttons_pressed = mouse_packet[0],
                .x_delta = mouse_packet[1],
                .y_delta = mouse_packet[2],
                .z_delta = mouse_packet[3]
            };
            mouse_queue_add(event);
            mouse_cycle = 0; // Reset for the next packet.
            break;
    }

    // Send End-of-Interrupt to the PICs, this is done manually by the mouse as it is a multi-byte interface
    outb(PIC2_COMMAND, PIC_EOI); // EOI for slave PIC (IRQ 8-15)
    outb(PIC1_COMMAND, PIC_EOI); // EOI for master PIC (IRQ 0-7)
}

// Initializes the mouse driver.
void mouse_init(void) {
    // Enable the auxiliary device (the mouse), there's no direct ACK for 0xA8, but we can check the status byte later.
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, ENABLE_AUX_DEV);

    // Enable interrupts from the mouse. To do this we get the status byte, update it to enable IRQ12 and disable
    // the mouse clock, then send the staus byte back to the mouse.
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, COMPAQ_STATUS_BYTE);
    mouse_wait_data();
    kuint8_t status = inb(MOUSE_DATA_PORT);
    status |= IRQ12_ENABLE_BIT;                 // Set bit 1 to enable IRQ12.
    status &= ~MOUSE_CLOCK_ENABLE_BIT;          // Clear bit 5, which disables the mouse clock.
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, WRITE_STATUS_BYTE);    // Command to set status byte.
    mouse_wait_signal();
    outb(MOUSE_DATA_PORT, status);

    // Tell the mouse to use default settings.
    mouse_write(MOUSE_DEFAULT_SETTINGS);
    kuint8_t ack = mouse_read();
    if (ack != ACK_SUCCESS) {
        LOG_WARN("Error attempting to set default settings for mouse!");
    }

    // Step 4: Enable packet streaming from the mouse.
    mouse_write(MOUSE_ENABLE_PACKET_STREAM);
    ack = mouse_read();
    if (ack != ACK_SUCCESS) {
        LOG_WARN("Error attempting to set packet streaming settings for mouse!");
    }

    // Enable mouse extended features... this is a very specific sequence of commands sent to the mouse
    // that can be used to enable extended features (scroll wheel, 4th/5th buttons, etc). First we will
    // try to enable the scroll wheel, then try to enable buttons 4 and 5.
    set_mouse_rate(200);
    set_mouse_rate(100);
    set_mouse_rate(80);

    // After this sequence, we can check if we have a scroll wheel by getting the mouse's ID.
    if (get_mouse_id() == 0x03) {
        // Ok we enabled the scroll wheel, lets try to enable buttons 4/5
        set_mouse_rate(200);
        set_mouse_rate(200);
        set_mouse_rate(80);
        if (get_mouse_id() == 0x04) {
            mouse_extended_buttons = true;
        }
        mouse_wheel_present = true;
        LOG_INFO("Mouse scroll wheel detected.");
    }
}