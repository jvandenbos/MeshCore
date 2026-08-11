// MERLIN UI — the non-ASCII glyph vocabulary.
//
// The design spec's iconography (up-triangle repeater, hop arrow, SNR bars,
// ACK glyphs, battery) is not in any 7-bit font, and adding a custom font just
// to carry nine shapes costs flash and a second rasterizer. They are drawn from
// Gfx primitives instead — crisp at 1x, and the same code paints the panel.
#pragma once

#include "gfx.h"

namespace merlin {
namespace icons {

// Node kind markers. Both occupy a 9x9 cell so rows line up.
void repeater(Gfx &g, int16_t x, int16_t y, Color c); // filled up-triangle
void companion(Gfx &g, int16_t x, int16_t y, Color c); // hollow ring

// "hops away" arrow, 9x7 cell; caller prints the digit after it.
void hopArrow(Gfx &g, int16_t x, int16_t y, Color c);

void chevronLeft(Gfx &g, int16_t x, int16_t y, Color c);  // 5x9
void chevronRight(Gfx &g, int16_t x, int16_t y, Color c); // 5x9
void sortArrow(Gfx &g, int16_t x, int16_t y, bool up, Color c); // 7x5

// 3-bar SNR meter, 12x10 cell. level 0..3 bars lit (0 = all unlit).
void snrBars(Gfx &g, int16_t x, int16_t y, int level, Color on, Color off);

// 18x9 battery outline + fill; `pct` clamps to 0..100.
void battery(Gfx &g, int16_t x, int16_t y, uint8_t pct, Color outline, Color fill);

// ACK glyph vocabulary, all in a 9x9 cell at the row's right edge.
void ackQueued(Gfx &g, int16_t x, int16_t y, Color c);    // .
void ackSent(Gfx &g, int16_t x, int16_t y, Color c);      // ->
void ackDelivered(Gfx &g, int16_t x, int16_t y, Color c); // check
void ackRepeat(Gfx &g, int16_t x, int16_t y, Color c);    // loop (digit follows)

void envelope(Gfx &g, int16_t x, int16_t y, Color c); // 11x8
void chargeArrow(Gfx &g, int16_t x, int16_t y, Color c); // 7x7 up-triangle

// Unread/read markers used by the Chats list: 5x5 filled or hollow.
void dot(Gfx &g, int16_t x, int16_t y, bool filled, Color c);

} // namespace icons
} // namespace merlin
