#include "statusbar.h"

#include "../icons.h"
#include "../textutils.h"
#include "../theme.h"

#include <stdio.h>

namespace merlin {
namespace statusbar {

void render(Gfx &g, const StatusView &s) {
  using namespace theme;
  g.fillRect(Rect(0, 0, SCREEN_W, STATUS_H), BG);
  g.hline(0, (int16_t)(STATUS_H - 1), SCREEN_W, HAIRLINE);

  Color batt_c = s.batt_pct <= 15 ? ALERT : (s.charging ? OK : FG);
  icons::battery(g, 3, 3, s.batt_pct, FG, batt_c);

  char buf[16];
  snprintf(buf, sizeof(buf), "%u%%", (unsigned)s.batt_pct);
  g.text(22, 3, buf, F_SMALL, batt_c);

  if (s.gps != GPS_OFF)
    g.text(48, 3, "GPS", F_SMALL, s.gps == GPS_FIX ? OK : WARN);
  if (s.ble) g.text(74, 3, "BLE", F_SMALL, ACCENT);

  // RX/TX heartbeat: green = heard, cyan = sent. The radio's pulse (rule 4).
  g.fillRect(Rect(100, 6, 3, 3), s.rx_blip ? OK : rgb(0x10, 0x38, 0x20));
  g.fillRect(Rect(106, 6, 3, 3), s.tx_blip ? ACCENT : rgb(0x12, 0x52, 0x5C));

  if (s.unread_total > 0) {
    snprintf(buf, sizeof(buf), "%u", (unsigned)s.unread_total);
    int16_t w = (int16_t)(g.textWidth(buf, F_SMALL) + 8);
    g.fillRect(Rect((int16_t)(276 - w), 2, w, 10), ALERT);
    g.text((int16_t)(280 - w), 3, buf, F_SMALL, BG);
  }

  text::formatClock(s.clock, buf, sizeof(buf));
  textRight(g, 317, 3, buf, F_SMALL, FG);
}

} // namespace statusbar
} // namespace merlin
