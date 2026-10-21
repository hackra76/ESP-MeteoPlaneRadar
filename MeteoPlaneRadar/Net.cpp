// =============================================================================
//  MeteoPlaneRadar
//  Shared HTTPS fetch helper. See Net.h.
//
// =============================================================================
#include "Net.h"
#include "Config.h"
#include "Outside.h"
#include "NetSink.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <string.h>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <lwip/sockets.h>

// Force immediate socket teardown (RST / no TIME_WAIT) to prevent socket table exhaustion
static void cleanupSocket(WiFiClient& client) {
  int s = client.fd();
  if (s >= 0) {
    struct linger sl = { .l_onoff = 1, .l_linger = 0 };
    setsockopt(s, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
  }
}

// Kept open across a burst of tile requests - see Net.h.
static WiFiClientSecure* s_sess = nullptr;
static SemaphoreHandle_t s_txtMtx = nullptr;

static inline void lockTxt() {
  if (!s_txtMtx) s_txtMtx = xSemaphoreCreateRecursiveMutex();
  if (s_txtMtx) xSemaphoreTakeRecursive(s_txtMtx, pdMS_TO_TICKS(15000));
}
static inline void unlockTxt() {
  if (s_txtMtx) xSemaphoreGiveRecursive(s_txtMtx);
}
struct TxtLockGuard {
  TxtLockGuard()  { lockTxt(); }
  ~TxtLockGuard() { unlockTxt(); }
};

static void (*s_poll)() = nullptr;
void Net_SetPollFn(void (*fn)()) { s_poll = fn; }
static inline void poll() { if (s_poll) s_poll(); }

// Collect the Date header on every request. The clock is seeded from whatever
// we happen to be fetching anyway - see Outside.h for why there is no NTP.
static const char* DATE_HDR[] = { "Date" };

void Net_SessionBegin() {
  if (s_sess) return;
  s_sess = new WiFiClientSecure();
  if (s_sess) {
    s_sess->setInsecure();
    s_sess->setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
  }
}

void Net_SessionEnd() {
  if (!s_sess) return;
  cleanupSocket(*s_sess);
  s_sess->stop();
  delete s_sess;
  s_sess = nullptr;
}

// Shared scratch for text bodies (PSRAM, reused across calls). It grows to what
// the server declares and never past NET_MAX_TEXT - http.getString() used to
// take the body with no ceiling at all, onto the heap TLS draws from.
static char*  s_txt    = nullptr;
static size_t s_txtCap = 0;

static bool txtReserve(size_t need) {
  if (need <= s_txtCap && s_txt) return true;
  char* nb = (char*)heap_caps_malloc(need, MALLOC_CAP_SPIRAM);
  if (!nb) { Serial.println("NET: body does not fit into PSRAM"); return false; }
  if (s_txt) heap_caps_free(s_txt);
  s_txt = nb; s_txtCap = need;
  return true;
}

void Net_FreeBuffers() {
  TxtLockGuard _lk;
  Net_SessionEnd();
  if (s_txt) {
    heap_caps_free(s_txt);
    s_txt = nullptr;
    s_txtCap = 0;
  }
}

bool Net_GetCString(const char* url, const char** outPtr, const char* tag,
                    const char* headerName, const char* headerVal,
                    uint32_t connectTimeoutMs, uint32_t reqTimeoutMs,
                    size_t* outLen) {
  if (outPtr) *outPtr = nullptr;
  if (outLen) *outLen = 0;
  if (WiFi.status() != WL_CONNECTED) return false;

  TxtLockGuard _lk;

  // Clean up any lingering keep-alive tile session before standalone HTTPS fetch
  Net_SessionEnd();

  // Plain http:// endpoints exist too - ip-api.com serves TLS to paying users
  // only - and they belong on the same path as everything else instead of
  // growing a second copy of this function elsewhere. The heap guard is about
  // the handshake, so it only applies when there is one.
  const bool tls = (strncmp(url, "http://", 7) != 0);
  if (tls && !Net_HeapOk(tag)) return false;

  WiFiClient       plain;
  WiFiClientSecure secure;
  if (tls) {
    secure.setInsecure();
    secure.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
  }
  WiFiClient& client = tls ? static_cast<WiFiClient&>(secure)
                           : static_cast<WiFiClient&>(plain);

  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(connectTimeoutMs);
  http.setTimeout(reqTimeoutMs);
  http.setReuse(false);
  http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
  if (!http.begin(client, url)) {
    Serial.printf("%s: begin() failed\n", tag);
    cleanupSocket(client);
    client.stop();
    plain.stop();
    if (tls) secure.stop();
    return false;
  }
  if (headerName && headerVal && strlen(headerName) > 0 && strlen(headerVal) > 0) {
    http.addHeader(headerName, headerVal);
  }
  http.addHeader("Connection", "close");
  http.collectHeaders(DATE_HDR, 1);

  poll();
  int code = http.GET();
  poll();
  if (code != HTTP_CODE_OK) {
    if (code < 0) Serial.printf("%s: HTTP %d (heap %u B, block %u B)\n", tag, code,
                                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    else        Serial.printf("%s: HTTP %d\n", tag, code);
    if (tls) secure.stop();
    client.stop();
    plain.stop();
    cleanupSocket(client);
    http.end();
    return false;
  }
  if (http.hasHeader("Date")) Outside_NoteHttpDate(http.header("Date").c_str());

  int declared = http.getSize();               // -1 when chunked / unknown
  if (declared > (int)NET_MAX_TEXT) {
    Serial.printf("%s: response %d B, limit is %u B\n",
                  tag, declared, (unsigned)NET_MAX_TEXT);
    if (tls) secure.stop();
    client.stop();
    plain.stop();
    cleanupSocket(client);
    http.end();
    return false;
  }
  if (!txtReserve((declared > 0) ? (size_t)declared + 1 : NET_MAX_TEXT)) {
    if (tls) secure.stop();
    client.stop();
    plain.stop();
    cleanupSocket(client);
    http.end();
    return false;
  }

  long len = Net_ReadBody(http, (uint8_t*)s_txt, s_txtCap, tag, s_poll);
  if (tls) secure.stop();
  client.stop();
  plain.stop();
  cleanupSocket(client);
  http.end();
  poll();

  if (len <= 0) {
    Serial.printf("%s: empty response\n", tag);
    return false;
  }
  if (outPtr) *outPtr = s_txt;
  if (outLen) *outLen = (size_t)len;
  return true;
}

bool Net_GetString(const char* url, String& out, const char* tag,
                   const char* headerName, const char* headerVal,
                   uint32_t connectTimeoutMs, uint32_t reqTimeoutMs) {
  out = "";
  TxtLockGuard _lk;
  const char* ptr = nullptr;
  if (!Net_GetCString(url, &ptr, tag, headerName, headerVal, connectTimeoutMs, reqTimeoutMs, nullptr) || !ptr) {
    return false;
  }
  out = ptr;
  return true;
}

bool Net_PostJson(const char* url, const char* jsonPayload, String& out, const char* tag, int* outStatusCode) {
  if (outStatusCode) *outStatusCode = 0;
  out = "";
  if (WiFi.status() != WL_CONNECTED) return false;
  if (!jsonPayload) return false;

  TxtLockGuard _lk;

  // Clean up any lingering keep-alive tile session before standalone HTTPS fetch
  Net_SessionEnd();

  const bool tls = (strncmp(url, "http://", 7) != 0);
  if (tls && !Net_HeapOk(tag)) return false;

  WiFiClient       plain;
  WiFiClientSecure secure;
  if (tls) {
    secure.setInsecure();
    secure.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
  }
  WiFiClient& client = tls ? static_cast<WiFiClient&>(secure)
                           : static_cast<WiFiClient&>(plain);

  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setReuse(false);
  http.setUserAgent("MeteoPlaneRadar-AI/1.0");
  if (!http.begin(client, url)) {
    Serial.printf("%s: begin() failed\n", tag);
    cleanupSocket(client);
    client.stop();
    plain.stop();
    if (tls) secure.stop();
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  poll();
  int code = http.POST((uint8_t*)jsonPayload, strlen(jsonPayload));
  poll();
  if (outStatusCode) *outStatusCode = code;
  if (code != HTTP_CODE_OK && code != 201) {
    if (code == 429) {
      Serial.printf("%s: HTTP POST 429 (Rate limit / Quota exceeded)\n", tag);
    } else {
      String errBody = http.getString();
      Serial.printf("%s: HTTP POST error %d: %s\n", tag, code, errBody.c_str());
    }
    cleanupSocket(client);
    http.end();
    client.stop();
    plain.stop();
    if (tls) secure.stop();
    return false;
  }

  int declared = http.getSize();
  if (declared > (int)NET_MAX_TEXT) {
    Serial.printf("%s: response %d B exceeds limit\n", tag, declared);
    cleanupSocket(client);
    http.end();
    client.stop();
    plain.stop();
    if (tls) secure.stop();
    return false;
  }
  if (!txtReserve((declared > 0) ? (size_t)declared + 1 : NET_MAX_TEXT)) {
    if (tls) secure.stop();
    client.stop();
    plain.stop();
    cleanupSocket(client);
    http.end();
    return false;
  }

  long len = Net_ReadBody(http, (uint8_t*)s_txt, s_txtCap, tag, s_poll);
  if (tls) secure.stop();
  client.stop();
  plain.stop();
  cleanupSocket(client);
  http.end();
  poll();

  if (len <= 0) { Serial.printf("%s: empty response\n", tag); return false; }
  out = s_txt;
  return true;
}

bool Net_GetBinary(const char* url, uint8_t* buf, size_t cap, size_t* outLen,
                   const char* tag) {
  if (outLen) *outLen = 0;
  if (!buf || cap == 0) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  const bool tls = (strncmp(url, "http://", 7) != 0);
  const bool sess = (s_sess != nullptr && tls);
  if (sess && !s_sess->connected()) {
    cleanupSocket(*s_sess);
    s_sess->stop();
    delete s_sess;
    s_sess = new WiFiClientSecure();
    if (s_sess) {
      s_sess->setInsecure();
      s_sess->setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
    }
  }
  const bool connected = (sess && s_sess && s_sess->connected());
  if (tls && !connected && !Net_HeapOk(tag)) return false;

  WiFiClient       plain;
  WiFiClientSecure ownSecure;
  if (tls && !sess) {
    ownSecure.setInsecure();
    ownSecure.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
  }
  WiFiClient& client = tls ? (sess && s_sess ? static_cast<WiFiClient&>(*s_sess)
                                             : static_cast<WiFiClient&>(ownSecure))
                           : static_cast<WiFiClient&>(plain);

  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setReuse(sess && s_sess != nullptr); // keep the socket open for the next tile
  http.setUserAgent("ESP-MeteoPlaneRadar/2.0 (contact: admin@esp-meteoplane.local)");
  if (!http.begin(client, url)) {
    if (!sess) { cleanupSocket(client); client.stop(); plain.stop(); if (tls) ownSecure.stop(); }
    return false;
  }
  http.collectHeaders(DATE_HDR, 1);

  poll();
  int code = http.GET();
  poll();
  if (code != HTTP_CODE_OK) {
    Serial.printf("%s: HTTP %d\n", tag, code);
    cleanupSocket(client);
    http.end();
    if (sess && s_sess) {
      s_sess->stop();
      delete s_sess;
      s_sess = nullptr;
    }
    if (!sess) { client.stop(); plain.stop(); if (tls) ownSecure.stop(); }
    return false;
  }
  if (http.hasHeader("Date")) Outside_NoteHttpDate(http.header("Date").c_str());

  int declared = http.getSize();
  if (declared > 0 && (size_t)declared > cap) {
    Serial.printf("%s: response %d B does not fit into %u B\n", tag, declared, (unsigned)cap);
    cleanupSocket(client);
    http.end();
    if (sess && s_sess) {
      s_sess->stop();
      delete s_sess;
      s_sess = nullptr;
    }
    if (!sess) { client.stop(); plain.stop(); if (tls) ownSecure.stop(); }
    return false;
  }

  long got = Net_ReadBody(http, buf, cap, tag, s_poll);
  if (!sess) {
    if (tls) ownSecure.stop();
    client.stop();
    plain.stop();
    cleanupSocket(client);
    http.end();
  } else {
    http.end();
  }
  poll();
  if (got <= 0) {
    if (sess && s_sess) {
      cleanupSocket(*s_sess);
      s_sess->stop();
      delete s_sess;
      s_sess = nullptr;
    }
    return false;
  }
  if (outLen) *outLen = got;
  return true;
}

bool Net_TouchDate(const char* url) {
  if (WiFi.status() != WL_CONNECTED) return false;
  Net_SessionEnd();
  if (!Net_HeapOk("CLOCK")) return false;

  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(5000);
  http.setTimeout(6000);
  http.setReuse(false);
  if (!http.begin(client, url)) return false;
  http.collectHeaders(DATE_HDR, 1);
  poll();
  int code = http.sendRequest("HEAD");
  poll();
  bool ok = false;
  if (code > 0 && http.hasHeader("Date")) {
    Outside_NoteHttpDate(http.header("Date").c_str());
    ok = true;
  }
  http.end();
  poll();
  return ok;
}
