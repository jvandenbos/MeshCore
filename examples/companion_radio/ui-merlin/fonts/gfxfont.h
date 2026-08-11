// Vendored from Adafruit-GFX-Library (gfxfont.h), BSD licence.
// Identical layout to the upstream struct so the vendored font tables in this
// directory are byte-compatible with the ones the device build would use.
#ifndef MERLIN_GFXFONT_H
#define MERLIN_GFXFONT_H

#include <stdint.h>

typedef struct {
  uint16_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset;
} GFXglyph;

typedef struct {
  uint8_t *bitmap;
  GFXglyph *glyph;
  uint16_t first;
  uint16_t last;
  uint8_t yAdvance;
} GFXfont;

#endif // MERLIN_GFXFONT_H
