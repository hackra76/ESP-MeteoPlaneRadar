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
#include "MapTiles.h"
#include "Outside.h"
#include "WebConfig.h"
#include "WiFiPortal.h"
#include "NightMode.h"
#include "Watchdog.h"
#include "NetSink.h"
#include "GithubOTA.h"
#include "FinanceData.h"
#include "IssData.h"
#include "YouTubeData.h"
#include "KlipperData.h"
#include "PetBrain.h"
#include "DisasterAlert.h"
#include "ScreenWeather.h"
#include <WiFi.h>
#include <freertos/idf_additions.h>
#include <esp_heap_caps.h>

static SemaphoreHandle_t s_mtxSettings = NULL;
static SemaphoreHandle_t s_mtxAdsb     = NULL;
static SemaphoreHandle_t s_mtxRadar    = NULL;
static SemaphoreHandle_t s_mtxForecast = NULL;
static SemaphoreHandle_t s_mtxFinance  = NULL;
static SemaphoreHandle_t s_mtxIss      = NULL;
static SemaphoreHandle_t s_mtxKlipper  = NULL;
static SemaphoreHandle_t s_mtxRoute    = NULL;

static TaskHandle_t s_netTaskHandle = NULL;
static volatile bool s_paused = false;
static volatile bool s_core0NetBusy = false;
static volatile bool s_radarBusy = false;
bool Async_IsNetBusy()   { return s_core0NetBusy; }
bool Async_IsRadarBusy() { return s_radarBusy; }

// Flags for Core 1 (UI)
static volatile bool s_adsbUpdated = false;
static volatile bool s_radarUpdated = false;
static volatile bool s_forecastUpdated = false;
static volatile bool s_financeUpdated = false;
static volatile bool s_issUpdated = false;
static volatile bool s_youtubeUpdated = false;
static volatile bool s_klipperUpdated = false;
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
static volatile bool s_reqYouTube = false;
static volatile bool s_reqKlipper = false;
static volatile bool s_reqPetThought = false;

// Mutex Helpers (Recursive mutexes prevent same-core self-deadlock)
void Async_LockSettings()  { if (s_mtxSettings) xSemaphoreTakeRecursive(s_mtxSettings, pdMS_TO_TICKS(1000)); }
void Async_UnlockSettings(){ if (s_mtxSettings) xSemaphoreGiveRecursive(s_mtxSettings); }

void Async_LockAdsb()      { if (s_mtxAdsb)     xSemaphoreTakeRecursive(s_mtxAdsb,     pdMS_TO_TICKS(1000)); }
void Async_UnlockAdsb()    { if (s_mtxAdsb)     xSemaphoreGiveRecursive(s_mtxAdsb); }

void Async_LockRadar()     { if (s_mtxRadar)    xSemaphoreTakeRecursive(s_mtxRadar,    pdMS_TO_TICKS(1000)); }
void Async_UnlockRadar()   { if (s_mtxRadar)    xSemaphoreGiveRecursive(s_mtxRadar); }

void Async_LockForecast()  { if (s_mtxForecast) xSemaphoreTakeRecursive(s_mtxForecast, pdMS_TO_TICKS(1000)); }
void Async_UnlockForecast(){ if (s_mtxForecast) xSemaphoreGiveRecursive(s_mtxForecast); }

void Async_LockFinance()   { if (s_mtxFinance)  xSemaphoreTakeRecursive(s_mtxFinance,  pdMS_TO_TICKS(1000)); }
void Async_UnlockFinance() { if (s_mtxFinance)  xSemaphoreGiveRecursive(s_mtxFinance); }

void Async_LockIss()       { if (s_mtxIss)      xSemaphoreTakeRecursive(s_mtxIss,      pdMS_TO_TICKS(1000)); }
void Async_UnlockIss()     { if (s_mtxIss)      xSemaphoreGiveRecursive(s_mtxIss); }

void Async_LockKlipper()   { if (s_mtxKlipper)  xSemaphoreTakeRecursive(s_mtxKlipper,  pdMS_TO_TICKS(1000)); }
void Async_UnlockKlipper() { if (s_mtxKlipper)  xSemaphoreGiveRecursive(s_mtxKlipper); }

void Async_LockRoute()     { if (s_mtxRoute)    xSemaphoreTakeRecursive(s_mtxRoute,    pdMS_TO_TICKS(1000)); }
void Async_UnlockRoute()   { if (s_mtxRoute)    xSemaphoreGiveRecursive(s_mtxRoute); }

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
  // Settings lock required: double is 8 bytes, written as two 32-bit stores
  // on Xtensa LX7. Without the lock, Core 0 can read a torn lat/lon value.
  Async_LockSettings();
  s_targetLat     = lat;
  s_targetLon     = lon;
  s_targetRangeKm = rangeKm;
  Async_UnlockSettings();
}

void Async_RequestAdsb()     { s_reqAdsb = true; }
void Async_RequestRadar()    { s_reqRadar = true; }
void Async_RequestForecast() {
  Forecast_Invalidate();
  s_reqForecast = true;
}

void Async_RequestRoute(const char* callsign, float lat, float lon) {
  Route_Select(callsign, lat, lon);
}

void Async_RequestFinance() {
  s_reqFinance = true;
  Finance_RequestFetch();
}
void Async_RequestIss()     { s_reqIss = true; }
void Async_RequestYouTube() {
  s_reqYouTube = true;
  YouTube_RequestFetch();
}
void Async_RequestKlipper() {
  if (s_activeScreen == SCREEN_KLIPPER_I) {
    s_reqKlipper = true;
    Klipper_RequestFetch();
  }
}
void Async_RequestPetThought() { s_reqPetThought = true; }

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

bool Async_TakeYouTubeUpdated() {
  if (s_youtubeUpdated) { s_youtubeUpdated = false; return true; }
  return false;
}

bool Async_TakeKlipperUpdated() {
  if (s_klipperUpdated) { s_klipperUpdated = false; return true; }
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
  unsigned long lastYouTubeFetch = 0;
  unsigned long lastKlipperFetch = 0;

  Serial.println("AsyncCore: Worker task running on Core 0");

  while (true) {
    Watchdog_Feed();
    if (GithubOTA_IsRequested()) {
      s_core0NetBusy = true;
      GithubOTA_RunUpdate();
      s_core0NetBusy = false;
      continue;
    }
    if (s_paused) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // 1. Serve Web Server only. WiFi reconnect/AP handling runs on Core 1
    //    (main.cpp loop) to avoid concurrent WiFi stack calls from two cores.
    WebConfig_Loop();

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
      const bool planesActive = (curScr == SCREEN_PLANES_I || curScr == SCREEN_TACTICAL_I || curScr == SCREEN_SONAR_I);
      const bool rvBusy = RainViewer_Busy();

      const bool clockActive = (curScr == SCREEN_CLOCK_I);
      const uint8_t clkTop = clockActive ? Settings_ClockTopSlot() : 255;
      const uint8_t clkBot = clockActive ? Settings_ClockBottomSlot() : 255;
      const bool clockNeedsAircraft = clockActive && (clkTop == CLOCK_SLOT_AIRCRAFT || clkBot == CLOCK_SLOT_AIRCRAFT);
      const bool clockNeedsFinance  = clockActive && (clkTop == CLOCK_SLOT_MARKET   || clkBot == CLOCK_SLOT_MARKET);
      const bool clockNeedsYouTube  = clockActive && (clkTop == CLOCK_SLOT_YOUTUBE  || clkBot == CLOCK_SLOT_YOUTUBE);
      const bool clockNeedsWeather  = clockActive && (clkTop == CLOCK_SLOT_WEATHER  || clkBot == CLOCK_SLOT_WEATHER ||
                                                      clkTop == CLOCK_SLOT_SPARKLINE|| clkBot == CLOCK_SLOT_SPARKLINE);

      // Klipper fetch requests are strictly discarded when not on Klipper screen
      if (curScr != SCREEN_KLIPPER_I) {
        s_reqKlipper = false;
      }

      // Clear background route lookup queue, photo, and map tiles on transition away from aircraft screens
      static bool s_lastPlanesActive = false;
      if (!planesActive && s_lastPlanesActive) {
        Route_ClearQueue();
        PlanePhoto_Clear();
        MapTiles_Cancel();
        s_reqAdsb = false;
      }
      s_lastPlanesActive = planesActive;

      // Fast clock sync on first WiFi connect if time is not yet valid from RTC
      if (!s_firstTimeReseed && !Outside_TimeValid() && !rvBusy) {
        s_firstTimeReseed = true;
        s_core0NetBusy = true;
        Outside_Tick();
        s_core0NetBusy = false;
        lastTlsTime = millis();
      }

      bool mtBusy = MapTiles_Busy();

      // Background silent OTA release check (first check 45s after boot, then every 12 hours)
      static unsigned long s_lastOtaAutoCheck = 0;
      if (((s_lastOtaAutoCheck == 0 && now >= 45000UL) ||
           (s_lastOtaAutoCheck > 0 && (now - s_lastOtaAutoCheck >= 12UL * 3600UL * 1000UL))) &&
          !GithubOTA_IsBusy() && GithubOTA_GetState() != GH_OTA_CHECKING && !rvBusy && !mtBusy) {
        if (Net_HeapOk("GH_OTA")) {
          s_lastOtaAutoCheck = now;
          s_core0NetBusy = true;
          GithubOTA_CheckSync();
          s_core0NetBusy = false;
          lastTlsTime = millis();
        }
      }

      // 1. Online Map Tiles (background stepping for aircraft radar underlay)
      if (mtBusy && (now - lastTlsTime >= 350)) {
        Watchdog_Feed();
        s_core0NetBusy = true;
        if (MapTiles_Step()) {
          s_adsbUpdated = true;
        }
        s_core0NetBusy = false;
        Watchdog_Feed();
        lastTlsTime = millis();
        vTaskDelay(pdMS_TO_TICKS(30));
      }
      // 2. Radar Tile Download (RainViewer background stepping - highest priority when on radar)
      else if (rvBusy) {
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
      else if (now - lastTlsTime >= 1000) {
        s_core0NetBusy = true;
        // RAII guard: ensures s_core0NetBusy is always cleared even if a called
        // function returns early or the WiFi condition changes mid-chain.
        struct NetBusyGuard {
          ~NetBusyGuard() { s_core0NetBusy = false; }
        } _guard;
        (void)_guard;

        static int s_adsbFails = 0;
        unsigned long adsbPeriodBase = (s_targetRangeKm <= ADSB_NEAR_KM) ? ADSB_PERIOD_NEAR_MS :
                                       (s_targetRangeKm <= ADSB_MID_KM)  ? ADSB_PERIOD_MID_MS  : ADSB_PERIOD_FAR_MS;
        unsigned long adsbPeriod = adsbPeriodBase;
        if (s_adsbFails > 0) {
          int shift = s_adsbFails > 5 ? 5 : s_adsbFails;
          adsbPeriod <<= shift;
        }

        static int s_radarFails = 0;
        unsigned long radarPeriod = TACTICAL_RADAR_PERIOD_MS;
        if (s_radarFails > 0) {
          int shift = s_radarFails > 3 ? 3 : s_radarFails;
          radarPeriod <<= shift;
        }
        const bool radarDue = (Settings_RadarSource() != RADAR_SRC_RAINVIEWER) &&
                              ((curScr == SCREEN_METEO_I && s_reqRadar) ||
                               (curScr == SCREEN_TACTICAL_I && (s_reqRadar || (now - lastRadarFetch >= radarPeriod))));

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
        // 2c. Weather & Tactical Radar Fetch for CHMU / SHMU on Core 0 (only when actually due)
        else if (radarDue) {
          s_reqRadar = false;
          lastRadarFetch = now;
          s_radarBusy = true;
          int wantFrames = (curScr == SCREEN_METEO_I) ? CHMU_ANIM_MAX : 1;
          int n = (Settings_RadarSource() == RADAR_SRC_SHMU) ? SHMU_FetchAnim(wantFrames) : CHMU_FetchAnim(wantFrames);
          s_radarBusy = false;
          s_radarUpdated = true;
          if (n > 0) {
            s_radarFails = 0;
          } else {
            s_radarFails++;
          }
          lastTlsTime = millis();
        }
        // 3. ADS-B Aircraft Fetching (when on Planes or Tactical screen, or periodic background scan for emergency squawks, or when clock complication needs aircraft)
        else if ((planesActive && (now - lastAdsbFetch >= adsbPeriod)) ||
                 (clockNeedsAircraft && (now - lastAdsbFetch >= 8000)) ||
                 (!planesActive && Settings_SquawkAlert() && (now - lastAdsbFetch >= 25000)) ||
                 s_reqAdsb) {
          s_reqAdsb = false;
          lastAdsbFetch = now;
          double lat = planesActive ? s_targetLat : Settings_Lat();
          double lon = planesActive ? s_targetLon : Settings_Lon();
          float rng = planesActive ? s_targetRangeKm : (clockNeedsAircraft ? 25.0f : 250.0f);
          if (ADSB_Fetch(lat, lon, rng)) {
            s_adsbUpdated = true;
            s_adsbFails = 0;
          } else {
            s_adsbFails++;
          }
          lastTlsTime = millis();
        }
        // 4. Forecast & Air Quality (immediate on first boot / WiFi connect, then periodic)
        else {
          static bool s_firstForecastDone = false;
          const bool needAqNow = Settings_ScreenEnabled(SCREEN_FORECAST_I) && !AirQuality_Valid();
          if (!s_firstForecastDone ||
              (clockNeedsWeather && !Forecast_CurrentValid() && (now - lastForecastFetch >= 15000UL)) ||
              (curScr == SCREEN_FORECAST_I && (!Forecast_Valid() || !AirQuality_Valid()) && (now - lastForecastFetch >= 5000UL)) ||
              (needAqNow && (now - lastForecastFetch >= 15000UL)) ||
              (now - lastForecastFetch >= FORECAST_PERIOD_MS) || s_reqForecast) {
            if (!Forecast_Valid()) Forecast_Invalidate();
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
          // Priority for active ISS screen
          else if (curScr == SCREEN_ISS_I && (s_reqIss || (now - lastIssFetch >= ISS_PERIOD_ACTIVE_MS))) {
            if (Iss_Step()) {
              s_issUpdated = true;
              lastTlsTime = millis();
            }
            s_reqIss = false;
            lastIssFetch = now;
          }
          // 6. Finance Data (Markets, Crypto, Stocks, Commodities)
          else if ((Finance_IsBusy() && Finance_ReadyToStep()) || s_reqFinance ||
                   (Finance_LastUpdated() == 0 && (now - lastFinanceFetch >= 15000UL)) ||
                   (curScr == SCREEN_FINANCE_I && (now - lastFinanceFetch >= 30000UL)) ||
                   (clockNeedsFinance && (now - lastFinanceFetch >= 60000UL)) ||
                   (now - lastFinanceFetch >= FINANCE_PERIOD_MS)) {
            if (Finance_Step()) {
              s_financeUpdated = true;
              lastTlsTime = millis();
            }
            if (!Finance_IsBusy()) {
              s_reqFinance = false;
              lastFinanceFetch = now;
            }
          }
          // 7. ISS Orbit Tracker (background)
          else if (s_reqIss ||
                   (now - lastIssFetch >= (Settings_ScreenEnabled(SCREEN_ISS_I) ? ((curScr == SCREEN_ISS_I) ? ISS_PERIOD_ACTIVE_MS : ISS_PERIOD_BG_MS) : 60000UL))) {
            if (Iss_Step()) {
              s_issUpdated = true;
              lastTlsTime = millis();
            }
            s_reqIss = false;
            lastIssFetch = now;
          }
          // 8. YouTube Analytics
          else if (s_reqYouTube ||
                   (clockNeedsYouTube && (now - lastYouTubeFetch >= YOUTUBE_PERIOD_ACTIVE_MS)) ||
                   (now - lastYouTubeFetch >= (Settings_ScreenEnabled(SCREEN_YOUTUBE_I) ? ((curScr == SCREEN_YOUTUBE_I) ? YOUTUBE_PERIOD_ACTIVE_MS : YOUTUBE_PERIOD_BG_MS) : 3600000UL))) {
            if (YouTube_Step()) {
              s_youtubeUpdated = true;
              lastTlsTime = millis();
            }
            s_reqYouTube = false;
            lastYouTubeFetch = now;
          }
          // 9. Klipper / Moonraker 3D Printer (strictly ONLY when displayed on device screen)
          else if (curScr == SCREEN_KLIPPER_I &&
                   (s_reqKlipper ||
                    (now - lastKlipperFetch >= (Klipper_IsConnected() ? KLIPPER_PERIOD_ACTIVE_MS : KLIPPER_PERIOD_OFFLINE_MS)))) {
            if (Klipper_Step()) {
              s_klipperUpdated = true;
            }
            s_reqKlipper = false;
            lastKlipperFetch = now;
          }
          // 9. AI Pet Companion (Gemini LLM thoughts)
          else if (s_reqPetThought) {
            s_reqPetThought = false;
            if (PetBrain_Step()) {
              lastTlsTime = millis();
            }
          }
          // 10. Global Disaster Warning System (USGS Real-Time Earthquake & Tsunami Feeds)
          else if (DisasterAlert_Step()) {
            lastTlsTime = millis();
          }
        }
        // (s_core0NetBusy = false handled by NetBusyGuard destructor above)
      }
    }

    // Short cooperative yield for FreeRTOS scheduler on Core 0
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void Async_Begin() {
  if (!s_mtxSettings) s_mtxSettings = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxAdsb)     s_mtxAdsb     = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxRadar)    s_mtxRadar    = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxForecast) s_mtxForecast = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxFinance)  s_mtxFinance  = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxIss)      s_mtxIss      = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxKlipper)  s_mtxKlipper  = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxRoute)    s_mtxRoute    = xSemaphoreCreateRecursiveMutex();
  if (!s_mtxI2c)      s_mtxI2c      = xSemaphoreCreateRecursiveMutex();

  MapTiles_Init();

  // Create background network worker task on Core 0 with 16KB stack in internal SRAM
  // (Internal SRAM required because flash cache disable operations check stack sanity)
  // 16 KB gives headroom for peak TLS handshake + ArduinoJson + PNGdec call chains
  BaseType_t res = xTaskCreatePinnedToCore(
    asyncWorkerTask,
    "AsyncNetWorker",
    16384, // 16KB stack (TLS + JSON peak can reach 13-14KB on LX7)
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
