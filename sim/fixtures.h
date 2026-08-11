// MERLIN sim — the fleet as it actually was on 2026-08-08, so the goldens can
// be compared frame-for-frame against the approved mockups.
//
// Conversations and messages live in the real MsgStore, not in fixture arrays:
// the screens read exactly the structure the device will hand them, and
// send/join/share go through it too.
#pragma once

#include "../examples/companion_radio/ui-merlin/model.h"
#include "../examples/companion_radio/ui-merlin/store.h"

#include <stddef.h>
#include <stdint.h>

// RAM-backed store persistence, with an optional file so a run can be resumed.
// Set MERLIN_STORE=<path> to keep the blob across invocations.
class SimStoreBackend : public merlin::StoreBackend {
public:
  SimStoreBackend();
  bool read(const char *ns, void *out, size_t cap, size_t &len) override;
  bool write(const char *ns, const void *data, size_t len) override;

private:
  const char *path_ = nullptr;
  unsigned char blob_[262144];
  size_t blob_len_ = 0;
};

class FixtureModel : public merlin::UiModel {
public:
  FixtureModel();

  merlin::StatusView status() override;
  merlin::DeviceView device() override;
  int convoCount() override;
  merlin::ConvoView convo(int i) override;
  int msgCount(int convo) override;
  merlin::MsgView msg(int convo, int i) override;
  int nodeCount() override;
  merlin::NodeView node(int i) override;
  int channelCount() override;
  merlin::ChannelView channel(int i) override;

  void setClock(uint32_t secs) { clock_ = secs; }
  void setBlips(bool rx, bool tx) { rx_ = rx; tx_ = tx; }
  uint32_t clock() const { return clock_; }

  merlin::MsgStore &store() { return store_; }
  // Write the store back through the backend (a no-op unless MERLIN_STORE is
  // set). On the device this is the idle-hook call, never a render-path one.
  bool persist();
  // Channel index -> conversation index (channels are the non-DM convos).
  int channelConvo(int i) const;
  // An own message starts QUEUED and climbs one step per tick, so a script can
  // snap the whole ACK progression.
  void trackSend(uint32_t id, int convo);
  void advanceAcks();
  // Deterministic per run, so a freshly created channel always mints the same
  // key and the goldens stay pixel-stable.
  void generateChannelKey(uint8_t *key);
  // Incoming traffic, from the sim's `rx` verb.
  int inject(const char *convo_name, const char *sender, const char *text);

private:
  void seed();

  merlin::MsgStore store_;
  SimStoreBackend backend_;
  uint32_t clock_ = 21 * 3600 + 42 * 60; // 21:42 local
  bool rx_ = true, tx_ = false;
  uint32_t key_seed_ = 1;

  struct Pending {
    uint32_t id;
    int convo;
    uint8_t stage;
  };
  Pending pending_[8] = {};
  int pending_n_ = 0;

  // channel() hands out a pointer to hex; a few views may be live at once.
  char key_hex_[4][merlin::channel::KEY_HEX + 1] = {};
  int hex_slot_ = 0;
};

class FixtureSink : public merlin::CommandSink {
public:
  void sendMsg(int convo, const char *text) override;
  void sendAdvert() override;
  void muteConvo(int convo, bool muted) override;
  void markRead(int convo) override;
  void pingNode(int node) override;
  void setRadio(const char *freq, const char *bw, uint8_t sf, uint8_t cr,
                int8_t tx_dbm) override;
  void setBrightness(uint8_t pct) override;

  void joinHashtag(const char *name) override;
  void createChannel(const char *name) override;
  void joinChannel(const char *name, const uint8_t *key16) override;
  void removeChannel(int idx) override;
  void shareChannel(int idx, int node_idx) override;
  void openDm(int node_idx) override;

  FixtureModel *model = nullptr;
};
