// =============================================================================
//  MeteoPlaneRadar
//  Map underlay: European country outlines and cities.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "EuBorder.h"
#include "EuMapData.h"
#include "CzCitiesData.h"   // Czech cities - they replace the European ones
#include "SkCitiesData.h"   // Slovak cities - they replace the European ones
#include "UI.h"
#include "Layout.h"
#include "Settings.h"
#include <string.h>
#include <math.h>

void EuBorder_Draw(ProjectFn project, uint16_t color,
                   float lat0, float lat1, float lon0, float lon1) {
  // Quantize visible window bounding box to integer degrees * 100
  int16_t qLon0 = (int16_t)roundf(lon0 * 100.0f);
  int16_t qLon1 = (int16_t)roundf(lon1 * 100.0f);
  int16_t qLat0 = (int16_t)roundf(lat0 * 100.0f);
  int16_t qLat1 = (int16_t)roundf(lat1 * 100.0f);

  for (uint16_t a = 0; a < GLOBAL_ARC_COUNT; a++) {
    const GlobalArcBox& box = GLOBAL_ARC_BOXES[a];

    // Ring/arc-level rejection: fast integer comparison discards >98% of the world in microseconds
    if (box.lonMax100 < qLon0 || box.lonMin100 > qLon1) continue;
    if (box.latMax100 < qLat0 || box.latMin100 > qLat1) continue;

    uint32_t start = box.startIdx;
    uint32_t end   = start + box.count;

    int   prevX = 0, prevY = 0;
    bool  prevIn = false;
    bool  first = true;

    for (uint32_t i = start; i < end; i++) {
      float lon = globalPtLon(GLOBAL_BORDER_PTS[i][0]);
      float lat = globalPtLat(GLOBAL_BORDER_PTS[i][1]);
      bool  in  = (lat >= lat0 && lat <= lat1 && lon >= lon0 && lon <= lon1);

      int sx, sy;
      project(lat, lon, &sx, &sy);

      if (!first && (in || prevIn)) {
        gfx->drawLine(prevX, prevY, sx, sy, color);
      }

      prevX = sx; prevY = sy;
      prevIn = in;
      first = false;
    }
  }
}

static bool haveCzechCity(float lat, float lon) {
  const float TOL = 0.05f;                 // about 5 km
  for (int i = 0; i < CZ_CITY_COUNT; i++) {
    if (fabsf(CZ_CITIES[i].lat - lat) <= TOL &&
        fabsf(CZ_CITIES[i].lon - lon) <= TOL) return true;
  }
  return false;
}

static bool haveSlovakCity(float lat, float lon) {
  const float TOL = 0.05f;                 // about 5 km
  for (int i = 0; i < SK_CITY_COUNT; i++) {
    if (fabsf(SK_CITIES[i].lat - lat) <= TOL &&
        fabsf(SK_CITIES[i].lon - lon) <= TOL) return true;
  }
  return false;
}

// --- Label collision ---------------------------------------------------------
static void drawOneCity(const EuCity& c, ProjectFn project, int cx, int cy,
                        long r2, uint16_t dotColor, uint16_t textColor,
                        bool showFull, uint8_t tier,
                        float lat0, float lat1, float lon0, float lon1);

void EuBorder_DrawCities(ProjectFn project, int cx, int cy, int radius,
                         uint16_t dotColor, uint16_t textColor,
                         bool showFull, uint8_t maxTier,
                         float lat0, float lat1, float lon0, float lon1) {
  long r2 = (long)radius * radius;

  // Tier by tier, biggest first - whoever claims the space first keeps it.
  for (uint8_t tier = 1; tier <= maxTier; tier++) {
    for (int i = 0; i < EU_CITY_COUNT; i++) {
      const EuCity& c = EU_CITIES[i];
      if (c.lat >= CZ_BOX_LAT0 && c.lat <= CZ_BOX_LAT1 &&
          c.lon >= CZ_BOX_LON0 && c.lon <= CZ_BOX_LON1 &&
          haveCzechCity(c.lat, c.lon)) continue;
      if (c.lat >= SK_BOX_LAT0 && c.lat <= SK_BOX_LAT1 &&
          c.lon >= SK_BOX_LON0 && c.lon <= SK_BOX_LON1 &&
          haveSlovakCity(c.lat, c.lon)) continue;
      drawOneCity(c, project, cx, cy, r2, dotColor, textColor,
                  showFull, tier, lat0, lat1, lon0, lon1);
    }
    for (int i = 0; i < CZ_CITY_COUNT; i++)
      drawOneCity(CZ_CITIES[i], project, cx, cy, r2, dotColor, textColor,
                  showFull, tier, lat0, lat1, lon0, lon1);
    for (int i = 0; i < SK_CITY_COUNT; i++)
      drawOneCity(SK_CITIES[i], project, cx, cy, r2, dotColor, textColor,
                  showFull, tier, lat0, lat1, lon0, lon1);
  }
}

static void drawOneCity(const EuCity& c, ProjectFn project, int cx, int cy,
                        long r2, uint16_t dotColor, uint16_t textColor,
                        bool showFull, uint8_t tier,
                        float lat0, float lat1, float lon0, float lon1) {
  {

    // This pass draws exactly one tier - see the loop in the caller.
    if (c.tier != tier) return;

    // Geographic cull before projecting.
    if (c.lat < lat0 || c.lat > lat1 || c.lon < lon0 || c.lon > lon1) return;

    int sx, sy;
    project(c.lat, c.lon, &sx, &sy);

    // Skip anything outside the display circle.
    long dx = sx - cx, dy = sy - cy;
    if (dx * dx + dy * dy > r2) return;

    // Where the label goes: to the right of the dot, or to the left when that
    // would spill out of the circle.
    const char* label = showFull ? c.name : c.abbr;
    int tw = Layout_TextW(label, 1);
    if (tw <= 0) tw = strlen(label) * 6;
    int ty = sy - 4;
    int tx = sx + 9;
    if ((long)(tx + tw - cx) * (tx + tw - cx) + dy * dy > r2) tx = sx - 9 - tw;

    gfx->fillCircle(sx, sy, 3, dotColor);
    if (Settings_ShowLegends()) {
      // Check collision: prevent city labels from colliding with each other or chrome
      if (Layout_Claim(tx - 2, ty - 2, tw + 4, 12)) {
        UI_Text(label, tx, ty, textColor, 1);
      }
    }
  }
}
