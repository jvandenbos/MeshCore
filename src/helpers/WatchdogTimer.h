#pragma once

#include <stdint.h>

/**
 * Hardware watchdog timer abstraction.
 * Resets the MCU if feed() is not called within the timeout period.
 * Zero RAM cost — uses hardware peripheral only.
 */
namespace WatchdogTimer {

#if defined(NRF52_PLATFORM)

  inline void init(uint32_t timeout_secs = 30) {
    NRF_WDT->CONFIG = 0x01;  // run during sleep
    // timeout = (CRV + 1) / 32768
    NRF_WDT->CRV = timeout_secs * 32768 - 1;
    NRF_WDT->RREN = 0x01;    // enable reload register 0
    NRF_WDT->TASKS_START = 1;
  }

  inline void feed() {
    NRF_WDT->RR[0] = 0x6E524635;  // reload magic value
  }

  inline bool wasWatchdogReset() {
    return (NRF_POWER->RESETREAS & 0x02) != 0;  // bit 1 = WDT
  }

  inline void clearResetReason() {
    NRF_POWER->RESETREAS = 0x02;  // write 1 to clear WDT bit
  }

#elif defined(ESP32)

  #include <esp_task_wdt.h>

  inline void init(uint32_t timeout_secs = 30) {
    esp_task_wdt_config_t config = {
      .timeout_ms = timeout_secs * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&config);
    esp_task_wdt_add(NULL);
  }

  inline void feed() {
    esp_task_wdt_reset();
  }

  inline bool wasWatchdogReset() {
    return false;  // ESP32 resets to bootloader, no persistent flag accessible here
  }

  inline void clearResetReason() { }

#elif defined(RP2040_PLATFORM)

  #include <hardware/watchdog.h>

  inline void init(uint32_t timeout_secs = 30) {
    // RP2040 watchdog max is ~8.3 seconds; use that as ceiling
    uint32_t ms = timeout_secs * 1000;
    if (ms > 8300) ms = 8300;
    watchdog_enable(ms, true);
  }

  inline void feed() {
    watchdog_update();
  }

  inline bool wasWatchdogReset() {
    return watchdog_caused_reboot();
  }

  inline void clearResetReason() { }

#else

  // Unsupported platform — no-op stubs so code still compiles
  inline void init(uint32_t timeout_secs = 30) { (void)timeout_secs; }
  inline void feed() { }
  inline bool wasWatchdogReset() { return false; }
  inline void clearResetReason() { }

#endif

} // namespace WatchdogTimer
