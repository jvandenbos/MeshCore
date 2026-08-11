// MERLIN UI — a self-contained SHA-256.
//
// Channel keys are derived from names, and the derivation has to be byte-identical
// on the simulator and on the radio or a channel joined on the T-Deck would be a
// channel nobody else is on. The firmware's SHA-256 comes from an Arduino library
// the host build cannot link, so the one shared implementation lives here, under
// the platform-pure rule: <stdint.h> and nothing else.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace merlin {

// Writes 32 bytes to `out`.
void sha256(const void *data, size_t len, uint8_t out[32]);

} // namespace merlin
