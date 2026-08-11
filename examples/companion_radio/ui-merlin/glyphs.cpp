#include "glyphs.h"

#include "fonts/FreeSans9pt7b.h"
#include "fonts/FreeSansBold9pt7b.h"
#include "fonts/glcdfont5x7.h"
#include "textutils.h"

namespace merlin {
namespace glyphs {

// --- 5x7 fixed font -------------------------------------------------------
// Column-major: 5 bytes per character, bit j of byte i = pixel (i, j).
constexpr int16_t kSmallW = 5, kSmallH = 7, kSmallAdv = 6, kSmallCell = 8;

// --- proportional GFX fonts ----------------------------------------------
// FreeSans9pt7b caps are 13 px tall with 4 px of descender room. We use a
// tighter cell than the font's own yAdvance (22) because 240 px of screen has
// no room for typographic generosity.
constexpr int16_t kBodyAscent = 13, kBodyCell = 17;

static const GFXfont *gfxFont(FontId f) {
  return f == F_BOLD ? &FreeSansBold9pt7b : &FreeSans9pt7b;
}

int16_t ascent(FontId f) { return f == F_SMALL ? kSmallH : kBodyAscent; }
int16_t cellHeight(FontId f) { return f == F_SMALL ? kSmallCell : kBodyCell; }

// The block glyph the UTF-8 collapse produces: a 50 % checkerboard box, which
// reads as "something was here that this font cannot show" at any size.
static int16_t blockAdvance(FontId f) { return f == F_SMALL ? kSmallAdv : 9; }

static void drawBlock(int16_t x, int16_t y_top, FontId f, SpanSink &sink) {
  int16_t w = (int16_t)(blockAdvance(f) - 1);
  int16_t h = f == F_SMALL ? kSmallH : 10;
  int16_t top = f == F_SMALL ? y_top : (int16_t)(y_top + kBodyAscent - h);
  for (int16_t row = 0; row < h; ++row)
    for (int16_t col = (int16_t)(row & 1); col < w; col += 2)
      sink.span((int16_t)(x + col), (int16_t)(top + row), 1);
}

// The degree ring: drawn, not looked up. See DEGREE in textutils.h.
static void drawDegree(int16_t x, int16_t y_top, FontId f, SpanSink &sink) {
  int16_t top = f == F_SMALL ? y_top : (int16_t)(y_top + kBodyAscent - 12);
  sink.span((int16_t)(x + 1), top, 2);
  sink.span((int16_t)(x + 1), (int16_t)(top + 3), 2);
  sink.span(x, (int16_t)(top + 1), 1);
  sink.span(x, (int16_t)(top + 2), 1);
  sink.span((int16_t)(x + 3), (int16_t)(top + 1), 1);
  sink.span((int16_t)(x + 3), (int16_t)(top + 2), 1);
}

static int16_t advanceOf(char c, FontId f) {
  if (c == text::BLOCK) return blockAdvance(f);
  if (c == text::DEGREE) return 6;
  if (f == F_SMALL) return kSmallAdv;
  const GFXfont *fn = gfxFont(f);
  if ((uint8_t)c < fn->first || (uint8_t)c > fn->last) return 0;
  return fn->glyph[(uint8_t)c - fn->first].xAdvance;
}

static void drawChar(char c, int16_t x, int16_t y_top, FontId f, SpanSink &sink) {
  if (c == text::BLOCK) { drawBlock(x, y_top, f, sink); return; }
  if (c == text::DEGREE) { drawDegree(x, y_top, f, sink); return; }
  if (c == ' ') return;

  if (f == F_SMALL) {
    const unsigned char *col = &glcdfont5x7[(uint8_t)c * 5];
    for (int16_t i = 0; i < kSmallW; ++i) {
      uint8_t bits = col[i];
      for (int16_t j = 0; j < kSmallH; ++j)
        if (bits & (1 << j)) sink.span((int16_t)(x + i), (int16_t)(y_top + j), 1);
    }
    return;
  }

  const GFXfont *fn = gfxFont(f);
  if ((uint8_t)c < fn->first || (uint8_t)c > fn->last) return;
  const GFXglyph &g = fn->glyph[(uint8_t)c - fn->first];
  const uint8_t *bmp = fn->bitmap + g.bitmapOffset;
  int16_t base = (int16_t)(y_top + kBodyAscent);
  uint32_t bit = 0;
  for (uint8_t yy = 0; yy < g.height; ++yy) {
    int16_t py = (int16_t)(base + g.yOffset + yy);
    int16_t run_start = -1;
    for (uint8_t xx = 0; xx < g.width; ++xx, ++bit) {
      bool on = (bmp[bit >> 3] << (bit & 7)) & 0x80;
      if (on && run_start < 0)
        run_start = (int16_t)(x + g.xOffset + xx);
      else if (!on && run_start >= 0) {
        sink.span(run_start, py, (int16_t)(x + g.xOffset + xx - run_start));
        run_start = -1;
      }
    }
    if (run_start >= 0)
      sink.span(run_start, py, (int16_t)(x + g.xOffset + g.width - run_start));
  }
}

int16_t width(const char *s, FontId f) {
  char buf[text::MAX_RENDER];
  size_t n = text::collapseUtf8(s, buf, sizeof(buf));
  int16_t w = 0;
  for (size_t i = 0; i < n; ++i) w = (int16_t)(w + advanceOf(buf[i], f));
  return w;
}

void draw(int16_t x, int16_t y, const char *s, FontId f, SpanSink &sink) {
  char buf[text::MAX_RENDER];
  size_t n = text::collapseUtf8(s, buf, sizeof(buf));
  for (size_t i = 0; i < n; ++i) {
    drawChar(buf[i], x, y, f, sink);
    x = (int16_t)(x + advanceOf(buf[i], f));
  }
}

} // namespace glyphs
} // namespace merlin
