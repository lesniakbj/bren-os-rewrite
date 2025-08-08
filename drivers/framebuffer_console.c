#include <drivers/terminal.h>
#include <kernel/multiboot.h>
#include <drivers/screen.h>
#include <fonts/font.h>

static kuint32_t* framebuffer;
static kuint32_t pitch;
static kuint32_t width;
static kuint32_t height;
static kuint16_t cursor_x;
static kuint16_t cursor_y;
static bool is_new_line = true;

#define SCROLLBACK_BUFFER_SIZE 1000 // Number of lines to keep in scrollback

static color_t current_color;

// Scrollback buffer
static char scrollback_buffer[SCROLLBACK_BUFFER_SIZE][256];
static int scrollback_head = 0;
static int scrollback_tail = 0;
static int scroll_offset = 0;

void framebuffer_console_init(multiboot_info_t* mbi) {
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 12)) {
        if(mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
            framebuffer = (kuint32_t*)((virtual_addr_t)mbi->framebuffer_addr);
            pitch = mbi->framebuffer_pitch;
            width = mbi->framebuffer_width;
            height = mbi->framebuffer_height;
            current_color = (color_t){255, 0, 0, 0};
        }
    }
    is_new_line = true;
    //framebuffer_clear();
}

static void framebuffer_maybe_prefix() {
    if (is_new_line) {
        screen_draw_char('>', cursor_x, cursor_y, color_to_uint(current_color));
        cursor_x += FONT_WIDTH;
        screen_draw_char(' ', cursor_x, cursor_y, color_to_uint(current_color));
        cursor_x += FONT_WIDTH;
        is_new_line = false;
    }
}

void framebuffer_clear(void) {
    color_t reset_color = (color_t){255, 0, 0, 0};
    //screen_clear(reset_color);
}

void framebuffer_putchar(char c) {
    framebuffer_maybe_prefix();
    // This is a graphical console, so we need to draw the character.
    // We'll need a font for this.
    // For now, let's just put a pixel.
    if (framebuffer) {
        if (c == '\n') {
            cursor_x = 0;
            cursor_y += FONT_HEIGHT;
            is_new_line = true;
            scrollback_head = (scrollback_head + 1) % SCROLLBACK_BUFFER_SIZE;
            if (scrollback_head == scrollback_tail) {
                scrollback_tail = (scrollback_tail + 1) % SCROLLBACK_BUFFER_SIZE;
            }
            memset(scrollback_buffer[scrollback_head], 0, 256);
            return;
        }

        screen_draw_char(c, cursor_x, cursor_y, color_to_uint(current_color));
        cursor_x += 8; // Assuming a font width of 8
        if (cursor_x >= width) {
            cursor_x = 0;
            cursor_y += FONT_HEIGHT;
            is_new_line = true;
        }
    }
}

void framebuffer_write(const char* data, int size) {
	for (int i = 0; i < size; i++) {
		framebuffer_putchar(data[i]);
	}
}

void framebuffer_writestring(const char* data) {
    int i = 0;
    while (data[i] != 0) {
    	framebuffer_putchar(data[i++]);
    }
}

void framebuffer_setcolor(vga_color_t fg, vga_color_t bg) {
    // Not implemented for framebuffer yet
}

void framebuffer_scroll() {

}