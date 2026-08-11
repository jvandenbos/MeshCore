// MERLIN UI — Settings · CHANNELS (design spec §Channels).
//
// Two screens, four flows. ChannelsScreen is the list plus the two ways in
// (create private, join with a key); ChannelScreen is one channel — reveal,
// share over the mesh, delete. Both fold their sub-steps into modes rather than
// pushing more depth, so the stack never grows past Settings > Channels >
// channel and `L` always means exactly one step back.
#pragma once

#include "../channel.h"
#include "../router.h"
#include "../widgets/listview.h"
#include "../widgets/textentry.h"

namespace merlin {
namespace screens {

class ChannelsScreen : public Screen {
public:
  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  enum Mode : uint8_t { M_LIST = 0, M_ADD, M_JOIN_NAME, M_JOIN_KEY };

  void rebuild(UiModel &m);
  bool keyEvent(const InputEvent &e, Router &r, UiModel &m, CommandSink *cmd);
  void renderKeyEntry(Gfx &g);

  ListView list_;
  TextEntry entry_;
  char hex_[channel::KEY_HEX + 1] = {0};
  int hex_len_ = 0;
  char join_name_[channel::NAME_CAP] = {0};
  Mode mode_ = M_LIST;
  bool key_error_ = false; // Enter pressed on a key that is not 32 digits
  int chan_count_ = 0;
};

class ChannelScreen : public Screen {
public:
  static constexpr int ACT_DELETE = 0;
  static constexpr int MAX_ROWS = 128;

  // Set by ChannelsScreen just before pushing a freshly created channel, which
  // lands on Share: a private channel nobody else holds the key to is useless.
  void setFresh(bool f) { fresh_ = f; }

  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;
  void onConfirm(int action, bool accepted, CommandSink *cmd) override;

private:
  enum Mode : uint8_t { M_DETAIL = 0, M_SHARE, M_DELETE, M_GONE };

  int rowCount() const { return deletable_ ? 3 : 2; }
  void renderDetail(Gfx &g, UiModel &m);
  void renderShare(Gfx &g, UiModel &m);
  void renderDelete(Gfx &g);

  ListView list_; // the contact picker, in share mode
  TextEntry confirm_;
  int16_t rows_[MAX_ROWS] = {0};
  int row_count_ = 0;
  int idx_ = 0;
  int sel_ = 0;
  Mode mode_ = M_DETAIL;
  bool fresh_ = false;
  bool reveal_ = false;
  bool deletable_ = true;
  char name_[channel::NAME_CAP] = {0};
  char shared_to_[channel::NAME_CAP] = {0};
};

} // namespace screens
} // namespace merlin
