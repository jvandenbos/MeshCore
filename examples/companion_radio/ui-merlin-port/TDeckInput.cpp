#include "TDeckInput.h"

// The panel rotation the display driver applies. Rotation 1 and 3 are both
// 320x240 landscape but 180 degrees apart, which flips what "up" means for the
// trackball and where a touch lands. Kept in one place so first light can
// correct both at once.
#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION 3
#endif
#if DISPLAY_ROTATION == 3
#define MERLIN_INPUT_FLIP 1
#else
#define MERLIN_INPUT_FLIP 0
#endif

// GT911 registers (16-bit, big-endian on the wire).
#define GT911_PRODUCT_ID 0x8140
#define GT911_POINT_INFO 0x814E
#define GT911_POINT_1 0x814F

#define KB_POLL_MS 15   // the C3 buffers exactly one char; a full matrix scan
                        // takes ~35 ms, so poll comfortably inside that
#define TOUCH_POLL_MS 20
#define LONG_PRESS_MS 600
#define SWIPE_MIN_PX 60
#define TAP_SLOP_PX 12

namespace merlin {

void TDeckInput::begin(TwoWire *wire) {
  wire_ = wire;

  pinMode(PIN_TB_UP, INPUT_PULLUP);
  pinMode(PIN_TB_DOWN, INPUT_PULLUP);
  pinMode(PIN_TB_LEFT, INPUT_PULLUP);
  pinMode(PIN_TB_RIGHT, INPUT_PULLUP);
  // PIN_USER_BTN is the trackball click and doubles as the ESP32-S3 boot
  // strapping pin, so it must be pulled up, not left floating.
  pinMode(PIN_USER_BTN, INPUT_PULLUP);

  tb_last_[0] = digitalRead(PIN_TB_UP);
  tb_last_[1] = digitalRead(PIN_TB_DOWN);
  tb_last_[2] = digitalRead(PIN_TB_LEFT);
  tb_last_[3] = digitalRead(PIN_TB_RIGHT);

  if (!wire_) return;

  wire_->beginTransmission(TDECK_KB_ADDR);
  kb_present_ = (wire_->endTransmission() == 0);

  pinMode(PIN_TOUCH_INT, INPUT);
  uint8_t id[4] = {0, 0, 0, 0};
  // The GT911's reset line is not wired to the MCU here, so the address cannot
  // be strapped and a soft reset would leave it unreachable. Probe instead.
  touch_present_ = touchRead(GT911_PRODUCT_ID, id, 4) && id[0] == '9';
}

void TDeckInput::poll(InputQueue &q) {
  pollTrackball(q); // every call: these are edges, and a missed one is lost
  unsigned long now = millis();
  if ((long)(now - next_kb_) >= 0) {
    next_kb_ = now + KB_POLL_MS;
    pollKeyboard(q);
  }
  if ((long)(now - next_touch_) >= 0) {
    next_touch_ = now + TOUCH_POLL_MS;
    pollTouch(q);
  }
}

// --- keyboard --------------------------------------------------------------

void TDeckInput::pollKeyboard(InputQueue &q) {
  if (!wire_) return;
  if (wire_->requestFrom((uint8_t)TDECK_KB_ADDR, (uint8_t)1) != 1) return;
  int v = wire_->read();
  if (v <= 0) return; // 0x00 is the C3's "no key", not a character

  char c = (char)v;
  if (c == '\r') c = '\n';        // the C3 sends CR for Enter; the UI wants LF
  if (c == 0x0C) return;          // Alt+C, no meaning in this UI
  if (c != '\n' && c != '\b' && (v < 0x20 || v > 0x7E)) return;
  q.push(InputEvent::key(c));
}

// --- trackball -------------------------------------------------------------

void TDeckInput::pollTrackball(InputQueue &q) {
  // Each direction is its own pulse line: it toggles once per increment of
  // motion that way. Levels carry no meaning, only changes do.
  static const uint8_t kPins[4] = {PIN_TB_UP, PIN_TB_DOWN, PIN_TB_LEFT,
                                   PIN_TB_RIGHT};
#if MERLIN_INPUT_FLIP
  // First light, final answer (both axes tested together 2026-08-09): under
  // rotation 3 the trackball needs NO remapping at all — the pulse-line pin
  // names read true on this unit. The rotation-flip theory was wrong on both
  // axes; keep the identity mapping.
  static const InputEvent::Nav kNav[4] = {InputEvent::U, InputEvent::D,
                                          InputEvent::L, InputEvent::R};
#else
  // First light 2026-08-09: vertical was correct but horizontal mirrored —
  // the trackball's L/R pulse lines are swapped relative to rotation 3.
  static const InputEvent::Nav kNav[4] = {InputEvent::U, InputEvent::D,
                                          InputEvent::R, InputEvent::L};
#endif

  // Detune (first-light feedback): each physical detent yields two toggles,
  // and unpaced steps outrun the thumb. Emit one step per MERLIN_TB_DIV
  // toggles, never faster than one step per MERLIN_TB_MIN_MS.
#ifndef MERLIN_TB_DIV
#define MERLIN_TB_DIV 2
#endif
#ifndef MERLIN_TB_MIN_MS
#define MERLIN_TB_MIN_MS 110 // 90 was still a touch jumpy on fast flicks
#endif
  unsigned long tnow = millis();
  if (tnow - tb_toggle_at_ > 250) { // stale half-pulses die between gestures
    for (int i = 0; i < 4; ++i) tb_accum_[i] = 0;
  }
  for (int i = 0; i < 4; ++i) {
    uint8_t v = digitalRead(kPins[i]);
    if (v != tb_last_[i]) {
      tb_last_[i] = v;
      tb_toggle_at_ = tnow;
      tb_accum_[i]++;
      if (tb_accum_[i] >= MERLIN_TB_DIV &&
          tnow - tb_step_at_ >= MERLIN_TB_MIN_MS) {
        for (int j = 0; j < 4; ++j) tb_accum_[j] = 0; // one axis wins cleanly
        tb_step_at_ = tnow;
        q.push(InputEvent::navi(kNav[i]));
      }
    }
  }

  bool down = digitalRead(PIN_USER_BTN) == LOW; // active low, pulled up
  unsigned long now = millis();
  if (down && !btn_last_) {
    btn_down_at_ = now;
  } else if (!down && btn_last_) {
    // Fire on release so a long hold can mean something else later; anything
    // shorter than 30 ms is contact bounce.
    if (now - btn_down_at_ >= 30) q.push(InputEvent::navi(InputEvent::CLICK));
  }
  btn_last_ = down;
}

// --- touch -----------------------------------------------------------------

bool TDeckInput::touchRead(uint16_t reg, uint8_t *buf, size_t len) {
  if (!wire_) return false;
  wire_->beginTransmission(TDECK_TOUCH_ADDR);
  wire_->write((uint8_t)(reg >> 8));
  wire_->write((uint8_t)(reg & 0xFF));
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom((uint8_t)TDECK_TOUCH_ADDR, (uint8_t)len) != (int)len)
    return false;
  for (size_t i = 0; i < len; ++i) buf[i] = (uint8_t)wire_->read();
  return true;
}

bool TDeckInput::touchWrite(uint16_t reg, uint8_t val) {
  if (!wire_) return false;
  wire_->beginTransmission(TDECK_TOUCH_ADDR);
  wire_->write((uint8_t)(reg >> 8));
  wire_->write((uint8_t)(reg & 0xFF));
  wire_->write(val);
  return wire_->endTransmission() == 0;
}

void TDeckInput::pollTouch(InputQueue &q) {
  if (!touch_present_) return;

  uint8_t status = 0;
  if (!touchRead(GT911_POINT_INFO, &status, 1)) return;

  int16_t sx = 0, sy = 0;
  bool have_point = false;
  if (status & 0x80) { // buffer ready
    int count = status & 0x0F;
    if (count > 0) {
      uint8_t p[8];
      if (touchRead(GT911_POINT_1, p, 8)) {
        uint16_t rx = (uint16_t)(p[1] | (p[2] << 8));
        uint16_t ry = (uint16_t)(p[3] | (p[4] << 8));
        // The panel is natively 240x320 portrait; the UI is 320x240 landscape.
        // Swap the axes, then mirror to match the display rotation.
#if MERLIN_INPUT_FLIP
        sx = (int16_t)(319 - (int)ry);
        sy = (int16_t)rx;
#else
        sx = (int16_t)ry;
        sy = (int16_t)(239 - (int)rx);
#endif
        if (sx < 0) sx = 0;
        if (sx > 319) sx = 319;
        if (sy < 0) sy = 0;
        if (sy > 239) sy = 239;
        have_point = true;
      }
    }
    // Mandatory: the controller will not refresh until the flag is cleared.
    touchWrite(GT911_POINT_INFO, 0);
  }

  unsigned long now = millis();
  if (have_point) {
    if (!down_) {
      down_ = true;
      long_fired_ = false;
      down_x_ = sx;
      down_y_ = sy;
      down_at_ = now;
    } else if (!long_fired_ && now - down_at_ >= LONG_PRESS_MS) {
      int dx = sx - down_x_, dy = sy - down_y_;
      if (dx * dx + dy * dy <= TAP_SLOP_PX * TAP_SLOP_PX) {
        long_fired_ = true;
        q.push(InputEvent::touch(InputEvent::LONG, down_x_, down_y_));
      }
    }
    last_x_ = sx;
    last_y_ = sy;
  } else if (down_) {
    down_ = false;
    if (long_fired_) return; // the long press was the whole gesture
    int dx = last_x_ - down_x_, dy = last_y_ - down_y_;
    if (dx <= -SWIPE_MIN_PX && abs(dy) < abs(dx)) {
      q.push(InputEvent::touch(InputEvent::SWIPE_L, last_x_, last_y_));
    } else if (dx >= SWIPE_MIN_PX && abs(dy) < abs(dx)) {
      q.push(InputEvent::touch(InputEvent::SWIPE_R, last_x_, last_y_));
    } else if (abs(dx) <= TAP_SLOP_PX && abs(dy) <= TAP_SLOP_PX) {
      q.push(InputEvent::touch(InputEvent::TAP, down_x_, down_y_));
    }
  }
}

} // namespace merlin
