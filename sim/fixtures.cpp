#include "fixtures.h"

#include "../examples/companion_radio/ui-merlin/channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace merlin;

// Our own station. A made-up spot on the Maple Bay waterfront — fixture data,
// not a real site; only the repeaters' advertised (public) positions are real.
static constexpr float kSelfLat = 48.8095f, kSelfLon = -123.5980f;

static const uint8_t kKeyOsprey[6] = {0xfc, 0x96, 0xe2, 0x0c, 0xee, 0xd2};
static const uint8_t kKeyGyr[6] = {0xc2, 0xa0, 0x69, 0xa1, 0x8f, 0x3d};
static const uint8_t kKeyEyrie[6] = {0xc6, 0x2c, 0x04, 0xa5, 0x1b, 0x77};
static const uint8_t kKeyTalon[6] = {0xc6, 0x2c, 0x04, 0xa5, 0x90, 0x12};
static const uint8_t kKeyKestrel[6] = {0x2b, 0x0f, 0xa0, 0xc4, 0x77, 0x31};
static const uint8_t kKeyCase[6] = {0x54, 0x1a, 0x9b, 0x3e, 0x08, 0xd4};
static const uint8_t kKeyFerndale[6] = {0x54, 0xe1, 0x22, 0x7a, 0x6c, 0x90};
static const uint8_t kKeyPrairie[6] = {0x25, 0x77, 0xb1, 0x0e, 0x44, 0xaa};

// #Salishnet is a private channel in the fixture so the Channels screen has one
// of each: a hashtag whose key is its name, and a key worth hiding.
static const uint8_t kSalishKey[16] = {0x8f, 0x21, 0x0c, 0xd4, 0x77, 0xa3,
                                       0x19, 0xbe, 0x40, 0x5c, 0xe2, 0x08,
                                       0x9a, 0x33, 0x61, 0xf7};

namespace {

struct NodeFix {
  const char *name;
  uint8_t kind;
  uint32_t age;
  int8_t flood, route;
  bool direct;
  int16_t snr_x4, rssi;
  bool has_pos;
  float lat, lon;
  int16_t alt;
  const uint8_t *key;
  const char *fw;
  const char *power;
};

// Ordered most-recently-heard first, which is what a real contact table looks
// like before the UI sorts it.
const NodeFix kNodes[] = {
    {"OSPREY-RPTR-525", NK_REPEATER, 120, 0, 0, true, 44, -41, true, 48.8093f,
     -123.5978f, 96, kKeyOsprey, "1.16.0", "3.96 V . up 41d"},
    {"TALON", NK_COMPANION, 300, 1, 1, false, 0, 0, true, 48.6500f, -123.4000f,
     12, kKeyTalon, "1.16.0", ""},
    {"EYRIE 525", NK_COMPANION, 480, 1, 1, false, 0, 0, true, 48.8094f,
     -123.5982f, 92, kKeyEyrie, "1.16.0", "USB"},
    {"GYRFALCON-RPTR-525", NK_REPEATER, 15120, 0, 0, true, 50, -71, true,
     48.7765f, -123.6150f, 480, kKeyGyr, "1.16.0", "4.02 V . up 16d"},
    {"CASE", NK_REPEATER, 43200, 2, -1, false, 0, 0, true, 48.7711f, -122.4976f,
     0, kKeyCase, "1.15.2", ""},
    {"RP Ferndale", NK_REPEATER, 86400, 3, -1, false, 0, 0, true, 48.8568f,
     -122.5966f, 0, kKeyFerndale, "1.16.0", ""},
    {"Prairie Ridge RPTR", NK_REPEATER, 172800, 3, -1, false, 0, 0, true,
     47.1642f, -122.0826f, 0, kKeyPrairie, "1.15.2", ""},
    // No advert position: exercises the "sinks to the bottom, never hidden"
    // rule in the distance sort and the "no position" branch on the card.
    {"KESTREL-525", NK_COMPANION, 259200, 2, -1, false, 0, 0, false, 0, 0, 0,
     kKeyKestrel, "1.16.0", ""},
};
constexpr int kNodeCount = (int)(sizeof(kNodes) / sizeof(kNodes[0]));

struct MsgFix {
  const char *sender;
  const char *text;
  uint32_t ts;
  bool own;
  uint8_t ack;
  uint8_t heard_n;
};

// #Public fixture traffic (all senders and messages invented). The OtterWatt
// and Tidepool lines carry the emoji the UTF-8 collapse has to survive
// (rule 6): a ZWJ sequence inside a mention, and an emoji in a sender name —
// the exact class of message that crashes UIs that never planned for it.
const MsgFix kPublic[] = {
    {"OtterWatt", "@[Moss\xF0\x9F\xA7\x91\xE2\x80\x8D\xF0\x9F\x8E\xA4] antenna day sat, bring tools",
     21 * 3600 + 31 * 60, false, ACK_NONE, 0},
    {"MERLIN525", "the 6-in-1 crimper is worth it", 21 * 3600 + 33 * 60, true,
     ACK_HEARD_N, 2},
    {"Tidepool \xF0\x9F\x90\xA0", "new radio cleared customs today",
     21 * 3600 + 35 * 60, false, ACK_NONE, 0},
    {"MERLIN525", "nice! flash it to 910.525 first", 21 * 3600 + 36 * 60, true,
     ACK_FAILED, 0},
    {"Nightjar", "any night owls on the mesh?", 21 * 3600 + 41 * 60, false,
     ACK_NONE, 0},
    {"MERLIN525", "evening from Maple Bay", 21 * 3600 + 42 * 60, true, ACK_SENT, 0},
    {"MERLIN525", "OSPREY is up if you need a hop", 21 * 3600 + 42 * 60, true,
     ACK_QUEUED, 0},
};

const MsgFix kTalon[] = {
    {"TALON", "just landed in Sidney, 2 bars on the handheld",
     21 * 3600 + 34 * 60, false, ACK_NONE, 0},
    {"MERLIN525", "nice - you should hear OSPREY direct from there",
     21 * 3600 + 35 * 60, true, ACK_DELIVERED, 0},
    {"TALON", "got it, +8dB. whats the repeater password policy?",
     21 * 3600 + 39 * 60, false, ACK_NONE, 0},
};

const MsgFix kSalish[] = {
    {"Harbourmaster", "net check-ins at 22:00", 21 * 3600 + 12 * 60, false,
     ACK_NONE, 0},
    {"NetControl", "copy, will be listening", 21 * 3600 + 14 * 60, false,
     ACK_NONE, 0},
};

const MsgFix kOsprey[] = {
    {"MERLIN525", "advert sent", 20 * 3600 + 58 * 60, true, ACK_DELIVERED, 0},
};

const MsgFix kGyr[] = {
    {"GYRFALCON", "bat 4.02V up 16d", 18 * 3600 + 4 * 60, false, ACK_NONE, 0},
};

const MsgFix kEyrie[] = {
    {"EYRIE", "logger restarted", 12 * 3600 + 31 * 60, false, ACK_NONE, 0},
};

struct ConvoFix {
  const char *name;
  uint8_t kind;
  const uint8_t *key;
  uint8_t unread;
  bool muted;
  const MsgFix *msgs;
  int count;
};

#define THREAD(a) a, (int)(sizeof(a) / sizeof(a[0]))

const ConvoFix kConvos[] = {
    {"#Public", CV_HASHTAG, nullptr, 1, false, THREAD(kPublic)},
    {"TALON", CV_DM, nullptr, 2, false, THREAD(kTalon)},
    {"#Salishnet", CV_PRIVATE, kSalishKey, 0, false, THREAD(kSalish)},
    {"OSPREY-RPTR-525", CV_DM, nullptr, 0, false, THREAD(kOsprey)},
    {"GYRFALCON-RPTR-525", CV_DM, nullptr, 0, false, THREAD(kGyr)},
    {"EYRIE 525", CV_DM, nullptr, 0, true, THREAD(kEyrie)},
};
constexpr int kConvoCount = (int)(sizeof(kConvos) / sizeof(kConvos[0]));

#undef THREAD

} // namespace

// --- persistence backend ---------------------------------------------------

SimStoreBackend::SimStoreBackend() { path_ = getenv("MERLIN_STORE"); }

bool SimStoreBackend::read(const char *ns, void *out, size_t cap, size_t &len) {
  (void)ns;
  if (path_) {
    FILE *fp = fopen(path_, "rb");
    if (!fp) return false;
    len = fread(out, 1, cap, fp);
    fclose(fp);
    return len > 0;
  }
  if (blob_len_ == 0 || blob_len_ > cap) return false;
  memcpy(out, blob_, blob_len_);
  len = blob_len_;
  return true;
}

bool SimStoreBackend::write(const char *ns, const void *data, size_t len) {
  (void)ns;
  if (path_) {
    FILE *fp = fopen(path_, "wb");
    if (!fp) return false;
    size_t w = fwrite(data, 1, len, fp);
    fclose(fp);
    return w == len;
  }
  if (len > sizeof(blob_)) return false;
  memcpy(blob_, data, len);
  blob_len_ = len;
  return true;
}

// --- model -----------------------------------------------------------------

FixtureModel::FixtureModel() {
  store_.attach(&backend_);
  // With MERLIN_STORE set, a run resumes where the last one stopped — the
  // same code path SPIFFS will take on the device. Without it the load fails
  // and we fall back to the fixture fleet, which is what the goldens replay.
  if (!store_.load()) seed();
}

bool FixtureModel::persist() { return store_.save(); }

void FixtureModel::seed() {
  store_.clear();
  for (int i = 0; i < kConvoCount; ++i) {
    const ConvoFix &f = kConvos[i];
    uint8_t derived[channel::KEY_BYTES];
    const uint8_t *key = f.key;
    if (!key && f.kind == CV_HASHTAG) {
      channel::deriveHashtagKey(f.name, derived);
      key = derived;
    }
    int c = store_.addConvo(f.name, f.kind, key);
    if (c < 0) continue;
    for (int j = 0; j < f.count; ++j) {
      const MsgFix &mf = f.msgs[j];
      store_.append(c, mf.sender, mf.text, mf.ts, mf.own, mf.ack, false);
    }
    // Unread is a property of the conversation, not of extra traffic: set it
    // straight rather than faking messages to produce a count.
    store_.setUnread(c, f.unread);
    store_.setMuted(c, f.muted);
  }
}

void FixtureModel::generateChannelKey(uint8_t *key) {
  channel::generateKey(key_seed_++, key);
}

StatusView FixtureModel::status() {
  StatusView s;
  s.batt_pct = 78;
  s.batt_mv = 4020;
  s.charging = true;
  s.gps = GPS_FIX;
  s.sats = 9;
  s.ble = true;
  s.clock = clock_;
  s.unread_total = (uint8_t)store_.unreadTotal();
  s.rx_blip = rx_;
  s.tx_blip = tx_;
  return s;
}

DeviceView FixtureModel::device() {
  DeviceView d;
  d.name = "MERLIN525";
  d.fw = "1.16.0";
  d.freq = "910.525";
  d.bw = "62.5";
  d.sf = 7;
  d.cr = 5;
  d.tx_dbm = 22;
  d.ble_peer = "iPhone";
  d.rx_today = 41;
  d.tx_today = 12;
  d.nodes_24h = 7;
  d.lat = kSelfLat;
  d.lon = kSelfLon;
  d.has_pos = true;
  d.uptime = "3d 4h";
  d.free_heap = 141u * 1024u;
  return d;
}

int FixtureModel::convoCount() { return store_.convoCount(); }

ConvoView FixtureModel::convo(int i) {
  ConvoView c;
  if (!store_.valid(i)) return c;
  c.name = store_.name(i);
  c.channel = store_.isChannel(i);
  c.unread = store_.unread(i);
  c.snippet = store_.snippet(i);
  c.ts = store_.lastTs(i);
  c.muted = store_.muted(i);
  c.tag = c.channel ? "" : "DM";
  return c;
}

int FixtureModel::msgCount(int convo) { return store_.msgCount(convo); }

MsgView FixtureModel::msg(int convo, int i) { return store_.msg(convo, i); }

int FixtureModel::channelConvo(int i) const {
  int seen = 0;
  for (int c = 0; c < store_.convoCount(); ++c) {
    if (!store_.isChannel(c)) continue;
    if (seen++ == i) return c;
  }
  return -1;
}

int FixtureModel::channelCount() {
  int n = 0;
  for (int c = 0; c < store_.convoCount(); ++c)
    if (store_.isChannel(c)) ++n;
  return n;
}

ChannelView FixtureModel::channel(int i) {
  ChannelView v;
  int c = channelConvo(i);
  if (c < 0) return v;
  v.name = store_.name(c);
  v.hashtag = store_.kind(c) == CV_HASHTAG;
  v.unread = store_.unread(c);
  v.undeletable = strcmp(v.name, "#Public") == 0;
  v.convo = c;
  hex_slot_ = (hex_slot_ + 1) % 4;
  store_.keyHex(c, key_hex_[hex_slot_], sizeof(key_hex_[hex_slot_]));
  v.key_hex = key_hex_[hex_slot_];
  return v;
}

int FixtureModel::nodeCount() { return kNodeCount; }

NodeView FixtureModel::node(int i) {
  NodeView n;
  if (i < 0 || i >= kNodeCount) return n;
  const NodeFix &f = kNodes[i];
  n.name = f.name;
  n.kind = f.kind;
  n.last_heard_secs = f.age;
  n.flood_hops = f.flood;
  n.route_hops = f.route;
  n.direct = f.direct;
  n.snr_x4 = f.snr_x4;
  n.rssi = f.rssi;
  n.has_pos = f.has_pos;
  n.lat = f.lat;
  n.lon = f.lon;
  n.alt_m = f.alt;
  n.pubkey6 = f.key;
  n.fw = f.fw;
  n.power = f.power;
  return n;
}

void FixtureModel::trackSend(uint32_t id, int convo) {
  if (pending_n_ >= (int)(sizeof(pending_) / sizeof(pending_[0]))) return;
  pending_[pending_n_].id = id;
  pending_[pending_n_].convo = convo;
  pending_[pending_n_].stage = 0;
  ++pending_n_;
}

void FixtureModel::advanceAcks() {
  int keep = 0;
  for (int i = 0; i < pending_n_; ++i) {
    Pending p = pending_[i];
    ++p.stage;
    if (p.stage == 1) {
      store_.setAck(p.id, ACK_SENT, 0);
      pending_[keep++] = p;
    } else {
      // A channel message is "heard repeated"; a DM gets a real delivery ack.
      if (store_.isChannel(p.convo)) store_.setAck(p.id, ACK_HEARD_N, 2);
      else store_.setAck(p.id, ACK_DELIVERED, 0);
    }
  }
  pending_n_ = keep;
}

int FixtureModel::inject(const char *convo_name, const char *sender,
                         const char *text) {
  int c = store_.findConvo(convo_name);
  if (c < 0) c = store_.addConvo(convo_name, CV_DM, nullptr);
  if (c < 0) return -1;
  store_.append(c, sender, text, clock_, false, ACK_NONE, true);
  return c;
}

// --- command sink ----------------------------------------------------------

void FixtureSink::sendMsg(int convo, const char *text) {
  printf("    [cmd] sendMsg %d: %s\n", convo, text);
  if (!model) return;
  uint32_t id = model->store().append(convo, "MERLIN525", text, model->clock(),
                                      true, ACK_QUEUED, false);
  model->trackSend(id, convo);
}

void FixtureSink::sendAdvert() { printf("    [cmd] sendAdvert\n"); }

void FixtureSink::muteConvo(int convo, bool muted) {
  printf("    [cmd] muteConvo %d -> %d\n", convo, (int)muted);
  if (model) model->store().setMuted(convo, muted);
}

void FixtureSink::markRead(int convo) {
  if (model) model->store().markRead(convo);
}

void FixtureSink::pingNode(int node) { printf("    [cmd] pingNode %d\n", node); }

void FixtureSink::setRadio(const char *freq, const char *bw, uint8_t sf,
                           uint8_t cr, int8_t tx_dbm) {
  printf("    [cmd] setRadio %s/%s SF%u CR%u %d dBm\n", freq, bw, (unsigned)sf,
         (unsigned)cr, (int)tx_dbm);
}

void FixtureSink::setBrightness(uint8_t pct) {
  printf("    [cmd] setBrightness %u\n", (unsigned)pct);
}

void FixtureSink::joinHashtag(const char *name) {
  printf("    [cmd] joinHashtag %s\n", name);
  if (!model) return;
  uint8_t key[channel::KEY_BYTES];
  channel::deriveHashtagKey(name, key);
  model->store().addConvo(name, CV_HASHTAG, key);
}

void FixtureSink::createChannel(const char *name) {
  printf("    [cmd] createChannel %s\n", name);
  if (!model) return;
  uint8_t key[channel::KEY_BYTES];
  model->generateChannelKey(key);
  model->store().addConvo(name, CV_PRIVATE, key);
}

void FixtureSink::joinChannel(const char *name, const uint8_t *key16) {
  printf("    [cmd] joinChannel %s\n", name);
  if (model) model->store().addConvo(name, CV_PRIVATE, key16);
}

void FixtureSink::removeChannel(int idx) {
  printf("    [cmd] removeChannel %d\n", idx);
  if (!model) return;
  int c = model->channelConvo(idx);
  if (c >= 0) model->store().removeConvo(c);
}

void FixtureSink::shareChannel(int idx, int node_idx) {
  if (!model) return;
  int c = model->channelConvo(idx);
  if (c < 0) return;
  char join[channel::JOIN_CAP];
  const uint8_t *key = model->store().key(c);
  channel::formatJoin(model->store().name(c), key, key != nullptr, join,
                      sizeof(join));
  printf("    [cmd] shareChannel %d -> node %d: %s\n", idx, node_idx, join);

  const char *peer = model->node(node_idx).name;
  int dm = model->store().findConvo(peer);
  if (dm < 0) dm = model->store().addConvo(peer, CV_DM, nullptr);
  if (dm < 0) return;
  uint32_t id = model->store().append(dm, "MERLIN525", join, model->clock(),
                                      true, ACK_QUEUED, false);
  model->trackSend(id, dm);
}

void FixtureSink::openDm(int node_idx) {
  if (!model) return;
  const char *peer = model->node(node_idx).name;
  printf("    [cmd] openDm %s\n", peer);
  model->store().addConvo(peer, CV_DM, nullptr);
}
