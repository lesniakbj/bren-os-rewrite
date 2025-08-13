// drivers/text_mode_console.h

#ifndef DRIVERS_TEXT_MODE_CONSOLE_H
#define DRIVERS_TEXT_MODE_CONSOLE_H

#include <libc/stdint.h>

/**
 * @brief Writes a string directly to a specific position on the VGA text screen.
 *
 * This function bypasses the standard terminal scrolling and newline handling
 * to allow for in-place updates, such as a status line or clock.
 *
 * @param row The 0-based row index (0 to VGA_HEIGHT-1).
 * @param col The 0-based column index (0 to VGA_WIDTH-1).
 * @param str The null-terminated string to write.
 * @param len The number of characters to write from the string.
 *            If negative, the entire string (up to null terminator) is written,
 *            or until the end of the line (VGA_WIDTH) is reached.
 * @param color The VGA color attribute to use for the text.
 */
void text_mode_write_at(int row, int col, const char* str, int len, kuint8_t color);

#endif // DRIVERS_TEXT_MODE_CONSOLE_H