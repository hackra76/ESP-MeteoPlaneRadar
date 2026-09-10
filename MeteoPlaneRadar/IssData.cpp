// =============================================================================
//  MeteoPlaneRadar
//  IssData.cpp - International Space Station (ISS) tracker & orbital mechanics.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1
// =============================================================================
#include "IssData.h"
#include "Net.h"
#include "AsyncCore.h"
#include "Settings.h"
#include "Buzzer.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <math.h>

static IssData s_data;
static bool s_busy = false;
static bool s_forceFetch = true;
static unsigned long s_lastFetch = 0;
static float s_prevLat = 0.0f;
static bool s_headingSouth = false;
static bool s_wasInRange = false;

static void computeObserverMetrics(float issLat, float issLon, float issAlt,
                                   float homeLat, float homeLon,
                                   float* outDistKm, float* outElevDeg, float* outAzDeg) {
  const float R_earth = 6371.0f;
  float phi1 = homeLat * (float)M_PI / 180.0f;
  float phi2 = issLat  * (float)M_PI / 180.0f;
  float dphi = (issLat - homeLat) * (float)M_PI / 180.0f;
  float dlam = (issLon - homeLon) * (float)M_PI / 180.0f;

  float a = sinf(dphi * 0.5f) * sinf(dphi * 0.5f) +
            cosf(phi1) * cosf(phi2) * sinf(dlam * 0.5f) * sinf(dlam * 0.5f);
  if (a > 1.0f) a = 1.0f;
  float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));

  float R_iss = R_earth + issAlt;
  float slantDist = sqrtf(R_earth * R_earth + R_iss * R_iss - 2.0f * R_earth * R_iss * cosf(c));
  if (slantDist < 1.0f) slantDist = 1.0f;
  *outDistKm = slantDist;

  float sin_c = sinf(c);
  if (sin_c < 1e-6f) {
    *outElevDeg = 90.0f;
  } else {
    float el_rad = atan2f(cosf(c) - (R_earth / R_iss), sin_c);
    *outElevDeg = el_rad * 180.0f / (float)M_PI;
  }

  float y = sinf(dlam) * cosf(phi2);
  float x = cosf(phi1) * sinf(phi2) - sinf(phi1) * cosf(phi2) * cosf(dlam);
  float az_rad = atan2f(y, x);
  float az_deg = az_rad * 180.0f / (float)M_PI;
  while (az_deg < 0.0f) az_deg += 360.0f;
  while (az_deg >= 360.0f) az_deg -= 360.0f;
  *outAzDeg = az_deg;
}

void Iss_GetOrbitPoint(float dt_minutes, float* out_lat, float* out_lon) {
  const float inc_rad = 0.90136f; // 51.643 deg inclination
  const float omega_orb = 2.0f * (float)M_PI / 92.9f;
  const float earth_rot_deg_min = 360.0f / 1436.0f;

  float cur_lat_rad = s_data.lat * (float)M_PI / 180.0f;
  float sin_u = sinf(cur_lat_rad) / sinf(inc_rad);
  if (sin_u > 1.0f) sin_u = 1.0f;
  if (sin_u < -1.0f) sin_u = -1.0f;

  float u = asinf(sin_u);
  if (s_headingSouth) {
    u = (float)M_PI - u;
  }

  float u_t = u + omega_orb * dt_minutes;
  float lat_t_rad = asinf(sinf(inc_rad) * sinf(u_t));
  float node_delta = atan2f(cosf(inc_rad) * sinf(u), cosf(u)) * 180.0f / (float)M_PI;
  float node_delta_t = atan2f(cosf(inc_rad) * sinf(u_t), cosf(u_t)) * 180.0f / (float)M_PI;
  float earth_rot = earth_rot_deg_min * dt_minutes;

  float lon_t = s_data.lon - node_delta + node_delta_t - earth_rot;
  while (lon_t > 180.0f) lon_t -= 360.0f;
  while (lon_t < -180.0f) lon_t += 360.0f;

  *out_lat = lat_t_rad * 180.0f / (float)M_PI;
  *out_lon = lon_t;
}

static void predictNextPass(float homeLat, float homeLon, int* outMin, float* outMaxEl) {
  *outMin = 0;
  *outMaxEl = 0.0f;

  if (s_data.inRange) {
    *outMin = 0;
    *outMaxEl = s_data.elevationDeg;
    return;
  }

  float bestEl = 0.0f;
  int riseMin = 0;
  bool inPass = false;

  for (int dt = 1; dt <= 720; dt += 2) {
    float p_lat = 0.0f, p_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &p_lat, &p_lon);
    float dKm = 0.0f, elDeg = 0.0f, azDeg = 0.0f;
    computeObserverMetrics(p_lat, p_lon, s_data.alt > 100.0f ? s_data.alt : 420.0f,
                           homeLat, homeLon, &dKm, &elDeg, &azDeg);

    if (elDeg > 0.0f) {
      if (!inPass) {
        inPass = true;
        riseMin = dt;
        bestEl = elDeg;
      } else if (elDeg > bestEl) {
        bestEl = elDeg;
      }
    } else if (inPass) {
      break;
    }
  }

  if (riseMin > 0) {
    *outMin = riseMin;
    *outMaxEl = bestEl;
  }
}

void Iss_Init() {
  memset(&s_data, 0, sizeof(s_data));
  s_data.alt = 420.0f;
  s_data.footprint = 4500.0f;
  strncpy(s_data.visibility, "daylight", sizeof(s_data.visibility));
  s_forceFetch = true;
}

void Iss_RequestFetch() {
  s_forceFetch = true;
}

bool Iss_IsBusy() {
  return s_busy;
}

unsigned long Iss_LastUpdated() {
  return s_data.lastUpdatedMs;
}

bool Iss_GetData(IssData* out) {
  if (!out) return false;
  Async_LockIss();
  *out = s_data;
  Async_UnlockIss();
  return s_data.valid;
}

const IssData* Iss_GetDataPtr() {
  return &s_data;
}

bool Iss_Step() {
  if (WiFi.status() != WL_CONNECTED) return false;

  s_busy = true;
  String body;
  if (!Net_GetString("https://api.wheretheiss.at/v1/satellites/25544", body, "ISS")) {
    s_busy = false;
    return false;
  }

  JsonDocument filter;
  filter["latitude"] = true;
  filter["longitude"] = true;
  filter["altitude"] = true;
  filter["velocity"] = true;
  filter["visibility"] = true;
  filter["footprint"] = true;
  filter["solar_lat"] = true;
  filter["solar_lon"] = true;
  filter["timestamp"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
  if (err) {
    Serial.printf("ISS: JSON parse error: %s\n", err.c_str());
    s_busy = false;
    return false;
  }

  float lat = doc["latitude"] | 0.0f;
  float lon = doc["longitude"] | 0.0f;
  float alt = doc["altitude"] | 420.0f;
  float vel = doc["velocity"] | 27600.0f;
  float fp  = doc["footprint"] | 4500.0f;
  const char* vis = doc["visibility"] | "daylight";
  float s_lat = doc["solar_lat"] | 0.0f;
  float s_lon = doc["solar_lon"] | 0.0f;
  unsigned long ts = doc["timestamp"] | 0UL;

  if (fabsf(s_prevLat) > 0.001f) {
    s_headingSouth = (lat < s_prevLat);
  }
  s_prevLat = lat;

  float homeLat = (float)Settings_Lat();
  float homeLon = (float)Settings_Lon();
  float distKm = 0.0f, elDeg = 0.0f, azDeg = 0.0f;
  computeObserverMetrics(lat, lon, alt, homeLat, homeLon, &distKm, &elDeg, &azDeg);

  Async_LockIss();
  s_data.lat = lat;
  s_data.lon = lon;
  s_data.alt = alt;
  s_data.velocity = vel;
  s_data.footprint = fp;
  strncpy(s_data.visibility, vis, sizeof(s_data.visibility) - 1);
  s_data.solarLat = s_lat;
  s_data.solarLon = s_lon;
  s_data.timestamp = ts;

  s_data.distanceKm = distKm;
  s_data.elevationDeg = elDeg;
  s_data.azimuthDeg = azDeg;
  s_data.inRange = (elDeg > 0.0f || distKm <= (fp * 0.5f));
  s_data.isOverhead = (elDeg >= 45.0f || distKm <= 500.0f);

  int nMin = 0;
  float nMaxEl = 0.0f;
  predictNextPass(homeLat, homeLon, &nMin, &nMaxEl);
  s_data.nextPassMinutes = nMin;
  s_data.nextPassMaxEl = nMaxEl;

  s_data.valid = true;
  s_data.lastUpdatedMs = millis();
  Async_UnlockIss();

  // Trigger overhead / in-range buzzer alert if configured
  if (s_data.inRange && !s_wasInRange) {
    if (Settings_IssAlert()) {
      Serial.println("ISS: Space Station entered visible range! Triggering alert.");
      Buzzer_Play(BEEP_SONAR_PING);
    }
  }
  s_wasInRange = s_data.inRange;

  s_lastFetch = millis();
  s_forceFetch = false;
  s_busy = false;
  return true;
}
