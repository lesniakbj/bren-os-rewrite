#include <libc/strings.h>
#include <drivers/terminal.h>
#include <kernel/multiboot.h>
#include <drivers/screen.h>
#include <fonts/font.h>

static physical_addr_t* framebuffer;
static kuint32_t pitch;
static kuint32_t width;
static kuint32_t height;
static kuint16_t cursor_x;
static kuint16_t cursor_y;
static bool is_new_line = true;
static color_t current_color;

// Scrollback buffer
static char scrollback_buffer[SCROLLBACK_BUFFER_SIZE][256];
static kint32_t scrollback_head = 0;
static kint32_t scrollback_tail = 0;
static kint32_t scroll_offset = 0;

void framebuffer_console_init(multiboot_info_t* mbi) {
    if(CHECK_MULTIBOOT_FLAG(mbi->flags, 12)) {
        if(mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
            // Store the physical address for VMM mapping
            framebuffer = (physical_addr_t*)((virtual_addr_t)mbi->framebuffer_addr);
            pitch = mbi->framebuffer_pitch;
            width = mbi->framebuffer_width;
            height = mbi->framebuffer_height;
            current_color = (color_t){255, 0, 0, 0};
        }
    }
    is_new_line = true;
}

void framebuffer_clear() {
    screen_clear(current_color);
}

void framebuffer_putchar(char c) {
    // Ensure we have a valid framebuffer before we attempt to draw a character
    if (framebuffer) {

        // Advance to a new line if we have a newline character, keep circular scrollback buffer in bounds
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

        // Handle tab characters ensuring we are tabbing to tab stops
        if (c == '\t') {
            // Calculate how many pixels to advance, snap cursor_x to the next tab stop
            kint32_t tab_stop_pixels = TAB_WIDTH * FONT_WIDTH;
            cursor_x = ((cursor_x / tab_stop_pixels) + 1) * tab_stop_pixels;

            // Handle line wrap if the tab pushes the cursor past the screen width
            if (cursor_x >= width) {
                cursor_x = 0;
                cursor_y += FONT_HEIGHT;
                is_new_line = true;
            }
            return;
        }

        // ... otherwise handle the drawing of this character
        screen_draw_char(c, cursor_x, cursor_y, color_to_uint(current_color));
        cursor_x += FONT_WIDTH;

        // Handle line wrap if the tab pushes the cursor past the screen width
        if (cursor_x >= width) {
            cursor_x = 0;
            cursor_y += FONT_HEIGHT;
            is_new_line = true;
        }
    }
}

// TODO: This can probably be removed...
// Writes a string to the framebuffer, delegates each character to the putchar function
kint32_t framebuffer_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        framebuffer_putchar(data[i]);
    }
    return size;
}

// Writes a string to the framebuffer, delegates each character to the putchar function
void framebuffer_writestring(const char* data) {
    kint32_t i = 0;
    while (data[i] != 0) {
    	framebuffer_putchar(data[i++]);
    }
}

void framebuffer_setcolor(vga_color_t fg, vga_color_t bg) {
    (void)fg;
    (void)bg;
    // Not implemented for framebuffer yet
}

void framebuffer_scroll() {
    // TODO: Implement scrolling...
}