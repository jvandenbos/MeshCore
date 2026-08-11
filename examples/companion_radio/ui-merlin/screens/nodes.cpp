#include "nodes.h"

#include "../textutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

namespace merlin {
namespace screens {

using namespace theme;

// Column stops. The mockups' stops assumed a narrow monospace face; with the
// real 9pt proportional font the metrics are pushed right so an 18-character
// node name still fits without truncation.
static constexpr int16_t NAME_X = 18;
static constexpr int16_t NAME_CLIP_R = 238;
static constexpr int16_t COL_AGE_R = 262;
static constexpr int16_t COL_HOP_X = 268;
static constexpr int16_t COL_SNR_X = 296;
static constexpr int16_t HEADER_H = 13;
static constexpr int16_t ROW_H = 23;

static const char *kSortLabel[NodesScreen::S_COUNT] = {"RECENT", "HOPS",
                                                       "DIRECT", "DIST", "NAME"};
// Natural direction per sort: newest / fewest / strongest / nearest / A-Z.
static const bool kSortUp[NodesScreen::S_COUNT] = {false, true, false, true, true};

static int snrLevel(int16_t snr_x4) {
  if (snr_x4 >= 32) return 3;  // >= +8 dB
  if (snr_x4 >= 0) return 2;   // >= 0 dB
  return 1;
}

// Case-insensitive compare. Not strcasecmp(): that lives in <strings.h>, which
// is POSIX, and ui-merlin/ may only use freestanding-class headers.
static int cmpFold(const char *a, const char *b) {
  for (;; ++a, ++b) {
    char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
    char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
    if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
    if (!ca) return 0;
  }
}

static float nodeDistance(UiModel &m, const NodeView &n) {
  DeviceView d = m.device();
  if (!d.has_pos || !n.has_pos) return 1e9f;
  return text::distanceKm(d.lat, d.lon, n.lat, n.lon);
}

void NodesScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  list_.setRowHeight(ROW_H);
  rebuild(m);
}

bool NodesScreen::matches(const NodeView &n) const {
  if (repeaters_only_ && n.kind != NK_REPEATER) return false;
  if (list_.filterLen() == 0) return true;
  return text::findFold(n.name, list_.filter()) >= 0;
}

void NodesScreen::rebuild(UiModel &m) {
  total_ = m.nodeCount();
  if (total_ > MAX_NODES) total_ = MAX_NODES;

  // Static because MAX_NODES int16_ts is ~1 KB against the device's 8 KB
  // Arduino loop stack. One UI task drives every screen, so nothing else can be
  // inside rebuild() while this is in use.
  static int16_t keep[MAX_NODES];
  int n = 0;
  for (int i = 0; i < total_; ++i)
    if (matches(m.node(i))) keep[n++] = (int16_t)i;
  shown_ = n;
  hidden_ = total_ - n;

  // Insertion sort — the fleet is tens of nodes, and it keeps the comparator
  // readable without dragging <algorithm> or a heap allocation onto the device.
  for (int i = 1; i < n; ++i) {
    int16_t v = keep[i];
    int j = i - 1;
    while (j >= 0) {
      NodeView a = m.node(keep[j]), b = m.node(v);
      bool swap = false;
      switch (sort_) {
        case S_RECENT:
          swap = a.last_heard_secs > b.last_heard_secs;
          break;
        case S_HOPS: {
          // Unknown hop count sinks to the bottom, never hides (design spec).
          int ah = a.flood_hops < 0 ? 999 : a.flood_hops;
          int bh = b.flood_hops < 0 ? 999 : b.flood_hops;
          swap = ah > bh || (ah == bh && a.last_heard_secs > b.last_heard_secs);
          break;
        }
        case S_DIRECT: {
          // Only direct nodes rank by SNR; relayed nodes all sink below the
          // divider, because their SNR describes their last hop, not our link.
          if (a.direct != b.direct)
            swap = !a.direct;
          else if (a.direct)
            swap = a.snr_x4 < b.snr_x4;
          else
            swap = a.last_heard_secs > b.last_heard_secs;
          break;
        }
        case S_DIST: {
          float ad = nodeDistance(m, a), bd = nodeDistance(m, b);
          swap = ad > bd;
          break;
        }
        case S_NAME:
          swap = cmpFold(a.name, b.name) > 0;
          break;
        default:
          break;
      }
      if (!swap) break;
      keep[j + 1] = keep[j];
      --j;
    }
    keep[j + 1] = v;
  }

  row_count_ = 0;
  bool divider_placed = false;
  for (int i = 0; i < n; ++i) {
    if (sort_ == S_DIRECT && !divider_placed && !m.node(keep[i]).direct) {
      rows_[row_count_++] = DIVIDER;
      divider_placed = true;
    }
    rows_[row_count_++] = keep[i];
  }

  int16_t list_y = (int16_t)(CONTENT_Y + HEADER_H + (list_.filterOpen() ? 14 : 1));
  list_.setArea(Rect(0, list_y, SCREEN_W, (int16_t)(SCREEN_H - TAB_H - list_y)));
  list_.setCount(row_count_);
  if (row_count_ > 0 && rows_[list_.sel()] == DIVIDER) list_.moveBy(1);
}

bool NodesScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                          CommandSink *cmd) {
  (void)cmd;
  if (list_.onEvent(e)) {
    // The relayed divider is a label, not a destination.
    if (row_count_ > 0 && rows_[list_.sel()] == DIVIDER)
      list_.moveBy(e.kind == InputEvent::NAV && e.nav == InputEvent::U ? -1 : 1);
    return true;
  }

  // R is NOT an open verb at the root. Rule 1 ("at root, L/R switches sibling
  // tabs") is the primary spatial model, and letting a root list eat R would
  // make the tab ring walkable leftwards only. Open = click / Enter / tap.
  bool open = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
              (e.kind == InputEvent::KEY && e.ch == '\n') ||
              (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
               list_.hitTest(e.tx, e.ty) == list_.sel() && list_.sel() >= 0);
  if (open && row_count_ > 0 && rows_[list_.sel()] != DIVIDER) {
    r.push(SC_NODE_CARD, rows_[list_.sel()]);
    return true;
  }

  if (e.kind != InputEvent::KEY) return false;

  // Verbs own a keystroke only while no filter is open; once one is, every
  // printable key extends it. This is what makes the spec's "Bksp = back when
  // no filter text" rule coherent — and why '/' exists, for the node whose
  // name starts with a verb letter.
  if (!list_.filterOpen()) {
    if (e.ch == 's') {
      sort_ = (Sort)((sort_ + 1) % S_COUNT);
      rebuild(m);
      return true;
    }
    if (e.ch == 't') {
      repeaters_only_ = !repeaters_only_;
      rebuild(m);
      return true;
    }
    if (e.ch == 'n' || e.ch == 'a' || e.ch == 'r' || e.ch == 'm') return false;
    if (routerOwnsKey(e.ch, true)) return false;
  }

  bool changed = false;
  if (list_.onFilterKey(e.ch, changed)) {
    if (changed) rebuild(m);
    return true;
  }
  return false;
}

void NodesScreen::render(Gfx &g, UiModel &m) {
  char buf[48];

  // --- sort header ---------------------------------------------------------
  g.fillRect(Rect(0, CONTENT_Y, SCREEN_W, HEADER_H), HEADER_BG);
  g.text(4, (int16_t)(CONTENT_Y + 3), "NODES", F_SMALL, FG);
  snprintf(buf, sizeof(buf), "%d", shown_);
  g.text(40, (int16_t)(CONTENT_Y + 3), buf, F_SMALL, MUTED);

  g.text(64, (int16_t)(CONTENT_Y + 3), kSortLabel[sort_], F_SMALL, ACCENT);
  icons::sortArrow(g, (int16_t)(66 + g.textWidth(kSortLabel[sort_], F_SMALL)),
                   (int16_t)(CONTENT_Y + 4), kSortUp[sort_], ACCENT);

  if (hidden_ > 0) {
    snprintf(buf, sizeof(buf), "x%d hidden", hidden_);
    g.text(126, (int16_t)(CONTENT_Y + 3), buf, F_SMALL, WARN);
  }
  if (repeaters_only_) g.text(196, (int16_t)(CONTENT_Y + 3), "RPTR", F_SMALL, OK);

  textRight(g, COL_AGE_R, (int16_t)(CONTENT_Y + 3), "AGE", F_SMALL, MUTED);
  g.text(COL_HOP_X, (int16_t)(CONTENT_Y + 3), "HOP", F_SMALL, MUTED);
  g.text(COL_SNR_X, (int16_t)(CONTENT_Y + 3), "SNR", F_SMALL, MUTED);
  g.hline(0, (int16_t)(CONTENT_Y + HEADER_H), SCREEN_W, RULE);

  // --- filter strip --------------------------------------------------------
  if (list_.filterOpen()) {
    const int16_t fy = (int16_t)(CONTENT_Y + HEADER_H + 1);
    g.fillRect(Rect(0, fy, SCREEN_W, 13), FILTER_BG);
    g.text(4, (int16_t)(fy + 3), "filter:", F_SMALL, ACCENT);
    g.text(50, (int16_t)(fy + 3), list_.filter(), F_SMALL, FG);
    vline(g, (int16_t)(51 + g.textWidth(list_.filter(), F_SMALL)),
          (int16_t)(fy + 2), 9, ACCENT);
    snprintf(buf, sizeof(buf), "%d of %d", shown_, total_);
    textRight(g, 314, (int16_t)(fy + 3), buf, F_SMALL, MUTED);
    g.hline(0, (int16_t)(fy + 13), SCREEN_W, RULE);
  }

  // --- rows ----------------------------------------------------------------
  Rect area = list_.area();
  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());

    if (rows_[i] == DIVIDER) {
      int16_t my = (int16_t)(rr.y + rr.h / 2);
      const char *lbl = "relayed (SNR n/a)";
      int16_t tw = g.textWidth(lbl, F_SMALL);
      g.text((int16_t)((SCREEN_W - tw) / 2), (int16_t)(my - 4), lbl, F_SMALL, MUTED);
      g.hline(8, my, (int16_t)((SCREEN_W - tw) / 2 - 16), RULE);
      g.hline((int16_t)((SCREEN_W + tw) / 2 + 8), my,
              (int16_t)((SCREEN_W - tw) / 2 - 16), RULE);
      continue;
    }

    NodeView n = m.node(rows_[i]);
    Color fg = sel ? SEL_FG : FG;
    Color mut = sel ? SEL_MUTED : MUTED;
    if (sel) g.fillRect(rr, ACCENT);

    if (n.kind == NK_REPEATER)
      icons::repeater(g, 4, (int16_t)(rr.y + 7), fg);
    else
      icons::companion(g, 4, (int16_t)(rr.y + 7), fg);

    g.clip(Rect(NAME_X, rr.y, (int16_t)(NAME_CLIP_R - NAME_X), rr.h));
    g.text(NAME_X, (int16_t)(rr.y + 3), n.name, sel ? F_BOLD : F_BODY, fg);
    g.unclip();

    // Show WHY a row matched: underline the matched span on the selected row.
    if (sel && list_.filterLen() > 0) {
      int off = text::findFold(n.name, list_.filter());
      if (off >= 0) {
        char head[64];
        size_t hn = (size_t)off < sizeof(head) - 1 ? (size_t)off : sizeof(head) - 1;
        memcpy(head, n.name, hn);
        head[hn] = 0;
        int16_t x0 = (int16_t)(NAME_X + g.textWidth(head, F_BOLD));
        char mid[32];
        size_t mn = (size_t)list_.filterLen() < sizeof(mid) - 1
                        ? (size_t)list_.filterLen()
                        : sizeof(mid) - 1;
        memcpy(mid, n.name + off, mn);
        mid[mn] = 0;
        g.hline(x0, (int16_t)(rr.y + 18), g.textWidth(mid, F_BOLD), SEL_FG);
      }
    }

    text::formatAge(n.last_heard_secs, buf, sizeof(buf));
    textRight(g, COL_AGE_R, (int16_t)(rr.y + 8), buf, F_SMALL, sel ? fg : mut);

    if (n.flood_hops >= 0) {
      icons::hopArrow(g, COL_HOP_X, (int16_t)(rr.y + 7), sel ? fg : mut);
      snprintf(buf, sizeof(buf), "%d", n.flood_hops);
      g.text((int16_t)(COL_HOP_X + 10), (int16_t)(rr.y + 8), buf, F_SMALL,
             sel ? fg : mut);
    } else {
      g.text((int16_t)(COL_HOP_X + 4), (int16_t)(rr.y + 8), "?", F_SMALL,
             sel ? fg : mut);
    }

    // Closest-hop rule: bars ONLY for nodes we heard directly.
    if (n.direct)
      icons::snrBars(g, COL_SNR_X, (int16_t)(rr.y + 7), snrLevel(n.snr_x4),
                     sel ? SEL_FG : OK, sel ? SEL_MUTED : BAR_OFF);
  }

  if (row_count_ == 0) {
    const char *msg = list_.filterLen() ? "no node matches the filter"
                                        : "no nodes heard yet";
    int16_t tw = g.textWidth(msg, F_BODY);
    g.text((int16_t)((SCREEN_W - tw) / 2), (int16_t)(area.y + 40), msg, F_BODY,
           MUTED);
  }

  list_.drawScrollbar(g);
}

} // namespace screens
} // namespace merlin
