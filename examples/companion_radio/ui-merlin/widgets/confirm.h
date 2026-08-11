// MERLIN UI — the only modal. Two buttons on L/R, never a stack.
// Cancel holds focus by default: on a radio-changing act the safe button wins.
#pragma once

#include "../gfx.h"
#include "../input.h"

namespace merlin {

class Confirm {
public:
  struct Spec {
    const char *title = "";
    const char *detail = "";
    const char *warn = "";
    const char *cancel = "Cancel";
    const char *accept = "Apply";
  };

  void show(const Spec &s);
  void dismiss() { active_ = false; }
  bool active() const { return active_; }
  bool acceptFocused() const { return accept_focus_; }

  // Returns true when the modal consumed the event. `resolved` is set when the
  // user committed; `accepted` carries which button won.
  bool onEvent(const InputEvent &e, bool &resolved, bool &accepted);

  void render(Gfx &g) const;

private:
  // The spec's strings are copied, not aliased: callers build them in stack
  // buffers and the modal outlives the call that raised it.
  char title_[40] = {0};
  char detail_[64] = {0};
  char warn_[64] = {0};
  char cancel_[16] = {0};
  char accept_[16] = {0};
  bool active_ = false;
  bool accept_focus_ = false;
};

} // namespace merlin
