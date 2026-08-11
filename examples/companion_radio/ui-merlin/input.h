// MERLIN UI — one event queue, three sources, one grammar.
#pragma once

#include <stdint.h>

namespace merlin {

struct InputEvent {
  enum Kind : uint8_t { KEY, NAV, TOUCH };
  enum Nav : uint8_t { U, D, L, R, CLICK };
  enum TouchKind : uint8_t { TAP, LONG, SWIPE_L, SWIPE_R };

  Kind kind = KEY;
  char ch = 0;              // KEY: ascii from the keyboard (incl. '\n', '\b')
  Nav nav = U;              // NAV
  TouchKind tk = TAP;       // TOUCH
  int16_t tx = 0, ty = 0;   // TOUCH

  static InputEvent key(char c) {
    InputEvent e; e.kind = KEY; e.ch = c; return e;
  }
  static InputEvent navi(Nav n) {
    InputEvent e; e.kind = NAV; e.nav = n; return e;
  }
  static InputEvent touch(TouchKind t, int16_t x, int16_t y) {
    InputEvent e; e.kind = TOUCH; e.tk = t; e.tx = x; e.ty = y; return e;
  }
};

} // namespace merlin
