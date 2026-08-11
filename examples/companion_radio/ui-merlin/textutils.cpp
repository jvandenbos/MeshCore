#include "textutils.h"

#include <math.h>
#include <stdio.h>

namespace merlin {
namespace text {

size_t collapseUtf8(const char *in, char *out, size_t out_cap) {
  if (!out || out_cap == 0) return 0;
  size_t n = 0;
  if (!in) { out[0] = 0; return 0; }

  for (const unsigned char *p = (const unsigned char *)in; *p && n + 1 < out_cap;) {
    unsigned char c = *p;
    if (c >= 0x20 && c < 0x7F) {
      out[n++] = (char)c;
      ++p;
      continue;
    }
    if (c < 0x20) { // control character: drop it, never render
      ++p;
      continue;
    }
    // 0x80..0xFF — a multi-byte sequence, a stray continuation byte, or plain
    // garbage. An unbroken RUN of them collapses to a SINGLE block: a ZWJ
    // emoji is one thing the font cannot show, not three.
    out[n++] = BLOCK;
    while (*p >= 0x80) ++p;
  }
  out[n] = 0;
  return n;
}

void formatAge(uint32_t secs, char *out, size_t cap) {
  if (secs < 60)
    snprintf(out, cap, "now");
  else if (secs < 3600)
    snprintf(out, cap, "%um", (unsigned)(secs / 60));
  else if (secs < 86400)
    snprintf(out, cap, "%uh", (unsigned)(secs / 3600));
  else
    snprintf(out, cap, "%ud", (unsigned)(secs / 86400));
}

void formatAgeLong(uint32_t secs, char *out, size_t cap) {
  if (secs < 60)
    snprintf(out, cap, "%us ago", (unsigned)secs);
  else if (secs < 3600)
    snprintf(out, cap, "%um ago", (unsigned)(secs / 60));
  else if (secs < 86400)
    snprintf(out, cap, "%uh %um ago", (unsigned)(secs / 3600),
             (unsigned)((secs % 3600) / 60));
  else
    snprintf(out, cap, "%ud %uh ago", (unsigned)(secs / 86400),
             (unsigned)((secs % 86400) / 3600));
}

void formatClock(uint32_t secs, char *out, size_t cap) {
  snprintf(out, cap, "%02u:%02u", (unsigned)((secs / 3600) % 24),
           (unsigned)((secs / 60) % 60));
}

void formatSnr(int16_t snr_x4, char *out, size_t cap) {
  int whole = snr_x4 / 4;
  int frac = (snr_x4 < 0 ? -snr_x4 : snr_x4) % 4;
  snprintf(out, cap, "%s%d.%d dB", snr_x4 >= 0 ? "+" : "-",
           whole < 0 ? -whole : whole, frac * 25 / 10);
}

void formatDistance(float km, char *out, size_t cap) {
  if (km < 1.0f)
    snprintf(out, cap, "%d m", (int)(km * 1000.0f + 0.5f));
  else if (km < 100.0f)
    snprintf(out, cap, "%.2f km", (double)km);
  else
    snprintf(out, cap, "%.0f km", (double)km);
}

void formatBearing(float deg, char *out, size_t cap) {
  static const char *kPoints[16] = {"N",  "NNE", "NE", "ENE", "E",  "ESE",
                                    "SE", "SSE", "S",  "SSW", "SW", "WSW",
                                    "W",  "WNW", "NW", "NNW"};
  while (deg < 0) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  int idx = (int)((deg + 11.25f) / 22.5f) & 15;
  snprintf(out, cap, "%d` %s", (int)(deg + 0.5f), kPoints[idx]);
}

static constexpr float kDeg2Rad = 3.14159265358979f / 180.0f;

float distanceKm(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371.0f;
  float p1 = lat1 * kDeg2Rad, p2 = lat2 * kDeg2Rad;
  float dp = (lat2 - lat1) * kDeg2Rad, dl = (lon2 - lon1) * kDeg2Rad;
  float a = sinf(dp / 2) * sinf(dp / 2) +
            cosf(p1) * cosf(p2) * sinf(dl / 2) * sinf(dl / 2);
  return R * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

float bearingDeg(float lat1, float lon1, float lat2, float lon2) {
  float p1 = lat1 * kDeg2Rad, p2 = lat2 * kDeg2Rad;
  float dl = (lon2 - lon1) * kDeg2Rad;
  float y = sinf(dl) * cosf(p2);
  float x = cosf(p1) * sinf(p2) - sinf(p1) * cosf(p2) * cosf(dl);
  float b = atan2f(y, x) / kDeg2Rad;
  return b < 0 ? b + 360.0f : b;
}

static char fold(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }

int findFold(const char *haystack, const char *needle) {
  if (!haystack || !needle || !*needle) return 0;
  for (int i = 0; haystack[i]; ++i) {
    int j = 0;
    while (needle[j] && fold(haystack[i + j]) == fold(needle[j])) ++j;
    if (!needle[j]) return i;
  }
  return -1;
}

} // namespace text
} // namespace merlin
