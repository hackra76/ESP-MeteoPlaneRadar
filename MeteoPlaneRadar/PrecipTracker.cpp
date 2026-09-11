// =============================================================================
//  MeteoPlaneRadar
//  PrecipTracker - Radar Nowcasting & Approaching Precipitation Detection.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
// =============================================================================
#include "PrecipTracker.h"
#include "Settings.h"
#include "Lang.h"
#include "Buzzer.h"
#include <math.h>

#define GRID_SIZE 32

static PrecipAlert s_alert;
static uint8_t s_grid0[GRID_SIZE][GRID_SIZE];
static uint8_t s_grid1[GRID_SIZE][GRID_SIZE];
static bool    s_hail1[GRID_SIZE][GRID_SIZE];
static bool s_hasGrid0 = false;
static time_t s_lastGridTime = 0;

void PrecipTracker_Init() {
  memset(&s_alert, 0, sizeof(s_alert));
  memset(s_grid0, 0, sizeof(s_grid0));
  memset(s_grid1, 0, sizeof(s_grid1));
  memset(s_hail1, 0, sizeof(s_hail1));
  s_hasGrid0 = false;
}

const PrecipAlert* PrecipTracker_GetAlert() {
  return &s_alert;
}

bool PrecipTracker_IsApproaching() {
  return (s_alert.status == PRECIP_STAT_APPROACHING);
}

bool PrecipTracker_IsCurrentlyActive() {
  return (s_alert.status == PRECIP_STAT_CURRENTLY_ACTIVE);
}

void PrecipTracker_DismissAlert() {
  s_alert.alertArmed = false;
}

static uint8_t extractIntensity(uint16_t col, bool* isHail) {
  if (isHail) *isHail = false;
  if (col == 0x0000) return 0;
  // Ignore SHMU / CHMU radar background grey/white colors
  if (col == 0xE71C || col == 0xD6DA || col == 0xE73C || col == 0xD69A || col == 0xFFFF || col == 0xC618) {
    return 0;
  }
  uint8_t r = (col >> 11) & 0x1F;
  uint8_t g = (col >> 5)  & 0x3F;
  uint8_t b = col & 0x1F;

  // Weight RGB channels to reflect meteorological radar reflectivity
  int val = (r * 4 + g * 2 + b * 4);
  if (val > 255) val = 255;

  // Detect intense convective core / hail (>50-55 dBZ):
  // True radar storm cores are represented by magenta / purple / deep violet:
  // - Magenta / purple: high R, high B, low G (e.g., r >= 24, b >= 20, g <= 18)
  // - Pure white core (>60 dBZ): maxed R, G, B with high overall intensity
  // Note: yellow/orange rain (high R, high G, low B) is standard moderate rain, NOT hail.
  if (isHail) {
    if ((r >= 24 && b >= 20 && g <= 18) || (val >= 245 && r >= 30 && b >= 28 && g >= 58)) {
      *isHail = true;
    }
  }

  return (uint8_t)val;
}

void PrecipTracker_ProcessFrames(const uint16_t* prevFrame, const uint16_t* curFrame,
                                int w, int h, float rangeKm, float dtMin,
                                double userLat, double userLon, float curTemp) {
  if (!curFrame || w <= 0 || h <= 0 || rangeKm <= 0.0f) return;
  if (dtMin <= 0.0f) dtMin = 5.0f;

  // Sample current frame into 32x32 grid with localized hail flags
  for (int gy = 0; gy < GRID_SIZE; gy++) {
    int py = (gy * h) / GRID_SIZE;
    for (int gx = 0; gx < GRID_SIZE; gx++) {
      int px = (gx * w) / GRID_SIZE;
      uint16_t c0 = curFrame[py * w + px];
      s_grid1[gy][gx] = extractIntensity(c0, &s_hail1[gy][gx]);
    }
  }

  // If prevFrame provided directly, sample it into grid0; otherwise use stored previous grid
  if (prevFrame) {
    for (int gy = 0; gy < GRID_SIZE; gy++) {
      int py = (gy * h) / GRID_SIZE;
      for (int gx = 0; gx < GRID_SIZE; gx++) {
        int px = (gx * w) / GRID_SIZE;
        s_grid0[gy][gx] = extractIntensity(prevFrame[py * w + px], nullptr);
      }
    }
    s_hasGrid0 = true;
  }

  time_t now = time(nullptr);
  s_alert.lastUpdate = now;

  // Check if precipitation is currently falling directly at user's location (center: 15..16)
  int centerIntensity = 0;
  bool centerHail = false;
  for (int cy = 15; cy <= 16; cy++) {
    for (int cx = 15; cx <= 16; cx++) {
      if (s_grid1[cy][cx] > centerIntensity) {
        centerIntensity = s_grid1[cy][cx];
      }
      if (s_hail1[cy][cx]) {
        centerHail = true;
      }
    }
  }

  if (centerIntensity > 32) {
    s_alert.status = PRECIP_STAT_CURRENTLY_ACTIVE;
    s_alert.distKm = 0.0f;
    s_alert.etaMin = 0;
    if (centerHail || centerIntensity >= 235) {
      s_alert.type = PRECIP_HAIL_STORM;
    } else if (curTemp <= 1.0f) {
      s_alert.type = PRECIP_SNOW;
    } else if (curTemp <= 3.0f) {
      s_alert.type = PRECIP_SLEET;
    } else {
      s_alert.type = PRECIP_RAIN;
    }
    // Save current grid as previous for next cycle
    memcpy(s_grid0, s_grid1, sizeof(s_grid0));
    s_hasGrid0 = true;
    return;
  }

  // If we don't have a previous frame yet, store and wait for the next radar tick
  if (!s_hasGrid0) {
    memcpy(s_grid0, s_grid1, sizeof(s_grid0));
    s_hasGrid0 = true;
    s_alert.status = PRECIP_STAT_CLEAR;
    return;
  }

  // Total precipitation echoes count in current frame
  int activeEchoCount = 0;
  for (int gy = 0; gy < GRID_SIZE; gy++) {
    for (int gx = 0; gx < GRID_SIZE; gx++) {
      if (s_grid1[gy][gx] > 20) activeEchoCount++;
    }
  }

  if (activeEchoCount < 5) {
    s_alert.status = PRECIP_STAT_CLEAR;
    s_alert.type = PRECIP_NONE;
    memcpy(s_grid0, s_grid1, sizeof(s_grid0));
    return;
  }

  // Compute motion vector via 2D spatial cross-correlation (TREC)
  int bestDx = 0, bestDy = 0;
  int64_t bestScore = 0;

  for (int dy = -5; dy <= 5; dy++) {
    for (int dx = -5; dx <= 5; dx++) {
      int64_t score = 0;
      int matchCount = 0;
      for (int y = 5; y < GRID_SIZE - 5; y++) {
        int y2 = y + dy;
        for (int x = 5; x < GRID_SIZE - 5; x++) {
          int x2 = x + dx;
          int v0 = s_grid0[y][x];
          int v1 = s_grid1[y2][x2];
          if (v0 > 15 || v1 > 15) {
            score += (int64_t)v0 * v1;
            matchCount++;
          }
        }
      }
      if (matchCount >= 8 && score > bestScore) {
        bestScore = score;
        bestDx = dx;
        bestDy = dy;
      }
    }
  }

  // Convert grid displacement to real-world velocity (km/h)
  const float kmPerCell = (2.0f * rangeKm) / (float)GRID_SIZE;
  float vx = (float)bestDx * kmPerCell * (60.0f / dtMin);
  float vy = (float)bestDy * kmPerCell * (60.0f / dtMin);
  float speedKmh = sqrtf(vx * vx + vy * vy);

  // Bearing: Direction FROM which the weather is coming
  int bearingDeg = 0;
  if (speedKmh > 1.0f) {
    float ang = atan2f(-vx, vy) * 57.29578f;
    if (ang < 0.0f) ang += 360.0f;
    bearingDeg = (int)roundf(ang);
  }

  s_alert.speedKmh = speedKmh;
  s_alert.bearingDeg = bearingDeg;

  // Scan all cells in grid1 to find cells heading towards the center (15.5, 15.5)
  const float centerG = 15.5f;
  bool foundApproaching = false;
  bool approachingHail = false;
  float minEta = 999.0f;
  float closestDist = 999.0f;
  int peakIntensity = 0;

  if (speedKmh >= 5.0f) {
    float ux = vx / speedKmh;
    float uy = vy / speedKmh;
    const float impactRadiusKm = 15.0f;

    for (int gy = 0; gy < GRID_SIZE; gy++) {
      for (int gx = 0; gx < GRID_SIZE; gx++) {
        uint8_t intVal = s_grid1[gy][gx];
        if (intVal <= 20) continue;

        // Vector from cell to user (km)
        float dxKm = (centerG - (float)gx) * kmPerCell;
        float dyKm = (centerG - (float)gy) * kmPerCell;
        float distKm = sqrtf(dxKm * dxKm + dyKm * dyKm);

        if (distKm > 110.0f) continue;

        // Distance along cloud motion vector towards user
        float sParallel = dxKm * ux + dyKm * uy;
        if (sParallel <= 1.0f) {
          // Cloud is moving AWAY or already behind the user -> skip!
          continue;
        }

        // Perpendicular miss distance
        float dPerp = sqrtf(fmaxf(0.0f, distKm * distKm - sParallel * sParallel));
        if (dPerp <= impactRadiusKm) {
          float eta = (sParallel / speedKmh) * 60.0f;
          if (eta >= 1.0f && eta <= 75.0f) {
            foundApproaching = true;
            if (eta < minEta) {
              minEta = eta;
              closestDist = distKm;
            }
            if (intVal > peakIntensity) peakIntensity = intVal;
            if (s_hail1[gy][gx]) approachingHail = true;
          }
        }
      }
    }
  }

  if (foundApproaching) {
    bool wasApproaching = (s_alert.status == PRECIP_STAT_APPROACHING);
    s_alert.status = PRECIP_STAT_APPROACHING;
    s_alert.etaMin = (int)roundf(minEta);
    s_alert.distKm = closestDist;

    if (approachingHail || peakIntensity >= 235) {
      s_alert.type = PRECIP_HAIL_STORM;
    } else if (curTemp <= 1.0f) {
      s_alert.type = PRECIP_SNOW;
    } else if (curTemp <= 3.0f) {
      s_alert.type = PRECIP_SLEET;
    } else {
      s_alert.type = PRECIP_RAIN;
    }

    if (!wasApproaching) {
      s_alert.alertArmed = true;
      if (Settings_BuzzerEnabled()) {
        Buzzer_Play(BEEP_CLICK); // Or special beep
      }
    }
  } else {
    // Precipitation exists in the region, but does NOT head towards the user
    s_alert.status = PRECIP_STAT_PASSING_BY;
    s_alert.etaMin = 0;
  }

  // Copy grid1 to grid0 for the next frame correlation
  memcpy(s_grid0, s_grid1, sizeof(s_grid0));
}

const char* PrecipTracker_GetBearingStr(int deg) {
  static const char* N8_EN[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
  static const char* N8_SK[8] = { "S", "SV", "V", "JV", "J", "JZ", "Z", "SZ" };
  int idx = ((deg + 22) / 45) % 8;
  return (Lang_Get() == LANG_EN) ? N8_EN[idx] : N8_SK[idx];
}

const char* PrecipTracker_GetStatusText(char* buf, size_t maxLen) {
  if (!buf || maxLen == 0) return "";
  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  if (s_alert.status == PRECIP_STAT_CURRENTLY_ACTIVE) {
    if (s_alert.type == PRECIP_SNOW) {
      snprintf(buf, maxLen, isEn ? "Snowing now" : (isSk ? "Prebieha snezenie" : "Snezi"));
    } else if (s_alert.type == PRECIP_HAIL_STORM) {
      snprintf(buf, maxLen, isEn ? "Storm / Hail now" : (isSk ? "Burka / Krupy" : "Boure / Kroupy"));
    } else {
      snprintf(buf, maxLen, isEn ? "Raining now" : (isSk ? "Prebieha dazd" : "Prsi"));
    }
    return buf;
  }

  if (s_alert.status == PRECIP_STAT_APPROACHING) {
    const char* typeStr = "Rain";
    if (s_alert.type == PRECIP_SNOW)       typeStr = isEn ? "Snow" : (isSk ? "Snezenie" : "Snezeni");
    else if (s_alert.type == PRECIP_SLEET) typeStr = isEn ? "Sleet" : (isSk ? "Dazd so snehom" : "Dest se snehem");
    else if (s_alert.type == PRECIP_HAIL_STORM) typeStr = isEn ? "Hail/Storm" : (isSk ? "Krupy/Burka" : "Kroupy/Boure");
    else                                   typeStr = isEn ? "Rain" : (isSk ? "Dazd" : "Dest");

    const char* bStr = PrecipTracker_GetBearingStr(s_alert.bearingDeg);
    snprintf(buf, maxLen, isEn ? "%s in ~%d min (%s)" : (isSk ? "%s o ~%d min (%s)" : "%s za ~%d min (%s)"),
             typeStr, s_alert.etaMin, bStr);
    return buf;
  }

  if (s_alert.status == PRECIP_STAT_PASSING_BY) {
    snprintf(buf, maxLen, isEn ? "Passing by" : (isSk ? "Zrazky v okoli" : "Srazky v okoli"));
    return buf;
  }

  snprintf(buf, maxLen, isEn ? "No precipitation" : (isSk ? "Bez zrazok" : "Bez srazek"));
  return buf;
}
