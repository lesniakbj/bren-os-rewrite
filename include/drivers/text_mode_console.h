#ifndef _TEXT_MODE_CONSOLE_H
#define _TEXT_MODE_CONSOLE_H

#include <drivers/screen.h>
#include <drivers/terminal.h>

void text_mode_console_init();
void text_mode_setcolor(vga_color_t fg, vga_color_t bg);
void text_mode_putchar(char c);
void text_mode_clear();
kint32_t text_mode_write(const char* data, size_t size);
void text_mode_writestring(const char* data);
void text_mode_scroll(kint32_t lines);
void text_mode_write_at(kint32_t row, kint32_t col, const char* str, kint32_t len, kuint8_t color);
void text_mode_console_refresh(void);

#endif
