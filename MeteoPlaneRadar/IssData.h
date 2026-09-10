// =============================================================================
//  MeteoPlaneRadar
//  IssData.h - International Space Station (ISS) tracker & orbital mechanics.
//
//  Fetches realtime orbital position, altitude, velocity, and ground footprint
//  from WhereTheISS.at REST API without requiring any API keys.
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

struct IssData {
  float lat;            // degrees (-90 to +90)
  float lon;            // degrees (-180 to +180)
  float alt;            // altitude in km (~415 - 430 km)
  float velocity;       // velocity in km/h (~27600 km/h)
  float footprint;      // diameter in km (~4400 km)
  char  visibility[16]; // "daylight" or "eclipsed"
  float solarLat;       // solar sub-point latitude
  float solarLon;       // solar sub-point longitude
  unsigned long timestamp; // epoch seconds

  // Observer-relative metrics
  float distanceKm;     // distance to station in km
  float elevationDeg;   // elevation angle above horizon in degrees
  float azimuthDeg;     // azimuth from observer in degrees (0..360)
  bool  inRange;        // within ground visibility circle (elevation > 0)
  bool  isOverhead;     // elevation >= 45 deg

  // Pass prediction
  int   nextPassMinutes; // minutes until next rise (>0), or 0 if currently visible
  float nextPassMaxEl;   // max elevation of next pass in deg

  bool  valid;
  unsigned long lastUpdatedMs;
};

void Iss_Init();
bool Iss_Step();
void Iss_RequestFetch();
bool Iss_IsBusy();
unsigned long Iss_LastUpdated();
bool Iss_GetData(IssData* out);
const IssData* Iss_GetDataPtr();

// Returns ground track point (lat, lon) for offset dt_minutes (-45..+90 min)
void Iss_GetOrbitPoint(float dt_minutes, float* out_lat, float* out_lon);
