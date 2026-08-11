#include "MeshModel.h"

#include <esp_heap_caps.h>
#include <target.h>

#include "../ui-merlin/channel.h"

#define REFRESH_INTERVAL_SECS 5
#define SELF_NODE 1 // node 0 is us

namespace merlin {

// --- small helpers ---------------------------------------------------------

// Single-cell Li-ion under a light load. Voltage is a poor fuel gauge on the
// flat top of the curve, so the top steps are deliberately coarse.
static uint8_t battPercent(uint16_t mv) {
  static const uint16_t kMv[] = {3300, 3450, 3550, 3650, 3750,
                                 3850, 3950, 4050, 4150};
  static const uint8_t kPct[] = {0, 5, 15, 30, 45, 60, 75, 90, 100};
  if (mv <= kMv[0]) return 0;
  const int n = sizeof(kPct) / sizeof(kPct[0]);
  if (mv >= kMv[n - 1]) return 100;
  for (int i = 1; i < n; ++i) {
    if (mv < kMv[i]) {
      int span = kMv[i] - kMv[i - 1];
      int into = mv - kMv[i - 1];
      return (uint8_t)(kPct[i - 1] + (kPct[i] - kPct[i - 1]) * into / span);
    }
  }
  return 100;
}

// A ContactInfo::name is 32 bytes wide, a MsgStore conversation name is
// NAME_CAP (24). Every name that crosses into the store has to cross at the
// SAME boundary or the two disagree about who a conversation is with: addConvo
// truncates, findConvo does not, so a contact whose name is >= NAME_CAP long
// gets a brand-new conversation per received message until the store fills, and
// nothing sent from it can ever find its contact again. One helper, used
// everywhere a name enters or is looked up in the store.
static void storeName(const char *in, char *out) {
  snprintf(out, MsgStore::NAME_CAP, "%s", in ? in : "");
}

// The firmware ships its public channel as "Public"; the UI calls it
// "#Public", as every other client does.
static void displayChannelName(const char *fw, char *out, size_t cap) {
  if (fw && fw[0] == '#') {
    snprintf(out, cap, "%s", fw);
  } else {
    snprintf(out, cap, "#%s", fw ? fw : "");
  }
}

uint32_t MeshModel::nowSecs() const { return rtc_clock.getCurrentTime(); }

// --- lifecycle -------------------------------------------------------------

bool MeshModel::begin(MsgStore *store, SpiffsStore *backend, NodePrefs *prefs) {
  store_ = store;
  backend_ = backend;
  prefs_ = prefs;

  nodes_ = (NodeRec *)heap_caps_malloc(sizeof(NodeRec) * (MAX_CONTACTS + 1),
                                       MALLOC_CAP_SPIRAM);
  if (!nodes_) return false;
  memset(nodes_, 0, sizeof(NodeRec) * (MAX_CONTACTS + 1));

  // Mirror the radio's channels into the store, so a channel the firmware can
  // already decrypt shows up as a conversation without waiting for traffic.
  for (int i = 0; i < MAX_GROUP_CHANNELS; ++i) {
    ChannelDetails ch;
    if (!the_mesh.getChannel(i, ch)) continue;
    if (ch.name[0] == 0) continue;

    char name[channel::NAME_CAP];
    displayChannelName(ch.name, name, sizeof(name));

    // A channel's kind is not stored anywhere, but it is recoverable: if the
    // key is what the name derives to, it is a hashtag channel and the key is
    // not a secret. Anything else was handed to us and must stay hidden.
    uint8_t derived[channel::KEY_BYTES];
    channel::deriveHashtagKey(name, derived);
    bool hashtag = memcmp(derived, ch.channel.secret, channel::KEY_BYTES) == 0;

    store_->addConvo(name, hashtag ? CV_HASHTAG : CV_PRIVATE,
                     ch.channel.secret);
  }

  refresh(true);
  return true;
}

void MeshModel::refresh(bool force) {
  uint32_t now = nowSecs();
  if (!force && next_refresh_ != 0 && now < next_refresh_) return;
  next_refresh_ = now + REFRESH_INTERVAL_SECS;
  if (!nodes_) return;

  // Us first, so the Nodes list always has an anchor. Screens that pick a peer
  // skip NK_SELF.
  NodeRec &self = nodes_[0];
  memset(&self, 0, sizeof(self));
  storeName(prefs_ ? prefs_->node_name : "MERLIN", self.name);
  self.type = 0xFF; // sentinel: mapped to NK_SELF in node()
  self.lastmod = now;
  self.gps_lat = (int32_t)(sensors.node_lat * 1000000.0);
  self.gps_lon = (int32_t)(sensors.node_lon * 1000000.0);
  self.route_hops = -1;
  self.flood_hops = -1;

  // The advert cache is the only place hop count and signal live: ContactInfo
  // records neither. Static because 16 AdvertPaths is ~1.8 KB and this runs on
  // the 8 KB Arduino loop stack; the UI is single-tasked, so there is no second
  // caller to race it.
  static AdvertPath recent[16];
  int n_recent = the_mesh.getRecentlyHeard(recent, 16);

  int n = the_mesh.getNumContacts();
  if (n > MAX_CONTACTS) n = MAX_CONTACTS;
  int out = SELF_NODE;
  for (int i = 0; i < n; ++i) {
    ContactInfo c;
    if (!the_mesh.getContactByIdx(i, c)) continue;
    NodeRec &r = nodes_[out];
    memset(&r, 0, sizeof(r));
    storeName(c.name, r.name);
    r.type = c.type;
    r.lastmod = c.lastmod;
    r.gps_lat = c.gps_lat;
    r.gps_lon = c.gps_lon;
    memcpy(r.pubkey, c.id.pub_key, sizeof(r.pubkey));
    r.route_hops =
        (c.out_path_len == OUT_PATH_UNKNOWN) ? -1 : (int8_t)c.out_path_len;
    r.flood_hops = -1;
    for (int j = 0; j < n_recent; ++j) {
      if (memcmp(recent[j].pubkey_prefix, c.id.pub_key,
                 sizeof(recent[j].pubkey_prefix)) != 0)
        continue;
      r.flood_hops = (int8_t)recent[j].path_len;
      // Closest-hop rule: an SNR from a relayed advert describes the relay,
      // not this node, so it is only kept when the advert arrived direct.
      if (recent[j].path_len == 0) {
        r.direct = true;
        r.snr_x4 = recent[j].snr_x4;
      }
      break;
    }
    ++out;
  }
  node_count_ = out;
}

// --- UiModel ---------------------------------------------------------------

StatusView MeshModel::status() {
  StatusView v;
  uint16_t mv = board.getBattMilliVolts();
  v.batt_mv = mv;
  v.batt_pct = battPercent(mv);
  v.charging = false; // the T-Deck exposes no charge-status line

  LocationProvider *gps = sensors.getLocationProvider();
  if (gps && gps->isEnabled()) {
    v.gps = gps->isValid() ? GPS_FIX : GPS_SEARCH;
    v.sats = (uint8_t)gps->satellitesCount();
  } else {
    v.gps = GPS_OFF;
  }

  // No timezone is configured anywhere in the firmware, so this clock is UTC.
  v.clock = nowSecs() % 86400;
  v.unread_total = store_ ? (uint8_t)store_->unreadTotal() : 0;
  return v;
}

DeviceView MeshModel::device() {
  DeviceView v;
  if (!prefs_) return v;
  snprintf(dev_name_, sizeof(dev_name_), "%s", prefs_->node_name);
  snprintf(freq_, sizeof(freq_), "%.3f", prefs_->freq);
  snprintf(bw_, sizeof(bw_), "%.1f", prefs_->bw);
  unsigned long up = millis() / 1000;
  if (up >= 86400) {
    snprintf(uptime_, sizeof(uptime_), "%lud %luh", up / 86400,
             (up % 86400) / 3600);
  } else {
    snprintf(uptime_, sizeof(uptime_), "%luh %lum", up / 3600,
             (up % 3600) / 60);
  }

  v.name = dev_name_;
  v.fw = FIRMWARE_VERSION;
  v.freq = freq_;
  v.bw = bw_;
  v.sf = prefs_->sf;
  v.cr = prefs_->cr;
  v.tx_dbm = prefs_->tx_power_dbm;
  v.lat = (float)sensors.node_lat;
  v.lon = (float)sensors.node_lon;
  v.has_pos = sensors.node_lat != 0 || sensors.node_lon != 0;
  v.uptime = uptime_;
  v.free_heap = ESP.getFreeHeap();

  uint32_t cutoff = nowSecs() - 86400;
  uint16_t recent = 0;
  for (int i = SELF_NODE; i < node_count_; ++i) {
    if (nodes_[i].lastmod >= cutoff) ++recent;
  }
  v.nodes_24h = recent;
  return v;
}

int MeshModel::convoCount() { return store_ ? store_->convoCount() : 0; }

ConvoView MeshModel::convo(int i) {
  ConvoView v;
  if (!store_ || !store_->valid(i)) return v;
  v.name = store_->name(i);
  v.channel = store_->isChannel(i);
  v.unread = store_->unread(i);
  v.snippet = store_->snippet(i);
  v.ts = store_->lastTs(i);
  v.muted = store_->muted(i);
  v.tag = v.channel ? "" : "DM";
  return v;
}

int MeshModel::msgCount(int convo) {
  return store_ ? store_->msgCount(convo) : 0;
}

MsgView MeshModel::msg(int convo, int i) {
  return store_ ? store_->msg(convo, i) : MsgView();
}

int MeshModel::nodeCount() { return node_count_; }

NodeView MeshModel::node(int i) {
  NodeView v;
  if (i < 0 || i >= node_count_ || !nodes_) return v;
  const NodeRec &r = nodes_[i];
  v.name = r.name;
  if (r.type == 0xFF) {
    v.kind = NK_SELF;
  } else {
    v.kind = (r.type == ADV_TYPE_REPEATER) ? NK_REPEATER : NK_COMPANION;
  }
  uint32_t now = nowSecs();
  v.last_heard_secs = (now > r.lastmod) ? (now - r.lastmod) : 0;
  v.flood_hops = r.flood_hops;
  v.route_hops = r.route_hops;
  v.direct = r.direct;
  v.snr_x4 = r.snr_x4;
  v.has_pos = r.gps_lat != 0 || r.gps_lon != 0;
  v.lat = r.gps_lat / 1000000.0f;
  v.lon = r.gps_lon / 1000000.0f;
  v.pubkey6 = r.pubkey;
  return v;
}

int MeshModel::channelConvo(int i) const {
  if (!store_) return -1;
  int seen = 0;
  for (int c = 0; c < store_->convoCount(); ++c) {
    if (!store_->isChannel(c)) continue;
    if (seen++ == i) return c;
  }
  return -1;
}

int MeshModel::channelCount() {
  if (!store_) return 0;
  int n = 0;
  for (int c = 0; c < store_->convoCount(); ++c)
    if (store_->isChannel(c)) ++n;
  return n;
}

ChannelView MeshModel::channel(int i) {
  ChannelView v;
  int c = channelConvo(i);
  if (c < 0) return v;
  v.name = store_->name(c);
  v.hashtag = store_->kind(c) == CV_HASHTAG;
  v.unread = store_->unread(c);
  v.undeletable = strcmp(v.name, "#Public") == 0;
  v.convo = c;
  hex_slot_ = (hex_slot_ + 1) % 4;
  store_->keyHex(c, key_hex_[hex_slot_], sizeof(key_hex_[hex_slot_]));
  v.key_hex = key_hex_[hex_slot_];
  return v;
}

// --- channel plumbing ------------------------------------------------------

int MeshModel::fwChannelForConvo(int convo) const {
  if (!store_) return -1;
  const uint8_t *key = store_->key(convo);
  if (!key) return -1;
  for (int i = 0; i < MAX_GROUP_CHANNELS; ++i) {
    ChannelDetails ch;
    if (!the_mesh.getChannel(i, ch)) continue;
    if (ch.name[0] == 0) continue;
    if (memcmp(ch.channel.secret, key, channel::KEY_BYTES) == 0) return i;
  }
  return -1;
}

int MeshModel::freeFwChannelSlot() const {
  for (int i = 0; i < MAX_GROUP_CHANNELS; ++i) {
    ChannelDetails ch;
    if (!the_mesh.getChannel(i, ch)) continue;
    if (ch.name[0] == 0) return i;
  }
  return -1;
}

int MeshModel::addChannel(const char *display_name, const uint8_t *key16,
                          uint8_t kind) {
  if (!store_) return -1;
  int slot = freeFwChannelSlot();
  if (slot < 0) return -1; // all 40 radio slots taken

  ChannelDetails ch;
  memset(&ch, 0, sizeof(ch));
  snprintf(ch.name, sizeof(ch.name), "%s", display_name);
  // setChannel decides key length by looking at bytes 16..31, so the whole
  // 32-byte secret must be zeroed first or it hashes 32 bytes and the channel
  // silently fails to match anyone else's.
  memcpy(ch.channel.secret, key16, channel::KEY_BYTES);
  if (!the_mesh.setChannel(slot, ch)) return -1;
  the_mesh.saveChannels();

  int c = store_->addConvo(display_name, kind, key16);
  dirty_ = true;
  return c;
}

ContactInfo *MeshModel::contactByName(const char *name) {
  // Exact match, not searchContactsByPrefix: a conversation named "OSPREY"
  // must not send to "OSPREY-RPTR-525" because it happens to be first in the
  // table. Messaging the wrong node is worse than not sending.
  //
  // The name arrives from MsgStore, which is NAME_CAP wide, so both sides are
  // compared through the same truncation — otherwise a contact with a name that
  // long is unreachable and its sends disappear without a word. Two contacts
  // sharing their first NAME_CAP-1 characters are indistinguishable here, but
  // that is the store's resolution: they are one conversation either way.
  char want[MsgStore::NAME_CAP];
  storeName(name, want);
  int n = the_mesh.getNumContacts();
  for (int i = 0; i < n; ++i) {
    ContactInfo c;
    if (!the_mesh.getContactByIdx(i, c)) continue;
    char have[MsgStore::NAME_CAP];
    storeName(c.name, have);
    if (strcmp(have, want) != 0) continue;
    return the_mesh.lookupContactByPubKey(c.id.pub_key, 6);
  }
  return nullptr;
}

// --- CommandSink -----------------------------------------------------------

void MeshModel::sendMsg(int convo, const char *text) {
  if (!store_ || !store_->valid(convo) || !text || !*text) return;
  uint32_t ts = nowSecs();

  if (store_->isChannel(convo)) {
    int slot = fwChannelForConvo(convo);
    if (slot < 0) return;
    ChannelDetails ch;
    if (!the_mesh.getChannel(slot, ch)) return;

    // The firmware prepends "<our name>: " and that counts against the same
    // 160-byte budget, truncating silently past it. Clamp here so what the
    // thread shows is what actually went out.
    const char *me = prefs_ ? prefs_->node_name : "";
    int budget = MAX_TEXT_LEN - (int)strlen(me) - 2;
    int len = (int)strlen(text);
    if (budget < 0) budget = 0;
    if (len > budget) len = budget;
    if (len <= 0) return;

    char sent[MsgStore::TEXT_CAP];
    snprintf(sent, sizeof(sent), "%.*s", len, text);
    if (!the_mesh.sendGroupMessage(ts, ch.channel, me, sent, len)) return;
    // A channel message has no addressee, so there is nothing to acknowledge:
    // SENT is as far as its state can honestly go.
    store_->append(convo, me, sent, ts, true, ACK_SENT, false);
    dirty_ = true;
    return;
  }

  ContactInfo *c = contactByName(store_->name(convo));
  if (!c) return;
  uint32_t expected_ack = 0, est_timeout = 0;
  int res = the_mesh.sendMessage(*c, ts, 0, text, expected_ack, est_timeout);
  const char *me = prefs_ ? prefs_->node_name : "";
  if (res == MSG_SEND_FAILED) {
    store_->append(convo, me, text, ts, true, ACK_FAILED, false);
    dirty_ = true;
    return;
  }
  uint32_t id = store_->append(convo, me, text, ts, true, ACK_SENT, false);
  dirty_ = true;

  // Without this the reply has nothing to match against and the message would
  // never resolve past SENT.
  the_mesh.trackExpectedAck(expected_ack, c);
  if (expected_ack && id != MsgStore::NO_ID) {
    if (pending_n_ >= (int)(sizeof(pending_) / sizeof(pending_[0]))) {
      // Same depth as the firmware's own table; drop the oldest to match.
      memmove(pending_, pending_ + 1, sizeof(pending_) - sizeof(pending_[0]));
      --pending_n_;
    }
    pending_[pending_n_].hash = expected_ack;
    pending_[pending_n_].msg_id = id;
    ++pending_n_;
  }
}

void MeshModel::sendAdvert() { the_mesh.advert(); }

void MeshModel::muteConvo(int convo, bool muted) {
  if (!store_) return;
  store_->setMuted(convo, muted);
  dirty_ = true;
}

void MeshModel::markRead(int convo) {
  if (!store_) return;
  store_->markRead(convo);
  dirty_ = true;
}

void MeshModel::pingNode(int node) {
  if (node < SELF_NODE || node >= node_count_) return;
  ContactInfo *c = the_mesh.lookupContactByPubKey(nodes_[node].pubkey, 6);
  if (!c) return;
  uint32_t tag = 0, est_timeout = 0;
  the_mesh.sendRequest(*c, REQ_TYPE_GET_STATUS, tag, est_timeout);
}

void MeshModel::setRadio(const char *freq, const char *bw, uint8_t sf,
                         uint8_t cr, int8_t tx_dbm) {
  if (!prefs_) return;
  prefs_->freq = atof(freq);
  prefs_->bw = atof(bw);
  prefs_->sf = sf;
  prefs_->cr = cr;
  prefs_->tx_power_dbm = tx_dbm;
  the_mesh.savePrefs();
  radio_driver.setParams(prefs_->freq, prefs_->bw, prefs_->sf, prefs_->cr);
  radio_driver.setTxPower(prefs_->tx_power_dbm);
}

void MeshModel::setName(const char *name) {
  if (!prefs_ || !name) return;
  snprintf(prefs_->node_name, sizeof(prefs_->node_name), "%s", name);
  the_mesh.savePrefs();
}

void MeshModel::setBrightness(uint8_t pct) {
  // The T-Deck backlight driver (AW9364-class charge pump on PIN_TFT_LEDA_CTL)
  // has 16 current levels selected by counting falling edges on its enable
  // line: >=2.5 ms low fully resets it, the first rising edge is level 1
  // (maximum), and each subsequent falling pulse steps one level down.
  // Pulses must be 0.5-500 us, so the train runs with interrupts masked.
#ifdef PIN_TFT_LEDA_CTL
  // Clamped before the subtraction: (100 - pct) is unsigned, so a caller that
  // ever passes 101 wraps it to 65535, asks for a 101-pulse train with
  // interrupts masked for ~6 ms, and leaves the driver near-dark.
  if (pct > 100) pct = 100;
  uint8_t level = pct == 0 ? 0 : (uint8_t)(1 + ((uint16_t)(100 - pct) * 15) / 100);
  digitalWrite(PIN_TFT_LEDA_CTL, LOW);
  delay(3); // full reset (also the pct==0 "off" state)
  if (level == 0) return;
  noInterrupts();
  digitalWrite(PIN_TFT_LEDA_CTL, HIGH); // level 1 = brightest
  for (uint8_t i = 1; i < level; ++i) {
    delayMicroseconds(30);
    digitalWrite(PIN_TFT_LEDA_CTL, LOW);
    delayMicroseconds(30);
    digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
  }
  interrupts();
#else
  (void)pct;
#endif
}

void MeshModel::joinHashtag(const char *name) {
  if (!store_ || store_->findConvo(name) >= 0) return;
  uint8_t key[channel::KEY_BYTES];
  channel::deriveHashtagKey(name, key);
  addChannel(name, key, CV_HASHTAG);
}

void MeshModel::createChannel(const char *name) {
  if (!store_ || store_->findConvo(name) >= 0) return;
  uint8_t key[channel::KEY_BYTES];
  // A channel key is a secret, so it comes from the hardware RNG, never from
  // the simulator's deterministic stand-in.
  esp_fill_random(key, sizeof(key));
  addChannel(name, key, CV_PRIVATE);
}

void MeshModel::joinChannel(const char *name, const uint8_t *key16) {
  if (!store_ || !key16 || store_->findConvo(name) >= 0) return;
  addChannel(name, key16, CV_PRIVATE);
}

void MeshModel::removeChannel(int idx) {
  int c = channelConvo(idx);
  if (c < 0) return;
  int slot = fwChannelForConvo(c);
  if (slot >= 0) {
    ChannelDetails empty;
    memset(&empty, 0, sizeof(empty));
    the_mesh.setChannel(slot, empty);
    the_mesh.saveChannels();
  }
  store_->removeConvo(c);
  dirty_ = true;
}

void MeshModel::shareChannel(int idx, int node_idx) {
  int c = channelConvo(idx);
  if (c < 0 || node_idx < SELF_NODE || node_idx >= node_count_) return;

  char join[channel::JOIN_CAP];
  const uint8_t *key = store_->key(c);
  channel::formatJoin(store_->name(c), key, key != nullptr, join, sizeof(join));

  const char *peer = nodes_[node_idx].name;
  int dm = store_->findConvo(peer);
  if (dm < 0) dm = store_->addConvo(peer, CV_DM, nullptr);
  if (dm < 0) return;
  sendMsg(dm, join);
}

void MeshModel::openDm(int node_idx) {
  if (!store_ || node_idx < SELF_NODE || node_idx >= node_count_) return;
  store_->addConvo(nodes_[node_idx].name, CV_DM, nullptr);
  dirty_ = true;
}

// --- from the firmware -----------------------------------------------------

int MeshModel::onIncoming(uint8_t path_len, const char *from,
                          const char *text) {
  (void)path_len;
  if (!store_ || !from || !text) return -1;
  uint32_t ts = nowSecs();

  // The firmware hands a channel name here for a channel message and a contact
  // name for a DM, with nothing to tell them apart, so the name is looked up:
  // a known channel wins, anything else is a DM.
  char chan[channel::NAME_CAP];
  displayChannelName(from, chan, sizeof(chan));
  int c = store_->findConvo(chan);
  if (c >= 0 && store_->isChannel(c)) {
    // A channel message arrives as "<sender>: <body>" because the sending
    // firmware prepends it. Split so the thread shows a real author.
    const char *sep = strstr(text, ": ");
    if (sep && sep - text < channel::NAME_CAP - 1) {
      char sender[channel::NAME_CAP];
      snprintf(sender, sizeof(sender), "%.*s", (int)(sep - text), text);
      store_->append(c, sender, sep + 2, ts, false, ACK_NONE, true);
    } else {
      store_->append(c, "", text, ts, false, ACK_NONE, true);
    }
    dirty_ = true;
    return c;
  }

  // Truncated once, then used for both the lookup and the add: a raw `from`
  // would miss the conversation addConvo already shortened and mint another one
  // on every message until the store is full.
  char dm[MsgStore::NAME_CAP];
  storeName(from, dm);
  c = store_->findConvo(dm);
  if (c < 0) c = store_->addConvo(dm, CV_DM, nullptr);
  if (c < 0) return -1;
  store_->append(c, dm, text, ts, false, ACK_NONE, true);
  dirty_ = true;
  return c;
}

void MeshModel::onAck(uint32_t ack_hash) {
  if (!store_) return;
  for (int i = 0; i < pending_n_; ++i) {
    if (pending_[i].hash != ack_hash) continue;
    store_->setAck(pending_[i].msg_id, ACK_DELIVERED, 0);
    dirty_ = true;
    for (int j = i; j + 1 < pending_n_; ++j) pending_[j] = pending_[j + 1];
    --pending_n_;
    return;
  }
}

} // namespace merlin
