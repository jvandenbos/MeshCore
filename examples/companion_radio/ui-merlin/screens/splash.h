// MERLIN UI — splash. <2 s, replaced by Chats. No bars: this is the one screen
// with no grammar attached, so advertising controls would be a lie.
#pragma once

#include "../router.h"

namespace merlin {
namespace screens {

class SplashScreen : public Screen {
public:
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;
  bool hidesTabBar() const override { return true; }
  bool hidesStatusBar() const override { return true; }

  void setProgress(uint8_t pct) { progress_ = pct; }

private:
  uint8_t progress_ = 66;
};

} // namespace screens
} // namespace merlin
