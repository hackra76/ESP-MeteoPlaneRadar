// =============================================================================
//  MeteoPlaneRadar
//  DisasterAlert.h - Global Real-Time Disaster Warning System
//
//  Tracks global and regional seismic activity, tsunami alerts, and local
//  physical ground vibrations:
//   - USGS Global Real-Time Seismic Feed (CDN-backed, all_hour & significant_week)
//   - NOAA / NTWC / PTWC Tsunami Warnings (flagged in USGS GeoJSON)
//   - Onboard QMI8658 6-axis IMU Hardware Seismometer (local ground tremors)
// =============================================================================
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

enum DisasterType : uint8_t {
  DISASTER_NONE = 0,
  DISASTER_EARTHQUAKE,
  DISASTER_TSUNAMI,
  DISASTER_LOCAL_TREMOR
};

enum DisasterSeverity : uint8_t {
  SEV_INFO = 1,
  SEV_ADVISORY,
  SEV_WARNING,
  SEV_EMERGENCY
};

struct DisasterEvent {
  char     id[24];
  char     title[64];
  char     place[48];
  float    mag;
  double   lat;
  double   lon;
  float    depthKm;
  uint32_t epochSec;
  bool     tsunami;
  float    distKm;
  float    bearingDeg;
  char     bearingStr[4];
  DisasterType     type;
  DisasterSeverity severity;
};

// Initialize Disaster Alert system and buffers
void DisasterAlert_Init();

// Background network polling step on Core 0 (AsyncCore)
bool DisasterAlert_Step();

// Fast periodic tick on Core 1 (UI & local IMU tremor sampling)
void DisasterAlert_Tick();

// Returns true if a high-priority alert is currently active
bool DisasterAlert_HasActiveAlert();

// Retrieve pointer to currently active highest-priority disaster event (or nullptr)
const DisasterEvent* DisasterAlert_GetActiveAlert();

// Returns true if a local physical tremor was detected by the hardware accelerometer
bool DisasterAlert_IsLocalTremorActive();

// Dismiss/acknowledge currently active alert audio & banner
void DisasterAlert_Dismiss();

// Check if currently active alert is still within newly set range perimeter
void DisasterAlert_CheckRange(uint16_t maxRngKm);

// Trigger a simulated disaster alert for testing/verification
void DisasterAlert_TriggerTest();

// Recent global disaster events count and access
int DisasterAlert_GetRecentCount();
const DisasterEvent* DisasterAlert_GetRecent(int idx);

// Format current state into JSON (for /api/disaster and WebConfig)
void DisasterAlert_ToJson(JsonObject& obj);

// Draw global disaster warning banner on active screen layer
void DisasterAlert_DrawOverlay();

// Handle touch tap on overlay (returns true if tap was handled/dismissed)
bool DisasterAlert_HandleTap(int x, int y);
