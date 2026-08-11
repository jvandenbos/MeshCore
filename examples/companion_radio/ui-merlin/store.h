// MERLIN UI — the message store. One source of truth for conversations,
// messages, unread counts and channel keys; every screen reads it through the
// model, nothing keeps a second copy.
//
// Platform-pure and allocation-free: a fixed ring per conversation, oldest
// evicted. The whole thing is one POD blob so persistence is a single write.
// ⚠ Device port: ~90 KB — place it in PSRAM, never in .bss.
#pragma once

#include "channel.h"
#include "model.h"

#include <stddef.h>
#include <stdint.h>

namespace merlin {

// Persistence seam: namespaced byte blobs. The UI never learns what a file is.
// Sim backs it with RAM (+ an optional tmpfile); the device port backs it with
// SPIFFS and writes ONLY from the idle hook, never from render or event paths.
struct StoreBackend {
  virtual ~StoreBackend() {}
  virtual bool read(const char *ns, void *out, size_t cap, size_t &len) = 0;
  virtual bool write(const char *ns, const void *data, size_t len) = 0;
};

enum ConvoKind : uint8_t {
  CV_DM = 0,       // direct message with a node
  CV_HASHTAG = 1,  // public channel, key derived from the name
  CV_PRIVATE = 2   // private channel, 128-bit key
};

class MsgStore {
public:
  // 40 firmware channel slots (MAX_GROUP_CHANNELS on the T-Deck) plus DM
  // headroom. The whole store is PSRAM-resident on the device.
  static constexpr int MAX_CONVOS = 48;
  static constexpr int MAX_MSGS = 24; // ring per convo, oldest evicted
  static constexpr int NAME_CAP = (int)channel::NAME_CAP;
  // MeshCore's MAX_TEXT_LEN is 10*CIPHER_BLOCK_SIZE = 160 (BaseChatMesh.h:8),
  // and the text arrives without its NUL, so a received message needs 161.
  static constexpr int TEXT_CAP = 161;
  static constexpr int SNIPPET_CAP = 96;
  static constexpr uint32_t NO_ID = 0;

  void clear();

  // --- conversations -------------------------------------------------------
  int convoCount() const { return d_.convo_count; }
  bool valid(int c) const { return c >= 0 && c < d_.convo_count; }
  const char *name(int c) const;
  uint8_t kind(int c) const;
  bool isChannel(int c) const { return valid(c) && d_.convos[c].kind != CV_DM; }
  bool muted(int c) const;
  void setMuted(int c, bool m);
  uint8_t unread(int c) const;
  int unreadTotal() const;
  void markRead(int c);
  void setUnread(int c, uint8_t n);
  const char *snippet(int c) const;
  uint32_t lastTs(int c) const;
  const uint8_t *key(int c) const;
  void keyHex(int c, char *out, size_t cap) const;

  int findConvo(const char *name) const;
  // Returns the existing index if the name is already known, a new one
  // otherwise, or -1 when the store is full.
  int addConvo(const char *name, uint8_t kind, const uint8_t *key16);
  void removeConvo(int c);

  // --- messages ------------------------------------------------------------
  int msgCount(int c) const;
  MsgView msg(int c, int i) const;
  // Returns the message id, or NO_ID if it could not be stored. `bump_unread`
  // is false for our own sends and for a thread the user is looking at.
  uint32_t append(int c, const char *sender, const char *text, uint32_t ts,
                  bool own, uint8_t ack, bool bump_unread);
  // ACK progression: QUEUED -> SENT -> DELIVERED / HEARD_N. Ids are unique
  // across the store, so the caller does not have to remember the convo.
  bool setAck(uint32_t id, uint8_t ack, uint8_t heard_n);

  // --- persistence ---------------------------------------------------------
  void attach(StoreBackend *b) { backend_ = b; }
  bool save(const char *ns = "msgs");
  bool load(const char *ns = "msgs");

private:
  struct Msg {
    char sender[NAME_CAP];
    char text[TEXT_CAP];
    uint32_t ts;
    uint32_t id;
    uint8_t own, ack, heard_n, pad;
  };
  struct Convo {
    char name[NAME_CAP];
    char snippet[SNIPPET_CAP];
    uint8_t key[channel::KEY_BYTES];
    uint32_t last_ts;
    uint16_t first; // ring index of the oldest message
    uint16_t count;
    uint8_t kind, unread, muted, has_key;
    Msg msgs[MAX_MSGS];
  };
  struct Data {
    Convo convos[MAX_CONVOS];
    uint32_t next_id;
    uint16_t convo_count;
    uint16_t pad;
  };

  void refreshSnippet(int c);

  Data d_ = {};
  StoreBackend *backend_ = nullptr;
};

} // namespace merlin
