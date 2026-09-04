// =============================================================================
//  MeteoPlaneRadar
//  QuickControl.cpp - Pull-down Control Center overlay implementation.
// =============================================================================
#include "QuickControl.h"
#include "Display_ST7701.h"
#include "Settings.h"
#include "NightMode.h"
#include "Lang.h"
#include "UI.h"
#include "Layout.h"
#include "AsyncCore.h"

extern void gotoScreen(int idx);

#define SCREEN_CLOCK_I    0
#define SCREEN_PLANES_I   1
#define SCREEN_METEO_I    2
#define SCREEN_TACTICAL_I 3
#define SCREEN_FORECAST_I 4
#define SCREEN_SETTINGS_I 5

static bool s_open = false;

bool QuickControl_IsOpen() { return s_open; }
void QuickControl_Open()   { s_open = true; }
void QuickControl_Close()  { s_open = false; }
void QuickControl_Toggle() { s_open = !s_open; }

static const int CW_W = 360;
static const int CW_H = 266;
static const int CW_X = (LCD_WIDTH - CW_W) / 2;
static const int CW_Y = 24;

static void drawButton(int x, int y, int w, int h, const char* label, bool active, uint16_t activeCol = C_CYAN) {
  uint16_t bg = active ? activeCol : 0x2104; // dark gray if inactive
  uint16_t fg = active ? C_BLACK   : C_WHITE;
  gfx->fillRoundRect(x, y, w, h, 8, bg);
  gfx->drawRoundRect(x, y, w, h, 8, active ? C_WHITE : C_GRAY);
  UI_TextCenteredIn(label, x, w, y + h / 2 - 7, fg, 1);
}

void QuickControl_Draw(int currentScreen) {
  if (!s_open) return;

  // Dark backing card with rounded corners and glowing border
  gfx->fillRoundRect(CW_X, CW_Y, CW_W, CW_H, 16, 0x0821);
  gfx->drawRoundRect(CW_X, CW_Y, CW_W, CW_H, 16, C_CYAN);

  // Title
  UI_TextCenteredIn("OVLÁDACIE CENTRUM", CW_X, CW_W, CW_Y + 10, C_WHITE, 1);

  // Close button 'X' in top-right
  const int cx = CW_X + CW_W - 20, cy = CW_Y + 16;
  gfx->fillCircle(cx, cy, 10, 0x3000);
  gfx->drawCircle(cx, cy, 10, C_GRAY);
  gfx->drawLine(cx - 4, cy - 4, cx + 4, cy + 4, C_WHITE);
  gfx->drawLine(cx - 4, cy + 4, cx + 4, cy - 4, C_WHITE);

  // --- Row 1: Brightness [-] [Jas: XX%] [+] ---
  const int y1 = CW_Y + 36;
  drawButton(CW_X + 16, y1, 44, 32, "-", false);
  drawButton(CW_X + CW_W - 16 - 44, y1, 44, 32, "+", false);

  char bTxt[32];
  snprintf(bTxt, sizeof(bTxt), "%s: %u%%", T(S_BRIGHTNESS), (unsigned)Settings_Backlight());
  UI_TextCenteredIn(bTxt, CW_X + 64, CW_W - 128, y1 + 7, C_YELLOW, 2);

  // --- Row 2: Night Mode & Full Settings ---
  const int y2 = CW_Y + 74;
  const char* nLabel = Settings_NightAuto() ? "Noc: AUTO"
                     : (Settings_IsNight() ? "Noc: ZAP" : "Noc: VYP");
  drawButton(CW_X + 16, y2, 158, 32, nLabel, Settings_IsNight(), 0x3BDF);
  drawButton(CW_X + 186, y2, 158, 32, "Všetky nastavenia", false);

  // Divider
  gfx->drawFastHLine(CW_X + 16, CW_Y + 114, CW_W - 32, 0x31A6);

  // --- Rows 3 & 4: Screen-specific quick toggles ---
  const int y3 = CW_Y + 122;
  const int y4 = CW_Y + 160;

  if (currentScreen == SCREEN_PLANES_I) {
    drawButton(CW_X + 16,  y3, 158, 34, Settings_RadarShowAirports() ? "Letiská: ZAP" : "Letiská: VYP", Settings_RadarShowAirports());
    drawButton(CW_X + 186, y3, 158, 34, Settings_RadarShowRings()    ? "Okruhy: ZAP"  : "Okruhy: VYP",  Settings_RadarShowRings());
    drawButton(CW_X + 16,  y4, 158, 34, Settings_ShowLegends()       ? "Popisy: ZAP"  : "Popisy: VYP",  Settings_ShowLegends());
    drawButton(CW_X + 186, y4, 158, 34, Settings_AutoRotateBearing() ? "Auto-rotácia: ZAP" : "Auto-rotácia: VYP", Settings_AutoRotateBearing());
  } else if (currentScreen == SCREEN_TACTICAL_I) {
    drawButton(CW_X + 16,  y3, 158, 34, Settings_RadarShowTrails()   ? "Trasy: ZAP"   : "Trasy: VYP",   Settings_RadarShowTrails());
    const char* tSrc = (Settings_RadarSource() == RADAR_SRC_SHMU) ? "Zdroj: SHMU" :
                       (Settings_RadarSource() == RADAR_SRC_RAINVIEWER) ? "Zdroj: RainViewer" : "Zdroj: CHMU";
    drawButton(CW_X + 186, y3, 158, 34, tSrc, false);
    const char* smLabel = Settings_SmoothRadar() ? "Vyhladenie: ZAP" : "Vyhladenie: VYP";
    drawButton(CW_X + 16,  y4, 158, 34, smLabel, Settings_SmoothRadar(), 0x07E0);
    drawButton(CW_X + 186, y4, 158, 34, Settings_RadarShowAirports() ? "Letiská: ZAP" : "Letiská: VYP", Settings_RadarShowAirports());
  } else if (currentScreen == SCREEN_METEO_I) {
    const char* mSrc = (Settings_RadarSource() == RADAR_SRC_SHMU) ? "Zdroj: SHMU" :
                       (Settings_RadarSource() == RADAR_SRC_RAINVIEWER) ? "Zdroj: RainViewer" : "Zdroj: CHMU";
    drawButton(CW_X + 16,  y3, 158, 34, mSrc, false);
    const char* smLabel = Settings_SmoothRadar() ? "Vyhladenie: ZAP" : "Vyhladenie: VYP";
    drawButton(CW_X + 186, y3, 158, 34, smLabel, Settings_SmoothRadar(), 0x07E0);
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, "Prepnúť radarový zdroj", true, 0x07E0);
  } else if (currentScreen == SCREEN_CLOCK_I) {
    const char* cStyle = "Ciferník: Digitálny";
    switch (Settings_ClockStyle()) {
      case 0: cStyle = "Ciferník: Digitálny"; break;
      case 1: cStyle = "Ciferník: Analógový"; break;
      case 2: cStyle = "Ciferník: Orbitálny"; break;
      case 3: cStyle = "Ciferník: HUD";       break;
      case 4: cStyle = "Ciferník: Regulátor"; break;
      case 5: cStyle = "Ciferník: Vrstvený";  break;
      case 6: cStyle = "Ciferník: Minimálny"; break;
    }
    drawButton(CW_X + 16,  y3, CW_W - 32, 34, cStyle, false);
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, Settings_ClockShowAstro() ? "Solárny oblúk: ZAP" : "Solárny oblúk: VYP", Settings_ClockShowAstro());
  } else if (currentScreen == SCREEN_FORECAST_I) {
    drawButton(CW_X + 16,  y3, CW_W - 32, 34, Settings_MetricUnits() ? "Jednotky: Metrické" : "Jednotky: Letecké", false);
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, "Aktualizovať predpoveď", true, 0x07E0);
  }

  // Bottom pull-up hint
  UI_TextCenteredIn("^ potiahnutím hore zatvoríte ^", CW_X, CW_W, CW_Y + CW_H - 20, C_GRAY, 1);
}

bool QuickControl_HandleTap(int x, int y, int currentScreen) {
  if (!s_open) return false;

  // Tap outside card -> close
  if (x < CW_X || x > CW_X + CW_W || y < CW_Y || y > CW_Y + CW_H) {
    s_open = false;
    return true;
  }

  // Close button tap
  const int cx = CW_X + CW_W - 20, cy = CW_Y + 16;
  if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= 16 * 16) {
    s_open = false;
    return true;
  }

  // Row 1: Brightness [-] and [+]
  const int y1 = CW_Y + 36;
  if (y >= y1 && y <= y1 + 32) {
    int curB = (int)Settings_Backlight();
    if (x >= CW_X + 16 && x <= CW_X + 16 + 44) {
      curB -= 15;
      if (curB < 10) curB = 10;
      Settings_SetBacklight((uint8_t)curB);
      Set_Backlight((uint8_t)curB);
      return true;
    }
    if (x >= CW_X + CW_W - 16 - 44 && x <= CW_X + CW_W - 16) {
      curB += 15;
      if (curB > 100) curB = 100;
      Settings_SetBacklight((uint8_t)curB);
      Set_Backlight((uint8_t)curB);
      return true;
    }
  }

  // Row 2: Night Mode & Full Settings
  const int y2 = CW_Y + 74;
  if (y >= y2 && y <= y2 + 32) {
    if (x >= CW_X + 16 && x <= CW_X + 174) {
      bool autoN = Settings_NightAuto();
      if (autoN) {
        Settings_SetNightAuto(false);
        Settings_SetNight(true);
      } else if (Settings_IsNight()) {
        Settings_SetNight(false);
      } else {
        Settings_SetNightAuto(true);
      }
      NightMode_Apply();
      return true;
    }
    if (x >= CW_X + 186 && x <= CW_X + 344) {
      s_open = false;
      gotoScreen(SCREEN_SETTINGS_I);
      return true;
    }
  }

  // Rows 3 & 4: Screen-specific toggles
  const int y3 = CW_Y + 122;
  const int y4 = CW_Y + 160;

  if (currentScreen == SCREEN_PLANES_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetRadarShowAirports(!Settings_RadarShowAirports());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetRadarShowRings(!Settings_RadarShowRings());
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetShowLegends(!Settings_ShowLegends());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetAutoRotateBearing(!Settings_AutoRotateBearing());
        return true;
      }
    }
  } else if (currentScreen == SCREEN_TACTICAL_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetRadarShowTrails(!Settings_RadarShowTrails());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        uint8_t curSrc = Settings_RadarSource();
        uint8_t nextSrc = (curSrc == RADAR_SRC_CHMU) ? RADAR_SRC_SHMU :
                          (curSrc == RADAR_SRC_SHMU) ? RADAR_SRC_RAINVIEWER : RADAR_SRC_CHMU;
        Settings_SetRadarSource(nextSrc);
        Async_RequestRadar();
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetSmoothRadar(!Settings_SmoothRadar());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetRadarShowAirports(!Settings_RadarShowAirports());
        return true;
      }
    }
  } else if (currentScreen == SCREEN_METEO_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        uint8_t curSrc = Settings_RadarSource();
        uint8_t nextSrc = (curSrc == RADAR_SRC_CHMU) ? RADAR_SRC_SHMU :
                          (curSrc == RADAR_SRC_SHMU) ? RADAR_SRC_RAINVIEWER : RADAR_SRC_CHMU;
        Settings_SetRadarSource(nextSrc);
        Async_RequestRadar();
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetSmoothRadar(!Settings_SmoothRadar());
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      uint8_t curSrc = Settings_RadarSource();
      uint8_t nextSrc = (curSrc == RADAR_SRC_CHMU) ? RADAR_SRC_SHMU :
                        (curSrc == RADAR_SRC_SHMU) ? RADAR_SRC_RAINVIEWER : RADAR_SRC_CHMU;
      Settings_SetRadarSource(nextSrc);
      Async_RequestRadar();
      return true;
    }
  } else if (currentScreen == SCREEN_CLOCK_I) {
    if (y >= y3 && y <= y3 + 34) {
      uint8_t nextSt = (Settings_ClockStyle() + 1) % 7;
      Settings_SetClockStyle(nextSt);
      return true;
    }
    if (y >= y4 && y <= y4 + 34) {
      Settings_SetClockShowAstro(!Settings_ClockShowAstro());
      return true;
    }
  } else if (currentScreen == SCREEN_FORECAST_I) {
    if (y >= y3 && y <= y3 + 34) {
      Settings_SetMetricUnits(!Settings_MetricUnits());
      return true;
    }
  }

  return true;
}
