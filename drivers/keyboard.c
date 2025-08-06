#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <libc/stdint.h>
#include <arch/i386/io.h>

// Forward declaration for the scroll function
void framebuffer_scroll(int lines);

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
                case 0x49: // Page Up
                    // framebuffer_scroll(-1); // Scroll up one line
                    break;
                case 0x51: // Page Down
                    // framebuffer_scroll(1); // Scroll down one line
                    break;
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
                    // For simplicity, we'll just handle ASCII characters here
                    // A proper implementation would use a scancode-to-ASCII map
                    if (scancode < 0x80) { // Not a release scancode
                        char c = 0;
                        // A very basic scancode to ASCII conversion
                        const char* scancode_map = "\x00\x1B""1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
                        if (scancode < sizeof(scancode_map)) {
                            c = scancode_map[scancode];
                        }
                        if (c != 0) {
                            terminal_putchar(c);
                        }
                    }
                    break;
            }
        }
    }
}

