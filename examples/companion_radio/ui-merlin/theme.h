// MERLIN UI — the design's palette + layout constants, all in one place.
#pragma once

#include "gfx.h"

namespace merlin {
namespace theme {

constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (Color)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Visual language (design spec §Visual language)
constexpr Color BG = rgb(0x00, 0x00, 0x00);     // #000
constexpr Color FG = rgb(0xE6, 0xE6, 0xE6);     // #E6E6E6
constexpr Color MUTED = rgb(0x8A, 0x8A, 0x8A);  // #8A8A8A
constexpr Color ACCENT = rgb(0x35, 0xC8, 0xDC); // #35C8DC
constexpr Color OK = rgb(0x3F, 0xD0, 0x7A);
constexpr Color WARN = rgb(0xE8, 0xB3, 0x3A);
constexpr Color ALERT = rgb(0xE5, 0x48, 0x4D);

// Derived tones used by the mockups
constexpr Color SEL_FG = BG;                       // text on the selection bar
constexpr Color SEL_MUTED = rgb(0x08, 0x38, 0x40); // muted text on selection
constexpr Color HEADER_BG = rgb(0x0C, 0x0C, 0x0C); // sort/title strips
constexpr Color FILTER_BG = rgb(0x06, 0x28, 0x2D); // filter strip + banner
constexpr Color RULE = rgb(0x24, 0x24, 0x24);      // hairline between rows
constexpr Color HAIRLINE = rgb(0x1E, 0x1E, 0x1E);  // bar borders
constexpr Color DIM = rgb(0x5A, 0x5A, 0x5A);       // read/idle glyphs
constexpr Color BAR_OFF = rgb(0x1C, 0x1C, 0x1C);   // unlit SNR bar
constexpr Color SCROLL_TRACK = rgb(0x33, 0x33, 0x33);

// Per-sender deterministic colours (hash -> index). Design spec: name line
// only, body text stays FG.
constexpr Color SENDER[6] = {
    rgb(0xE0, 0xA2, 0x5C), rgb(0x9C, 0x86, 0xE0), rgb(0x5F, 0xCB, 0x8C),
    rgb(0xE0, 0x6C, 0x75), rgb(0x6C, 0xB6, 0xE0), rgb(0xD0, 0xC0, 0x60)};

// Layout: 240 px = 14 status + 212 content + 14 tab bar
constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 240;
constexpr int16_t STATUS_H = 14;
constexpr int16_t TAB_H = 14;
constexpr int16_t CONTENT_Y = STATUS_H;
constexpr int16_t CONTENT_H = SCREEN_H - STATUS_H - TAB_H; // 212
constexpr int16_t STRIP_H = 14;                            // title/sort strips
constexpr int16_t SCROLLBAR_X = 317;
constexpr int16_t SCROLLBAR_W = 3;

inline Color senderColor(const char *name) {
  uint32_t h = 2166136261u;
  for (const char *p = name; p && *p; ++p) {
    h ^= (uint8_t)*p;
    h *= 16777619u;
  }
  return SENDER[h % 6];
}

} // namespace theme
} // namespace merlin
