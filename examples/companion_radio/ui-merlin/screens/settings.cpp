#include "settings.h"

#include "../textutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

namespace merlin {
namespace screens {

using namespace theme;

static constexpr int16_t ROW_H = 30;

// CHANNELS is a top-level group: channel edits are frequent and
// destructive-adjacent, so they earn their own row (mockup frame 7 ruling).
enum Group : int8_t {
  G_IDENTITY = 0, G_RADIO, G_CHANNELS, G_DISPLAY, G_ALERTS, G_SYSTEM, G_COUNT
};
static const char *kGroupName[G_COUNT] = {"IDENTITY", "RADIO",  "CHANNELS",
                                          "DISPLAY",  "ALERTS", "SYSTEM"};

static void groupState(Group gp, UiModel &m, char *out, size_t cap) {
  DeviceView d = m.device();
  switch (gp) {
    case G_IDENTITY:
      snprintf(out, cap, "%s . advert every 4h", d.name);
      break;
    case G_RADIO:
      snprintf(out, cap, "%s . BW%s . SF%u . %d dBm", d.freq, d.bw,
               (unsigned)d.sf, (int)d.tx_dbm);
      break;
    case G_CHANNELS: {
      // The group line is the channel list itself, so Settings never disagrees
      // with what Channels shows.
      size_t w = 0;
      out[0] = 0;
      for (int i = 0; i < m.channelCount() && w + 1 < cap; ++i) {
        int n = snprintf(out + w, cap - w, "%s%s", w ? " . " : "",
                         m.channel(i).name);
        if (n < 0) break;
        w += (size_t)n;
      }
      if (w == 0) snprintf(out, cap, "no channels");
      break;
    }
    case G_DISPLAY:
      snprintf(out, cap, "bright 70%% . off 30s . lock on");
      break;
    case G_ALERTS:
      snprintf(out, cap, "buzzer on . DM+chan . advert off");
      break;
    case G_SYSTEM:
      snprintf(out, cap, "fw %s . up %s . heap %uk", d.fw, d.uptime,
               (unsigned)(d.free_heap / 1024));
      break;
    default:
      out[0] = 0;
      break;
  }
}

void SettingsScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  list_.setRowHeight(ROW_H);
  list_.setArea(Rect(0, CONTENT_Y, SCREEN_W, CONTENT_H));
  rebuild(m);
}

void SettingsScreen::rebuild(UiModel &m) {
  row_count_ = 0;
  char state[80];
  for (int i = 0; i < G_COUNT; ++i) {
    if (list_.filterLen()) {
      groupState((Group)i, m, state, sizeof(state));
      if (text::findFold(kGroupName[i], list_.filter()) < 0 &&
          text::findFold(state, list_.filter()) < 0)
        continue;
    }
    rows_[row_count_++] = (int8_t)i;
  }
  hidden_ = G_COUNT - row_count_;
  list_.setCount(row_count_);
}

bool SettingsScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                             CommandSink *cmd) {
  (void)cmd;
  if (list_.onEvent(e)) return true;

  // R belongs to the tab ring at the root (see nodes.cpp for the reasoning);
  // open is click / Enter / tap.
  bool open = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
              (e.kind == InputEvent::KEY && e.ch == '\n') ||
              (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
               list_.hitTest(e.tx, e.ty) == list_.sel());
  if (open && row_count_ > 0) {
    if (rows_[list_.sel()] == G_IDENTITY) r.push(SC_IDENTITY, 0);
    if (rows_[list_.sel()] == G_RADIO) r.push(SC_RADIO, 0);
    if (rows_[list_.sel()] == G_CHANNELS) r.push(SC_CHANNELS, 0);
    return true;
  }

  if (e.kind != InputEvent::KEY) return false;
  if (!list_.filterOpen() && routerOwnsKey(e.ch, true)) return false;
  bool changed = false;
  if (list_.onFilterKey(e.ch, changed)) {
    if (changed) rebuild(m);
    return true;
  }
  return false;
}

void SettingsScreen::render(Gfx &g, UiModel &m) {
  char state[80], buf[32];
  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    Group gp = (Group)rows_[i];
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());
    if (sel) g.fillRect(rr, ACCENT);

    g.text(6, (int16_t)(rr.y + 3), kGroupName[gp], sel ? F_BOLD : F_BODY,
           sel ? SEL_FG : FG);
    groupState(gp, m, state, sizeof(state));
    g.clip(Rect(6, rr.y, 292, rr.h));
    g.text(6, (int16_t)(rr.y + 20), state, F_SMALL, sel ? SEL_MUTED : MUTED);
    g.unclip();
    icons::chevronRight(g, 302, (int16_t)(rr.y + 10), sel ? SEL_FG : MUTED);
    if (!sel && i < list_.lastVisible())
      g.hline(0, (int16_t)(rr.bottom() - 1), SCREEN_W, RULE);
  }

  int16_t hint_y = (int16_t)(CONTENT_Y + row_count_ * ROW_H + 4);
  if (hint_y < 206) {
    hint(g, hint_y, "R / Enter opens . type to filter");
    hint(g, (int16_t)(hint_y + 10), "L back to Nodes . 0-3 jumps tabs");
  }
  if (list_.filterOpen()) {
    g.fillRect(Rect(0, 199, SCREEN_W, 13), FILTER_BG);
    g.text(4, 202, "filter:", F_SMALL, ACCENT);
    g.text(50, 202, list_.filter(), F_SMALL, FG);
    snprintf(buf, sizeof(buf), "x%d hidden", hidden_);
    textRight(g, 314, 202, buf, F_SMALL, WARN);
  }
  list_.drawScrollbar(g);
}

// --- radio editor ---------------------------------------------------------

static const char *kFieldName[5] = {"Frequency", "Bandwidth", "Spread factor",
                                    "Coding rate", "TX power"};
static const char *kFieldUnit[5] = {"MHz", "kHz", "", "", "dBm"};

// The 915 MHz ISM band, which is also what the hint line on this screen
// advertises — the two are one rule, stated twice.
static constexpr float FREQ_MIN_MHZ = 902.0f;
static constexpr float FREQ_MAX_MHZ = 928.0f;

// Hand-rolled rather than strtod(): ui-merlin may only use freestanding-class
// headers, and a scan of our own lets a second '.' be an error instead of being
// quietly ignored the way the library call would.
static bool parseFreqMHz(const char *s, float &out) {
  if (!s || !*s) return false;
  float whole = 0.0f, frac = 0.0f, scale = 1.0f;
  bool seen_digit = false, seen_dot = false;
  for (const char *p = s; *p; ++p) {
    if (*p == '.') {
      if (seen_dot) return false;
      seen_dot = true;
      continue;
    }
    if (*p < '0' || *p > '9') return false;
    seen_digit = true;
    if (seen_dot) {
      scale *= 0.1f;
      frac += (float)(*p - '0') * scale;
    } else {
      whole = whole * 10.0f + (float)(*p - '0');
    }
  }
  if (!seen_digit) return false;
  out = whole + frac;
  return true;
}

static bool freqInBand(const char *s) {
  float mhz = 0.0f;
  if (!parseFreqMHz(s, mhz)) return false;
  return mhz >= FREQ_MIN_MHZ && mhz <= FREQ_MAX_MHZ;
}

void RadioScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  field_ = 0;
  unsaved_ = false;
  freq_error_ = false;
  DeviceView d = m.device();
  snprintf(freq_, sizeof(freq_), "%s", d.freq);
  snprintf(orig_freq_, sizeof(orig_freq_), "%s", d.freq);
  // Copied, not aliased: the strings in a DeviceView are only promised to live
  // until the next frame, and these have to survive as far as the confirm.
  snprintf(bw_, sizeof(bw_), "%s", d.bw);
  sf_ = d.sf;
  cr_ = d.cr;
  tx_dbm_ = d.tx_dbm;
}

bool RadioScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                          CommandSink *cmd) {
  (void)m; (void)cmd;
  if (e.kind == InputEvent::NAV) {
    if (e.nav == InputEvent::U) { if (field_ > 0) --field_; return true; }
    if (e.nav == InputEvent::D) { if (field_ < 4) ++field_; return true; }
    if (e.nav == InputEvent::CLICK) {
      if (unsaved_) {
        // Rejected in place, the way a short join key is: an empty or
        // out-of-band field would be applied as 0.0 MHz and saved, and the node
        // would come back off air after a reboot with nothing to say why.
        if (!freqInBand(freq_)) {
          freq_error_ = true;
          return true;
        }
        char detail[48];
        snprintf(detail, sizeof(detail), "%s -> %s MHz", orig_freq_, freq_);
        Confirm::Spec s;
        s.title = "APPLY RADIO CHANGE?";
        s.detail = detail;
        s.warn = "Radio restarts. Links drop ~3 s.";
        r.showConfirm(s, this, 0);
      }
      return true;
    }
    return false;
  }
  if (e.kind == InputEvent::KEY) {
    if (field_ == 0 && ((e.ch >= '0' && e.ch <= '9') || e.ch == '.')) {
      size_t n = strlen(freq_);
      if (n + 1 < sizeof(freq_)) {
        freq_[n] = e.ch;
        freq_[n + 1] = 0;
        unsaved_ = true;
        freq_error_ = false; // editing is the answer to the complaint
      }
      return true;
    }
    if (e.ch == '\b') {
      size_t n = strlen(freq_);
      if (field_ == 0 && n > 0) {
        freq_[n - 1] = 0;
        unsaved_ = true;
        freq_error_ = false;
        return true;
      }
      return false;
    }
  }
  return false;
}

void RadioScreen::onConfirm(int action, bool accepted, CommandSink *cmd) {
  (void)action;
  if (!accepted) return;
  // Frequency from the editor, everything else exactly as it was found: this
  // screen only offers to change one number, so it must not quietly rewrite the
  // other four on its way past.
  if (cmd) cmd->setRadio(freq_, bw_, sf_, cr_, tx_dbm_);
  unsaved_ = false;
  freq_error_ = false;
  snprintf(orig_freq_, sizeof(orig_freq_), "%s", freq_);
}

void RadioScreen::render(Gfx &g, UiModel &m) {
  DeviceView d = m.device();
  titleStrip(g, "RADIO", unsaved_ ? "unsaved" : "", WARN);

  char value[5][16];
  snprintf(value[0], sizeof(value[0]), "%s", freq_);
  snprintf(value[1], sizeof(value[1]), "%s", d.bw);
  snprintf(value[2], sizeof(value[2]), "%u", (unsigned)d.sf);
  snprintf(value[3], sizeof(value[3]), "%u", (unsigned)d.cr);
  snprintf(value[4], sizeof(value[4]), "%d", (int)d.tx_dbm);

  for (int i = 0; i < 5; ++i) {
    int16_t y = (int16_t)(32 + i * 22);
    bool sel = (i == field_);
    // The field under edit IS the selection bar — one visual language for
    // "selected" and "being edited".
    if (sel) g.fillRect(Rect(176, y, 104, 18), ACCENT);
    g.text(6, (int16_t)(y + 1), kFieldName[i], F_BODY, sel ? FG : MUTED);
    g.text(182, (int16_t)(y + 1), value[i], sel ? F_BOLD : F_BODY,
           sel ? SEL_FG : FG);
    if (sel)
      vline(g, (int16_t)(184 + g.textWidth(value[i], F_BOLD)), (int16_t)(y + 2),
            14, BG);
    if (*kFieldUnit[i])
      g.text(286, (int16_t)(y + 4), kFieldUnit[i], F_SMALL, sel ? FG : MUTED);
  }
  if (freq_error_) {
    char err[64];
    snprintf(err, sizeof(err), "%.1f-%.1f MHz only - this radio stays put",
             (double)FREQ_MIN_MHZ, (double)FREQ_MAX_MHZ);
    g.fillRect(Rect(0, 146, SCREEN_W, 14), rgb(0x38, 0x0E, 0x10));
    g.text(6, 149, err, F_SMALL, ALERT);
    hint(g, 162, "digits edit . Bksp edits");
  } else {
    hint(g, 148, "digits edit . U/D next field . 902.0-928.0");
    if (!unsaved_) hint(g, 160, "click applies (confirm first)");
  }
}

// --- identity editor -------------------------------------------------------

void IdentityScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  entry_.set(m.device().name);
}

bool IdentityScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                             CommandSink *cmd) {
  bool apply = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
               (e.kind == InputEvent::KEY && e.ch == '\n');
  if (apply) {
    if (!entry_.empty() && strcmp(entry_.text(), m.device().name) != 0) {
      if (cmd) cmd->setName(entry_.text());
      r.showBanner("IDENTITY", "name set . advert to announce it",
                   Banner::NO_CONVO);
    }
    r.pop();
    return true;
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
      e.ty < CONTENT_Y + STRIP_H) {
    r.pop();
    return true;
  }
  if (e.kind == InputEvent::KEY) {
    if (e.ch == '\b' && entry_.empty()) return false; // router: back
    return entry_.onKey(e.ch);
  }
  return false;
}

void IdentityScreen::render(Gfx &g, UiModel &m) {
  g.fillRect(Rect(0, CONTENT_Y, SCREEN_W, STRIP_H), HEADER_BG);
  g.hline(0, (int16_t)(CONTENT_Y + STRIP_H - 1), SCREEN_W, HAIRLINE);
  icons::chevronLeft(g, 4, (int16_t)(CONTENT_Y + 3), ACCENT);
  g.text(14, (int16_t)(CONTENT_Y + 3), "IDENTITY", F_SMALL, ACCENT);

  g.text(4, 40, "This name rides every advert and", F_BODY, MUTED);
  g.text(4, 55, "prefixes your channel messages.", F_BODY, MUTED);

  entry_.render(g, Rect(0, 80, SCREEN_W, 20), "name:", FG);
  char buf[48];
  snprintf(buf, sizeof(buf), "current: %s", m.device().name);
  g.text(4, 110, buf, F_SMALL, MUTED);

  hint(g, 140, "Enter applies . Backspace-empty backs out");
}

} // namespace screens
} // namespace merlin
