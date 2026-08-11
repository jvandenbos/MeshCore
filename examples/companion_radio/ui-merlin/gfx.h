// MERLIN UI — the only thing a screen may do to pixels.
// Platform-pure C++17: no Arduino, no ESP-IDF — enforced by `make nopollute`.
#pragma once

#include <stdint.h>

namespace merlin {

struct Rect {
  int16_t x = 0, y = 0, w = 0, h = 0;
  constexpr Rect() = default;
  constexpr Rect(int16_t x_, int16_t y_, int16_t w_, int16_t h_)
      : x(x_), y(y_), w(w_), h(h_) {}
  constexpr int16_t right() const { return (int16_t)(x + w); }
  constexpr int16_t bottom() const { return (int16_t)(y + h); }
};

typedef uint16_t Color; // RGB565, constants in theme.h

enum FontId : uint8_t {
  F_BODY = 0, // FreeSans9pt7b   — proportional, ~36 chars/line
  F_BOLD = 1, // FreeSansBold9pt7b
  F_SMALL = 2 // 5x7 fixed       — status bar, labels, snippets
};

struct Gfx {
  virtual ~Gfx() {}
  virtual int16_t width() const = 0;  // 320
  virtual int16_t height() const = 0; // 240
  virtual void fillRect(Rect r, Color c) = 0;
  virtual void hline(int16_t x, int16_t y, int16_t w, Color c) = 0;
  // (x, y) is the top-left of the text cell; the renderer applies the font
  // baseline itself so callers never juggle font metrics.
  virtual void text(int16_t x, int16_t y, const char *s, FontId f, Color c) = 0;
  virtual int16_t textWidth(const char *s, FontId f) = 0;
  virtual void clip(Rect r) = 0;
  virtual void unclip() = 0;
};

// Convenience helpers built on the six primitives (header-only, no state).
inline void vline(Gfx &g, int16_t x, int16_t y, int16_t h, Color c) {
  g.fillRect(Rect(x, y, 1, h), c);
}
inline void frameRect(Gfx &g, Rect r, Color c) {
  g.hline(r.x, r.y, r.w, c);
  g.hline(r.x, (int16_t)(r.bottom() - 1), r.w, c);
  vline(g, r.x, r.y, r.h, c);
  vline(g, (int16_t)(r.right() - 1), r.y, r.h, c);
}
inline void textRight(Gfx &g, int16_t right_x, int16_t y, const char *s,
                      FontId f, Color c) {
  g.text((int16_t)(right_x - g.textWidth(s, f)), y, s, f, c);
}

} // namespace merlin
