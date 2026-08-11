#include "splash.h"

#include "common.h"

#include <stdio.h>

// The owner line on the boot splash. Builders put their own callsign/QTH here
// with -D MERLIN_SPLASH_TAG='"..."' in their env; the default claims nobody.
#ifndef MERLIN_SPLASH_TAG
#define MERLIN_SPLASH_TAG "open firmware . MIT"
#endif

namespace merlin {
namespace screens {

using namespace theme;

bool SplashScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                           CommandSink *cmd) {
  (void)e; (void)r; (void)m; (void)cmd;
  return false; // the router drops any input straight into Chats
}

void SplashScreen::render(Gfx &g, UiModel &m) {
  DeviceView d = m.device();
  char buf[48];

  // Mesh glyph: three lit nodes over a dim lattice, all rectangles so it stays
  // crisp at 1x on the panel.
  const int16_t nx[4] = {158, 124, 192, 158};
  const int16_t ny[4] = {32, 54, 54, 74};
  const Color dim = rgb(0x1F, 0x64, 0x70);
  for (int a = 0; a < 4; ++a)
    for (int b = a + 1; b < 4; ++b) {
      int16_t x0 = nx[a], y0 = ny[a], x1 = nx[b], y1 = ny[b];
      int steps = (x1 - x0 > 0 ? x1 - x0 : x0 - x1) +
                  (y1 - y0 > 0 ? y1 - y0 : y0 - y1);
      for (int s = 0; s <= steps; ++s)
        g.fillRect(Rect((int16_t)(x0 + (x1 - x0) * s / steps),
                        (int16_t)(y0 + (y1 - y0) * s / steps), 1, 1),
                   dim);
    }
  for (int a = 0; a < 3; ++a)
    g.fillRect(Rect((int16_t)(nx[a] - 3), (int16_t)(ny[a] - 3), 6, 6), ACCENT);
  g.fillRect(Rect((int16_t)(nx[3] - 4), (int16_t)(ny[3] - 4), 8, 8), FG);

  int16_t tw = g.textWidth("MERLIN", F_BOLD);
  g.text((int16_t)((SCREEN_W - tw) / 2), 100, "MERLIN", F_BOLD, ACCENT);
  const char *sub = "MeshCore . T-Deck Plus";
  g.text((int16_t)((SCREEN_W - g.textWidth(sub, F_SMALL)) / 2), 124, sub,
         F_SMALL, MUTED);

  g.hline(0, 152, SCREEN_W, RULE);
  snprintf(buf, sizeof(buf), "fw %s", d.fw);
  g.text(60, 162, buf, F_SMALL, MUTED);
  snprintf(buf, sizeof(buf), "%s MHz", d.freq);
  textRight(g, 260, 162, buf, F_SMALL, MUTED);

  frameRect(g, Rect(60, 186, 200, 8), rgb(0x2A, 0x2A, 0x2A));
  g.fillRect(Rect(61, 187, (int16_t)(198 * progress_ / 100), 6), ACCENT);
  g.text(60, 200, "loading contacts...", F_SMALL, MUTED);
  g.text(60, 220, MERLIN_SPLASH_TAG, F_SMALL, DIM);
}

} // namespace screens
} // namespace merlin
