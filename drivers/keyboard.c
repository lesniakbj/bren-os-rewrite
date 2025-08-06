#include <drivers/keyboard.h>
#include <drivers/keyboard_events.h>
#include <libc/stdint.h>
#include <arch/i386/io.h>
#include <drivers/terminal.h>

#define KEYBOARD_BUFFER_SIZE 256

static keyboard_event_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static kuint8_t keyboard_buffer_head = 0;
static kuint8_t keyboard_buffer_tail = 0;

void keyboard_init(void) {
    keyboard_buffer_head = 0;
    keyboard_buffer_tail = 0;
}

static void enqueue_event(keyboard_event_t event) {
    int next = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboard_buffer_tail) {
        keyboard_buffer[keyboard_buffer_head] = event;
        keyboard_buffer_head = next;
    }
}

bool keyboard_poll(keyboard_event_t *out_event) {
    if (keyboard_buffer_tail != keyboard_buffer_head) {
        *out_event = keyboard_buffer[keyboard_buffer_tail];
        keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
        return true;
    }
    return false;
}

void keyboard_handler(struct registers *regs) {
    static bool is_extended = false;
    kuint8_t scancode = inb(0x60);
    keyboard_event_t event = {0};

    if(scancode == 0xFA) {
        // cmd ack
        return;
    }

    if (scancode == 0xE0) {
        is_extended = true;
        return;
    }

    event.type = (scancode & 0x80) ? KEY_RELEASE : KEY_PRESS;
    event.scancode = scancode;

    if (is_extended) {
        switch (scancode) {
            case 0x48: event.special_key = KEY_UP_ARROW; break;
            case 0x50: event.special_key = KEY_DOWN_ARROW; break;
            case 0x4B: event.special_key = KEY_LEFT_ARROW; break;
            case 0x4D: event.special_key = KEY_RIGHT_ARROW; break;
            case 0x49: event.special_key = KEY_PAGE_UP; break;
            case 0x51: event.special_key = KEY_PAGE_DOWN; break;
        }
        is_extended = false;
    } else {
        const char* scancode_map = "\x00\x1B""1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
        if (scancode < sizeof(scancode_map)) {
            event.ascii = scancode_map[scancode];
        }
        if (scancode == 0x01) {
            event.special_key = KEY_ESCAPE;
        }
    }

    enqueue_event(event);
    return;
}