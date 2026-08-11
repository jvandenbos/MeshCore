// MERLIN UI — "one list widget rules all" (design rule 5).
// Owns selection, scroll window, hit-testing and the pinned scrollbar; screens
// own row painting, because only they know what a row means.
#pragma once

#include "../gfx.h"
#include "../input.h"

namespace merlin {

class ListView {
public:
  void setArea(Rect a) { area_ = a; }
  void setRowHeight(int16_t h) { row_h_ = h; }
  void setCount(int n);

  Rect area() const { return area_; }
  int count() const { return count_; }
  int sel() const { return sel_; }
  int top() const { return top_; }
  int16_t rowHeight() const { return row_h_; }
  int rowsVisible() const { return row_h_ > 0 ? area_.h / row_h_ : 0; }
  int lastVisible() const;
  bool visible(int i) const { return i >= top_ && i <= lastVisible(); }
  Rect rowRect(int i) const;

  void select(int i);
  void moveBy(int delta);
  void ensureVisible();

  // Consumes U/D and taps inside the list area. Everything else falls through
  // so the screen (then the router) can apply the grammar.
  bool onEvent(const InputEvent &e);
  // Index under a touch point, or -1.
  int hitTest(int16_t x, int16_t y) const;

  void drawScrollbar(Gfx &g) const;

  // --- type-to-filter (design rule 3, with the Phase 2 '/' amendment) ------
  // Lives here rather than in each screen because every list filters the same
  // way; screens supply only the matching predicate.
  bool filterOpen() const { return open_; }
  const char *filter() const { return filter_; }
  int filterLen() const { return filter_len_; }
  void closeFilter();

  // Call after the screen's own verbs have declined the key. Returns true when
  // the key was consumed; `changed` asks the screen to rebuild its row list.
  //
  //   '/'        always opens the filter, even on a verb-heavy screen — it is
  //              the only way to filter a name that starts with a verb letter.
  //   printable  extends the filter (and opens it, which is what makes bare
  //              letters filter once no verb has claimed them).
  //   '\b'       trims; an implicitly-opened filter closes when it empties, so
  //              "Backspace = back when no filter text" still holds.
  bool onFilterKey(char c, bool &changed);

private:
  Rect area_;
  int16_t row_h_ = 23;
  int count_ = 0;
  int sel_ = 0;
  int top_ = 0;

  char filter_[24] = {0};
  uint8_t filter_len_ = 0;
  bool open_ = false;
  bool explicit_ = false; // opened with '/', so it survives an empty buffer
};

} // namespace merlin
