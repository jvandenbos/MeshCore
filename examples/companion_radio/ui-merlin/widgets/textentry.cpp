#include "textentry.h"

#include "../theme.h"

namespace merlin {

void TextEntry::clear() {
  len_ = 0;
  buf_[0] = 0;
}

void TextEntry::set(const char *s) {
  len_ = 0;
  for (const char *p = s; p && *p && len_ < max_; ++p) buf_[len_++] = *p;
  buf_[len_] = 0;
}

void TextEntry::setMax(int n) {
  if (n < 1) n = 1;
  if (n > CAP) n = CAP;
  max_ = n;
  // Whatever is already typed obeys the new limit too, so the buffer can never
  // be longer than the field admits to being.
  if (len_ > max_) {
    len_ = max_;
    buf_[len_] = 0;
  }
}

bool TextEntry::onKey(char c) {
  if (c == '\b') {
    if (len_ == 0) return false;
    buf_[--len_] = 0;
    return true;
  }
  if (c >= ' ' && c < 0x7F) {
    if (len_ < max_) {
      buf_[len_++] = c;
      buf_[len_] = 0;
    }
    return true;
  }
  return false;
}

void TextEntry::render(Gfx &g, Rect r, const char *label, Color text_c) const {
  using namespace theme;
  g.fillRect(r, FILTER_BG);
  int16_t x = (int16_t)(r.x + 4);
  if (label && *label) {
    g.text(x, (int16_t)(r.y + 4), label, F_SMALL, ACCENT);
    x = (int16_t)(x + g.textWidth(label, F_SMALL) + 6);
  }
  g.text(x, (int16_t)(r.y + 1), buf_, F_BODY, text_c);
  vline(g, (int16_t)(x + g.textWidth(buf_, F_BODY) + 1), (int16_t)(r.y + 2),
        (int16_t)(r.h - 4), ACCENT);
}

} // namespace merlin
