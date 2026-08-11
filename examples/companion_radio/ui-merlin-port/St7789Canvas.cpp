#include "St7789Canvas.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "../ui-merlin/glyphs.h"

namespace merlin {

St7789Canvas::~St7789Canvas() {
  if (fb_) heap_caps_free(fb_);
}

bool St7789Canvas::begin(int16_t w, int16_t h) {
  if (fb_) return true;
  // MALLOC_CAP_SPIRAM, not ps_malloc(), so a board without working PSRAM fails
  // here instead of quietly eating 150 KB of the internal heap the mesh needs.
  fb_ = (uint16_t *)heap_caps_malloc((size_t)w * h * sizeof(uint16_t),
                                     MALLOC_CAP_SPIRAM);
  if (!fb_) return false;
  w_ = w;
  h_ = h;
  clip_ = Rect(0, 0, w, h);
  memset(fb_, 0, (size_t)w * h * sizeof(uint16_t));
  return true;
}

void St7789Canvas::pixel(int16_t x, int16_t y, Color c) {
  if (x < clip_.x || x >= clip_.right() || y < clip_.y || y >= clip_.bottom())
    return;
  if (x < 0 || x >= w_ || y < 0 || y >= h_) return;
  fb_[(size_t)y * w_ + x] = c;
}

void St7789Canvas::fillRect(Rect r, Color c) {
  // Clip once, then write spans directly: the per-pixel path costs ~5x on a
  // full-screen clear, which happens on every frame.
  int16_t x0 = r.x > clip_.x ? r.x : clip_.x;
  int16_t y0 = r.y > clip_.y ? r.y : clip_.y;
  int16_t x1 = r.right() < clip_.right() ? r.right() : clip_.right();
  int16_t y1 = r.bottom() < clip_.bottom() ? r.bottom() : clip_.bottom();
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > w_) x1 = w_;
  if (y1 > h_) y1 = h_;
  for (int16_t y = y0; y < y1; ++y) {
    uint16_t *row = fb_ + (size_t)y * w_;
    for (int16_t x = x0; x < x1; ++x) row[x] = c;
  }
}

void St7789Canvas::hline(int16_t x, int16_t y, int16_t w, Color c) {
  fillRect(Rect(x, y, w, 1), c);
}

namespace {
struct Sink : glyphs::SpanSink {
  St7789Canvas &g;
  Color c;
  Sink(St7789Canvas &gg, Color cc) : g(gg), c(cc) {}
  void span(int16_t x, int16_t y, int16_t w) override {
    for (int16_t i = 0; i < w; ++i) g.pixel((int16_t)(x + i), y, c);
  }
};
} // namespace

void St7789Canvas::text(int16_t x, int16_t y, const char *s, FontId f,
                        Color c) {
  Sink sink(*this, c);
  glyphs::draw(x, y, s, f, sink);
}

int16_t St7789Canvas::textWidth(const char *s, FontId f) {
  return glyphs::width(s, f);
}

void St7789Canvas::clip(Rect r) {
  // Intersect, never widen — same rule as the simulator's SimGfx::clip.
  int16_t x0 = r.x > clip_.x ? r.x : clip_.x;
  int16_t y0 = r.y > clip_.y ? r.y : clip_.y;
  int16_t x1 = r.right() < clip_.right() ? r.right() : clip_.right();
  int16_t y1 = r.bottom() < clip_.bottom() ? r.bottom() : clip_.bottom();
  clip_ = Rect(x0, y0, (int16_t)(x1 > x0 ? x1 - x0 : 0),
               (int16_t)(y1 > y0 ? y1 - y0 : 0));
}

void St7789Canvas::unclip() { clip_ = Rect(0, 0, w_, h_); }

void St7789Canvas::endFrame() {
  if (!fb_ || !panel_) return;
  panel_->drawRGBBitmap(0, 0, fb_, w_, h_);
}

} // namespace merlin
