// drivers/mouse.c

#include <drivers/mouse.h>
#include <arch/i386/io.h>
#include <kernel/log.h> // Add this for LOG_INFO/LOG_ERR

// PS/2 Controller and Mouse Ports
#define MOUSE_DATA_PORT   0x60
#define MOUSE_CMD_PORT    0x64

// Mouse event queue size
#define MOUSE_EVENT_QUEUE_SIZE 16

// State for reading mouse packets
static kuint8_t mouse_cycle = 0;
static kint8_t  mouse_packet[3];

// State for managing the read queue
static mouse_event_t mouse_event_queue[MOUSE_EVENT_QUEUE_SIZE];
static kuint8_t mouse_queue_head = 0;
static kuint8_t mouse_queue_tail = 0;

// Waits for the PS/2 controller's input buffer to be empty.
static void mouse_wait_signal() {
    kuint32_t timeout = 100000;
    while (timeout--) {
        // Bit 1 of the status port is the input buffer status.
        // 0 means empty, 1 means full. We wait until it's empty.
        if ((inb(MOUSE_CMD_PORT) & 0x02) == 0) {
            return;
        }
    }
    // Consider logging a timeout error if this loop exits
    LOG_WARN("mouse_wait_signal timeout\n");
}

// Waits for the PS/2 controller's output buffer to be full.
static void mouse_wait_data() {
    kuint32_t timeout = 100000;
    while (timeout--) {
        // Bit 0 of the status port is the output buffer status.
        // 1 means full, 0 means empty. We wait until it's full.
        if ((inb(MOUSE_CMD_PORT) & 0x01) == 1) {
            return;
        }
    }
    // Consider logging a timeout error if this loop exits
    LOG_WARN("mouse_wait_data timeout\n");
}

// Sends a command byte to the mouse device.
static void mouse_write(kuint8_t cmd) {
    // Tell the controller we want to send a byte to the mouse.
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, 0xD4);
    // Send the command byte.
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

int mouse_poll(mouse_event_t* event_out) {
    if(mouse_queue_head == mouse_queue_tail) {
        return 0; // Empty
    }

    *event_out = mouse_event_queue[mouse_queue_tail];
    mouse_queue_tail = (mouse_queue_tail + 1) % MOUSE_EVENT_QUEUE_SIZE;
    return 1;
}

// The interrupt handler for the mouse.
void mouse_handler(struct registers *regs) {
    LOG_INFO("Mouse IRQ\n"); // Add this for basic interrupt confirmation

    // Read the byte from the mouse. This MUST be done to acknowledge
    // the interrupt to the PS/2 controller.
    kuint8_t data = inb(MOUSE_DATA_PORT);
    LOG_INFO("Mouse data: 0x%x\n", data); // Log raw data byte

    // This is a state machine to read the 3-byte mouse packet.
    switch(mouse_cycle) {
        case 0:
            // The first byte should have bit 3 set. This is a sync check.
            // If it's not set, we are out of sync, so we reset.
            if ((data & 0x08) != 0) {
                mouse_packet[0] = data;
                mouse_cycle++;
            } else {
                LOG_INFO("Mouse sync error, byte: %02x\n", data);
            }
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_packet[2] = data;
            mouse_event_t event = {
                .buttons_pressed = mouse_packet[0],
                .x_delta = mouse_packet[1],
                .y_delta = mouse_packet[2]
            };
            mouse_queue_add(event);
            mouse_cycle = 0; // Reset for the next packet.
            break;
    }

    // Send End-of-Interrupt to the PICs. This MUST be done to allow
    // further interrupts to be processed.
    outb(0xA0, 0x20); // EOI for slave PIC (IRQ 8-15)
    outb(0x20, 0x20); // EOI for master PIC (IRQ 0-7)
}

// Initializes the mouse driver.
void mouse_init(void) {
    LOG_INFO("Initializing PS/2 Mouse...\n");

    // Step 1: Enable the auxiliary device (the mouse).
    LOG_INFO("  Enabling auxiliary device...\n");
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, 0xA8);

    // There's no direct ACK for 0xA8, but we can check the status byte later.

    // Step 2: Enable interrupts from the mouse.
    // We do this by getting the PS/2 controller's "Compaq Status Byte"
    // and setting the IRQ12 enable bit (bit 1).
    LOG_INFO("  Enabling mouse interrupts...\n");
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, 0x20); // Command to get status byte.
    mouse_wait_data();
    kuint8_t status = inb(MOUSE_DATA_PORT);
    LOG_INFO("  PS/2 Status Byte before: 0x%x\n", status);
    status |= 0x02;  // Set bit 1 to enable IRQ12.
    status &= ~0x20; // Clear bit 5, which disables the mouse clock.
                     // This is important for some controllers.
    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, 0x60); // Command to set status byte.
    mouse_wait_signal();
    outb(MOUSE_DATA_PORT, status);

    mouse_wait_signal();
    outb(MOUSE_CMD_PORT, 0x20); // Command to get status byte again.
    mouse_wait_data();
    kuint8_t status_after = inb(MOUSE_DATA_PORT);
    LOG_INFO("  PS/2 Status Byte after: 0x%x\n", status_after);

    // Step 3: Tell the mouse to use default settings.
    LOG_INFO("  Sending mouse reset command (0xF6)...\n");
    mouse_write(0xF6);
    kuint8_t ack1 = mouse_read(); // Acknowledge the command.
    LOG_INFO("  Mouse ACK for 0xF6: 0x%x (expected 0xFA)\n", ack1);
    if (ack1 != 0xFA) {
        LOG_ERR("  Mouse failed to acknowledge 0xF6 command\n");
        // Consider returning or handling the error
    }

    // Step 4: Enable packet streaming from the mouse.
    LOG_INFO("  Enabling mouse data reporting (0xF4)...\n");
    mouse_write(0xF4);
    kuint8_t ack2 = mouse_read(); // Acknowledge the command.
    LOG_INFO("  Mouse ACK for 0xF4: 0x%x (expected 0xFA)\n", ack2);
    if (ack2 != 0xFA) {
        LOG_ERR("  Mouse failed to acknowledge 0xF4 command\n");
        // Consider returning or handling the error
    }
    
    LOG_INFO("PS/2 Mouse initialization complete.\n");
}