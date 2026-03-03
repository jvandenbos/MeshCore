#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <helpers/CommonCLI.h>

// Forward declare — avoid circular include
class MyMesh;

enum UIScreen {
  SCREEN_STATUS = 0,
  SCREEN_NEIGHBORS,
  SCREEN_TRAFFIC,
  SCREEN_RADIO,
  SCREEN_COUNT  // must be last
};

class UITask {
  DisplayDriver* _display;
  MyMesh* _mesh;
  unsigned long _next_read, _next_refresh, _auto_off;
  int _prevBtnState;
  NodePrefs* _node_prefs;
  char _version_info[32];
  UIScreen _current_screen;
  int _scroll_offset;  // for neighbor list scrolling

  void renderCurrScreen();
  void renderBootScreen();
  void renderStatusScreen();
  void renderNeighborScreen();
  void renderTrafficScreen();
  void renderRadioScreen();

  // helper to format uptime as "Xh Ym"
  static void formatUptime(char* buf, uint32_t secs);

public:
  UITask(DisplayDriver& display) : _display(&display), _mesh(nullptr),
    _current_screen(SCREEN_STATUS), _scroll_offset(0) { _next_read = _next_refresh = 0; }
  void begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version);
  void setMesh(MyMesh* mesh) { _mesh = mesh; }

  void loop();
};
