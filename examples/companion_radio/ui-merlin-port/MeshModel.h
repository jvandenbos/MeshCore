// MERLIN device port — the read-model and command sink over the live mesh.
//
// The UI asks for views by index and expects the strings in them to stay valid
// until the next frame, so nothing here hands out a pointer into a temporary.
// Contacts are snapshotted into a PSRAM-resident cache on a timer; messages and
// conversations live in MsgStore, which is already stable storage.
#pragma once

#include <Arduino.h>

#include "../MyMesh.h"
#include "../NodePrefs.h"
#include "../ui-merlin/model.h"
#include "../ui-merlin/store.h"
#include "SpiffsStore.h"

namespace merlin {

class MeshModel : public UiModel, public CommandSink {
public:
  // `store` and its backing memory are owned by the caller (PSRAM).
  bool begin(MsgStore *store, SpiffsStore *backend, NodePrefs *prefs);

  // --- UiModel -------------------------------------------------------------
  StatusView status() override;
  DeviceView device() override;
  int convoCount() override;
  ConvoView convo(int i) override;
  int msgCount(int convo) override;
  MsgView msg(int convo, int i) override;
  int nodeCount() override;
  NodeView node(int i) override;
  int channelCount() override;
  ChannelView channel(int i) override;

  // --- CommandSink ---------------------------------------------------------
  void sendMsg(int convo, const char *text) override;
  void sendAdvert() override;
  void muteConvo(int convo, bool muted) override;
  void markRead(int convo) override;
  void pingNode(int node) override;
  void setRadio(const char *freq, const char *bw, uint8_t sf, uint8_t cr,
                int8_t tx_dbm) override;
  void setName(const char *name) override;
  void setBrightness(uint8_t pct) override;
  void joinHashtag(const char *name) override;
  void createChannel(const char *name) override;
  void joinChannel(const char *name, const uint8_t *key16) override;
  void removeChannel(int idx) override;
  void shareChannel(int idx, int node_idx) override;
  void openDm(int node_idx) override;

  // --- fed from the firmware -----------------------------------------------
  // A message arrived. `from` is a contact name for a DM or a channel name for
  // a channel message; the two are told apart by looking the name up. Returns
  // the conversation it landed in, or -1 — the banner needs a real target.
  int onIncoming(uint8_t path_len, const char *from, const char *text);
  // One of our sends was acknowledged.
  void onAck(uint32_t ack_hash);
  // Refreshes the contact snapshot if it is stale. Idle-hook work.
  void refresh(bool force = false);

  bool storeDirty() const { return dirty_; }
  void clearStoreDirty() { dirty_ = false; }

private:
  struct NodeRec {
    // MsgStore::NAME_CAP, not the firmware's 32: a node name is what the store
    // gets asked for a conversation by, and a name the model reports wider than
    // the store can hold is a name no lookup can ever match again.
    char name[MsgStore::NAME_CAP];
    uint8_t type;
    uint32_t lastmod;
    int32_t gps_lat, gps_lon;
    uint8_t pubkey[6];
    int8_t route_hops;
    int8_t flood_hops;
    int16_t snr_x4;
    bool direct;
  };

  int channelConvo(int i) const;
  // Index of the firmware channel slot whose key matches this conversation's,
  // or -1. The key, not the name, is the identity of a channel.
  int fwChannelForConvo(int convo) const;
  int freeFwChannelSlot() const;
  // Adds a channel to both the firmware and the store. Returns the convo index.
  int addChannel(const char *display_name, const uint8_t *key16, uint8_t kind);
  ContactInfo *contactByName(const char *name);
  uint32_t nowSecs() const;

  MsgStore *store_ = nullptr;
  SpiffsStore *backend_ = nullptr;
  NodePrefs *prefs_ = nullptr;

  NodeRec *nodes_ = nullptr; // PSRAM snapshot
  int node_count_ = 0;
  uint32_t next_refresh_ = 0;
  bool dirty_ = false;

  // Sends awaiting an ACK: the mesh identifies them by hash, the store by id.
  struct PendingAck {
    uint32_t hash;
    uint32_t msg_id;
  };
  PendingAck pending_[8] = {};
  int pending_n_ = 0;

  // Scratch that outlives a frame, because the views point into it.
  char key_hex_[4][channel::KEY_HEX + 1] = {};
  int hex_slot_ = 0;
  char dev_name_[32] = {0};
  char freq_[16] = {0}, bw_[16] = {0};
  char uptime_[24] = {0};
  char chan_name_[4][channel::NAME_CAP] = {};
  int chan_slot_ = 0;
};

} // namespace merlin
