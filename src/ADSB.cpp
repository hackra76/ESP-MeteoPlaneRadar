// =============================================================================
//  MeteoPlaneRadar
//  ADS-B client - fetching aircraft data from adsb.fi.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ADSB.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>   // strcmp / strncpy for the hex identity handling
#include "esp_heap_caps.h"   // PSRAM body buffer
#include "Outside.h"         // clock is seeded from the response header

static const float KM_PER_NM = 1.852f;

static Aircraft s_list[ADSB_MAX];   // last GOOD snapshot shown on screen
static int s_count = 0;
// Scratch buffer we parse into. The visible list (s_list/s_count) is only
// overwritten once a fetch has fully and correctly parsed - so a truncated or
// otherwise broken JSON never blanks the radar, it just keeps the previous
// aircraft until the next good fetch.
static Aircraft s_tmp[ADSB_MAX];
static void (*s_poll)() = nullptr;

void ADSB_SetPollFn(void (*fn)()) { s_poll = fn; }
int  ADSB_Count() { return s_count; }
const Aircraft* ADSB_List() { return s_list; }

int ADSB_FindByHex(const char* hex) {
  if (!hex || !hex[0]) return -1;
  for (int i = 0; i < s_count; i++) {
    if (strcmp(s_list[i].hex, hex) == 0) return i;
  }
  return -1;   // no longer in the data
}

static void poll() { if (s_poll) s_poll(); }

// Reads a number even when the JSON holds it as a string.
static bool readFloat(JsonObjectConst o, const char* key, float* out) {
  JsonVariantConst v = o[key];
  if (v.is<float>() || v.is<double>() || v.is<int>()) { *out = v.as<float>(); return true; }
  if (v.is<const char*>()) {
    const char* s = v.as<const char*>();
    if (s && *s) { *out = (float)atof(s); return true; }
  }
  return false;
}

// Copies the ICAO hex address. This is the aircraft's stable identity and is
// stored for every target, even when a callsign is present - the callsign is a
// *flight* number (it changes between rotations, and two aircraft can carry the
// same one on different days), so it is no good as a key.
static void copyHex(Aircraft* a, JsonObjectConst plane) {
  const char* hex = plane["hex"] | "";
  strncpy(a->hex, hex, sizeof(a->hex) - 1);
  a->hex[sizeof(a->hex) - 1] = '\0';
}

static void copyCallsign(Aircraft* a, JsonObjectConst plane) {
  const char* flight = plane["flight"] | "";
  const char* hex = plane["hex"] | "";
  const char* src = (flight[0] != '\0') ? flight : hex;
  while (*src == ' ') src++;   // skip leading spaces
  int i = 0;
  while (src[i] && i < (int)sizeof(a->callsign) - 1) { a->callsign[i] = src[i]; i++; }
  while (i > 0 && a->callsign[i-1] == ' ') i--;   // trim trailing spaces
  a->callsign[i] = '\0';
}

// -----------------------------------------------------------------------------
//  Body buffer (PSRAM). Reused across fetches so we do not fragment the internal
//  heap with a big String on every poll. The WHOLE HTTP body is read here before
//  parsing, so the JSON parser always sees a complete document. The old
//  "IncompleteInput" came from parsing straight off a TLS stream that ended
//  mid-object.
// -----------------------------------------------------------------------------
static char*  s_body    = nullptr;
static size_t s_bodyCap = 0;
static const size_t ADSB_MAX_BODY = 1024 * 1024;   // 1 MB hard cap

static bool bodyReserve(size_t need) {
  if (need <= s_bodyCap) return true;
  size_t cap = need + 2048;
  char* nb = (char*)heap_caps_realloc(s_body, cap, MALLOC_CAP_SPIRAM);
  if (!nb) nb = (char*)heap_caps_realloc(s_body, cap, MALLOC_CAP_DEFAULT);
  if (!nb) return false;
  s_body = nb; s_bodyCap = cap;
  return true;
}

// Read the whole HTTP body into s_body. Returns the byte count (>= 0) or -1 on a
// hard error (alloc / no stream). *complete is set false when the server
// declared a Content-Length we did not fully receive (a cut-off download).
static long readBody(HTTPClient& http, bool* complete) {
  *complete = true;
  int declared = http.getSize();              // -1 when unknown / chunked
  if (declared > (int)ADSB_MAX_BODY) return -1;
  WiFiClient* stream = http.getStreamPtr();
  if (!stream) return -1;

  size_t want = (declared > 0) ? (size_t)declared : 8192;
  if (!bodyReserve(want + 1)) return -1;

  size_t total = 0;
  unsigned long last = millis();
  while (http.connected() && (declared < 0 || total < (size_t)declared)) {
    poll();                                    // yield + feed watchdog
    size_t avail = stream->available();
    if (avail) {
      if (total + avail + 1 > s_bodyCap) {
        if (total + avail + 1 > ADSB_MAX_BODY) { *complete = false; break; }
        if (!bodyReserve(total + avail + 1)) return -1;
      }
      int r = stream->readBytes(s_body + total, avail);
      if (r <= 0) break;
      total += r;
      last = millis();
    } else {
      if (millis() - last > 8000) break;       // stalled mid-transfer
      delay(2);
    }
  }
  if (s_body) s_body[total] = '\0';
  if (declared > 0 && total < (size_t)declared) *complete = false;
  return (long)total;
}

// Filter document: only the keys we actually use are kept, so the parsed
// JsonDocument stays small no matter how much adsb.fi sends. alt_baro MUST stay
// - it carries the literal "ground" used to detect aircraft on the ground.
static void buildFilter(JsonDocument& filter) {
  JsonObject o = filter["ac"].add<JsonObject>();   // ArduinoJson 7 idiom
  o["hex"]          = true;
  o["flight"]       = true;
  o["lat"]          = true;
  o["lon"]          = true;
  o["track"]        = true;
  o["true_heading"] = true;
  o["alt_baro"]     = true;
  o["gs"]           = true;
  o["baro_rate"]    = true;
  o["t"]            = true;
  o["type"]         = true;
}

// A TLS handshake allocates roughly 45 kB of INTERNAL RAM (PSRAM cannot be used
// for it). When that allocation fails, mbedTLS reports it as a plain "HTTP -1"
// with no hint of the real cause. Checking first turns a mystery into a log
// line, and skipping the poll leaves the previous data on screen.
static bool netHeapOk(const char* what) {
  size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (freeInt >= NET_MIN_HEAP) return true;
  Serial.printf("%s: malo volne pameti (%u B < %u B), stahovani preskoceno\n",
                what, (unsigned)freeInt, (unsigned)NET_MIN_HEAP);
  return false;
}

bool ADSB_Fetch(double lat, double lon, float radiusKm) {
  // No link -> keep whatever we last drew (do NOT blank the radar).
  if (WiFi.status() != WL_CONNECTED) { Serial.println("ADSB: no WiFi"); return false; }
  if (!netHeapOk("ADSB")) return false;

  float distNm = radiusKm / KM_PER_NM;
  char url[128];
  snprintf(url, sizeof(url), "%s%.5f/lon/%.5f/dist/%.1f",
           ADSB_API_BASE, lat, lon, distNm);
  Serial.printf("ADSB: %s\n", url);

  // Up to two attempts. A single transient hiccup (dropped TLS, truncated body)
  // no longer skips a whole poll cycle; two failures fall through and the
  // previous snapshot stays on screen.
  const int MAX_ATTEMPTS = 2;
  for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
    poll();
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setConnectTimeout(8000);   // ms - TCP + TLS handshake
    http.setTimeout(12000);         // ms - per-read timeout
    http.setReuse(false);           // clean close, do not pool the socket
    if (!http.begin(client, url)) {
      Serial.println("ADSB: begin failed");
      if (attempt < MAX_ATTEMPTS) { delay(200); continue; }
      return false;
    }
    // The response header carries the current time - that is where the clock
    // comes from, so ask HTTPClient to keep it (see Outside.h).
    static const char* WANTED[] = { "Date" };
    http.collectHeaders(WANTED, 1);
    // Identify ourselves - adsb.fi's free API asks callers to be polite.
    http.addHeader("User-Agent", "MeteoPlaneRadar/1.0 (+https://chiptron.cz)");
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    // Seed the clock even from a non-OK answer - the header is there either way.
    if (http.hasHeader("Date")) Outside_NoteHttpDate(http.header("Date").c_str());
    if (code != HTTP_CODE_OK) {
      Serial.printf("ADSB: HTTP %d (attempt %d)\n", code, attempt);
      http.end();
      if (attempt < MAX_ATTEMPTS) { delay(200); continue; }
      return false;   // keep last good data
    }

    // Read the WHOLE body first (into PSRAM), then parse - no live-stream parse.
    bool complete = true;
    long len = readBody(http, &complete);
    http.end();

    if (len < 0) {
      Serial.println("ADSB: body read failed");
      if (attempt < MAX_ATTEMPTS) { delay(200); continue; }
      return false;
    }
    if (!complete) {
      Serial.printf("ADSB: truncated body (attempt %d)\n", attempt);
      if (attempt < MAX_ATTEMPTS) { delay(200); continue; }
      return false;
    }
    if (len < 8) {   // empty / nonsense
      Serial.printf("ADSB: short body %ld (attempt %d)\n", len, attempt);
      if (attempt < MAX_ATTEMPTS) { delay(200); continue; }
      return false;
    }

    // Filtered parse of the complete in-memory buffer.
    JsonDocument filter;
    buildFilter(filter);
    JsonDocument doc;
    DeserializationError err =
        deserializeJson(doc, s_body, (size_t)len, DeserializationOption::Filter(filter));
    if (err) {
      Serial.printf("ADSB: JSON %s (attempt %d) - keeping last %d\n",
                    err.c_str(), attempt, s_count);
      if (attempt < MAX_ATTEMPTS) { delay(200); continue; }
      return false;
    }

    if (!doc["ac"].is<JsonArray>()) {
      Serial.println("ADSB: no 'ac' array - keeping last data");
      return false;   // valid JSON but wrong shape; a retry would not help
    }

    // Parse into the SCRATCH list; commit to the live list only on success.
    JsonArrayConst ac = doc["ac"].as<JsonArrayConst>();
    int n = 0;
    for (JsonObjectConst plane : ac) {
      float plat, plon;
      if (!readFloat(plane, "lat", &plat) || !readFloat(plane, "lon", &plon)) continue;

      // Aircraft on the ground are dropped here so they never take a slot from
      // an airborne target (near an airport they could otherwise fill ADSB_MAX).
      JsonVariantConst ab = plane["alt_baro"];
      bool ground = ab.is<const char*>() && strcmp(ab.as<const char*>(), "ground") == 0;
      if (ground) continue;

      if (n >= ADSB_MAX) break;
      s_tmp[n].lat = plat;
      s_tmp[n].lon = plon;
      s_tmp[n].onGround = false;
      // Ground track - record whether it is present at all.
      float tr = 0;
      if (readFloat(plane, "track", &tr) || readFloat(plane, "true_heading", &tr)) {
        s_tmp[n].track = tr;
        s_tmp[n].hasTrack = true;
      } else {
        s_tmp[n].track = 0;
        s_tmp[n].hasTrack = false;
      }
      // Altitude (barometric), speed, climb rate.
      float f = 0;
      s_tmp[n].altFt    = readFloat(plane, "alt_baro", &f) ? f : 0;
      s_tmp[n].gsKt     = readFloat(plane, "gs", &f) ? f : 0;
      s_tmp[n].baroRate = readFloat(plane, "baro_rate", &f) ? f : 0;
      // Aircraft type (the key varies by source).
      const char* ty = plane["t"] | (plane["type"] | "");
      strncpy(s_tmp[n].type, ty, sizeof(s_tmp[n].type) - 1);
      s_tmp[n].type[sizeof(s_tmp[n].type) - 1] = '\0';
      copyHex(&s_tmp[n], plane);
      copyCallsign(&s_tmp[n], plane);
      n++;
    }

    // Commit the scratch snapshot to the live list in one go.
    for (int i = 0; i < n; i++) s_list[i] = s_tmp[i];
    s_count = n;
    Serial.printf("ADSB: %d aircraft (%ld bytes)\n", n, len);
    return true;
  }
  return false;
}
