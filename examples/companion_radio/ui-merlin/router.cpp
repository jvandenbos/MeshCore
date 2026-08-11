#include "router.h"

#include "theme.h"
#include "widgets/statusbar.h"
#include "widgets/tabbar.h"

namespace merlin {

constexpr ScreenId Router::kTabRoot[4];

void Router::start(ScreenId id) {
  splash_ = (id == SC_SPLASH);
  depth_ = 0;
  stack_[0] = id;
  args_[0] = 0;
  if (!splash_) {
    for (int i = 0; i < 4; ++i)
      if (kTabRoot[i] == id) tab_ = i;
  }
  enter(id, 0);
}

void Router::enter(ScreenId id, int arg) {
  if (screens_[id]) screens_[id]->onEnter(arg, model_);
  dirty_ = true;
}

void Router::gotoTab(int tab) {
  if (tab < 0 || tab > 3) return;
  splash_ = false;
  tab_ = tab;
  depth_ = 0;
  stack_[0] = kTabRoot[tab];
  args_[0] = 0;
  enter(stack_[0], 0);
}

void Router::push(ScreenId id, int arg) {
  if (depth_ + 1 >= kMaxDepth) return;
  ++depth_;
  stack_[depth_] = id;
  args_[depth_] = arg;
  enter(id, arg);
}

void Router::pop() {
  if (depth_ == 0) return;
  --depth_;
  enter(stack_[depth_], args_[depth_]);
}

void Router::showBanner(const char *who, const char *text, int convo) {
  banner_.show(who, text, convo, now_);
  dirty_ = true;
}

void Router::showConfirm(const Confirm::Spec &spec, Screen *owner, int action) {
  confirm_.show(spec);
  confirm_owner_ = owner;
  confirm_action_ = action;
  dirty_ = true;
}

void Router::tick(uint32_t now_secs) {
  now_ = now_secs;
  bool was = banner_.active();
  banner_.tick(now_);
  if (was != banner_.active()) dirty_ = true;
  dirty_ = true; // status bar clock/blips repaint every tick (rule 4)
}

void Router::handle(const InputEvent &e) {
  dirty_ = true;

  // Splash owns nothing: any input drops us into Chats.
  if (splash_) { gotoTab(1); return; }

  // 1. Overlay first — a confirm is modal, a banner only claims its own strip.
  if (confirm_.active()) {
    bool resolved = false, accepted = false;
    if (confirm_.onEvent(e, resolved, accepted)) {
      if (resolved && confirm_owner_)
        confirm_owner_->onConfirm(confirm_action_, accepted, cmd_);
      return;
    }
  }
  if (banner_.active()) {
    bool tapped = e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
                  banner_.hit(e.tx, e.ty);
    if (tapped) {
      int convo = banner_.convo();
      banner_.dismiss();
      // Only a message banner is a destination. A tap on any other toast is
      // just a dismissal — navigating on it would mark someone else's
      // conversation read and open a thread nobody asked for.
      if (convo == Banner::NO_CONVO) return;
      if (cmd_) cmd_->markRead(convo);
      gotoTab(1);
      push(SC_THREAD, convo);
      return;
    }
    // Any other interaction lets the toast keep running; it never steals focus.
  }

  // 2. Tab bar taps are router-level, not screen-level.
  Screen *s = screens_[stack_[depth_]];
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::TAP &&
      !(s && s->hidesTabBar())) {
    int t = tabbar::hitTest(e.tx, e.ty);
    if (t >= 0) { gotoTab(t); return; }
  }

  // 3. The screen gets its verbs.
  if (s && s->onEvent(e, *this, model_, cmd_)) return;

  // 4. Grammar fallback, implemented once.
  grammarFallback(e);
}

bool Router::grammarFallback(const InputEvent &e) {
  if (e.kind == InputEvent::KEY) {
    if (e.ch >= '0' && e.ch <= '3') { gotoTab(e.ch - '0'); return true; }
    if (e.ch == '\b') {
      if (depth_ > 0) pop();
      return true;
    }
    return false;
  }
  if (e.kind == InputEvent::NAV) {
    // Horizontal = hierarchy: back/open when deep, tab ring at the root.
    if (e.nav == InputEvent::L) {
      if (depth_ > 0) pop();
      else gotoTab((tab_ + 3) % 4);
      return true;
    }
    if (e.nav == InputEvent::R) {
      if (depth_ == 0) gotoTab((tab_ + 1) % 4);
      return true;
    }
    return false;
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::SWIPE_R) {
    if (depth_ > 0) pop();
    else gotoTab((tab_ + 3) % 4);
    return true;
  }
  if (e.kind == InputEvent::TOUCH && e.tk == InputEvent::SWIPE_L) {
    if (depth_ == 0) gotoTab((tab_ + 1) % 4);
    return true;
  }
  return false;
}

void Router::render(Gfx &g) {
  Screen *s = screens_[stack_[depth_]];
  g.fillRect(Rect(0, 0, theme::SCREEN_W, theme::SCREEN_H), theme::BG);

  if (s) {
    g.clip(s->hidesStatusBar()
               ? Rect(0, 0, theme::SCREEN_W, theme::SCREEN_H)
               : Rect(0, theme::CONTENT_Y, theme::SCREEN_W,
                      (int16_t)(theme::SCREEN_H - theme::CONTENT_Y)));
    s->render(g, model_);
    g.unclip();
  }

  if (!s || !s->hidesStatusBar()) statusbar::render(g, model_.status());
  if (s && !s->hidesTabBar() && !s->hidesStatusBar())
    tabbar::render(g, tab_, model_.status().unread_total > 0);

  banner_.render(g, now_);
  confirm_.render(g);
  dirty_ = false;
}

} // namespace merlin
