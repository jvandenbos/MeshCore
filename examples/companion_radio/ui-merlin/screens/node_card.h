// MERLIN UI — 2a · NODE CARD. Everything known about one node, plus two
// actions. Shows BOTH hop numbers (heard vs reachable) and refuses to print an
// SNR for a relayed node.
#pragma once

#include "../router.h"

namespace merlin {
namespace screens {

class NodeCardScreen : public Screen {
public:
  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  void runAction(Router &r, UiModel &m, CommandSink *cmd);

  int node_ = 0;
  int action_ = 0; // 0 = Message, 1 = Ping
};

} // namespace screens
} // namespace merlin
