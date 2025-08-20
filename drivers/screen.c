#include <drivers/screen.h>
#include <drivers/terminal.h>
#include <fonts/font.h>
#include <fonts/cascadiamono.h>

static physical_addr_t* framebuffer;
static kuint32_t pitch;
static kuint32_t width;
static kuint32_t height;

// Initializes a screen, right now this will also initialize a terminal
// which is used to display information strings
void screen_init(multiboot_info_t* mbi) {
    bool useFramebuffer = CHECK_MULTIBOOT_FLAG(mbi->flags, 12) && mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
    if(useFramebuffer) {
        // Store the physical address for VMM mapping
        framebuffer = (physical_addr_t*)((physical_addr_t)mbi->framebuffer_addr);
        pitch = mbi->framebuffer_pitch;
        width = mbi->framebuffer_width;
        height = mbi->framebuffer_height;
    }
    terminal_init(mbi);
}

// Clears the screen if we have a framebuffer. The VGA text mode terminal
// handles all of the clearing on its own.
void screen_clear(color_t color) {
    if (!framebuffer) return;

    // Iterate over every pixel on the screen.
    for(kuint32_t y = 0; y < height; y++) {
        for(kuint32_t x = 0; x < width; x++) {
            kuint32_t* pixel = framebuffer + (y * (pitch / 4)) + x;
            *pixel = color_to_uint(color);
        }
    }
}

kuint32_t color_to_uint(color_t color) {
    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

static color_t uint_to_color(kuint32_t color_val) {
    color_t color;
    color.a = (color_val >> 24) & 0xFF;
    color.r = (color_val >> 16) & 0xFF;
    color.g = (color_val >> 8) & 0xFF;
    color.b = color_val & 0xFF;
    return color;
}

// Draws a character to the screen. To do this we need font data, as well as a framebuffer
// where we are drawing the character.
void screen_draw_char(char c, kuint32_t x, kuint32_t y, kuint32_t color_val) {
    if (!framebuffer) return;

    // Get the current font we are using. For now are using a static font.
    const Font* font = &g_cascadia_mono_font;
    kuint8_t char_height = font->asciiHeight;
    kuint8_t char_width = font->asciiWidths[(kint32_t)c];
    const kuint8_t* glyph = font->asciiGlyphs[(kint32_t)c];
    color_t text_color = uint_to_color(color_val);

    // For X..Y in the framebuffer for the size of the character we have,
    // draw the pixels for the character based on the font
    for (kuint8_t y_offset = 0; y_offset < char_height; y_offset++) {
        for (kuint8_t x_offset = 0; x_offset < char_width; x_offset++) {

            // Gives the intensity of the pixel at this point for the font
            kuint8_t intensity = glyph[y_offset * char_width + x_offset];

            // Only draw if the pixel is not fully transparent.
            if (intensity > 0) {
                kuint32_t screen_x = x + x_offset;
                kuint32_t screen_y = y + y_offset;

                // Bounds check to ensure we don't draw outside the framebuffer.
                if (screen_x < width && screen_y < height) {
                    kuint32_t* pixel_addr = framebuffer + (screen_y * (pitch / 4)) + screen_x;

                    // Alpha blending: FinalColor = TextColor * alpha + BgColor * (1 - alpha)
                    color_t bg_color = uint_to_color(*pixel_addr);
                    kuint8_t r = (text_color.r * intensity + bg_color.r * (255 - intensity)) / 255;
                    kuint8_t g = (text_color.g * intensity + bg_color.g * (255 - intensity)) / 255;
                    kuint8_t b = (text_color.b * intensity + bg_color.b * (255 - intensity)) / 255;

                    *pixel_addr = color_to_uint((color_t){r, g, b, 255});
                }
            }
        }
    }
}