// MERLIN UI — the ONE glyph rasterizer. sim_gfx and the Phase-3 device canvas
// both call this, so a PNG golden is a true prediction of the panel.
//
// It emits horizontal spans; the Gfx implementation decides how to paint and
// clip them. Nothing here allocates or touches the platform.
#pragma once

#include "gfx.h"

namespace merlin {
namespace glyphs {

struct SpanSink {
  virtual ~SpanSink() {}
  virtual void span(int16_t x, int16_t y, int16_t w) = 0;
};

// Distance from the top of a text cell to the baseline.
int16_t ascent(FontId f);
// Full cell height (ascent + descender room) — the pitch a caller should use
// when stacking lines tightly.
int16_t cellHeight(FontId f);

// Width in pixels of `s` after the UTF-8 collapse.
int16_t width(const char *s, FontId f);

// Draw `s` with its cell top-left at (x, y). Applies the UTF-8 collapse.
void draw(int16_t x, int16_t y, const char *s, FontId f, SpanSink &sink);

} // namespace glyphs
} // namespace merlin
