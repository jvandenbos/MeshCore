#include "node_card.h"

#include "../textutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

namespace merlin {
namespace screens {

using namespace theme;

// Full-width stacked rows (first-light feedback 2026-08-09): the actions are
// picked with U/D, so they must READ as a vertical list — the old side-by-side
// layout made eyes say L/R while the grammar required U/D.
static constexpr Rect kMsgBtn(4, 185, 312, 18);
static constexpr Rect kPingBtn(4, 205, 312, 18);

void NodeCardScreen::onEnter(int arg, UiModel &m) {
  (void)m;
  node_ = arg;
  action_ = 0;
}

bool NodeCardScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                             CommandSink *cmd) {
  (void)m;
  if (e.kind == InputEvent::NAV) {
    // U/D picks the action, L is always back — the card never traps you and
    // horizontal keeps its single meaning everywhere in the UI.
    if (e.nav == InputEvent::D) { action_ = 1; return true; }
    if (e.nav == InputEvent::U) { action_ = 0; return true; }
    if (e.nav == InputEvent::CLICK) { runAction(r, m, cmd); return true; }
    return false; // L = back, R = nothing deeper to open
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP) {
    if (e.ty >= kMsgBtn.y && e.ty < kMsgBtn.bottom()) {
      action_ = 0; runAction(r, m, cmd); return true;
    }
    if (e.ty >= kPingBtn.y && e.ty < kPingBtn.bottom()) {
      action_ = 1; runAction(r, m, cmd); return true;
    }
    if (e.ty < CONTENT_Y + STRIP_H) { r.pop(); return true; }
  }
  if (e.kind == InputEvent::KEY && e.ch == '\n') {
    runAction(r, m, cmd);
    return true;
  }
  return false;
}

void NodeCardScreen::runAction(Router &r, UiModel &m, CommandSink *cmd) {
  if (action_ == 1) {
    if (cmd) cmd->pingNode(node_);
    // The request really goes out, but repeaters only answer status reqs
    // after admin login — say what happened, promise nothing.
    r.showBanner("PING", "status req sent . reply not guaranteed",
                 Banner::NO_CONVO);
    return;
  }
  // Message: open (or create) the DM and land in the conversation. Was a
  // dead button until first light found it.
  if (cmd) cmd->openDm(node_);
  const char *name = m.node(node_).name;
  for (int i = 0; i < m.convoCount(); ++i) {
    if (strcmp(m.convo(i).name, name) != 0) continue;
    r.pop();
    r.push(SC_THREAD, i);
    return;
  }
}

void NodeCardScreen::render(Gfx &g, UiModel &m) {
  NodeView n = m.node(node_);
  DeviceView d = m.device();
  char buf[64], val[80];

  // --- title strip: kind glyph + name + hop badge --------------------------
  g.fillRect(Rect(0, CONTENT_Y, SCREEN_W, STRIP_H), HEADER_BG);
  g.hline(0, (int16_t)(CONTENT_Y + STRIP_H - 1), SCREEN_W, HAIRLINE);
  icons::chevronLeft(g, 4, (int16_t)(CONTENT_Y + 3), ACCENT);
  if (n.kind == NK_REPEATER)
    icons::repeater(g, 12, (int16_t)(CONTENT_Y + 2), ACCENT);
  else
    icons::companion(g, 12, (int16_t)(CONTENT_Y + 2), ACCENT);
  g.clip(Rect(24, CONTENT_Y, 240, STRIP_H));
  g.text(24, (int16_t)(CONTENT_Y + 3), n.name, F_SMALL, ACCENT);
  g.unclip();
  if (n.flood_hops >= 0) {
    icons::hopArrow(g, 288, (int16_t)(CONTENT_Y + 3), n.direct ? OK : MUTED);
    snprintf(buf, sizeof(buf), "%d", n.flood_hops);
    g.text(300, (int16_t)(CONTENT_Y + 4), buf, F_SMALL, n.direct ? OK : MUTED);
  }

  // --- identity ------------------------------------------------------------
  if (n.pubkey6)
    snprintf(buf, sizeof(buf), "%02x%02x%02x%02x %02x%02x", n.pubkey6[0],
             n.pubkey6[1], n.pubkey6[2], n.pubkey6[3], n.pubkey6[4], n.pubkey6[5]);
  else
    snprintf(buf, sizeof(buf), "unknown");
  field(g, 34, "KEY", buf);
  snprintf(buf, sizeof(buf), "%s%s%s", n.kind == NK_REPEATER ? "repeater" : "companion",
           n.fw && *n.fw ? " . fw " : "", n.fw ? n.fw : "");
  field(g, 51, "ROLE", buf);
  g.hline(0, 68, SCREEN_W, RULE);

  // --- position ------------------------------------------------------------
  if (n.has_pos) {
    snprintf(buf, sizeof(buf), "%.4f, %.4f", (double)n.lat, (double)n.lon);
    field(g, 74, "POS", buf);
    char dist[24], brg[24];
    if (d.has_pos) {
      text::formatDistance(text::distanceKm(d.lat, d.lon, n.lat, n.lon), dist,
                           sizeof(dist));
      text::formatBearing(text::bearingDeg(d.lat, d.lon, n.lat, n.lon), brg,
                          sizeof(brg));
    } else {
      dist[0] = brg[0] = 0;
    }
    if (n.alt_m > 0)
      snprintf(val, sizeof(val), "%s . %s . %dm", dist, brg, (int)n.alt_m);
    else
      snprintf(val, sizeof(val), "%s . %s", dist, brg);
    field(g, 91, "RANGE", val);
  } else {
    field(g, 74, "POS", "no position advertised", MUTED);
    field(g, 91, "RANGE", "unknown", MUTED);
  }
  g.hline(0, 108, SCREEN_W, RULE);

  // --- link ----------------------------------------------------------------
  text::formatAgeLong(n.last_heard_secs, buf, sizeof(buf));
  snprintf(val, sizeof(val), "%s . %s", buf, n.direct ? "direct" : "relayed");
  field(g, 114, "HEARD", val, n.direct ? FG : MUTED);

  // Both numbers, per the locked spec: audible vs reachable.
  if (n.route_hops >= 0)
    snprintf(val, sizeof(val), "heard %d . route %d", (int)n.flood_hops,
             (int)n.route_hops);
  else
    snprintf(val, sizeof(val), "heard %d . route none", (int)n.flood_hops);
  field(g, 131, "HOPS", val, n.route_hops >= 0 ? FG : WARN);

  // The closest-hop rule, stated out loud rather than implied by a blank.
  if (n.direct) {
    text::formatSnr(n.snr_x4, buf, sizeof(buf));
    g.text(4, 151, "SIGNAL", F_SMALL, MUTED);
    g.text(66, 148, buf, F_BODY, OK);
    snprintf(buf, sizeof(buf), "%d dBm", (int)n.rssi);
    g.text(160, 148, buf, F_BODY, MUTED);
  } else {
    field(g, 148, "SIGNAL", "n/a (relayed)", MUTED);
  }
  field(g, 165, "POWER", n.power && *n.power ? n.power : "not reported",
        n.power && *n.power ? FG : MUTED);
  g.hline(0, 182, SCREEN_W, RULE);

  button(g, kMsgBtn, "Message", action_ == 0);
  button(g, kPingBtn, "Ping", action_ == 1);
  // hint dropped: stacked rows read as the list they are
}

} // namespace screens
} // namespace merlin
