#ifndef KERNEL_SCREEN_H
#define KERNEL_SCREEN_H

#include <libc/stdint.h>
#include <multiboot.h>

// ==============
//  Frame Buffer
// ==============
typedef struct {
    kuint8_t r;
    kuint8_t g;
    kuint8_t b;
    kuint8_t a;
} color_t;

void screen_init(multiboot_info_t* mbi);
void screen_clear(color_t color);
void screen_draw_char(char c, kuint32_t x, kuint32_t y, kuint32_t color_val);
kuint32_t color_to_uint(color_t color);

#endif