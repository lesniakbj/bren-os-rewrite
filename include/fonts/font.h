#ifndef FONT_H
#define FONT_H

#include <libc/stdint.h>

// A pretty basic font renderer.
//
// Various fonts can be implemented by defining their
// pixel values in a header.
//
// Each character will have different widths, but all of
// them will have a normalized height.

// The main font structure. All members are read-only.
typedef struct Font {
    const kuint8_t *const *asciiGlyphs;
    const kuint8_t *       asciiWidths;
    kuint8_t               asciiHeight;
    kuint8_t               maxWidth;
    kuint8_t               fontSize;
    const char *     fontName;
} Font;

#endif