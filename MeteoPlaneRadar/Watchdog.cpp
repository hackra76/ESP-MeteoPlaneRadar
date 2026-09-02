// =============================================================================
//  MeteoPlaneRadar
//  Hardware watchdog for 24/7 operation.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "Watchdog.h"
#include "esp_task_wdt.h"

static bool s_initialized = false;

void Watchdog_Begin() {
  if (s_initialized) {
    Watchdog_RegisterTask();
    return;
  }
  // ESP32 core 3.x uses a config struct.
  esp_task_wdt_config_t cfg = {
    .timeout_ms = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = 0,       // watch only our tasks, not the idle tasks
    .trigger_panic = true,     // reboot on timeout
  };
  // Arduino already initialised the TWDT - just reconfigure it; if not, init.
  if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) {
    esp_task_wdt_init(&cfg);
  }
  s_initialized = true;
  Watchdog_RegisterTask();
}

void Watchdog_RegisterTask() {
  if (!s_initialized) {
    Watchdog_Begin();
    return;
  }
  if (esp_task_wdt_status(NULL) != ESP_OK) {
    esp_task_wdt_add(NULL);
  }
}

void Watchdog_Feed() {
  if (!s_initialized) return;
  if (esp_task_wdt_status(NULL) == ESP_OK) {
    esp_task_wdt_reset();
  } else {
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();
  }
}
