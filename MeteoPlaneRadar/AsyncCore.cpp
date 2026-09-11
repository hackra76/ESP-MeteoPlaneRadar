// =============================================================================
//  MeteoPlaneRadar
//  AsyncCore.cpp - Dual-Core FreeRTOS multitasking and cross-core synchronization.
//
//  Decouples the rendering and touch pipeline (Core 1) from all network I/O,
//  TLS handshakes, JSON parsing, PNG decoding, and WebServer serving (Core 0).
//
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (ESP32-S3R8, dual-core 240MHz)
// =============================================================================
#include "AsyncCore.h"
#include "Config.h"
#include "Settings.h"
#include "ADSB.h"
#include "CHMU.h"
#include "SHMU.h"
#include "RainViewer.h"
#include "Forecast.h"
#include "Route.h"
#include "PlanePhoto.h"
#include "Outside.h"
#include "WebConfig.h"
#include "WiFiPortal.h"
#include "NightMode.h"
#include "Watchdog.h"
#include "GithubOTA.h"
#include "FinanceData.h"
#include "IssData.h"
#include "ScreenWeather.h"
#include <WiFi.h>

static SemaphoreHandle_t s_mtxSettings = NULL;
static SemaphoreHandle_t s_mtxAdsb     = NULL;
static SemaphoreHandle_t s_mtxRadar    = NULL;
static SemaphoreHandle_t s_mtxForecast = NULL;
static SemaphoreHandle_t s_mtxFinance  = NULL;
static SemaphoreHandle_t s_mtxIss      = NULL;
static SemaphoreHandle_t s_mtxRoute    = NULL;

static TaskHandle_t s_netTaskHandle = NULL;
static volatile bool s_paused = false;
static volatile bool s_core0NetBusy = false;
bool Async_IsNetBusy() { return s_core0NetBusy; }

// Flags for Core 1 (UI)
static volatile bool s_adsbUpdated = false;
static volatile bool s_radarUpdated = false;
static volatile bool s_forecastUpdated = false;
static volatile bool s_financeUpdated = false;
static volatile bool s_issUpdated = false;
static volatile bool s_routeUpdated = false;

// Active state for Core 0
static volatile uint8_t s_activeScreen = SCREEN_PLANES_I;
static double s_targetLat = DEFAULT_LAT;
static double s_targetLon = DEFAULT_LON;
static float  s_targetRangeKm = 25.0f;
static volatile bool s_reqAdsb = false;
static volatile bool s_reqRadar = false;
static volatile bool s_reqForecast = false;
static volatile bool s_reqFinance = false;
static volatile bool s_reqIss = false;

// Mutex Helpers
void Async_LockSettings()  { if (s_mtxSettings) xSemaphoreTake(s_mtxSettings, pdMS_TO_TICKS(200)); }
void Async_UnlockSettings(){ if (s_mtxSettings) xSemaphoreGive(s_mtxSettings); }

void Async_LockAdsb()      { if (s_mtxAdsb) xSemaphoreTake(s_mtxAdsb, pdMS_TO_TICKS(200)); }
void Async_UnlockAdsb()    { if (s_mtxAdsb) xSemaphoreGive(s_mtxAdsb); }

void Async_LockRadar()     { if (s_mtxRadar) xSemaphoreTake(s_mtxRadar, pdMS_TO_TICKS(200)); }
void Async_UnlockRadar()   { if (s_mtxRadar) xSemaphoreGive(s_mtxRadar); }

void Async_LockForecast()  { if (s_mtxForecast) xSemaphoreTake(s_mtxForecast, pdMS_TO_TICKS(200)); }
void Async_UnlockForecast(){ if (s_mtxForecast) xSemaphoreGive(s_mtxForecast); }

void Async_LockFinance()   { if (s_mtxFinance) xSemaphoreTake(s_mtxFinance, pdMS_TO_TICKS(200)); }
void Async_UnlockFinance() { if (s_mtxFinance) xSemaphoreGive(s_mtxFinance); }

void Async_LockIss()       { if (s_mtxIss) xSemaphoreTake(s_mtxIss, pdMS_TO_TICKS(200)); }
void Async_UnlockIss()     { if (s_mtxIss) xSemaphoreGive(s_mtxIss); }

void Async_LockRoute()     { if (s_mtxRoute) xSemaphoreTake(s_mtxRoute, pdMS_TO_TICKS(200)); }
void Async_UnlockRoute()   { if (s_mtxRoute) xSemaphoreGive(s_mtxRoute); }

static SemaphoreHandle_t s_mtxI2c = NULL;
void Async_LockI2C()       { if (s_mtxI2c) xSemaphoreTakeRecursive(s_mtxI2c, pdMS_TO_TICKS(150)); }
void Async_UnlockI2C()     { if (s_mtxI2c) xSemaphoreGiveRecursive(s_mtxI2c); }

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

void Async_RequestFinance() { s_reqFinance = true; }
void Async_RequestIss()     { s_reqIss = true; }

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

bool Async_TakeFinanceUpdated() {
  if (s_financeUpdated) { s_financeUpdated = false; return true; }
  return false;
}

bool Async_TakeIssUpdated() {
  if (s_issUpdated) { s_issUpdated = false; return true; }
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
  unsigned long lastRadarFetch = 0;
  unsigned long lastFinanceFetch = 0;
  unsigned long lastIssFetch = 0;

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

    // While a firmware update is running, suspend all background network/TLS operations
    if (WebConfig_UpdateBusy()) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (WiFi.status() == WL_CONNECTED) {
      unsigned long now = millis();
      static unsigned long lastTlsTime = 0;
      static bool s_firstTimeReseed = false;

      const uint8_t curScr = s_activeScreen;
      const bool planesActive = (curScr == SCREEN_PLANES_I || curScr == SCREEN_TACTICAL_I);
      const bool rvBusy = RainViewer_Busy();

      // Clear background route lookup queue and photo when leaving aircraft screens
      if (!planesActive) {
        Route_ClearQueue();
        PlanePhoto_Clear();
      }

      // Fast clock sync on first WiFi connect if time is not yet valid from RTC
      if (!s_firstTimeReseed && !Outside_TimeValid() && !rvBusy) {
        if (ScreenWeather_IsLoading()) {
          vTaskDelay(pdMS_TO_TICKS(50));
          continue;
        }
        s_firstTimeReseed = true;
        s_core0NetBusy = true;
        Outside_Tick();
        s_core0NetBusy = false;
        lastTlsTime = millis();
      }

      // Background silent OTA release check (first check 45s after boot, then every 12 hours)
      static unsigned long s_lastOtaAutoCheck = 0;
      if (((s_lastOtaAutoCheck == 0 && now >= 45000UL) ||
           (s_lastOtaAutoCheck > 0 && (now - s_lastOtaAutoCheck >= 12UL * 3600UL * 1000UL))) &&
          !GithubOTA_IsBusy() && GithubOTA_GetState() != GH_OTA_CHECKING && !rvBusy) {
        s_lastOtaAutoCheck = now;
        GithubOTA_CheckAsync();
      }

      // 1. Radar Tile Download (RainViewer background stepping - highest priority when on radar)
      if (rvBusy) {
        Watchdog_Feed();
        s_core0NetBusy = true;
        if (RainViewer_Step()) {
          s_radarUpdated = true;
        }
        s_core0NetBusy = false;
        Watchdog_Feed();
        lastTlsTime = millis();
        vTaskDelay(pdMS_TO_TICKS(15));
      }
      else if (now - lastTlsTime >= 600) {
        if (ScreenWeather_IsLoading()) {
          vTaskDelay(pdMS_TO_TICKS(50));
          continue;
        }

        s_core0NetBusy = true;
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
        // 2b. Pending Plane Photo Lookups
        else if (planesActive && (now - lastAdsbFetch >= 1000) && PlanePhoto_HasPending()) {
          PlanePhoto_Tick();
          lastTlsTime = millis();
          if (PlanePhoto_TakeChanged()) {
            s_routeUpdated = true;
          }
        }
        // 2c. Tactical Radar Fetch for CHMU / SHMU (when on Tactical screen and not RainViewer)
        else if ((curScr == SCREEN_TACTICAL_I) && (Settings_RadarSource() != RADAR_SRC_RAINVIEWER) &&
                 (s_reqRadar || (now - lastRadarFetch >= TACTICAL_RADAR_PERIOD_MS))) {
          s_reqRadar = false;
          lastRadarFetch = now;
          int n = (Settings_RadarSource() == RADAR_SRC_SHMU) ? SHMU_FetchAnim(1) : CHMU_FetchAnim(1);
          if (n > 0) {
            s_radarUpdated = true;
          }
          lastTlsTime = millis();
        }
        // 3. ADS-B Aircraft Fetching (when on Planes or Tactical screen, or periodic background scan for emergency squawks)
        else if ((planesActive && (now - lastAdsbFetch >= adsbPeriod)) ||
                 (!planesActive && Settings_SquawkAlert() && (now - lastAdsbFetch >= 25000)) ||
                 s_reqAdsb) {
          s_reqAdsb = false;
          lastAdsbFetch = now;
          double lat = planesActive ? s_targetLat : Settings_Lat();
          double lon = planesActive ? s_targetLon : Settings_Lon();
          float rng = planesActive ? s_targetRangeKm : 250.0f;
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
          // 6. Finance Data (Markets, Crypto, Stocks, Commodities)
          else if (curScr == SCREEN_FINANCE_I || s_reqFinance || (now - lastFinanceFetch >= FINANCE_PERIOD_MS)) {
            if (Finance_Step()) {
              s_financeUpdated = true;
              lastTlsTime = millis();
            }
            if (!Finance_IsBusy()) {
              s_reqFinance = false;
              lastFinanceFetch = now;
            }
          }
          // 7. ISS Orbit Tracker
          else if (curScr == SCREEN_ISS_I || s_reqIss ||
                   (now - lastIssFetch >= (Settings_ScreenEnabled(SCREEN_ISS_I) ? ((curScr == SCREEN_ISS_I) ? ISS_PERIOD_ACTIVE_MS : ISS_PERIOD_BG_MS) : 60000UL))) {
            if (Iss_Step()) {
              s_issUpdated = true;
              lastTlsTime = millis();
            }
            s_reqIss = false;
            lastIssFetch = now;
          }
        }
        s_core0NetBusy = false;
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
  if (!s_mtxFinance)  s_mtxFinance  = xSemaphoreCreateMutex();
  if (!s_mtxIss)      s_mtxIss      = xSemaphoreCreateMutex();
  if (!s_mtxRoute)    s_mtxRoute    = xSemaphoreCreateMutex();
  if (!s_mtxI2c)      s_mtxI2c      = xSemaphoreCreateRecursiveMutex();

  // Create background network worker task on Core 0 with 24KB stack
  BaseType_t res = xTaskCreatePinnedToCore(
    asyncWorkerTask,
    "AsyncNetWorker",
    24576, // 24KB stack (safe for deep mbedTLS handshake + JSON parsing)
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
