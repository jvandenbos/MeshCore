// MERLIN UI — the composer (design rule 2: in a conversation, just start
// typing). It owns every printable key while open, grows one line at a time to
// three, and takes the tab bar's 14 px back — a message being written outranks
// navigation.
#pragma once

#include "../gfx.h"
#include "../input.h"

namespace merlin {

class Composer {
public:
  static constexpr int CAP = 152;     // MeshCore's text payload, rounded down
  static constexpr int WARN_AT = 140; // char counter appears past this
  static constexpr int MAX_LINES = 3;

  bool active() const { return active_; }
  const char *text() const { return buf_; }
  int len() const { return len_; }

  void open() { active_ = true; }
  void close(); // cancel or post-send: clears the buffer

  // Handles one key while open. `sent` comes back true when Enter committed a
  // non-empty message — the caller transmits, then calls close(). Returns
  // false only for keys the composer has no use for, and even those are
  // swallowed by the thread screen so a stray verb cannot jump tabs mid-word.
  bool onKey(char c, bool &sent);

  // Pixel height the composer currently needs, including its hint strip.
  int16_t height(Gfx &g) const;
  void render(Gfx &g) const;

private:
  // Greedy word wrap into [start,end) pairs. Returns the number of lines,
  // which may exceed `max` conceptually — the render shows the last MAX_LINES,
  // so the caret is always visible.
  int wrap(Gfx &g, int *starts, int *ends, int max) const;

  char buf_[CAP + 1] = {0};
  int len_ = 0;
  bool active_ = false;
};

} // namespace merlin
