#include "channel.h"

#include "sha256.h"

#include <string.h>

namespace merlin {
namespace channel {

static const char kHex[] = "0123456789abcdef";
static const char kPrefix[] = "meshcore://join/";
static constexpr size_t kPrefixLen = sizeof(kPrefix) - 1;

void toHex(const uint8_t *key, int n, char *out, size_t cap) {
  size_t w = 0;
  for (int i = 0; i < n && w + 2 < cap; ++i) {
    out[w++] = kHex[key[i] >> 4];
    out[w++] = kHex[key[i] & 0x0F];
  }
  if (cap) out[w < cap ? w : cap - 1] = 0;
}

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

int fromHex(const char *hex, uint8_t *key, int max_bytes) {
  int n = 0;
  int hi = -1;
  for (const char *p = hex; p && *p; ++p) {
    if (*p == ' ' || *p == '-' || *p == ':') continue;
    int v = hexVal(*p);
    if (v < 0) return -1;
    if (hi < 0) {
      hi = v;
    } else {
      if (n >= max_bytes) return -1;
      key[n++] = (uint8_t)((hi << 4) | v);
      hi = -1;
    }
  }
  return hi < 0 ? n : -1; // a dangling nibble is not a whole byte
}

uint8_t checksum(const uint8_t *key, int n) {
  // FNV-1a folded to a byte. Short, stable, and good enough to catch the
  // transposed pair that hand-copied hex actually suffers from.
  uint32_t h = 2166136261u;
  for (int i = 0; i < n; ++i) {
    h ^= key[i];
    h *= 16777619u;
  }
  return (uint8_t)((h ^ (h >> 8) ^ (h >> 16) ^ (h >> 24)) & 0xFF);
}

static size_t appendStr(char *out, size_t cap, size_t w, const char *s) {
  for (const char *p = s; *p && w + 1 < cap; ++p) out[w++] = *p;
  return w;
}

void formatJoin(const char *name, const uint8_t *key, bool has_key, char *out,
                size_t cap) {
  if (!cap) return;
  size_t w = appendStr(out, cap, 0, kPrefix);
  w = appendStr(out, cap, w, name ? name : "");
  if (has_key) {
    if (w + 1 < cap) out[w++] = '/';
    char hex[KEY_HEX + 1];
    toHex(key, KEY_BYTES, hex, sizeof(hex));
    w = appendStr(out, cap, w, hex);
  }
  out[w] = 0;
}

bool parseJoin(const char *s, char *name, size_t name_cap, uint8_t *key,
               bool &has_key) {
  has_key = false;
  if (!s || !name || name_cap == 0) return false;
  for (size_t i = 0; i < kPrefixLen; ++i)
    if (s[i] != kPrefix[i]) return false;

  const char *n = s + kPrefixLen;
  const char *slash = n;
  while (*slash && *slash != '/') ++slash;
  size_t nlen = (size_t)(slash - n);
  if (nlen == 0 || nlen >= name_cap) return false;
  for (size_t i = 0; i < nlen; ++i) {
    // A join string that carries control bytes is a malformed join string, not
    // a channel name to be pasted into the UI (rule 6: never crash on content).
    if (n[i] < ' ' || (uint8_t)n[i] >= 0x7F) return false;
    name[i] = n[i];
  }
  name[nlen] = 0;

  if (*slash != '/') return true; // hashtag invite: no key travels with it
  int got = fromHex(slash + 1, key, KEY_BYTES);
  if (got != KEY_BYTES) return false;
  has_key = true;
  return true;
}

// The default public channel does NOT follow the hashtag rule: MeshCore ships
// it with a fixed PSK (PUBLIC_GROUP_PSK, "izOH6cXN6mrJ5e26oRXNcg==" in
// examples/companion_radio/MyMesh.cpp). Deriving a key from the name "#Public"
// would put us alone on a channel of our own.
static const uint8_t kPublicPsk[KEY_BYTES] = {
    0x8b, 0x33, 0x87, 0xe9, 0xc5, 0xcd, 0xea, 0x6a,
    0xc9, 0xe5, 0xed, 0xba, 0xa1, 0x15, 0xcd, 0x72};

static bool isPublic(const char *name) {
  return name && (strcmp(name, "#Public") == 0 || strcmp(name, "Public") == 0);
}

void deriveHashtagKey(const char *name, uint8_t *key) {
  if (isPublic(name)) {
    memcpy(key, kPublicPsk, KEY_BYTES);
    return;
  }
  // Every other hashtag channel: the first 16 bytes of sha256 over the name
  // exactly as written, leading '#' included and no trailing NUL. This is the
  // derivation every other MeshCore client uses (docs/companion_protocol.md
  // "Hashtag Channels"), so a channel joined here is the same channel there.
  uint8_t digest[32];
  merlin::sha256(name, name ? strlen(name) : 0, digest);
  memcpy(key, digest, KEY_BYTES);
}

void generateKey(uint32_t seed, uint8_t *key) {
  uint32_t h = seed * 2654435761u + 0x9E3779B9u;
  for (int i = 0; i < KEY_BYTES; ++i) {
    h ^= h << 13;
    h ^= h >> 17;
    h ^= h << 5;
    key[i] = (uint8_t)(h >> 8);
  }
}

} // namespace channel
} // namespace merlin
