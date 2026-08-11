// MERLIN UI — bounded text hygiene. Rule 6: never crash on content.
// Every string that reaches the glyph rasterizer passes through
// collapseUtf8() exactly once, here, so no screen can forget to do it.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace merlin {
namespace text {

// Sentinel that the rasterizer paints as the block glyph (design spec's U+2592
// "unknown glyph" collapse). 0x7F is outside every vendored font's range, so it
// can never collide with a real glyph.
constexpr char BLOCK = 0x7F;

// The degree sign, which no 7-bit font carries. The backtick is never printed
// literally anywhere in this UI, so it is reused as the sentinel and the
// rasterizer draws a ring for it.
constexpr char DEGREE = '`';

// Longest string the UI will ever rasterize in one call. Anything longer is
// truncated rather than allowed to run the render loop off a cliff.
constexpr size_t MAX_RENDER = 128;

// Collapse UTF-8 to renderable ASCII. Printable ASCII passes through; every
// multi-byte sequence (valid or not) becomes exactly one BLOCK; control
// characters are dropped. Always NUL-terminates, never writes past out_cap,
// returns the number of characters written.
size_t collapseUtf8(const char *in, char *out, size_t out_cap);

// --- small formatters used by several screens (all bounded, no allocation) ---

// "now" / "5m" / "4h" / "3d"
void formatAge(uint32_t secs, char *out, size_t cap);
// "4h 12m ago"
void formatAgeLong(uint32_t secs, char *out, size_t cap);
// seconds-since-midnight -> "21:42"
void formatClock(uint32_t secs, char *out, size_t cap);
// signed dB with one decimal: "+12.5 dB"
void formatSnr(int16_t snr_x4, char *out, size_t cap);
// "26 m" below a kilometre, else "2.48 km"
void formatDistance(float km, char *out, size_t cap);
// "158 deg SSE" -> written as "158` SSE" (degree sign is not ASCII)
void formatBearing(float deg, char *out, size_t cap);

// Great-circle distance (km) and true bearing (deg) between two positions.
float distanceKm(float lat1, float lon1, float lat2, float lon2);
float bearingDeg(float lat1, float lon1, float lat2, float lon2);

// Case-insensitive substring search; returns offset or -1.
int findFold(const char *haystack, const char *needle);

} // namespace text
} // namespace merlin
