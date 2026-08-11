// MERLIN UI — 0 · HOME, the glance. Identity, radio, power, links, counters
// and three quick actions. No live map, no graphs in v1.
#pragma once

#include "../router.h"

namespace merlin {
namespace screens {

class HomeScreen : public Screen {
public:
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  void fire(Router &r, CommandSink *cmd, UiModel &m);
  int action_ = 0; // 0 Advert, 1 Mute, 2 Bright
  uint8_t bright_ = 70;
};

} // namespace screens
} // namespace merlin
