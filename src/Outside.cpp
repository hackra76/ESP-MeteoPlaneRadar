// =============================================================================
//  MeteoPlaneRadar - clock (from HTTP Date) + outside temperature (Open-Meteo).
//  See Outside.h for why there is no NTP client here.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Outside.h"
#include "TimeUtil.h"
#include "Config.h"
#include "Settings.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
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

static bool fetchTemp() {
  if (WiFi.status() != WL_CONNECTED) return false;
  // Same guard as the other endpoints: a TLS handshake wants ~45 kB of internal
  // RAM and fails as a bare "HTTP -1" when it cannot get it.
  size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (freeInt < NET_MIN_HEAP) {
    Serial.printf("TEPLOTA: malo volne pameti (%u B), preskoceno\n", (unsigned)freeInt);
    return false;
  }

  char url[160];
  snprintf(url, sizeof(url),
           "%s?latitude=%.4f&longitude=%.4f&current=temperature_2m",
           OUTSIDE_TEMP_URL, Settings_Lat(), Settings_Lon());

  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(6000);
  http.setTimeout(8000);
  http.setReuse(false);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); Serial.printf("TEPLOTA: HTTP %d\n", code); return false; }

  // The whole answer is a few hundred bytes, so take it as one string rather
  // than parsing off the stream - a chunked transfer could otherwise cut the
  // parser short halfway through.
  String body = http.getString();
  http.end();
  if (body.length() == 0) { Serial.println("TEPLOTA: prazdna odpoved"); return false; }

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

void Outside_Tick() {
  // Nothing to try without a link - and crucially, do NOT start the timer here.
  // The first version did, so an attempt made while the WiFi was still coming
  // up counted as "tried" and the next one was ten minutes away. That is why
  // the temperature seemed not to work at all.
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();
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
