#include "banner.h"

#include "../icons.h"
#include "../theme.h"

#include <stdio.h>
#include <string.h>

namespace merlin {

static void copyBounded(char *dst, size_t cap, const char *src) {
  if (!src) { dst[0] = 0; return; }
  size_t n = strlen(src);
  if (n >= cap) n = cap - 1;
  memcpy(dst, src, n);
  dst[n] = 0;
}

void Banner::show(const char *who, const char *text, int convo, uint32_t now) {
  copyBounded(who_, sizeof(who_), who);
  copyBounded(text_, sizeof(text_), text);
  convo_ = convo;
  shown_ = now;
  active_ = true;
}

void Banner::tick(uint32_t now) {
  if (active_ && now - shown_ >= LIFETIME) active_ = false;
}

bool Banner::hit(int16_t x, int16_t y) const {
  (void)x;
  return active_ && y >= theme::STATUS_H && y < theme::STATUS_H + HEIGHT;
}

void Banner::render(Gfx &g, uint32_t now) const {
  using namespace theme;
  if (!active_) return;
  const int16_t y = STATUS_H;
  g.fillRect(Rect(0, y, SCREEN_W, HEIGHT), FILTER_BG);
  g.hline(0, (int16_t)(y + HEIGHT - 1), SCREEN_W, ACCENT);
  g.fillRect(Rect(0, y, 3, HEIGHT), ACCENT);

  icons::envelope(g, 8, (int16_t)(y + 3), ACCENT);
  g.text(24, (int16_t)(y + 3), who_, F_SMALL, ACCENT);
  int16_t x = (int16_t)(24 + g.textWidth(who_, F_SMALL) + 8);
  g.clip(Rect(x, y, (int16_t)(288 - x), HEIGHT));
  g.text(x, (int16_t)(y + 3), text_, F_SMALL, FG);
  g.unclip();

  uint32_t left = LIFETIME - (now - shown_ > LIFETIME ? LIFETIME : now - shown_);
  char buf[8];
  snprintf(buf, sizeof(buf), "%us", (unsigned)left);
  textRight(g, 314, (int16_t)(y + 3), buf, F_SMALL, ACCENT);
}

} // namespace merlin
