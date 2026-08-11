#include "channels.h"

#include "../textutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

namespace merlin {
namespace screens {

using namespace theme;

static constexpr int16_t ROW_H = 26;
static constexpr int16_t LIST_Y = CONTENT_Y + STRIP_H; // 28

static bool isHexDigit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

// --- the channel list ------------------------------------------------------

void ChannelsScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  mode_ = M_LIST;
  entry_.clear();
  // The entry is only ever a channel name, and a channel name is what the radio
  // will hold: bounded here so createChannel cannot truncate underneath us. It
  // used to accept the widget's 40, so a longer name came back from the model
  // shortened, the post-create match failed, Share was never reached — and each
  // retry minted a fresh key into another of the 40 firmware slots.
  entry_.setMax((int)channel::NAME_CAP - 1);
  hex_len_ = 0;
  hex_[0] = 0;
  key_error_ = false;
  list_.setRowHeight(ROW_H);
  list_.setArea(Rect(0, LIST_Y, SCREEN_W, (int16_t)(SCREEN_H - TAB_H - LIST_Y)));
  rebuild(m);
}

void ChannelsScreen::rebuild(UiModel &m) {
  chan_count_ = m.channelCount();
  list_.setCount(chan_count_ + 2); // + create + join-with-key
}

bool ChannelsScreen::keyEvent(const InputEvent &e, Router &r, UiModel &m,
                              CommandSink *cmd) {
  const bool enter = (e.kind == InputEvent::KEY && e.ch == '\n') ||
                     (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK);

  switch (mode_) {
    case M_ADD:
      if (enter) {
        if (entry_.len() < 2) return true; // "#" alone is not a channel
        if (cmd) cmd->createChannel(entry_.text());
        rebuild(m);
        for (int i = 0; i < m.channelCount(); ++i) {
          if (strcmp(m.channel(i).name, entry_.text()) != 0) continue;
          mode_ = M_LIST;
          list_.select(i);
          ChannelScreen *cs = static_cast<ChannelScreen *>(r.screen(SC_CHANNEL));
          if (cs) cs->setFresh(true);
          r.push(SC_CHANNEL, i);
          return true;
        }
        mode_ = M_LIST; // the command did not land; say nothing we cannot show
        return true;
      }
      break;

    case M_JOIN_NAME:
      if (enter) {
        if (entry_.len() < 2) return true;
        snprintf(join_name_, sizeof(join_name_), "%s", entry_.text());
        mode_ = M_JOIN_KEY;
        hex_len_ = 0;
        hex_[0] = 0;
        key_error_ = false;
        return true;
      }
      break;

    case M_JOIN_KEY: {
      if (enter) {
        uint8_t key[channel::KEY_BYTES];
        if (hex_len_ != channel::KEY_HEX ||
            channel::fromHex(hex_, key, channel::KEY_BYTES) !=
                channel::KEY_BYTES) {
          key_error_ = true; // a short key is a wrong key, not a partial one
          return true;
        }
        if (cmd) cmd->joinChannel(join_name_, key);
        mode_ = M_LIST;
        rebuild(m);
        return true;
      }
      if (e.kind != InputEvent::KEY) return false;
      if (e.ch == '\b') {
        if (hex_len_ > 0) {
          hex_[--hex_len_] = 0;
          key_error_ = false;
        } else {
          mode_ = M_JOIN_NAME;
        }
        return true;
      }
      if (e.ch == ' ') return true; // groups are drawn, not typed
      if (isHexDigit(e.ch)) {
        if (hex_len_ < channel::KEY_HEX) {
          // Lower-cased on the way in: a key is a value, not a spelling.
          hex_[hex_len_++] = (e.ch >= 'A' && e.ch <= 'F') ? (char)(e.ch + 32)
                                                          : e.ch;
          hex_[hex_len_] = 0;
          key_error_ = false;
        }
        return true;
      }
      return true; // non-hex never reaches the buffer
    }

    default:
      break;
  }

  // Name entry for M_ADD / M_JOIN_NAME.
  if (e.kind != InputEvent::KEY) return false;
  if (e.ch == '\b' && entry_.empty()) {
    mode_ = M_LIST;
    return true;
  }
  return entry_.onKey(e.ch);
}

bool ChannelsScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                             CommandSink *cmd) {
  if (mode_ != M_LIST) return keyEvent(e, r, m, cmd);

  if (list_.onEvent(e)) return true;

  bool open = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
              (e.kind == InputEvent::KEY && e.ch == '\n') ||
              (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
               list_.hitTest(e.tx, e.ty) == list_.sel());
  if (open) {
    int i = list_.sel();
    if (i < chan_count_) {
      ChannelScreen *cs = static_cast<ChannelScreen *>(r.screen(SC_CHANNEL));
      if (cs) cs->setFresh(false);
      r.push(SC_CHANNEL, i);
    } else if (i == chan_count_) {
      mode_ = M_ADD;
      entry_.set("#");
    } else {
      mode_ = M_JOIN_NAME;
      entry_.set("#");
    }
    return true;
  }
  return false;
}

void ChannelsScreen::renderKeyEntry(Gfx &g) {
  char buf[48];
  snprintf(buf, sizeof(buf), "JOIN %s", join_name_);
  titleStrip(g, buf, "key");

  hint(g, 32, "read the 32-digit key off the other radio");

  // Four groups per row, two rows: hex is copied by eye, and eyes count in 4s.
  static constexpr int16_t GROUP_W = 40, PITCH = 52, X0 = 60;
  for (int grp = 0; grp < 8; ++grp) {
    int16_t gx = (int16_t)(X0 + (grp % 4) * PITCH);
    int16_t gy = (int16_t)(48 + (grp / 4) * 30);
    bool active = (grp == hex_len_ / 4) && hex_len_ < channel::KEY_HEX;
    char cell[5] = {0, 0, 0, 0, 0};
    int have = hex_len_ - grp * 4;
    if (have > 4) have = 4;
    for (int i = 0; i < have; ++i) cell[i] = hex_[grp * 4 + i];
    g.text(gx, gy, cell, F_BODY, have == 4 ? FG : ACCENT);
    // Every slot keeps a visible rule: the count of empty groups is how you
    // know how much key is left to read out.
    g.hline(gx, (int16_t)(gy + 16), GROUP_W, active ? ACCENT : DIM);
    if (active)
      vline(g, (int16_t)(gx + g.textWidth(cell, F_BODY) + 1), (int16_t)(gy + 1),
            14, ACCENT);
  }

  // Live checksum: the digest of what is typed so far, so two people reading a
  // key aloud find out they disagree before they trust the channel.
  uint8_t key[channel::KEY_BYTES];
  int bytes = channel::fromHex(hex_, key, channel::KEY_BYTES);
  bool full = hex_len_ == channel::KEY_HEX && bytes == channel::KEY_BYTES;
  g.text(60, 116, "checksum", F_SMALL, MUTED);
  if (bytes > 0) {
    snprintf(buf, sizeof(buf), "%02x", (unsigned)channel::checksum(key, bytes));
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  g.text(120, 112, buf, F_BOLD, full ? OK : WARN);
  if (full) icons::ackDelivered(g, 146, 113, OK);

  snprintf(buf, sizeof(buf), "%d/%d", hex_len_, channel::KEY_HEX);
  textRight(g, 260, 116, buf, F_SMALL, full ? OK : MUTED);

  if (key_error_) {
    snprintf(buf, sizeof(buf), "a key is exactly %d digits - you have %d",
             channel::KEY_HEX, hex_len_);
    g.fillRect(Rect(0, 136, SCREEN_W, 14), rgb(0x38, 0x0E, 0x10));
    g.text(6, 139, buf, F_SMALL, ALERT);
  }
  hint(g, 158, full ? "Enter joins . Bksp edits"
                    : "hex digits only . Bksp edits . L back");
}

void ChannelsScreen::render(Gfx &g, UiModel &m) {
  char buf[64];

  if (mode_ == M_JOIN_KEY) { renderKeyEntry(g); return; }

  if (mode_ == M_ADD || mode_ == M_JOIN_NAME) {
    const bool add = mode_ == M_ADD;
    titleStrip(g, add ? "NEW CHANNEL" : "JOIN CHANNEL", "");
    entry_.render(g, Rect(0, LIST_Y, SCREEN_W, 20), "name:", FG);
    if (add) {
      hint(g, 56, "a 128-bit key is generated for you");
      hint(g, 68, "share it from the channel screen - without");
      hint(g, 80, "the key, nobody else can read the channel");
    } else {
      hint(g, 56, "next: the 32-digit key from the other radio");
      hint(g, 68, "for a public channel, join it from Chats");
      hint(g, 80, "with n and a #name instead");
    }
    hint(g, 104, "Enter continues . Bksp cancels");
    return;
  }

  snprintf(buf, sizeof(buf), "%d", chan_count_);
  titleStrip(g, "CHANNELS", buf);

  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());
    if (sel) g.fillRect(rr, ACCENT);
    Color fg = sel ? SEL_FG : FG;
    Color mut = sel ? SEL_MUTED : MUTED;

    if (i >= chan_count_) {
      const char *label = (i == chan_count_) ? "+  Create private channel"
                                             : "+  Join with a key";
      g.text(8, (int16_t)(rr.y + 5), label, sel ? F_BOLD : F_BODY,
             sel ? SEL_FG : ACCENT);
      if (!sel && i < list_.lastVisible())
        g.hline(0, (int16_t)(rr.bottom() - 1), SCREEN_W, RULE);
      continue;
    }

    ChannelView c = m.channel(i);
    icons::dot(g, 6, (int16_t)(rr.y + 11), c.unread > 0,
               sel ? SEL_FG : (c.unread ? ALERT : DIM));
    g.clip(Rect(16, rr.y, 220, rr.h));
    g.text(16, (int16_t)(rr.y + 4), c.name, sel ? F_BOLD : F_BODY, fg);
    g.unclip();
    textRight(g, 300, (int16_t)(rr.y + 9), c.hashtag ? "public" : "private",
              F_SMALL, sel ? fg : (c.hashtag ? MUTED : OK));
    icons::chevronRight(g, 304, (int16_t)(rr.y + 8), mut);
    if (!sel && i < list_.lastVisible())
      g.hline(0, (int16_t)(rr.bottom() - 1), SCREEN_W, RULE);
  }
  list_.drawScrollbar(g);
}

// --- one channel -----------------------------------------------------------

void ChannelScreen::onEnter(int arg, UiModel &m) {
  idx_ = arg;
  sel_ = 0;
  reveal_ = false;
  shared_to_[0] = 0;
  confirm_.clear();
  ChannelView c = m.channel(idx_);
  snprintf(name_, sizeof(name_), "%s", c.name);
  deletable_ = !c.undeletable;
  mode_ = fresh_ ? M_SHARE : M_DETAIL;

  row_count_ = 0;
  int n = m.nodeCount();
  if (n > MAX_ROWS) n = MAX_ROWS;
  for (int i = 0; i < n; ++i)
    if (m.node(i).kind != NK_SELF) rows_[row_count_++] = (int16_t)i;
  list_.setRowHeight(22);
  list_.setArea(Rect(0, 74, SCREEN_W, (int16_t)(SCREEN_H - TAB_H - 74)));
  list_.setCount(row_count_);
}

bool ChannelScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                            CommandSink *cmd) {
  const bool enter = (e.kind == InputEvent::KEY && e.ch == '\n') ||
                     (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK);

  if (mode_ == M_GONE) {
    r.pop(); // the channel is not there to look at any more
    return true;
  }

  if (mode_ == M_SHARE) {
    if (list_.onEvent(e)) return true;
    if (enter && row_count_ > 0) {
      int node = rows_[list_.sel()];
      if (cmd) cmd->shareChannel(idx_, node);
      snprintf(shared_to_, sizeof(shared_to_), "%s", m.node(node).name);
      mode_ = M_DETAIL;
      fresh_ = false;
      return true;
    }
    if (e.kind == InputEvent::KEY && e.ch == '\b') {
      mode_ = M_DETAIL;
      fresh_ = false;
      return true;
    }
    return false;
  }

  if (mode_ == M_DELETE) {
    if (enter) {
      if (strcmp(confirm_.text(), name_) != 0) return true;
      Confirm::Spec s;
      s.title = "DELETE CHANNEL?";
      s.detail = name_;
      s.warn = "The key is gone for good.";
      s.cancel = "Keep";
      s.accept = "Delete";
      r.showConfirm(s, this, ACT_DELETE);
      return true;
    }
    if (e.kind != InputEvent::KEY) return false;
    if (e.ch == '\b' && confirm_.empty()) {
      mode_ = M_DETAIL;
      return true;
    }
    return confirm_.onKey(e.ch);
  }

  // --- detail ---
  if (e.kind == InputEvent::NAV) {
    if (e.nav == InputEvent::U) { if (sel_ > 0) --sel_; return true; }
    if (e.nav == InputEvent::D) { if (sel_ + 1 < rowCount()) ++sel_; return true; }
  }
  if (enter) {
    if (sel_ == 0) reveal_ = !reveal_;
    else if (sel_ == 1) { mode_ = M_SHARE; shared_to_[0] = 0; }
    else if (sel_ == 2 && deletable_) { mode_ = M_DELETE; confirm_.clear(); }
    return true;
  }
  return false;
}

void ChannelScreen::onConfirm(int action, bool accepted, CommandSink *cmd) {
  if (action != ACT_DELETE || !accepted) {
    mode_ = M_DETAIL;
    return;
  }
  if (cmd) cmd->removeChannel(idx_);
  mode_ = M_GONE;
}

void ChannelScreen::renderShare(Gfx &g, UiModel &m) {
  char buf[64];
  snprintf(buf, sizeof(buf), "SHARE %s", name_);
  titleStrip(g, buf, fresh_ ? "created" : "");

  if (fresh_) {
    g.fillRect(Rect(0, 28, SCREEN_W, 14), rgb(0x06, 0x28, 0x2D));
    g.text(6, 31, "channel created - key generated on this device", F_SMALL, OK);
  }
  hint(g, fresh_ ? 46 : 32, "sends an encrypted invite as a direct message");
  hint(g, fresh_ ? 58 : 44, "one Enter on their side joins them");

  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    NodeView v = m.node(rows_[i]);
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());
    if (sel) g.fillRect(rr, ACCENT);
    Color fg = sel ? SEL_FG : FG;
    if (v.kind == NK_REPEATER) icons::repeater(g, 4, (int16_t)(rr.y + 7), fg);
    else icons::companion(g, 4, (int16_t)(rr.y + 7), fg);
    g.clip(Rect(18, rr.y, 240, rr.h));
    g.text(18, (int16_t)(rr.y + 2), v.name, sel ? F_BOLD : F_BODY, fg);
    g.unclip();
    char age[16];
    text::formatAge(v.last_heard_secs, age, sizeof(age));
    textRight(g, 312, (int16_t)(rr.y + 7), age, F_SMALL,
              sel ? SEL_MUTED : MUTED);
  }
  list_.drawScrollbar(g);
}

void ChannelScreen::renderDelete(Gfx &g) {
  char buf[64];
  titleStrip(g, name_, "delete");
  g.fillRect(Rect(0, 28, SCREEN_W, 14), rgb(0x38, 0x0E, 0x10));
  g.text(6, 31, "this cannot be undone", F_SMALL, ALERT);
  hint(g, 48, "the key is stored nowhere else. Delete it and");
  hint(g, 60, "this radio can never read the channel again.");

  snprintf(buf, sizeof(buf), "type  %s  to confirm", name_);
  g.text(6, 82, buf, F_SMALL, MUTED);
  confirm_.render(g, Rect(0, 94, SCREEN_W, 20), "name:", FG);

  bool match = strcmp(confirm_.text(), name_) == 0;
  if (match) {
    icons::ackDelivered(g, 6, 120, OK);
    g.text(20, 119, "Enter to delete", F_SMALL, OK);
  } else {
    g.text(6, 119, "Bksp cancels", F_SMALL, MUTED);
  }
}

void ChannelScreen::renderDetail(Gfx &g, UiModel &m) {
  ChannelView c = m.channel(idx_);
  titleStrip(g, name_, c.hashtag ? "public" : "private");

  field(g, 32, "Type", c.hashtag ? "public hashtag" : "private key",
        c.hashtag ? FG : OK);
  hint(g, 52, c.hashtag ? "the key derives from the name - anyone who"
                        : "128-bit key, generated on this device and");
  hint(g, 64, c.hashtag ? "knows the name can read it" : "shared only by you");

  // Guardrail: the key never renders until it is asked for.
  char masked[channel::KEY_HEX + 1];
  const char *shown = c.key_hex;
  if (!reveal_) {
    int n = (int)sizeof(masked) - 1;
    for (int i = 0; i < n; ++i) masked[i] = '.';
    masked[n] = 0;
    shown = masked;
  }
  g.text(6, 86, "Key", F_SMALL, MUTED);
  g.text(40, 84, *c.key_hex ? shown : "(derived)", F_SMALL,
         reveal_ ? WARN : DIM);

  static const char *kRow[3] = {"Reveal key", "Share to contact",
                                "Delete channel"};
  for (int i = 0; i < rowCount(); ++i) {
    Rect rr(0, (int16_t)(102 + i * 24), SCREEN_W, 22);
    bool sel = (i == sel_);
    if (sel) g.fillRect(rr, ACCENT);
    const char *label = (i == 0 && reveal_) ? "Hide key" : kRow[i];
    Color c2 = sel ? SEL_FG : (i == 2 ? ALERT : FG);
    g.text(8, (int16_t)(rr.y + 2), label, sel ? F_BOLD : F_BODY, c2);
    icons::chevronRight(g, 304, (int16_t)(rr.y + 6), sel ? SEL_FG : MUTED);
    if (!sel) g.hline(0, (int16_t)(rr.bottom() - 1), SCREEN_W, RULE);
  }

  if (!deletable_)
    hint(g, 180, "#Public is part of every MeshCore radio");
  if (shared_to_[0]) {
    char buf[64];
    snprintf(buf, sizeof(buf), "invite sent to %s", shared_to_);
    g.fillRect(Rect(0, 196, SCREEN_W, 14), rgb(0x06, 0x28, 0x2D));
    g.text(6, 199, buf, F_SMALL, OK);
  }
}

void ChannelScreen::render(Gfx &g, UiModel &m) {
  switch (mode_) {
    case M_SHARE: renderShare(g, m); break;
    case M_DELETE: renderDelete(g); break;
    case M_GONE:
      titleStrip(g, name_, "deleted");
      hint(g, 40, "channel deleted");
      break;
    default: renderDetail(g, m); break;
  }
}

} // namespace screens
} // namespace merlin
