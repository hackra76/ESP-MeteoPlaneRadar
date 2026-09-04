// =============================================================================
//  MeteoPlaneRadar
//  The configuration web server. See WebConfig.h.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
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
#include "NightMode.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Watchdog.h"
#include "QMI8658.h"
#include "AsyncCore.h"
#include "PCF85063.h"
#include "Outside.h"
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

bool WebConfig_UpdateBusy()        { return s_updating; }
bool WebConfig_WantsWifiConnect()  { return s_wantConnect; }
void WebConfig_ClearWifiConnect()  { s_wantConnect = false; }
bool WebConfig_WantsRestart()      { return s_wantRestart; }

int  WebConfig_TakeScreen()     { int v = s_reqScreen;     s_reqScreen = -1;    return v; }
int  WebConfig_TakeScreenStep() { int v = s_reqScreenStep; s_reqScreenStep = 0; return v; }
int  WebConfig_TakeRangeStep()  { int v = s_reqRangeStep;  s_reqRangeStep = 0;  return v; }
bool WebConfig_TakeRedraw()     { bool r = s_reqRedraw;    s_reqRedraw = false; return r; }
void WebConfig_RequestRedraw()  { s_reqRedraw = true; }

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
                          (Settings_ScreenEnabled(SCREEN_FORECAST_I) << 4);

  Settings_FromJson(doc.as<JsonObjectConst>());
  s_reqRedraw = true;

  // Applied straight away - these are the ones you want to see change while
  // you are still looking at the slider.
  NightMode_Apply();

  const uint8_t newMask = (Settings_ScreenEnabled(SCREEN_CLOCK_I) << 0) |
                          (Settings_ScreenEnabled(SCREEN_PLANES_I) << 1) |
                          (Settings_ScreenEnabled(SCREEN_METEO_I) << 2) |
                          (Settings_ScreenEnabled(SCREEN_TACTICAL_I) << 3) |
                          (Settings_ScreenEnabled(SCREEN_FORECAST_I) << 4);
  const bool moved = (fabs(oldLat - Settings_Lat()) > 1e-6) ||
                     (fabs(oldLon - Settings_Lon()) > 1e-6);
  if (moved) Forecast_Invalidate();

  // These reach too far into cached state (decoded radar frames, allocated
  // buffers, which screen is even reachable) to be worth unpicking at runtime.
  // A restart is a second and a half and guarantees a clean result.
  if (moved || oldSrc != Settings_RadarSource() || oldMask != newMask)
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

  // RTC PCF85063 details
  doc["rtcDetected"] = PCF85063_IsDetected();
  doc["rtcOscStopped"] = PCF85063_IsOscillatorStopped();
  struct tm rtcTm;
  if (PCF85063_ReadTime(&rtcTm)) {
    char rtcBuf[32];
    snprintf(rtcBuf, sizeof(rtcBuf), "%02d:%02d:%02d (%d.%d.%04d)",
             rtcTm.tm_hour, rtcTm.tm_min, rtcTm.tm_sec,
             rtcTm.tm_mday, rtcTm.tm_mon + 1, rtcTm.tm_year + 1900);
    doc["rtcTime"] = rtcBuf;
  } else {
    doc["rtcTime"] = "-";
  }

  // Active I2C Bus Devices scan (Targeted scan with I2C bus lock)
  struct I2CTarget { uint8_t addr; const char* name; };
  static const I2CTarget targets[] = {
    { 0x15, "Dotykový kontrolér (Touch CST820)" },
    { 0x38, "Dotykový kontrolér (Touch CHSC6540)" },
    { 0x20, "I/O Expandér (TCA9554)" },
    { 0x43, "I/O Expandér (TCA9554 alt)" },
    { 0x51, "Hardware RTC (PCF85063)" },
    { 0x6B, "6-osové IMU (QMI8658)" },
  };

  Async_LockI2C();
  JsonArray i2cArr = doc["i2cBus"].to<JsonArray>();
  for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
    Wire.beginTransmission(targets[i].addr);
    if (Wire.endTransmission() == 0) {
      JsonObject dev = i2cArr.add<JsonObject>();
      char hexBuf[8]; snprintf(hexBuf, sizeof(hexBuf), "0x%02X", targets[i].addr);
      dev["addr"] = hexBuf;
      dev["name"] = targets[i].name;
    }
  }
  Async_UnlockI2C();

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
  if (!Net_GetString(url, body, "GEOKOD")) { s_srv.send(502, "application/json", "[]"); return; }

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
  }
  JsonDocument res; res["ok"] = true;
  res["legends"] = Settings_ShowLegends();
  res["clockStyle"] = Settings_ClockStyle();
  sendJson(200, res);
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
// The RGB panel streams its framebuffer out of PSRAM continuously, and writing
// flash suspends the cache from under it - the picture tears and jumps. Nothing
// the drawing code can do about it, so the backlight goes off for the duration
// and the browser shows the real progress bar.
static void otaStart() {
  const uint8_t lang = Lang_Get();
  s_updating = true;
  gfx->fillScreen(C_BLACK);
  const char* upTxt = (lang == LANG_EN) ? "Updating firmware..."
                    : ((lang == LANG_SK) ? "Prebieha aktualizacia..." : "Probiha aktualizace...");
  const char* pwrTxt = (lang == LANG_EN) ? "Do not disconnect power"
                     : ((lang == LANG_SK) ? "Neodpajaj napajanie" : "Neodpojuj napajeni");
  UI_TextCentered(upTxt, LCD_HEIGHT / 2 - 10, C_WHITE, 2);
  UI_TextCentered(pwrTxt, LCD_HEIGHT / 2 + 20, C_GRAY, 1);
  gfx->flush();
  delay(700);
  Set_Backlight(0);
}

static void otaEnd(bool ok) {
  const uint8_t lang = Lang_Get();
  Set_Backlight(Settings_Backlight());
  gfx->fillScreen(C_BLACK);
  const char* doneTxt = ok ? ((lang == LANG_EN) ? "Done, restarting..."
                             : ((lang == LANG_SK) ? "Hotovo, restartujem..." : "Hotovo, restartuji..."))
                           : ((lang == LANG_EN) ? "Update failed"
                             : ((lang == LANG_SK) ? "Aktualizacia zlyhala" : "Aktualizace selhala"));
  UI_TextCentered(doneTxt, LCD_HEIGHT / 2, ok ? C_GREEN : C_RED, 2);
  gfx->flush();
  s_updating = false;
}

// --- GitHub Online OTA ------------------------------------------------------
enum OtaState : uint8_t {
  OTA_IDLE = 0,
  OTA_CHECKING,
  OTA_DOWNLOADING,
  OTA_FLASHING,
  OTA_SUCCESS,
  OTA_ERROR
};
static volatile OtaState s_otaState = OTA_IDLE;
static volatile int      s_otaProgress = 0;
static String            s_otaError = "";
static String            s_targetOtaUrl = "";
static String            s_targetOtaTag = "";
static TaskHandle_t      s_otaTaskHandle = nullptr;

static void githubOtaTask(void* param) {
  (void)param;
  s_otaState = OTA_DOWNLOADING;
  s_otaProgress = 0;
  s_otaError = "";

  otaStart();

  String currentUrl = s_targetOtaUrl;
  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(15);
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(30000);
  http.setUserAgent(HTTP_USER_AGENT);
  static const char* WANTED_HEADERS[] = { "Location", "Content-Length" };
  http.collectHeaders(WANTED_HEADERS, 2);

  bool connected = false;
  int redirects = 0;
  while (redirects < 5) {
    Watchdog_Feed();
    if (!http.begin(client, currentUrl)) {
      s_otaError = "Connection failed";
      break;
    }
    int code = http.GET();
    if (code == 301 || code == 302 || code == 307 || code == 308) {
      String newLoc = http.header("Location");
      while (client.available()) client.read();
      http.end();
      client.stop();
      delay(50);
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
    while (client.available()) client.read();
    http.end();
    client.stop();
    break;
  }

  if (!connected) {
    s_otaState = OTA_ERROR;
    otaEnd(false);
    s_otaTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
  }

  int totalLen = http.getSize();
  size_t updateSize = (totalLen > 0) ? (size_t)totalLen : UPDATE_SIZE_UNKNOWN;
  if (!Update.begin(updateSize)) {
    s_otaError = Update.errorString();
    while (client.available()) client.read();
    http.end();
    client.stop();
    s_otaState = OTA_ERROR;
    otaEnd(false);
    s_otaTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
  }

  WiFiClient* stream = http.getStreamPtr();
  const size_t BUF_SZ = 4096;
  uint8_t* buf = (uint8_t*)heap_caps_malloc(BUF_SZ, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!buf) buf = (uint8_t*)malloc(BUF_SZ);
  if (!buf) {
    s_otaError = "No memory for buffer";
    Update.abort();
    while (client.available()) client.read();
    http.end();
    client.stop();
    s_otaState = OTA_ERROR;
    otaEnd(false);
    s_otaTaskHandle = nullptr;
    vTaskDelete(NULL);
    return;
  }

  size_t written = 0;
  unsigned long lastFeed = millis();
  while (http.connected() && (totalLen <= 0 || written < (size_t)totalLen)) {
    size_t avail = stream ? stream->available() : 0;
    if (avail) {
      size_t toRead = avail > BUF_SZ ? BUF_SZ : avail;
      int r = stream->readBytes(buf, toRead);
      if (r > 0) {
        if (Update.write(buf, (size_t)r) != (size_t)r) {
          s_otaError = Update.errorString();
          Update.abort();
          break;
        }
        written += (size_t)r;
        if (totalLen > 0) {
          s_otaProgress = (int)(written * 100 / (size_t)totalLen);
        }
      }
    } else {
      delay(10);
    }
    if (millis() - lastFeed > 1000) {
      Watchdog_Feed();
      lastFeed = millis();
    }
  }

  if (buf) {
    if (esp_ptr_external_ram(buf)) heap_caps_free(buf);
    else free(buf);
  }
  while (client.available()) client.read();
  http.end();
  client.stop();

  if (s_otaError.length() == 0 && Update.end(true)) {
    s_otaProgress = 100;
    s_otaState = OTA_SUCCESS;
    otaEnd(true);
    Serial.printf("OTA: GitHub update to %s successful (%u B)\n", s_targetOtaTag.c_str(), (unsigned)written);
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP.restart();
  } else {
    if (s_otaError.length() == 0) s_otaError = Update.errorString();
    s_otaState = OTA_ERROR;
    otaEnd(false);
  }

  s_otaTaskHandle = nullptr;
  vTaskDelete(NULL);
}

static void handleOtaCheck() {
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
    s_srv.send(500, "application/json", "{\"error\":\"begin_failed\"}");
    return;
  }
  http.addHeader("Accept", "application/vnd.github.v3+json");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    while (client.available()) client.read();
    http.end();
    client.stop();
    char errJson[64];
    snprintf(errJson, sizeof(errJson), "{\"error\":\"github_http_%d\"}", code);
    s_srv.send(code > 0 ? code : 502, "application/json", errJson);
    return;
  }

  JsonDocument filter;
  filter["tag_name"] = true;
  filter["name"] = true;
  filter["body"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  while (client.available()) client.read();
  http.end();
  client.stop();

  if (err) {
    s_srv.send(500, "application/json", "{\"error\":\"json_parse_failed\"}");
    return;
  }

  const char* tag = doc["tag_name"] | "";
  const char* title = doc["name"] | "";
  const char* body = doc["body"] | "";
  String otaUrl = "";

  JsonArrayConst assets = doc["assets"].as<JsonArrayConst>();
  for (JsonObjectConst a : assets) {
    const char* aname = a["name"] | "";
    if (strstr(aname, "-ota.bin") || strstr(aname, "ota.bin")) {
      otaUrl = a["browser_download_url"] | "";
      break;
    }
  }

  const char* cur = FW_VERSION;
  const char* lat = tag;
  if (lat[0] == 'v' || lat[0] == 'V') lat++;

  int curMaj = 0, curMin = 0, curPat = 0;
  int latMaj = 0, latMin = 0, latPat = 0;
  sscanf(cur, "%d.%d.%d", &curMaj, &curMin, &curPat);
  sscanf(lat, "%d.%d.%d", &latMaj, &latMin, &latPat);

  bool updateAvail = false;
  if (latMaj > curMaj) updateAvail = true;
  else if (latMaj == curMaj && latMin > curMin) updateAvail = true;
  else if (latMaj == curMaj && latMin == curMin && latPat > curPat) updateAvail = true;

  JsonDocument out;
  out["current"] = FW_VERSION;
  out["latest"] = tag;
  out["updateAvailable"] = updateAvail;
  out["name"] = title;
  out["body"] = body;
  out["url"] = otaUrl;

  String resp;
  serializeJson(out, resp);
  s_srv.send(200, "application/json", resp);
}

static void handleOtaStatus() {
  const char* stStr = "idle";
  switch (s_otaState) {
    case OTA_CHECKING:    stStr = "checking"; break;
    case OTA_DOWNLOADING: stStr = "downloading"; break;
    case OTA_FLASHING:    stStr = "flashing"; break;
    case OTA_SUCCESS:     stStr = "success"; break;
    case OTA_ERROR:       stStr = "error"; break;
    default:              stStr = "idle"; break;
  }

  char buf[256];
  snprintf(buf, sizeof(buf),
           "{\"state\":\"%s\",\"progress\":%d,\"error\":\"%s\"}",
           stStr, s_otaProgress, s_otaError.c_str());
  s_srv.send(200, "application/json", buf);
}

static void handleOtaStart() {
  String body = s_srv.hasArg("plain") ? s_srv.arg("plain") : "";
  JsonDocument doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    s_srv.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!authed(doc)) return;

  if (s_otaState == OTA_DOWNLOADING || s_otaState == OTA_FLASHING) {
    s_srv.send(409, "application/json", "{\"error\":\"already_running\"}");
    return;
  }

  String url = doc["url"] | "";
  String tag = doc["tag"] | "";

  if (url.length() == 0) {
    s_srv.send(400, "application/json", "{\"error\":\"missing_url\"}");
    return;
  }

  s_targetOtaUrl = url;
  s_targetOtaTag = tag;
  s_otaError = "";
  s_otaProgress = 0;

  BaseType_t ret = xTaskCreatePinnedToCore(githubOtaTask, "GhOta", 20480, NULL, 5, &s_otaTaskHandle, 0);
  if (ret != pdPASS) {
    s_srv.send(500, "application/json", "{\"error\":\"task_create_failed\"}");
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

  switch (up.status) {
    case UPLOAD_FILE_START:
      s_updOk = false;
      s_updErr = "";
      if (!updateAuthed()) { s_updErr = "auth"; return; }
      Serial.printf("OTA: %s\n", up.filename.c_str());
      otaStart();
      // The browser does not announce the image size up front, so let Update
      // take the whole free OTA slot.
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        s_updErr = Update.errorString();
        otaEnd(false);
      }
      break;

    case UPLOAD_FILE_WRITE:
      if (s_updErr.length()) return;              // already failed, drain the body
      if (Update.write(up.buf, up.currentSize) != up.currentSize) {
        s_updErr = Update.errorString();
        Update.abort();
        otaEnd(false);
        return;
      }
      Watchdog_Feed();
      break;

    case UPLOAD_FILE_END:
      if (s_updErr.length()) return;
      if (Update.end(true)) {
        s_updOk = true;
        Serial.printf("OTA: hotovo, %u B\n", (unsigned)up.totalSize);
        otaEnd(true);
      } else {
        s_updErr = Update.errorString();
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
      if (MDNS.begin(WEB_HOSTNAME)) MDNS.addService("http", "tcp", WEB_PORT);
      Serial.printf("Web: http://%s.local/ nebo http://%s/\n",
                    WEB_HOSTNAME, WiFi.localIP().toString().c_str());
    }
    return;
  }

  s_srv.on("/", HTTP_GET, handleRoot);
  s_srv.on("/api/config", HTTP_GET, handleGetConfig);
  s_srv.on("/api/config", HTTP_POST, handlePostConfig);
  s_srv.on("/api/status", HTTP_GET, handleStatus);
  s_srv.on("/api/hardware", HTTP_GET, handleHardware);
  s_srv.on("/api/rtc/sync_ntp", HTTP_POST, handleRtcSyncNtp);
  s_srv.on("/api/rtc/sync_browser", HTTP_POST, handleRtcSyncBrowser);
  s_srv.on("/api/toggle-legends", HTTP_POST, handleToggleLegends);
  s_srv.on("/api/input", HTTP_POST, handleInput);
  s_srv.on("/api/screen", HTTP_POST, handleScreen);
  s_srv.on("/api/range", HTTP_POST, handleRange);
  s_srv.on("/api/scan", HTTP_GET, handleScan);
  s_srv.on("/api/wifi", HTTP_POST, handleWifi);
  s_srv.on("/api/geocode", HTTP_GET, handleGeocode);
  s_srv.on("/api/export", HTTP_GET, handleExport);
  s_srv.on("/api/import", HTTP_POST, handleImport);
  s_srv.on("/api/reboot", HTTP_POST, handleReboot);
  s_srv.on("/api/reset", HTTP_POST, handleReset);
  s_srv.on("/api/ota/check", HTTP_GET, handleOtaCheck);
  s_srv.on("/api/ota/status", HTTP_GET, handleOtaStatus);
  s_srv.on("/api/ota/start", HTTP_POST, handleOtaStart);
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
    Serial.printf("Web: portal na http://%s/\n", WiFi.softAPIP().toString().c_str());
  } else {
    if (MDNS.begin(WEB_HOSTNAME)) {
      MDNS.addService("http", "tcp", WEB_PORT);
      Serial.printf("Web: http://%s.local/ nebo http://%s/\n",
                    WEB_HOSTNAME, WiFi.localIP().toString().c_str());
    } else {
      Serial.printf("Web: http://%s/  (mDNS se nespustilo)\n",
                    WiFi.localIP().toString().c_str());
    }
  }
}

void WebConfig_Loop() {
  if (!s_running) return;
  if (s_apMode) s_dns.processNextRequest();
  s_srv.handleClient();
}
