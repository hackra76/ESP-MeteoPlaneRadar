// =============================================================================
//  MeteoPlaneRadar
//  Shared HTTPS fetch helper. See Net.h.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Net.h"
#include "Config.h"
#include "Outside.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>

// Kept open across a burst of tile requests - see Net.h.
static WiFiClientSecure* s_sess = nullptr;

static void (*s_poll)() = nullptr;
void Net_SetPollFn(void (*fn)()) { s_poll = fn; }
static inline void poll() { if (s_poll) s_poll(); }

// A TLS handshake needs roughly 45 kB of INTERNAL RAM (PSRAM will not do).
// Starting one with less fails deep inside mbedTLS and surfaces as a bare
// "HTTP -1", which tells nobody anything - so refuse early and say why.
static bool heapOk(const char* tag) {
  size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (freeInt >= NET_MIN_HEAP) return true;
  Serial.printf("%s: malo volne pameti (%u B), preskoceno\n", tag, (unsigned)freeInt);
  return false;
}

// Collect the Date header on every request. The clock is seeded from whatever
// we happen to be fetching anyway - see Outside.h for why there is no NTP.
static const char* DATE_HDR[] = { "Date" };

void Net_SessionBegin() {
  if (s_sess) return;
  s_sess = new WiFiClientSecure();
  if (s_sess) s_sess->setInsecure();
}

void Net_SessionEnd() {
  if (!s_sess) return;
  s_sess->stop();
  delete s_sess;
  s_sess = nullptr;
}

bool Net_GetString(const char* url, String& out, const char* tag) {
  out = "";
  if (WiFi.status() != WL_CONNECTED) return false;
  if (!heapOk(tag)) return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(6000);
  http.setTimeout(10000);
  http.setReuse(false);
  if (!http.begin(client, url)) { Serial.printf("%s: begin() selhalo\n", tag); return false; }
  http.collectHeaders(DATE_HDR, 1);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("%s: HTTP %d\n", tag, code);
    http.end();
    return false;
  }
  if (http.hasHeader("Date")) Outside_NoteHttpDate(http.header("Date").c_str());

  poll();
  out = http.getString();
  http.end();
  poll();

  if (out.length() == 0) { Serial.printf("%s: prazdna odpoved\n", tag); return false; }
  return true;
}

bool Net_GetBinary(const char* url, uint8_t* buf, size_t cap, size_t* outLen,
                   const char* tag) {
  if (outLen) *outLen = 0;
  if (!buf || cap == 0) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  // Inside a session the connection is already up, so the big handshake
  // allocation is not about to happen and the heap guard would only get in the
  // way. Outside one it still applies.
  const bool sess = (s_sess != nullptr);
  if (!sess && !heapOk(tag)) return false;

  WiFiClientSecure  own;
  WiFiClientSecure& client = sess ? *s_sess : own;
  if (!sess) client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(6000);
  http.setTimeout(10000);
  http.setReuse(sess);            // keep the socket open for the next tile
  if (!http.begin(client, url)) return false;
  http.collectHeaders(DATE_HDR, 1);

  int code = http.GET();
  if (code != HTTP_CODE_OK) { Serial.printf("%s: HTTP %d\n", tag, code); http.end(); return false; }
  if (http.hasHeader("Date")) Outside_NoteHttpDate(http.header("Date").c_str());

  int declared = http.getSize();
  if (declared > 0 && (size_t)declared > cap) {
    Serial.printf("%s: odpoved %d B se nevejde do %u B\n", tag, declared, (unsigned)cap);
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t got = 0;
  unsigned long last = millis();
  while (http.connected() && got < cap) {
    size_t avail = stream->available();
    if (avail) {
      size_t want = cap - got;
      if (avail < want) want = avail;
      int r = stream->readBytes(buf + got, want);
      if (r <= 0) break;
      got += (size_t)r;
      last = millis();
      poll();
    } else {
      if (declared > 0 && got >= (size_t)declared) break;   // complete
      if (millis() - last > 8000) { Serial.printf("%s: timeout\n", tag); break; }
      delay(2);
      poll();
    }
  }
  http.end();

  // A truncated image decodes into garbage, so an incomplete transfer has to be
  // a failure rather than something the decoder finds out about later.
  if (declared > 0 && got != (size_t)declared) {
    Serial.printf("%s: neuplne (%u z %d B)\n", tag, (unsigned)got, declared);
    return false;
  }
  if (got == 0) return false;
  if (outLen) *outLen = got;
  return true;
}

bool Net_TouchDate(const char* url) {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (!heapOk("HODINY")) return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(6000);
  http.setReuse(false);
  if (!http.begin(client, url)) return false;
  http.collectHeaders(DATE_HDR, 1);
  int code = http.sendRequest("HEAD");
  bool ok = false;
  if (code > 0 && http.hasHeader("Date")) {
    Outside_NoteHttpDate(http.header("Date").c_str());
    ok = true;
  }
  http.end();
  return ok;
}
