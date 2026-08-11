// MERLIN UI — 2 · NODES. The full grammar lives here: five sorts, a hiding
// type-to-filter, the repeaters-only toggle and the SNR-direct-only rule.
#pragma once

#include "../router.h"
#include "../widgets/listview.h"

namespace merlin {
namespace screens {

class NodesScreen : public Screen {
public:
  // Design spec: s cycles Recent -> Hops -> Direct(SNR) -> Distance -> Name.
  enum Sort : uint8_t { S_RECENT = 0, S_HOPS, S_DIRECT, S_DIST, S_NAME, S_COUNT };

  static constexpr int MAX_NODES = 512;
  static constexpr int DIVIDER = -1; // pseudo-row in the Direct(SNR) view

  void onEnter(int arg, UiModel &m) override;
  bool onEvent(const InputEvent &e, Router &r, UiModel &m,
               CommandSink *cmd) override;
  void render(Gfx &g, UiModel &m) override;

  // Exposed so the node card can resolve "the row I was opened from".
  Sort sort() const { return sort_; }
  const char *filter() const { return list_.filter(); }

private:
  void rebuild(UiModel &m);
  bool matches(const NodeView &n) const;

  ListView list_;
  int16_t rows_[MAX_NODES + 1] = {0}; // model indices, or DIVIDER
  int row_count_ = 0;
  int hidden_ = 0;
  int shown_ = 0;
  int total_ = 0;
  Sort sort_ = S_RECENT;
  bool repeaters_only_ = false;
};

} // namespace screens
} // namespace merlin
