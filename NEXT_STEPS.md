# MeshCore: Next Steps — Network Usability Roadmap

## Context

After hardening the repeater firmware (watchdog, persistent stats, EWMA link quality, duplicate TX suppression, congestion backpressure, health in adverts, richer admin telemetry, login rate limiting, constant-time MAC, packet validation, snprintf bounds checks), these are the next priorities for network usability.

Branch: `janv/salishmesh-customizations`
Date: 2026-03-03

---

## Tier 1: Visibility — See the telemetry you already built

### 1. MeshCore Monitoring Dashboard (Python + Web)
**Impact: Highest** — all the telemetry data exists but has no UI.

- Connect to JSPRVR01 via BLE/serial as companion client
- Login to Duncan Sky over mesh, poll GET_STATUS + GET_NEIGHBOURS + GET_PERSISTENT_STATS periodically
- Store time-series data in SQLite
- Serve web UI (Flask or similar) with:
  - Neighbor topology map
  - Link quality trends (EWMA SNR/RSSI over time)
  - Battery/uptime graphs
  - Packet throughput charts
  - Queue pressure history
- **Alerting**: notification when battery drops, watchdog reset detected, neighbor disappears, queue pressure spikes
- Foundation: `meshcore_py` library (implements companion protocol)
- Could run on Pi5 or any always-on machine in BLE range of JSPRVR01

### 2. Advert Health Decoder in Companion Firmware
**Impact: High** — zero-effort monitoring for every phone app user.

- Companion firmware doesn't decode feat1/feat2 health fields from repeater adverts yet
- Adding decode logic means every app user sees repeater health just by receiving adverts — no admin login needed
- Fields already encoded: queue pressure, free pool count, uptime bucket, battery %, boot count

---

## Tier 2: Operational efficiency

### 3. Remote Config Over Mesh
**Impact: High** — eliminates roof climbs for config changes.

- Repeater already has CommonCLI with `set name/freq/tx/sf/bw/cr/etc`
- Expose a subset as authenticated admin commands over mesh (new REQ_TYPE_SET_CONFIG or similar)
- Critical configs to expose remotely:
  - Radio params (freq, bw, sf, cr, tx_power)
  - Node name
  - Advert intervals
  - Airtime budget factor
  - TX delay tuning params
- Safety: require admin permission level, echo back new value for confirmation
- Could also add REQ_TYPE_REBOOT for remote restart after config change

### 4. Auto-Recovery Enhancements
**Impact: High** — handles soft failures the watchdog can't catch.

- **Radio health check**: if no packets received for N minutes (configurable, e.g. 10), reinit radio hardware
- **Radio self-test**: periodically verify TX complete interrupt fires (detect stuck radio)
- **Flash integrity**: CRC/checksum on prefs and stats files, recover from defaults if corrupted
- **Memory monitoring**: track free heap, alert/reboot if critically low (ESP32 especially)
- **Automatic radio reinit on repeated TX failures**: if N consecutive TX attempts fail, reinit SX1262

---

## Tier 3: Network growth

### 5. Tzouhalem Deployment Prep
**Impact: Medium** — enables second repeater commissioning.

- Multi-repeater dashboard view (once dashboard exists)
- Cross-repeater neighbour overlap analysis (which nodes does each see?)
- Automated propagation comparison between sites
- Having monitoring before deploying = can commission remotely

### 6. OTA Firmware Updates
**Impact: Highest long-term** — eliminates all physical access for updates.

- ESP32 (Tzouhalem/Heltec V4): native OTA partition support, "just" need chunk transfer protocol over mesh
- NRF52 (Duncan Sky/RAK4631): needs DFU-over-mesh bridge or BLE DFU relay
- Approach options:
  - **Simple**: companion node in BLE range acts as OTA relay (BLE DFU from Pi → repeater)
  - **Medium**: chunk transfer over mesh admin protocol (new REQ_TYPE_OTA_* commands), store to secondary flash partition, validate CRC, reboot into new firmware
  - **Full**: mesh-native firmware distribution with delta updates
- Safety: firmware signature verification, automatic rollback on boot failure (watchdog detects crash loop → revert to known-good partition)
- Start with BLE DFU relay (simplest path for Duncan Sky since JSPRVR01 is in BLE range)

---

## Implementation Order

```
Phase 5 (Visibility):
  1. Monitoring dashboard      ← see everything you built
  2. Advert health decoder     ← passive monitoring for all users

Phase 6 (Operational):
  3. Remote config over mesh   ← no more roof climbs for config
  4. Auto-recovery             ← handle soft failures

Phase 7 (Growth):
  5. Tzouhalem deployment prep ← second repeater
  6. OTA firmware updates      ← never climb again
```
