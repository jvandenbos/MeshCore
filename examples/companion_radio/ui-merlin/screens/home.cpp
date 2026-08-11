#include "home.h"

#include "../textutils.h"
#include "common.h"

#include <stdio.h>

namespace merlin {
namespace screens {

using namespace theme;

// Mute removed 2026-08-09: no sound subsystem exists yet (T-Deck speaker is
// I2S, not a buzzer) — a toggle that silences nothing is a lie. Returns with
// the audio milestone. Real per-chat mute lives in Chats ('m').
static constexpr Rect kBtn[2] = {Rect(4, 174, 312, 18), Rect(4, 196, 312, 18)};

void HomeScreen::fire(Router &r, CommandSink *cmd, UiModel &m) {
  (void)m;
  switch (action_) {
    case 0:
      if (cmd) cmd->sendAdvert();
      r.showBanner("ADVERT", "zero-hop advert sent", Banner::NO_CONVO);
      break;
    case 1:
      // 30 . 45 . 60 . 75 . 90 . 100, then round to 30. The last step is
      // clamped rather than allowed to land on 105: a percentage above 100 is
      // not just cosmetic, it underflows the backlight driver's level maths and
      // the screen goes dark, which reads as a crash.
      if (bright_ >= 100) bright_ = 30;
      else bright_ = (uint8_t)(bright_ + 15 > 100 ? 100 : bright_ + 15);
      if (cmd) cmd->setBrightness(bright_);
      break;
    default:
      break;
  }
}

bool HomeScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                         CommandSink *cmd) {
  if (e.kind == InputEvent::NAV) {
    // Vertical moves selection, horizontal is hierarchy — even for a row of
    // buttons. Giving the quick actions L/R would eat the root tab ring.
    if (e.nav == InputEvent::D) { if (action_ < 1) ++action_; return true; }
    if (e.nav == InputEvent::U) { if (action_ > 0) --action_; return true; }
    if (e.nav == InputEvent::CLICK) { fire(r, cmd, m); return true; }
    return false;
  }
  if (e.kind == InputEvent::KEY) {
    if (e.ch == 'a') { action_ = 0; fire(r, cmd, m); return true; }
    if (e.ch == '\n') { fire(r, cmd, m); return true; }
    return false;
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP) {
    for (int i = 0; i < 2; ++i)
      if (e.ty >= kBtn[i].y && e.ty < kBtn[i].bottom() && e.tx >= kBtn[i].x &&
          e.tx < kBtn[i].right()) {
        action_ = i;
        fire(r, cmd, m);
        return true;
      }
  }
  return false;
}

void HomeScreen::render(Gfx &g, UiModel &m) {
  DeviceView d = m.device();
  StatusView s = m.status();
  char buf[64];

  g.text(4, 17, d.name, F_BOLD, ACCENT);
  snprintf(buf, sizeof(buf), "fw %s", d.fw);
  textRight(g, 316, 21, buf, F_SMALL, MUTED);

  g.hline(0, 36, SCREEN_W, RULE);
  g.text(4, 39, "RADIO", F_SMALL, MUTED);
  snprintf(buf, sizeof(buf), "%s MHz", d.freq);
  g.text(4, 49, buf, F_BODY, FG);
  snprintf(buf, sizeof(buf), "SF%u . CR%u", (unsigned)d.sf, (unsigned)d.cr);
  g.text(160, 49, buf, F_BODY, MUTED);
  snprintf(buf, sizeof(buf), "BW %s kHz", d.bw);
  g.text(4, 64, buf, F_BODY, FG);
  snprintf(buf, sizeof(buf), "TX %d dBm", (int)d.tx_dbm);
  g.text(160, 64, buf, F_BODY, MUTED);

  g.hline(0, 81, SCREEN_W, RULE);
  g.text(4, 84, "POWER", F_SMALL, MUTED);
  icons::battery(g, 4, 96, s.batt_pct, FG, s.batt_pct <= 15 ? ALERT : OK);
  snprintf(buf, sizeof(buf), "%u%%", (unsigned)s.batt_pct);
  g.text(26, 94, buf, F_BODY, OK);
  snprintf(buf, sizeof(buf), "%u.%02u V", (unsigned)(s.batt_mv / 1000),
           (unsigned)((s.batt_mv % 1000) / 10));
  g.text(72, 94, buf, F_BODY, FG);
  if (s.charging) {
    icons::chargeArrow(g, 150, 97, OK);
    g.text(162, 94, "charging", F_BODY, OK);
  }

  // LINKS + TRAFFIC compressed to one row each (first-light 2026-08-09): the
  // reclaimed height lets the quick actions stack as full-width rows, so the
  // U/D selection finally matches what the eye sees (same fix as node card).
  g.hline(0, 112, SCREEN_W, RULE);
  g.text(4, 115, "LINKS", F_SMALL, MUTED);
  const char *gps = s.gps == GPS_FIX ? "3D fix"
                                     : (s.gps == GPS_SEARCH ? "searching" : "off");
  snprintf(buf, sizeof(buf), "GPS %s . %u sats", gps, (unsigned)s.sats);
  g.text(4, 126, buf, F_BODY, s.gps == GPS_FIX ? OK : MUTED);
  g.text(180, 126, s.ble ? "BLE linked" : "BLE idle", F_BODY,
         s.ble ? ACCENT : MUTED);

  g.hline(0, 143, SCREEN_W, RULE);
  g.text(4, 146, "TRAFFIC", F_SMALL, MUTED);
  snprintf(buf, sizeof(buf), "RX %u", (unsigned)d.rx_today);
  g.text(72, 146, buf, F_BODY, FG);
  snprintf(buf, sizeof(buf), "TX %u", (unsigned)d.tx_today);
  g.text(130, 146, buf, F_BODY, FG);
  snprintf(buf, sizeof(buf), "%u nodes / 24h", (unsigned)d.nodes_24h);
  g.text(196, 146, buf, F_SMALL, MUTED);
  g.hline(0, 160, SCREEN_W, RULE);

  char bright[16];
  snprintf(bright, sizeof(bright), "Bright %u%%", (unsigned)bright_);
  button(g, kBtn[0], "Advert", action_ == 0);
  button(g, kBtn[1], bright, action_ == 1);
}

} // namespace screens
} // namespace merlin
