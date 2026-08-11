// MERLIN UI — channel identity helpers: the join string, hex entry, and the
// read-back checksum. Platform-pure and allocation-free, because both the
// simulator and the device port need exactly the same string on the wire.
//
// Join string (design spec §Channels): meshcore://join/<name>/<32 hex>
// Hashtag channels carry no key — the name derives it — so they stop at
// meshcore://join/<name>.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace merlin {
namespace channel {

constexpr int KEY_BYTES = 16;      // 128-bit channel key
constexpr int KEY_HEX = 32;        // its hex form, no separators
constexpr size_t JOIN_CAP = 80;    // longest join string + NUL
constexpr size_t NAME_CAP = 24;    // channel name incl. leading '#'

// Hex without separators. Always NUL-terminates.
void toHex(const uint8_t *key, int n, char *out, size_t cap);

// Parse hex, ignoring spaces (the entry UI groups digits in 4s). Returns the
// number of bytes decoded, or -1 on a non-hex character / odd digit count.
int fromHex(const char *hex, uint8_t *key, int max_bytes);

// Two-hex-digit read-back digest of a key (or of a partial key during entry).
// Not cryptographic — it is there so two humans reading a key aloud find out
// they disagree before they trust the channel.
uint8_t checksum(const uint8_t *key, int n);

// Build meshcore://join/... . Always NUL-terminates.
void formatJoin(const char *name, const uint8_t *key, bool has_key, char *out,
                size_t cap);

// True iff `s` is a join string; fills name/key. `has_key` comes back false for
// a hashtag invite. Rejects anything malformed rather than half-filling.
bool parseJoin(const char *s, char *name, size_t name_cap, uint8_t *key,
               bool &has_key);

// Hashtag channels derive their key from the name, so anyone who knows the name
// is in: the first 16 bytes of sha256 over the name as written, '#' included.
// This is MeshCore's own derivation, so a hashtag joined here is the same
// channel every other client sees. "#Public" is the one exception — it carries
// a fixed PSK rather than a derived key.
void deriveHashtagKey(const char *name, uint8_t *key);

// Deterministic key source for "create private". Seeded per store so a fresh
// simulator run always mints the same keys and the goldens stay pixel-stable;
// the device port replaces this with the hardware RNG.
void generateKey(uint32_t seed, uint8_t *key);

} // namespace channel
} // namespace merlin
