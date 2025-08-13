#include <drivers/terminal.h>
#include <kernel/multiboot.h>

extern void text_mode_console_init(void);
extern void text_mode_clear(void);
extern void text_mode_putchar(char c);
extern void text_mode_write(const char* data, int size);
extern void text_mode_writestring(const char* data);
extern void text_mode_setcolor(vga_color_t fg, vga_color_t bg);
extern void text_mode_scroll(int lines);

extern void framebuffer_console_init(multiboot_info_t* mbi);
extern void framebuffer_clear(void);
extern void framebuffer_putchar(char c);
extern void framebuffer_write(const char* data, int size);
extern void framebuffer_writestring(const char* data);
extern void framebuffer_setcolor(vga_color_t fg, vga_color_t bg);
extern void framebuffer_scroll(int lines);

static struct terminal_driver active_driver;

void terminal_initialize(multiboot_info_t* mbi) {
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 12)) {
        if(mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
            framebuffer_console_init(mbi);
            active_driver.clear = framebuffer_clear;
            active_driver.putchar = framebuffer_putchar;
            active_driver.write = framebuffer_write;
            active_driver.writestring = framebuffer_writestring;
            active_driver.setcolor = framebuffer_setcolor;
            active_driver.scroll = framebuffer_scroll;
            return;
        }
    }

    text_mode_console_init();
    active_driver.clear = text_mode_clear;
    active_driver.putchar = text_mode_putchar;
    active_driver.write = text_mode_write;
    active_driver.writestring = text_mode_writestring;
    active_driver.setcolor = text_mode_setcolor;
    active_driver.scroll = text_mode_scroll;
}

void terminal_clear(void) {
    active_driver.clear();
}

void terminal_putchar(char c) {
    active_driver.putchar(c);
}

void terminal_write(const char* data, int size) {
    active_driver.write(data, size);
}

void terminal_write_string(const char* data) {
    active_driver.writestring(data);
}

void terminal_setcolor(vga_color_t fg, vga_color_t bg) {
    active_driver.setcolor(fg, bg);
}

void terminal_scroll(int lines) {
    active_driver.scroll(lines);
}

void terminal_write_stringf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256]; // A reasonably sized buffer for formatted output
    kuint64_t i = 0;
    while (*format != '\0' && i < sizeof(buffer) - 1) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char* s = va_arg(args, char*);
                while (*s != '\0' && i < sizeof(buffer) - 1) {
                    buffer[i++] = *s++;
                }
            } else if (*format == 'd') {
                kint32_t d = va_arg(args, kint32_t);
                char num_buf[32];
                itoa(num_buf, 'd', d);
                char* s = num_buf;
                while (*s != '\0' && i < sizeof(buffer) - 1) {
                    buffer[i++] = *s++;
                }
            } else if (*format == 'x') {
                kuint32_t x = va_arg(args, kuint32_t);
                char num_buf[32];
                itoa(num_buf, 'x', x);
                char* s = num_buf;
                while (*s != '\0' && i < sizeof(buffer) - 1) {
                    buffer[i++] = *s++;
                }
            } else if (*format == '%') {
                buffer[i++] = '%';
            }
        } else {
            buffer[i++] = *format;
        }
        format++;
    }
    buffer[i] = '\0';

    va_end(args);
    terminal_write_string(buffer);
}

void terminal_write_hex(kuint32_t n) {
    terminal_write_string("0x");
    char buf[9];
    char* hex_chars = "0123456789ABCDEF";
    buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    terminal_write_string(buf);
}