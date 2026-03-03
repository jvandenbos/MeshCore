#pragma once

#include "BaseSerialInterface.h"

/**
 * Wraps two BaseSerialInterface instances (e.g. BLE + USB) and presents
 * them as a single interface to the mesh stack. Commands received on
 * either transport get processed; responses route back to whichever
 * interface last sent data. Unsolicited outbound frames (incoming
 * messages, adverts) go to whichever interface is connected, preferring
 * the primary (first) interface if both are connected.
 */
class DualSerialInterface : public BaseSerialInterface {
  BaseSerialInterface* _primary;    // e.g. BLE
  BaseSerialInterface* _secondary;  // e.g. USB
  BaseSerialInterface* _active;     // last interface that sent us data

public:
  DualSerialInterface(BaseSerialInterface& primary, BaseSerialInterface& secondary)
    : _primary(&primary), _secondary(&secondary), _active(&primary) {}

  void enable() override {
    _primary->enable();
    _secondary->enable();
  }

  void disable() override {
    _primary->disable();
    _secondary->disable();
  }

  bool isEnabled() const override {
    return _primary->isEnabled() || _secondary->isEnabled();
  }

  bool isConnected() const override {
    return _primary->isConnected() || _secondary->isConnected();
  }

  bool isWriteBusy() const override {
    // busy if the active (response target) interface is busy
    return _active->isWriteBusy();
  }

  size_t checkRecvFrame(uint8_t dest[]) override {
    // check primary first, then secondary
    size_t len = _primary->checkRecvFrame(dest);
    if (len > 0) {
      _active = _primary;
      return len;
    }
    len = _secondary->checkRecvFrame(dest);
    if (len > 0) {
      _active = _secondary;
      return len;
    }
    return 0;
  }

  size_t writeFrame(const uint8_t src[], size_t len) override {
    // Send to the active interface (whoever last sent us a command).
    // If active isn't connected, try the other one.
    if (_active->isConnected()) {
      return _active->writeFrame(src, len);
    }
    // fallback to whichever is connected
    if (_primary->isConnected()) {
      return _primary->writeFrame(src, len);
    }
    if (_secondary->isConnected()) {
      return _secondary->writeFrame(src, len);
    }
    return 0;
  }
};
