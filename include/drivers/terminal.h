#ifndef TERMINAL_H
#define TERMINAL_H

#include <libc/stdlib.h>
#include <kernel/multiboot.h>
#include <libc/stdint.h>

#define SCROLLBACK_BUFFER_SIZE 1000

#define TAB_WIDTH 4
#define FONT_WIDTH 8
#define FONT_HEIGHT 16

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

struct terminal_driver {
    void (*clear)(void);
    void (*putchar)(char c);
    kint32_t (*write)(const char* data, size_t size);
    void (*writestring)(const char* data);
    void (*setcolor)(vga_color_t fg, vga_color_t bg);
    void (*scroll)(kint32_t lines);
};

void terminal_init(multiboot_info_t* mbi);
void framebuffer_console_init(multiboot_info_t* mbi);
void text_mode_console_init();

void terminal_clear();

void terminal_putchar(char c);
kint32_t terminal_write(const char* data, size_t size);
void terminal_write_string(const char* data);

void terminal_setcolor(vga_color_t fg, vga_color_t bg);

void terminal_scroll(kint32_t lines);

#endif //TERMINAL_H
