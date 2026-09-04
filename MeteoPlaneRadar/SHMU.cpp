// MeteoPlaneRadar - vyvoj / development: chiptron.cz
// =============================================================================
//  MeteoPlaneRadar - meteoradar SHMU: stahovani do PSRAM (1 snimek + animace).
// =============================================================================
#include "SHMU.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "esp_heap_caps.h"
#include "Config.h"
#include "TimeUtil.h"
#include "Outside.h"
#include "NetSink.h"
#include <string.h>      // strstr / memcmp

static const char* NAME_PREFIX = "cmax.kruh.";

static void (*s_poll)() = nullptr;
void SHMU_SetPollFn(void (*fn)()) { s_poll = fn; }

// -----------------------------------------------------------------------------
//  Spolecne pomucky
// -----------------------------------------------------------------------------
static String extractTimestamp(const String& name) {
  int start = name.indexOf(NAME_PREFIX);
  if (start < 0) return "";
  int ds = start + strlen(NAME_PREFIX);
  if ((int)name.length() < ds + 13) return "";
  if (name[ds + 8] != '.') return "";
  String date = name.substring(ds, ds + 8);
  String hhmm = name.substring(ds + 9, ds + 13);
  for (unsigned i = 0; i < date.length(); i++) if (!isDigit(date[i])) return "";
  for (unsigned i = 0; i < hhmm.length(); i++) if (!isDigit(hhmm[i])) return "";
  return date + hhmm;   // YYYYMMDDHHMM
}

static String timeTextFromName(const String& name) {
  String ts = extractTimestamp(name);
  if (ts.length() < 12) return "";
  int Y  = ts.substring(0, 4).toInt();
  int Mo = ts.substring(4, 6).toInt();
  int D  = ts.substring(6, 8).toInt();
  int hh = ts.substring(8, 10).toInt();
  int mm = ts.substring(10, 12).toInt();
  if (Y < 2000 || Mo < 1 || Mo > 12 || D < 1 || D > 31) return "";

  time_t utc = TimeUtil_UtcToEpoch(Y, Mo, D, hh, mm, 0);
  struct tm lt;
  localtime_r(&utc, &lt);
  char out[6];
  snprintf(out, sizeof(out), "%02d:%02d", lt.tm_hour, lt.tm_min);
  return String(out);
}

// Stahne dany PNG do zadaneho bufferu. Vraci true a naplni *outSize.
static bool downloadNameTo(const String& name, uint8_t* buf, size_t cap, size_t* outSize) {
  *outSize = 0;
  if (!buf) return false;
  if (!Net_HeapOk("SHMU")) return false;
  String url = String(SHMU_BASE_URL) + name;
  WiFiClientSecure client; client.setInsecure();
  client.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
  HTTPClient http;
  http.setConnectTimeout(6000);   // TCP connect only, NOT the TLS handshake
  http.setTimeout(15000);
  http.setUserAgent("Mozilla/5.0 (ESP-MeteoPlaneRadar)");
  if (!http.begin(client, url)) {
    client.stop();
    return false;
  }
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("SHMU: GET %s selhal (%d)\n", name.c_str(), code);
    while (client.available()) client.read();
    http.end();
    client.stop();
    return false;
  }
  int total = http.getSize();
  if (total > (int)cap) {
    Serial.printf("SHMU: %s prilis velky (%d > %u)\n", name.c_str(), total, (unsigned)cap);
    while (client.available()) client.read();
    http.end();
    client.stop();
    return false;
  }
  long got = Net_ReadBody(http, buf, cap, "SHMU", s_poll);
  http.end();
  client.stop();
  if (got < 0) return false;

  if (got < 8 || memcmp(buf, "\x89PNG\r\n\x1a\n", 8) != 0) {
    Serial.printf("SHMU: %s neni PNG (%ld B)\n", name.c_str(), got);
    return false;
  }
  *outSize = (size_t)got;
  return true;
}

// -----------------------------------------------------------------------------
//  Animace - nejnovejsich N ramcu
// -----------------------------------------------------------------------------
static uint8_t* s_animBuf[SHMU_ANIM_MAX] = {0};
static size_t   s_animSize[SHMU_ANIM_MAX] = {0};
static String   s_animName[SHMU_ANIM_MAX];
static int      s_animCount = 0;

int      SHMU_AnimCount() { return s_animCount; }
uint8_t* SHMU_AnimData(int i) { return (i >= 0 && i < s_animCount) ? s_animBuf[i] : nullptr; }
size_t   SHMU_AnimSize(int i) { return (i >= 0 && i < s_animCount) ? s_animSize[i] : 0; }
String   SHMU_AnimTimeText(int i) { return (i >= 0 && i < s_animCount) ? timeTextFromName(s_animName[i]) : String(""); }

// Bezici "top-N" nejnovejsich nazvu (vzestupne dle casu).
static String s_topName[SHMU_ANIM_MAX];
static String s_topTs[SHMU_ANIM_MAX];
static int    s_topCount = 0;

static void topInsert(const String& name, const String& ts) {
  for (int i = 0; i < s_topCount; i++) if (s_topTs[i] == ts) return;   // duplicita
  if (s_topCount < SHMU_ANIM_MAX) {
    int p = s_topCount;
    while (p > 0 && s_topTs[p - 1] > ts) { s_topTs[p] = s_topTs[p - 1]; s_topName[p] = s_topName[p - 1]; p--; }
    s_topTs[p] = ts; s_topName[p] = name; s_topCount++;
  } else if (ts > s_topTs[0]) {   // nahradime nejstarsi
    int p = 0;
    while (p < SHMU_ANIM_MAX - 1 && s_topTs[p + 1] < ts) { s_topTs[p] = s_topTs[p + 1]; s_topName[p] = s_topName[p + 1]; p++; }
    s_topTs[p] = ts; s_topName[p] = name;
  }
}

// -----------------------------------------------------------------------------
//  Scanning getradardata JSON
// -----------------------------------------------------------------------------
static void scanTop(const char* text, void* user) {
  (void)user;
  const char* pos = text;
  while (true) {
    const char* idx = strstr(pos, NAME_PREFIX); if (!idx) break;
    const char* end = strstr(idx, ".png");      if (!end) break;
    String name; name.concat(idx, (size_t)(end + 4 - idx));
    String ts = extractTimestamp(name);
    if (ts.length()) topInsert(name, ts);
    pos = end + 4;
  }
}

static bool ensureAnimBuffer(int i) {
  if (s_animBuf[i]) return true;
  s_animBuf[i] = (uint8_t*)heap_caps_malloc(SHMU_MAX_PNG, MALLOC_CAP_SPIRAM);
  return s_animBuf[i] != nullptr;
}

int SHMU_FetchAnim(int wantN) {
  if (WiFi.status() != WL_CONNECTED) return s_animCount;
  if (wantN > SHMU_ANIM_MAX) wantN = SHMU_ANIM_MAX;
  if (wantN < 1) wantN = 1;

  // 1) Projdi JSON API a najdi N nejnovejsich nazvu souboru
  if (!Net_HeapOk("SHMU")) return s_animCount;
  s_topCount = 0;
  {
    WiFiClientSecure client; client.setInsecure();
    client.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
    HTTPClient http;
    http.setConnectTimeout(6000);   // TCP connect only, NOT the TLS handshake
    http.setTimeout(15000);
    http.setUserAgent("Mozilla/5.0 (ESP-MeteoPlaneRadar)");
    static const char* WANTED[] = { "Date" };
    http.collectHeaders(WANTED, 1);
    if (!http.begin(client, SHMU_API_URL)) {
      client.stop();
      return s_animCount;
    }
    int code = http.GET();
    if (http.hasHeader("Date")) Outside_NoteHttpDate(http.header("Date").c_str());
    if (code != HTTP_CODE_OK) {
      while (client.available()) client.read();
      http.end();
      client.stop();
      return s_animCount;
    }
    long ilen = Net_ScanBody(http, scanTop, nullptr, "SHMU", s_poll);
    http.end();
    client.stop();
    if (ilen <= 0) return s_animCount;
    Serial.printf("SHMU: API body %ld B, nalezeno %d nazvu\n", ilen, s_topCount);
  }
  if (s_topCount == 0) return s_animCount;

  // Kratky odpocinek a uvolneni sitovych struktur pred stahovanim PNG
  if (s_poll) s_poll();
  delay(150);

  // 2) Stahni N nejnovejsich (top pole je vzestupne, bereme konec)
  int n = s_topCount < wantN ? s_topCount : wantN;
  int startIdx = s_topCount - n;
  int got = 0;
  for (int i = 0; i < n; i++) {
    if (s_poll) s_poll();
    if (!ensureAnimBuffer(i)) break;
    size_t sz = 0;
    if (downloadNameTo(s_topName[startIdx + i], s_animBuf[i], SHMU_MAX_PNG, &sz)) {
      s_animSize[i] = sz; s_animName[i] = s_topName[startIdx + i]; got++;
    } else {
      break;
    }
    if (s_poll) s_poll();
    delay(100);
  }
  s_animCount = got;
  Serial.printf("SHMU meteoradar: %d ramcu\n", got);
  return got;
}
