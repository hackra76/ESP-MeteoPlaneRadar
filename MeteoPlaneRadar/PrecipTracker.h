// =============================================================================
//  MeteoPlaneRadar
//  PrecipTracker - Radar Nowcasting & Approaching Precipitation Detection.
//
//  Tracks motion vectors of radar echoes (TREC - Tracking Radar Echoes by
//  Correlation) across consecutive 5-minute radar frames. Determines whether
//  rain, snow or hail is heading directly towards the user's location, and
//  calculates Estimated Time of Arrival (ETA), speed, and distance.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
// =============================================================================
#pragma once
#include <Arduino.h>
#include <time.h>

enum PrecipType : uint8_t {
  PRECIP_NONE = 0,
  PRECIP_RAIN,          // Rain (Dážď)
  PRECIP_SNOW,          // Snow (Sneženie, temp <= 1°C)
  PRECIP_SLEET,         // Sleet / Rain & Snow (Dážď so snehom, 1°C < temp <= 3°C)
  PRECIP_HAIL_STORM     // Severe convective storm / Hail (Krúpy / prudká búrka, > 50 dBZ)
};

enum PrecipStatus : uint8_t {
  PRECIP_STAT_CLEAR = 0,        // No precipitation in proximity
  PRECIP_STAT_APPROACHING,      // Precipitation is heading directly towards user!
  PRECIP_STAT_PASSING_BY,       // Precipitation in area, but moving away or missing user
  PRECIP_STAT_CURRENTLY_ACTIVE  // Precipitation is currently active at user's location
};

struct PrecipAlert {
  PrecipStatus status = PRECIP_STAT_CLEAR;
  PrecipType   type   = PRECIP_NONE;
  int          etaMin = 0;        // Minutes until arrival (e.g. 25)
  float        distKm = 0.0f;     // Distance to precipitation front (km)
  float        speedKmh = 0.0f;   // Front movement speed (km/h)
  int          bearingDeg = 0;    // Compass heading from which rain is coming (0-360)
  int          intensity = 0;     // 1=light, 2=moderate, 3=heavy/severe
  time_t       lastUpdate = 0;    // Timestamp of last calculation
  bool         alertArmed = false;// True if a new alert hasn't been acknowledged
};

// Initialize module
void PrecipTracker_Init();

// Feed two consecutive radar frames (or downsampled intensity grids) for analysis.
// prevFrame & curFrame: RGB565 buffers or nullptr
// w, h: buffer dimensions
// rangeKm: radar coverage radius in km
// dtMin: time delta between frames in minutes (typically 5 or 10)
// userLat, userLon: current location coordinates
// curTemp: current outside temperature from forecast
void PrecipTracker_ProcessFrames(const uint16_t* prevFrame, const uint16_t* curFrame,
                                int w, int h, float rangeKm, float dtMin,
                                double userLat, double userLon, float curTemp);

// Get current precipitation status and alert info
const PrecipAlert* PrecipTracker_GetAlert();

// Returns true if precipitation is actively approaching user's location
bool PrecipTracker_IsApproaching();

// Returns true if it's currently raining/snowing at user's location
bool PrecipTracker_IsCurrentlyActive();

// Acknowledge buzzer alert
void PrecipTracker_DismissAlert();

// Short localized text description of current alert for UI widgets
const char* PrecipTracker_GetStatusText(char* buf, size_t maxLen);

// Compass direction string (e.g. "SW", "JZ", "SZ")
const char* PrecipTracker_GetBearingStr(int deg);
