// =============================================================================
//  MeteoPlaneRadar
//  Daily aircraft flight statistics tracking implementation.
//
// =============================================================================
#include "FlightStats.h"
#include "Settings.h"
#include "Outside.h"
#include "Config.h"
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <algorithm>

#define MAX_SEEN_ICAO 4096

static const float STATS_RANGES[] = PLANE_RANGES_KM;
static const uint8_t NUM_STATS_RANGES = sizeof(STATS_RANGES) / sizeof(STATS_RANGES[0]);
static const uint8_t TOTAL_STATS_SCOPES = NUM_STATS_RANGES + 1;

struct ScopeStats {
  uint32_t* seenIcao = nullptr;
  uint32_t  uniqueCount = 0;
  float     maxDistKm = 0.0f;
  float     maxSpeedKt = 0.0f;
  char      maxSpeedCallsign[12] = "";
  float     maxAltFt = 0.0f;
  float     minAltFt = 999999.0f;
  uint32_t  totalSightings = 0;
};

static ScopeStats s_scopes[TOTAL_STATS_SCOPES];
static int        s_lastDay = -1;
static bool       s_statsDirty = false;
static unsigned long s_statsDirtyMs = 0;

static void FlightStats_Save();
static void FlightStats_Load();

static float haversineKm(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371.0f;
  float dLat = (lat2 - lat1) * 0.0174532925f;
  float dLon = (lon2 - lon1) * 0.0174532925f;
  float a = sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
            cosf(lat1 * 0.0174532925f) * cosf(lat2 * 0.0174532925f) *
            sinf(dLon * 0.5f) * sinf(dLon * 0.5f);
  return R * 2.0f * asinf(fminf(1.0f, sqrtf(a)));
}

uint8_t FlightStats_ScopeCount() {
  return TOTAL_STATS_SCOPES;
}

float FlightStats_ScopeRangeKm(uint8_t scope) {
  if (scope == 0 || scope >= TOTAL_STATS_SCOPES) return 0.0f;
  return STATS_RANGES[scope - 1];
}

void FlightStats_Init() {
  for (int s = 0; s < TOTAL_STATS_SCOPES; s++) {
    if (!s_scopes[s].seenIcao) {
      s_scopes[s].seenIcao = (uint32_t*)heap_caps_malloc(MAX_SEEN_ICAO * sizeof(uint32_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!s_scopes[s].seenIcao) {
        s_scopes[s].seenIcao = (uint32_t*)malloc(MAX_SEEN_ICAO * sizeof(uint32_t));
      }
    }
  }
  // Restore persisted 24-hour daily statistics from NVS
  FlightStats_Load();
}

static void FlightStats_Save() {
  Preferences prefs;
  if (!prefs.begin("fstats", false)) return;

  if (Outside_TimeValid()) {
    time_t now = time(nullptr);
    struct tm lt;
    localtime_r(&now, &lt);
    prefs.putInt("yday", lt.tm_yday);
    prefs.putInt("year", lt.tm_year);
  } else if (s_lastDay >= 0) {
    prefs.putInt("yday", s_lastDay);
  }

  for (int s = 0; s < TOTAL_STATS_SCOPES; s++) {
    char key[16];
    snprintf(key, sizeof(key), "u_%d", s);
    prefs.putUInt(key, s_scopes[s].uniqueCount);
    snprintf(key, sizeof(key), "tot_%d", s);
    prefs.putUInt(key, s_scopes[s].totalSightings);
    snprintf(key, sizeof(key), "dist_%d", s);
    prefs.putFloat(key, s_scopes[s].maxDistKm);
    snprintf(key, sizeof(key), "spd_%d", s);
    prefs.putFloat(key, s_scopes[s].maxSpeedKt);
    snprintf(key, sizeof(key), "altH_%d", s);
    prefs.putFloat(key, s_scopes[s].maxAltFt);
    snprintf(key, sizeof(key), "altL_%d", s);
    prefs.putFloat(key, s_scopes[s].minAltFt);
    snprintf(key, sizeof(key), "cs_%d", s);
    prefs.putString(key, s_scopes[s].maxSpeedCallsign);

    // Save up to 450 unique ICAOs per scope in NVS to preserve deduplication
    snprintf(key, sizeof(key), "seen_%d", s);
    uint32_t count = s_scopes[s].uniqueCount;
    if (count > 450) count = 450;
    if (count > 0 && s_scopes[s].seenIcao) {
      prefs.putBytes(key, s_scopes[s].seenIcao, count * sizeof(uint32_t));
    } else {
      prefs.remove(key);
    }
  }
  prefs.end();
}

static void FlightStats_Load() {
  Preferences prefs;
  if (!prefs.begin("fstats", true)) return;

  int savedDay = prefs.getInt("yday", -1);
  int savedYear = prefs.getInt("year", -1);

  if (Outside_TimeValid()) {
    time_t now = time(nullptr);
    struct tm lt;
    localtime_r(&now, &lt);
    if (savedDay != -1 && (savedDay != lt.tm_yday || savedYear != lt.tm_year)) {
      prefs.end();
      FlightStats_Reset(-1);
      s_lastDay = lt.tm_yday;
      return;
    }
    s_lastDay = lt.tm_yday;
  } else {
    s_lastDay = savedDay;
  }

  for (int s = 0; s < TOTAL_STATS_SCOPES; s++) {
    char key[16];
    snprintf(key, sizeof(key), "u_%d", s);
    s_scopes[s].uniqueCount = prefs.getUInt(key, 0);
    snprintf(key, sizeof(key), "tot_%d", s);
    s_scopes[s].totalSightings = prefs.getUInt(key, 0);
    snprintf(key, sizeof(key), "dist_%d", s);
    s_scopes[s].maxDistKm = prefs.getFloat(key, 0.0f);
    snprintf(key, sizeof(key), "spd_%d", s);
    s_scopes[s].maxSpeedKt = prefs.getFloat(key, 0.0f);
    snprintf(key, sizeof(key), "altH_%d", s);
    s_scopes[s].maxAltFt = prefs.getFloat(key, 0.0f);
    snprintf(key, sizeof(key), "altL_%d", s);
    s_scopes[s].minAltFt = prefs.getFloat(key, 999999.0f);
    snprintf(key, sizeof(key), "cs_%d", s);
    prefs.getString(key, s_scopes[s].maxSpeedCallsign, sizeof(s_scopes[s].maxSpeedCallsign));

    snprintf(key, sizeof(key), "seen_%d", s);
    size_t len = prefs.getBytesLength(key);
    if (len > 0 && len <= MAX_SEEN_ICAO * sizeof(uint32_t) && s_scopes[s].seenIcao) {
      prefs.getBytes(key, s_scopes[s].seenIcao, len);
    }
  }
  prefs.end();
}

void FlightStats_Reset(int scope) {
  int start = (scope >= 0 && scope < TOTAL_STATS_SCOPES) ? scope : 0;
  int end   = (scope >= 0 && scope < TOTAL_STATS_SCOPES) ? scope + 1 : TOTAL_STATS_SCOPES;

  for (int s = start; s < end; s++) {
    s_scopes[s].uniqueCount = 0;
    s_scopes[s].maxDistKm = 0.0f;
    s_scopes[s].maxSpeedKt = 0.0f;
    s_scopes[s].maxSpeedCallsign[0] = '\0';
    s_scopes[s].maxAltFt = 0.0f;
    s_scopes[s].minAltFt = 999999.0f;
    s_scopes[s].totalSightings = 0;
    if (s_scopes[s].seenIcao) {
      memset(s_scopes[s].seenIcao, 0, MAX_SEEN_ICAO * sizeof(uint32_t));
    }
  }
  FlightStats_Save();
  s_statsDirty = false;
}

void FlightStats_CheckMidnight() {
  if (!Outside_TimeValid()) return;
  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);

  if (s_lastDay == -1) {
    s_lastDay = lt.tm_yday;
    Preferences prefs;
    if (prefs.begin("fstats", true)) {
      int savedDay = prefs.getInt("yday", -1);
      int savedYear = prefs.getInt("year", -1);
      prefs.end();
      if (savedDay != -1 && (savedDay != lt.tm_yday || savedYear != lt.tm_year)) {
        FlightStats_Reset(-1);
      }
    }
    return;
  }

  // Day rollover at midnight
  if (lt.tm_yday != s_lastDay) {
    s_lastDay = lt.tm_yday;
    FlightStats_Reset(-1);
    Serial.printf("FlightStats: Midnight reset, starting new day %d\n", s_lastDay);
  }
}

void FlightStats_Tick() {
  const unsigned long now = millis();
  FlightStats_CheckMidnight();

  // Increased debounce to 15 minutes (900000ms) to prevent excessive NVS flash wear
  if (s_statsDirty && (now - s_statsDirtyMs >= 900000UL)) {
    s_statsDirty = false;
    FlightStats_Save();
  }
}

static bool registerIcao(int scopeIdx, uint32_t icao) {
  if (scopeIdx < 0 || scopeIdx >= TOTAL_STATS_SCOPES) return false;
  ScopeStats& sc = s_scopes[scopeIdx];
  if (icao == 0 || !sc.seenIcao) return false;
  if (sc.uniqueCount == 0) {
    sc.seenIcao[0] = icao;
    sc.uniqueCount = 1;
    return true;
  }

  // Binary search in sorted array
  uint32_t* it = std::lower_bound(sc.seenIcao, sc.seenIcao + sc.uniqueCount, icao);
  if (it != sc.seenIcao + sc.uniqueCount && *it == icao) {
    return false; // Already seen today in this scope
  }

  if (sc.uniqueCount < MAX_SEEN_ICAO) {
    size_t idx = it - sc.seenIcao;
    memmove(sc.seenIcao + idx + 1, sc.seenIcao + idx, (sc.uniqueCount - idx) * sizeof(uint32_t));
    sc.seenIcao[idx] = icao;
    sc.uniqueCount++;
    return true;
  }
  return false;
}

void FlightStats_Update(const Aircraft* list, int count) {
  if (!list || count <= 0) return;
  if (!s_scopes[0].seenIcao) FlightStats_Init();
  FlightStats_CheckMidnight();

  const float homeLat = (float)Settings_Lat();
  const float homeLon = (float)Settings_Lon();
  const bool hasHome = (homeLat != 0.0f || homeLon != 0.0f);

  for (int i = 0; i < count; i++) {
    const Aircraft& a = list[i];
    float d = -1.0f;
    if (hasHome && a.lat != 0.0f && a.lon != 0.0f) {
      d = haversineKm(homeLat, homeLon, a.lat, a.lon);
    }

    uint32_t icao = 0;
    if (a.hex[0]) {
      icao = (uint32_t)strtoul(a.hex, nullptr, 16);
    }

    // Update Scope 0 (ALL) and every zoom scope whose boundary contains the plane
    for (int s = 0; s < TOTAL_STATS_SCOPES; s++) {
      if (s > 0) {
        float maxKm = STATS_RANGES[s - 1];
        if (!hasHome || d < 0.0f || d > maxKm) {
          continue; // Plane is outside this zoom range
        }
      }

      ScopeStats& sc = s_scopes[s];
      sc.totalSightings++;

      if (icao != 0) {
        registerIcao(s, icao);
      }

      if (d >= 0.0f && d > sc.maxDistKm) {
        sc.maxDistKm = d;
      }

      if (a.gsKt > sc.maxSpeedKt && a.gsKt < 1500.0f) {
        sc.maxSpeedKt = a.gsKt;
        if (a.callsign[0]) {
          strncpy(sc.maxSpeedCallsign, a.callsign, sizeof(sc.maxSpeedCallsign) - 1);
          sc.maxSpeedCallsign[sizeof(sc.maxSpeedCallsign) - 1] = '\0';
        } else if (a.hex[0]) {
          strncpy(sc.maxSpeedCallsign, a.hex, sizeof(sc.maxSpeedCallsign) - 1);
          sc.maxSpeedCallsign[sizeof(sc.maxSpeedCallsign) - 1] = '\0';
        }
      }

      if (!a.onGround && a.altFt > 0.0f && a.altFt < 100000.0f) {
        if (a.altFt > sc.maxAltFt) sc.maxAltFt = a.altFt;
        if (a.altFt < sc.minAltFt) sc.minAltFt = a.altFt;
      }
    }
  }
  s_statsDirty = true;
  s_statsDirtyMs = millis();
}

static inline int resolveScope(int scope) {
  if (scope < 0 || scope >= TOTAL_STATS_SCOPES) {
    return (int)Settings_StatsScope();
  }
  return scope;
}

uint32_t FlightStats_TodayCount(int scope) {
  int s = resolveScope(scope);
  return s_scopes[s].uniqueCount;
}

float FlightStats_MaxDistKm(int scope) {
  int s = resolveScope(scope);
  return s_scopes[s].maxDistKm;
}

float FlightStats_MaxSpeedKt(int scope) {
  int s = resolveScope(scope);
  return s_scopes[s].maxSpeedKt;
}

const char* FlightStats_MaxSpeedCallsign(int scope) {
  int s = resolveScope(scope);
  return s_scopes[s].maxSpeedCallsign;
}

float FlightStats_MaxAltFt(int scope) {
  int s = resolveScope(scope);
  return s_scopes[s].maxAltFt;
}

float FlightStats_MinAltFt(int scope) {
  int s = resolveScope(scope);
  return (s_scopes[s].minAltFt >= 900000.0f) ? 0.0f : s_scopes[s].minAltFt;
}

uint32_t FlightStats_TotalSightings(int scope) {
  int s = resolveScope(scope);
  return s_scopes[s].totalSightings;
}

