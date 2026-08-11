// MERLIN UI — chrome every screen shares. Keeps "learn once, used everywhere"
// literal: one function draws a title strip, one draws a hint line.
#pragma once

#include "../gfx.h"
#include "../icons.h"
#include "../theme.h"

namespace merlin {
namespace screens {

// 14 px strip under the status bar: back chevron + title, optional right note.
inline void titleStrip(Gfx &g, const char *title, const char *note,
                       Color note_c = theme::MUTED) {
  using namespace theme;
  g.fillRect(Rect(0, CONTENT_Y, SCREEN_W, STRIP_H), HEADER_BG);
  g.hline(0, (int16_t)(CONTENT_Y + STRIP_H - 1), SCREEN_W, HAIRLINE);
  icons::chevronLeft(g, 4, (int16_t)(CONTENT_Y + 3), ACCENT);
  g.text(12, (int16_t)(CONTENT_Y + 3), title, F_SMALL, ACCENT);
  if (note && *note) textRight(g, 314, (int16_t)(CONTENT_Y + 3), note, F_SMALL, note_c);
}

// Bottom-of-content hint line, muted 5x7.
inline void hint(Gfx &g, int16_t y, const char *s) {
  g.text(4, y, s, F_SMALL, theme::MUTED);
}

// Label/value readout row — the one idiom for static fields (Home, node card).
inline void field(Gfx &g, int16_t y, const char *label, const char *value,
                  Color value_c = theme::FG) {
  g.text(4, (int16_t)(y + 3), label, F_SMALL, theme::MUTED);
  g.text(66, y, value, F_BODY, value_c);
}

// Tab digits stay router-level while the filter buffer is empty; once you are
// typing a filter, every printable key extends it. This is the same precedence
// rule that makes "Bksp = back when no filter text" coherent.
inline bool routerOwnsKey(char c, bool filter_empty) {
  return filter_empty && c >= '0' && c <= '3';
}

inline void button(Gfx &g, Rect r, const char *label, bool focused) {
  using namespace theme;
  g.fillRect(r, focused ? ACCENT : BG);
  frameRect(g, r, focused ? ACCENT : rgb(0x4A, 0x4A, 0x4A));
  FontId f = focused ? F_BOLD : F_BODY;
  int16_t tw = g.textWidth(label, f);
  g.text((int16_t)(r.x + (r.w - tw) / 2), (int16_t)(r.y + 1), label, f,
         focused ? BG : FG);
}

} // namespace screens
} // namespace merlin
