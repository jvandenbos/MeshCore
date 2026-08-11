#include "listview.h"

#include "../theme.h"

namespace merlin {

void ListView::setCount(int n) {
  count_ = n < 0 ? 0 : n;
  if (sel_ >= count_) sel_ = count_ > 0 ? count_ - 1 : 0;
  if (sel_ < 0) sel_ = 0;
  ensureVisible();
}

int ListView::lastVisible() const {
  int n = rowsVisible();
  int last = top_ + (n > 0 ? n - 1 : 0);
  return last >= count_ ? count_ - 1 : last;
}

Rect ListView::rowRect(int i) const {
  return Rect(area_.x, (int16_t)(area_.y + (i - top_) * row_h_), area_.w, row_h_);
}

void ListView::select(int i) {
  if (count_ == 0) { sel_ = 0; top_ = 0; return; }
  if (i < 0) i = 0;
  if (i >= count_) i = count_ - 1;
  sel_ = i;
  ensureVisible();
}

void ListView::moveBy(int delta) { select(sel_ + delta); }

void ListView::ensureVisible() {
  int n = rowsVisible();
  if (n <= 0) { top_ = 0; return; }
  if (sel_ < top_) top_ = sel_;
  if (sel_ > top_ + n - 1) top_ = sel_ - n + 1;
  int max_top = count_ - n;
  if (max_top < 0) max_top = 0;
  if (top_ > max_top) top_ = max_top;
  if (top_ < 0) top_ = 0;
}

int ListView::hitTest(int16_t x, int16_t y) const {
  if (x < area_.x || x >= area_.right() || y < area_.y || y >= area_.bottom())
    return -1;
  int i = top_ + (y - area_.y) / row_h_;
  return (i >= 0 && i < count_) ? i : -1;
}

bool ListView::onEvent(const InputEvent &e) {
  if (e.kind == InputEvent::NAV) {
    if (e.nav == InputEvent::U) { moveBy(-1); return true; }
    if (e.nav == InputEvent::D) { moveBy(1); return true; }
    return false;
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP) {
    int i = hitTest(e.tx, e.ty);
    if (i >= 0 && i != sel_) { select(i); return true; }
  }
  return false;
}

void ListView::closeFilter() {
  filter_[0] = 0;
  filter_len_ = 0;
  open_ = false;
  explicit_ = false;
}

bool ListView::onFilterKey(char c, bool &changed) {
  changed = false;
  if (c == '/') {
    // '/' is the door, never a filter character.
    if (!open_) {
      open_ = true;
      explicit_ = true;
      filter_len_ = 0;
      filter_[0] = 0;
      changed = true;
    }
    return true;
  }
  if (c == '\b') {
    if (!open_) return false; // let the screen, then the router, treat it as back
    if (filter_len_ > 0) {
      filter_[--filter_len_] = 0;
      if (filter_len_ == 0 && !explicit_) closeFilter();
    } else {
      closeFilter();
    }
    changed = true;
    return true;
  }
  if (c >= ' ' && c < 0x7F) {
    if (filter_len_ + 1 >= (int)sizeof(filter_)) return true; // full: swallow
    open_ = true;
    filter_[filter_len_++] = c;
    filter_[filter_len_] = 0;
    changed = true;
    return true;
  }
  return false;
}

void ListView::drawScrollbar(Gfx &g) const {
  int n = rowsVisible();
  if (count_ <= 0 || n <= 0) return;
  g.fillRect(Rect(theme::SCROLLBAR_X, area_.y, theme::SCROLLBAR_W, area_.h),
             theme::SCROLL_TRACK);
  int16_t th = (int16_t)((int32_t)area_.h * (n < count_ ? n : count_) / count_);
  if (th < 8) th = 8;
  if (th > area_.h) th = area_.h;
  int16_t travel = (int16_t)(area_.h - th);
  int denom = count_ - n;
  int16_t off = denom > 0 ? (int16_t)((int32_t)travel * top_ / denom) : 0;
  g.fillRect(Rect(theme::SCROLLBAR_X, (int16_t)(area_.y + off),
                  theme::SCROLLBAR_W, th),
             theme::ACCENT);
}

} // namespace merlin
