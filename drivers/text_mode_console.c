#include <drivers/terminal.h>
#include <kernel/multiboot.h>
#include <drivers/screen.h>
#include <arch/i386/io.h>

#define TEXT_MODE_SCROLLBACK_ROWS 1000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static kuint16_t* vga_buffer;
static char scrollback_buffer[TEXT_MODE_SCROLLBACK_ROWS][VGA_WIDTH];
static int scrollback_head = 0;
static int scroll_offset = 0;
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
	vga_buffer[index] = vga_entry(c, color);
}

static void text_mode_update_cursor(void) {
	kuint16_t pos = terminal_row * VGA_WIDTH + terminal_column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (kuint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (kuint8_t) ((pos >> 8) & 0xFF));
}

static void text_mode_redraw() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        int scrollback_row = (scrollback_head - scroll_offset - (VGA_HEIGHT - 1) + y + TEXT_MODE_SCROLLBACK_ROWS) % TEXT_MODE_SCROLLBACK_ROWS;
        for (int x = 0; x < VGA_WIDTH; x++) {
            text_mode_putentryat(scrollback_buffer[scrollback_row][x], terminal_color, x, y);
        }
    }
    text_mode_update_cursor();
}

void text_mode_console_init(void) {
	terminal_row = VGA_HEIGHT - 1;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	vga_buffer = (kuint16_t*) 0xB8000;
    for (int i = 0; i < TEXT_MODE_SCROLLBACK_ROWS; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
            scrollback_buffer[i][j] = ' ';
        }
    }
	text_mode_redraw();
	is_new_line = true;
}

void text_mode_setcolor(vga_color_t fg, vga_color_t bg) {
	terminal_color = vga_entry_color(fg, bg);
}

void text_mode_clear(void) {
    for (int i = 0; i < TEXT_MODE_SCROLLBACK_ROWS; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
            scrollback_buffer[i][j] = ' ';
        }
    }
    scrollback_head = 0;
    scroll_offset = 0;
    text_mode_redraw();
}

void text_mode_putchar(char c) {
    if (is_new_line) {
        scrollback_buffer[scrollback_head][terminal_column++] = '>';
        scrollback_buffer[scrollback_head][terminal_column++] = ' ';
        is_new_line = false;
    }

	if (c == '\n') {
		terminal_column = 0;
		scrollback_head = (scrollback_head + 1) % TEXT_MODE_SCROLLBACK_ROWS;
        is_new_line = true;
	} else {
		scrollback_buffer[scrollback_head][terminal_column] = c;
		if (++terminal_column == VGA_WIDTH) {
			terminal_column = 0;
			scrollback_head = (scrollback_head + 1) % TEXT_MODE_SCROLLBACK_ROWS;
            is_new_line = true;
		}
	}

    if (is_new_line) {
        scrollback_buffer[scrollback_head][terminal_column++] = '>';
        scrollback_buffer[scrollback_head][terminal_column++] = ' ';
        is_new_line = false;
    }

    scroll_offset = 0; // Always show latest input
    text_mode_redraw();
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

void text_mode_scroll(int lines) {
    scroll_offset += lines;
    if (scroll_offset < 0) {
        scroll_offset = 0;
    }
    if (scroll_offset > scrollback_head) {
        scroll_offset = scrollback_head;
    }
    text_mode_redraw();
}