#ifndef KERNEL_SCREEN_H
#define KERNEL_SCREEN_H

#include <libc/stdint.h>
#include <kernel/multiboot.h>

typedef struct {
    kuint8_t r;
    kuint8_t g;
    kuint8_t b;
    kuint8_t a;
} color_t;

void screen_init(multiboot_info_t* mbi);
void screen_clear(color_t color);
void screen_draw_pixel(kuint32_t x, kuint32_t y, kuint32_t color);
void screen_draw_rect(kuint32_t x, kuint32_t y, kuint32_t rect_width, kuint32_t rect_height, kuint32_t color);
void screen_draw_line(kuint32_t x1, kuint32_t y1, kuint32_t x2, kuint32_t y2, kuint32_t color);
void screen_draw_char(char c, kuint32_t x, kuint32_t y, kuint32_t color_val);
kuint32_t color_to_uint(color_t color);

#endif