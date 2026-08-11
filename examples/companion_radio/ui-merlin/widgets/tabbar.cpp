#include "tabbar.h"

#include "../theme.h"

namespace merlin {
namespace tabbar {

const char *const LABELS[4] = {"HOME", "CHATS", "NODES", "SETTG"};

void render(Gfx &g, int active, bool chats_unread) {
  using namespace theme;
  const int16_t y = (int16_t)(SCREEN_H - TAB_H);
  g.fillRect(Rect(0, y, SCREEN_W, TAB_H), BG);
  g.hline(0, y, SCREEN_W, HAIRLINE);

  for (int i = 0; i < 4; ++i) {
    int16_t x = (int16_t)(i * TAB_W);
    bool on = (i == active);
    if (on) g.fillRect(Rect(x, (int16_t)(y + 1), TAB_W, (int16_t)(TAB_H - 1)), ACCENT);
    int16_t tw = g.textWidth(LABELS[i], F_SMALL);
    g.text((int16_t)(x + (TAB_W - tw) / 2), (int16_t)(y + 4), LABELS[i], F_SMALL,
           on ? BG : MUTED);
    if (i == 1 && chats_unread)
      g.fillRect(Rect((int16_t)(x + TAB_W - 14), (int16_t)(y + 4), 4, 4),
                 on ? BG : ALERT);
  }
}

int hitTest(int16_t x, int16_t y) {
  if (y < theme::SCREEN_H - theme::TAB_H || y >= theme::SCREEN_H) return -1;
  int i = x / TAB_W;
  return (i >= 0 && i < 4) ? i : -1;
}

} // namespace tabbar
} // namespace merlin
