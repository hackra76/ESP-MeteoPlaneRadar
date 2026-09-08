// =============================================================================
//  MeteoPlaneRadar
//  WiFi connection and the configuration access point. See WiFiPortal.h.
//
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
  const uint8_t lang = Lang_Get();
  gfx->fillScreen(C_BLACK);

  UI_TextCentered("MeteoPlaneRadar", 34, C_CYAN, 2);
  UI_TextCentered("H4CKR4", 58, C_GRAY, 1);
  const char* scanTxt = (lang == LANG_EN) ? "Scan with your phone:"
                      : ((lang == LANG_SK) ? "Naskenuj mobilom:" : "Naskenuj mobilem:");
  UI_TextCentered(scanTxt, 78, C_GRAY, 1);

  const int qrSize = 190;
  UI_DrawWifiQR(AP_SSID, AP_PASSWORD, /*open=*/true,
                (LCD_WIDTH - qrSize) / 2, 98, qrSize);

  UI_TextCentered(AP_SSID, 300, C_WHITE, 1);
  const char* openTxt = (lang == LANG_EN) ? "no password  |  then open 192.168.4.1"
                      : ((lang == LANG_SK) ? "bez hesla  |  potom otvor 192.168.4.1" : "bez hesla  |  pak otevri 192.168.4.1");
  UI_TextCentered(openTxt, 322, C_GRAY, 1);
  const char* waitTxt = (lang == LANG_EN) ? "Waiting for your network..."
                      : ((lang == LANG_SK) ? "Cakam na zadanie siete..." : "Cekam na zadani site...");
  UI_TextCentered(waitTxt, 348, C_YELLOW, 1);
  gfx->flush();
}

static void drawConnecting(const char* ssid) {
  const uint8_t lang = Lang_Get();
  gfx->fillScreen(C_BLACK);
  const char* connTxt = (lang == LANG_EN) ? "Connecting to WiFi..."
                      : ((lang == LANG_SK) ? "Pripajam k WiFi..." : "Pripojuji k WiFi...");
  UI_TextCentered(connTxt, LCD_HEIGHT / 2 - 20, C_WHITE, 2);
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
  Serial.printf("WiFi: Access point %s, http://%s/\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());
  WebConfig_Begin(true);
  WiFi_DrawApScreen();
}

static unsigned long s_lastDisconnect = 0;
static volatile bool s_needsRedraw = false;

bool WiFi_TakeNeedsRedraw() {
  if (s_needsRedraw) {
    s_needsRedraw = false;
    return true;
  }
  return false;
}

// One connection attempt. Blocking, but bounded - and the watchdog is fed
// throughout, so a slow router cannot cause a reboot.
static bool tryConnect(const char* ssid, const char* pass, uint32_t timeoutMs) {
  if (!ssid || !*ssid) return false;
  drawConnecting(ssid);

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(Settings_Hostname());
  WiFi.begin(ssid, pass);

  const unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      s_ap = false;
      Serial.printf("WiFi: Connected, SSID %s, IP %s\n", ssid, WiFi.localIP().toString().c_str());
      WebConfig_Begin(false);
      s_needsRedraw = true;
      return true;
    }
    Watchdog_Feed();
    delay(100);
  }
  Serial.printf("WiFi: Connection to %s failed\n", ssid);
  return false;
}

void WiFi_Begin() {
  int count = Settings_WifiNetworkCount();
  if (count == 0) {
    startAP();
    return;
  }

  // If only 1 network is saved, directly try connecting to it
  if (count == 1) {
    WifiCredential cred;
    if (Settings_GetWifiNetwork(0, &cred) && tryConnect(cred.ssid, cred.pass, 20000)) {
      return;
    }
    startAP();
    return;
  }

  // Multiple networks saved: scan and find which saved networks are present, sorted by RSSI
  drawConnecting("Scanning WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Watchdog_Feed();

  int n = WiFi.scanNetworks();
  Watchdog_Feed();

  struct ScannedCandidate {
    int netIdx;
    int rssi;
  };
  ScannedCandidate cand[MAX_WIFI_NETWORKS];
  int candCount = 0;

  for (int i = 0; i < count; i++) {
    WifiCredential cred;
    if (!Settings_GetWifiNetwork(i, &cred)) continue;
    int bestRssi = -999;
    for (int j = 0; j < n; j++) {
      if (WiFi.SSID(j).equals(cred.ssid)) {
        if (WiFi.RSSI(j) > bestRssi) bestRssi = WiFi.RSSI(j);
      }
    }
    if (bestRssi > -999) {
      cand[candCount].netIdx = i;
      cand[candCount].rssi = bestRssi;
      candCount++;
    }
  }
  WiFi.scanDelete();

  // Sort candidates by RSSI descending
  for (int i = 0; i < candCount - 1; i++) {
    for (int j = i + 1; j < candCount; j++) {
      if (cand[j].rssi > cand[i].rssi) {
        ScannedCandidate t = cand[i];
        cand[i] = cand[j];
        cand[j] = t;
      }
    }
  }

  // Try connecting to scanned candidates in order of strength
  for (int i = 0; i < candCount; i++) {
    WifiCredential cred;
    if (Settings_GetWifiNetwork(cand[i].netIdx, &cred)) {
      Serial.printf("WiFi: Found known network %s (RSSI %d dBm)\n", cred.ssid, cand[i].rssi);
      if (tryConnect(cred.ssid, cred.pass, 15000)) {
        Settings_SetActiveWifi(cand[i].netIdx);
        return;
      }
    }
  }

  // Fallback: If scan didn't find any or connections failed (e.g. hidden SSID), try all saved sequentially
  for (int i = 0; i < count; i++) {
    WifiCredential cred;
    if (Settings_GetWifiNetwork(i, &cred)) {
      bool alreadyTried = false;
      for (int c = 0; c < candCount; c++) {
        if (cand[c].netIdx == i) { alreadyTried = true; break; }
      }
      if (!alreadyTried) {
        if (tryConnect(cred.ssid, cred.pass, 12000)) {
          Settings_SetActiveWifi(i);
          return;
        }
      }
    }
  }

  startAP();
}

void WiFi_Loop() {
  // New credentials arrived from the portal.
  if (WebConfig_WantsWifiConnect()) {
    WebConfig_ClearWifiConnect();
    WifiCredential cred;
    if (Settings_GetWifiNetwork(0, &cred) && tryConnect(cred.ssid, cred.pass, 20000)) {
      // Connected - full settings page served on home network
      return;
    }
    // Wrong password, out of range: back to AP so user can retry
    startAP();
    return;
  }

  if (s_ap) return;                       // nothing to keep alive in AP mode

  if (WiFi.status() == WL_CONNECTED) {
    s_lastDisconnect = 0;
    return;
  }

  unsigned long now = millis();
  if (s_lastDisconnect == 0) s_lastDisconnect = now;

  if (now - s_lastReconnect < 15000) return;
  s_lastReconnect = now;
  Serial.println("WiFi: Connection lost, reconnecting...");
  WiFi.reconnect();

  // If disconnected for more than 45s and multiple networks are saved, re-scan to switch
  if (Settings_WifiNetworkCount() > 1 && (now - s_lastDisconnect > 45000)) {
    Serial.println("WiFi: Long outage, scanning for other known networks");
    WiFi_Begin();
  }
}

void WiFi_Reset() {
  Settings_ClearWifi();
  WiFi.disconnect(true, true);
  startAP();
}
