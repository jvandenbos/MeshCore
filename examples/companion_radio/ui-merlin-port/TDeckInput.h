// MERLIN device port — the three T-Deck input sources, normalised into the
// one InputEvent grammar the UI understands.
//
// Hardware facts (LilyGo T-Deck schematic + vendor examples; no code taken
// from any GPL project):
//   keyboard  an on-board ESP32-C3 acting as an I2C slave at 0x55. A 1-byte
//             read returns the ASCII of the key that was pressed, or 0x00 when
//             nothing was. Enter is CR (0x0D), backspace 0x08. Shift and the
//             symbol layer are resolved on the C3, so the master only ever
//             sees a finished character.
//   trackball four independent pulse lines, one per direction, plus the click
//             on PIN_USER_BTN. They are not quadrature: a line simply toggles
//             once per motion increment, so the driver counts edges and the
//             level between edges means nothing.
//   touch     GT911 at 0x5D. Its reset line is not wired to the ESP32-S3 on
//             this board, so the address cannot be strapped and a soft reset
//             must be avoided; probe instead.
#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "../ui-merlin/input.h"

namespace merlin {

// Trackball direction pins. Named after the vendor's net labels so they can be
// checked against the schematic.
#ifndef PIN_TB_UP
#define PIN_TB_UP 3 // G_S1
#endif
#ifndef PIN_TB_RIGHT
#define PIN_TB_RIGHT 2 // G_S2
#endif
#ifndef PIN_TB_DOWN
#define PIN_TB_DOWN 15 // G_S3
#endif
#ifndef PIN_TB_LEFT
#define PIN_TB_LEFT 1 // G_S4
#endif

#ifndef TDECK_KB_ADDR
#define TDECK_KB_ADDR 0x55
#endif
#ifndef TDECK_TOUCH_ADDR
#define TDECK_TOUCH_ADDR 0x5D
#endif
#ifndef PIN_TOUCH_INT
#define PIN_TOUCH_INT 16
#endif

// A small ring so a burst of trackball pulses between UI frames is not lost.
class InputQueue {
public:
  bool push(const InputEvent &e) {
    int nxt = (head_ + 1) % CAP;
    if (nxt == tail_) return false; // full: drop the newest, keep order
    buf_[head_] = e;
    head_ = nxt;
    return true;
  }
  bool pop(InputEvent &out) {
    if (tail_ == head_) return false;
    out = buf_[tail_];
    tail_ = (tail_ + 1) % CAP;
    return true;
  }

private:
  static constexpr int CAP = 24;
  InputEvent buf_[CAP];
  int head_ = 0, tail_ = 0;
};

class TDeckInput {
public:
  void begin(TwoWire *wire);
  // Polls all three sources and queues whatever they produced.
  void poll(InputQueue &q);

  bool hasKeyboard() const { return kb_present_; }
  bool hasTouch() const { return touch_present_; }

private:
  void pollKeyboard(InputQueue &q);
  void pollTrackball(InputQueue &q);
  void pollTouch(InputQueue &q);

  bool touchRead(uint16_t reg, uint8_t *buf, size_t len);
  bool touchWrite(uint16_t reg, uint8_t val);

  TwoWire *wire_ = nullptr;
  bool kb_present_ = false;
  bool touch_present_ = false;

  unsigned long next_kb_ = 0;
  unsigned long next_touch_ = 0;

  // Trackball edge state, one per direction pin.
  uint8_t tb_last_[4] = {0, 0, 0, 0};
  uint8_t tb_accum_[4] = {0, 0, 0, 0};
  unsigned long tb_step_at_ = 0;
  unsigned long tb_toggle_at_ = 0;
  bool btn_last_ = false;
  unsigned long btn_down_at_ = 0;

  // Touch gesture state.
  bool down_ = false;
  int16_t down_x_ = 0, down_y_ = 0;
  int16_t last_x_ = 0, last_y_ = 0;
  unsigned long down_at_ = 0;
  bool long_fired_ = false;
};

} // namespace merlin
