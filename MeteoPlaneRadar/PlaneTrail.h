// =============================================================================
//  MeteoPlaneRadar
//  PlaneTrail.h - Aircraft trajectory trails (breadcrumbs history).
//
//  Maintains a ring buffer of recent geographic positions for each active
//  aircraft to draw fading breadcrumb trails on radar screens.
//
//  Author:  Petr / chiptron.cz & Antigravity
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "ADSB.h"

#define TRAIL_MAX_POINTS  8     // number of historical breadcrumbs per aircraft
#define TRAIL_MAX_PLANES  ADSB_MAX
#define TRAIL_EXPIRY_MS   120000UL // 2 minutes

struct TrailPoint {
  float lat = 0;
  float lon = 0;
  float altFt = 0;
};

// Function signature for projecting (lat, lon) -> (screen_x, screen_y)
typedef void (*TrailProjectFn)(float lat, float lon, int* sx, int* sy);

// Update trail history with the newest list of active aircraft
void PlaneTrail_Update(const Aircraft* list, int count);

// Clear all historical trails (e.g. on range or location change)
void PlaneTrail_Clear();

// Draw the historical trajectory trail for a single aircraft
// projectFn converts lat/lon to display coordinates; baseCol is the current altitude color.
void PlaneTrail_Draw(const char* hex, TrailProjectFn projectFn, uint16_t baseCol, int currentSx, int currentSy);

// Get altitude color gradient based on barometric altitude in feet
uint16_t PlaneTrail_AltColor(float altFt, bool known);
