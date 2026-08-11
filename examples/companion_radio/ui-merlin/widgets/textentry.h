// MERLIN UI — a one-line bounded text field. Used by the new-message target,
// channel naming and the delete-confirmation typing. Not the composer: that one
// wraps, grows and owns the bottom of the screen.
#pragma once

#include "../gfx.h"

namespace merlin {

class TextEntry {
public:
  static constexpr int CAP = 40;

  const char *text() const { return buf_; }
  int len() const { return len_; }
  bool empty() const { return len_ == 0; }
  void clear();
  void set(const char *s);

  // Some fields are bounded by whatever receives them rather than by the
  // widget: a channel name past channel::NAME_CAP is truncated on the way into
  // the radio, and then nothing the screen typed matches what comes back.
  // Clamped to CAP; a lower limit takes effect immediately.
  void setMax(int n);

  // Printable keys and backspace only. Returns false on backspace-at-empty so
  // the screen can treat it as "back", the way every list does.
  bool onKey(char c);

  // Label on the left, text and caret after it. `r` is the whole strip.
  void render(Gfx &g, Rect r, const char *label, Color text_c) const;

private:
  char buf_[CAP + 1] = {0};
  int len_ = 0;
  int max_ = CAP;
};

} // namespace merlin
