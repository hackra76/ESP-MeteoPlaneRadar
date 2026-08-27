// =============================================================================
//  MeteoPlaneRadar - clock (from HTTP Date) + outside temperature (Open-Meteo).
//  See Outside.h for why there is no NTP client here.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Outside.h"
#include "TimeUtil.h"
#include "Config.h"
#include "Net.h"
#include "Settings.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

// --- Clock ------------------------------------------------------------------
static bool s_timeOk = false;

bool Outside_TimeValid() { return s_timeOk; }

static int monthFromName(const char* m) {
  static const char* N[12] = { "Jan","Feb","Mar","Apr","May","Jun",
                               "Jul","Aug","Sep","Oct","Nov","Dec" };
  for (int i = 0; i < 12; i++) if (strncmp(m, N[i], 3) == 0) return i + 1;
  return 0;
}

void Outside_NoteHttpDate(const char* date) {
  if (!date || !*date) return;
  // RFC 7231 preferred form: "Sun, 09 Aug 2026 20:00:56 GMT".
  const char* p = strchr(date, ' ');
  if (!p) return;
  p++;
  int D = 0, Y = 0, hh = 0, mm = 0, ss = 0;
  char mon[4] = {0};
  if (sscanf(p, "%d %3s %d %d:%d:%d", &D, mon, &Y, &hh, &mm, &ss) != 6) return;
  int Mo = monthFromName(mon);
  if (Mo == 0 || D < 1 || D > 31 || Y < 2025 || hh > 23 || mm > 59 || ss > 60) return;

  time_t utc = TimeUtil_UtcToEpoch(Y, Mo, D, hh, mm, ss);
  struct timeval tv = { .tv_sec = utc, .tv_usec = 0 };
  settimeofday(&tv, nullptr);

  if (!s_timeOk) {
    s_timeOk = true;
    struct tm lt; localtime_r(&utc, &lt);
    Serial.printf("Cas nastaven z hlavicky Date: %02d:%02d\n", lt.tm_hour, lt.tm_min);
  }
}

// --- Outside temperature ----------------------------------------------------
static bool  s_tempOk   = false;
static float s_tempC    = 0.0f;
static unsigned long s_lastTry = 0;
static bool  s_everTried = false;

// When the forecast last handed us a temperature. While that keeps arriving
// there is nothing for this module to do - the forecast asks the same server
// for the same value, so fetching it again would just be a second connection
// for no new information.
static unsigned long s_extAt = 0;
static bool          s_extOk = false;
#define OUTSIDE_EXT_FRESH_MS (FORECAST_PERIOD_MS + 300000UL)   // 30 min + slack

void Outside_NoteTemp(float degC) {
  s_tempC  = degC;
  s_tempOk = true;
  s_extOk  = true;
  s_extAt  = millis();
}

static bool fetchTemp() {
  if (WiFi.status() != WL_CONNECTED) return false;

  char url[160];
  snprintf(url, sizeof(url),
           "%s?latitude=%.4f&longitude=%.4f&current=temperature_2m",
           OUTSIDE_TEMP_URL, Settings_Lat(), Settings_Lon());

  // Same path as every other text response: heap guard, handshake timeout,
  // chunked decoding and the body cap all live in Net_GetString().
  String body;
  if (!Net_GetString(url, body, "TEPLOTA")) return false;

  JsonDocument filter;
  filter["current"]["temperature_2m"] = true;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body,
                                             DeserializationOption::Filter(filter));
  if (err) { Serial.printf("TEPLOTA: JSON %s\n", err.c_str()); return false; }

  JsonVariant t = doc["current"]["temperature_2m"];
  if (t.isNull()) { Serial.println("TEPLOTA: v odpovedi neni temperature_2m"); return false; }
  s_tempC = t.as<float>();
  s_tempOk = true;
  Serial.printf("Teplota: %.1f C\n", s_tempC);
  return true;
}

// Safety net for the clock. Normally the time arrives with whatever we are
// fetching anyway - the forecast alone reseeds it every half hour. But if every
// one of those requests fails (or the only enabled screen is the clock and the
// forecast server is unreachable), the display would sit there with no time at
// all. A HEAD request costs one round trip and only ever runs while we still
// have nothing.
static void reseedClockIfNeeded() {
  if (Outside_TimeValid()) return;
  static unsigned long lastTry = 0;
  unsigned long now = millis();
  if (lastTry && now - lastTry < CLOCK_RESEED_MS) return;
  if (!lastTry && now < 20000UL) return;      // give the normal fetches a chance
  lastTry = now;
  Net_TouchDate(CLOCK_RESEED_URL);
}

void Outside_Tick() {
  // Nothing to try without a link - and crucially, do NOT start the timer here.
  // The first version did, so an attempt made while the WiFi was still coming
  // up counted as "tried" and the next one was ten minutes away. That is why
  // the temperature seemed not to work at all.
  if (WiFi.status() != WL_CONNECTED) return;

  reseedClockIfNeeded();

  unsigned long now = millis();

  // Standing down while the forecast keeps the value fresh.
  if (s_extOk && (now - s_extAt) < OUTSIDE_EXT_FRESH_MS) return;

  if (s_everTried) {
    // Ten minutes once we have a reading, one minute while we still have none.
    unsigned long wait = s_tempOk ? OUTSIDE_TEMP_PERIOD_MS : OUTSIDE_TEMP_RETRY_MS;
    if (now - s_lastTry < wait) return;
  }
  s_everTried = true;
  s_lastTry = now;
  if (!fetchTemp() && !s_tempOk)
    Serial.println("TEPLOTA: zatim se nepodarilo, zkusim za minutu");
}

// --- Readout ----------------------------------------------------------------
void Outside_StatusText(char* buf, size_t cap) {
  if (!buf || cap == 0) return;
  buf[0] = '\0';

  char t[8] = "";
  if (s_timeOk) {
    time_t now = time(nullptr);
    struct tm lt; localtime_r(&now, &lt);
    snprintf(t, sizeof(t), "%02d:%02d", lt.tm_hour, lt.tm_min);
  }

  // Temperature is rounded to whole degrees - a tenth of a degree is noise for
  // a model value and the extra two characters are the difference between
  // fitting on the line and not.
  char c[12] = "";
  if (s_tempOk) {
    int deg = (int)lroundf(s_tempC);
    if (deg < -60 || deg > 60) { /* nonsense - do not show it */ }
    else snprintf(c, sizeof(c), "%d %s", deg, OUTSIDE_DEG_TEXT);
  }

  if (t[0] && c[0]) snprintf(buf, cap, "%s   %s", t, c);
  else if (t[0])    snprintf(buf, cap, "%s", t);
  else if (c[0])    snprintf(buf, cap, "%s", c);
}
