#include <drivers/terminal.h>
#include <kernel/multiboot.h>
#include <drivers/screen.h>

static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static kuint16_t* terminal_buffer;
static int terminal_row;
static int terminal_column;
static kuint8_t terminal_color;
static bool is_new_line = true;

static inline kuint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
	return fg | bg << 4;
}

static inline kuint16_t vga_entry(unsigned char uc, kuint8_t color) {
	return (kuint16_t) uc | (kuint16_t) color << 8;
}

void text_mode_putentryat(char c, kuint8_t color, int x, int y) {
	const int index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

static void text_mode_maybe_prefix() {
    if (is_new_line) {
        text_mode_putentryat('>', terminal_color, terminal_column, terminal_row);
        terminal_column++;
        text_mode_putentryat(' ', terminal_color, terminal_column, terminal_row);
        terminal_column++;
        is_new_line = false;
    }
}

void text_mode_console_init(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (kuint16_t*) 0xB8000;
	for (int y = 0; y < VGA_HEIGHT; y++) {
		for (int x = 0; x < VGA_WIDTH; x++) {
			const int index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
	is_new_line = true;
}

void text_mode_setcolor(vga_color_t fg, vga_color_t bg) {
	terminal_color = vga_entry_color(fg, bg);
}

void text_mode_clear(void) {
    screen_clear((color_t){255, 0, 0, 0});
}

void text_mode_putchar(char c) {
    text_mode_maybe_prefix();
	if (c == '\n') {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			terminal_row = 0;
		}
        is_new_line = true;
		return;
	}
	text_mode_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			terminal_row = 0;
		}
        is_new_line = true;
	}
}

void text_mode_write(const char* data, int size) {
	for (int i = 0; i < size; i++) {
		text_mode_putchar(data[i]);
	}
}

void text_mode_writestring(const char* data) {
	int i = 0;
	while (data[i] != 0) {
		text_mode_putchar(data[i++]);
	}
}
