// =============================================================================
//  MeteoPlaneRadar
//  AsyncCore.cpp - Dual-Core FreeRTOS multitasking and cross-core synchronization.
//
//  Decouples the rendering and touch pipeline (Core 1) from all network I/O,
//  TLS handshakes, JSON parsing, PNG decoding, and WebServer serving (Core 0).
//
//  Author:  Petr / chiptron.cz & Antigravity
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (ESP32-S3R8, dual-core 240MHz)
// =============================================================================
#include "AsyncCore.h"
#include "Config.h"
#include "Settings.h"
#include "ADSB.h"
#include "CHMU.h"
#include "RainViewer.h"
#include "Forecast.h"
#include "Route.h"
#include "Outside.h"
#include "WebConfig.h"
#include "WiFiPortal.h"
#include "NightMode.h"
#include "Watchdog.h"
#include <WiFi.h>

static SemaphoreHandle_t s_mtxSettings = NULL;
static SemaphoreHandle_t s_mtxAdsb     = NULL;
static SemaphoreHandle_t s_mtxRadar    = NULL;
static SemaphoreHandle_t s_mtxForecast = NULL;
static SemaphoreHandle_t s_mtxRoute    = NULL;

static TaskHandle_t s_netTaskHandle = NULL;
static volatile bool s_paused = false;

// Flags for Core 1 (UI)
static volatile bool s_adsbUpdated = false;
static volatile bool s_radarUpdated = false;
static volatile bool s_forecastUpdated = false;
static volatile bool s_routeUpdated = false;

// Active state for Core 0
static volatile uint8_t s_activeScreen = SCREEN_PLANES_I;
static double s_targetLat = DEFAULT_LAT;
static double s_targetLon = DEFAULT_LON;
static float  s_targetRangeKm = 25.0f;
static volatile bool s_reqAdsb = false;
static volatile bool s_reqRadar = false;
static volatile bool s_reqForecast = false;

// Mutex Helpers
void Async_LockSettings()  { if (s_mtxSettings) xSemaphoreTake(s_mtxSettings, portMAX_DELAY); }
void Async_UnlockSettings(){ if (s_mtxSettings) xSemaphoreGive(s_mtxSettings); }

void Async_LockAdsb()      { if (s_mtxAdsb) xSemaphoreTake(s_mtxAdsb, portMAX_DELAY); }
void Async_UnlockAdsb()    { if (s_mtxAdsb) xSemaphoreGive(s_mtxAdsb); }

void Async_LockRadar()     { if (s_mtxRadar) xSemaphoreTake(s_mtxRadar, portMAX_DELAY); }
void Async_UnlockRadar()   { if (s_mtxRadar) xSemaphoreGive(s_mtxRadar); }

void Async_LockForecast()  { if (s_mtxForecast) xSemaphoreTake(s_mtxForecast, portMAX_DELAY); }
void Async_UnlockForecast(){ if (s_mtxForecast) xSemaphoreGive(s_mtxForecast); }

void Async_LockRoute()     { if (s_mtxRoute) xSemaphoreTake(s_mtxRoute, portMAX_DELAY); }
void Async_UnlockRoute()   { if (s_mtxRoute) xSemaphoreGive(s_mtxRoute); }

static SemaphoreHandle_t s_mtxI2c = NULL;
void Async_LockI2C()       { if (!s_mtxI2c) s_mtxI2c = xSemaphoreCreateMutex(); if (s_mtxI2c) xSemaphoreTake(s_mtxI2c, portMAX_DELAY); }
void Async_UnlockI2C()     { if (s_mtxI2c) xSemaphoreGive(s_mtxI2c); }

void Async_Pause()         { s_paused = true; }
void Async_Resume()        { s_paused = false; }
bool Async_IsPaused()      { return s_paused; }

void Async_SetActiveScreen(uint8_t screenIdx) {
  s_activeScreen = screenIdx;
}

void Async_SetAdsbTarget(double lat, double lon, float rangeKm) {
  s_targetLat = lat;
  s_targetLon = lon;
  s_targetRangeKm = rangeKm;
}

void Async_RequestAdsb()     { s_reqAdsb = true; }
void Async_RequestRadar()    { s_reqRadar = true; }
void Async_RequestForecast() { s_reqForecast = true; }

void Async_RequestRoute(const char* callsign, float lat, float lon) {
  Route_Select(callsign, lat, lon);
}

bool Async_TakeAdsbUpdated() {
  if (s_adsbUpdated) { s_adsbUpdated = false; return true; }
  return false;
}

bool Async_TakeRadarUpdated() {
  if (s_radarUpdated) { s_radarUpdated = false; return true; }
  return false;
}

bool Async_TakeForecastUpdated() {
  if (s_forecastUpdated) { s_forecastUpdated = false; return true; }
  return false;
}

bool Async_TakeRouteUpdated() {
  if (s_routeUpdated) { s_routeUpdated = false; return true; }
  return false;
}

// -----------------------------------------------------------------------------
//  Core 0 Worker Task
// -----------------------------------------------------------------------------
static void asyncWorkerTask(void* param) {
  (void)param;
  Watchdog_RegisterTask();
  unsigned long lastAdsbFetch = 0;
  unsigned long lastForecastFetch = 0;
  unsigned long lastOutsideFetch = 0;

  Serial.println("AsyncCore: Worker task running on Core 0");

  while (true) {
    Watchdog_Feed();
    if (s_paused) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // 1. Serve Web Server and WiFi Portal
    WebConfig_Loop();
    WiFi_Loop();

    if (WiFi.status() == WL_CONNECTED) {
      unsigned long now = millis();
      static unsigned long lastTlsTime = 0;
      static bool s_firstTimeReseed = false;

      const uint8_t curScr = s_activeScreen;
      const bool planesActive = (curScr == SCREEN_PLANES_I || curScr == SCREEN_TACTICAL_I);
      const bool rvBusy = RainViewer_Busy();

      // Clear background route lookup queue when leaving aircraft screens
      if (!planesActive) {
        Route_ClearQueue();
      }

      // Fast clock sync on first WiFi connect if time is not yet valid from RTC
      if (!s_firstTimeReseed && !Outside_TimeValid() && !rvBusy) {
        s_firstTimeReseed = true;
        Outside_Tick();
        lastTlsTime = millis();
      }

      // 1. Radar Tile Download (RainViewer background stepping - highest priority when on radar)
      if (rvBusy) {
        if (RainViewer_Step()) {
          s_radarUpdated = true;
        }
        lastTlsTime = millis();
      }
      else if (now - lastTlsTime >= 600) {
        unsigned long adsbPeriod = (s_targetRangeKm <= ADSB_NEAR_KM) ? ADSB_PERIOD_NEAR_MS :
                                   (s_targetRangeKm <= ADSB_MID_KM)  ? ADSB_PERIOD_MID_MS  : ADSB_PERIOD_FAR_MS;

        // 2. Pending Route Lookups (only when active on planes screen, queue not empty, and not right after ADS-B)
        if (planesActive && (now - lastAdsbFetch >= 1000) && Route_HasPending()) {
          Route_Tick();
          lastTlsTime = millis();
          if (Route_TakeChanged()) {
            s_routeUpdated = true;
          }
        }
        // 3. ADS-B Aircraft Fetching (when on Planes or Tactical screen, or requested)
        else if ((planesActive && (now - lastAdsbFetch >= adsbPeriod)) || s_reqAdsb) {
          s_reqAdsb = false;
          lastAdsbFetch = now;
          double lat = s_targetLat, lon = s_targetLon;
          float rng = s_targetRangeKm;
          if (ADSB_Fetch(lat, lon, rng)) {
            s_adsbUpdated = true;
          }
          lastTlsTime = millis();
        }
        // 4. Forecast & Air Quality (immediate on first boot / WiFi connect, then periodic)
        else {
          static bool s_firstForecastDone = false;
          if (!s_firstForecastDone || (now - lastForecastFetch >= FORECAST_PERIOD_MS) || s_reqForecast) {
            s_reqForecast = false;
            s_firstForecastDone = true;
            lastForecastFetch = now;
            Forecast_Tick();
            s_forecastUpdated = true;
            lastTlsTime = millis();
          }
          // 5. Outside Temperature & Time Seeding
          else if (now - lastOutsideFetch >= OUTSIDE_TEMP_PERIOD_MS) {
            lastOutsideFetch = now;
            Outside_Tick();
            lastTlsTime = millis();
          }
        }
      }
    }

    // Short cooperative yield for FreeRTOS scheduler on Core 0
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void Async_Begin() {
  if (!s_mtxSettings) s_mtxSettings = xSemaphoreCreateMutex();
  if (!s_mtxAdsb)     s_mtxAdsb     = xSemaphoreCreateMutex();
  if (!s_mtxRadar)    s_mtxRadar    = xSemaphoreCreateMutex();
  if (!s_mtxForecast) s_mtxForecast = xSemaphoreCreateMutex();
  if (!s_mtxRoute)    s_mtxRoute    = xSemaphoreCreateMutex();

  // Create background network worker task on Core 0 with 8KB stack
  BaseType_t res = xTaskCreatePinnedToCore(
    asyncWorkerTask,
    "AsyncNetWorker",
    8192,
    NULL,
    1, // Priority 1 (idle is 0, UI on Core 1 is 1)
    &s_netTaskHandle,
    0  // Core 0
  );

  if (res == pdPASS) {
    Serial.println("AsyncCore: Worker task initialized on Core 0");
  } else {
    Serial.println("AsyncCore ERROR: Failed to create worker task on Core 0");
  }
}
