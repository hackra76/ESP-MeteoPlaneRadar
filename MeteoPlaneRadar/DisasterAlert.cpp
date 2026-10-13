// =============================================================================
//  MeteoPlaneRadar
//  DisasterAlert.cpp - Global Real-Time Disaster Warning System
// =============================================================================
#include "DisasterAlert.h"
#include "Config.h"
#include "Settings.h"
#include "Net.h"
#include "Buzzer.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "QMI8658.h"
#include <WiFi.h>
#include <math.h>

#define USGS_ALL_HOUR_URL    "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_hour.geojson"
#define USGS_SIGNIFICANT_URL "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/significant_week.geojson"

#define MAX_RECENT_EVENTS 10
#define ALERT_MAX_AGE_SEC   10800UL // 3 hours window for active quake alerts
#define TSUNAMI_MAX_AGE_SEC 21600UL // 6 hours window for active tsunami alerts

static DisasterEvent s_recentEvents[MAX_RECENT_EVENTS];
static int           s_recentCount = 0;
static DisasterEvent s_activeAlert;
static bool          s_hasActiveAlert = false;
static bool          s_alertDismissed = false;
static char          s_lastAlertId[24] = "";
static bool          s_needAlarm = false;

static unsigned long s_lastPollMs = 0;
static unsigned long s_lastSigPollMs = 0;
static unsigned long s_localTremorUntilMs = 0;

// IMU Seismometer filter state
static float         s_accelEma = 1.0f;
static int           s_shakeCount = 0;
static float         s_lastPeakDeltaG = 0.0f;

// Haversine distance in km
static float haversineKm(double lat1, double lon1, double lat2, double lon2) {
  const double r = 6371.0;
  const double dLat = (lat2 - lat1) * (M_PI / 180.0);
  const double dLon = (lon2 - lon1) * (M_PI / 180.0);
  const double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
                   cos(lat1 * (M_PI / 180.0)) * cos(lat2 * (M_PI / 180.0)) *
                   sin(dLon / 2.0) * sin(dLon / 2.0);
  const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return (float)(r * c);
}

// Great-circle compass bearing from device to epicenter
static float calcBearing(double lat1, double lon1, double lat2, double lon2) {
  double dLon = (lon2 - lon1) * (M_PI / 180.0);
  double rLat1 = lat1 * (M_PI / 180.0);
  double rLat2 = lat2 * (M_PI / 180.0);
  double y = sin(dLon) * cos(rLat2);
  double x = cos(rLat1) * sin(rLat2) - sin(rLat1) * cos(rLat2) * cos(dLon);
  double b = atan2(y, x) * (180.0 / M_PI);
  if (b < 0.0) b += 360.0;
  return (float)b;
}

static void getBearingStr(float deg, char* out, size_t cap) {
  static const char* const DIRS[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };
  int idx = (int)roundf(deg / 22.5f) % 16;
  if (idx < 0) idx += 16;
  snprintf(out, cap, "%s", DIRS[idx]);
}

void DisasterAlert_Init() {
  memset(s_recentEvents, 0, sizeof(s_recentEvents));
  memset(&s_activeAlert, 0, sizeof(s_activeAlert));
  s_recentCount = 0;
  s_hasActiveAlert = false;
  s_alertDismissed = false;
  s_lastAlertId[0] = '\0';
  s_needAlarm = false;
  s_accelEma = 1.0f;
  s_shakeCount = 0;
  s_lastPeakDeltaG = 0.0f;
}

static void evaluateEventSeverity(DisasterEvent& ev, double homeLat, double homeLon) {
  ev.distKm = haversineKm(homeLat, homeLon, ev.lat, ev.lon);
  ev.bearingDeg = calcBearing(homeLat, homeLon, ev.lat, ev.lon);
  getBearingStr(ev.bearingDeg, ev.bearingStr, sizeof(ev.bearingStr));

  if (ev.tsunami) {
    ev.type = DISASTER_TSUNAMI;
    ev.severity = SEV_EMERGENCY;
    return;
  }

  ev.type = DISASTER_EARTHQUAKE;
  if (ev.mag >= 7.0f) {
    ev.severity = (ev.distKm <= 1500.0f) ? SEV_EMERGENCY : SEV_WARNING;
  } else if (ev.mag >= 5.5f) {
    if (ev.distKm <= 300.0f) ev.severity = SEV_EMERGENCY;
    else if (ev.distKm <= 800.0f) ev.severity = SEV_WARNING;
    else ev.severity = SEV_ADVISORY;
  } else if (ev.mag >= 4.0f) {
    if (ev.distKm <= 150.0f) ev.severity = SEV_WARNING;
    else if (ev.distKm <= 400.0f) ev.severity = SEV_ADVISORY;
    else ev.severity = SEV_INFO;
  } else if (ev.mag >= 2.5f) {
    if (ev.distKm <= 60.0f) ev.severity = SEV_ADVISORY;
    else ev.severity = SEV_INFO;
  } else {
    ev.severity = SEV_INFO;
  }
}

static void parseUsgsFeed(const String& json) {
  if (json.length() < 50) return;

  JsonDocument filter;
  filter["features"][0]["properties"]["mag"] = true;
  filter["features"][0]["properties"]["place"] = true;
  filter["features"][0]["properties"]["time"] = true;
  filter["features"][0]["properties"]["title"] = true;
  filter["features"][0]["properties"]["tsunami"] = true;
  filter["features"][0]["geometry"]["coordinates"] = true;
  filter["features"][0]["id"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, DeserializationOption::Filter(filter));
  if (err) {
    Serial.printf("Disaster: JSON parse error: %s\n", err.c_str());
    return;
  }

  JsonArrayConst features = doc["features"].as<JsonArrayConst>();
  if (features.isNull() || features.size() == 0) return;

  const double homeLat = Settings_Lat();
  const double homeLon = Settings_Lon();
  time_t now = time(nullptr);

  for (JsonObjectConst f : features) {
    JsonObjectConst props = f["properties"];
    JsonArrayConst coords = f["geometry"]["coordinates"];
    if (props.isNull() || coords.isNull() || coords.size() < 2) continue;

    const char* id = f["id"] | "";
    if (!id || !*id) continue;

    // Check if already in recent list
    bool exists = false;
    for (int i = 0; i < s_recentCount; i++) {
      if (strcmp(s_recentEvents[i].id, id) == 0) {
        exists = true;
        break;
      }
    }

    DisasterEvent ev;
    memset(&ev, 0, sizeof(ev));
    snprintf(ev.id, sizeof(ev.id), "%s", id);
    snprintf(ev.title, sizeof(ev.title), "%s", props["title"] | "");
    snprintf(ev.place, sizeof(ev.place), "%s", props["place"] | "");
    ev.mag = props["mag"] | 0.0f;
    ev.tsunami = (props["tsunami"] | 0) != 0;
    
    uint64_t epochMs = props["time"] | 0ULL;
    ev.epochSec = (uint32_t)(epochMs / 1000ULL);

    ev.lon = coords[0] | 0.0;
    ev.lat = coords[1] | 0.0;
    ev.depthKm = (coords.size() >= 3) ? (coords[2].as<float>()) : 10.0f;

    evaluateEventSeverity(ev, homeLat, homeLon);

    // Insert into recent list (shift older down if full)
    if (!exists) {
      if (s_recentCount < MAX_RECENT_EVENTS) {
        s_recentEvents[s_recentCount++] = ev;
      } else {
        for (int k = MAX_RECENT_EVENTS - 1; k > 0; k--) {
          s_recentEvents[k] = s_recentEvents[k - 1];
        }
        s_recentEvents[0] = ev;
      }
    }

    // Check active alert candidate (fresh emergency within time window and range limit)
    uint32_t ageSec = (now > (time_t)ev.epochSec) ? (uint32_t)(now - ev.epochSec) : 0;
    uint32_t maxAge = ev.tsunami ? TSUNAMI_MAX_AGE_SEC : ALERT_MAX_AGE_SEC;
    uint16_t maxRng = Settings_DisasterRange();
    bool inRange = (maxRng == 0 || ev.distKm <= (float)maxRng);

    if ((ev.severity >= SEV_WARNING || ev.tsunami) && (ageSec <= maxAge) && inRange) {
      if (!s_hasActiveAlert || ev.severity > s_activeAlert.severity ||
          (ev.severity == s_activeAlert.severity && ev.epochSec > s_activeAlert.epochSec)) {
        
        bool isNew = (strcmp(s_lastAlertId, ev.id) != 0);
        s_activeAlert = ev;
        s_hasActiveAlert = true;
        if (isNew) {
          snprintf(s_lastAlertId, sizeof(s_lastAlertId), "%s", ev.id);
          s_alertDismissed = false;
          s_needAlarm = true;
          Serial.printf("Disaster: NEW ALERT! [%s] Mag=%.1f Tsunami=%d Dist=%.0fkm\n",
                        ev.title, ev.mag, ev.tsunami ? 1 : 0, ev.distKm);
        }
      }
    }
  }
}

bool DisasterAlert_Step() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (!Settings_DisasterAlert()) return false;

  unsigned long now = millis();
  // Poll all_hour feed every 60 seconds
  if (now - s_lastPollMs >= 60000UL || s_lastPollMs == 0) {
    s_lastPollMs = now;
    String body;
    if (Net_GetString(USGS_ALL_HOUR_URL, body, "Disaster")) {
      parseUsgsFeed(body);
    }
    return true;
  }

  // Poll significant_week feed every 10 minutes
  if (now - s_lastSigPollMs >= 600000UL || s_lastSigPollMs == 0) {
    s_lastSigPollMs = now;
    String body;
    if (Net_GetString(USGS_SIGNIFICANT_URL, body, "DisasterSig")) {
      parseUsgsFeed(body);
    }
    return true;
  }

  return false;
}

void DisasterAlert_Tick() {
  unsigned long now = millis();

  // 1. Hardware IMU Seismometer check (QMI8658)
  if (QMI8658_Available()) {
    QMI_Data d;
    QMI8658_GetData(&d);
    float gTot = sqrtf(d.ax * d.ax + d.ay * d.ay + d.az * d.az);
    s_accelEma = 0.92f * s_accelEma + 0.08f * gTot;
    float deltaG = fabsf(gTot - s_accelEma);

    if (deltaG > s_lastPeakDeltaG) s_lastPeakDeltaG = deltaG;

    // Detect sustained physical tremor (> 0.22g delta over 3+ consecutive samples)
    if (deltaG > 0.22f) {
      s_shakeCount++;
      if (s_shakeCount >= 3) {
        s_localTremorUntilMs = now + 15000UL;
        if (!s_hasActiveAlert) {
          s_hasActiveAlert = true;
          s_activeAlert.type = DISASTER_LOCAL_TREMOR;
          s_activeAlert.severity = SEV_WARNING;
          snprintf(s_activeAlert.title, sizeof(s_activeAlert.title), "LOCAL SEISMIC TREMOR");
          snprintf(s_activeAlert.place, sizeof(s_activeAlert.place), "Hardware Seismometer");
          s_activeAlert.mag = 0.0f;
          s_activeAlert.distKm = 0.0f;
          s_activeAlert.epochSec = (uint32_t)time(nullptr);
          s_alertDismissed = false;
          s_needAlarm = true;
          Serial.println("Disaster: Physical seismic tremor detected by onboard IMU!");
        }
      }
    } else {
      if (s_shakeCount > 0) s_shakeCount--;
    }
  }

  // Clear local tremor alert when timeout expires
  if (s_hasActiveAlert && s_activeAlert.type == DISASTER_LOCAL_TREMOR && now > s_localTremorUntilMs) {
    s_hasActiveAlert = false;
  }

  // 2. Trigger audio buzzer siren when new disaster is armed
  if (s_needAlarm) {
    s_needAlarm = false;
    if (Settings_DisasterAudio()) {
      Buzzer_Play(BEEP_EMERGENCY, true);
    }
    UI_InvalidateActiveScreen();
  }
}

bool DisasterAlert_HasActiveAlert() {
  return s_hasActiveAlert && !s_alertDismissed && Settings_DisasterAlert();
}

const DisasterEvent* DisasterAlert_GetActiveAlert() {
  if (!s_hasActiveAlert) return nullptr;
  return &s_activeAlert;
}

bool DisasterAlert_IsLocalTremorActive() {
  return (millis() < s_localTremorUntilMs);
}

void DisasterAlert_Dismiss() {
  s_alertDismissed = true;
  s_hasActiveAlert = false;
  Buzzer_Stop();
  UI_InvalidateActiveScreen();
}

void DisasterAlert_CheckRange(uint16_t maxRngKm) {
  if (s_hasActiveAlert && s_activeAlert.type != DISASTER_LOCAL_TREMOR && maxRngKm > 0) {
    if (s_activeAlert.distKm > (float)maxRngKm) {
      s_hasActiveAlert = false;
      Buzzer_Stop();
      UI_InvalidateActiveScreen();
    }
  }
}

void DisasterAlert_TriggerTest() {
  memset(&s_activeAlert, 0, sizeof(s_activeAlert));
  snprintf(s_activeAlert.id, sizeof(s_activeAlert.id), "test_%lu", millis());
  snprintf(s_activeAlert.title, sizeof(s_activeAlert.title), "M7.2 TEST WARNING");
  snprintf(s_activeAlert.place, sizeof(s_activeAlert.place), "Simulated Coastal Fault Zone");
  s_activeAlert.mag = 7.2f;

  uint16_t maxRng = Settings_DisasterRange();
  float simDist = 420.0f;
  if (maxRng > 0 && maxRng < 450) {
    simDist = (float)maxRng * 0.6f;
    if (simDist < 10.0f) simDist = 10.0f;
  }
  s_activeAlert.distKm = simDist;
  s_activeAlert.bearingDeg = 215.0f;
  snprintf(s_activeAlert.bearingStr, sizeof(s_activeAlert.bearingStr), "SW");
  s_activeAlert.tsunami = true;
  s_activeAlert.type = DISASTER_TSUNAMI;
  s_activeAlert.severity = SEV_WARNING;
  s_activeAlert.epochSec = (uint32_t)time(nullptr);

  s_hasActiveAlert = true;
  s_alertDismissed = false;
  s_needAlarm = true;

  bool exists = false;
  for (int i = 0; i < s_recentCount; i++) {
    if (strcmp(s_recentEvents[i].id, s_activeAlert.id) == 0) { exists = true; break; }
  }
  if (!exists) {
    if (s_recentCount < MAX_RECENT_EVENTS) {
      s_recentEvents[s_recentCount++] = s_activeAlert;
    } else {
      for (int k = MAX_RECENT_EVENTS - 1; k > 0; k--) {
        s_recentEvents[k] = s_recentEvents[k - 1];
      }
      s_recentEvents[0] = s_activeAlert;
    }
  }

  UI_InvalidateActiveScreen();
}

int DisasterAlert_GetRecentCount() {
  return s_recentCount;
}

const DisasterEvent* DisasterAlert_GetRecent(int idx) {
  if (idx < 0 || idx >= s_recentCount) return nullptr;
  return &s_recentEvents[idx];
}

void DisasterAlert_ToJson(JsonObject& obj) {
  obj["enabled"] = Settings_DisasterAlert();
  obj["audio"] = Settings_DisasterAudio();
  obj["maxRangeKm"] = Settings_DisasterRange();
  obj["hasAlert"] = DisasterAlert_HasActiveAlert();

  if (s_hasActiveAlert) {
    JsonObject act = obj["activeAlert"].to<JsonObject>();
    act["type"] = (s_activeAlert.type == DISASTER_TSUNAMI) ? "TSUNAMI" :
                  (s_activeAlert.type == DISASTER_LOCAL_TREMOR) ? "LOCAL_TREMOR" : "EARTHQUAKE";
    act["title"] = s_activeAlert.title;
    act["place"] = s_activeAlert.place;
    act["mag"] = s_activeAlert.mag;
    act["distKm"] = (int)roundf(s_activeAlert.distKm);
    act["bearing"] = s_activeAlert.bearingStr;
    act["tsunami"] = s_activeAlert.tsunami;
    act["severity"] = (int)s_activeAlert.severity;
    act["time"] = s_activeAlert.epochSec;
  }

  JsonObject imu = obj["seismometer"].to<JsonObject>();
  imu["available"] = QMI8658_Available();
  imu["peakDeltaG"] = (int)roundf(s_lastPeakDeltaG * 100.0f) / 100.0f;
  imu["shaking"] = DisasterAlert_IsLocalTremorActive();

  JsonArray rec = obj["recent"].to<JsonArray>();
  for (int i = 0; i < s_recentCount; i++) {
    JsonObject item = rec.add<JsonObject>();
    item["title"] = s_recentEvents[i].title;
    item["mag"] = s_recentEvents[i].mag;
    item["distKm"] = (int)roundf(s_recentEvents[i].distKm);
    item["bearing"] = s_recentEvents[i].bearingStr;
    item["tsunami"] = s_recentEvents[i].tsunami;
    item["time"] = s_recentEvents[i].epochSec;
  }
}

// Draw global high-visibility disaster alert HUD banner at the top of the display
void DisasterAlert_DrawOverlay() {
  if (!DisasterAlert_HasActiveAlert()) return;

  const DisasterEvent* al = &s_activeAlert;
  const int bw = 320;
  const int bh = 42;
  const int bx = (LCD_WIDTH - bw) / 2;
  const int by = 64;

  // Pulsing border effect (500ms flash)
  bool flash = ((millis() / 500UL) % 2) == 0;
  uint16_t bgCol = 0x3800; // Deep dark crimson
  uint16_t borderCol = flash ? C_RED : 0xF9C0; // Flashing bright red / yellow

  gfx->fillRoundRect(bx, by, bw, bh, 10, bgCol);
  gfx->drawRoundRect(bx, by, bw, bh, 10, borderCol);

  // Warning text formatting
  char header[48];
  if (al->type == DISASTER_TSUNAMI) {
    snprintf(header, sizeof(header), "! TSUNAMI WARNING !");
  } else if (al->type == DISASTER_LOCAL_TREMOR) {
    snprintf(header, sizeof(header), "! LOCAL SEISMIC SHAKE !");
  } else {
    snprintf(header, sizeof(header), "! EARTHQUAKE M%.1f !", al->mag);
  }

  UI_TextCentered(header, by + 5, flash ? C_WHITE : C_YELLOW, 2);

  // Subtext: Distance, bearing, time elapsed
  char sub[64];
  if (al->type == DISASTER_LOCAL_TREMOR) {
    snprintf(sub, sizeof(sub), "Desk Tremor Detected * Tap to Dismiss");
  } else {
    time_t now = time(nullptr);
    int ageMin = (now > (time_t)al->epochSec) ? (int)((now - al->epochSec) / 60) : 0;
    char ageStr[16];
    if (ageMin >= 60) {
      snprintf(ageStr, sizeof(ageStr), "%dh %dm ago", ageMin / 60, ageMin % 60);
    } else {
      snprintf(ageStr, sizeof(ageStr), "%dm ago", ageMin);
    }

    if (al->distKm > 2000.0f) {
      snprintf(sub, sizeof(sub), "Global: %s (%s)", al->place, ageStr);
    } else {
      snprintf(sub, sizeof(sub), "%.0f km %s * %s * Tap to Dismiss", al->distKm, al->bearingStr, ageStr);
    }
  }
  UI_TextCentered(sub, by + 26, 0xCE79, 1);
}

bool DisasterAlert_HandleTap(int x, int y) {
  if (!DisasterAlert_HasActiveAlert()) return false;
  // Banner occupies y = 60..110, x = 70..410
  if (y >= 58 && y <= 112 && x >= 70 && x <= 410) {
    DisasterAlert_Dismiss();
    return true;
  }
  return false;
}
