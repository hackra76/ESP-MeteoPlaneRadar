// =============================================================================
//  MeteoPlaneRadar
//  Daily aircraft flight statistics tracking implementation.
//
// =============================================================================
#include "FlightStats.h"
#include "Settings.h"
#include "Outside.h"
#include <esp_heap_caps.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <algorithm>

#define MAX_SEEN_ICAO 4096

static uint32_t* s_seenIcao = nullptr;
static uint32_t  s_uniqueCount = 0;
static float     s_maxDistKm = 0.0f;
static float     s_maxSpeedKt = 0.0f;
static char      s_maxSpeedCallsign[12] = "";
static float     s_maxAltFt = 0.0f;
static float     s_minAltFt = 999999.0f;
static uint32_t  s_totalSightings = 0;
static int       s_lastDay = -1;

static float haversineKm(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371.0f;
  float dLat = (lat2 - lat1) * 0.0174532925f;
  float dLon = (lon2 - lon1) * 0.0174532925f;
  float a = sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
            cosf(lat1 * 0.0174532925f) * cosf(lat2 * 0.0174532925f) *
            sinf(dLon * 0.5f) * sinf(dLon * 0.5f);
  return R * 2.0f * asinf(fminf(1.0f, sqrtf(a)));
}

void FlightStats_Init() {
  if (!s_seenIcao) {
    s_seenIcao = (uint32_t*)heap_caps_malloc(MAX_SEEN_ICAO * sizeof(uint32_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_seenIcao) {
      s_seenIcao = (uint32_t*)malloc(MAX_SEEN_ICAO * sizeof(uint32_t));
    }
  }
  FlightStats_Reset();
}

void FlightStats_Reset() {
  s_uniqueCount = 0;
  s_maxDistKm = 0.0f;
  s_maxSpeedKt = 0.0f;
  s_maxSpeedCallsign[0] = '\0';
  s_maxAltFt = 0.0f;
  s_minAltFt = 999999.0f;
  s_totalSightings = 0;
  if (s_seenIcao) {
    memset(s_seenIcao, 0, MAX_SEEN_ICAO * sizeof(uint32_t));
  }
}

void FlightStats_CheckMidnight() {
  if (!Outside_TimeValid()) return;
  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);

  if (s_lastDay == -1) {
    s_lastDay = lt.tm_yday;
    return;
  }

  // Day rollover at midnight
  if (lt.tm_yday != s_lastDay) {
    s_lastDay = lt.tm_yday;
    FlightStats_Reset();
    Serial.printf("FlightStats: Midnight reset, starting new day %d\n", s_lastDay);
  }
}

static bool registerIcao(uint32_t icao) {
  if (icao == 0 || !s_seenIcao) return false;
  if (s_uniqueCount == 0) {
    s_seenIcao[0] = icao;
    s_uniqueCount = 1;
    return true;
  }

  // Binary search in sorted array
  uint32_t* it = std::lower_bound(s_seenIcao, s_seenIcao + s_uniqueCount, icao);
  if (it != s_seenIcao + s_uniqueCount && *it == icao) {
    return false; // Already seen today
  }

  if (s_uniqueCount < MAX_SEEN_ICAO) {
    size_t idx = it - s_seenIcao;
    memmove(s_seenIcao + idx + 1, s_seenIcao + idx, (s_uniqueCount - idx) * sizeof(uint32_t));
    s_seenIcao[idx] = icao;
    s_uniqueCount++;
    return true;
  }
  return false;
}

void FlightStats_Update(const Aircraft* list, int count) {
  if (!list || count <= 0) return;
  if (!s_seenIcao) FlightStats_Init();
  FlightStats_CheckMidnight();

  const float homeLat = (float)Settings_Lat();
  const float homeLon = (float)Settings_Lon();
  const bool hasHome = (homeLat != 0.0f || homeLon != 0.0f);

  for (int i = 0; i < count; i++) {
    const Aircraft& a = list[i];
    s_totalSightings++;

    // Register ICAO
    if (a.hex[0]) {
      uint32_t icao = (uint32_t)strtoul(a.hex, nullptr, 16);
      registerIcao(icao);
    }

    // Distance
    if (hasHome && a.lat != 0.0f && a.lon != 0.0f) {
      float d = haversineKm(homeLat, homeLon, a.lat, a.lon);
      if (d > s_maxDistKm) s_maxDistKm = d;
    }

    // Ground Speed
    if (a.gsKt > s_maxSpeedKt && a.gsKt < 1500.0f) {
      s_maxSpeedKt = a.gsKt;
      if (a.callsign[0]) {
        strncpy(s_maxSpeedCallsign, a.callsign, sizeof(s_maxSpeedCallsign) - 1);
      } else if (a.hex[0]) {
        strncpy(s_maxSpeedCallsign, a.hex, sizeof(s_maxSpeedCallsign) - 1);
      }
    }

    // Altitude
    if (!a.onGround && a.altFt > 0.0f && a.altFt < 100000.0f) {
      if (a.altFt > s_maxAltFt) s_maxAltFt = a.altFt;
      if (a.altFt < s_minAltFt) s_minAltFt = a.altFt;
    }
  }
}

uint32_t FlightStats_TodayCount() { return s_uniqueCount; }
float    FlightStats_MaxDistKm()  { return s_maxDistKm; }
float    FlightStats_MaxSpeedKt() { return s_maxSpeedKt; }
const char* FlightStats_MaxSpeedCallsign() { return s_maxSpeedCallsign; }
float    FlightStats_MaxAltFt()   { return s_maxAltFt; }
float    FlightStats_MinAltFt()   { return (s_minAltFt >= 900000.0f) ? 0.0f : s_minAltFt; }
uint32_t FlightStats_TotalSightings() { return s_totalSightings; }
