#include "chats.h"

#include "../textutils.h"
#include "common.h"
#include "thread.h"

#include <stdio.h>

namespace merlin {
namespace screens {

using namespace theme;

static constexpr int16_t ROW_H = 35;

void ChatsScreen::onEnter(int arg, UiModel &m) {
  (void)arg;
  list_.setRowHeight(ROW_H);
  list_.setArea(Rect(0, CONTENT_Y, SCREEN_W, CONTENT_H));
  rebuild(m);
}

void ChatsScreen::rebuild(UiModel &m) {
  int total = m.convoCount();
  if (total > MAX_CONVOS) total = MAX_CONVOS;
  row_count_ = 0;
  for (int i = 0; i < total; ++i) {
    ConvoView c = m.convo(i);
    if (list_.filterLen() && text::findFold(c.name, list_.filter()) < 0) continue;
    rows_[row_count_++] = (int16_t)i;
  }
  hidden_ = total - row_count_;

  // Unread first, then recency. The store hands conversations back in the
  // order they were created, which is not a display order, so sort on both
  // keys here rather than trusting the model to arrive pre-sorted.
  for (int i = 1; i < row_count_; ++i) {
    int16_t v = rows_[i];
    int j = i - 1;
    while (j >= 0) {
      ConvoView a = m.convo(rows_[j]), b = m.convo(v);
      bool swap = (a.unread == 0) != (b.unread == 0) ? a.unread == 0
                                                     : a.ts < b.ts;
      if (!swap) break;
      rows_[j + 1] = rows_[j];
      --j;
    }
    rows_[j + 1] = v;
  }
  list_.setCount(row_count_);
}

bool ChatsScreen::onEvent(const InputEvent &e, Router &r, UiModel &m,
                          CommandSink *cmd) {
  if (list_.onEvent(e)) return true;

  // R belongs to the tab ring at the root (see nodes.cpp for the reasoning);
  // open is click / Enter / tap.
  bool open = (e.kind == InputEvent::NAV && e.nav == InputEvent::CLICK) ||
              (e.kind == InputEvent::KEY && e.ch == '\n') ||
              (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
               list_.hitTest(e.tx, e.ty) == list_.sel());
  if (open && row_count_ > 0) {
    int convo = rows_[list_.sel()];
    // Opening a conversation is what makes it read; the store owns the count,
    // so it goes through the sink like every other mutation.
    if (cmd) cmd->markRead(convo);
    r.push(SC_THREAD, convo);
    return true;
  }

  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::LONG) {
    int i = list_.hitTest(e.tx, e.ty);
    if (i >= 0 && cmd) {
      list_.select(i);
      cmd->muteConvo(rows_[i], !m.convo(rows_[i]).muted);
      return true;
    }
  }

  if (e.kind != InputEvent::KEY) return false;
  if (!list_.filterOpen()) {
    if (e.ch == 'm') {
      if (cmd && row_count_ > 0)
        cmd->muteConvo(rows_[list_.sel()], !m.convo(rows_[list_.sel()]).muted);
      return true;
    }
    if (e.ch == 'n') { r.push(SC_NEWCHAT, 0); return true; }
    if (e.ch == 'r' && row_count_ > 0) {
      int convo = rows_[list_.sel()];
      if (cmd) cmd->markRead(convo);
      r.push(SC_THREAD, convo);
      ThreadScreen *t = static_cast<ThreadScreen *>(r.screen(SC_THREAD));
      if (t) t->beginReply();
      return true;
    }
    if (routerOwnsKey(e.ch, true)) return false;
  }

  bool changed = false;
  if (list_.onFilterKey(e.ch, changed)) {
    if (changed) rebuild(m);
    return true;
  }
  return false;
}

void ChatsScreen::render(Gfx &g, UiModel &m) {
  char buf[32];
  for (int i = list_.top(); i <= list_.lastVisible() && i >= 0; ++i) {
    ConvoView c = m.convo(rows_[i]);
    Rect rr = list_.rowRect(i);
    bool sel = (i == list_.sel());
    if (sel) g.fillRect(rr, ACCENT);

    Color name_c = sel ? SEL_FG : (c.muted ? DIM : FG);
    Color mut = sel ? SEL_MUTED : (c.muted ? DIM : MUTED);

    icons::dot(g, 6, (int16_t)(rr.y + 6), c.unread > 0,
               sel ? SEL_FG : (c.unread ? ALERT : DIM));

    // Park the tag after the name, but never past the column stop — a long
    // repeater name would otherwise run straight through it.
    FontId nf = sel ? F_BOLD : F_BODY;
    int16_t tag_x = (int16_t)(20 + g.textWidth(c.name, nf));
    if (tag_x > 212) tag_x = 212;
    g.clip(Rect(16, rr.y, (int16_t)(tag_x - 20), rr.h));
    g.text(16, (int16_t)(rr.y + 2), c.name, nf, name_c);
    g.unclip();
    if (c.tag && *c.tag)
      g.text(tag_x, (int16_t)(rr.y + 6), c.tag, F_SMALL, mut);
    if (c.muted) g.text(224, (int16_t)(rr.y + 6), "muted", F_SMALL, mut);

    text::formatClock(c.ts, buf, sizeof(buf));
    textRight(g, 312, (int16_t)(rr.y + 6), buf, F_SMALL, mut);

    g.clip(Rect(16, rr.y, 296, rr.h));
    g.text(16, (int16_t)(rr.y + 21), c.snippet, F_SMALL, mut);
    g.unclip();

    if (!sel && i < list_.lastVisible())
      g.hline(0, (int16_t)(rr.bottom() - 1), SCREEN_W, RULE);
  }

  if (list_.filterOpen()) {
    g.fillRect(Rect(0, 199, SCREEN_W, 13), FILTER_BG);
    g.text(4, 202, "filter:", F_SMALL, ACCENT);
    g.text(50, 202, list_.filter(), F_SMALL, FG);
    snprintf(buf, sizeof(buf), "x%d hidden", hidden_);
    textRight(g, 314, 202, buf, F_SMALL, WARN);
  }

  list_.drawScrollbar(g);
}

} // namespace screens
} // namespace merlin
