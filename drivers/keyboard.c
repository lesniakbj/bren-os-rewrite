#include <keyboard.h>
#include <terminal.h>
#include <io.h>
#include <libc/stdint.h>

void keyboard_init(void) {
    // Initialization logic for the keyboard driver
}

void keyboard_handler(struct registers *regs) {
    static bool is_extended = false;
    kuint8_t scancode = inb(0x60);

    if (scancode == 0xE0) {
        is_extended = true;
    } else {
        if (is_extended) {
            switch (scancode) {
                case 77: // Right arrow
                    terminal_writestring("Right arrow pressed!\n");
                    break;
                case 205: // Right arrow released
                    terminal_writestring("Right arrow released!\n");
                    break;
                default:
                    break;
            }
            is_extended = false;
        } else {
            switch (scancode) {
                case 0x01: // Escape
                    terminal_writestringf("Escape pressed!\n");
                    break;
                case 0x81: // Escape released
                    terminal_writestringf("Escape released!\n");
                    break;
                default:
                    break;
            }
        }
    }
}
