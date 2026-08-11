// MERLIN sim — Gfx backed by an RGB565 framebuffer, dumped to PNG.
// Deliberately the same pixel format as the ST7789 canvas so a golden PNG is a
// prediction of the panel, not an approximation of it.
#pragma once

#include "../examples/companion_radio/ui-merlin/gfx.h"

#include <stdint.h>
#include <vector>

class SimGfx : public merlin::Gfx {
public:
  SimGfx(int16_t w, int16_t h);

  int16_t width() const override { return w_; }
  int16_t height() const override { return h_; }
  void fillRect(merlin::Rect r, merlin::Color c) override;
  void hline(int16_t x, int16_t y, int16_t w, merlin::Color c) override;
  void text(int16_t x, int16_t y, const char *s, merlin::FontId f,
            merlin::Color c) override;
  int16_t textWidth(const char *s, merlin::FontId f) override;
  void clip(merlin::Rect r) override;
  void unclip() override;

  void clear(merlin::Color c);
  bool writePng(const char *path) const;

  // Exposed for the span sink and for test assertions.
  void pixel(int16_t x, int16_t y, merlin::Color c);
  merlin::Color at(int16_t x, int16_t y) const;

private:
  int16_t w_, h_;
  std::vector<uint16_t> fb_;
  merlin::Rect clip_;
};
