#include "thread.h"

#include "../channel.h"
#include "../textutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

namespace merlin {
namespace screens {

using namespace theme;

static constexpr int16_t ROW_H = 26;
static constexpr int16_t LIST_Y = CONTENT_Y + STRIP_H;            // 28
static constexpr int16_t COMPOSER_Y = SCREEN_H - TAB_H - STRIP_H; // 212

void ThreadScreen::onEnter(int arg, UiModel &m) {
  convo_ = arg;
  composer_.close();
  list_.setRowHeight(ROW_H);
  list_.setArea(Rect(0, LIST_Y, SCREEN_W, (int16_t)(COMPOSER_Y - LIST_Y)));
  last_count_ = m.msgCount(convo_);
  list_.setCount(last_count_);
  list_.select(last_count_ - 1); // newest in view, like every messenger
}

// An invite is data, not prose: parse before rendering so the row can offer the
// action instead of showing the user a URL to retype.
static bool joinInvite(const MsgView &msg, char *name, size_t name_cap,
                       uint8_t *key, bool &has_key) {
  return channel::parseJoin(msg.text, name, name_cap, key, has_key);
}

bool ThreadScreen::activateRow(Router &r, UiModel &m, CommandSink *cmd) {
  int i = list_.sel();
  if (i < 0 || i >= m.msgCount(convo_) || !cmd) return false;
  MsgView msg = m.msg(convo_, i);

  char name[channel::NAME_CAP];
  uint8_t key[channel::KEY_BYTES];
  bool has_key = false;
  if (joinInvite(msg, name, sizeof(name), key, has_key)) {
    if (has_key) cmd->joinChannel(name, key);
    else cmd->joinHashtag(name);
    // Land in the channel you just joined; "it worked" is better shown than
    // announced. Nothing below this line touches our own state.
    for (int c = 0; c < m.convoCount(); ++c) {
      if (strcmp(m.convo(c).name, name) != 0) continue;
      r.pop();
      r.push(SC_THREAD, c);
      break;
    }
    return true;
  }
  // Design spec: Enter on a failed message retries it.
  if (msg.own && msg.ack == ACK_FAILED) {
    cmd->sendMsg(convo_, msg.text);
    return true;
  }
  return false;
}

bool ThreadScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                           CommandSink *cmd) {
  (void)r;
  // --- compose mode owns the keyboard -------------------------------------
  if (composer_.active()) {
    if (e.kind == InputEvent::KEY) {
      bool sent = false;
      composer_.onKey(e.ch, sent);
      if (sent) {
        if (cmd) cmd->sendMsg(convo_, composer_.text());
        composer_.close();
      }
      return true;
    }
    if (e.kind == InputEvent::NAV && e.nav == InputEvent::L) {
      composer_.close();
      return true;
    }
    if (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) {
      if (composer_.len() > 0 && cmd) cmd->sendMsg(convo_, composer_.text());
      composer_.close();
      return true;
    }
    if (list_.onEvent(e)) return true;
    // Nothing else escapes: a stray swipe must not throw away a draft.
    return true;
  }

  if (list_.onEvent(e)) return true;

  bool open = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
              (e.kind == InputEvent::KEY && e.ch == '\n') ||
              (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
               list_.hitTest(e.tx, e.ty) == list_.sel());
  if (open && activateRow(r, m, cmd)) return true;

  if (e.kind != InputEvent::KEY) return false;
  // The BlackBerry rule: any printable key starts a message. Tab digits lose
  // here on purpose — inside a conversation, typing beats navigating.
  if (e.ch >= ' ' && e.ch < 0x7F) {
    composer_.open();
    bool sent = false;
    composer_.onKey(e.ch, sent);
    list_.select(m.msgCount(convo_) - 1);
    return true;
  }
  return false;
}

static void ackGlyph(Gfx &g, int16_t y, const MsgView &msg, bool sel) {
  switch (msg.ack) {
    case ACK_QUEUED: icons::ackQueued(g, 300, y, sel ? SEL_FG : MUTED); break;
    case ACK_SENT: icons::ackSent(g, 300, y, sel ? SEL_FG : MUTED); break;
    case ACK_DELIVERED: icons::ackDelivered(g, 300, y, sel ? SEL_FG : OK); break;
    case ACK_HEARD_N: {
      icons::ackRepeat(g, 292, y, sel ? SEL_FG : OK);
      char b[8];
      snprintf(b, sizeof(b), "%u", (unsigned)msg.heard_n);
      g.text(302, (int16_t)(y + 1), b, F_SMALL, sel ? SEL_FG : OK);
      break;
    }
    case ACK_FAILED:
      g.text(302, (int16_t)(y - 3), "!", F_BOLD, sel ? SEL_FG : ALERT);
      break;
    default: break;
  }
}

void ThreadScreen::render(Gfx &g, UiModel &m) {
  // New traffic can land between events (an incoming message, our own send),
  // so the row count is reconciled here rather than only on entry.
  int n = m.msgCount(convo_);
  if (n != last_count_) {
    bool was_at_end = list_.sel() >= last_count_ - 1;
    list_.setCount(n);
    if (was_at_end || composer_.active()) list_.select(n - 1);
    last_count_ = n;
  }

  int16_t bottom = composer_.active()
                       ? (int16_t)(SCREEN_H - composer_.height(g))
                       : COMPOSER_Y;
  list_.setArea(Rect(0, LIST_Y, SCREEN_W, (int16_t)(bottom - LIST_Y)));
  list_.ensureVisible();

  ConvoView c = m.convo(convo_);
  char buf[48];
  snprintf(buf, sizeof(buf), "%d heard", n);
  titleStrip(g, c.name, buf);

  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    MsgView msg = m.msg(convo_, i);
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());
    if (sel) g.fillRect(rr, ACCENT);

    Color name_c =
        sel ? SEL_FG : (msg.own ? ACCENT : theme::senderColor(msg.sender));
    g.text(4, (int16_t)(rr.y + 1), msg.sender, F_SMALL, name_c);
    text::formatClock(msg.ts, buf, sizeof(buf));
    g.text((int16_t)(10 + g.textWidth(msg.sender, F_SMALL)), (int16_t)(rr.y + 1),
           buf, F_SMALL, sel ? SEL_MUTED : MUTED);

    char jname[channel::NAME_CAP];
    uint8_t jkey[channel::KEY_BYTES];
    bool has_key = false;
    if (joinInvite(msg, jname, sizeof(jname), jkey, has_key)) {
      // An invite gets an action row, never the raw join string.
      Rect box(4, (int16_t)(rr.y + 10), 300, 14);
      frameRect(g, box, sel ? SEL_FG : ACCENT);
      snprintf(buf, sizeof(buf), "Join %s?", jname);
      g.text(8, (int16_t)(box.y + 2), buf, F_SMALL, sel ? SEL_FG : ACCENT);
      textRight(g, 300, (int16_t)(box.y + 2),
                has_key ? "private . Enter joins" : "Enter joins", F_SMALL,
                sel ? SEL_MUTED : MUTED);
      continue;
    }

    g.clip(Rect(4, rr.y, 284, rr.h));
    g.text(4, (int16_t)(rr.y + 9), msg.text, F_BODY, sel ? SEL_FG : FG);
    g.unclip();

    if (msg.own) ackGlyph(g, (int16_t)(rr.y + 10), msg, sel);
  }

  if (composer_.active()) {
    composer_.render(g);
  } else {
    g.fillRect(Rect(0, COMPOSER_Y, SCREEN_W, STRIP_H), HEADER_BG);
    g.hline(0, COMPOSER_Y, SCREEN_W, HAIRLINE);
    g.text(4, (int16_t)(COMPOSER_Y + 3), "> type to compose . L back to Chats",
           F_SMALL, MUTED);
  }

  list_.drawScrollbar(g);
}

} // namespace screens
} // namespace merlin
