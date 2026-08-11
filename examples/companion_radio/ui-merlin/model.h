// MERLIN UI — the read-model. The UI never owns mesh data, only views it.
// Implemented by FixtureModel (sim) and MeshModel (device, Phase 3).
#pragma once

#include <stdint.h>

namespace merlin {

enum NodeKind : uint8_t { NK_REPEATER = 0, NK_COMPANION = 1, NK_SELF = 2 };
enum GpsState : uint8_t { GPS_OFF = 0, GPS_SEARCH = 1, GPS_FIX = 2 };
enum AckState : uint8_t {
  ACK_NONE = 0,   // incoming message
  ACK_QUEUED,     // .
  ACK_SENT,       // ->
  ACK_DELIVERED,  // v   (DM ack only)
  ACK_HEARD_N,    // (N  (channel: heard repeated N times)
  ACK_FAILED      // !
};

struct NodeView {
  const char *name = "";
  uint8_t kind = NK_COMPANION;
  uint32_t last_heard_secs = 0;
  int8_t flood_hops = -1; // last advert's path_len; -1 unknown
  int8_t route_hops = -1; // out_path_len; -1 = no route
  bool direct = false;    // heard at path_len == 0
  int16_t snr_x4 = 0;     // valid iff direct (closest-hop rule)
  int16_t rssi = 0;       // valid iff direct
  bool has_pos = false;
  float lat = 0, lon = 0;
  int16_t alt_m = 0;
  const uint8_t *pubkey6 = nullptr;
  const char *fw = "";
  const char *power = ""; // "4.02 V . up 16d" — repeater-reported, may be ""
};

struct MsgView {
  const char *sender = "";
  const char *text = "";
  uint32_t ts = 0; // seconds since local midnight (display clock)
  bool own = false;
  uint8_t ack = ACK_NONE;
  uint8_t heard_n = 0;
};

struct ConvoView {
  const char *name = "";
  bool channel = false;
  uint8_t unread = 0;
  const char *snippet = "";
  uint32_t ts = 0;
  bool muted = false;
  const char *tag = ""; // "DM", "" for channels
};

// A channel as Settings > Channels sees it: name + key + how it was obtained.
struct ChannelView {
  const char *name = "";
  bool hashtag = false;     // key derived from the name, so not a secret
  const char *key_hex = ""; // 32 hex chars, "" when there is no key to reveal
  uint8_t unread = 0;
  bool undeletable = false; // #Public
  int convo = -1;           // index into the conversation list
};

struct StatusView {
  uint8_t batt_pct = 0;
  uint16_t batt_mv = 0;
  bool charging = false;
  uint8_t gps = GPS_OFF;
  uint8_t sats = 0;
  bool ble = false;
  uint32_t clock = 0; // seconds since local midnight
  uint8_t unread_total = 0;
  bool rx_blip = false, tx_blip = false;
};

// Everything Home needs that is not per-node or per-convo.
struct DeviceView {
  const char *name = "";
  const char *fw = "";
  const char *freq = "";  // "910.525"
  const char *bw = "";    // "62.5"
  uint8_t sf = 0, cr = 0;
  int8_t tx_dbm = 0;
  const char *ble_peer = "";
  uint16_t rx_today = 0, tx_today = 0;
  uint16_t nodes_24h = 0;
  float lat = 0, lon = 0; // our own position, for distance/bearing
  bool has_pos = false;
  const char *uptime = "";
  uint32_t free_heap = 0;
};

struct UiModel {
  virtual ~UiModel() {}
  virtual StatusView status() = 0;
  virtual DeviceView device() = 0;
  virtual int convoCount() = 0;
  virtual ConvoView convo(int i) = 0;
  virtual int msgCount(int convo) = 0;
  virtual MsgView msg(int convo, int i) = 0;
  virtual int nodeCount() = 0;
  virtual NodeView node(int i) = 0;
  virtual int channelCount() = 0;
  virtual ChannelView channel(int i) = 0;
};

// Fire-and-forget; model updates arrive on a later frame.
struct CommandSink {
  virtual ~CommandSink() {}
  virtual void sendMsg(int convo, const char *text) { (void)convo; (void)text; }
  virtual void sendAdvert() {}
  virtual void muteConvo(int convo, bool muted) { (void)convo; (void)muted; }
  // Called when a conversation is opened; clears its unread counter.
  virtual void markRead(int convo) { (void)convo; }
  virtual void pingNode(int node) { (void)node; }
  virtual void setRadio(const char *freq, const char *bw, uint8_t sf, uint8_t cr,
                        int8_t tx_dbm) {
    (void)freq; (void)bw; (void)sf; (void)cr; (void)tx_dbm;
  }
  virtual void setName(const char *name) { (void)name; }
  virtual void setBrightness(uint8_t pct) { (void)pct; }

  // --- channels (design spec §Channels) ------------------------------------
  // All fire-and-forget: the UI re-reads the model to find what landed, so a
  // device implementation may take a frame (or fail) without the UI lying.
  virtual void joinHashtag(const char *name) { (void)name; }
  virtual void createChannel(const char *name) { (void)name; }
  virtual void joinChannel(const char *name, const uint8_t *key16) {
    (void)name; (void)key16;
  }
  virtual void removeChannel(int idx) { (void)idx; }
  // Sends the channel's join string to a node as an encrypted DM.
  virtual void shareChannel(int idx, int node_idx) { (void)idx; (void)node_idx; }
  // Opens (creating if needed) the DM conversation with a node.
  virtual void openDm(int node_idx) { (void)node_idx; }
};

} // namespace merlin
