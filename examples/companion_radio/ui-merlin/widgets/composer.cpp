#include "composer.h"

#include "../theme.h"

#include <stdio.h>

namespace merlin {

using namespace theme;

static constexpr int16_t HINT_H = 10; // hint / char-counter strip
static constexpr int16_t PAD = 3;
static constexpr int16_t LINE_H = 16;
static constexpr int16_t TEXT_X = 16;
static constexpr int16_t TEXT_R = 314;
static constexpr int16_t TEXT_W = TEXT_R - TEXT_X;
static constexpr int MAX_WRAP = 8;

void Composer::close() {
  active_ = false;
  len_ = 0;
  buf_[0] = 0;
}

bool Composer::onKey(char c, bool &sent) {
  sent = false;
  if (c == '\n') {
    sent = len_ > 0;
    return true;
  }
  if (c == '\b') {
    if (len_ > 0) buf_[--len_] = 0;
    else close(); // Backspace on empty cancels (design rule 2)
    return true;
  }
  if (c >= ' ' && c < 0x7F) {
    if (len_ < CAP) {
      buf_[len_++] = c;
      buf_[len_] = 0;
    }
    return true;
  }
  return false;
}

int Composer::wrap(Gfx &g, int *starts, int *ends, int max) const {
  int n = 0, i = 0;
  char one[2] = {0, 0};
  while (n < max) {
    int line_start = i, last_space = -1, j = i;
    int16_t w = 0;
    while (j < len_) {
      one[0] = buf_[j];
      int16_t cw = g.textWidth(one, F_BODY);
      if (w + cw > TEXT_W) break;
      if (buf_[j] == ' ') last_space = j;
      w = (int16_t)(w + cw);
      ++j;
    }
    starts[n] = line_start;
    if (j >= len_) {
      ends[n++] = len_;
      break;
    }
    // Break after the last space that fits; a single unbroken word is cut.
    int brk = last_space > line_start ? last_space + 1 : j;
    ends[n++] = brk;
    i = brk;
  }
  return n ? n : 1;
}

int16_t Composer::height(Gfx &g) const {
  int starts[MAX_WRAP], ends[MAX_WRAP];
  int n = wrap(g, starts, ends, MAX_WRAP);
  if (n > MAX_LINES) n = MAX_LINES;
  return (int16_t)(HINT_H + PAD * 2 + n * LINE_H);
}

void Composer::render(Gfx &g) const {
  if (!active_) return;

  int starts[MAX_WRAP], ends[MAX_WRAP];
  int n = wrap(g, starts, ends, MAX_WRAP);
  int shown = n > MAX_LINES ? MAX_LINES : n;
  int first = n - shown; // keep the caret's line on screen

  int16_t h = (int16_t)(HINT_H + PAD * 2 + shown * LINE_H);
  int16_t top = (int16_t)(SCREEN_H - h);
  g.fillRect(Rect(0, top, SCREEN_W, h), HEADER_BG);
  // A live accent rule is the whole "you are in compose mode" signal — no
  // second modal, no banner, just the seam glowing.
  g.hline(0, top, SCREEN_W, ACCENT);

  g.text(4, (int16_t)(top + 2), "COMPOSE", F_SMALL, ACCENT);
  char buf[24];
  if (len_ > WARN_AT) {
    snprintf(buf, sizeof(buf), "%d/%d", len_, CAP);
    textRight(g, TEXT_R, (int16_t)(top + 2), buf, F_SMALL,
              len_ >= CAP ? ALERT : WARN);
  } else {
    textRight(g, TEXT_R, (int16_t)(top + 2), "Enter send . Bksp cancel", F_SMALL,
              MUTED);
  }

  int16_t y = (int16_t)(top + HINT_H + PAD);
  char line[CAP + 1];
  for (int i = first; i < n; ++i) {
    int len = ends[i] - starts[i];
    if (len < 0) len = 0;
    for (int k = 0; k < len; ++k) line[k] = buf_[starts[i] + k];
    line[len] = 0;
    if (i == first) g.text(4, (int16_t)(y + 2), ">", F_SMALL, ACCENT);
    g.text(TEXT_X, y, line, F_BODY, FG);
    if (i == n - 1)
      vline(g, (int16_t)(TEXT_X + g.textWidth(line, F_BODY) + 1),
            (int16_t)(y + 1), 13, ACCENT);
    y = (int16_t)(y + LINE_H);
  }
}

} // namespace merlin
