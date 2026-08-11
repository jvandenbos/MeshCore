// MERLIN device port — Gfx over an off-screen RGB565 canvas in PSRAM.
//
// The UI draws into the canvas; endFrame() pushes it to the ST7789 in one
// bus transaction. Drawing straight to the panel would show every fill and
// glyph as it lands, and the panel cannot be read back, so a full-frame
// buffer is what makes the render loop's "paint over the top" style legible.
//
// 320 x 240 x 2 bytes = 150 KB. That does not fit in internal DRAM alongside
// the mesh's packet pool, so the canvas lives in PSRAM (the T-Deck has 8 MB).
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "../ui-merlin/gfx.h"

namespace merlin {

class St7789Canvas : public Gfx {
public:
  // `panel` is the already-initialised display the companion build owns; this
  // class never re-inits the bus, it only blits.
  explicit St7789Canvas(Adafruit_ST7789 *panel) : panel_(panel) {}
  ~St7789Canvas() override;

  // Allocates the framebuffer in PSRAM. Returns false if PSRAM is missing or
  // exhausted, in which case the caller must not render.
  bool begin(int16_t w, int16_t h);

  int16_t width() const override { return w_; }
  int16_t height() const override { return h_; }

  void fillRect(Rect r, Color c) override;
  void hline(int16_t x, int16_t y, int16_t w, Color c) override;
  void text(int16_t x, int16_t y, const char *s, FontId f, Color c) override;
  int16_t textWidth(const char *s, FontId f) override;
  void clip(Rect r) override;
  void unclip() override;

  // Paint one pixel through the current clip. Public because the glyph span
  // sink writes through it.
  void pixel(int16_t x, int16_t y, Color c);

  // Push the whole canvas to the panel.
  void endFrame();

private:
  Adafruit_ST7789 *panel_;
  uint16_t *fb_ = nullptr;
  int16_t w_ = 0, h_ = 0;
  Rect clip_;
};

} // namespace merlin
