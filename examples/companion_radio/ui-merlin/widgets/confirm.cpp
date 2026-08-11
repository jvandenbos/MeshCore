#include "confirm.h"

#include "../theme.h"

#include <string.h>

namespace merlin {

static constexpr Rect kBox(8, 158, 304, 64);
static constexpr Rect kCancel(16, 203, 140, 18);
static constexpr Rect kAccept(164, 203, 140, 18);

static bool inside(Rect r, int16_t x, int16_t y) {
  return x >= r.x && x < r.right() && y >= r.y && y < r.bottom();
}

static void copyBounded(char *dst, size_t cap, const char *src) {
  if (!src) { dst[0] = 0; return; }
  size_t n = strlen(src);
  if (n >= cap) n = cap - 1;
  memcpy(dst, src, n);
  dst[n] = 0;
}

void Confirm::show(const Spec &s) {
  copyBounded(title_, sizeof(title_), s.title);
  copyBounded(detail_, sizeof(detail_), s.detail);
  copyBounded(warn_, sizeof(warn_), s.warn);
  copyBounded(cancel_, sizeof(cancel_), s.cancel);
  copyBounded(accept_, sizeof(accept_), s.accept);
  accept_focus_ = false; // Cancel is default focus
  active_ = true;
}

bool Confirm::onEvent(const InputEvent &e, bool &resolved, bool &accepted) {
  resolved = false;
  accepted = false;
  if (!active_) return false;

  if (e.kind == InputEvent::NAV) {
    switch (e.nav) {
      case InputEvent::L: accept_focus_ = false; return true;
      case InputEvent::R: accept_focus_ = true; return true;
      case InputEvent::CLICK:
        resolved = true; accepted = accept_focus_; active_ = false; return true;
      default: return true; // U/D do nothing, but the modal still swallows them
    }
  }
  if (e.kind == InputEvent::KEY) {
    if (e.ch == '\n') { resolved = true; accepted = accept_focus_; active_ = false; }
    else if (e.ch == '\b' || e.ch == 27) { resolved = true; accepted = false; active_ = false; }
    return true;
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP) {
    if (inside(kCancel, e.tx, e.ty)) { resolved = true; accepted = false; active_ = false; }
    else if (inside(kAccept, e.tx, e.ty)) { resolved = true; accepted = true; active_ = false; }
    return true;
  }
  return true;
}

static void button(Gfx &g, Rect r, const char *label, bool focused) {
  using namespace theme;
  g.fillRect(r, focused ? ACCENT : BG);
  frameRect(g, r, focused ? ACCENT : rgb(0x4A, 0x4A, 0x4A));
  int16_t tw = g.textWidth(label, focused ? F_BOLD : F_BODY);
  g.text((int16_t)(r.x + (r.w - tw) / 2), (int16_t)(r.y + 2), label,
         focused ? F_BOLD : F_BODY, focused ? BG : FG);
}

void Confirm::render(Gfx &g) const {
  using namespace theme;
  if (!active_) return;
  g.fillRect(kBox, BG);
  frameRect(g, kBox, ACCENT);
  g.text(16, 162, title_, F_SMALL, ACCENT);
  g.text(16, 174, detail_, F_BODY, FG);
  g.text(16, 192, warn_, F_SMALL, WARN);
  button(g, kCancel, cancel_, !accept_focus_);
  button(g, kAccept, accept_, accept_focus_);
}

} // namespace merlin
