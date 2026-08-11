// MERLIN UI — the 14 px tab ring at the bottom. Tappable, highlights active.
#pragma once

#include "../gfx.h"

namespace merlin {
namespace tabbar {

constexpr int16_t TAB_W = 80;
extern const char *const LABELS[4];

void render(Gfx &g, int active, bool chats_unread);
// Tab index under a touch point, or -1 if the point is not on the bar.
int hitTest(int16_t x, int16_t y);

} // namespace tabbar
} // namespace merlin
