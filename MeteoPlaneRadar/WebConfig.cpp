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

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
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

bool WebConfig_UpdateBusy()        { return s_updating; }
bool WebConfig_WantsWifiConnect()  { return s_wantConnect; }
void WebConfig_ClearWifiConnect()  { s_wantConnect = false; }
bool WebConfig_WantsRestart()      { return s_wantRestart; }

int WebConfig_TakeScreen()     { int v = s_reqScreen;     s_reqScreen = -1;    return v; }
int WebConfig_TakeScreenStep() { int v = s_reqScreenStep; s_reqScreenStep = 0; return v; }
int WebConfig_TakeRangeStep()  { int v = s_reqRangeStep;  s_reqRangeStep = 0;  return v; }

// --- Helpers ----------------------------------------------------------------
static void sendJson(int code, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  s_srv.send(code, "application/json", out);
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
      // The update page's HTTP Basic credentials are handed to ElegantOTA once,
      // when its handler is registered. Re-registering mid-request would mean
      // tearing down the server that is still writing this reply, so the new
      // password takes effect on the restart below instead.
      s_wantRestart = true;
    }
  }

  const double oldLat = Settings_Lat(), oldLon = Settings_Lon();
  const uint8_t oldSrc = Settings_RadarSource();
  const uint8_t oldMask = (Settings_ScreenEnabled(SCREEN_CLOCK_I) << 0) |
                          (Settings_ScreenEnabled(SCREEN_PLANES_I) << 1) |
                          (Settings_ScreenEnabled(SCREEN_METEO_I) << 2) |
                          (Settings_ScreenEnabled(SCREEN_FORECAST_I) << 3);

  Settings_FromJson(doc.as<JsonObjectConst>());

  // Applied straight away - these are the ones you want to see change while
  // you are still looking at the slider.
  NightMode_Apply();

  const uint8_t newMask = (Settings_ScreenEnabled(SCREEN_CLOCK_I) << 0) |
                          (Settings_ScreenEnabled(SCREEN_PLANES_I) << 1) |
                          (Settings_ScreenEnabled(SCREEN_METEO_I) << 2) |
                          (Settings_ScreenEnabled(SCREEN_FORECAST_I) << 3);
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
  if      (scr == SCREEN_PLANES_I) ScreenPlanes_RangeText(rb, sizeof(rb));
  else if (scr == SCREEN_METEO_I)  ScreenWeather_RangeText(rb, sizeof(rb));
  doc["range"] = rb;                       // empty on screens without one

  JsonArray en = doc["enabled"].to<JsonArray>();
  for (uint8_t i = 0; i < SCREEN_N; i++) en.add(Settings_ScreenEnabled(i));

  char b[64];
  Status_Text(ST_ADSB, b, sizeof(b));     doc["adsb"] = b;
  Status_Text(ST_RADAR, b, sizeof(b));    doc["radar"] = b;
  Status_Text(ST_FORECAST, b, sizeof(b)); doc["forecast"] = b;
  sendJson(200, doc);
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
  snprintf(url, sizeof(url), "%s?name=%s&count=8&language=%s&format=json",
           GEOCODE_URL, enc.c_str(), Lang_Get() == LANG_EN ? "en" : "cs");

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
  s_updating = true;
  gfx->fillScreen(C_BLACK);
  UI_TextCentered("Probiha aktualizace...", LCD_HEIGHT / 2 - 10, C_WHITE, 2);
  UI_TextCentered("Neodpojuj napajeni", LCD_HEIGHT / 2 + 20, C_GRAY, 1);
  gfx->flush();
  delay(700);
  Set_Backlight(0);
}

static void otaProgress(size_t cur, size_t total) {
  (void)cur; (void)total;
  Watchdog_Feed();
}

static void otaEnd(bool ok) {
  Set_Backlight(Settings_Backlight());
  gfx->fillScreen(C_BLACK);
  UI_TextCentered(ok ? "Hotovo, restartuji..." : "Aktualizace selhala",
                  LCD_HEIGHT / 2, ok ? C_GREEN : C_RED, 2);
  gfx->flush();
  s_updating = false;
}

// --- Lifecycle --------------------------------------------------------------
void WebConfig_Begin(bool apMode) {
  s_apMode = apMode;

  // The handlers are registered ONCE for the life of the process. This function
  // is called at least twice in a normal boot - first for the access point,
  // then again after joining the home network - and WebServer::on() appends to
  // a list rather than replacing, so registering again would leave a duplicate
  // of every route (and a second ElegantOTA instance) behind.
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
  s_srv.on("/api/screen", HTTP_POST, handleScreen);
  s_srv.on("/api/range", HTTP_POST, handleRange);
  s_srv.on("/api/scan", HTTP_GET, handleScan);
  s_srv.on("/api/wifi", HTTP_POST, handleWifi);
  s_srv.on("/api/geocode", HTTP_GET, handleGeocode);
  s_srv.on("/api/export", HTTP_GET, handleExport);
  s_srv.on("/api/import", HTTP_POST, handleImport);
  s_srv.on("/api/reboot", HTTP_POST, handleReboot);
  s_srv.on("/api/reset", HTTP_POST, handleReset);
  s_srv.onNotFound(handleNotFound);

  // The update page authenticates with HTTP Basic when a password is set. See
  // the note in Settings.h about why the password is stored in the clear.
  if (Settings_HasAdminPassword())
    ElegantOTA.begin(&s_srv, WEB_ADMIN_USER, Settings_AdminPassword());
  else
    ElegantOTA.begin(&s_srv);
  ElegantOTA.setAutoReboot(true);
  ElegantOTA.onStart(otaStart);
  ElegantOTA.onProgress(otaProgress);
  ElegantOTA.onEnd(otaEnd);

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
  ElegantOTA.loop();
}
