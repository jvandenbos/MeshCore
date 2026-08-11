#include "SpiffsStore.h"

#include <Arduino.h>
#include <SPIFFS.h>

// SPIFFS wears out, and the store changes on every received message, so a
// write-on-every-change would burn the flash for no benefit: a lost minute of
// history after a crash costs nothing, the radio still has the traffic.
#define WRITE_INTERVAL_MS 60000
// One flash page group per pass. Small enough that no single f.write() can sit
// on the loop task long enough to matter, large enough that the per-call
// overhead stays in the noise.
#define WRITE_CHUNK_BYTES 4096

namespace merlin {

static void pathFor(const char *ns, char *out, size_t cap) {
  snprintf(out, cap, "/merlin_%s", ns ? ns : "msgs");
}

bool SpiffsStore::read(const char *ns, void *out, size_t cap, size_t &len) {
  char path[32];
  pathFor(ns, path, sizeof(path));
  File f = SPIFFS.open(path, FILE_READ);
  if (!f) return false;
  size_t sz = f.size();
  if (sz > cap) { // a different build's store; MsgStore::load will reject it
    f.close();
    return false;
  }
  len = f.read((uint8_t *)out, sz);
  f.close();
  return len == sz;
}

bool SpiffsStore::write(const char *ns, const void *data, size_t len) {
  char path[32];
  pathFor(ns, path, sizeof(path));
  // Write to a temp file and rename, so a reset mid-write leaves the previous
  // store intact rather than a truncated one.
  char tmp[40];
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  File f = SPIFFS.open(tmp, FILE_WRITE);
  if (!f) return false;

  // Chunked, with a yield between chunks. The store is ~230 KB and this runs on
  // the Arduino loop task, the same task that drives the_mesh.loop() and the UI
  // (see main.cpp): one blocking write that size stalls LoRa RX and BLE for as
  // long as SPIFFS takes, and if it outlasts the task watchdog window the IDLE
  // task never gets to run and the device panics — a reboot that looks exactly
  // like the crash-to-blank first light saw under real traffic.
  const uint8_t *p = (const uint8_t *)data;
  size_t left = len;
  while (left > 0) {
    size_t n = left > WRITE_CHUNK_BYTES ? (size_t)WRITE_CHUNK_BYTES : left;
    // A short write means the filesystem is full or failing, not that it wants
    // another try: stop here and leave the previous store as the good one.
    if (f.write(p, n) != n) {
      f.close();
      SPIFFS.remove(tmp);
      return false;
    }
    p += n;
    left -= n;
    delay(1); // one tick: feeds the watchdog and lets the radio breathe
  }
  f.close();

  SPIFFS.remove(path);
  return SPIFFS.rename(tmp, path);
}

bool SpiffsStore::flushIfDue(MsgStore &store, unsigned long now_ms) {
  if (!dirty_) return false;
  if (next_write_ms_ != 0 && (long)(now_ms - next_write_ms_) < 0) return false;
  dirty_ = false;
  next_write_ms_ = now_ms + WRITE_INTERVAL_MS;
  return store.save();
}

} // namespace merlin
