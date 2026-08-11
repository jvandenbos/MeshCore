// MERLIN UI — 1a · THREAD. Message rows with real ACK glyphs, the composer
// (typing anywhere enters it), and the join-invite action row: a channel invite
// arriving as a DM is rendered as "Join #name?", never as a raw URL.
#pragma once

#include "../router.h"
#include "../widgets/composer.h"
#include "../widgets/listview.h"

namespace merlin {
namespace screens {

class ThreadScreen : public Screen {
public:
  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;
  // The tab bar stands down while composing (design spec ruling).
  bool hidesTabBar() const override { return composer_.active(); }
  // `r` in Chats means reply: open the thread with the composer already up.
  // Inside a thread `r` cannot mean reply — there, every printable key is the
  // first letter of a message (design rule 2 beats rule 3 in a conversation).
  void beginReply() { composer_.open(); }

private:
  // Enter / click / tap on the selected row: joins an invite, retries a failed
  // send, and otherwise does nothing (a message is not a destination).
  bool activateRow(Router &r, UiModel &m, CommandSink *cmd);

  ListView list_;
  Composer composer_;
  int convo_ = 0;
  int last_count_ = -1;
};

} // namespace screens
} // namespace merlin
