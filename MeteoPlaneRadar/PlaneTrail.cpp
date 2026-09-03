// =============================================================================
//  MeteoPlaneRadar
//  PlaneTrail.cpp - Aircraft trajectory trails (breadcrumbs history) and colors.
//
//  Author:  Petr / chiptron.cz & Antigravity
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "PlaneTrail.h"
#include "Display_ST7701.h"
#include "UI.h"
#include <Arduino_GFX_Library.h>
#include <string.h>
#include <math.h>
#include "esp_heap_caps.h"

extern Arduino_GFX* gfx;

struct TrailEntry {
  char       hex[8] = "";
  uint8_t    count = 0;
  uint8_t    head = 0;   // index of next write
  TrailPoint points[TRAIL_MAX_POINTS];
  unsigned long lastSeenMs = 0;
};

static TrailEntry* s_trails = nullptr;

static bool ensureTrails() {
  if (!s_trails) {
    s_trails = (TrailEntry*)heap_caps_malloc(sizeof(TrailEntry) * TRAIL_MAX_PLANES, MALLOC_CAP_SPIRAM);
    if (!s_trails) s_trails = (TrailEntry*)malloc(sizeof(TrailEntry) * TRAIL_MAX_PLANES);
    if (s_trails) {
      for (int i = 0; i < TRAIL_MAX_PLANES; i++) s_trails[i] = TrailEntry();
    }
  }
  return s_trails != nullptr;
}

// Scale an RGB565 color towards dark/black
static uint16_t dimColor(uint16_t c, uint8_t num, uint8_t den) {
  if (den == 0) return 0;
  uint16_t r = (c >> 11) & 0x1F;
  uint16_t g = (c >> 5) & 0x3F;
  uint16_t b = c & 0x1F;
  r = (uint16_t)((uint32_t)r * num / den);
  g = (uint16_t)((uint32_t)g * num / den);
  b = (uint16_t)((uint32_t)b * num / den);
  return (uint16_t)((r << 11) | (g << 5) | b);
}

static TrailEntry* findTrail(const char* hex) {
  if (!hex || !hex[0] || !ensureTrails()) return nullptr;
  for (int i = 0; i < TRAIL_MAX_PLANES; i++) {
    if (s_trails[i].hex[0] && strcasecmp(s_trails[i].hex, hex) == 0) {
      return &s_trails[i];
    }
  }
  return nullptr;
}

static TrailEntry* allocTrail(const char* hex) {
  if (!ensureTrails()) return nullptr;
  unsigned long now = millis();
  TrailEntry* oldest = nullptr;
  unsigned long oldestTime = 0xFFFFFFFF;

  // 1. Look for free or expired slot
  for (int i = 0; i < TRAIL_MAX_PLANES; i++) {
    if (!s_trails[i].hex[0] || (now - s_trails[i].lastSeenMs > TRAIL_EXPIRY_MS)) {
      s_trails[i] = TrailEntry();
      strncpy(s_trails[i].hex, hex, sizeof(s_trails[i].hex) - 1);
      s_trails[i].hex[sizeof(s_trails[i].hex) - 1] = '\0';
      s_trails[i].lastSeenMs = now;
      return &s_trails[i];
    }
    if (s_trails[i].lastSeenMs < oldestTime) {
      oldestTime = s_trails[i].lastSeenMs;
      oldest = &s_trails[i];
    }
  }

  // 2. Reuse oldest slot
  if (oldest) {
    *oldest = TrailEntry();
    strncpy(oldest->hex, hex, sizeof(oldest->hex) - 1);
    oldest->hex[sizeof(oldest->hex) - 1] = '\0';
    oldest->lastSeenMs = now;
    return oldest;
  }

  return nullptr;
}

void PlaneTrail_Clear() {
  if (!s_trails) return;
  for (int i = 0; i < TRAIL_MAX_PLANES; i++) {
    s_trails[i] = TrailEntry();
  }
}

void PlaneTrail_Update(const Aircraft* list, int count) {
  if (!list || count <= 0 || !ensureTrails()) return;
  unsigned long now = millis();

  for (int i = 0; i < count; i++) {
    if (list[i].onGround || !list[i].hex[0]) continue;

    TrailEntry* t = findTrail(list[i].hex);
    if (!t) {
      t = allocTrail(list[i].hex);
    }
    if (!t) continue;

    t->lastSeenMs = now;

    // Check if position changed enough to warrant a new point (~30-50m)
    bool shouldAdd = true;
    if (t->count > 0) {
      int lastIdx = (t->head + TRAIL_MAX_POINTS - 1) % TRAIL_MAX_POINTS;
      float dLat = fabsf(list[i].lat - t->points[lastIdx].lat);
      float dLon = fabsf(list[i].lon - t->points[lastIdx].lon);
      if (dLat < 0.0003f && dLon < 0.0004f) {
        shouldAdd = false; // plane hasn't moved significantly
      }
    }

    if (shouldAdd) {
      t->points[t->head].lat = list[i].lat;
      t->points[t->head].lon = list[i].lon;
      t->points[t->head].altFt = list[i].altFt;
      t->head = (t->head + 1) % TRAIL_MAX_POINTS;
      if (t->count < TRAIL_MAX_POINTS) t->count++;
    }
  }
}

void PlaneTrail_Draw(const char* hex, TrailProjectFn projectFn, uint16_t baseCol, int currentSx, int currentSy) {
  if (!hex || !hex[0] || !projectFn || !gfx) return;
  TrailEntry* t = findTrail(hex);
  if (!t || t->count < 2) return;

  const int R_CX = LCD_WIDTH / 2;
  const int R_CY = LCD_HEIGHT / 2;
  const long R_MAX_SQ = (long)(LCD_WIDTH / 2 - 4) * (long)(LCD_WIDTH / 2 - 4);

  // Traverse from oldest point to newest
  int prevX = -9999, prevY = -9999;
  int startOffset = (t->head + TRAIL_MAX_POINTS - t->count) % TRAIL_MAX_POINTS;

  for (int i = 0; i < t->count; i++) {
    int idx = (startOffset + i) % TRAIL_MAX_POINTS;
    int sx, sy;
    projectFn(t->points[idx].lat, t->points[idx].lon, &sx, &sy);

    long ddx = sx - R_CX, ddy = sy - R_CY;
    bool inBounds = (ddx * ddx + ddy * ddy <= R_MAX_SQ);

    if (inBounds) {
      // Fade trail color: older points are dimmer (e.g. 25% -> 100%)
      uint8_t factor = (uint8_t)(4 + (i * 12) / t->count); // 4..16 out of 16
      uint16_t segCol = dimColor(baseCol, factor, 16);

      // Draw dot
      if (i < t->count - 1) {
        gfx->fillCircle(sx, sy, (i < 3) ? 1 : 2, segCol);
      }

      // Draw connecting trail line segment
      if (prevX != -9999 && prevY != -9999) {
        gfx->drawLine(prevX, prevY, sx, sy, segCol);
      }
    }

    prevX = inBounds ? sx : -9999;
    prevY = inBounds ? sy : -9999;
  }

  // Connect last point to current plane position
  if (prevX != -9999 && prevY != -9999) {
    long cddx = currentSx - R_CX, cddy = currentSy - R_CY;
    if (cddx * cddx + cddy * cddy <= R_MAX_SQ) {
      uint16_t headCol = dimColor(baseCol, 14, 16);
      gfx->drawLine(prevX, prevY, currentSx, currentSy, headCol);
    }
  }
}

// -----------------------------------------------------------------------------
//  5-Tier Rich Altitude Gradient Palette
// -----------------------------------------------------------------------------
uint16_t PlaneTrail_AltColor(float altFt, bool known) {
  if (!known) return C_GRAY;
  float km = altFt * 0.0003048f; // feet to km

  // < 2.0 km (< 6,500 ft): Low altitude / approach / light aircraft -> Emerald Green
  if (km < 2.0f)  return 0x2FE6; // Bright Mint / Emerald Green

  // 2.0 - 5.0 km (6,500 - 16,500 ft): Regional / Climbing -> Golden Amber / Yellow
  if (km < 5.0f)  return 0xFDE0; // Warm Yellow / Gold

  // 5.0 - 9.0 km (16,500 - 30,000 ft): Lower cruise -> Electric Sky Blue / Cyan
  if (km < 9.0f)  return 0x07FF; // Cyan / Sky Blue

  // 9.0 - 12.0 km (30,000 - 40,000 ft): Long-haul Jetway Cruise -> Royal Blue
  if (km < 12.0f) return 0x3BDF; // Royal Blue

  // >= 12.0 km (> 40,000 ft): High-altitude FL400+ / Supersonic -> Electric Magenta
  return 0xF81F; // Magenta / Violet
}
