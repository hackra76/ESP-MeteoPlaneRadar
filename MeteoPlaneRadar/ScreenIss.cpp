// =============================================================================
//  MeteoPlaneRadar
//  ScreenIss.cpp - International Space Station (ISS) mission control UI screen.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenIss.h"
#include "IssData.h"
#include "WorldMapData.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Config.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "AsyncCore.h"
#include "Settings.h"
#include <WiFi.h>
#include <math.h>

#define MAP_X 80
#define MAP_Y 102
#define MAP_W 320
#define MAP_H 160

static unsigned long s_lastDrawTick = 0;
static unsigned long s_lastTapTime = 0;

void ScreenIss_Enter() {
  Async_RequestIss();
}

bool ScreenIss_Tick() {
  if (Async_TakeIssUpdated()) {
    return true;
  }
  unsigned long now = millis();
  if (now - s_lastDrawTick >= 1000) {
    s_lastDrawTick = now;
    return true;
  }
  return false;
}

static inline bool getMapBit(int x, int y) {
  if (x < 0 || x >= WORLD_MAP_W || y < 0 || y >= WORLD_MAP_H) return false;
  int byteIdx = y * 45 + (x >> 3);
  return (pgm_read_byte(&WORLD_MAP_BITS[byteIdx]) >> (7 - (x & 7))) & 1;
}

static const char* getCompassDirection(float azDeg) {
  static const char* const DIRS[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };
  int idx = (int)((azDeg + 11.25f) / 22.5f) % 16;
  return DIRS[idx];
}

static void drawWorldMap(const IssData& iss) {
  // Outline card container
  gfx->fillRoundRect(MAP_X - 4, MAP_Y - 4, MAP_W + 8, MAP_H + 8, 8, RGB565(8, 16, 28));
  gfx->drawRoundRect(MAP_X - 4, MAP_Y - 4, MAP_W + 8, MAP_H + 8, 8, RGB565(35, 65, 95));

  // Solar sub-point for Day/Night terminator shading
  float solLatRad = iss.solarLat * (float)M_PI / 180.0f;
  float solLonRad = iss.solarLon * (float)M_PI / 180.0f;
  float sinSolLat = sinf(solLatRad);
  float cosSolLat = cosf(solLatRad);

  uint16_t lineBuf[MAP_W];

  for (int y = 0; y < MAP_H; y++) {
    float latDeg = 90.0f - (float)y / (float)(MAP_H - 1) * 180.0f;
    float latRad = latDeg * (float)M_PI / 180.0f;
    float sinLat = sinf(latRad);
    float cosLat = cosf(latRad);
    int srcY = (y * (WORLD_MAP_H - 1)) / (MAP_H - 1);

    for (int x = 0; x < MAP_W; x++) {
      float lonDeg = (float)x / (float)(MAP_W - 1) * 360.0f - 180.0f;
      float lonRad = lonDeg * (float)M_PI / 180.0f;

      float cosZenith = sinLat * sinSolLat + cosLat * cosSolLat * cosf(lonRad - solLonRad);
      bool isDay = (cosZenith > 0.0f);

      int srcX = (x * (WORLD_MAP_W - 1)) / (MAP_W - 1);
      bool isLand = getMapBit(srcX, srcY);

      if (isLand) {
        bool isCoast = (!getMapBit(srcX - 1, srcY) || !getMapBit(srcX + 1, srcY) ||
                        !getMapBit(srcX, srcY - 1) || !getMapBit(srcX, srcY + 1));
        if (isCoast) {
          lineBuf[x] = isDay ? RGB565(56, 189, 248) : RGB565(26, 95, 130);
        } else {
          lineBuf[x] = isDay ? RGB565(24, 52, 68) : RGB565(13, 26, 36);
        }
      } else {
        lineBuf[x] = isDay ? RGB565(10, 22, 38) : RGB565(4, 10, 18);
      }

      // Subtle dotted grid lines
      bool isEq = (y == (MAP_H / 2));
      bool isLatGrid = (y == (MAP_H / 4) || y == (3 * MAP_H / 4));
      bool isLonGrid = (x == (MAP_W / 6) || x == (2 * MAP_W / 6) || x == (3 * MAP_W / 6) ||
                        x == (4 * MAP_W / 6) || x == (5 * MAP_W / 6));

      if (isEq) {
        if ((x & 3) == 0) lineBuf[x] = RGB565(35, 65, 95);
      } else if (isLatGrid && (x & 7) == 0) {
        lineBuf[x] = RGB565(22, 40, 60);
      } else if (isLonGrid && (y & 7) == 0) {
        lineBuf[x] = RGB565(22, 40, 60);
      }
    }
    gfx->draw16bitRGBBitmap(MAP_X, MAP_Y + y, lineBuf, MAP_W, 1);
  }

  // Draw ISS orbit ground track (past 45 min to next 45 min)
  int prevX = -1, prevY = -1;
  for (int dt = -45; dt <= 45; dt += 2) {
    float o_lat = 0.0f, o_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &o_lat, &o_lon);

    int px = MAP_X + (int)((o_lon + 180.0f) / 360.0f * (MAP_W - 1));
    int py = MAP_Y + (int)((90.0f - o_lat) / 180.0f * (MAP_H - 1));

    if (prevX >= 0) {
      if (abs(px - prevX) < (MAP_W / 2)) {
        gfx->drawLine(prevX, prevY, px, py, RGB565(255, 180, 0));
        gfx->drawLine(prevX, prevY + 1, px, py + 1, RGB565(255, 180, 0));
      }
    }
    prevX = px;
    prevY = py;
  }

  // Next orbit pass (dashed forward track +45 to +90 min)
  prevX = -1; prevY = -1;
  for (int dt = 45; dt <= 92; dt += 2) {
    float o_lat = 0.0f, o_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &o_lat, &o_lon);

    int px = MAP_X + (int)((o_lon + 180.0f) / 360.0f * (MAP_W - 1));
    int py = MAP_Y + (int)((90.0f - o_lat) / 180.0f * (MAP_H - 1));

    if (prevX >= 0) {
      if (abs(px - prevX) < (MAP_W / 2) && ((dt / 2) & 1) == 0) {
        gfx->drawLine(prevX, prevY, px, py, RGB565(160, 115, 15));
      }
    }
    prevX = px;
    prevY = py;
  }

  // Current ISS Position
  int ix = MAP_X + (int)((iss.lon + 180.0f) / 360.0f * (MAP_W - 1));
  int iy = MAP_Y + (int)((90.0f - iss.lat) / 180.0f * (MAP_H - 1));

  // Ground Footprint Horizon Ring (~2200 km radius -> ~20 px)
  int fp_r = 20;
  uint16_t fp_col = iss.inRange ? RGB565(0, 255, 150) : RGB565(0, 200, 255);
  gfx->drawCircle(ix, iy, fp_r, fp_col);
  gfx->drawCircle(ix, iy, fp_r + 1, fp_col);

  // Observer Home Pin
  float homeLat = (float)Settings_Lat();
  float homeLon = (float)Settings_Lon();
  int hx = MAP_X + (int)((homeLon + 180.0f) / 360.0f * (MAP_W - 1));
  int hy = MAP_Y + (int)((90.0f - homeLat) / 180.0f * (MAP_H - 1));

  gfx->drawFastHLine(hx - 4, hy, 9, RGB565(255, 220, 0));
  gfx->drawFastVLine(hx, hy - 4, 9, RGB565(255, 220, 0));
  gfx->fillCircle(hx, hy, 2, C_RED);

  // ISS Satellite Icon (Solar arrays + Module)
  gfx->fillRect(ix - 6, iy - 2, 13, 5, C_WHITE);
  gfx->drawFastVLine(ix - 5, iy - 4, 9, RGB565(0, 220, 255));
  gfx->drawFastVLine(ix + 5, iy - 4, 9, RGB565(0, 220, 255));
  gfx->fillCircle(ix, iy, 2, C_RED);
}

void ScreenIss_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  Layout_ReserveBand(LY_DOTS - 6, 12);

  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  IssData iss;
  bool valid = Iss_GetData(&iss);

  // Top Screen Title (y=36)
  const char* title = isEn ? "ISS ORBIT TRACKER" : (isSk ? "VESMÍRNA STANICA ISS" : "VESMÍRNÁ STANICE ISS");
  UI_TextCentered(title, 36, C_WHITE, 1);

  // Live status dot
  int liveX = 240 + (int)(strlen(title) * 4) + 10;
  if (valid && (millis() - iss.lastUpdatedMs < 60000)) {
    gfx->fillCircle(liveX, 33, 3, RGB565(0, 255, 120));
  } else {
    gfx->fillCircle(liveX, 33, 3, RGB565(255, 180, 0));
  }

  // 3 Top HUD Telemetry Capsules (Altitude, Velocity, Distance)
  const int y_hud = 56;
  const int h_hud = 34;
  const int w_hud = 86;
  const int gap_hud = 8;
  const int start_hud = (LCD_WIDTH - (3 * w_hud + 2 * gap_hud)) / 2; // 103 -> 103..377

  char valBuf[32];

  // Capsule 1: Altitude
  int cx = start_hud;
  gfx->fillRoundRect(cx, y_hud, w_hud, h_hud, 6, RGB565(12, 22, 35));
  gfx->drawRoundRect(cx, y_hud, w_hud, h_hud, 6, RGB565(25, 48, 72));
  UI_TextCenteredIn(isEn ? "ALTITUDE" : "VÝŠKA", cx, w_hud, y_hud + 3, RGB565(130, 150, 175), 1);
  if (valid) snprintf(valBuf, sizeof(valBuf), "%.0f km", iss.alt);
  else snprintf(valBuf, sizeof(valBuf), "---");
  UI_TextCenteredIn(valBuf, cx, w_hud, y_hud + 17, RGB565(0, 220, 255), 1);

  // Capsule 2: Velocity
  cx += w_hud + gap_hud;
  gfx->fillRoundRect(cx, y_hud, w_hud, h_hud, 6, RGB565(12, 22, 35));
  gfx->drawRoundRect(cx, y_hud, w_hud, h_hud, 6, RGB565(25, 48, 72));
  UI_TextCenteredIn(isEn ? "VELOCITY" : "RÝCHLOSŤ", cx, w_hud, y_hud + 3, RGB565(130, 150, 175), 1);
  if (valid) snprintf(valBuf, sizeof(valBuf), "%.1fk", iss.velocity / 1000.0f);
  else snprintf(valBuf, sizeof(valBuf), "---");
  UI_TextCenteredIn(valBuf, cx, w_hud, y_hud + 17, C_WHITE, 1);

  // Capsule 3: Range / Distance
  cx += w_hud + gap_hud;
  gfx->fillRoundRect(cx, y_hud, w_hud, h_hud, 6, RGB565(12, 22, 35));
  gfx->drawRoundRect(cx, y_hud, w_hud, h_hud, 6, RGB565(25, 48, 72));
  UI_TextCenteredIn(isEn ? "DISTANCE" : "VZDIAL.", cx, w_hud, y_hud + 3, RGB565(130, 150, 175), 1);
  if (valid) snprintf(valBuf, sizeof(valBuf), "%.0f km", iss.distanceKm);
  else snprintf(valBuf, sizeof(valBuf), "---");
  UI_TextCenteredIn(valBuf, cx, w_hud, y_hud + 17, iss.inRange ? RGB565(0, 255, 180) : RGB565(200, 220, 240), 1);

  // World Map
  drawWorldMap(iss);

  // Bottom Status & Telemetry Card (fit within round display: W=300, X=90, Y=276..366)
  const int y_bot = 276;
  const int h_bot = 90;
  const int w_bot = 300;
  const int x_bot = (LCD_WIDTH - w_bot) / 2; // 90

  gfx->fillRoundRect(x_bot, y_bot, w_bot, h_bot, 10, RGB565(10, 18, 30));
  gfx->drawRoundRect(x_bot, y_bot, w_bot, h_bot, 10, RGB565(30, 55, 80));

  // In-Range Status Pill
  const char* stPill = "OUT OF RANGE";
  uint16_t stPillBg = RGB565(20, 28, 42);
  uint16_t stPillBorder = RGB565(40, 55, 75);
  uint16_t stPillFg = RGB565(140, 160, 180);
  if (iss.isOverhead) {
    stPill = isEn ? "OVERHEAD" : (isSk ? "PRIAMO NAD HLAVOU" : "PŘÍMO NAD HLAVOU");
    stPillBg = RGB565(160, 90, 0);
    stPillBorder = RGB565(255, 180, 0);
    stPillFg = C_WHITE;
  } else if (iss.inRange) {
    stPill = isEn ? "IN RANGE" : (isSk ? "V DOHĽADE" : "V DOHLEDU");
    stPillBg = RGB565(0, 110, 55);
    stPillBorder = RGB565(0, 220, 120);
    stPillFg = C_WHITE;
  } else {
    stPill = isEn ? "OUT OF RANGE" : (isSk ? "MIMO DOHĽAD" : "MIMO DOHLED");
    stPillBg = RGB565(20, 28, 42);
    stPillBorder = RGB565(40, 55, 75);
    stPillFg = RGB565(140, 160, 180);
  }

  int pillW = Layout_TextW(stPill, 1) + 12;
  gfx->fillRoundRect(x_bot + 10, y_bot + 10, pillW, 20, 5, stPillBg);
  gfx->drawRoundRect(x_bot + 10, y_bot + 10, pillW, 20, 5, stPillBorder);
  UI_TextCenteredIn(stPill, x_bot + 10, pillW, y_bot + 14, stPillFg, 1);

  // Azimuth & Sunlight status
  char subBuf[64];
  const char* sunStr = (strcmp(iss.visibility, "daylight") == 0)
                       ? (isEn ? "Sunlit" : "Osvetlená")
                       : (isEn ? "In Shadow" : "V tieni Zeme");
  snprintf(subBuf, sizeof(subBuf), "Az: %.0f° (%s) | %s",
           iss.azimuthDeg, getCompassDirection(iss.azimuthDeg), sunStr);
  UI_Text(subBuf, x_bot + pillW + 14, y_bot + 14, RGB565(190, 215, 240), 1);

  // Next pass prediction line
  char passBuf[64];
  if (iss.inRange) {
    snprintf(passBuf, sizeof(passBuf), isEn ? "Pass in progress! Peak: %.0f°" : "Prelet prebieha! Max: %.0f°",
             iss.nextPassMaxEl > 0.0f ? iss.nextPassMaxEl : iss.elevationDeg);
  } else if (iss.nextPassMinutes > 0) {
    int hrs = iss.nextPassMinutes / 60;
    int mins = iss.nextPassMinutes % 60;
    if (hrs > 0) {
      snprintf(passBuf, sizeof(passBuf), isEn ? "Next pass in %dh %02dm (Max: %.0f°)" : "Najbližší prelet o %dh %02dm (Max: %.0f°)",
               hrs, mins, iss.nextPassMaxEl);
    } else {
      snprintf(passBuf, sizeof(passBuf), isEn ? "Next pass in %d min (Max: %.0f°)" : "Najbližší prelet o %d min (Max: %.0f°)",
               mins, iss.nextPassMaxEl);
    }
  } else {
    snprintf(passBuf, sizeof(passBuf), isEn ? "Calculating next orbital pass..." : "Výpočet najbližšieho preletu...");
  }
  UI_Text(passBuf, x_bot + 12, y_bot + 38, RGB565(160, 200, 240), 1);

  // Orbit & Alert line
  char orbBuf[64];
  bool alertOn = Settings_IssAlert();
  snprintf(orbBuf, sizeof(orbBuf), "%s | %s",
           isEn ? "Orbit: 93 min • Inc: 51.6°" : "Orbita: 93 min • Sklon: 51.6°",
           alertOn ? (isEn ? "Alert: ON" : "Výstraha: ZAP") : (isEn ? "Alert: OFF" : "Výstraha: VYP"));
  UI_Text(orbBuf, x_bot + 12, y_bot + 62, alertOn ? RGB565(0, 220, 160) : RGB565(120, 140, 160), 1);

  // Bottom micro hint
  UI_TextCentered(isEn ? "Dbl-tap: Refresh • Swipe down: Control Center" : "Dvojklik: Obnova • Potiahnutie dolu: Ovládanie", 396, RGB565(90, 110, 130), 1);
}

bool ScreenIss_HandleTap(int x, int y) {
  unsigned long now = millis();
  if (now - s_lastTapTime < 350) {
    // Double tap -> force immediate refresh
    Buzzer_Play(BEEP_CLICK);
    Async_RequestIss();
    s_lastTapTime = 0;
    return true;
  }
  s_lastTapTime = now;
  return false;
}
