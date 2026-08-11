// MERLIN device port — StoreBackend over SPIFFS.
//
// The store blob is ~230 KB, so a write is far too slow to sit in a render or
// event path: flushing there would stall the UI mid-keystroke and, worse, hold
// the CPU while the radio wants servicing. Instead callers mark the store dirty
// and the idle hook does the write when nothing else is happening.
#pragma once

#include <stddef.h>

#include "../ui-merlin/store.h"

namespace merlin {

class SpiffsStore : public StoreBackend {
public:
  bool read(const char *ns, void *out, size_t cap, size_t &len) override;
  bool write(const char *ns, const void *data, size_t len) override;

  // Called from anywhere, cheap: just raises the flag.
  void markDirty() { dirty_ = true; }
  bool dirty() const { return dirty_; }

  // Call ONLY from the idle hook. Writes the store if it is dirty and enough
  // time has passed since the last write; returns true if it wrote.
  bool flushIfDue(MsgStore &store, unsigned long now_ms);

private:
  bool dirty_ = false;
  unsigned long next_write_ms_ = 0;
};

} // namespace merlin
