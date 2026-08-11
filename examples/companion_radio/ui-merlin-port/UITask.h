// MERLIN device port — the UITask main.cpp expects.
//
// This deliberately keeps the class name, constructor and begin() signature of
// the other UIs so main.cpp needs no #ifdef: the include path decides which
// UITask.h is compiled, and the env swaps ui-new for ui-merlin + ui-merlin-port.
#pragma once

#include <Arduino.h>
#include <helpers/BaseSerialInterface.h>
#include <helpers/SensorManager.h>
#include <helpers/ui/DisplayDriver.h>

#include "../AbstractUITask.h"
#include "../NodePrefs.h"
#include "../ui-merlin/router.h"
#include "../ui-merlin/screens/channels.h"
#include "../ui-merlin/screens/chats.h"
#include "../ui-merlin/screens/home.h"
#include "../ui-merlin/screens/newchat.h"
#include "../ui-merlin/screens/node_card.h"
#include "../ui-merlin/screens/nodes.h"
#include "../ui-merlin/screens/settings.h"
#include "../ui-merlin/screens/splash.h"
#include "../ui-merlin/screens/thread.h"
#include "../ui-merlin/store.h"
#include "MeshModel.h"
#include "SpiffsStore.h"
#include "St7789Canvas.h"
#include "TDeckInput.h"

class UITask : public AbstractUITask {
public:
  UITask(mesh::MainBoard *board, BaseSerialInterface *serial)
      : AbstractUITask(board, serial) {}

  void begin(DisplayDriver *display, SensorManager *sensors,
             NodePrefs *node_prefs);

  // from AbstractUITask
  void msgRead(int msgcount) override;
  void newMsg(uint8_t path_len, const char *from_name, const char *text,
              int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void ackRecv(uint32_t ack) override;
  void loop() override;

  bool hasDisplay() const { return canvas_ != nullptr; }

private:
  void render();

  DisplayDriver *display_ = nullptr;
  NodePrefs *prefs_ = nullptr;

  merlin::St7789Canvas *canvas_ = nullptr;
  merlin::MsgStore *store_ = nullptr; // PSRAM: ~230 KB
  merlin::SpiffsStore backend_;
  merlin::MeshModel model_;
  merlin::Router *router_ = nullptr;
  merlin::TDeckInput input_;
  merlin::InputQueue queue_;

  merlin::screens::SplashScreen splash_;
  merlin::screens::HomeScreen home_;
  merlin::screens::ChatsScreen chats_;
  merlin::screens::ThreadScreen thread_;
  merlin::screens::NodesScreen nodes_;
  merlin::screens::NodeCardScreen card_;
  merlin::screens::SettingsScreen settings_;
  merlin::screens::RadioScreen radio_;
  merlin::screens::IdentityScreen identity_;
  merlin::screens::NewChatScreen newchat_;
  merlin::screens::ChannelsScreen channels_;
  merlin::screens::ChannelScreen channel_;

  unsigned long boot_ms_ = 0;
  uint32_t last_tick_secs_ = 0;
  bool on_splash_ = false;
  bool ready_ = false;
};
