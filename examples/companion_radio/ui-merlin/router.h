// MERLIN UI — tab ring + depth stack + the grammar, implemented ONCE.
// Screens only handle their own verbs; everything in the design's "six rules"
// that is not screen-specific lives here.
#pragma once

#include "gfx.h"
#include "input.h"
#include "model.h"
#include "widgets/banner.h"
#include "widgets/confirm.h"

namespace merlin {

enum ScreenId : uint8_t {
  SC_SPLASH = 0,
  SC_HOME,
  SC_CHATS,
  SC_THREAD,
  SC_NODES,
  SC_NODE_CARD,
  SC_SETTINGS,
  SC_RADIO,
  SC_NEWCHAT,  // depth 1 under Chats: new DM / join #hashtag
  SC_CHANNELS, // depth 1 under Settings: the channel list
  SC_CHANNEL,  // depth 2: one channel — key, share, delete
  SC_IDENTITY, // depth 1 under Settings: node name editor
  SC_COUNT
};

class Router;

struct Screen {
  virtual ~Screen() {}
  // `arg` is the row the parent screen opened (convo index, node index, ...).
  virtual void onEnter(int arg, UiModel &m) { (void)arg; (void)m; }
  // Return true to consume; false lets the router's grammar fallback run.
  virtual bool onEvent(const InputEvent &e, Router &r, UiModel &m,
                       CommandSink *cmd) = 0;
  virtual void render(Gfx &g, UiModel &m) = 0;
  // Compose mode owns the bottom 70 px, so the tab bar stands down (design
  // spec judgment call, mockup frame 4).
  virtual bool hidesTabBar() const { return false; }
  virtual bool hidesStatusBar() const { return false; }
  // A screen answers false here when R at depth 0 should walk the tab ring
  // instead of opening something.
  virtual void onConfirm(int action, bool accepted, CommandSink *cmd) {
    (void)action; (void)accepted; (void)cmd;
  }
};

class Router {
public:
  Router(UiModel &model, CommandSink *cmd) : model_(model), cmd_(cmd) {}

  void registerScreen(ScreenId id, Screen *s) { screens_[id] = s; }

  // Boot lands on Chats — it's a messenger (design spec §Screen inventory).
  void start(ScreenId id);

  void handle(const InputEvent &e);
  void tick(uint32_t now_secs); // drives banner expiry + status blips
  void render(Gfx &g);

  void gotoTab(int tab);
  void push(ScreenId id, int arg);
  void pop();

  void showBanner(const char *who, const char *text, int convo);
  void showConfirm(const Confirm::Spec &spec, Screen *owner, int action);

  int tab() const { return tab_; }
  int depth() const { return depth_; }
  ScreenId current() const { return stack_[depth_]; }
  int currentArg() const { return args_[depth_]; }
  Screen *screen(ScreenId id) const { return screens_[id]; }
  void markDirty() { dirty_ = true; }
  bool dirty() const { return dirty_; }
  uint32_t now() const { return now_; }

private:
  bool grammarFallback(const InputEvent &e);
  void enter(ScreenId id, int arg);

  UiModel &model_;
  CommandSink *cmd_ = nullptr;
  Screen *screens_[SC_COUNT] = {nullptr};

  static constexpr int kMaxDepth = 3;
  static constexpr ScreenId kTabRoot[4] = {SC_HOME, SC_CHATS, SC_NODES,
                                           SC_SETTINGS};
  ScreenId stack_[kMaxDepth] = {SC_CHATS, SC_CHATS, SC_CHATS};
  int args_[kMaxDepth] = {0, 0, 0};
  int depth_ = 0;
  int tab_ = 1;
  bool splash_ = false;
  bool dirty_ = true;
  uint32_t now_ = 0;

  Banner banner_;
  Confirm confirm_;
  Screen *confirm_owner_ = nullptr;
  int confirm_action_ = 0;
};

} // namespace merlin
