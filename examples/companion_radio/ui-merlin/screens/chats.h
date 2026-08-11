// MERLIN UI — 1 · CHATS, the boot screen. Channels + DMs unified, unread-first
// then recency. `n` opens the new-message picker (DM or #hashtag join), `r`
// opens the selected thread with the composer already up, `m` mutes.
#pragma once

#include "../router.h"
#include "../widgets/listview.h"

namespace merlin {
namespace screens {

class ChatsScreen : public Screen {
public:
  static constexpr int MAX_CONVOS = 128;

  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  void rebuild(UiModel &m);

  ListView list_;
  int16_t rows_[MAX_CONVOS] = {0};
  int row_count_ = 0;
  int hidden_ = 0;
};

} // namespace screens
} // namespace merlin
