// drivers/text_mode_console.c

#include <drivers/terminal.h>
#include <drivers/text_mode_console.h> // Include the new header
#include <kernel/multiboot.h>
#include <drivers/screen.h>
#include <arch/i386/io.h>
#include <kernel/time.h>
#include <libc/strings.h>

#define TEXT_MODE_SCROLLBACK_ROWS 1000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Make internal variables accessible to functions in this file and others that include the header
kuint16_t* vga_buffer;
static kuint16_t back_buffer[VGA_WIDTH * VGA_HEIGHT];
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

static void text_mode_update_cursor(void) {
	kuint16_t pos = terminal_row * VGA_WIDTH + terminal_column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (kuint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (kuint8_t) ((pos >> 8) & 0xFF));
}

void text_mode_console_refresh(void) {
    // 1. Draw scrollback content to back_buffer
    for (int y = 0; y < VGA_HEIGHT; y++) {
        int scrollback_row = (scrollback_head - scroll_offset - (VGA_HEIGHT - 1) + y + TEXT_MODE_SCROLLBACK_ROWS) % TEXT_MODE_SCROLLBACK_ROWS;
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            back_buffer[index] = vga_entry(scrollback_buffer[scrollback_row][x], terminal_color);
        }
    }

    // 2. Draw clock on top of back_buffer
    int clock_len = strlen(console_time_buffer);
    int clock_start_col = 57;
    for (int i = 0; i < clock_len; i++) {
        if (clock_start_col + i < VGA_WIDTH) {
            const int index = 0 * VGA_WIDTH + (clock_start_col + i);
            back_buffer[index] = vga_entry(console_time_buffer[i], console_clock_color);
        }
    }

    // 3. Copy only changed parts from back_buffer to vga_buffer
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        if (back_buffer[i] != vga_buffer[i]) {
            vga_buffer[i] = back_buffer[i];
        }
    }

    // 4. Update cursor
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
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        back_buffer[i] = vga_entry(' ', terminal_color);
    }
	text_mode_console_refresh();
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
    text_mode_console_refresh();
}

void text_mode_putchar(char c) {
    // --- Process the Character ---
    if (c == '\t') {
        const int TAB_WIDTH = 4; // Define your desired tab width

        // Calculate the column of the next tab stop based on the absolute column
        // This ensures alignment to global tab stops (e.g., columns 0, 8, 16, 24, ...)
        int next_tab_stop = ((terminal_column / TAB_WIDTH) + 1) * TAB_WIDTH;

        // Fill with spaces up to the next tab stop, but do not exceed line width
        while (terminal_column < next_tab_stop && terminal_column < VGA_WIDTH) {
            scrollback_buffer[scrollback_head][terminal_column] = ' ';
            terminal_column++;
        }

        // Check for line wrap after filling spaces
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            scrollback_head = (scrollback_head + 1) % TEXT_MODE_SCROLLBACK_ROWS;
            is_new_line = true; // Next char will be at the start of a new line
        }

    } else if (c == '\n') {
        // Handle newline: Move to start of next line
        terminal_column = 0;
        scrollback_head = (scrollback_head + 1) % TEXT_MODE_SCROLLBACK_ROWS;
        is_new_line = true; // Next char will be at the start of a new line

    } else {
        // Handle regular character
        scrollback_buffer[scrollback_head][terminal_column] = c;
        terminal_column++;

        // Check for line wrap after placing the character
        if (terminal_column >= VGA_WIDTH) {
            terminal_column = 0;
            scrollback_head = (scrollback_head + 1) % TEXT_MODE_SCROLLBACK_ROWS;
            is_new_line = true; // Next char will be at the start of a new line
        }
    }
    // --- End of Character Processing ---

    // --- Update Display ---
    scroll_offset = 0; // Always show the latest output
    text_mode_console_refresh(); // Refresh the VGA buffer
    // --- End of Display Update ---
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
    text_mode_console_refresh();
}


// --- NEW FUNCTION ---
void text_mode_write_at(int row, int col, const char* str, int len, kuint8_t color) {
    // Validate row and column
    if (row < 0 || row >= VGA_HEIGHT || col < 0 || col >= VGA_WIDTH) {
        return; // Invalid coordinates
    }

    // Determine the actual number of characters to write
    int chars_to_write = 0;
    if (len < 0) {
        // Write until null terminator or end of line
        chars_to_write = 0;
        while (str[chars_to_write] != '\0' && (col + chars_to_write) < VGA_WIDTH) {
            chars_to_write++;
        }
    } else {
        // Write up to 'len' characters or end of line, whichever comes first
        chars_to_write = (len < (VGA_WIDTH - col)) ? len : (VGA_WIDTH - col);
    }

    // Write characters directly to both buffers to maintain sync
    for (int i = 0; i < chars_to_write; i++) {
        kuint16_t entry = vga_entry(str[i], color);
        int index = row * VGA_WIDTH + col + i;
        vga_buffer[index] = entry;
        back_buffer[index] = entry;
    }
}
// --- END NEW FUNCTION ---
