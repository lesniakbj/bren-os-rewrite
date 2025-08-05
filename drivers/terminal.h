#ifndef TERMINAL_H
#define TERMINAL_H

#include <libc/stdlib.h>
#include <multiboot.h>
#include <libc/stdint.h>

typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

struct terminal_driver {
    void (*clear)(void);
    void (*putchar)(char c);
    void (*write)(const char* data, int size);
    void (*writestring)(const char* data);
    void (*setcolor)(vga_color_t fg, vga_color_t bg);
};

void terminal_initialize(multiboot_info_t* mbi);
void terminal_clear(void);
void terminal_putchar(char c);
void terminal_write(const char* data, int size);
void terminal_writestring(const char* data);
void terminal_setcolor(vga_color_t fg, vga_color_t bg);
void terminal_writestringf(const char* format, ...);
void terminal_write_hex(kuint32_t n);

void text_mode_console_init(void);
void framebuffer_console_init(multiboot_info_t* mbi);

#endif //TERMINAL_H
