// =============================================================================
//  MeteoPlaneRadar
//  The configuration web server. See WebConfig.h.
//
// =============================================================================
#include "WebConfig.h"
#include "WebPage.h"
#include "ScreenPlanes.h"
#include "ScreenWeather.h"
#include "ScreenTactical.h"
#include "Settings.h"
#include "Status.h"
#include "Version.h"
#include "Config.h"
#include "Lang.h"
#include "Net.h"
#include "Forecast.h"
#include "CHMU.h"
#include "SHMU.h"
#include "RainViewer.h"
#include "NightMode.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Watchdog.h"
#include "QMI8658.h"
#include "AsyncCore.h"
#include "PCF85063.h"
#include "Outside.h"
#include "TimeUtil.h"
#include "Buzzer.h"
#include "GithubOTA.h"
#include "FlightStats.h"
#include "PrecipTracker.h"
#include <Wire.h>

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <math.h>

static WebServer  s_srv(WEB_PORT);
static DNSServer  s_dns;
static bool s_apMode = false;
static bool s_running = false;
static bool s_wantConnect = false;
static bool s_wantRestart = false;
static volatile bool s_updating = false;

// Queued remote-control requests - see the note in WebConfig.h.
static int s_reqScreen     = -1;
static int s_reqScreenStep = 0;
static int s_reqRangeStep  = 0;
static bool s_reqRedraw    = false;
static bool s_reqSelectPlane = false;

bool WebConfig_UpdateBusy()        { return s_updating || GithubOTA_IsBusy(); }
bool WebConfig_WantsWifiConnect()  { return s_wantConnect; }
void WebConfig_ClearWifiConnect()  { s_wantConnect = false; }
bool WebConfig_WantsRestart()      { return s_wantRestart; }

int  WebConfig_TakeScreen()     { int v = s_reqScreen;     s_reqScreen = -1;    return v; }
int  WebConfig_TakeScreenStep() { int v = s_reqScreenStep; s_reqScreenStep = 0; return v; }
int  WebConfig_TakeRangeStep()  { int v = s_reqRangeStep;  s_reqRangeStep = 0;  return v; }
bool WebConfig_TakeRedraw()     { bool r = s_reqRedraw;    s_reqRedraw = false; return r; }
void WebConfig_RequestRedraw()  { s_reqRedraw = true; }
bool WebConfig_TakeSelectPlane() { bool r = s_reqSelectPlane; s_reqSelectPlane = false; return r; }

// --- Helpers ----------------------------------------------------------------
static void sendJson(int code, JsonDocument& doc) {
  size_t len = measureJson(doc);
  String out;
  if (out.reserve(len + 1)) {
    serializeJson(doc, out);
    s_srv.send(code, "application/json", out);
  } else {
    s_srv.send(500, "text/plain", "OOM");
  }
}

static bool readBody(JsonDocument& doc) {
  if (!s_srv.hasArg("plain")) return false;
  return deserializeJson(doc, s_srv.arg("plain")) == DeserializationError::Ok;
}

// Every destructive endpoint goes through here. When no password is set this
// waves everything through - that is the documented default, and the device
// only listens on the home LAN.
static bool authed(JsonDocument& body) {
  const char* pw = body["password"] | "";
  if (Settings_CheckAdminPassword(pw)) return true;
  s_srv.send(403, "application/json", "{\"error\":\"password\"}");
  return false;
}

// --- Endpoints --------------------------------------------------------------
static void handleRoot() {
  s_srv.sendHeader("Cache-Control", "no-store");
  // The charset belongs in the HEADER, not only in a meta tag: a browser that
  // reads the header first would otherwise guess, and the Czech accents on the
  // page would come out as mojibake.
  s_srv.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

static void handleGetConfig() {
  JsonDocument doc;
  JsonObject o = doc.to<JsonObject>();
  Settings_ToJson(o);
  o["version"] = FW_VERSION;
  o["apMode"]  = s_apMode;
  o["ip"]      = s_apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  sendJson(200, doc);
}

static void handlePostConfig() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }

  // The password is write-only and never round-trips through the page, so it is
  // handled apart from the rest of the settings.
  //
  // Changing it requires the current one. Without that check anyone who can
  // reach the page could set a password and lock the owner out of the update
  // and reset endpoints - the settings themselves are deliberately open, but
  // that must not be a way to take the device over.
  JsonVariantConst np = doc["newPassword"];
  if (!np.isNull()) {
    const char* p = np.as<const char*>();
    if (p && *p) {
      if (!authed(doc)) return;                 // sends 403 and explains itself
      Settings_SetAdminPassword(p);
      // The update page checks the password on every request, so a new one is
      // live immediately - no restart needed.
    }
  }

  const double oldLat = Settings_Lat(), oldLon = Settings_Lon();
  const uint8_t oldSrc = Settings_RadarSource();
  const uint8_t oldMask = (Settings_ScreenEnabled(SCREEN_CLOCK_I) << 0) |
                          (Settings_ScreenEnabled(SCREEN_PLANES_I) << 1) |
                          (Settings_ScreenEnabled(SCREEN_METEO_I) << 2) |
                          (Settings_ScreenEnabled(SCREEN_TACTICAL_I) << 3) |
                          (Settings_ScreenEnabled(SCREEN_FORECAST_I) << 4) |
                          (Settings_ScreenEnabled(SCREEN_INFO_I) << 5);

  Settings_FromJson(doc.as<JsonObjectConst>());
  s_reqRedraw = true;

  // Applied straight away - these are the ones you want to see change while
  // you are still looking at the slider.
  NightMode_Apply();

  const uint8_t newMask = (Settings_ScreenEnabled(SCREEN_CLOCK_I) << 0) |
                          (Settings_ScreenEnabled(SCREEN_PLANES_I) << 1) |
                          (Settings_ScreenEnabled(SCREEN_METEO_I) << 2) |
                          (Settings_ScreenEnabled(SCREEN_TACTICAL_I) << 3) |
                          (Settings_ScreenEnabled(SCREEN_FORECAST_I) << 4) |
                          (Settings_ScreenEnabled(SCREEN_INFO_I) << 5);
  const bool moved = (fabs(oldLat - Settings_Lat()) > 1e-6) ||
                     (fabs(oldLon - Settings_Lon()) > 1e-6);
  if (moved) Forecast_Invalidate();

  // Handle radar source change cleanly at runtime without requiring an ESP restart
  if (oldSrc != Settings_RadarSource()) {
    Async_RequestRadar();
  }

  // Location change or enabled screen set change requires a clean start / redraw
  if (moved || oldMask != newMask)
    s_wantRestart = true;

  JsonDocument res;
  res["ok"] = true;
  res["restart"] = s_wantRestart;
  sendJson(200, res);
}

// Jump to a screen, or step one along. Disabled screens are refused rather than
// silently ignored, so the page can say why nothing happened.
static void handleScreen() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }

  JsonVariantConst idx = doc["index"];
  if (!idx.isNull()) {
    int i = idx.as<int>();
    if (i < 0 || i >= SCREEN_N) {
      s_srv.send(400, "application/json", "{\"error\":\"range\"}");
      return;
    }
    if (!Settings_ScreenEnabled((uint8_t)i)) {
      s_srv.send(409, "application/json", "{\"error\":\"disabled\"}");
      return;
    }
    s_reqScreen = i;
  } else {
    int st = doc["step"] | 0;
    s_reqScreenStep = (st < 0) ? -1 : ((st > 0) ? +1 : 0);
  }
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

// Change the range on whichever radar screen is showing. The screens that have
// no range simply ignore it, exactly as a swipe does.
static void handleRange() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  int st = doc["step"] | 0;
  s_reqRangeStep = (st < 0) ? -1 : ((st > 0) ? +1 : 0);
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

static void handleStatus() {
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  doc["ip"]   = s_apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["ssid"] = s_apMode ? String(AP_SSID) : WiFi.SSID();
  doc["rssi"] = s_apMode ? 0 : WiFi.RSSI();

  unsigned long up = millis() / 1000UL;
  char ub[32];
  snprintf(ub, sizeof(ub), "%lud %02lu:%02lu:%02lu",
           up / 86400UL, (up / 3600UL) % 24UL, (up / 60UL) % 60UL, up % 60UL);
  doc["uptime"] = ub;

  doc["heap"]  = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  doc["psram"] = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  const char* rr = "?";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  rr = "power on"; break;
    case ESP_RST_SW:       rr = "software"; break;
    case ESP_RST_PANIC:    rr = "PANIC"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      rr = "WATCHDOG"; break;
    case ESP_RST_BROWNOUT: rr = "BROWNOUT"; break;
    case ESP_RST_EXT:      rr = "reset pin"; break;
    default: break;
  }
  doc["resetReason"] = rr;

  // What the device is showing right now, so the remote control can highlight
  // the active screen and print the range instead of guessing.
  const uint8_t scr = Settings_Screen();
  doc["screen"] = scr;
  char rb[24] = "";
  if      (scr == SCREEN_PLANES_I)   ScreenPlanes_RangeText(rb, sizeof(rb));
  else if (scr == SCREEN_METEO_I)    ScreenWeather_RangeText(rb, sizeof(rb));
  else if (scr == SCREEN_TACTICAL_I) ScreenTactical_RangeText(rb, sizeof(rb));
  doc["range"] = rb;                       // empty on screens without one

  JsonArray en = doc["enabled"].to<JsonArray>();
  for (uint8_t i = 0; i < SCREEN_N; i++) en.add(Settings_ScreenEnabled(i));

  char b[64];
  Status_Text(ST_ADSB, b, sizeof(b));     doc["adsb"] = b;
  Status_Text(ST_RADAR, b, sizeof(b));    doc["radar"] = b;
  Status_Text(ST_FORECAST, b, sizeof(b)); doc["forecast"] = b;

  char pb[64] = "";
  PrecipTracker_GetStatusText(pb, sizeof(pb));
  doc["precip"] = pb;
  const PrecipAlert* pa = PrecipTracker_GetAlert();
  doc["precipStatus"]  = pa ? (int)pa->status : 0;
  doc["precipType"]    = pa ? (int)pa->type : 0;
  doc["precipEta"]     = pa ? pa->etaMin : 0;
  doc["precipDist"]    = pa ? (int)roundf(pa->distKm) : 0;
  doc["precipSpeed"]   = pa ? (int)roundf(pa->speedKmh) : 0;
  doc["precipBearing"] = pa ? PrecipTracker_GetBearingStr(pa->bearingDeg) : "-";

  sendJson(200, doc);
}

static void handleHardware() {
  JsonDocument doc;
  doc["cpuFreq"] = getCpuFrequencyMhz();
  doc["cpuModel"] = ESP.getChipModel();
  doc["cpuRev"] = ESP.getChipRevision();
  doc["cpuCores"] = ESP.getChipCores();
  doc["cpuTemp"] = (int)round(temperatureRead());
  doc["version"] = FW_VERSION;

  uint32_t heapFree = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  uint32_t heapTotal = (uint32_t)heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  uint32_t psramFree = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  uint32_t psramTotal = ESP.getPsramSize();
  doc["heapFree"] = heapFree;
  doc["heapTotal"] = heapTotal;
  doc["psramFree"] = psramFree;
  doc["psramTotal"] = psramTotal;
  doc["flashSize"] = (uint32_t)ESP.getFlashChipSize();
  doc["flashSpeed"] = (uint32_t)ESP.getFlashChipSpeed();

  doc["ssid"] = s_apMode ? String(AP_SSID) : WiFi.SSID();
  doc["rssi"] = s_apMode ? 0 : WiFi.RSSI();
  doc["bssid"] = s_apMode ? String("-") : WiFi.BSSIDstr();
  doc["ip"] = s_apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["gw"] = s_apMode ? String("-") : WiFi.gatewayIP().toString();
  doc["mask"] = s_apMode ? String("-") : WiFi.subnetMask().toString();
  doc["dns"] = s_apMode ? String("-") : WiFi.dnsIP().toString();
  doc["mac"] = WiFi.macAddress();

  doc["imuOk"] = QMI8658_Available();
  if (QMI8658_Available()) {
    QMI_Data d;
    QMI8658_GetData(&d);
    doc["ax"] = (int)round(d.ax * 100) / 100.0;
    doc["ay"] = (int)round(d.ay * 100) / 100.0;
    doc["az"] = (int)round(d.az * 100) / 100.0;
    doc["gx"] = (int)round(d.gx);
    doc["gy"] = (int)round(d.gy);
    doc["gz"] = (int)round(d.gz);
    doc["pitch"] = (int)round(d.pitch);
    doc["roll"] = (int)round(d.roll);
  }

  doc["dispDriver"] = "ST7701 (480x480 RGB 16-bit)";
  doc["touchDriver"] = "CHSC6540 / CST820 (I2C)";
  doc["expander"] = "TCA9554 (I2C 0x20 / 0x43)";

  // Local time and timezone offset details
  time_t now = time(nullptr);
  struct tm localTm;
  localtime_r(&now, &localTm);
  char locBuf[36];
  snprintf(locBuf, sizeof(locBuf), "%02d:%02d:%02d (%02d.%02d.%04d)",
           localTm.tm_hour, localTm.tm_min, localTm.tm_sec,
           localTm.tm_mday, localTm.tm_mon + 1, localTm.tm_year + 1900);
  doc["localTime"] = locBuf;

  // Calculate local timezone offset vs UTC
  time_t localAsUtc = TimeUtil_UtcToEpoch(localTm.tm_year + 1900, localTm.tm_mon + 1, localTm.tm_mday,
                                          localTm.tm_hour, localTm.tm_min, localTm.tm_sec);
  long offsetSec = (long)(localAsUtc - now);
  int offHours = offsetSec / 3600;
  int offMins = abs((offsetSec % 3600) / 60);
  char offStr[40];
  const char* tzAbbr = tzname[localTm.tm_isdst > 0 ? 1 : 0];
  if (tzAbbr && *tzAbbr) {
    snprintf(offStr, sizeof(offStr), "UTC%+03d:%02d (%s%s)",
             offHours, offMins, tzAbbr, localTm.tm_isdst > 0 ? " - letný čas" : "");
  } else {
    snprintf(offStr, sizeof(offStr), "UTC%+03d:%02d%s",
             offHours, offMins, localTm.tm_isdst > 0 ? " (letný čas)" : "");
  }
  doc["tzOffset"] = offStr;
  doc["isDst"] = (localTm.tm_isdst > 0);

  // RTC PCF85063 details (stores UTC)
  doc["rtcDetected"] = PCF85063_IsDetected();
  doc["rtcOscStopped"] = PCF85063_IsOscillatorStopped();
  struct tm rtcTm;
  if (PCF85063_ReadTime(&rtcTm)) {
    char rtcBuf[40];
    snprintf(rtcBuf, sizeof(rtcBuf), "%02d:%02d:%02d (%02d.%02d.%04d) UTC",
             rtcTm.tm_hour, rtcTm.tm_min, rtcTm.tm_sec,
             rtcTm.tm_mday, rtcTm.tm_mon + 1, rtcTm.tm_year + 1900);
    doc["rtcTime"] = rtcBuf;
  } else {
    doc["rtcTime"] = "-";
  }


  unsigned long up = millis() / 1000UL;
  char ub[32];
  snprintf(ub, sizeof(ub), "%lud %02lu:%02lu:%02lu",
           up / 86400UL, (up / 3600UL) % 24UL, (up / 60UL) % 60UL, up % 60UL);
  doc["uptime"] = ub;

  const char* rr = "?";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  rr = "power on"; break;
    case ESP_RST_SW:       rr = "software"; break;
    case ESP_RST_PANIC:    rr = "PANIC"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      rr = "WATCHDOG"; break;
    case ESP_RST_BROWNOUT: rr = "BROWNOUT"; break;
    case ESP_RST_EXT:      rr = "reset pin"; break;
    default: break;
  }
  doc["resetReason"] = rr;

  sendJson(200, doc);
}

static void handleRtcSyncNtp() {
  if (Outside_TimeValid()) {
    time_t now = time(nullptr);
    PCF85063_SetTime(now);
    JsonDocument res; res["ok"] = true; res["time"] = (long)now;
    sendJson(200, res);
  } else {
    s_srv.send(400, "application/json", "{\"ok\":false,\"error\":\"NTP čas nie je k dispozícii\"}");
  }
}

static void handleRtcSyncBrowser() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  long epoch = doc["epoch"] | 0L;
  if (epoch > 1700000000L) {
    struct timeval tv = { (time_t)epoch, 0 };
    settimeofday(&tv, nullptr);
    PCF85063_SetTime((time_t)epoch);
    JsonDocument res; res["ok"] = true;
    sendJson(200, res);
  } else {
    s_srv.send(400, "application/json", "{\"ok\":false,\"error\":\"Neplatný čas\"}");
  }
}

static void handleScan() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n && i < 20; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
  }
  WiFi.scanDelete();
  sendJson(200, doc);
}

static void handleWifi() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  const char* ssid = doc["ssid"] | "";
  const char* pass = doc["pass"] | "";
  if (!*ssid) { s_srv.send(400, "application/json", "{\"error\":\"ssid\"}"); return; }
  Settings_SetWifi(ssid, pass);
  s_wantConnect = true;
  JsonDocument res; res["ok"] = true;
  sendJson(200, res);
}

static void handleWifiDelete() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  if (!authed(doc)) return;
  const char* ssid = doc["ssid"] | "";
  if (!*ssid) { s_srv.send(400, "application/json", "{\"error\":\"ssid\"}"); return; }
  bool ok = Settings_DeleteWifiNetworkBySsid(ssid);
  JsonDocument res; res["ok"] = ok;
  sendJson(200, res);
}

// Town name -> coordinates, so nobody has to look up their latitude by hand.
// Proxied through the device because the page is served from the device and a
// browser would refuse the cross-origin call.
static void handleGeocode() {
  String q = s_srv.arg("q");
  if (q.length() == 0) { s_srv.send(400, "application/json", "[]"); return; }

  String enc;
  for (size_t i = 0; i < q.length(); i++) {
    char ch = q[i];
    if (isalnum((unsigned char)ch)) enc += ch;
    else { char b[4]; snprintf(b, sizeof(b), "%%%02X", (unsigned char)ch); enc += b; }
  }

  char url[220];
  const uint8_t curLang = Lang_Get();
  const char* langCode = (curLang == LANG_EN) ? "en" : ((curLang == LANG_SK) ? "sk" : "cs");
  snprintf(url, sizeof(url), "%s?name=%s&count=8&language=%s&format=json",
           GEOCODE_URL, enc.c_str(), langCode);

  String body;
  if (!Net_GetString(url, body, "GEOCODE")) { s_srv.send(502, "application/json", "[]"); return; }

  JsonDocument filter;
  JsonObject f = filter["results"].add<JsonObject>();
  f["name"] = true; f["latitude"] = true; f["longitude"] = true; f["country"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
    s_srv.send(502, "application/json", "[]");
    return;
  }

  JsonDocument out;
  JsonArray arr = out.to<JsonArray>();
  for (JsonObjectConst r : doc["results"].as<JsonArrayConst>()) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = r["name"];
    o["country"] = r["country"];
    o["lat"] = r["latitude"];
    o["lon"] = r["longitude"];
  }
  sendJson(200, out);
}


static void handleToggleLegends() {
  if (Settings_Screen() == SCREEN_CLOCK_I) {
    uint8_t nextStyle = (Settings_ClockStyle() + 1) % (CLOCK_STYLE_MAX + 1);
    Settings_SetClockStyle(nextStyle);
    s_reqRedraw = true;
  } else {
    Settings_ToggleLegends();
  }
  JsonDocument res;
  res["ok"] = true;
  res["legends"] = Settings_ShowLegends();
  res["clockStyle"] = Settings_ClockStyle();
  sendJson(200, res);
}

static void handleInput() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  const char* cmd = doc["cmd"] | "";
  if (strcmp(cmd, "toggle_legends") == 0 || strcmp(cmd, "dbl_tap") == 0) {
    if (Settings_Screen() == SCREEN_CLOCK_I) {
      uint8_t nextStyle = (Settings_ClockStyle() + 1) % (CLOCK_STYLE_MAX + 1);
      Settings_SetClockStyle(nextStyle);
      s_reqRedraw = true;
    } else {
      Settings_ToggleLegends();
    }
  } else if (strcmp(cmd, "swipe_left") == 0 || strcmp(cmd, "range_plus") == 0) {
    s_reqRangeStep = +1;
  } else if (strcmp(cmd, "swipe_right") == 0 || strcmp(cmd, "range_minus") == 0) {
    s_reqRangeStep = -1;
  } else if (strcmp(cmd, "next_screen") == 0) {
    s_reqScreenStep = +1;
  } else if (strcmp(cmd, "prev_screen") == 0) {
    s_reqScreenStep = -1;
  } else if (strcmp(cmd, "select_plane") == 0) {
    s_reqSelectPlane = true;
  }
  JsonDocument res; res["ok"] = true;
  res["legends"] = Settings_ShowLegends();
  res["clockStyle"] = Settings_ClockStyle();
  sendJson(200, res);
}

static void handleBuzzerTest() {
  Buzzer_Play(BEEP_WATCHED);
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

static void handleSerialRead() {
  uint32_t since = 0;
  if (s_srv.hasArg("since")) {
    since = (uint32_t)strtoul(s_srv.arg("since").c_str(), nullptr, 10);
  }
  static char s_serialReadBuf[16384];
  uint32_t nextOffset = 0;
  bool overflow = false;
  SerialLog_Read(since, s_serialReadBuf, sizeof(s_serialReadBuf), &nextOffset, &overflow);

  JsonDocument doc;
  doc["head"] = nextOffset;
  doc["overflow"] = overflow;
  doc["data"] = s_serialReadBuf;
  sendJson(200, doc);
}

static void handleSerialClear() {
  SerialLog_Clear();
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

static void handleSerialSend() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  const char* text = doc["text"] | "";
  if (text && strlen(text) > 0) {
    if (strcmp(text, "ping") == 0) {
      Serial.println("[Console] pong");
    } else if (strcmp(text, "heap") == 0 || strcmp(text, "mem") == 0) {
      Serial.printf("[Console] Heap free: %u B, PSRAM free: %u B\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram());
    } else if (strcmp(text, "restart") == 0 || strcmp(text, "reboot") == 0) {
      Serial.println("[Console] Rebooting...");
      delay(200);
      ESP.restart();
    } else {
      Serial.printf("[Console] %s\n", text);
    }
  }
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

static void handleScreenshot() {
  const uint16_t* fb = LCD_GetActiveBuffer();
  if (!fb) {
    s_srv.send(503, "text/plain", "Display buffer not available");
    return;
  }
  WiFiClient client = s_srv.client();
  if (!client) {
    s_srv.send(500, "text/plain", "Client error");
    return;
  }

  const uint32_t width = LCD_WIDTH;
  const uint32_t height = LCD_HEIGHT;
  const uint32_t rowSize = width * 3;
  const uint32_t imageSize = rowSize * height;
  const uint32_t fileSize = 54 + imageSize;

  uint8_t header[54];
  memset(header, 0, 54);
  header[0] = 'B';
  header[1] = 'M';
  header[2] = (uint8_t)(fileSize);
  header[3] = (uint8_t)(fileSize >> 8);
  header[4] = (uint8_t)(fileSize >> 16);
  header[5] = (uint8_t)(fileSize >> 24);
  header[10] = 54;

  header[14] = 40; // biSize
  header[18] = (uint8_t)(width);
  header[19] = (uint8_t)(width >> 8);
  header[20] = (uint8_t)(width >> 16);
  header[21] = (uint8_t)(width >> 24);
  header[22] = (uint8_t)(height);
  header[23] = (uint8_t)(height >> 8);
  header[24] = (uint8_t)(height >> 16);
  header[25] = (uint8_t)(height >> 24);
  header[26] = 1;  // planes
  header[28] = 24; // bits per pixel
  header[34] = (uint8_t)(imageSize);
  header[35] = (uint8_t)(imageSize >> 8);
  header[36] = (uint8_t)(imageSize >> 16);
  header[37] = (uint8_t)(imageSize >> 24);

  s_srv.sendHeader("Content-Disposition", "inline; filename=\"screenshot.bmp\"");
  s_srv.setContentLength(fileSize);
  s_srv.send(200, "image/bmp", "");

  client.write(header, 54);

  static uint8_t rowBuf[1440];
  for (int y = (int)height - 1; y >= 0; y--) {
    const uint16_t* src = fb + (y * width);
    int p = 0;
    for (uint32_t x = 0; x < width; x++) {
      uint16_t c = src[x];
      // RGB565 -> BGR888
      uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
      uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
      uint8_t b = (c & 0x1F) * 255 / 31;
      rowBuf[p++] = b;
      rowBuf[p++] = g;
      rowBuf[p++] = r;
    }
    client.write(rowBuf, rowSize);
    if ((y & 15) == 0) Watchdog_Feed();
  }
}

static void handleStats() {
  FlightStats_CheckMidnight();
  JsonDocument doc;
  doc["todayCount"] = FlightStats_TodayCount();
  doc["maxDistKm"] = FlightStats_MaxDistKm();
  doc["maxSpeedKt"] = FlightStats_MaxSpeedKt();
  doc["maxSpeedCallsign"] = FlightStats_MaxSpeedCallsign();
  doc["maxAltFt"] = FlightStats_MaxAltFt();
  doc["minAltFt"] = (FlightStats_MinAltFt() > 900000.0f) ? 0.0f : FlightStats_MinAltFt();
  doc["totalSightings"] = FlightStats_TotalSightings();
  sendJson(200, doc);
}

static void handleStatsReset() {
  FlightStats_Reset();
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(200, doc);
}

static void handleExport() {
  JsonDocument doc;
  JsonObject o = doc.to<JsonObject>();
  Settings_ToJson(o);
  o["version"] = FW_VERSION;
  // Deliberately absent: the WiFi password and the admin password. A backup
  // file ends up in a download folder, in an email, in a support ticket.
  String out;
  serializeJsonPretty(doc, out);
  s_srv.sendHeader("Content-Disposition", "attachment; filename=meteoplaneradar.json");
  s_srv.send(200, "application/json", out);
}

static void handleImport() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  if (!authed(doc)) return;
  JsonObjectConst cfg = doc["config"];
  if (cfg.isNull()) { s_srv.send(400, "application/json", "{\"error\":\"config\"}"); return; }
  Settings_FromJson(cfg);
  NightMode_Apply();
  s_wantRestart = true;
  JsonDocument res; res["ok"] = true;
  sendJson(200, res);
}

static void handleReboot() {
  s_srv.send(200, "application/json", "{\"ok\":true}");
  delay(200);
  ESP.restart();
}

static void handleReset() {
  JsonDocument doc;
  readBody(doc);
  if (!authed(doc)) return;
  s_srv.send(200, "application/json", "{\"ok\":true}");
  delay(200);
  Settings_ClearAll();
  ESP.restart();
}

// Captive portal: whatever the phone asks for, hand it the setup page. Without
// this the "sign in to network" banner never appears and the user is left
// typing an IP address they have not been told.
static void handleNotFound() {
  if (s_apMode) {
    s_srv.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    s_srv.send(302, "text/plain", "");
    return;
  }
  s_srv.send(404, "text/plain", "404");
}

// --- OTA callbacks ----------------------------------------------------------
static void otaStart() {
  Async_Pause();
  s_updating = true;
  LCD_SetPclk(5000000);
  LCD_Restart();
  UI_DrawOtaProgress("Web OTA", 0, 0, 0, (Lang_Get() == LANG_EN) ? "Preparing upload..." : "Pripravujem nahrávanie...");
}

static void otaEnd(bool ok) {
  s_updating = false;
  LCD_SetPclk(RGB_FREQ_HZ);
  LCD_Restart();
  Async_Resume();
}

// --- GitHub Online OTA ------------------------------------------------------
static void handleOtaCheck() {
  if (GithubOTA_GetState() == GH_OTA_IDLE) {
    GithubOTA_CheckSync();
  }

  JsonDocument out;
  out["current"] = FW_VERSION;
  out["latest"] = GithubOTA_GetLatestVersion();
  out["updateAvailable"] = GithubOTA_IsUpdateAvailable();
  out["name"] = GithubOTA_GetReleaseTitle();
  out["body"] = GithubOTA_GetReleaseBody();
  out["url"] = GithubOTA_GetDownloadUrl();

  String resp;
  serializeJson(out, resp);
  s_srv.send(200, "application/json", resp);
}

static void handleOtaStatus() {
  JsonDocument doc;
  doc["state"] = GithubOTA_GetStateStr();
  doc["progress"] = GithubOTA_GetProgress();
  doc["error"] = GithubOTA_GetError();
  String resp;
  serializeJson(doc, resp);
  s_srv.send(200, "application/json", resp);
}

static void handleOtaStart() {
  String body = s_srv.hasArg("plain") ? s_srv.arg("plain") : "";
  JsonDocument doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    s_srv.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!authed(doc)) return;

  if (GithubOTA_IsBusy() || s_updating) {
    s_srv.send(409, "application/json", "{\"error\":\"already_running\"}");
    return;
  }

  String url = doc["url"] | "";
  String tag = doc["tag"] | "";

  if (!GithubOTA_StartUpdateAsync(url.length() ? url.c_str() : nullptr, tag.length() ? tag.c_str() : nullptr)) {
    s_srv.send(500, "application/json", "{\"error\":\"start_failed\"}");
    return;
  }

  s_srv.send(200, "application/json", "{\"status\":\"started\"}");
}

// --- Firmware update --------------------------------------------------------
// This replaces the ElegantOTA library. That library is AGPL-3.0, which would
// have made every binary built from this project AGPL too; the Update class
// ships with the ESP32 core and the web server was already here, so the page
// below was the only piece actually missing.

static bool   s_updOk = false;
static String s_updErr;          // empty = no failure yet

// HTTP Basic, and only when a password is set - the open default is documented
// in Settings.h and in the README.
static bool updateAuthed() {
  if (!Settings_HasAdminPassword()) return true;
  return s_srv.authenticate(WEB_ADMIN_USER, Settings_AdminPassword());
}

static const char UPDATE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="{{LANG}}"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>{{TITLE}}</title><style>
body{background:#111;color:#eee;font:16px system-ui,sans-serif;margin:0;padding:24px;
 max-width:520px;margin-inline:auto}
h1{font-size:20px;margin:0 0 4px}
p{color:#aaa;line-height:1.5}
input[type=file]{width:100%;padding:12px;background:#1c1c1c;border:1px solid #333;
 border-radius:8px;color:#eee;box-sizing:border-box}
button{width:100%;padding:14px;margin-top:12px;font-size:16px;border:0;border-radius:8px;
 background:#2d7;color:#000;font-weight:600;cursor:pointer}
button:disabled{background:#444;color:#888;cursor:default}
progress{width:100%;height:10px;margin-top:16px}
#s{margin-top:10px;min-height:1.4em;font-weight:600}
a{color:#5bf}
</style></head><body>
<h1>{{TITLE}}</h1>
<p>{{HINT}}</p>
<input type="file" id="f" accept=".bin">
<button id="b">{{SEND}}</button>
<progress id="p" value="0" max="100"></progress>
<div id="s"></div>
<p><a href="/">{{BACK}}</a></p>
<script>
var f=document.getElementById('f'),b=document.getElementById('b'),
    p=document.getElementById('p'),s=document.getElementById('s');
b.onclick=function(){
 if(!f.files.length){s.textContent='{{PICK}}';return;}
 var fd=new FormData();fd.append('update',f.files[0]);
 var x=new XMLHttpRequest();x.open('POST','/update');
 x.upload.onprogress=function(e){if(e.lengthComputable){
   var v=Math.round(e.loaded/e.total*100);p.value=v;s.textContent=v+' %';}};
 x.onload=function(){b.disabled=false;f.disabled=false;
   if(x.status==200){p.value=100;s.textContent='{{OK}}';}
   else{s.textContent='{{FAIL}}: '+(x.responseText||x.status);}};
 x.onerror=function(){b.disabled=false;f.disabled=false;s.textContent='{{FAIL}}';};
 b.disabled=true;f.disabled=true;s.textContent='0 %';
 x.send(fd);};
</script></body></html>)rawliteral";

static void handleUpdatePage() {
  if (!updateAuthed()) { s_srv.requestAuthentication(); return; }
  const uint8_t lang = Lang_Get();
  String p = FPSTR(UPDATE_HTML);
  p.replace("{{LANG}}",  (lang == LANG_EN) ? "en" : ((lang == LANG_SK) ? "sk" : "cs"));
  p.replace("{{TITLE}}", (lang == LANG_EN) ? "Firmware update"
                       : ((lang == LANG_SK) ? "Aktualizácia firmvéru" : "Aktualizace firmwaru"));
  p.replace("{{HINT}}",  (lang == LANG_EN) ? "Pick the <b>.ino.bin</b> or <b>firmware.bin</b> file (the one without "
                              "<i>merged</i> / <i>factory</i>). The display goes dark while the "
                              "flash is written and comes back on when it is done."
                       : ((lang == LANG_SK) ? "Vyberte súbor <b>.ino.bin</b> alebo <b>firmware.bin</b> (ten bez <i>merged</i> / <i>factory</i>). "
                              "Displej počas zápisu zhasne a po dokončení sa "
                              "sám rozsvieti."
                       : "Vyberte soubor <b>.ino.bin</b> (ten bez <i>merged</i>). "
                              "Displej po dobu zapisu zhasne a po dokonceni se "
                              "sam rozsviti."));
  p.replace("{{SEND}}",  (lang == LANG_EN) ? "Upload"           : ((lang == LANG_SK) ? "Nahrať" : "Nahrát"));
  p.replace("{{BACK}}",  (lang == LANG_EN) ? "Back to settings" : ((lang == LANG_SK) ? "Späť na nastavenia" : "Zpět na nastavení"));
  p.replace("{{PICK}}",  (lang == LANG_EN) ? "Pick a file first." : ((lang == LANG_SK) ? "Najskôr vyberte súbor." : "Nejdřív vyberte soubor."));
  p.replace("{{OK}}",    (lang == LANG_EN) ? "Done. The device is restarting."
                       : ((lang == LANG_SK) ? "Hotovo. Zariadenie sa reštartuje." : "Hotovo. Zařízení se restartuje."));
  p.replace("{{FAIL}}",  (lang == LANG_EN) ? "Update failed"    : ((lang == LANG_SK) ? "Aktualizácia zlyhala" : "Aktualizace selhala"));
  s_srv.sendHeader("Cache-Control", "no-store");
  s_srv.send(200, "text/html; charset=utf-8", p);
}

// Called repeatedly by WebServer as the body arrives. The whole transfer runs
// inside one handleClient(), so nothing else can be drawing meanwhile - but the
// watchdog still has to be fed by hand.
static void handleUpdateUpload() {
  HTTPUpload& up = s_srv.upload();
  static int s_lastWebProg = -1;
  static unsigned long s_lastWebDraw = 0;
  static size_t s_updExpectedSize = 0;

  switch (up.status) {
    case UPLOAD_FILE_START:
      s_updOk = false;
      s_updErr = "";
      s_lastWebProg = -1;
      s_lastWebDraw = 0;
      if (!updateAuthed()) { s_updErr = "auth"; return; }
      Serial.printf("OTA: %s\n", up.filename.c_str());
      if (strstr(up.filename.c_str(), "factory") || strstr(up.filename.c_str(), "Factory") ||
          strstr(up.filename.c_str(), "merged")  || strstr(up.filename.c_str(), "Merged")) {
        s_updErr = (Lang_Get() == LANG_EN) ? "Factory/merged binary cannot be flashed via OTA. Use -ota.bin."
                 : ((Lang_Get() == LANG_SK) ? "Factory/merged binarku nemozno nahrat cez OTA. Pouzite -ota.bin."
                 : "Factory/merged binarku nelze nahrat pres OTA. Pouzijte -ota.bin.");
        otaEnd(false);
        return;
      }
      otaStart();
      s_updExpectedSize = 0;
      if (s_srv.hasHeader("Content-Length")) {
        long cl = s_srv.header("Content-Length").toInt();
        if (cl > 4000) s_updExpectedSize = (size_t)(cl - 350);
      }
      UI_DrawOtaProgress("Web OTA", 0, 0, s_updExpectedSize, (Lang_Get() == LANG_EN) ? "Writing to flash..." : "Zapisujem do flash pamäte...");
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        s_updErr = Update.errorString();
        UI_DrawOtaProgress("Web OTA", 0, 0, 0, s_updErr.c_str());
        otaEnd(false);
      }
      break;

    case UPLOAD_FILE_WRITE:
      if (s_updErr.length()) return;              // already failed, drain the body
      if (!Update.isRunning()) {
        s_updErr = "Update not running";
        otaEnd(false);
        return;
      }
      if (up.buf && up.currentSize > 0) {
        if (Update.write(up.buf, up.currentSize) != up.currentSize) {
          s_updErr = Update.errorString();
          Update.abort();
          UI_DrawOtaProgress("Web OTA", 0, up.totalSize, s_updExpectedSize, s_updErr.c_str());
          otaEnd(false);
          return;
        }
        int prog = (s_updExpectedSize > 0) ? (int)(up.totalSize * 100 / s_updExpectedSize) : 0;
        if (prog > 99) prog = 99;
        if (prog != s_lastWebProg && (millis() - s_lastWebDraw >= 200 || prog == 100)) {
          s_lastWebProg = prog;
          s_lastWebDraw = millis();
          UI_DrawOtaProgress("Web OTA", prog, up.totalSize, s_updExpectedSize, nullptr);
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      }
      Watchdog_Feed();
      break;

    case UPLOAD_FILE_END:
      if (s_updErr.length()) return;
      if (Update.isRunning() && Update.end(true)) {
        s_updOk = true;
        Serial.printf("OTA: done, %u B\n", (unsigned)up.totalSize);
        UI_DrawOtaProgress("Web OTA", 100, up.totalSize, up.totalSize, (Lang_Get() == LANG_EN) ? "Success! Restarting..." : "Hotovo! Reštartujem...");
        delay(600);
        otaEnd(true);
      } else {
        s_updErr = Update.errorString();
        UI_DrawOtaProgress("Web OTA", 0, up.totalSize, s_updExpectedSize, s_updErr.c_str());
        delay(2000);
        otaEnd(false);
      }
      break;

    case UPLOAD_FILE_ABORTED:
      if (s_updErr == "auth") return;             // never started, keep the 401
      if (Update.isRunning()) Update.abort();
      if (s_updating) otaEnd(false);              // only if the screen was taken over
      s_updErr = "aborted";
      break;
  }
}


// Runs once the body has been consumed, so this is where the verdict is sent.
static void handleUpdateDone() {
  if (s_updErr == "auth") { s_updErr = ""; s_srv.requestAuthentication(); return; }
  s_srv.sendHeader("Connection", "close");
  if (s_updOk) {
    s_srv.send(200, "text/plain", "OK");
    delay(400);
    ESP.restart();
    return;
  }
  s_srv.send(500, "text/plain", s_updErr.length() ? s_updErr : String("update failed"));
  s_updErr = "";
}

// --- Lifecycle --------------------------------------------------------------
void WebConfig_Begin(bool apMode) {
  s_apMode = apMode;

  // The handlers are registered ONCE for the life of the process. This function
  // is called at least twice in a normal boot - first for the access point,
  // then again after joining the home network - and WebServer::on() appends to
  // a list rather than replacing, so registering again would leave a duplicate
  // of every route behind.
  if (s_running) {
    // Only the role-specific parts change.
    if (apMode) {
      s_dns.setErrorReplyCode(DNSReplyCode::NoError);
      s_dns.start(53, "*", WiFi.softAPIP());
    } else {
      s_dns.stop();
      MDNS.end();
      if (MDNS.begin(Settings_Hostname())) MDNS.addService("http", "tcp", WEB_PORT);
      Serial.printf("Web: http://%s.local/ or http://%s/\n",
                    Settings_Hostname(), WiFi.localIP().toString().c_str());
    }
    return;
  }

  const char* headerkeys[] = {"Content-Length"};
  s_srv.collectHeaders(headerkeys, 1);

  s_srv.on("/", HTTP_GET, handleRoot);
  s_srv.on("/api/config", HTTP_GET, handleGetConfig);
  s_srv.on("/api/config", HTTP_POST, handlePostConfig);
  s_srv.on("/api/status", HTTP_GET, handleStatus);
  s_srv.on("/api/hardware", HTTP_GET, handleHardware);
  s_srv.on("/api/rtc/sync_ntp", HTTP_POST, handleRtcSyncNtp);
  s_srv.on("/api/rtc/sync_browser", HTTP_POST, handleRtcSyncBrowser);
  s_srv.on("/api/toggle-legends", HTTP_POST, handleToggleLegends);
  s_srv.on("/api/buzzer/test", HTTP_POST, handleBuzzerTest);
  s_srv.on("/api/input", HTTP_POST, handleInput);
  s_srv.on("/api/screen", HTTP_POST, handleScreen);
  s_srv.on("/api/range", HTTP_POST, handleRange);
  s_srv.on("/api/scan", HTTP_GET, handleScan);
  s_srv.on("/api/wifi", HTTP_POST, handleWifi);
  s_srv.on("/api/wifi/delete", HTTP_POST, handleWifiDelete);
  s_srv.on("/api/geocode", HTTP_GET, handleGeocode);
  s_srv.on("/api/export", HTTP_GET, handleExport);
  s_srv.on("/api/screenshot.bmp", HTTP_GET, handleScreenshot);
  s_srv.on("/api/stats", HTTP_GET, handleStats);
  s_srv.on("/api/stats/reset", HTTP_POST, handleStatsReset);
  s_srv.on("/api/import", HTTP_POST, handleImport);
  s_srv.on("/api/reboot", HTTP_POST, handleReboot);
  s_srv.on("/api/reset", HTTP_POST, handleReset);
  s_srv.on("/api/ota/check", HTTP_GET, handleOtaCheck);
  s_srv.on("/api/ota/status", HTTP_GET, handleOtaStatus);
  s_srv.on("/api/ota/start", HTTP_POST, handleOtaStart);
  s_srv.on("/api/serial/read", HTTP_GET, handleSerialRead);
  s_srv.on("/api/serial/clear", HTTP_POST, handleSerialClear);
  s_srv.on("/api/serial/send", HTTP_POST, handleSerialSend);
  // The update page authenticates with HTTP Basic when a password is set. See
  // the note in Settings.h about why the password is stored in the clear.
  s_srv.on("/update", HTTP_GET, handleUpdatePage);
  s_srv.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  s_srv.onNotFound(handleNotFound);

  s_srv.begin();
  s_running = true;

  if (apMode) {
    s_dns.setErrorReplyCode(DNSReplyCode::NoError);
    s_dns.start(53, "*", WiFi.softAPIP());
    Serial.printf("Web: portal at http://%s/\n", WiFi.softAPIP().toString().c_str());
  } else {
    if (MDNS.begin(Settings_Hostname())) {
      MDNS.addService("http", "tcp", WEB_PORT);
      Serial.printf("Web: http://%s.local/ or http://%s/\n",
                    Settings_Hostname(), WiFi.localIP().toString().c_str());
    } else {
      Serial.printf("Web: http://%s/  (mDNS failed to start)\n",
                    WiFi.localIP().toString().c_str());
    }
  }
}

void WebConfig_Loop() {
  if (!s_running) return;
  if (s_apMode) s_dns.processNextRequest();
  s_srv.handleClient();
}
