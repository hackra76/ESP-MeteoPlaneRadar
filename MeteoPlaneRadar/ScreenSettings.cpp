// =============================================================================
//  MeteoPlaneRadar
//  Screen: settings (brightness, orientation, units, language, web address).
//
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
#include "GithubOTA.h"

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

#define BTN_L_X   75
#define BTN_R_X  245
#define BTN_W    160
#define BTN_H     36
#define BTN_R1_Y 286        // Row 1: Units (left) & Smooth (right)
#define BTN_R2_Y 334        // Row 2: Language (left) & Forget WiFi (right)

#define BTN_OTA_X   100
#define BTN_OTA_Y   378
#define BTN_OTA_W   280
#define BTN_OTA_H    32

#define MODAL_X      50
#define MODAL_Y     110
#define MODAL_W     380
#define MODAL_H     260

static bool s_wantsWifiReset = false;
static bool s_otaModalOpen = false;
static GithubOtaState s_lastOtaState = GH_OTA_IDLE;
static int s_lastOtaProgress = -1;
static unsigned long s_lastAnimTick = 0;
static uint8_t s_animDot = 0;

bool ScreenSettings_WantsWifiReset() { return s_wantsWifiReset; }
void ScreenSettings_ClearWifiReset() { s_wantsWifiReset = false; }

bool ScreenSettings_IsModalOpen() { return s_otaModalOpen; }
void ScreenSettings_CloseModal()  { if (!GithubOTA_IsBusy()) s_otaModalOpen = false; }
void ScreenSettings_OpenOtaModal() { s_otaModalOpen = true; }

void ScreenSettings_Enter() {}

bool ScreenSettings_Tick() {
  if (GithubOTA_IsBusy()) {
    if (!s_otaModalOpen) s_otaModalOpen = true;
  }
  if (s_otaModalOpen) {
    GithubOtaState st = GithubOTA_GetState();
    int prog = GithubOTA_GetProgress();
    unsigned long now = millis();
    if (st != s_lastOtaState || prog != s_lastOtaProgress || (now - s_lastAnimTick >= 250)) {
      s_lastOtaState = st;
      s_lastOtaProgress = prog;
      s_lastAnimTick = now;
      s_animDot = (s_animDot + 1) % 4;
      return true;
    }
  }
  return false;
}

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
  // Modal touch handling
  if (s_otaModalOpen) {
    if (GithubOTA_IsBusy()) return true;
    GithubOtaState st = GithubOTA_GetState();

    if (st == GH_OTA_AVAILABLE) {
      if (x >= MODAL_X + 25 && x <= MODAL_X + 180 && y >= MODAL_Y + 195 && y <= MODAL_Y + 242) {
        GithubOTA_StartUpdateAsync();
        return true;
      }
      if (x >= MODAL_X + 200 && x <= MODAL_X + 355 && y >= MODAL_Y + 195 && y <= MODAL_Y + 242) {
        s_otaModalOpen = false;
        return true;
      }
    } else if (st == GH_OTA_UP_TO_DATE) {
      if (x >= MODAL_X + 25 && x <= MODAL_X + 180 && y >= MODAL_Y + 195 && y <= MODAL_Y + 242) {
        GithubOTA_CheckAsync();
        return true;
      }
      if (x >= MODAL_X + 200 && x <= MODAL_X + 355 && y >= MODAL_Y + 195 && y <= MODAL_Y + 242) {
        s_otaModalOpen = false;
        return true;
      }
    } else if (st == GH_OTA_CHECKING) {
      if (x >= MODAL_X + 115 && x <= MODAL_X + 265 && y >= MODAL_Y + 195 && y <= MODAL_Y + 242) {
        s_otaModalOpen = false;
        return true;
      }
    } else {
      if (x >= MODAL_X + 115 && x <= MODAL_X + 265 && y >= MODAL_Y + 195 && y <= MODAL_Y + 242) {
        s_otaModalOpen = false;
        GithubOTA_Reset();
        return true;
      }
    }

    if (x < MODAL_X || x > MODAL_X + MODAL_W || y < MODAL_Y || y > MODAL_Y + MODAL_H) {
      s_otaModalOpen = false;
      return true;
    }
    return true;
  }

  // OTA Check button on main Settings screen
  if (x >= BTN_OTA_X && x <= BTN_OTA_X + BTN_OTA_W && y >= BTN_OTA_Y && y <= BTN_OTA_Y + BTN_OTA_H) {
    s_otaModalOpen = true;
    if (GithubOTA_GetState() == GH_OTA_IDLE || GithubOTA_GetState() == GH_OTA_ERROR) {
      GithubOTA_CheckAsync();
    }
    return true;
  }

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

  // Row 1: Units (left) & Smooth (right)
  if (y >= BTN_R1_Y && y <= BTN_R1_Y + BTN_H) {
    if (x >= BTN_L_X && x <= BTN_L_X + BTN_W) {
      Settings_SetMetricUnits(!Settings_MetricUnits());
      return true;
    }
    if (x >= BTN_R_X && x <= BTN_R_X + BTN_W) {
      Settings_SetSmoothRadar(!Settings_SmoothRadar());
      return true;
    }
  }

  // Row 2: Language (left) & Forget WiFi (right)
  if (y >= BTN_R2_Y && y <= BTN_R2_Y + BTN_H) {
    if (x >= BTN_L_X && x <= BTN_L_X + BTN_W) {
      uint8_t cur = Lang_Get();
      uint8_t nextLang = (cur == LANG_CZ) ? LANG_SK : ((cur == LANG_SK) ? LANG_EN : LANG_CZ);
      Settings_SetLanguage(nextLang);
      return true;
    }
    if (x >= BTN_R_X && x <= BTN_R_X + BTN_W) {
      s_wantsWifiReset = true;
      return true;
    }
  }

  return false;
}

static void drawOtaModal() {
  const uint8_t lang = Lang_Get();
  const GithubOtaState st = GithubOTA_GetState();

  // Dark card with stylish rounded border
  gfx->fillRoundRect(MODAL_X, MODAL_Y, MODAL_W, MODAL_H, 16, 0x0841);
  uint16_t bcol = (st == GH_OTA_AVAILABLE) ? C_GREEN : ((st == GH_OTA_ERROR) ? C_RED : C_CYAN);
  gfx->drawRoundRect(MODAL_X, MODAL_Y, MODAL_W, MODAL_H, 16, bcol);
  gfx->drawRoundRect(MODAL_X + 1, MODAL_Y + 1, MODAL_W - 2, MODAL_H - 2, 15, bcol);

  // Title
  const char* title = (lang == LANG_EN) ? "Firmware Update"
                    : ((lang == LANG_SK) ? "Aktualizacia firmveru" : "Aktualizace firmwaru");
  UI_TextCenteredIn(title, MODAL_X, MODAL_W, MODAL_Y + 16, C_WHITE, 2);
  gfx->drawFastHLine(MODAL_X + 16, MODAL_Y + 42, MODAL_W - 32, 0x39E7);

  // Current version info
  char curBuf[36];
  snprintf(curBuf, sizeof(curBuf), (lang == LANG_EN) ? "Installed: v%s"
                                  : ((lang == LANG_SK) ? "Nainstalovana: v%s" : "Nainstalovana: v%s"),
           FW_VERSION);
  UI_TextCenteredIn(curBuf, MODAL_X, MODAL_W, MODAL_Y + 52, C_GRAY, 1);

  if (st == GH_OTA_CHECKING) {
    const char* dots[4] = { "Kontrolujem GitHub", "Kontrolujem GitHub.", "Kontrolujem GitHub..", "Kontrolujem GitHub..." };
    const char* dotsEn[4] = { "Checking GitHub", "Checking GitHub.", "Checking GitHub..", "Checking GitHub..." };
    UI_TextCenteredIn((lang == LANG_EN) ? dotsEn[s_animDot] : dots[s_animDot],
                      MODAL_X, MODAL_W, MODAL_Y + 95, C_CYAN, 2);
    UI_TextCenteredIn((lang == LANG_EN) ? "Querying releases..." : "Pripajam sa k serveru...",
                      MODAL_X, MODAL_W, MODAL_Y + 130, C_GRAY, 1);

    gfx->fillRoundRect(MODAL_X + 115, MODAL_Y + 195, 150, 42, 10, C_DKGRAY);
    UI_TextCenteredIn((lang == LANG_EN) ? "Cancel" : "Zrusit",
                      MODAL_X + 115, 150, MODAL_Y + 208, C_WHITE, 2);
  }
  else if (st == GH_OTA_UP_TO_DATE) {
    UI_TextCenteredIn("✓", MODAL_X, MODAL_W, MODAL_Y + 80, C_GREEN, 3);
    const char* upMsg = (lang == LANG_EN) ? "Firmware is up to date"
                      : ((lang == LANG_SK) ? "Mate najnovsiu verziu" : "Mate nejnovejsi verzi");
    UI_TextCenteredIn(upMsg, MODAL_X, MODAL_W, MODAL_Y + 118, C_WHITE, 2);

    char relBuf[32];
    snprintf(relBuf, sizeof(relBuf), "GitHub: %s", GithubOTA_GetLatestVersion());
    UI_TextCenteredIn(relBuf, MODAL_X, MODAL_W, MODAL_Y + 148, C_GRAY, 1);

    gfx->fillRoundRect(MODAL_X + 25, MODAL_Y + 195, 155, 42, 10, C_DKGRAY);
    UI_TextCenteredIn((lang == LANG_EN) ? "Check again" : "Znova",
                      MODAL_X + 25, 155, MODAL_Y + 208, C_WHITE, 2);

    gfx->fillRoundRect(MODAL_X + 200, MODAL_Y + 195, 155, 42, 10, C_CYAN);
    UI_TextCenteredIn((lang == LANG_EN) ? "Close" : ((lang == LANG_SK) ? "Zavriet" : "Zavrit"),
                      MODAL_X + 200, 155, MODAL_Y + 208, C_BLACK, 2);
  }
  else if (st == GH_OTA_AVAILABLE) {
    const char* avMsg = (lang == LANG_EN) ? "New version available!"
                      : ((lang == LANG_SK) ? "Nova verzia je dostupna!" : "Nova verze je dostupna!");
    UI_TextCenteredIn(avMsg, MODAL_X, MODAL_W, MODAL_Y + 76, C_YELLOW, 2);

    UI_TextCenteredIn(GithubOTA_GetLatestVersion(), MODAL_X, MODAL_W, MODAL_Y + 106, C_GREEN, 3);

    const char* titleRel = GithubOTA_GetReleaseTitle();
    if (titleRel && strlen(titleRel) > 0) {
      UI_TextCenteredIn(titleRel, MODAL_X, MODAL_W, MODAL_Y + 146, C_WHITE, 1);
    }

    gfx->fillRoundRect(MODAL_X + 25, MODAL_Y + 195, 155, 44, 10, 0x05E0);
    const char* updTxt = (lang == LANG_EN) ? "Update"
                       : ((lang == LANG_SK) ? "Aktualizovat" : "Aktualizovat");
    UI_TextCenteredIn(updTxt, MODAL_X + 25, 155, MODAL_Y + 208, C_BLACK, 2);

    gfx->fillRoundRect(MODAL_X + 200, MODAL_Y + 195, 155, 44, 10, C_DKGRAY);
    const char* canTxt = (lang == LANG_EN) ? "Cancel"
                       : ((lang == LANG_SK) ? "Zrusit" : "Zrusit");
    UI_TextCenteredIn(canTxt, MODAL_X + 200, 155, MODAL_Y + 208, C_WHITE, 2);
  }
  else if (st == GH_OTA_DOWNLOADING || st == GH_OTA_FLASHING) {
    const char* stTxt = (st == GH_OTA_FLASHING)
      ? ((lang == LANG_EN) ? "Writing to flash..." : ((lang == LANG_SK) ? "Zapisujem do pamate..." : "Zapisuji do pameti..."))
      : ((lang == LANG_EN) ? "Downloading from GitHub..." : ((lang == LANG_SK) ? "Stahujem z GitHubu..." : "Stahuji z GitHubu..."));
    UI_TextCenteredIn(stTxt, MODAL_X, MODAL_W, MODAL_Y + 78, C_CYAN, 2);

    int prog = GithubOTA_GetProgress();
    int barX = MODAL_X + 30;
    int barY = MODAL_Y + 112;
    int barW = MODAL_W - 60;
    int barH = 22;
    gfx->fillRoundRect(barX, barY, barW, barH, 11, 0x2104);
    int fillW = (barW * prog) / 100;
    if (fillW > 0) gfx->fillRoundRect(barX, barY, fillW, barH, 11, C_GREEN);
    gfx->drawRoundRect(barX, barY, barW, barH, 11, C_WHITE);

    char pbuf[32];
    size_t wr = GithubOTA_GetBytesWritten();
    size_t tot = GithubOTA_GetTotalBytes();
    if (tot > 0) {
      snprintf(pbuf, sizeof(pbuf), "%d%% (%.1f / %.1f MB)", prog, wr / 1048576.0f, tot / 1048576.0f);
    } else {
      snprintf(pbuf, sizeof(pbuf), "%d%%", prog);
    }
    UI_TextCenteredIn(pbuf, MODAL_X, MODAL_W, MODAL_Y + 144, C_WHITE, 2);

    const char* pwrWarn = (lang == LANG_EN) ? "DO NOT TURN OFF POWER!"
                        : ((lang == LANG_SK) ? "NEVYPINAJTE NAPAJANIE!" : "NEODPOJUJTE NAPAJENI!");
    UI_TextCenteredIn(pwrWarn, MODAL_X, MODAL_W, MODAL_Y + 195, C_ORANGE, 2);
  }
  else if (st == GH_OTA_SUCCESS) {
    UI_TextCenteredIn("✓", MODAL_X, MODAL_W, MODAL_Y + 76, C_GREEN, 3);
    const char* scTxt = (lang == LANG_EN) ? "Update Successful!"
                      : ((lang == LANG_SK) ? "Aktualizacia uspesna!" : "Aktualizace uspesna!");
    UI_TextCenteredIn(scTxt, MODAL_X, MODAL_W, MODAL_Y + 115, C_GREEN, 2);

    const char* rbTxt = (lang == LANG_EN) ? "Restarting device..."
                      : ((lang == LANG_SK) ? "Restartujem zariadenie..." : "Restartuji zarizeni...");
    UI_TextCenteredIn(rbTxt, MODAL_X, MODAL_W, MODAL_Y + 155, C_WHITE, 2);
  }
  else if (st == GH_OTA_ERROR) {
    const char* erTxt = (lang == LANG_EN) ? "Update Failed"
                      : ((lang == LANG_SK) ? "Aktualizacia zlyhala" : "Aktualizace selhala");
    UI_TextCenteredIn(erTxt, MODAL_X, MODAL_W, MODAL_Y + 76, C_RED, 2);

    const char* errReason = GithubOTA_GetError();
    if (!errReason || strlen(errReason) == 0) errReason = "Unknown error";
    UI_TextCenteredIn(errReason, MODAL_X, MODAL_W, MODAL_Y + 115, C_YELLOW, 1);

    gfx->fillRoundRect(MODAL_X + 115, MODAL_Y + 195, 150, 42, 10, C_DKGRAY);
    UI_TextCenteredIn((lang == LANG_EN) ? "Close" : ((lang == LANG_SK) ? "Zavriet" : "Zavrit"),
                      MODAL_X + 115, 150, MODAL_Y + 208, C_WHITE, 2);
  }
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
    char hostLocal[48];
    snprintf(hostLocal, sizeof(hostLocal), "%s.local", Settings_Hostname());
    UI_Text(hostLocal, SL_X, ROW_WEB + 12, C_CYAN, 1);
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

  // --- Buttons (2x2 grid to preserve ample room above H4CKR4) ---
  // Row 1: Units & Radar Smoothing
  gfx->fillRoundRect(BTN_L_X, BTN_R1_Y, BTN_W, BTN_H, 10, C_GRAY);
  UI_TextCenteredIn(Settings_MetricUnits() ? T(S_UNITS_METRIC) : T(S_UNITS_AVIA),
                    BTN_L_X, BTN_W, BTN_R1_Y + BTN_H / 2 - 8, C_BLACK, 2);

  bool sm = Settings_SmoothRadar();
  gfx->fillRoundRect(BTN_R_X, BTN_R1_Y, BTN_W, BTN_H, 10, sm ? 0x05E0 : C_DKGRAY);
  const char* smBtn = (Lang_Get() == LANG_EN) ? (sm ? "Smooth: ON" : "Smooth: OFF")
                    : ((Lang_Get() == LANG_SK) ? (sm ? "Vyhlad.: ZAP" : "Vyhlad.: VYP")
                                               : (sm ? "Vyhlaz.: ZAP" : "Vyhlaz.: VYP"));
  UI_TextCenteredIn(smBtn, BTN_R_X, BTN_W, BTN_R1_Y + BTN_H / 2 - 8, sm ? C_BLACK : C_WHITE, 2);

  // Row 2: Language & Forget WiFi
  gfx->fillRoundRect(BTN_L_X, BTN_R2_Y, BTN_W, BTN_H, 10, C_CYAN);
  const char* langBtn = (Lang_Get() == LANG_EN) ? "English"
                      : ((Lang_Get() == LANG_SK) ? "Slovencina" : "Cestina");
  UI_TextCenteredIn(langBtn, BTN_L_X, BTN_W, BTN_R2_Y + BTN_H / 2 - 8, C_BLACK, 2);

  gfx->fillRoundRect(BTN_R_X, BTN_R2_Y, BTN_W, BTN_H, 10, C_ORANGE);
  const char* forgetWifiBtn = (Lang_Get() == LANG_EN) ? "Reset WiFi"
                            : ((Lang_Get() == LANG_SK) ? "Reset WiFi" : "Reset WiFi");
  UI_TextCenteredIn(forgetWifiBtn, BTN_R_X, BTN_W, BTN_R2_Y + BTN_H / 2 - 8, C_BLACK, 2);

  // --- OTA Check Button ---
  GithubOtaState st = GithubOTA_GetState();
  if (st == GH_OTA_AVAILABLE) {
    gfx->fillRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, 0x05E0);
    char buf[40];
    const char* fmt = (Lang_Get() == LANG_EN) ? "New: %s (Tap to update)"
                    : ((Lang_Get() == LANG_SK) ? "Nova: %s (Aktualizovat)" : "Nova: %s (Aktualizovat)");
    snprintf(buf, sizeof(buf), fmt, GithubOTA_GetLatestVersion());
    UI_TextCenteredIn(buf, BTN_OTA_X, BTN_OTA_W, BTN_OTA_Y + BTN_OTA_H / 2 - 8, C_BLACK, 2);
  } else if (st == GH_OTA_CHECKING) {
    gfx->fillRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, 0x18E3);
    const char* chkTxt = (Lang_Get() == LANG_EN) ? "Checking GitHub..."
                       : ((Lang_Get() == LANG_SK) ? "Kontrolujem GitHub..." : "Kontroluji GitHub...");
    UI_TextCenteredIn(chkTxt, BTN_OTA_X, BTN_OTA_W, BTN_OTA_Y + BTN_OTA_H / 2 - 8, C_YELLOW, 2);
  } else if (st == GH_OTA_DOWNLOADING || st == GH_OTA_FLASHING) {
    gfx->fillRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, 0x2104);
    char buf[32];
    snprintf(buf, sizeof(buf), "OTA: %d%%", GithubOTA_GetProgress());
    UI_TextCenteredIn(buf, BTN_OTA_X, BTN_OTA_W, BTN_OTA_Y + BTN_OTA_H / 2 - 8, C_GREEN, 2);
  } else if (st == GH_OTA_UP_TO_DATE) {
    gfx->fillRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, 0x10A2);
    gfx->drawRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, 0x2965);
    const char* curTxt = (Lang_Get() == LANG_EN) ? "✓ Firmware is up to date"
                       : ((Lang_Get() == LANG_SK) ? "✓ Verzia je aktualna" : "✓ Verze je aktualni");
    UI_TextCenteredIn(curTxt, BTN_OTA_X, BTN_OTA_W, BTN_OTA_Y + BTN_OTA_H / 2 - 8, 0x8FE0, 2);
  } else {
    gfx->fillRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, C_DKGRAY);
    gfx->drawRoundRect(BTN_OTA_X, BTN_OTA_Y, BTN_OTA_W, BTN_OTA_H, 8, 0x4208);
    const char* btnTxt = (Lang_Get() == LANG_EN) ? "Check for update"
                       : ((Lang_Get() == LANG_SK) ? "Overit aktualizaciu" : "Overit aktualizaci");
    UI_TextCenteredIn(btnTxt, BTN_OTA_X, BTN_OTA_W, BTN_OTA_Y + BTN_OTA_H / 2 - 8, C_WHITE, 2);
  }

  UI_TextCentered("H4CKR4", LY_FOOTER, C_GREEN, 2);

  // If modal is open, draw it on top of the screen
  if (s_otaModalOpen) {
    drawOtaModal();
  }
}

