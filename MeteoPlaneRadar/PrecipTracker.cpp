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

void PrecipTracker_Reset() {
  memset(&s_alert, 0, sizeof(s_alert));
  s_alert.status = PRECIP_STAT_CLEAR;
  s_alert.type = PRECIP_NONE;
  s_alert.distKm = 0.0f;
  s_alert.etaMin = 0;
  s_alert.speedKmh = 0.0f;
  s_alert.bearingDeg = 0;
  s_alert.alertArmed = false;
  s_hasGrid0 = false;
}

void PrecipTracker_Init() {
  PrecipTracker_Reset();
  memset(s_grid0, 0, sizeof(s_grid0));
  memset(s_grid1, 0, sizeof(s_grid1));
  memset(s_hail1, 0, sizeof(s_hail1));
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
  uint8_t g31 = g >> 1;

  // Meteorological radar echoes are vividly colored (cyan, blue, green, yellow, orange, red, magenta).
  // Discard all grayscale, neutral, border, river, and desaturated background pixels.
  uint8_t maxC = r;
  if (g31 > maxC) maxC = g31;
  if (b > maxC) maxC = b;

  uint8_t minC = r;
  if (g31 < minC) minC = g31;
  if (b < minC) minC = b;

  if ((maxC - minC) < 5) {
    return 0;
  }

  // Weight RGB channels to reflect meteorological radar reflectivity
  int val = (r * 4 + g * 2 + b * 4);
  if (val > 255) val = 255;

  // Detect intense convective core / hail (>50-55 dBZ):
  // True radar storm cores are represented by magenta / purple / deep violet:
  // - Magenta / purple: high R, high B, low G (e.g., r >= 24, b >= 20, g <= 18)
  if (isHail) {
    if (r >= 24 && b >= 20 && g <= 18) {
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

  // Check if precipitation is currently falling directly at user's location (tight 3 km physical radius)
  int centerIntensity = 0;
  bool centerHail = false;
  int cx0 = w / 2;
  int cy0 = h / 2;
  float pxPerKm = (float)(w / 2) / rangeKm;
  int rPx = (int)ceilf(3.0f * pxPerKm); // 3 km radius around user GPS coordinates
  if (rPx < 1) rPx = 1;
  if (rPx > w / 4) rPx = w / 4;

  int sampleCount = 0;
  int activeEchoCountInRadius = 0;

  for (int dy = -rPx; dy <= rPx; dy++) {
    int py = cy0 + dy;
    if (py < 0 || py >= h) continue;
    for (int dx = -rPx; dx <= rPx; dx++) {
      if (dx * dx + dy * dy > rPx * rPx) continue;
      int px = cx0 + dx;
      if (px < 0 || px >= w) continue;

      bool isH = false;
      uint8_t intVal = extractIntensity(curFrame[py * w + px], &isH);
      sampleCount++;
      if (intVal > centerIntensity) {
        centerIntensity = intVal;
      }
      if (isH) {
        centerHail = true;
      }
      if (intVal >= 55) { // Moderate rain threshold (~25+ dBZ)
        activeEchoCountInRadius++;
      }
    }
  }

  bool isRainingLocally = false;
  if (sampleCount > 0) {
    if (sampleCount <= 4) {
      if (activeEchoCountInRadius >= 2 && (centerIntensity >= 160 || centerHail)) {
        isRainingLocally = true;
      }
    } else {
      if (activeEchoCountInRadius >= 2 && (activeEchoCountInRadius * 100 / sampleCount) >= 10 && (centerIntensity >= 160 || centerHail)) {
        isRainingLocally = true; // Severe convective core or hail directly over location
      } else if (activeEchoCountInRadius >= 3 && (activeEchoCountInRadius * 100 / sampleCount) >= 20 && centerIntensity >= 55) {
        isRainingLocally = true; // Substantial rain coverage directly over location
      }
    }
  }

  if (isRainingLocally) {
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
  int64_t scores[11][11] = {0};
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
      if (matchCount >= 8) {
        scores[dy + 5][dx + 5] = score;
        if (score > bestScore) {
          bestScore = score;
          bestDx = dx;
          bestDy = dy;
        }
      }
    }
  }

  float fDx = (float)bestDx;
  float fDy = (float)bestDy;

  // Sub-pixel peak interpolation (Quadratic fitting) for smooth ETAs
  if (bestDx > -5 && bestDx < 5 && bestDy > -5 && bestDy < 5 && bestScore > 0) {
      int64_t s0 = scores[bestDy + 5][bestDx + 5];
      
      int64_t smx = scores[bestDy + 5][bestDx - 1 + 5];
      int64_t spx = scores[bestDy + 5][bestDx + 1 + 5];
      float denomX = 2.0f * (float)(smx - 2LL * s0 + spx);
      if (denomX != 0.0f) fDx += (float)(smx - spx) / denomX;

      int64_t smy = scores[bestDy - 1 + 5][bestDx + 5];
      int64_t spy = scores[bestDy + 1 + 5][bestDx + 5];
      float denomY = 2.0f * (float)(smy - 2LL * s0 + spy);
      if (denomY != 0.0f) fDy += (float)(smy - spy) / denomY;
  }

  // Convert grid displacement to real-world velocity (km/h)
  const float kmPerCell = (2.0f * rangeKm) / (float)GRID_SIZE;
  float vx = fDx * kmPerCell * (60.0f / dtMin);
  float vy = fDy * kmPerCell * (60.0f / dtMin);
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
    const float impactRadiusKm = 6.0f; // Tight corridor: must be tracking directly towards user

    for (int gy = 0; gy < GRID_SIZE; gy++) {
      for (int gx = 0; gx < GRID_SIZE; gx++) {
        uint8_t intVal = s_grid1[gy][gx];
        if (intVal < 50) continue; // Filter out faint cloud moisture and ground clutter (< 25 dBZ)

        // Vector from cell to user (km)
        float dxKm = (centerG - (float)gx) * kmPerCell;
        float dyKm = (centerG - (float)gy) * kmPerCell;
        float distKm = sqrtf(dxKm * dxKm + dyKm * dyKm);

        if (distKm > 60.0f) continue; // Focus on cells within realistic imminent reach

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
          if (eta >= 1.0f && eta <= 35.0f) { // Imminent: within 35 minutes
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
