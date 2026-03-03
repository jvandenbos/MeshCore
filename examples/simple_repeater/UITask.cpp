#include "UITask.h"
#include "MyMesh.h"
#include <Arduino.h>
#include <helpers/CommonCLI.h>
#include <algorithm>

#define AUTO_OFF_MILLIS      20000  // 20 seconds
#define BOOT_SCREEN_MILLIS   4000   // 4 seconds
#define LINES_PER_PAGE       3      // neighbor entries per screen (128x64 OLED)

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe,
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe,
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc,
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00,
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00,
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8,
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8,
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0,
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00,
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00,
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8,
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8,
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8,
};

void UITask::formatUptime(char* buf, uint32_t secs) {
  if (secs < 3600) {
    sprintf(buf, "%um", secs / 60);
  } else if (secs < 86400) {
    sprintf(buf, "%uh%um", secs / 3600, (secs % 3600) / 60);
  } else {
    sprintf(buf, "%ud%uh", secs / 86400, (secs % 86400) / 3600);
  }
}

void UITask::begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version) {
  _prevBtnState = HIGH;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  _node_prefs = node_prefs;
  _display->turnOn();

  // strip off dash and commit hash: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  sprintf(_version_info, "%s (%s)", version, build_date);
  free(version);
}

void UITask::renderBootScreen() {
  // meshcore logo
  _display->setColor(DisplayDriver::BLUE);
  int logoWidth = 128;
  _display->drawXbm((_display->width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

  // version info
  _display->setColor(DisplayDriver::LIGHT);
  _display->setTextSize(1);
  uint16_t versionWidth = _display->getTextWidth(_version_info);
  _display->setCursor((_display->width() - versionWidth) / 2, 22);
  _display->print(_version_info);

  // node type
  const char* node_type = "< Repeater >";
  uint16_t typeWidth = _display->getTextWidth(node_type);
  _display->setCursor((_display->width() - typeWidth) / 2, 35);
  _display->print(node_type);
}

void UITask::renderStatusScreen() {
  char tmp[80];

  // Line 1: node name
  _display->setCursor(0, 0);
  _display->setTextSize(1);
  _display->setColor(DisplayDriver::GREEN);
  _display->print(_node_prefs->node_name);

  if (!_mesh) return;

  // Line 2: battery + uptime
  _display->setCursor(0, 16);
  _display->setColor(DisplayDriver::YELLOW);
  char uptime_buf[16];
  formatUptime(uptime_buf, _mesh->getUptimeSeconds());
  sprintf(tmp, "BAT:%umV UP:%s", _mesh->getBoard().getBattMilliVolts(), uptime_buf);
  _display->print(tmp);

  // Line 3: RX/TX/Queue
  _display->setCursor(0, 28);
  _display->setColor(DisplayDriver::LIGHT);
  sprintf(tmp, "RX:%-5u TX:%-5u Q:%u",
    radio_driver.getPacketsRecv(),
    radio_driver.getPacketsSent(),
    _mesh->getPacketManager()->getOutboundCount(0xFFFFFFFF));
  _display->print(tmp);

  // Line 4: noise floor + neighbor/sighting count
  _display->setCursor(0, 40);
  int nbrs = 0;
  int sights = 0;
#if MAX_NEIGHBOURS
  nbrs = _mesh->getNeighboursCount();
#endif
#if MAX_SIGHTINGS
  sights = _mesh->getSightingsCount();
#endif
  sprintf(tmp, "NF:%-4d NBR:%d NOD:%d",
    (int16_t)_mesh->getRadio()->getNoiseFloor(), nbrs, sights);
  _display->print(tmp);
}

void UITask::renderNeighborScreen() {
  char tmp[80];

  if (!_mesh) return;

#if MAX_SIGHTINGS
  // Build sorted list of active sightings (most recent first)
  const NodeSighting* all = _mesh->getSightings();
  const NodeSighting* sorted[MAX_SIGHTINGS];
  int count = 0;
  for (int i = 0; i < MAX_SIGHTINGS; i++) {
    if (all[i].last_seen > 0) {
      sorted[count++] = &all[i];
    }
  }
  std::sort(sorted, sorted + count, [](const NodeSighting* a, const NodeSighting* b) {
    return a->last_seen > b->last_seen;
  });

  // Header
  int pages = (count + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
  if (pages < 1) pages = 1;
  int page = (_scroll_offset / LINES_PER_PAGE) + 1;
  _display->setCursor(0, 0);
  _display->setTextSize(1);
  _display->setColor(DisplayDriver::GREEN);
  sprintf(tmp, "NODES (%d)          %d/%d", count, page, pages);
  _display->print(tmp);

  // Entries
  _display->setColor(DisplayDriver::LIGHT);
  for (int i = 0; i < LINES_PER_PAGE; i++) {
    int idx = _scroll_offset + i;
    if (idx >= count) break;
    const NodeSighting* s = sorted[idx];

    // Truncate name to fit with RSSI and SNR
    char name[13];
    strncpy(name, s->name, 12);
    name[12] = '\0';

    int y = 16 + (i * 16);
    _display->setCursor(0, y);
    sprintf(tmp, "%-12s %4d %+.1f",
      name,
      (int)s->last_rssi,
      ((float)s->last_snr) / 4.0f);
    _display->print(tmp);
  }
#else
  _display->setCursor(0, 0);
  _display->setColor(DisplayDriver::LIGHT);
  _display->print("Sightings disabled");
#endif
}

void UITask::renderTrafficScreen() {
  char tmp[80];

  _display->setCursor(0, 0);
  _display->setTextSize(1);
  _display->setColor(DisplayDriver::GREEN);
  _display->print("TRAFFIC");

  if (!_mesh) return;

  _display->setColor(DisplayDriver::LIGHT);

  // Line 2: flood stats
  _display->setCursor(0, 16);
  sprintf(tmp, "Flood: %utx %urx",
    _mesh->getNumSentFlood(), _mesh->getNumRecvFlood());
  _display->print(tmp);

  // Line 3: direct stats
  _display->setCursor(0, 28);
  sprintf(tmp, "Direct: %utx %urx",
    _mesh->getNumSentDirect(), _mesh->getNumRecvDirect());
  _display->print(tmp);

  // Line 4: totals + dupes
  _display->setCursor(0, 40);
  sprintf(tmp, "Tot: %u/%u Err:%u",
    radio_driver.getPacketsRecv(),
    radio_driver.getPacketsSent(),
    radio_driver.getPacketsRecvErrors());
  _display->print(tmp);
}

void UITask::renderRadioScreen() {
  char tmp[80];

  // Line 1: freq/sf/bw/cr
  _display->setCursor(0, 0);
  _display->setTextSize(1);
  _display->setColor(DisplayDriver::GREEN);
  sprintf(tmp, "%.3f SF%d BW%.0f CR%d",
    _node_prefs->freq, _node_prefs->sf, _node_prefs->bw, _node_prefs->cr);
  _display->print(tmp);

  if (!_mesh) return;

  _display->setColor(DisplayDriver::LIGHT);

  // Line 2: TX power + airtime
  _display->setCursor(0, 16);
  sprintf(tmp, "TX:%ddBm Air:%us",
    _node_prefs->tx_power_dbm,
    _mesh->getTotalAirTime() / 1000);
  _display->print(tmp);

  // Line 3: RSSI + SNR
  _display->setCursor(0, 28);
  _display->setColor(DisplayDriver::YELLOW);
  sprintf(tmp, "RSSI:%d  SNR:%.1f",
    (int)radio_driver.getLastRSSI(),
    radio_driver.getLastSNR());
  _display->print(tmp);

  // Line 4: noise floor + rx airtime
  _display->setCursor(0, 40);
  _display->setColor(DisplayDriver::LIGHT);
  sprintf(tmp, "NF:%d  RXair:%us",
    (int16_t)_mesh->getRadio()->getNoiseFloor(),
    _mesh->getReceiveAirTime() / 1000);
  _display->print(tmp);
}

void UITask::renderCurrScreen() {
  if (millis() < BOOT_SCREEN_MILLIS) {
    renderBootScreen();
    return;
  }

  switch (_current_screen) {
    case SCREEN_STATUS:    renderStatusScreen(); break;
    case SCREEN_NEIGHBORS: renderNeighborScreen(); break;
    case SCREEN_TRAFFIC:   renderTrafficScreen(); break;
    case SCREEN_RADIO:     renderRadioScreen(); break;
    default:               renderStatusScreen(); break;
  }
}

void UITask::loop() {
#ifdef PIN_USER_BTN
  if (millis() >= _next_read) {
    int btnState = digitalRead(PIN_USER_BTN);
    if (btnState != _prevBtnState) {
      if (btnState == LOW) {  // pressed?
        if (_display->isOn()) {
          // cycle to next screen
          if (_current_screen == SCREEN_NEIGHBORS) {
            // in neighbor view, scroll first, then move to next screen
#if MAX_SIGHTINGS
            int count = _mesh ? _mesh->getSightingsCount() : 0;
            if (_scroll_offset + LINES_PER_PAGE < count) {
              _scroll_offset += LINES_PER_PAGE;
            } else {
              _scroll_offset = 0;
              _current_screen = (UIScreen)((_current_screen + 1) % SCREEN_COUNT);
            }
#else
            _current_screen = (UIScreen)((_current_screen + 1) % SCREEN_COUNT);
#endif
          } else {
            _scroll_offset = 0;
            _current_screen = (UIScreen)((_current_screen + 1) % SCREEN_COUNT);
          }
        } else {
          _display->turnOn();
        }
        _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
      }
      _prevBtnState = btnState;
    }
    _next_read = millis() + 200;  // 5 reads per second
  }
#endif

  if (_display->isOn()) {
    if (millis() >= _next_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }
}
