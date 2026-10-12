// =============================================================================
//  MeteoPlaneRadar
//  Day / night brightness. See NightMode.h.
//
// =============================================================================
#include "NightMode.h"
#include "Settings.h"
#include "Forecast.h"
#include "Outside.h"
#include "Display_ST7701.h"
#include <time.h>

static bool s_applied = false;
static bool s_stateKnown = false;
static unsigned long s_wakeUntil = 0;
static bool s_wasUltra = false;
static bool s_idle = false;

bool NightMode_IsUltraNightActive() {
  return Settings_IsNight() && Settings_UltraNight() && (millis() >= s_wakeUntil);
}

void NightMode_WakeTemporary(uint32_t ms) {
  s_wakeUntil = millis() + ms;
  NightMode_Apply();
}

void NightMode_Apply() {
  if (NightMode_IsUltraNightActive()) {
    Set_Backlight(2); // 2% deep sleep-friendly level
  } else if (s_idle) {
    Set_Backlight(10); // 10% auto-dimming level
  } else {
    Set_Backlight(Settings_Backlight());
  }
  s_applied = true;
  s_wasUltra = NightMode_IsUltraNightActive();
}

void NightMode_SetIdle(bool idle) {
  if (s_idle != idle) {
    s_idle = idle;
    NightMode_Apply();
  }
}

void NightMode_Toggle() {
  if (Settings_NightAuto()) return;
  Settings_SetNight(!Settings_IsNight());
  NightMode_Apply();
}

// Astronomical sunrise/sunset calculation (NOAA algorithm)
static bool calculateSolarTimes(double latDeg, double lonDeg, time_t epochUtc, time_t* outRiseUtc, time_t* outSetUtc) {
  struct tm tm;
  gmtime_r(&epochUtc, &tm);
  int yday = tm.tm_yday; // 0-365
  int hour = tm.tm_hour;
  double gamma = 2.0 * M_PI / 365.0 * (yday + (hour - 12.0) / 24.0);
  double eqtime = 229.18 * (0.000075 + 0.001868 * cos(gamma) - 0.032077 * sin(gamma)
                           - 0.014615 * cos(2 * gamma) - 0.040849 * sin(2 * gamma));
  double decl = 0.006918 - 0.399912 * cos(gamma) + 0.070257 * sin(gamma)
              - 0.006758 * cos(2 * gamma) + 0.000907 * sin(2 * gamma)
              - 0.002697 * cos(3 * gamma) + 0.00148 * sin(3 * gamma);
  double latRad = latDeg * M_PI / 180.0;
  double zenithRad = 90.833 * M_PI / 180.0;
  double cosHA = (cos(zenithRad) / (cos(latRad) * cos(decl))) - (tan(latRad) * tan(decl));
  if (cosHA > 1.0 || cosHA < -1.0) return false;
  double haDeg = acos(cosHA) * 180.0 / M_PI;
  double solarNoonMin = 720.0 - 4.0 * lonDeg - eqtime;
  double riseMin = solarNoonMin - haDeg * 4.0;
  double setMin  = solarNoonMin + haDeg * 4.0;
  time_t midnightUtc = epochUtc - (epochUtc % 86400);
  if (outRiseUtc) *outRiseUtc = midnightUtc + (time_t)(riseMin * 60.0);
  if (outSetUtc)  *outSetUtc  = midnightUtc + (time_t)(setMin  * 60.0);
  return true;
}

void NightMode_Tick() {
  // Check if temporary wake-up timer expired
  if (s_wakeUntil != 0 && millis() >= s_wakeUntil) {
    s_wakeUntil = 0;
    NightMode_Apply();
  }

  // Detect ultra-night state toggle
  if (NightMode_IsUltraNightActive() != s_wasUltra) {
    NightMode_Apply();
  }

  // Push the initial brightness once, even before anything else is known.
  if (!s_applied) NightMode_Apply();

  if (!Settings_NightAuto()) return;

  static unsigned long s_lastSolarCheck = 0;
  if (millis() - s_lastSolarCheck < 2000 && s_stateKnown) return;
  s_lastSolarCheck = millis();

  time_t now = time(nullptr);
  if (now < 1700000000) return; // Wait until clock has valid epoch time (RTC or NTP/HTTP)

  time_t rise = 0, set = 0;
  if (!Forecast_SunTimes(&rise, &set)) {
    // If weather forecast has not provided sunrise/sunset yet, compute directly from location
    if (Settings_HasLocation()) {
      calculateSolarTimes(Settings_Lat(), Settings_Lon(), now, &rise, &set);
    }
  }
  if (!rise || !set) return;

  const long off = (long)Settings_NightOffsetMin() * 60L;
  const time_t nightEnds   = rise + off;     // dawn, shifted later by the offset
  const time_t nightStarts = set  - off;     // dusk, shifted earlier

  bool night = (now < nightEnds || now >= nightStarts);

  if (!s_stateKnown || night != Settings_IsNight()) {
    s_stateKnown = true;
    Settings_SetNight(night);
    NightMode_Apply();
    Serial.printf("[NightMode] Solar state: %s (brightness %u%%)\n", night ? "NIGHT" : "DAY",
                  (unsigned)Settings_Backlight());
  }
}
