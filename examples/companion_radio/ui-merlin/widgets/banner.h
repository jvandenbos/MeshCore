// MERLIN UI — new-message toast. One line, 4 s, never steals compose focus.
// Sits UNDER the status bar: rule 4 (status never occluded) outranks the
// inventory's "over status bar" wording.
#pragma once

#include "../gfx.h"

namespace merlin {

class Banner {
public:
  static constexpr uint32_t LIFETIME = 4;
  static constexpr int16_t HEIGHT = 14;
  // Not every toast is about a message. ADVERT/PING/IDENTITY have nowhere to
  // send you, and they say so with this instead of naming a conversation they
  // do not mean — tapping one used to mark convo 0 read and open a dead thread.
  static constexpr int NO_CONVO = -1;

  void show(const char *who, const char *text, int convo, uint32_t now);
  void dismiss() { active_ = false; }
  void tick(uint32_t now);

  bool active() const { return active_; }
  int convo() const { return convo_; }
  bool hit(int16_t x, int16_t y) const;

  void render(Gfx &g, uint32_t now) const;

private:
  bool active_ = false;
  uint32_t shown_ = 0;
  int convo_ = 0;
  char who_[24] = {0};
  char text_[64] = {0};
};

} // namespace merlin
