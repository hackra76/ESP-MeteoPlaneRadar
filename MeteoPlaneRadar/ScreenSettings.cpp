// =============================================================================
//  MeteoPlaneRadar
//  Screen: settings (brightness, orientation, units, language, web address).
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenSettings.h"
#include "Settings.h"
#include "WiFiPortal.h"
#include "NightMode.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Config.h"
#include "Version.h"

#include <WiFi.h>
#include <math.h>

// Rows down the middle of the circle. The self-test in Layout.cpp checks the
// shared bands; these are local to this screen and are spaced so that nothing
// can touch its neighbour even with the longest string in either language.
#define ROW_BRIGHT   96     // brightness label     (size 2, 16 px)
#define SL_Y        118     // slider               (24 px)
#define ROW_WIFI    156     // SSID                 (size 2)
#define ROW_IP      178     // IP address           (size 2)
#define ROW_WEB     202     // where to find the web UI (size 1)
#define ROT_Y       232     // "top of the map" row
#define ROT_H        40

#define SL_X  90
#define SL_W  300
#define SL_H  24

#define ROT_MINUS_X  240
#define ROT_BTN_W     42
#define ROT_VAL_X    288
#define ROT_VAL_W     56
#define ROT_PLUS_X   350

#define COMPASS_CX    48
#define COMPASS_CY   252
#define COMPASS_R     24

#define BTN_X   90
#define BTN_W   300
#define BTN_H    36
#define BTN0_Y  288        // units
#define BTN1_Y  330        // language
#define BTN2_Y  372        // forget WiFi

static bool s_wantsWifiReset = false;

bool ScreenSettings_WantsWifiReset() { return s_wantsWifiReset; }
void ScreenSettings_ClearWifiReset() { s_wantsWifiReset = false; }

void ScreenSettings_Enter() {}
bool ScreenSettings_Tick() { return false; }

// Short compass label for the eight main directions. Anything that is not a
// multiple of 45 deg (possible if MAP_ROT_STEP_DEG is changed) falls back to
// plain degrees, so the row never shows nonsense.
static const char* bearingLabel(uint16_t deg, char* buf, size_t len) {
  static const char* N8_CZ[8] = { "S", "SV", "V", "JV", "J", "JZ", "Z", "SZ" };
  static const char* N8_EN[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
  if (deg % 45 == 0) return (Lang_Get() == LANG_EN) ? N8_EN[(deg / 45) % 8]
                                                    : N8_CZ[(deg / 45) % 8];
  snprintf(buf, len, "%u\xC2\xB0", (unsigned)deg);
  return buf;
}

bool ScreenSettings_HandleTap(int x, int y) {
  // Brightness slider (generous touch zone around the track). It sets whichever
  // level is currently in force, so adjusting it at night sets the night one -
  // which is what you meant if you are adjusting it at night.
  if (y >= SL_Y - 25 && y <= SL_Y + SL_H + 25 && x >= SL_X - 10 && x <= SL_X + SL_W + 10) {
    int pct = (x - SL_X) * 100 / SL_W;
    if (pct < 10) pct = 10;
    if (pct > 100) pct = 100;
    Settings_SetBacklight((uint8_t)pct);
    Set_Backlight((uint8_t)pct);
    return true;
  }
  // Which bearing is at the top. "+" walks clockwise: S -> SV -> V -> JV ...
  if (y >= ROT_Y && y <= ROT_Y + ROT_H) {
    int top = (int)Settings_TopBearing();
    if (x >= ROT_MINUS_X && x <= ROT_MINUS_X + ROT_BTN_W) {
      Settings_SetTopBearing((uint16_t)((top - MAP_ROT_STEP_DEG + 360) % 360));
      return true;
    }
    if (x >= ROT_PLUS_X && x <= ROT_PLUS_X + ROT_BTN_W) {
      Settings_SetTopBearing((uint16_t)((top + MAP_ROT_STEP_DEG) % 360));
      return true;
    }
  }
  if (x < BTN_X || x > BTN_X + BTN_W) return false;

  if (y >= BTN0_Y && y <= BTN0_Y + BTN_H) {
    Settings_SetMetricUnits(!Settings_MetricUnits());
    return true;
  }
  if (y >= BTN1_Y && y <= BTN1_Y + BTN_H) {
    uint8_t cur = Lang_Get();
    uint8_t nextLang = (cur == LANG_CZ) ? LANG_SK : ((cur == LANG_SK) ? LANG_EN : LANG_CZ);
    Settings_SetLanguage(nextLang);
    return true;
  }
  if (y >= BTN2_Y && y <= BTN2_Y + BTN_H) {
    s_wantsWifiReset = true;
    return true;
  }
  return false;
}

void ScreenSettings_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  Layout_ReserveBand(LY_DOTS - 6, 12);

  UI_TextCentered(T(S_SETTINGS), 40, C_WHITE, 3);
  { char v[28];
    const char* fwFmt = (Lang_Get() == LANG_EN) ? "firmware v%s" : ((Lang_Get() == LANG_SK) ? "firmvér v%s" : "firmware v%s");
    snprintf(v, sizeof(v), fwFmt, FW_VERSION);
    UI_TextCentered(v, 68, C_GRAY, 1); }

  // --- Brightness ---
  UI_Text(T(S_BRIGHTNESS), SL_X, ROW_BRIGHT, C_GRAY, 2);
  if (Settings_NightAuto() || Settings_IsNight()) {
    const char* dn = Settings_IsNight() ? (Lang_Get() == LANG_EN ? "(night)" : (Lang_Get() == LANG_SK ? "(nočný)" : "(noční)"))
                                        : (Lang_Get() == LANG_EN ? "(day)"   : (Lang_Get() == LANG_SK ? "(denný)" : "(denní)"));
    UI_Text(dn, SL_X + Layout_TextW(T(S_BRIGHTNESS), 2) + 8, ROW_BRIGHT + 4, C_GRAY, 1);
  }
  uint8_t bl = Settings_Backlight();
  char blbuf[8]; snprintf(blbuf, sizeof(blbuf), "%d%%", bl);
  UI_Text(blbuf, SL_X + SL_W - Layout_TextW(blbuf, 2), ROW_BRIGHT, C_WHITE, 2);

  gfx->fillRoundRect(SL_X, SL_Y, SL_W, SL_H, SL_H / 2, C_DKGRAY);
  int fillW = SL_W * bl / 100;
  gfx->fillRoundRect(SL_X, SL_Y, fillW, SL_H, SL_H / 2, C_CYAN);
  gfx->fillCircle(SL_X + fillW, SL_Y + SL_H / 2, 15, C_WHITE);

  // --- WiFi + where to configure it ---
  UI_Text("WiFi: ", SL_X, ROW_WIFI, C_GRAY, 2);
  int wLabel = Layout_TextW("WiFi: ", 2);
  if (WiFi_IsConnected()) {
    UI_Text(WiFi_SSID(), SL_X + wLabel, ROW_WIFI, C_GREEN, 2);
    UI_Text(WiFi_IP(), SL_X, ROW_IP, C_WHITE, 2);
    UI_Text(T(S_WEB_HINT), SL_X, ROW_WEB, C_CYAN, 1);
    UI_Text(WEB_HOSTNAME ".local", SL_X, ROW_WEB + 12, C_CYAN, 1);
  } else if (WiFi_IsAP()) {
    UI_Text(AP_SSID, SL_X + wLabel, ROW_WIFI, C_YELLOW, 2);
    UI_Text(PORTAL_IP, SL_X, ROW_WEB, C_CYAN, 1);
  } else {
    UI_Text(T(S_NOT_CONNECTED), SL_X + wLabel, ROW_WIFI, C_YELLOW, 1);
  }

  // --- Which bearing is at the top ---
  UI_Text(T(S_TOP), SL_X, ROT_Y + 12, C_GRAY, 2);

  uint16_t top = Settings_TopBearing();
  gfx->fillRoundRect(ROT_MINUS_X, ROT_Y, ROT_BTN_W, ROT_H, 8, C_DKGRAY);
  UI_TextCenteredIn("-", ROT_MINUS_X, ROT_BTN_W, ROT_Y + 12, C_WHITE, 2);
  gfx->fillRoundRect(ROT_PLUS_X, ROT_Y, ROT_BTN_W, ROT_H, 8, C_DKGRAY);
  UI_TextCenteredIn("+", ROT_PLUS_X, ROT_BTN_W, ROT_Y + 12, C_WHITE, 2);
  { char rb[8];
    UI_TextCenteredIn(bearingLabel(top, rb, sizeof(rb)),
                      ROT_VAL_X, ROT_VAL_W, ROT_Y + 12, C_YELLOW, 2); }

  // Small compass preview: a ring, a needle and "S" for north. North sits at
  // screen angle (0 - top), the same rule the radar uses.
  {
    gfx->drawCircle(COMPASS_CX, COMPASS_CY, COMPASS_R, C_DKGRAY);
    float a = -(float)top * 0.0174532925f;
    int nx = COMPASS_CX + (int)((COMPASS_R - 6) * sinf(a));
    int ny = COMPASS_CY - (int)((COMPASS_R - 6) * cosf(a));
    gfx->drawLine(COMPASS_CX, COMPASS_CY, nx, ny, C_WHITE);
    gfx->fillCircle(COMPASS_CX, COMPASS_CY, 2, C_GRAY);
    int lx = COMPASS_CX + (int)(COMPASS_R * sinf(a)) - 2;
    int ly = COMPASS_CY - (int)(COMPASS_R * cosf(a)) - 3;
    UI_Text(Lang_Get() == LANG_EN ? "N" : "S", lx, ly, C_WHITE, 1);
  }

  // --- Buttons ---
  gfx->fillRoundRect(BTN_X, BTN0_Y, BTN_W, BTN_H, 10, C_GRAY);
  UI_TextCentered(Settings_MetricUnits() ? T(S_UNITS_METRIC) : T(S_UNITS_AVIA),
                  BTN0_Y + BTN_H / 2 - 8, C_BLACK, 2);

  gfx->fillRoundRect(BTN_X, BTN1_Y, BTN_W, BTN_H, 12, C_CYAN);
  const char* langBtn = (Lang_Get() == LANG_EN) ? "Language: English"
                      : ((Lang_Get() == LANG_SK) ? "Jazyk: slovenčina" : "Jazyk: čeština");
  UI_TextCentered(langBtn, BTN1_Y + BTN_H / 2 - 8, C_BLACK, 2);

  gfx->fillRoundRect(BTN_X, BTN2_Y, BTN_W, BTN_H, 12, C_ORANGE);
  const char* forgetWifiBtn = (Lang_Get() == LANG_EN) ? "Forget WiFi"
                            : ((Lang_Get() == LANG_SK) ? "Zabudnúť WiFi" : "Zapomenout WiFi");
  UI_TextCentered(forgetWifiBtn, BTN2_Y + BTN_H / 2 - 8, C_BLACK, 2);

  UI_TextCentered("H4CKR4", LY_FOOTER, C_GREEN, 2);
}
