// =============================================================================
//  MeteoPlaneRadar
//  WiFi connection and the configuration access point. See WiFiPortal.h.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "WiFiPortal.h"
#include "WebConfig.h"
#include "Settings.h"
#include "Lang.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Watchdog.h"

#include <WiFi.h>
#include <string.h>

static bool s_ap = false;
static unsigned long s_lastReconnect = 0;

bool   WiFi_IsAP()        { return s_ap; }
bool   WiFi_IsConnected() { return WiFi.status() == WL_CONNECTED; }
String WiFi_SSID() { return WiFi_IsConnected() ? WiFi.SSID() : String(Settings_WifiSsid()); }
String WiFi_IP()   { return WiFi_IsConnected() ? WiFi.localIP().toString()
                                               : (s_ap ? WiFi.softAPIP().toString() : String("-")); }

// --- Screens ----------------------------------------------------------------
void WiFi_DrawApScreen() {
  const bool en = (Lang_Get() == LANG_EN);
  gfx->fillScreen(C_BLACK);

  UI_TextCentered("MeteoPlaneRadar", 34, C_CYAN, 2);
  UI_TextCentered("chiptron.cz", 58, C_GRAY, 1);
  UI_TextCentered(en ? "Scan with your phone:" : "Naskenuj mobilem:", 78, C_GRAY, 1);

  const int qrSize = 190;
  UI_DrawWifiQR(AP_SSID, AP_PASSWORD, /*open=*/true,
                (LCD_WIDTH - qrSize) / 2, 98, qrSize);

  UI_TextCentered(AP_SSID, 300, C_WHITE, 1);
  UI_TextCentered(en ? "no password  |  then open 192.168.4.1"
                     : "bez hesla  |  pak otevri 192.168.4.1", 322, C_GRAY, 1);
  UI_TextCentered(en ? "Waiting for your network..."
                     : "Cekam na zadani site...", 348, C_YELLOW, 1);
  gfx->flush();
}

static void drawConnecting(const char* ssid) {
  const bool en = (Lang_Get() == LANG_EN);
  gfx->fillScreen(C_BLACK);
  UI_TextCentered(en ? "Connecting to WiFi..." : "Pripojuji k WiFi...",
                  LCD_HEIGHT / 2 - 20, C_WHITE, 2);
  if (ssid && *ssid) UI_TextCentered(ssid, LCD_HEIGHT / 2 + 12, C_CYAN, 2);
  gfx->flush();
}

// --- Access point -----------------------------------------------------------
static void startAP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  const char* pass = (strlen(AP_PASSWORD) == 0) ? nullptr : AP_PASSWORD;
  WiFi.softAP(AP_SSID, pass);
  delay(200);
  s_ap = true;
  Serial.printf("WiFi: pristupovy bod %s, http://%s/\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());
  WebConfig_Begin(true);
  WiFi_DrawApScreen();
}

// One connection attempt. Blocking, but bounded - and the watchdog is fed
// throughout, so a slow router cannot cause a reboot.
static bool tryConnect(uint32_t timeoutMs) {
  if (!Settings_HasWifi()) return false;
  drawConnecting(Settings_WifiSsid());

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(Settings_WifiSsid(), Settings_WifiPass());

  const unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      s_ap = false;
      Serial.printf("WiFi ok, IP %s\n", WiFi.localIP().toString().c_str());
      WebConfig_Begin(false);
      return true;
    }
    Watchdog_Feed();
    delay(100);
  }
  Serial.printf("WiFi: pripojeni k %s se nepovedlo\n", Settings_WifiSsid());
  return false;
}

void WiFi_Begin() {
  if (Settings_HasWifi() && tryConnect(20000)) return;
  startAP();
}

void WiFi_Loop() {
  // New credentials arrived from the portal.
  if (WebConfig_WantsWifiConnect()) {
    WebConfig_ClearWifiConnect();
    if (tryConnect(20000)) {
      // Connected - the portal has done its job and the full settings page is
      // now served on the home network instead.
      return;
    }
    // Wrong password, out of range, router asleep: back to the access point so
    // the user can simply try again. Their entry is kept in the form.
    startAP();
    return;
  }

  if (s_ap) return;                       // nothing to keep alive in AP mode

  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - s_lastReconnect < 15000) return;
  s_lastReconnect = now;
  Serial.println("WiFi: spojeni ztraceno, zkousim znovu");
  WiFi.reconnect();
}

void WiFi_Reset() {
  Settings_ClearWifi();
  WiFi.disconnect(true, true);
  startAP();
}
