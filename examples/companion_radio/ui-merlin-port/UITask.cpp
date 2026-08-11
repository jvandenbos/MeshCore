#include "UITask.h"

#include <esp_heap_caps.h>
#include <helpers/ui/ST7789LCDDisplay.h>
#include <new>
#include <target.h>

#include "../ui-merlin/theme.h"

#define SPLASH_MS 1500
#define FRAME_INTERVAL_MS 100 // 10 Hz ceiling, per the architecture doc

using namespace merlin;

void UITask::begin(DisplayDriver *display, SensorManager *sensors,
                   NodePrefs *node_prefs) {
  (void)sensors; // reached through the global the model already uses
  display_ = display;
  prefs_ = node_prefs;
  boot_ms_ = millis();

  if (!display_) return; // no panel: stay dark rather than render into nothing

  canvas_ = new St7789Canvas(((ST7789LCDDisplay *)display_)->getPanel());
  if (!canvas_->begin(theme::SCREEN_W, theme::SCREEN_H)) {
    delete canvas_;
    canvas_ = nullptr;
    MESH_DEBUG_PRINTLN("MERLIN: no PSRAM for the framebuffer, UI disabled");
    return;
  }

  // ~230 KB. This cannot live in .bss: the internal heap is needed for the
  // packet pool and the radio driver.
  void *mem = heap_caps_malloc(sizeof(MsgStore), MALLOC_CAP_SPIRAM);
  if (!mem) {
    MESH_DEBUG_PRINTLN("MERLIN: no PSRAM for the message store, UI disabled");
    return;
  }
  store_ = new (mem) MsgStore();
  store_->attach(&backend_);
  store_->load(); // a blob from a different build is refused, not half-read

  if (!model_.begin(store_, &backend_, prefs_)) {
    MESH_DEBUG_PRINTLN("MERLIN: model init failed, UI disabled");
    return;
  }

  input_.begin(&Wire);

  router_ = new Router(model_, &model_);
  router_->registerScreen(SC_SPLASH, &splash_);
  router_->registerScreen(SC_HOME, &home_);
  router_->registerScreen(SC_CHATS, &chats_);
  router_->registerScreen(SC_THREAD, &thread_);
  router_->registerScreen(SC_NODES, &nodes_);
  router_->registerScreen(SC_NODE_CARD, &card_);
  router_->registerScreen(SC_SETTINGS, &settings_);
  router_->registerScreen(SC_RADIO, &radio_);
  router_->registerScreen(SC_IDENTITY, &identity_);
  router_->registerScreen(SC_NEWCHAT, &newchat_);
  router_->registerScreen(SC_CHANNELS, &channels_);
  router_->registerScreen(SC_CHANNEL, &channel_);

  last_tick_secs_ = rtc_clock.getCurrentTime();
  router_->tick(last_tick_secs_ % 86400);
  router_->start(SC_SPLASH);
  on_splash_ = true;
  ready_ = true;
  render();
}

void UITask::render() {
  if (!ready_ || !canvas_ || !router_) return;
  router_->render(*canvas_);
  canvas_->endFrame();
}

void UITask::msgRead(int msgcount) { (void)msgcount; }

void UITask::newMsg(uint8_t path_len, const char *from_name, const char *text,
                    int msgcount) {
  (void)msgcount;
  if (!ready_) return;
  int convo = model_.onIncoming(path_len, from_name, text);
  // Rule 5: traffic announces itself without stealing the screen. The banner
  // carries the conversation it came from, because tapping it opens that
  // thread.
  if (router_ && convo >= 0) {
    // A muted conversation still counts unread and re-sorts the list — it
    // just stops interrupting. This is what Chats' 'm' verb actually promises.
    if (!model_.convo(convo).muted) router_->showBanner(from_name, text, convo);
    router_->markDirty();
  }
}

void UITask::notify(UIEventType t) {
  (void)t;
  if (router_) router_->markDirty();
}

void UITask::ackRecv(uint32_t ack) {
  if (!ready_) return;
  model_.onAck(ack);
  if (router_) router_->markDirty();
}

void UITask::loop() {
  if (!ready_) return;

  unsigned long now_ms = millis();

  if (on_splash_ && now_ms - boot_ms_ >= SPLASH_MS) {
    on_splash_ = false;
    router_->start(SC_CHATS);
  }

  input_.poll(queue_);
  InputEvent e;
  bool had_input = false;
  while (queue_.pop(e)) {
    if (on_splash_) {
      on_splash_ = false; // any key skips the splash
    }
    router_->handle(e);
    had_input = true;
  }

  // One tick per wall-clock second: the status bar clock and blips repaint on
  // it, and Router::tick marks the frame dirty, so calling it every pass would
  // pin the render loop at full rate for nothing.
  uint32_t now_secs = rtc_clock.getCurrentTime();
  if (now_secs != last_tick_secs_) {
    last_tick_secs_ = now_secs;
    router_->tick(now_secs % 86400);
    model_.refresh(); // contact snapshot, on its own slower timer inside
  }

  static unsigned long next_frame = 0;
  if (router_->dirty() && (long)(now_ms - next_frame) >= 0) {
    next_frame = now_ms + FRAME_INTERVAL_MS;
    render();
  }

  // Idle-hook work only: a store write is ~230 KB to SPIFFS and must never
  // land in an event or render path.
  if (!had_input) {
    if (model_.storeDirty()) {
      backend_.markDirty();
      model_.clearStoreDirty();
    }
    backend_.flushIfDue(*store_, now_ms);
  }
}
