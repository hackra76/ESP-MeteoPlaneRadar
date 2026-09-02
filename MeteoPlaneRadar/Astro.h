// =============================================================================
//  MeteoPlaneRadar
//  Astro.h - Astronomical engine: Moon phase, solar position & twilight arcs.
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#pragma once
#include <Arduino.h>
#include <time.h>

enum MoonPhase : uint8_t {
  MOON_NEW = 0,
  MOON_WAXING_CRESCENT,
  MOON_FIRST_QUARTER,
  MOON_WAXING_GIBBOUS,
  MOON_FULL,
  MOON_WANING_GIBBOUS,
  MOON_LAST_QUARTER,
  MOON_WANING_CRESCENT
};

struct MoonInfo {
  float phase;          // 0.0 = new, 0.25 = 1st qtr, 0.5 = full, 0.75 = last qtr
  float illumination;   // 0.0% to 100.0%
  MoonPhase phaseEnum;
  const char* name;     // Localised phase name
};

struct SolarTimes {
  time_t sunrise;
  time_t sunset;
  time_t civilDawn;
  time_t civilDusk;
  time_t goldenHourStart;
  time_t goldenHourEnd;
  bool   valid;
};

// Calculate Moon phase and illumination for the given UNIX timestamp
MoonInfo Astro_GetMoon(time_t t);

// Calculate Solar sunrise/sunset/twilight times for given location and timestamp
SolarTimes Astro_GetSolar(double lat, double lon, time_t t);

// Draw shaded Moon Phase graphic icon into Arduino_GFX
void Astro_DrawMoonIcon(int cx, int cy, int r, float phase);

// Draw 24-Hour Solar Twilight Ring around the clock screen
void Astro_DrawSolarArc(int cx, int cy, int r, int thickness, double lat, double lon, time_t now);
