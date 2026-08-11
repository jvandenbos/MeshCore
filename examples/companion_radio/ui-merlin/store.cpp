#include "store.h"

#include <string.h>

namespace merlin {

static void copyBounded(char *dst, size_t cap, const char *src) {
  if (!cap) return;
  if (!src) { dst[0] = 0; return; }
  size_t n = strlen(src);
  if (n >= cap) n = cap - 1;
  memcpy(dst, src, n);
  dst[n] = 0;
}

void MsgStore::clear() {
  memset(&d_, 0, sizeof(d_));
  d_.next_id = 1;
}

const char *MsgStore::name(int c) const {
  return valid(c) ? d_.convos[c].name : "";
}

uint8_t MsgStore::kind(int c) const {
  return valid(c) ? d_.convos[c].kind : (uint8_t)CV_DM;
}

bool MsgStore::muted(int c) const { return valid(c) && d_.convos[c].muted; }

void MsgStore::setMuted(int c, bool m) {
  if (valid(c)) d_.convos[c].muted = m ? 1 : 0;
}

uint8_t MsgStore::unread(int c) const {
  return valid(c) ? d_.convos[c].unread : 0;
}

int MsgStore::unreadTotal() const {
  int t = 0;
  for (int i = 0; i < d_.convo_count; ++i) t += d_.convos[i].unread;
  return t > 255 ? 255 : t;
}

void MsgStore::markRead(int c) {
  if (valid(c)) d_.convos[c].unread = 0;
}

void MsgStore::setUnread(int c, uint8_t n) {
  if (valid(c)) d_.convos[c].unread = n;
}

const char *MsgStore::snippet(int c) const {
  return valid(c) ? d_.convos[c].snippet : "";
}

uint32_t MsgStore::lastTs(int c) const {
  return valid(c) ? d_.convos[c].last_ts : 0;
}

const uint8_t *MsgStore::key(int c) const {
  return valid(c) && d_.convos[c].has_key ? d_.convos[c].key : nullptr;
}

void MsgStore::keyHex(int c, char *out, size_t cap) const {
  if (!cap) return;
  if (!valid(c) || !d_.convos[c].has_key) { out[0] = 0; return; }
  channel::toHex(d_.convos[c].key, channel::KEY_BYTES, out, cap);
}

int MsgStore::findConvo(const char *n) const {
  if (!n) return -1;
  for (int i = 0; i < d_.convo_count; ++i)
    if (strcmp(d_.convos[i].name, n) == 0) return i;
  return -1;
}

int MsgStore::addConvo(const char *n, uint8_t kind, const uint8_t *key16) {
  int existing = findConvo(n);
  if (existing >= 0) return existing;
  if (d_.convo_count >= MAX_CONVOS) return -1;
  Convo &c = d_.convos[d_.convo_count];
  memset(&c, 0, sizeof(c));
  copyBounded(c.name, sizeof(c.name), n);
  c.kind = kind;
  if (key16) {
    memcpy(c.key, key16, channel::KEY_BYTES);
    c.has_key = 1;
  }
  return d_.convo_count++;
}

void MsgStore::removeConvo(int c) {
  if (!valid(c)) return;
  for (int i = c; i + 1 < d_.convo_count; ++i) d_.convos[i] = d_.convos[i + 1];
  --d_.convo_count;
  memset(&d_.convos[d_.convo_count], 0, sizeof(Convo));
}

int MsgStore::msgCount(int c) const {
  return valid(c) ? d_.convos[c].count : 0;
}

MsgView MsgStore::msg(int c, int i) const {
  MsgView v;
  if (!valid(c)) return v;
  const Convo &cv = d_.convos[c];
  if (i < 0 || i >= cv.count) return v;
  const Msg &m = cv.msgs[(cv.first + i) % MAX_MSGS];
  v.sender = m.sender;
  v.text = m.text;
  v.ts = m.ts;
  v.own = m.own != 0;
  v.ack = m.ack;
  v.heard_n = m.heard_n;
  return v;
}

void MsgStore::refreshSnippet(int c) {
  Convo &cv = d_.convos[c];
  if (cv.count == 0) { cv.snippet[0] = 0; return; }
  const Msg &m = cv.msgs[(cv.first + cv.count - 1) % MAX_MSGS];
  // "you: ..." for our own line, "<sender>: ..." otherwise — derived, never
  // stored twice, so a snippet can never drift from the message it describes.
  const char *who = m.own ? "you" : m.sender;
  size_t w = 0;
  for (const char *p = who; *p && w + 3 < sizeof(cv.snippet); ++p)
    cv.snippet[w++] = *p;
  cv.snippet[w++] = ':';
  cv.snippet[w++] = ' ';

  // A channel invite is never shown as a raw join string — not in the thread,
  // and not here either (design spec §Channels).
  char jname[channel::NAME_CAP];
  uint8_t jkey[channel::KEY_BYTES];
  bool has_key = false;
  char summary[channel::NAME_CAP + 16];
  const char *body = m.text;
  if (channel::parseJoin(m.text, jname, sizeof(jname), jkey, has_key)) {
    size_t s = 0;
    for (const char *p = "invite to "; *p; ++p) summary[s++] = *p;
    for (const char *p = jname; *p && s + 1 < sizeof(summary); ++p)
      summary[s++] = *p;
    summary[s] = 0;
    body = summary;
  }
  for (const char *p = body; *p && w + 1 < sizeof(cv.snippet); ++p)
    cv.snippet[w++] = *p;
  cv.snippet[w] = 0;
}

uint32_t MsgStore::append(int c, const char *sender, const char *text,
                          uint32_t ts, bool own, uint8_t ack,
                          bool bump_unread) {
  if (!valid(c)) return NO_ID;
  Convo &cv = d_.convos[c];
  uint16_t slot;
  if (cv.count < MAX_MSGS) {
    slot = (uint16_t)((cv.first + cv.count) % MAX_MSGS);
    ++cv.count;
  } else {
    slot = cv.first; // ring is full: the oldest message makes way
    cv.first = (uint16_t)((cv.first + 1) % MAX_MSGS);
  }
  Msg &m = cv.msgs[slot];
  memset(&m, 0, sizeof(m));
  copyBounded(m.sender, sizeof(m.sender), sender);
  copyBounded(m.text, sizeof(m.text), text);
  m.ts = ts;
  m.own = own ? 1 : 0;
  m.ack = ack;
  m.id = d_.next_id++;
  cv.last_ts = ts;
  if (bump_unread && cv.unread < 255) ++cv.unread;
  refreshSnippet(c);
  return m.id;
}

bool MsgStore::setAck(uint32_t id, uint8_t ack, uint8_t heard_n) {
  if (id == NO_ID) return false;
  for (int c = 0; c < d_.convo_count; ++c) {
    Convo &cv = d_.convos[c];
    for (int i = 0; i < cv.count; ++i) {
      Msg &m = cv.msgs[(cv.first + i) % MAX_MSGS];
      if (m.id != id) continue;
      m.ack = ack;
      m.heard_n = heard_n;
      return true;
    }
  }
  return false;
}

bool MsgStore::save(const char *ns) {
  return backend_ && backend_->write(ns, &d_, sizeof(d_));
}

bool MsgStore::load(const char *ns) {
  if (!backend_) return false;
  size_t len = 0;
  // Read straight into the live struct: a second copy is 90 KB, which is a
  // stack the device does not have. A rejected blob leaves an empty store,
  // never a half-read one.
  if (!backend_->read(ns, &d_, sizeof(d_), len)) {
    clear();
    return false;
  }
  // A short or long blob is a different build's store, not ours. Refuse it
  // rather than reading a struct that no longer means what it says.
  if (len != sizeof(d_) || d_.convo_count > MAX_CONVOS) {
    clear();
    return false;
  }
  for (int i = 0; i < d_.convo_count; ++i) {
    Convo &c = d_.convos[i];
    if (c.count > MAX_MSGS || c.first >= MAX_MSGS) {
      clear();
      return false;
    }
    c.name[NAME_CAP - 1] = 0;
    c.snippet[SNIPPET_CAP - 1] = 0;
  }
  return true;
}

} // namespace merlin
