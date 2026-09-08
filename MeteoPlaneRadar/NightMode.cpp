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
static bool s_lastState = false;
static unsigned long s_wakeUntil = 0;
static bool s_wasUltra = false;

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
  } else {
    Set_Backlight(Settings_Backlight());
  }
  s_applied = true;
  s_wasUltra = NightMode_IsUltraNightActive();
}

void NightMode_Toggle() {
  if (Settings_NightAuto()) return;
  Settings_SetNight(!Settings_IsNight());
  NightMode_Apply();
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
  if (!Outside_TimeValid()) return;          // no clock yet - nothing to compare

  time_t rise = 0, set = 0;
  if (!Forecast_SunTimes(&rise, &set)) return;

  const long off = (long)Settings_NightOffsetMin() * 60L;
  const time_t nightEnds   = rise + off;     // dawn, shifted later by the offset
  const time_t nightStarts = set  - off;     // dusk, shifted earlier

  time_t now = time(nullptr);

  // Note on the day boundary: the sun times we hold are for the day of the last
  // fetch. Just after midnight they are yesterday's, so both events are already
  // in the past and the test below still says "night" - which is correct. The
  // next refresh (half hourly) replaces them with today's.
  bool night = (now < nightEnds) || (now >= nightStarts);

  if (night != s_lastState || !s_applied) {
    s_lastState = night;
    Settings_SetNight(night);
    NightMode_Apply();
    Serial.printf("Mode: %s (brightness %u%%)\n", night ? "night" : "day",
                  (unsigned)Settings_Backlight());
  }
}
