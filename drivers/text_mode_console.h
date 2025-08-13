#ifndef _TEXT_MODE_CONSOLE_H
#define _TEXT_MODE_CONSOLE_H

#include <drivers/screen.h>

void text_mode_console_init(void);
void text_mode_setcolor(vga_color_t fg, vga_color_t bg);
void text_mode_putchar(char c);
void text_mode_clear(void);
void text_mode_write(const char* data, int size);
void text_mode_writestring(const char* data);
void text_mode_scroll(int lines);
void text_mode_write_at(int row, int col, const char* str, int len, kuint8_t color);
void text_mode_console_refresh(void);

#endif // _TEXT_MODE_CONSOLE_H
