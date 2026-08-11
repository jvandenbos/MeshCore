#include "icons.h"

namespace merlin {
namespace icons {

void repeater(Gfx &g, int16_t x, int16_t y, Color c) {
  for (int16_t i = 0; i < 5; ++i)
    g.hline((int16_t)(x + 4 - i), (int16_t)(y + 2 + i), (int16_t)(1 + 2 * i), c);
}

void companion(Gfx &g, int16_t x, int16_t y, Color c) {
  g.hline((int16_t)(x + 2), (int16_t)(y + 2), 4, c);
  g.hline((int16_t)(x + 2), (int16_t)(y + 7), 4, c);
  vline(g, (int16_t)(x + 1), (int16_t)(y + 3), 4, c);
  vline(g, (int16_t)(x + 6), (int16_t)(y + 3), 4, c);
}

void hopArrow(Gfx &g, int16_t x, int16_t y, Color c) {
  g.hline(x, (int16_t)(y + 2), 5, c);
  g.hline(x, (int16_t)(y + 5), 5, c);
  vline(g, (int16_t)(x + 5), (int16_t)(y + 1), 6, c);
  vline(g, (int16_t)(x + 6), (int16_t)(y + 2), 4, c);
  vline(g, (int16_t)(x + 7), (int16_t)(y + 3), 2, c);
}

void chevronLeft(Gfx &g, int16_t x, int16_t y, Color c) {
  for (int16_t i = 0; i < 4; ++i) {
    g.fillRect(Rect((int16_t)(x + 3 - i), (int16_t)(y + i), 2, 1), c);
    g.fillRect(Rect((int16_t)(x + i), (int16_t)(y + 4 + i), 2, 1), c);
  }
}

void chevronRight(Gfx &g, int16_t x, int16_t y, Color c) {
  for (int16_t i = 0; i < 4; ++i) {
    g.fillRect(Rect((int16_t)(x + i), (int16_t)(y + i), 2, 1), c);
    g.fillRect(Rect((int16_t)(x + 3 - i), (int16_t)(y + 4 + i), 2, 1), c);
  }
}

void sortArrow(Gfx &g, int16_t x, int16_t y, bool up, Color c) {
  for (int16_t i = 0; i < 4; ++i) {
    int16_t row = up ? i : (int16_t)(3 - i);
    g.hline((int16_t)(x + 3 - i), (int16_t)(y + row), (int16_t)(1 + 2 * i), c);
  }
}

void snrBars(Gfx &g, int16_t x, int16_t y, int level, Color on, Color off) {
  static const int16_t kH[3] = {4, 7, 10};
  for (int16_t i = 0; i < 3; ++i)
    g.fillRect(Rect((int16_t)(x + i * 4), (int16_t)(y + 10 - kH[i]), 2, kH[i]),
               i < level ? on : off);
}

void battery(Gfx &g, int16_t x, int16_t y, uint8_t pct, Color outline,
             Color fill) {
  if (pct > 100) pct = 100;
  frameRect(g, Rect(x, y, 14, 8), outline);
  g.fillRect(Rect((int16_t)(x + 14), (int16_t)(y + 2), 2, 4), outline);
  int16_t w = (int16_t)((12 * pct) / 100);
  if (w > 0) g.fillRect(Rect((int16_t)(x + 1), (int16_t)(y + 1), w, 6), fill);
}

void ackQueued(Gfx &g, int16_t x, int16_t y, Color c) {
  g.fillRect(Rect((int16_t)(x + 3), (int16_t)(y + 4), 2, 2), c);
}

void ackSent(Gfx &g, int16_t x, int16_t y, Color c) {
  g.hline(x, (int16_t)(y + 4), 7, c);
  vline(g, (int16_t)(x + 5), (int16_t)(y + 2), 5, c);
  vline(g, (int16_t)(x + 6), (int16_t)(y + 3), 3, c);
}

void ackDelivered(Gfx &g, int16_t x, int16_t y, Color c) {
  for (int16_t i = 0; i < 3; ++i)
    g.fillRect(Rect((int16_t)(x + i), (int16_t)(y + 3 + i), 2, 1), c);
  for (int16_t i = 0; i < 5; ++i)
    g.fillRect(Rect((int16_t)(x + 3 + i), (int16_t)(y + 5 - i), 2, 1), c);
}

void ackRepeat(Gfx &g, int16_t x, int16_t y, Color c) {
  g.hline((int16_t)(x + 1), (int16_t)(y + 1), 5, c);
  g.hline((int16_t)(x + 1), (int16_t)(y + 7), 5, c);
  vline(g, x, (int16_t)(y + 2), 5, c);
  vline(g, (int16_t)(x + 6), (int16_t)(y + 4), 3, c);
  g.fillRect(Rect((int16_t)(x + 4), y, 3, 3), c);
}

void envelope(Gfx &g, int16_t x, int16_t y, Color c) {
  frameRect(g, Rect(x, y, 11, 8), c);
  for (int16_t i = 0; i < 4; ++i) {
    g.fillRect(Rect((int16_t)(x + 1 + i), (int16_t)(y + 1 + i), 1, 1), c);
    g.fillRect(Rect((int16_t)(x + 9 - i), (int16_t)(y + 1 + i), 1, 1), c);
  }
}

void chargeArrow(Gfx &g, int16_t x, int16_t y, Color c) {
  for (int16_t i = 0; i < 4; ++i)
    g.hline((int16_t)(x + 3 - i), (int16_t)(y + i), (int16_t)(1 + 2 * i), c);
  g.fillRect(Rect((int16_t)(x + 2), (int16_t)(y + 4), 3, 3), c);
}

void dot(Gfx &g, int16_t x, int16_t y, bool filled, Color c) {
  if (filled)
    g.fillRect(Rect(x, y, 5, 5), c);
  else
    frameRect(g, Rect(x, y, 5, 5), c);
}

} // namespace icons
} // namespace merlin
