#include "newchat.h"

#include "../textutils.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

namespace merlin {
namespace screens {

using namespace theme;

static constexpr int16_t ENTRY_Y = CONTENT_Y + STRIP_H; // 28
static constexpr int16_t ENTRY_H = 20;
static constexpr int16_t HINT_Y = ENTRY_Y + ENTRY_H;    // 48
static constexpr int16_t LIST_Y = HINT_Y + 14;          // 62
static constexpr int16_t ROW_H = 22;

void NewChatScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  entry_.clear();
  list_.setRowHeight(ROW_H);
  list_.setArea(Rect(0, LIST_Y, SCREEN_W, (int16_t)(SCREEN_H - TAB_H - LIST_Y)));
  rebuild(m);
}

void NewChatScreen::rebuild(UiModel &m) {
  row_count_ = 0;
  int n = m.nodeCount();
  if (n > MAX_ROWS) n = MAX_ROWS;
  for (int i = 0; i < n; ++i) {
    NodeView v = m.node(i);
    if (v.kind == NK_SELF) continue;
    if (entry_.len() && text::findFold(v.name, entry_.text()) < 0) continue;
    rows_[row_count_++] = (int16_t)i;
  }
  list_.setCount(row_count_);
}

void NewChatScreen::openConvo(Router &r, UiModel &m, const char *name) {
  for (int i = 0; i < m.convoCount(); ++i) {
    if (strcmp(m.convo(i).name, name) != 0) continue;
    // Land in the conversation itself, not back on the picker.
    r.pop();
    r.push(SC_THREAD, i);
    return;
  }
  r.pop(); // the command did not land; Chats is the honest place to be
}

bool NewChatScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                            CommandSink *cmd) {
  if (!isHashtag() && list_.onEvent(e)) return true;

  bool go = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
            (e.kind == InputEvent::KEY && e.ch == '\n') ||
            (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
             !isHashtag() && list_.hitTest(e.tx, e.ty) == list_.sel());
  if (go) {
    if (isHashtag()) {
      // "#" alone is not a channel — same guard as the Channels screen. Joining
      // it would name a conversation "#" and spend one of the 40 firmware
      // channel slots on it.
      if (entry_.len() < 2) return true;
      if (cmd) cmd->joinHashtag(entry_.text());
      openConvo(r, m, entry_.text());
    } else if (row_count_ > 0) {
      int node = rows_[list_.sel()];
      if (cmd) cmd->openDm(node);
      openConvo(r, m, m.node(node).name);
    }
    return true;
  }

  if (e.kind != InputEvent::KEY) return false;
  if (e.ch == '\b' && entry_.empty()) return false; // router: back to Chats
  if (entry_.onKey(e.ch)) {
    rebuild(m);
    return true;
  }
  return false;
}

void NewChatScreen::render(Gfx &g, UiModel &m) {
  char buf[64];
  titleStrip(g, "NEW MESSAGE", "n");

  entry_.render(g, Rect(0, ENTRY_Y, SCREEN_W, ENTRY_H), "to:", FG);

  if (isHashtag()) {
    hint(g, (int16_t)(HINT_Y + 3), "channel key derives from the name");
    Rect box(8, (int16_t)(LIST_Y + 10), 304, 26);
    frameRect(g, box, ACCENT);
    snprintf(buf, sizeof(buf), "Join %s", entry_.text());
    g.text(14, (int16_t)(box.y + 5), buf, F_BOLD, ACCENT);
    textRight(g, 306, (int16_t)(box.y + 9), "Enter joins", F_SMALL, MUTED);
    hint(g, (int16_t)(box.bottom() + 10),
         "anyone who knows the name can read it");
    hint(g, (int16_t)(box.bottom() + 22),
         "private channels: Settings . Channels");
    return;
  }

  snprintf(buf, sizeof(buf), "%d contact%s . or type #name to join a channel",
           row_count_, row_count_ == 1 ? "" : "s");
  hint(g, (int16_t)(HINT_Y + 3), buf);

  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    NodeView v = m.node(rows_[i]);
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());
    if (sel) g.fillRect(rr, ACCENT);
    Color fg = sel ? SEL_FG : FG;

    if (v.kind == NK_REPEATER) icons::repeater(g, 4, (int16_t)(rr.y + 7), fg);
    else icons::companion(g, 4, (int16_t)(rr.y + 7), fg);

    g.clip(Rect(18, rr.y, 236, rr.h));
    g.text(18, (int16_t)(rr.y + 2), v.name, sel ? F_BOLD : F_BODY, fg);
    g.unclip();

    text::formatAge(v.last_heard_secs, buf, sizeof(buf));
    textRight(g, 312, (int16_t)(rr.y + 7), buf, F_SMALL,
              sel ? SEL_MUTED : MUTED);
  }

  if (row_count_ == 0) {
    const char *msg = "no contact matches";
    int16_t tw = g.textWidth(msg, F_BODY);
    g.text((int16_t)((SCREEN_W - tw) / 2), (int16_t)(LIST_Y + 40), msg, F_BODY,
           MUTED);
  }
  list_.drawScrollbar(g);
}

} // namespace screens
} // namespace merlin
