// =============================================================================
//  MeteoPlaneRadar
//  PlanePhoto.cpp - Aircraft thumbnail photos fetched from Planespotters.net API
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#include "PlanePhoto.h"
#include "Config.h"
#include "Watchdog.h"
#include "Net.h"
#include "NetSink.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MALLOC(sz)      heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define STBI_REALLOC(p, sz)  heap_caps_realloc(p, sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define STBI_FREE(p)         heap_caps_free(p)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb_image.h"
#pragma GCC diagnostic pop

#define PHOTO_CACHE_N   4
#define PHOTO_BUF_MAX   49152   // up to 48 KB JPEG buffer

struct PhotoEntry {
  char      reg[12]          = "";
  char      hex[8]           = "";
  char      photographer[36] = "";
  uint16_t* rgb565           = nullptr;
  int       width            = 0;
  int       height           = 0;
  PhotoState state           = PHOTO_IDLE;
  unsigned long stamp        = 0;
};

static SemaphoreHandle_t s_mtx = nullptr;
static PhotoEntry s_cache[PHOTO_CACHE_N];
static int  s_activeSlot = -1;
static bool s_pending = false;
static bool s_changed = false;
static uint8_t* s_tempJpeg = nullptr;

static void lock() {
  if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
  xSemaphoreTake(s_mtx, portMAX_DELAY);
}

static void unlock() {
  if (s_mtx) xSemaphoreGive(s_mtx);
}

static int findEntry(const char* reg, const char* hex) {
  for (int i = 0; i < PHOTO_CACHE_N; i++) {
    if (reg && *reg && strcasecmp(s_cache[i].reg, reg) == 0) return i;
    if (hex && *hex && strcasecmp(s_cache[i].hex, hex) == 0) return i;
  }
  return -1;
}

static int allocSlot() {
  for (int i = 0; i < PHOTO_CACHE_N; i++) {
    if (s_cache[i].state == PHOTO_IDLE) return i;
  }
  int oldest = 0;
  unsigned long minStamp = s_cache[0].stamp;
  for (int i = 1; i < PHOTO_CACHE_N; i++) {
    if (s_cache[i].stamp < minStamp) {
      minStamp = s_cache[i].stamp;
      oldest = i;
    }
  }
  return oldest;
}

void PlanePhoto_Select(const char* reg, const char* hex) {
  char cleanReg[12] = "";
  char cleanHex[8] = "";
  if (reg) strncpy(cleanReg, reg, sizeof(cleanReg) - 1);
  if (hex) strncpy(cleanHex, hex, sizeof(cleanHex) - 1);

  if (!cleanReg[0] && !cleanHex[0]) {
    PlanePhoto_Clear();
    return;
  }

  lock();
  int idx = findEntry(cleanReg, cleanHex);
  if (idx >= 0) {
    s_activeSlot = idx;
    if (s_cache[idx].state == PHOTO_OK || s_cache[idx].state == PHOTO_NONE) {
      s_pending = false;
      unlock();
      return;
    }
  } else {
    idx = allocSlot();
    s_activeSlot = idx;
    strncpy(s_cache[idx].reg, cleanReg, sizeof(s_cache[idx].reg) - 1);
    strncpy(s_cache[idx].hex, cleanHex, sizeof(s_cache[idx].hex) - 1);
    s_cache[idx].photographer[0] = '\0';
    if (s_cache[idx].rgb565) {
      heap_caps_free(s_cache[idx].rgb565);
      s_cache[idx].rgb565 = nullptr;
    }
    s_cache[idx].width = 0;
    s_cache[idx].height = 0;
    s_cache[idx].state = PHOTO_WAIT;
    s_cache[idx].stamp = millis();
    s_pending = true;
  }
  unlock();
}

void PlanePhoto_Clear() {
  lock();
  s_activeSlot = -1;
  s_pending = false;
  unlock();
}

bool PlanePhoto_HasPending() {
  lock();
  bool p = s_pending && (s_activeSlot >= 0);
  unlock();
  return p;
}

bool PlanePhoto_TakeChanged() {
  lock();
  bool c = s_changed;
  s_changed = false;
  unlock();
  return c;
}

PhotoState PlanePhoto_GetState() {
  lock();
  PhotoState st = PHOTO_IDLE;
  if (s_activeSlot >= 0 && s_activeSlot < PHOTO_CACHE_N) {
    st = s_cache[s_activeSlot].state;
  }
  unlock();
  return st;
}

const char* PlanePhoto_GetPhotographer() {
  lock();
  static char s_tmp[36];
  s_tmp[0] = '\0';
  if (s_activeSlot >= 0 && s_activeSlot < PHOTO_CACHE_N) {
    strncpy(s_tmp, s_cache[s_activeSlot].photographer, sizeof(s_tmp) - 1);
  }
  unlock();
  return s_tmp;
}

const uint16_t* PlanePhoto_GetRgb565(int* outW, int* outH) {
  lock();
  const uint16_t* ptr = nullptr;
  if (outW) *outW = 0;
  if (outH) *outH = 0;
  if (s_activeSlot >= 0 && s_activeSlot < PHOTO_CACHE_N) {
    if (s_cache[s_activeSlot].state == PHOTO_OK && s_cache[s_activeSlot].rgb565) {
      ptr = s_cache[s_activeSlot].rgb565;
      if (outW) *outW = s_cache[s_activeSlot].width;
      if (outH) *outH = s_cache[s_activeSlot].height;
    }
  }
  unlock();
  return ptr;
}

// Perform query to Planespotters.net API
static bool queryPlanespotters(const char* kind, const char* value, char* thumbUrl, size_t thumbUrlCap, char* photographer, size_t photogCap) {
  if (!value || !*value) return false;

  char url[128];
  snprintf(url, sizeof(url), "https://api.planespotters.net/pub/photos/%s/%s", kind, value);

  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(7000);
  http.setReuse(false);

  if (!http.begin(client, url)) return false;
  http.setUserAgent(HTTP_USER_AGENT);
  http.addHeader("Accept", "application/json");

  Watchdog_Feed();
  int code = http.GET();
  if (code != 200) {
    while (client.available()) client.read();
    http.end();
    client.stop();
    delay(50);
    return false;
  }

  char jsonBuf[2048];
  long got = Net_ReadBody(http, (uint8_t*)jsonBuf, sizeof(jsonBuf), "PHOTO_API");
  while (client.available()) client.read();
  http.end();
  client.stop();
  delay(50);
  if (got <= 0) return false;

  JsonDocument filter;
  filter["photos"][0]["thumbnail"]["src"] = true;
  filter["photos"][0]["photographer"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonBuf, DeserializationOption::Filter(filter));

  if (err || !doc["photos"].is<JsonArrayConst>() || doc["photos"].size() == 0) {
    return false;
  }

  JsonVariantConst p = doc["photos"][0];
  const char* src = p["thumbnail"]["src"].is<const char*>() ? p["thumbnail"]["src"].as<const char*>() : nullptr;
  const char* photog = p["photographer"].is<const char*>() ? p["photographer"].as<const char*>() : nullptr;

  if (!src || !*src) return false;

  strncpy(thumbUrl, src, thumbUrlCap - 1);
  thumbUrl[thumbUrlCap - 1] = '\0';

  if (photog && *photog) {
    strncpy(photographer, photog, photogCap - 1);
    photographer[photogCap - 1] = '\0';
  } else {
    photographer[0] = '\0';
  }

  return true;
}

// Download thumbnail JPEG image into memory
static bool downloadJpeg(const char* url, uint8_t* dstBuf, size_t* dstLen) {
  if (!url || !*url) return false;

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(7000);
  http.setReuse(false);

  bool isHttps = (strncmp(url, "https://", 8) == 0);
  WiFiClientSecure clientSecure;
  WiFiClient clientPlain;

  if (isHttps) {
    clientSecure.setInsecure();
    clientSecure.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);
    if (!http.begin(clientSecure, url)) return false;
  } else {
    if (!http.begin(clientPlain, url)) return false;
  }

  http.setUserAgent(HTTP_USER_AGENT);

  Watchdog_Feed();
  int code = http.GET();
  if (code != 200) {
    if (isHttps) {
      while (clientSecure.available()) clientSecure.read();
      http.end();
      clientSecure.stop();
    } else {
      while (clientPlain.available()) clientPlain.read();
      http.end();
      clientPlain.stop();
    }
    delay(50);
    return false;
  }

  long got = Net_ReadBody(http, dstBuf, PHOTO_BUF_MAX, "PHOTO");
  if (isHttps) {
    while (clientSecure.available()) clientSecure.read();
    http.end();
    clientSecure.stop();
  } else {
    while (clientPlain.available()) clientPlain.read();
    http.end();
    clientPlain.stop();
  }
  delay(50);

  if (got > 500) {
    *dstLen = (size_t)got;
    return true;
  }
  return false;
}

void PlanePhoto_Tick() {
  lock();
  if (!s_pending || s_activeSlot < 0 || s_activeSlot >= PHOTO_CACHE_N) {
    s_pending = false;
    unlock();
    return;
  }
  int slot = s_activeSlot;
  char reg[12], hex[8];
  strncpy(reg, s_cache[slot].reg, sizeof(reg) - 1);
  strncpy(hex, s_cache[slot].hex, sizeof(hex) - 1);
  unlock();

  char thumbUrl[256] = "";
  char photog[36] = "";
  bool found = false;

  if (reg[0]) {
    found = queryPlanespotters("reg", reg, thumbUrl, sizeof(thumbUrl), photog, sizeof(photog));
  }
  if (!found && hex[0]) {
    found = queryPlanespotters("hex", hex, thumbUrl, sizeof(thumbUrl), photog, sizeof(photog));
  }

  if (found && thumbUrl[0]) {
    if (!s_tempJpeg) {
      s_tempJpeg = (uint8_t*)heap_caps_malloc(PHOTO_BUF_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }

    if (s_tempJpeg) {
      // Prefer plain HTTP thumbnail CDN to skip redundant TLS handshake
      char fetchUrl[256];
      strncpy(fetchUrl, thumbUrl, sizeof(fetchUrl) - 1);
      fetchUrl[sizeof(fetchUrl) - 1] = '\0';
      if (strncmp(fetchUrl, "https://t.plnspttrs.net/", 24) == 0) {
        char rewritten[256];
        snprintf(rewritten, sizeof(rewritten), "http://t.plnspttrs.net/%s", fetchUrl + 24);
        strncpy(fetchUrl, rewritten, sizeof(fetchUrl) - 1);
      }

      size_t jpegLen = 0;
      if (downloadJpeg(fetchUrl, s_tempJpeg, &jpegLen) || downloadJpeg(thumbUrl, s_tempJpeg, &jpegLen)) {
        Watchdog_Feed();
        int w = 0, h = 0, channels = 0;
        stbi_uc* rgb = stbi_load_from_memory(s_tempJpeg, (int)jpegLen, &w, &h, &channels, 3);
        if (rgb && w > 0 && h > 0 && w <= 320 && h <= 240) {
          size_t pixelCount = (size_t)w * (size_t)h;
          uint16_t* rgb565 = (uint16_t*)heap_caps_malloc(pixelCount * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
          if (rgb565) {
            for (size_t i = 0; i < pixelCount; i++) {
              uint8_t r = rgb[3 * i];
              uint8_t g = rgb[3 * i + 1];
              uint8_t b = rgb[3 * i + 2];
              rgb565[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            }
            stbi_image_free(rgb);

            lock();
            if (s_cache[slot].rgb565) heap_caps_free(s_cache[slot].rgb565);
            s_cache[slot].rgb565 = rgb565;
            s_cache[slot].width = w;
            s_cache[slot].height = h;
            strncpy(s_cache[slot].photographer, photog, sizeof(s_cache[slot].photographer) - 1);
            s_cache[slot].state = PHOTO_OK;
            s_cache[slot].stamp = millis();
            s_pending = false;
            s_changed = true;
            unlock();
            return;
          }
          stbi_image_free(rgb);
        } else if (rgb) {
          stbi_image_free(rgb);
        }
      }
    }
  }

  // No photo found or decode failed
  lock();
  s_cache[slot].state = PHOTO_NONE;
  s_cache[slot].stamp = millis();
  s_pending = false;
  s_changed = true;
  unlock();
}
