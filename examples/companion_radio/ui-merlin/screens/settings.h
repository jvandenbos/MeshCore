// MERLIN UI — 3 · SETTINGS, grouped list. Phase 2 scope: the group list is
// navigable and RADIO opens its editor; the other groups are placeholders
// until Phase 3 wires the preference store.
#pragma once

#include "../router.h"
#include "../widgets/listview.h"
#include "../widgets/textentry.h"

namespace merlin {
namespace screens {

class SettingsScreen : public Screen {
public:
  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  void rebuild(UiModel &m);
  ListView list_;
  int8_t rows_[8] = {0};
  int row_count_ = 0;
  int hidden_ = 0;
};

// 3a · RADIO editor — numeric fields + the confirm-before-apply overlay.
class RadioScreen : public Screen {
public:
  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;
  void onConfirm(int action, bool accepted, CommandSink *cmd) override;

private:
  int field_ = 0;
  bool unsaved_ = false;
  bool freq_error_ = false; // Enter pressed on a frequency off the band
  char freq_[16] = {0};
  char orig_freq_[16] = {0};
  // The four parameters this editor does NOT change, snapshotted on entry so
  // applying a frequency can hand them straight back. setRadio() writes all
  // five at once, so anything not carried across is silently reset.
  char bw_[16] = {0};
  uint8_t sf_ = 0, cr_ = 0;
  int8_t tx_dbm_ = 0;
};

// IDENTITY editor — the node name. Applied on Enter; the new name rides the
// next advert, so the banner points there instead of pretending it's instant.
class IdentityScreen : public Screen {
public:
  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

private:
  TextEntry entry_;
};

} // namespace screens
} // namespace merlin
