// MERLIN UI — 1b · NEW MESSAGE. Reached with `n` from Chats. One field does
// both jobs the design spec asks of it: type a contact's name to open a DM, or
// type #something to join a hashtag channel outright (the headline flow —
// the key derives from the name, so there is nothing else to ask for).
#pragma once

#include "../router.h"
#include "../widgets/listview.h"
#include "../widgets/textentry.h"

namespace merlin {
namespace screens {

class NewChatScreen : public Screen {
public:
  static constexpr int MAX_ROWS = 128;

  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  void rebuild(UiModel &m);
  bool isHashtag() const { return entry_.len() > 0 && entry_.text()[0] == '#'; }
  // Jump to the conversation the command just created, wherever it landed.
  void openConvo(Router &r, UiModel &m, const char *name);

  ListView list_;
  TextEntry entry_;
  int16_t rows_[MAX_ROWS] = {0}; // node indices
  int row_count_ = 0;
};

} // namespace screens
} // namespace merlin
