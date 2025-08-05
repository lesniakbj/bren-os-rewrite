#include <screen.h>
#include <font.h>
#include <cascadiamono.h>
#include <terminal.h>

static kuint32_t* framebuffer;
static kuint32_t pitch;
static kuint32_t width;
static kuint32_t height;

void screen_init(multiboot_info_t* mbi) {
    bool useFramebuffer = CHECK_MULTIBOOT_FLAG(mbi->flags, 12) && mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
    if(useFramebuffer) {
        framebuffer = (kuint32_t*)((virtual_addr_t)mbi->framebuffer_addr);
        pitch = mbi->framebuffer_pitch;
        width = mbi->framebuffer_width;
        height = mbi->framebuffer_height;
    }
    terminal_initialize(mbi);
}

void screen_clear(color_t color) {
    if (!framebuffer) return;
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

void screen_draw_char(char c, kuint32_t x, kuint32_t y, kuint32_t color_val) {
    if (!framebuffer) return;
    const Font* font = &g_cascadia_mono_font;
    kuint8_t char_height = font->asciiHeight;
    kuint8_t char_width = font->asciiWidths[(int)c];
    const kuint8_t* glyph = font->asciiGlyphs[(int)c];
    color_t text_color = uint_to_color(color_val);

    for (kuint8_t y_offset = 0; y_offset < char_height; y_offset++) {
        for (kuint8_t x_offset = 0; x_offset < char_width; x_offset++) {
            kuint8_t intensity = glyph[y_offset * char_width + x_offset];

            if (intensity > 0) {
                kuint32_t screen_x = x + x_offset;
                kuint32_t screen_y = y + y_offset;

                if (screen_x < width && screen_y < height) {
                    kuint32_t* pixel_addr = framebuffer + (screen_y * (pitch / 4)) + screen_x;
                    
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