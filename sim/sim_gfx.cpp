#include "sim_gfx.h"

#include "../examples/companion_radio/ui-merlin/glyphs.h"

#include "stb_image_write.h"

using namespace merlin;

SimGfx::SimGfx(int16_t w, int16_t h)
    : w_(w), h_(h), fb_((size_t)w * h, 0), clip_(0, 0, w, h) {}

void SimGfx::pixel(int16_t x, int16_t y, Color c) {
  if (x < clip_.x || x >= clip_.right() || y < clip_.y || y >= clip_.bottom())
    return;
  if (x < 0 || x >= w_ || y < 0 || y >= h_) return;
  fb_[(size_t)y * w_ + x] = c;
}

Color SimGfx::at(int16_t x, int16_t y) const {
  if (x < 0 || x >= w_ || y < 0 || y >= h_) return 0;
  return fb_[(size_t)y * w_ + x];
}

void SimGfx::fillRect(Rect r, Color c) {
  for (int16_t y = r.y; y < r.bottom(); ++y)
    for (int16_t x = r.x; x < r.right(); ++x) pixel(x, y, c);
}

void SimGfx::hline(int16_t x, int16_t y, int16_t w, Color c) {
  for (int16_t i = 0; i < w; ++i) pixel((int16_t)(x + i), y, c);
}

namespace {
struct Sink : merlin::glyphs::SpanSink {
  SimGfx &g;
  Color c;
  Sink(SimGfx &gg, Color cc) : g(gg), c(cc) {}
  void span(int16_t x, int16_t y, int16_t w) override {
    for (int16_t i = 0; i < w; ++i) g.pixel((int16_t)(x + i), y, c);
  }
};
} // namespace

void SimGfx::text(int16_t x, int16_t y, const char *s, FontId f, Color c) {
  Sink sink(*this, c);
  glyphs::draw(x, y, s, f, sink);
}

int16_t SimGfx::textWidth(const char *s, FontId f) {
  return glyphs::width(s, f);
}

void SimGfx::clip(Rect r) {
  // Intersect, never widen: a nested clip can only ever tighten.
  int16_t x0 = r.x > clip_.x ? r.x : clip_.x;
  int16_t y0 = r.y > clip_.y ? r.y : clip_.y;
  int16_t x1 = r.right() < clip_.right() ? r.right() : clip_.right();
  int16_t y1 = r.bottom() < clip_.bottom() ? r.bottom() : clip_.bottom();
  clip_ = Rect(x0, y0, (int16_t)(x1 > x0 ? x1 - x0 : 0),
               (int16_t)(y1 > y0 ? y1 - y0 : 0));
}

void SimGfx::unclip() { clip_ = Rect(0, 0, w_, h_); }

void SimGfx::clear(Color c) {
  for (auto &p : fb_) p = c;
}

bool SimGfx::writePng(const char *path) const {
  std::vector<uint8_t> rgb((size_t)w_ * h_ * 3);
  for (size_t i = 0; i < fb_.size(); ++i) {
    uint16_t v = fb_[i];
    uint8_t r = (uint8_t)((v >> 11) & 0x1F), g = (uint8_t)((v >> 5) & 0x3F),
            b = (uint8_t)(v & 0x1F);
    // Bit replication, the same expansion the panel does.
    rgb[i * 3 + 0] = (uint8_t)((r << 3) | (r >> 2));
    rgb[i * 3 + 1] = (uint8_t)((g << 2) | (g >> 4));
    rgb[i * 3 + 2] = (uint8_t)((b << 3) | (b >> 2));
  }
  return stbi_write_png(path, w_, h_, 3, rgb.data(), w_ * 3) != 0;
}
