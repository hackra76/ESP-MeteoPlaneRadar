// =============================================================================
//  MeteoPlaneRadar
//  GithubOTA.cpp - GitHub Releases Over-The-Air firmware update engine.
//
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (ESP32-S3R8)
// =============================================================================
#include "GithubOTA.h"
#include "Config.h"
#include "Version.h"
#include "Watchdog.h"
#include "AsyncCore.h"
#include "NetSink.h"
#include "UI.h"
#include "Lang.h"
#include "Display_ST7701.h"
#include "RainViewer.h"
#include "CHMU.h"
#include "SHMU.h"
#include "ScreenWeather.h"
#include "ScreenTactical.h"
#include "PlanePhoto.h"
#include "Route.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_heap_caps.h>

static volatile GithubOtaState s_otaState = GH_OTA_IDLE;
static volatile int            s_otaProgress = 0;
static volatile size_t         s_bytesWritten = 0;
static volatile size_t         s_totalBytes = 0;

static String s_latestTag = "";
static String s_releaseTitle = "";
static String s_releaseBody = "";
static String s_downloadUrl = "";
static String s_otaError = "";

static TaskHandle_t s_checkTaskHandle = nullptr;
static TaskHandle_t s_updateTaskHandle = nullptr;

void GithubOTA_Init() {
  s_otaState = GH_OTA_IDLE;
  s_otaProgress = 0;
  s_bytesWritten = 0;
  s_totalBytes = 0;
}

GithubOtaState GithubOTA_GetState() {
  return s_otaState;
}

const char* GithubOTA_GetStateStr() {
  switch (s_otaState) {
    case GH_OTA_CHECKING:    return "checking";
    case GH_OTA_UP_TO_DATE:  return "up_to_date";
    case GH_OTA_AVAILABLE:   return "available";
    case GH_OTA_DOWNLOADING: return "downloading";
    case GH_OTA_FLASHING:    return "flashing";
    case GH_OTA_SUCCESS:     return "success";
    case GH_OTA_ERROR:       return "error";
    default:                 return "idle";
  }
}

bool GithubOTA_IsUpdateAvailable() {
  return (s_otaState == GH_OTA_AVAILABLE);
}

bool GithubOTA_IsBusy() {
  return (s_otaState == GH_OTA_DOWNLOADING || s_otaState == GH_OTA_FLASHING);
}

int    GithubOTA_GetProgress()      { return s_otaProgress; }
size_t GithubOTA_GetBytesWritten()  { return s_bytesWritten; }
size_t GithubOTA_GetTotalBytes()    { return s_totalBytes; }

const char* GithubOTA_GetLatestVersion() { return s_latestTag.c_str(); }
const char* GithubOTA_GetReleaseTitle()   { return s_releaseTitle.c_str(); }
const char* GithubOTA_GetReleaseBody()    { return s_releaseBody.c_str(); }
const char* GithubOTA_GetDownloadUrl()    { return s_downloadUrl.c_str(); }
const char* GithubOTA_GetError()          { return s_otaError.c_str(); }

void GithubOTA_Reset() {
  if (GithubOTA_IsBusy()) return;
  s_otaState = GH_OTA_IDLE;
  s_otaProgress = 0;
  s_bytesWritten = 0;
  s_totalBytes = 0;
  s_otaError = "";
}

// Compare semantic version strings e.g. "1.5.9" vs "v1.6.0"
static bool isNewerVersion(const char* latest, const char* current) {
  if (!latest || !current) return false;
  if (latest[0] == 'v' || latest[0] == 'V') latest++;
  if (current[0] == 'v' || current[0] == 'V') current++;

  int curMaj = 0, curMin = 0, curPat = 0;
  int latMaj = 0, latMin = 0, latPat = 0;
  sscanf(current, "%d.%d.%d", &curMaj, &curMin, &curPat);
  sscanf(latest, "%d.%d.%d", &latMaj, &latMin, &latPat);

  if (latMaj > curMaj) return true;
  if (latMaj == curMaj && latMin > curMin) return true;
  if (latMaj == curMaj && latMin == curMin && latPat >= curPat) return true;
  return false;
}

// -----------------------------------------------------------------------------
//  Check GitHub Releases
// -----------------------------------------------------------------------------
static bool doCheckGitHubReleases() {
  if (WiFi.status() != WL_CONNECTED) {
    s_otaError = "No WiFi connection";
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  s_otaState = GH_OTA_CHECKING;
  s_otaError = "";

  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(NET_TLS_HANDSHAKE_S);

  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(15000);
  http.setUserAgent(HTTP_USER_AGENT);

  String url = "https://api.github.com/repos/" GITHUB_REPO "/releases/latest";
  if (!http.begin(client, url)) {
    client.stop();
    s_otaError = "Connection failed";
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  http.addHeader("Accept", "application/vnd.github.v3+json");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    while (client.available()) client.read();
    http.end();
    client.stop();
    s_otaError = "HTTP " + String(code);
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  const size_t OTA_JSON_MAX = 65536;
  uint8_t* jsonBuf = (uint8_t*)heap_caps_malloc(OTA_JSON_MAX, MALLOC_CAP_SPIRAM);
  if (!jsonBuf) jsonBuf = (uint8_t*)malloc(OTA_JSON_MAX);
  if (!jsonBuf) {
    while (client.available()) client.read();
    http.end();
    client.stop();
    s_otaError = "No RAM for JSON";
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  long len = Net_ReadBody(http, jsonBuf, OTA_JSON_MAX, "OTA");
  while (client.available()) client.read();
  http.end();
  client.stop();

  if (len <= 0) {
    if (esp_ptr_external_ram(jsonBuf)) heap_caps_free(jsonBuf); else free(jsonBuf);
    s_otaError = "Empty release body";
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  JsonDocument filter;
  filter["tag_name"] = true;
  filter["name"] = true;
  filter["body"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, (char*)jsonBuf, (size_t)len, DeserializationOption::Filter(filter));
  if (esp_ptr_external_ram(jsonBuf)) heap_caps_free(jsonBuf); else free(jsonBuf);

  if (err) {
    Serial.printf("GithubOTA: JSON parse error: %s (len=%ld)\n", err.c_str(), len);
    s_otaError = String("JSON: ") + err.c_str();
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  s_latestTag = (const char*)(doc["tag_name"] | "");
  s_releaseTitle = (const char*)(doc["name"] | "");
  s_releaseBody = (const char*)(doc["body"] | "");
  s_downloadUrl = "";

  String bestUrl = "";
  int bestPriority = 0; // 0 = none, 1 = generic .bin, 2 = firmware.bin, 3 = ota.bin

  JsonArrayConst assets = doc["assets"].as<JsonArrayConst>();
  for (JsonObjectConst a : assets) {
    const char* aname = a["name"] | "";
    const char* url = a["browser_download_url"] | "";
    if (!aname || !url || strlen(url) == 0) continue;

    // Explicitly reject merged / factory flash images (starts at 0x0 with bootloader, cannot be flashed via OTA)
    if (strstr(aname, "factory") || strstr(aname, "Factory") ||
        strstr(aname, "merged")  || strstr(aname, "Merged")) {
      continue;
    }

    if (strstr(aname, "-ota.bin") || strstr(aname, "_ota.bin") || strstr(aname, "ota.bin") || strstr(aname, "OTA.bin")) {
      bestUrl = url;
      bestPriority = 3;
      break;
    } else if (strstr(aname, "firmware.bin") && bestPriority < 2) {
      bestUrl = url;
      bestPriority = 2;
    } else if (strstr(aname, ".bin") && bestPriority < 1) {
      bestUrl = url;
      bestPriority = 1;
    }
  }
  s_downloadUrl = bestUrl;

  Serial.printf("GithubOTA: Current %s, Latest on GitHub: %s, Asset URL: %s\n",
                FW_VERSION, s_latestTag.c_str(), s_downloadUrl.c_str());

  if (s_latestTag.length() > 0 && isNewerVersion(s_latestTag.c_str(), FW_VERSION)) {
    if (s_downloadUrl.length() == 0) {
      s_otaError = "No OTA asset found in release";
      s_otaState = GH_OTA_ERROR;
      return false;
    }
    s_otaState = GH_OTA_AVAILABLE;
    return true;
  } else {
    s_otaState = GH_OTA_UP_TO_DATE;
    return true;
  }
}

static void checkTask(void* param) {
  (void)param;
  doCheckGitHubReleases();
  s_checkTaskHandle = nullptr;
  vTaskDelete(NULL);
}

bool GithubOTA_CheckAsync() {
  if (s_otaState == GH_OTA_CHECKING || GithubOTA_IsBusy()) return false;
  if (s_checkTaskHandle != nullptr) return false;

  BaseType_t ret = xTaskCreatePinnedToCore(checkTask, "GhOtaChk", 16384, NULL, 5, &s_checkTaskHandle, 0);
  return (ret == pdPASS);
}

bool GithubOTA_CheckSync() {
  if (GithubOTA_IsBusy()) return false;
  return doCheckGitHubReleases();
}

// -----------------------------------------------------------------------------
//  Free large caches before and during OTA update
// -----------------------------------------------------------------------------
static void clearCachesForOTA() {
  RainViewer_FreeBuffers();
  CHMU_FreeBuffers();
  SHMU_FreeBuffers();
  ScreenWeather_FreeBuffers();
  ScreenTactical_FreeBuffers();
  PlanePhoto_ClearCache();
  Route_ClearQueue();
  Serial.printf("GithubOTA: Cleared caches. Free SRAM: %u B (largest %u B), Free PSRAM: %u B (largest %u B)\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

// -----------------------------------------------------------------------------
//  Download & Flash Firmware
// -----------------------------------------------------------------------------
static void downloadAndFlashTask(void* param) {
  (void)param;
  s_otaState = GH_OTA_DOWNLOADING;
  s_otaProgress = 0;
  s_bytesWritten = 0;
  s_totalBytes = 0;
  s_otaError = "";

  Async_Pause();
  clearCachesForOTA();

  UI_DrawOtaProgress("GitHub OTA", 0, 0, 0,
                     (Lang_Get() == LANG_EN) ? "Connecting to GitHub..." : "Pripajam sa k GitHubu...");

  String currentUrl = s_downloadUrl;
  if (currentUrl.length() == 0) {
    s_otaError = "No download URL";
    s_otaState = GH_OTA_ERROR;
    Async_Resume();
    s_updateTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
  }

  WiFiClientSecure* client = nullptr;
  HTTPClient* http = nullptr;
  bool connected = false;
  int redirects = 0;

  while (redirects < 6) {
    Watchdog_Feed();

    // Clean up previous connection completely before following redirect
    if (http) {
      while (client && client->available()) client->read();
      http->end();
      delete http;
      http = nullptr;
    }
    if (client) {
      client->stop();
      delete client;
      client = nullptr;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    Serial.printf("GithubOTA: [%d] Free SRAM: %u B (largest %u B)\n",
                  redirects,
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    Serial.printf("GithubOTA: Connecting to %s\n", currentUrl.c_str());

    client = new WiFiClientSecure();
    if (!client) {
      s_otaError = "Client alloc failed";
      break;
    }
    client->setInsecure();
    client->setHandshakeTimeout(NET_TLS_HANDSHAKE_S);

    http = new HTTPClient();
    if (!http) {
      s_otaError = "HTTP alloc failed";
      break;
    }
    http->setConnectTimeout(12000);
    http->setTimeout(30000);
    http->setReuse(false);
    http->setUserAgent(HTTP_USER_AGENT);

    static const char* WANTED_HEADERS[] = { "Location", "Content-Length" };
    http->collectHeaders(WANTED_HEADERS, 2);

    if (!http->begin(*client, currentUrl)) {
      s_otaError = "Connection begin failed";
      break;
    }

    int code = http->GET();
    Serial.printf("GithubOTA: HTTP response code %d (%s)\n", code, HTTPClient::errorToString(code).c_str());

    if (code == 301 || code == 302 || code == 307 || code == 308) {
      String newLoc = http->header("Location");
      if (newLoc.length() == 0) {
        s_otaError = "Empty redirect";
        break;
      }
      currentUrl = newLoc;
      redirects++;
      continue;
    }

    if (code == HTTP_CODE_OK) {
      connected = true;
      break;
    }

    s_otaError = "HTTP " + String(code);
    break;
  }

  if (!connected) {
    if (http) {
      while (client && client->available()) client->read();
      http->end();
      delete http;
      http = nullptr;
    }
    if (client) {
      client->stop();
      delete client;
      client = nullptr;
    }
    s_otaState = GH_OTA_ERROR;
    Async_Resume();
    s_updateTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
  }

  int totalLen = http->getSize();
  s_totalBytes = (totalLen > 0) ? (size_t)totalLen : 0;
  size_t updateSize = (totalLen > 0) ? (size_t)totalLen : UPDATE_SIZE_UNKNOWN;

  UI_DrawOtaProgress("GitHub OTA", 0, 0, (size_t)totalLen,
                     (Lang_Get() == LANG_EN) ? "Downloading firmware..." : "Stahujem firmver...");

  size_t ramCap = (totalLen > 0) ? ((size_t)totalLen + 65536) : 2600000;
  uint8_t* ramBuf = (uint8_t*)heap_caps_malloc(ramCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!ramBuf) {
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (largest > 1000000 && (totalLen <= 0 || largest > (size_t)totalLen + 32768)) {
      ramCap = largest - 65536;
      ramBuf = (uint8_t*)heap_caps_malloc(ramCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
  }

  bool directFlash = false;
  if (!ramBuf) {
    Serial.println("GithubOTA: PSRAM buffer unavailable, falling back to direct flash streaming OTA");
    if (!Update.begin(updateSize)) {
      s_otaError = Update.errorString();
      while (client && client->available()) client->read();
      http->end();
      delete http; http = nullptr;
      delete client; client = nullptr;
      s_otaState = GH_OTA_ERROR;
      UI_DrawOtaProgress("GitHub OTA", 0, 0, 0, s_otaError.c_str());
      Async_Resume();
      s_updateTaskHandle = nullptr;
      vTaskDelete(NULL);
      return;
    }
    directFlash = true;
  }

  size_t downloaded = 0;
  unsigned long lastFeed = millis();
  int lastDrawnProg = -1;
  unsigned long lastDrawnMs = 0;
  WiFiClient* stream = http->getStreamPtr();

  while (http->connected() && (totalLen <= 0 || downloaded < (size_t)totalLen)) {
    size_t avail = stream ? stream->available() : 0;
    if (avail) {
      if (directFlash) {
        uint8_t chunk[512];
        size_t toRead = (avail > sizeof(chunk)) ? sizeof(chunk) : avail;
        int r = stream->readBytes(chunk, toRead);
        if (r > 0) {
          if (Update.write(chunk, (size_t)r) != (size_t)r) {
            s_otaError = Update.errorString();
            Update.abort();
            break;
          }
          downloaded += (size_t)r;
          s_bytesWritten = downloaded;
          if (totalLen > 0) {
            s_otaProgress = (int)(downloaded * 100 / (size_t)totalLen);
          }
          if (s_otaProgress != lastDrawnProg && (millis() - lastDrawnMs >= 150 || s_otaProgress == 100)) {
            lastDrawnProg = s_otaProgress;
            lastDrawnMs = millis();
            UI_DrawOtaProgress("GitHub OTA", s_otaProgress, downloaded, (size_t)totalLen, nullptr);
          }
        }
      } else {
        if (downloaded + avail > ramCap) {
          size_t newCap = ramCap + 524288;
          uint8_t* newBuf = (uint8_t*)heap_caps_realloc(ramBuf, newCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
          if (!newBuf) {
            s_otaError = "PSRAM realloc failed";
            break;
          }
          ramBuf = newBuf;
          ramCap = newCap;
        }
        int r = stream->readBytes(ramBuf + downloaded, avail);
        if (r > 0) {
          downloaded += (size_t)r;
          s_bytesWritten = downloaded;
          if (totalLen > 0) {
            s_otaProgress = (int)(downloaded * 100 / (size_t)totalLen);
          }
          // Smooth 100% stable progress on display!
          if (s_otaProgress != lastDrawnProg && (millis() - lastDrawnMs >= 150 || s_otaProgress == 100)) {
            lastDrawnProg = s_otaProgress;
            lastDrawnMs = millis();
            UI_DrawOtaProgress("GitHub OTA", s_otaProgress, downloaded, (size_t)totalLen, nullptr);
          }
        }
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (millis() - lastFeed > 800) {
      Watchdog_Feed();
      lastFeed = millis();
    }
  }

  // Network transfer complete - tear down client and http before writing flash
  if (http) {
    while (client && client->available()) client->read();
    http->end();
    delete http;
    http = nullptr;
  }
  if (client) {
    client->stop();
    delete client;
    client = nullptr;
  }

  if (totalLen > 0 && downloaded < (size_t)totalLen && s_otaError.length() == 0) {
    s_otaError = "Incomplete download";
  }

  if (directFlash) {
    if (s_otaError.length() == 0 && Update.end(true)) {
      s_otaProgress = 100;
      s_otaState = GH_OTA_SUCCESS;
      Serial.printf("GithubOTA: Direct flash write complete (%u B), restarting...\n", (unsigned)downloaded);
      UI_DrawOtaWritingStaticScreen("GitHub OTA", (Lang_Get() == LANG_EN) ? "Success! Restarting..." : "Hotovo! Reštartujem...");
      vTaskDelay(pdMS_TO_TICKS(1500));
      Safe_Restart();
    } else {
      if (s_otaError.length() == 0) s_otaError = Update.errorString();
      s_otaState = GH_OTA_ERROR;
      UI_DrawOtaWritingStaticScreen("GitHub OTA", s_otaError.c_str());
      vTaskDelay(pdMS_TO_TICKS(3000));
      Async_Resume();
    }
    s_updateTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
  }

  if (s_otaError.length() == 0 && downloaded > 0) {
    s_otaState = GH_OTA_FLASHING;
    Serial.printf("GithubOTA: Download complete (%u bytes in PSRAM). Writing to flash...\n", (unsigned)downloaded);
    UI_DrawOtaWritingStaticScreen("GitHub OTA");
    vTaskDelay(pdMS_TO_TICKS(150));

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      s_otaError = Update.errorString();
    } else {
      size_t written = 0;
      const size_t CHUNK_SZ = 16384;
      while (written < downloaded) {
        size_t toWrite = (downloaded - written > CHUNK_SZ) ? CHUNK_SZ : (downloaded - written);
        if (Update.write(ramBuf + written, toWrite) != toWrite) {
          s_otaError = Update.errorString();
          Update.abort();
          break;
        }
        written += toWrite;
        vTaskDelay(pdMS_TO_TICKS(20)); // Give ST7701 RGB DMA uninterrupted access to PSRAM between flash writes
        Watchdog_Feed();
      }
      if (s_otaError.length() == 0 && Update.end(true)) {
        s_otaProgress = 100;
        s_otaState = GH_OTA_SUCCESS;
        Serial.printf("GithubOTA: Update successful (%u bytes). Restarting...\n", (unsigned)downloaded);
        UI_DrawOtaWritingStaticScreen("GitHub OTA", (Lang_Get() == LANG_EN) ? "Success! Restarting..." : "Hotovo! Reštartujem...");
        vTaskDelay(pdMS_TO_TICKS(1500));
        Safe_Restart();
      }
    }
  }

  if (ramBuf) {
    heap_caps_free(ramBuf);
    ramBuf = nullptr;
  }

  if (s_otaState != GH_OTA_SUCCESS) {
    if (s_otaError.length() == 0) s_otaError = Update.errorString();
    s_otaState = GH_OTA_ERROR;
    UI_DrawOtaWritingStaticScreen("GitHub OTA", s_otaError.c_str());
    vTaskDelay(pdMS_TO_TICKS(3000));
    Async_Resume();
  }

  s_updateTaskHandle = nullptr;
  vTaskDelete(NULL);
}

bool GithubOTA_StartUpdateAsync(const char* url, const char* tag) {
  if (GithubOTA_IsBusy() || Update.isRunning()) return false;
  if (s_updateTaskHandle != nullptr) return false;

  if (url && strlen(url) > 0) s_downloadUrl = url;
  if (tag && strlen(tag) > 0) s_latestTag = tag;

  if (s_downloadUrl.length() == 0) {
    s_otaError = "Missing download URL";
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  if (strstr(s_downloadUrl.c_str(), "factory") || strstr(s_downloadUrl.c_str(), "Factory") ||
      strstr(s_downloadUrl.c_str(), "merged")  || strstr(s_downloadUrl.c_str(), "Merged")) {
    s_otaError = "Cannot flash factory binary via OTA";
    s_otaState = GH_OTA_ERROR;
    return false;
  }

  clearCachesForOTA();

  BaseType_t ret = xTaskCreatePinnedToCore(downloadAndFlashTask, "GhOtaUpd", 14336, NULL, 5, &s_updateTaskHandle, 0);
  return (ret == pdPASS);
}
