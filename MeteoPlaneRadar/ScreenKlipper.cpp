// =============================================================================
//  MeteoPlaneRadar
//  ScreenKlipper.cpp - Moonraker / Klipper 3D Printer telemetry screen.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenKlipper.h"
#include "KlipperData.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Config.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "AsyncCore.h"
#include "Settings.h"
#include "FontEngine.h"
#include <WiFi.h>

static lv_obj_t* s_screenObj = nullptr;
static lv_timer_t* s_timer = nullptr;
static unsigned long s_lastDrawTick = 0;
static unsigned long s_lastTapTime = 0;

static bool ScreenKlipper_HandleTap(int x, int y) {
  (void)x; (void)y;
  unsigned long now = millis();
  if (now - s_lastTapTime < 800) return false;
  s_lastTapTime = now;

  if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
  Klipper_RequestFetch();
  Async_RequestKlipper();
  if (s_screenObj) lv_obj_invalidate(s_screenObj);
  return true;
}

// Format duration helper (e.g. "1h 24m" or "42m 15s")
static void formatDuration(uint32_t sec, char* buf, size_t bufLen) {
  uint32_t h = sec / 3600U;
  uint32_t m = (sec % 3600U) / 60U;
  uint32_t s = sec % 60U;
  if (h > 0) {
    snprintf(buf, bufLen, "%luh %02lum", (unsigned long)h, (unsigned long)m);
  } else if (m > 0) {
    snprintf(buf, bufLen, "%lum %02lus", (unsigned long)m, (unsigned long)s);
  } else {
    snprintf(buf, bufLen, "%lus", (unsigned long)s);
  }
}

// Vector icon rendering primitives for 2026-era 3D printer HUD
static void drawNozzleIcon(int x, int y, uint16_t color) {
  // Heat block
  gfx->fillRect(x, y + 2, 10, 5, color);
  // Tapered nozzle cone
  gfx->fillTriangle(x + 1, y + 7, x + 8, y + 7, x + 5, y + 11, color);
  // Filament inlet tube
  gfx->drawFastVLine(x + 4, y - 2, 4, color);
  gfx->drawFastVLine(x + 5, y - 2, 4, color);
}

static void drawBedIcon(int x, int y, uint16_t color) {
  // Heated bed platform
  gfx->fillRoundRect(x, y + 6, 14, 4, 1, color);
  // Rising heat waves
  gfx->drawFastVLine(x + 3, y + 1, 3, color);
  gfx->drawFastVLine(x + 7, y - 1, 5, color);
  gfx->drawFastVLine(x + 11, y + 1, 3, color);
}

static void drawCubeIcon(int cx, int cy, int size) {
  int dx = (int)(size * 0.866f + 0.5f);
  int halfS = size / 2;

  // Shaded isometric faces
  // Top Face (diamond)
  gfx->fillTriangle(cx, cy - size, cx - dx, cy - halfS, cx, cy, RGB565(125, 211, 252));
  gfx->fillTriangle(cx, cy - size, cx + dx, cy - halfS, cx, cy, RGB565(186, 230, 253));

  // Left Face
  gfx->fillTriangle(cx - dx, cy - halfS, cx - dx, cy + halfS, cx, cy, RGB565(14, 165, 233));
  gfx->fillTriangle(cx - dx, cy + halfS, cx, cy + size, cx, cy, RGB565(2, 132, 199));

  // Right Face
  gfx->fillTriangle(cx, cy, cx + dx, cy - halfS, cx + dx, cy + halfS, RGB565(3, 105, 161));
  gfx->fillTriangle(cx, cy, cx + dx, cy + halfS, cx, cy + size, RGB565(7, 89, 133));

  // Wireframe highlights
  uint16_t wire = RGB565(224, 242, 254);
  gfx->drawLine(cx, cy - size, cx - dx, cy - halfS, wire);
  gfx->drawLine(cx, cy - size, cx + dx, cy - halfS, wire);
  gfx->drawLine(cx - dx, cy - halfS, cx, cy, wire);
  gfx->drawLine(cx + dx, cy - halfS, cx, cy, wire);
  gfx->drawLine(cx, cy, cx, cy + size, wire);
  gfx->drawLine(cx - dx, cy - halfS, cx - dx, cy + halfS, wire);
  gfx->drawLine(cx + dx, cy - halfS, cx + dx, cy + halfS, wire);
  gfx->drawLine(cx - dx, cy + halfS, cx, cy + size, wire);
  gfx->drawLine(cx + dx, cy + halfS, cx, cy + size, wire);
}

static void drawHomingIcon(int cx, int cy, int r, uint16_t color) {
  gfx->drawCircle(cx, cy, r, color);
  gfx->drawCircle(cx, cy, r / 2, color);
  gfx->fillCircle(cx, cy, 2, C_WHITE);
  gfx->drawFastHLine(cx - r - 5, cy, 6, color);
  gfx->drawFastHLine(cx + r, cy, 6, color);
  gfx->drawFastVLine(cx, cy - r - 5, 6, color);
  gfx->drawFastVLine(cx, cy + r, 6, color);
}

static void drawMeshIcon(int cx, int cy, int size, uint16_t color) {
  int half = size / 2;
  gfx->drawRect(cx - half, cy - half, size, size, color);
  gfx->drawFastHLine(cx - half, cy - half / 3, size, color);
  gfx->drawFastHLine(cx - half, cy + half / 3, size, color);
  gfx->drawFastVLine(cx - half / 3, cy - half, size, color);
  gfx->drawFastVLine(cx + half / 3, cy - half, size, color);
  gfx->fillCircle(cx, cy, 2, C_WHITE);
  gfx->fillCircle(cx - half / 3, cy - half / 3, 1, color);
  gfx->fillCircle(cx + half / 3, cy - half / 3, 1, color);
  gfx->fillCircle(cx - half / 3, cy + half / 3, 1, color);
  gfx->fillCircle(cx + half / 3, cy + half / 3, 1, color);
}

static void drawDocIcon(int x, int y, uint16_t color) {
  gfx->drawRoundRect(x, y, 11, 14, 2, color);
  gfx->drawFastHLine(x + 2, y + 4, 7, color);
  gfx->drawFastHLine(x + 2, y + 7, 5, color);
  gfx->drawFastHLine(x + 2, y + 10, 6, color);
}

static void ScreenKlipper_Draw() {
  KlipperStatus k;
  bool valid = Klipper_GetData(&k);
  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  // Subtle circular bezel accent ring
  gfx->drawCircle(LCD_WIDTH / 2, LCD_HEIGHT / 2, 238, RGB565(18, 26, 38));

  // 1. Top Header Title (y=32) & Status Line (y=50)
  const char* customName = Settings_KlipperName();
  char titleStr[48];
  if (customName && customName[0] != '\0') {
    if (strlen(customName) <= 14) {
      snprintf(titleStr, sizeof(titleStr), "%s · KLIPPER", customName);
    } else {
      snprintf(titleStr, sizeof(titleStr), "%.24s", customName);
    }
  } else {
    snprintf(titleStr, sizeof(titleStr), "%s",
             isEn ? "3D PRINTER · KLIPPER" : (isSk ? "3D TLAČIAREŇ · KLIPPER" : "3D TISKÁRNA · KLIPPER"));
  }
  for (char* p = titleStr; *p; ++p) *p = toupper((unsigned char)*p);
  UI_TextCentered(titleStr, 32, RGB565(180, 215, 255), FONT_SMALL);
  UI_DrawStatusLine(50);

  // 2. Dynamic Printer State Badge (y=68, w=200, h=26)
  const int badgeW = 200, badgeH = 26;
  const int badgeX = (LCD_WIDTH - badgeW) / 2;
  const int badgeY = 68;

  uint16_t badgeBg = RGB565(20, 28, 40);
  uint16_t badgeBorder = RGB565(75, 85, 99);
  uint16_t badgeFg = RGB565(156, 163, 175);
  uint16_t badgeDot = RGB565(100, 116, 139);
  const char* stateLabel = "OFFLINE";

  switch (k.detailedState) {
    case KLIPPER_STATE_NO_HOST:
      stateLabel = isEn ? "NO HOST" : "BEZ HOSTU";
      badgeBg = RGB565(26, 32, 44); badgeBorder = RGB565(75, 85, 99); badgeFg = RGB565(156, 163, 175); badgeDot = RGB565(100, 116, 139);
      break;

    case KLIPPER_STATE_OFFLINE:
      stateLabel = isEn ? "DISCONNECTED" : (isSk ? "ODPOJENÉ" : "ODPOJENO");
      badgeBg = RGB565(26, 32, 44); badgeBorder = RGB565(75, 85, 99); badgeFg = RGB565(156, 163, 175); badgeDot = RGB565(100, 116, 139);
      break;

    case KLIPPER_STATE_MCU_SHUTDOWN:
      stateLabel = isEn ? "MCU SHUTDOWN" : (isSk ? "MCU VYPNUTÉ" : "MCU VYPNUTO");
      badgeBg = RGB565(127, 29, 29); badgeBorder = RGB565(239, 68, 68); badgeFg = RGB565(254, 202, 202); badgeDot = RGB565(239, 68, 68);
      break;

    case KLIPPER_STATE_STARTUP:
      stateLabel = isEn ? "STARTING UP" : (isSk ? "ŠTARTOVANIE" : "STARTOVÁNÍ");
      badgeBg = RGB565(39, 39, 42); badgeBorder = RGB565(234, 179, 8); badgeFg = RGB565(254, 240, 138); badgeDot = RGB565(234, 179, 8);
      break;

    case KLIPPER_STATE_ERROR:
      stateLabel = isEn ? "ERROR" : "CHYBA";
      badgeBg = RGB565(127, 29, 29); badgeBorder = RGB565(239, 68, 68); badgeFg = RGB565(252, 165, 165); badgeDot = RGB565(239, 68, 68);
      break;

    case KLIPPER_STATE_CANCELLED:
      stateLabel = isEn ? "CANCELLED" : (isSk ? "ZRUŠENÉ" : "ZRUŠENO");
      badgeBg = RGB565(30, 41, 59); badgeBorder = RGB565(100, 116, 139); badgeFg = RGB565(203, 213, 225); badgeDot = RGB565(100, 116, 139);
      break;

    case KLIPPER_STATE_COMPLETE:
      stateLabel = isEn ? "COMPLETED" : (isSk ? "DOKONČENÉ" : "DOKONČENO");
      badgeBg = RGB565(6, 78, 59); badgeBorder = RGB565(16, 185, 129); badgeFg = RGB565(110, 231, 183); badgeDot = RGB565(16, 185, 129);
      break;

    case KLIPPER_STATE_PAUSED:
      stateLabel = isEn ? "PAUSED" : (isSk ? "POZASTAVENÉ" : "POZASTAVENO");
      badgeBg = RGB565(120, 53, 15); badgeBorder = RGB565(245, 158, 11); badgeFg = RGB565(253, 224, 71); badgeDot = RGB565(245, 158, 11);
      break;

    case KLIPPER_STATE_FILAMENT_RUNOUT:
      stateLabel = isEn ? "FILAMENT RUNOUT" : (isSk ? "VÝMENA FILAMENTU" : "VÝMĚNA FILAMENTU");
      badgeBg = RGB565(136, 19, 55); badgeBorder = RGB565(244, 63, 94); badgeFg = RGB565(254, 205, 211); badgeDot = RGB565(244, 63, 94);
      break;

    case KLIPPER_STATE_HOMING:
      stateLabel = isEn ? "HOMING" : (isSk ? "HÚMOVANIE" : "HOMOVÁNÍ");
      badgeBg = RGB565(49, 46, 129); badgeBorder = RGB565(99, 102, 241); badgeFg = RGB565(199, 210, 254); badgeDot = RGB565(99, 102, 241);
      break;

    case KLIPPER_STATE_PROBING:
      stateLabel = isEn ? "PROBING / MESH" : (isSk ? "KALIBRÁCIA" : "KALIBRACE");
      badgeBg = RGB565(88, 28, 135); badgeBorder = RGB565(168, 85, 247); badgeFg = RGB565(233, 213, 255); badgeDot = RGB565(168, 85, 247);
      break;

    case KLIPPER_STATE_HEAT_SOAK:
      stateLabel = isEn ? "HEAT SOAKING" : (isSk ? "PREHRIEVANIE" : "PROHŘÍVÁNÍ");
      badgeBg = RGB565(120, 53, 15); badgeBorder = RGB565(245, 158, 11); badgeFg = RGB565(254, 240, 138); badgeDot = RGB565(245, 158, 11);
      break;

    case KLIPPER_STATE_HEATING:
      stateLabel = isEn ? "HEATING" : (isSk ? "NAHRIEVANIE" : "NAHŘÍVÁNÍ");
      badgeBg = RGB565(124, 45, 18); badgeBorder = RGB565(249, 115, 22); badgeFg = RGB565(253, 186, 116); badgeDot = RGB565(249, 115, 22);
      break;

    case KLIPPER_STATE_CLEANING:
      stateLabel = isEn ? "PURGING / CLEAN" : (isSk ? "ČISTENIE TRYSKY" : "ČIŠTĚNÍ TRYSKY");
      badgeBg = RGB565(19, 78, 74); badgeBorder = RGB565(20, 184, 166); badgeFg = RGB565(153, 246, 228); badgeDot = RGB565(20, 184, 166);
      break;

    case KLIPPER_STATE_COOLING:
      stateLabel = isEn ? "COOLING DOWN" : (isSk ? "CHLADENIE" : "CHLAZENÍ");
      badgeBg = RGB565(8, 51, 68); badgeBorder = RGB565(6, 182, 212); badgeFg = RGB565(165, 243, 252); badgeDot = RGB565(6, 182, 212);
      break;

    case KLIPPER_STATE_PRINTING:
      stateLabel = isEn ? "PRINTING" : (isSk ? "TLAČÍ SA" : "TISKNE SE");
      badgeBg = RGB565(20, 83, 45); badgeBorder = RGB565(34, 197, 94); badgeFg = RGB565(74, 222, 128); badgeDot = RGB565(34, 197, 94);
      break;

    case KLIPPER_STATE_STANDBY:
    default:
      stateLabel = isEn ? "STANDBY" : (isSk ? "PRIPRAVENÝ" : "PŘIPRAVEN");
      badgeBg = RGB565(12, 74, 110); badgeBorder = RGB565(56, 189, 248); badgeFg = RGB565(125, 211, 252); badgeDot = RGB565(56, 189, 248);
      break;
  }

  gfx->fillRoundRect(badgeX, badgeY, badgeW, badgeH, 13, badgeBg);
  gfx->drawRoundRect(badgeX, badgeY, badgeW, badgeH, 13, badgeBorder);

  // Status LED inside badge
  gfx->fillCircle(badgeX + 15, badgeY + badgeH / 2, 4, badgeDot);
  gfx->drawCircle(badgeX + 15, badgeY + badgeH / 2, 6, badgeBorder);
  UI_TextCenteredBox(stateLabel, badgeX + 24, badgeY, badgeW - 32, badgeH, badgeFg, FONT_SMALL);

  // 3. Central Circular Progress Gauge (cx=240, cy=176, r=74)
  const int cx = LCD_WIDTH / 2;
  const int cy = 176;
  const int rGauge = 74;
  const int gaugeWidth = 8;
  const int startAngle = 135;
  const int totalAngle = 270;

  // Background track & decorative outer bezel arc
  gfx->drawArc(cx, cy, rGauge + 6, startAngle, startAngle + totalAngle, RGB565(30, 42, 60), 1);
  gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(20, 28, 42), gaugeWidth);

  // Active progress arc & center values
  char pctBuf[32] = "---";
  char subGauge[48] = "";
  int pct = (int)roundf(k.progress * 100.0f);
  pct = constrain(pct, 0, 100);

  if (!valid || !k.connected) {
    UI_TextCenteredBox("OFFLINE", cx - 70, cy - 24, 140, 28, RGB565(156, 163, 175), FONT_TITLE);
    UI_TextCenteredBox(isEn ? "Waiting for Host" : (isSk ? "Čaká na hostiteľa" : "Čeká na hostitele"),
                       cx - 85, cy + 8, 170, 20, RGB565(100, 116, 139), FONT_TINY);
  } else if (k.detailedState == KLIPPER_STATE_PRINTING) {
    int sweep = (int)roundf(k.progress * (float)totalAngle);
    if (sweep > 0) {
      uint16_t arcColor = (pct >= 100) ? RGB565(34, 197, 94) : RGB565(56, 189, 248);
      gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + sweep, arcColor, gaugeWidth);
    }
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    if (k.progress > 0.005f) {
      uint32_t etaSec = 0;
      uint32_t totalEst = (uint32_t)((float)k.printDuration / k.progress);
      if (totalEst > k.printDuration) etaSec = totalEst - k.printDuration;
      char durStr[24];
      formatDuration(etaSec, durStr, sizeof(durStr));
      snprintf(subGauge, sizeof(subGauge), "ETA: %s", durStr);
    } else {
      char durStr[24];
      formatDuration(k.printDuration, durStr, sizeof(durStr));
      snprintf(subGauge, sizeof(subGauge), "%s: %s", isEn ? "Time" : "Čas", durStr);
    }
    UI_TextCenteredBox(pctBuf, cx - 70, cy - 36, 140, 36, C_WHITE, FONT_HERO);
    UI_TextCenteredBox(subGauge, cx - 85, cy + 6, 170, 22, RGB565(160, 210, 255), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_HEATING) {
    float heatRatio = 0.0f;
    if (k.extruderTarget > 20.0f) {
      heatRatio = constrain(k.extruderTemp / k.extruderTarget, 0.0f, 1.0f);
    } else if (k.bedTarget > 20.0f) {
      heatRatio = constrain(k.bedTemp / k.bedTarget, 0.0f, 1.0f);
    }
    int sweep = (int)roundf(heatRatio * (float)totalAngle);
    if (sweep > 0) {
      gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + sweep, RGB565(249, 115, 22), gaugeWidth);
    }
    snprintf(pctBuf, sizeof(pctBuf), "%.0f°", (k.extruderTarget > 0.0f) ? k.extruderTemp : k.bedTemp);
    snprintf(subGauge, sizeof(subGauge), "%s: %.0f°C",
             isEn ? "Target" : (isSk ? "Cieľ" : "Cíl"),
             (k.extruderTarget > 0.0f) ? k.extruderTarget : k.bedTarget);
    UI_TextCenteredBox(pctBuf, cx - 70, cy - 36, 140, 36, RGB565(253, 186, 116), FONT_HERO);
    UI_TextCenteredBox(subGauge, cx - 85, cy + 6, 170, 22, RGB565(254, 240, 138), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_HEAT_SOAK) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(245, 158, 11), gaugeWidth);
    UI_TextCenteredBox("SOAK", cx - 70, cy - 36, 140, 36, RGB565(253, 224, 71), FONT_HERO);
    UI_TextCenteredBox(isEn ? "Heat Soaking" : (isSk ? "Prehrievanie" : "Prohřívání"),
                       cx - 85, cy + 6, 170, 22, RGB565(254, 240, 138), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_HOMING) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(99, 102, 241), gaugeWidth);
    drawHomingIcon(cx, cy - 14, 16, RGB565(129, 140, 248));
    UI_TextCenteredBox(isEn ? "HOMING AXES" : (isSk ? "HÚMOVANIE OSÍ" : "HOMOVÁNÍ OS"),
                       cx - 85, cy + 14, 170, 20, RGB565(199, 210, 254), FONT_SMALL);
    UI_TextCenteredBox("G28 ACTIVE", cx - 70, cy + 34, 140, 16, RGB565(129, 140, 248), FONT_TINY);
  } else if (k.detailedState == KLIPPER_STATE_PROBING) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(168, 85, 247), gaugeWidth);
    drawMeshIcon(cx, cy - 14, 24, RGB565(192, 132, 252));
    UI_TextCenteredBox(isEn ? "BED MESH" : (isSk ? "KALIBRÁCIA" : "KALIBRACE"),
                       cx - 85, cy + 14, 170, 20, RGB565(233, 213, 255), FONT_SMALL);
    UI_TextCenteredBox("PROBING LEVEL", cx - 70, cy + 34, 140, 16, RGB565(168, 85, 247), FONT_TINY);
  } else if (k.detailedState == KLIPPER_STATE_CLEANING) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(20, 184, 166), gaugeWidth);
    drawNozzleIcon(cx - 5, cy - 14, RGB565(20, 184, 166));
    UI_TextCenteredBox(isEn ? "PURGING" : (isSk ? "ČISTENIE" : "ČIŠTĚNÍ"),
                       cx - 85, cy + 14, 170, 20, RGB565(153, 246, 228), FONT_SMALL);
    UI_TextCenteredBox("CLEANING NOZZLE", cx - 75, cy + 34, 150, 16, RGB565(20, 184, 166), FONT_TINY);
  } else if (k.detailedState == KLIPPER_STATE_FILAMENT_RUNOUT) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(244, 63, 94), gaugeWidth);
    UI_TextCenteredBox("M600", cx - 70, cy - 36, 140, 36, RGB565(244, 63, 94), FONT_HERO);
    UI_TextCenteredBox(isEn ? "Change Filament" : (isSk ? "Vymeňte filament" : "Vyměňte filament"),
                       cx - 85, cy + 6, 170, 22, RGB565(254, 205, 211), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_PAUSED) {
    int sweep = (int)roundf(k.progress * (float)totalAngle);
    if (sweep > 0) {
      gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + sweep, RGB565(245, 158, 11), gaugeWidth);
    }
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
    UI_TextCenteredBox(pctBuf, cx - 70, cy - 36, 140, 36, RGB565(253, 224, 71), FONT_HERO);
    UI_TextCenteredBox(isEn ? "PAUSED" : (isSk ? "POZASTAVENÉ" : "POZASTAVENO"),
                       cx - 85, cy + 6, 170, 22, RGB565(245, 158, 11), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_COOLING) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(6, 182, 212), gaugeWidth);
    snprintf(pctBuf, sizeof(pctBuf), "%.0f°", k.extruderTemp);
    UI_TextCenteredBox(pctBuf, cx - 70, cy - 36, 140, 36, RGB565(165, 243, 252), FONT_HERO);
    UI_TextCenteredBox(isEn ? "Cooling Down" : (isSk ? "Ochladzovanie" : "Ochlazování"),
                       cx - 85, cy + 6, 170, 22, RGB565(6, 182, 212), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_COMPLETE) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(16, 185, 129), gaugeWidth);
    UI_TextCenteredBox("100%", cx - 70, cy - 36, 140, 36, RGB565(110, 231, 183), FONT_HERO);
    char durStr[24];
    formatDuration(k.totalDuration, durStr, sizeof(durStr));
    snprintf(subGauge, sizeof(subGauge), "%s: %s", isEn ? "Total" : (isSk ? "Celkovo" : "Celkem"), durStr);
    UI_TextCenteredBox(subGauge, cx - 85, cy + 6, 170, 22, RGB565(16, 185, 129), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_CANCELLED) {
    UI_TextCenteredBox("STOP", cx - 70, cy - 36, 140, 36, RGB565(203, 213, 225), FONT_HERO);
    UI_TextCenteredBox(isEn ? "Cancelled" : (isSk ? "Zrušené" : "Zrušeno"),
                       cx - 85, cy + 6, 170, 22, RGB565(100, 116, 139), FONT_SMALL);
  } else if (k.detailedState == KLIPPER_STATE_MCU_SHUTDOWN || k.detailedState == KLIPPER_STATE_ERROR) {
    gfx->drawArc(cx, cy, rGauge, startAngle, startAngle + totalAngle, RGB565(239, 68, 68), gaugeWidth);
    UI_TextCenteredBox("HALT", cx - 70, cy - 36, 140, 36, RGB565(239, 68, 68), FONT_HERO);
    const char* em = k.webhookMsg[0] ? k.webhookMsg : k.message;
    UI_TextCenteredBox(em[0] ? em : (isEn ? "Error" : "Chyba"),
                       cx - 85, cy + 6, 170, 20, RGB565(252, 165, 165), FONT_TINY);
  } else { // STANDBY
    gfx->drawArc(cx, cy, rGauge, 135, 185, RGB565(14, 116, 144), gaugeWidth);
    gfx->drawArc(cx, cy, rGauge, 355, 405, RGB565(14, 116, 144), gaugeWidth);
    drawCubeIcon(cx, cy - 14, 17);
    UI_TextCenteredBox(isEn ? "READY" : (isSk ? "PRIPRAVENÝ" : "PŘIPRAVEN"),
                       cx - 60, cy + 12, 120, 22, RGB565(125, 211, 252), FONT_MEDIUM);
    UI_TextCenteredBox(isEn ? "IDLE" : (isSk ? "NEČINNÝ" : "NEČINNÝ"),
                       cx - 40, cy + 32, 80, 16, RGB565(100, 130, 165), FONT_TINY);
  }

  // 4. Dual Thermal Cards (Hotend & Heatbed) (y=258..326, h=68, w=182)
  const int cardW = 182, cardH = 68;
  const int card1X = 52;
  const int card2X = 246;
  const int cardY = 258;

  // Card A: Hotend
  gfx->fillRoundRect(card1X, cardY, cardW, cardH, 12, RGB565(16, 24, 38));
  gfx->drawRoundRect(card1X, cardY, cardW, cardH, 12, RGB565(42, 58, 82));
  gfx->fillRoundRect(card1X + 2, cardY + 12, 3, cardH - 24, 1, RGB565(249, 115, 22));

  drawNozzleIcon(card1X + 14, cardY + 8, RGB565(249, 115, 22));
  UI_Text(isEn ? "HOTEND" : "TRYSKA", card1X + 32, cardY + 8, RGB565(251, 146, 60), FONT_TINY);

  char tempBuf[32];
  if (valid && k.connected) {
    snprintf(tempBuf, sizeof(tempBuf), "%.0f°", k.extruderTemp);
  } else {
    snprintf(tempBuf, sizeof(tempBuf), "---");
  }
  uint16_t hotColor = (valid && k.extruderTarget > 0 && fabsf(k.extruderTemp - k.extruderTarget) < 3.0f)
                      ? RGB565(255, 160, 80) : C_WHITE;
  UI_TextCenteredBox(tempBuf, card1X, cardY + 18, cardW, 28, hotColor, FONT_HUGE);

  char tgtBuf[32];
  uint16_t tgtColor = RGB565(110, 125, 145);
  if (valid && k.connected && k.extruderTarget > 0) {
    snprintf(tgtBuf, sizeof(tgtBuf), "TGT %.0f°C", k.extruderTarget);
    tgtColor = RGB565(254, 215, 170);
  } else if (valid && k.connected) {
    snprintf(tgtBuf, sizeof(tgtBuf), "OFF");
  } else {
    snprintf(tgtBuf, sizeof(tgtBuf), "--");
  }
  int pillW = 96, pillH = 16;
  int pill1X = card1X + (cardW - pillW) / 2;
  int pillY = cardY + 47;
  gfx->fillRoundRect(pill1X, pillY, pillW, pillH, 8, RGB565(26, 36, 52));
  gfx->drawRoundRect(pill1X, pillY, pillW, pillH, 8, RGB565(48, 66, 92));
  UI_TextCenteredBox(tgtBuf, pill1X, pillY, pillW, pillH, tgtColor, FONT_TINY);

  // Card B: Heatbed
  gfx->fillRoundRect(card2X, cardY, cardW, cardH, 12, RGB565(16, 24, 38));
  gfx->drawRoundRect(card2X, cardY, cardW, cardH, 12, RGB565(42, 58, 82));
  gfx->fillRoundRect(card2X + 2, cardY + 12, 3, cardH - 24, 1, RGB565(6, 182, 212));

  drawBedIcon(card2X + 14, cardY + 8, RGB565(6, 182, 212));
  UI_Text(isEn ? "BED" : "PODLOŽKA", card2X + 32, cardY + 8, RGB565(56, 189, 248), FONT_TINY);

  if (valid && k.connected) {
    snprintf(tempBuf, sizeof(tempBuf), "%.0f°", k.bedTemp);
  } else {
    snprintf(tempBuf, sizeof(tempBuf), "---");
  }
  uint16_t bedColor = (valid && k.bedTarget > 0 && fabsf(k.bedTemp - k.bedTarget) < 2.0f)
                      ? RGB565(100, 210, 255) : C_WHITE;
  UI_TextCenteredBox(tempBuf, card2X, cardY + 18, cardW, 28, bedColor, FONT_HUGE);

  char bedTgtBuf[32];
  uint16_t bedTgtColor = RGB565(110, 125, 145);
  if (valid && k.connected && k.bedTarget > 0) {
    snprintf(bedTgtBuf, sizeof(bedTgtBuf), "TGT %.0f°C", k.bedTarget);
    bedTgtColor = RGB565(186, 230, 253);
  } else if (valid && k.connected) {
    snprintf(bedTgtBuf, sizeof(bedTgtBuf), "OFF");
  } else {
    snprintf(bedTgtBuf, sizeof(bedTgtBuf), "--");
  }
  int pill2X = card2X + (cardW - pillW) / 2;
  gfx->fillRoundRect(pill2X, pillY, pillW, pillH, 8, RGB565(26, 36, 52));
  gfx->drawRoundRect(pill2X, pillY, pillW, pillH, 8, RGB565(48, 66, 92));
  UI_TextCenteredBox(bedTgtBuf, pill2X, pillY, pillW, pillH, bedTgtColor, FONT_TINY);

  // 5. Homing Status & Coordinates Bar (y=334..356)
  const bool homedX = (valid && k.connected && strchr(k.homedAxes, 'x') != nullptr);
  const bool homedY = (valid && k.connected && strchr(k.homedAxes, 'y') != nullptr);
  const bool homedZ = (valid && k.connected && strchr(k.homedAxes, 'z') != nullptr);

  const int axisY = 334;
  const int axW = 26, axH = 20;
  auto drawAxisBadge = [&](int axX, const char* label, bool homed) {
    uint16_t bg = homed ? RGB565(20, 83, 45) : RGB565(28, 34, 46);
    uint16_t br = homed ? RGB565(34, 197, 94) : RGB565(60, 70, 85);
    uint16_t fg = homed ? RGB565(134, 239, 172) : RGB565(120, 130, 145);
    gfx->fillRoundRect(axX, axisY, axW, axH, 5, bg);
    gfx->drawRoundRect(axX, axisY, axW, axH, 5, br);
    UI_TextCenteredBox(label, axX, axisY, axW, axH, fg, FONT_TINY);
  };

  drawAxisBadge(86, "X", homedX);
  drawAxisBadge(116, "Y", homedY);
  drawAxisBadge(146, "Z", homedZ);

  // Toolhead Z position pill
  char posBuf[32];
  if (valid && k.connected) {
    snprintf(posBuf, sizeof(posBuf), "Z: %.2f mm", k.posZ);
  } else {
    snprintf(posBuf, sizeof(posBuf), "Z: ---");
  }
  gfx->fillRoundRect(178, axisY, 112, axH, 5, RGB565(20, 28, 42));
  gfx->drawRoundRect(178, axisY, 112, axH, 5, RGB565(42, 58, 82));
  UI_TextCenteredBox(posBuf, 178, axisY, 112, axH, RGB565(186, 230, 253), FONT_TINY);

  // Filament Used / Job Info Pill
  char chipBuf[32];
  if (valid && k.connected && k.filamentUsed > 0.0f) {
    if (k.filamentUsed >= 1000.0f) {
      snprintf(chipBuf, sizeof(chipBuf), "%.1f m", k.filamentUsed / 1000.0f);
    } else {
      snprintf(chipBuf, sizeof(chipBuf), "%.0f mm", k.filamentUsed);
    }
  } else if (valid && k.connected) {
    snprintf(chipBuf, sizeof(chipBuf), "Flow 100%%");
  } else {
    snprintf(chipBuf, sizeof(chipBuf), "---");
  }
  gfx->fillRoundRect(296, axisY, 98, axH, 5, RGB565(20, 28, 42));
  gfx->drawRoundRect(296, axisY, 98, axH, 5, RGB565(42, 58, 82));
  UI_TextCenteredBox(chipBuf, 296, axisY, 98, axH, RGB565(253, 230, 138), FONT_TINY);

  // 6. Current G-code file Card (y=368..394, w=356, h=26)
  const int fileW = 356, fileH = 26;
  const int fileX = (LCD_WIDTH - fileW) / 2;
  const int fileY = 368;

  gfx->fillRoundRect(fileX, fileY, fileW, fileH, 8, RGB565(16, 24, 38));
  gfx->drawRoundRect(fileX, fileY, fileW, fileH, 8, RGB565(42, 58, 82));
  drawDocIcon(fileX + 10, fileY + 6, RGB565(140, 175, 215));

  if (valid && k.connected && strlen(k.filename) > 0) {
    UI_TextCenteredBox(k.filename, fileX + 26, fileY, fileW - 32, fileH, RGB565(210, 230, 255), FONT_TINY);
  } else if (valid && k.connected && strlen(k.message) > 0 && k.detailedState != KLIPPER_STATE_STANDBY) {
    UI_TextCenteredBox(k.message, fileX + 26, fileY, fileW - 32, fileH, RGB565(160, 175, 195), FONT_TINY);
  } else {
    UI_TextCenteredBox(isEn ? "No active print job" : (isSk ? "Žiadna aktívna tlač" : "Žádný aktivní tisk"),
                       fileX + 26, fileY, fileW - 32, fileH, RGB565(120, 135, 150), FONT_TINY);
  }

  // 7. Bottom Status / Timestamp / Refresh Hint (y=410..422)
  char upBuf[64];
  if (valid && k.lastUpdatedMs > 0) {
    unsigned long ageSec = (millis() - k.lastUpdatedMs) / 1000;
    if (ageSec < 60) {
      snprintf(upBuf, sizeof(upBuf), "%s", isEn ? "Live Telemetry" : "Naživo");
    } else {
      snprintf(upBuf, sizeof(upBuf), "%s: %lum ago", isEn ? "Updated" : (isSk ? "Aktualizované" : "Aktualizováno"), ageSec / 60);
    }
  } else {
    snprintf(upBuf, sizeof(upBuf), "%s",
             isEn ? "Tap screen to refresh" : (isSk ? "Ťuknite pre obnovenie" : "Klepněte pro obnovení"));
  }
  int tw = Font_TextWidth(upBuf, FONT_TINY);
  gfx->fillCircle(LCD_WIDTH / 2 - tw / 2 - 8, 416, 3, (valid && k.connected) ? C_GREEN : C_RED);
  gfx->drawCircle(LCD_WIDTH / 2 - tw / 2 - 8, 416, 5, (valid && k.connected) ? RGB565(20, 83, 45) : RGB565(127, 29, 29));
  UI_TextCentered(upBuf, 412, RGB565(130, 155, 180), FONT_TINY);
}

static void ScreenKlipper_TimerCb(lv_timer_t* t) {
  if (UI_GetActiveScreen() != SCREEN_KLIPPER_I) return;
  if (Async_TakeKlipperUpdated()) {
    lv_obj_invalidate(s_screenObj);
    return;
  }
  unsigned long now = millis();
  if (now - s_lastDrawTick >= 1000) {
    s_lastDrawTick = now;
    lv_obj_invalidate(s_screenObj);
  }
}

static void ScreenKlipper_EventCb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_DRAW_MAIN) {
    lv_layer_t* layer = lv_event_get_layer(e);
    gfx->setLayer(layer);
    ScreenKlipper_Draw();
    gfx->setLayer(nullptr);
  } else if (code == LV_EVENT_CLICKED) {
    if (UI_IsSwipeActive()) return;
    lv_indev_t* indev = lv_indev_active();
    if (indev) {
      lv_point_t pt;
      lv_indev_get_point(indev, &pt);
      ScreenKlipper_HandleTap(pt.x, pt.y);
    }
  } else if (code == LV_EVENT_SCREEN_LOAD_START) {
    Async_RequestKlipper();
  }
}

void ScreenKlipper_Init(lv_obj_t* parent) {
  s_screenObj = parent;
  lv_obj_set_size(s_screenObj, 480, 480);
  lv_obj_center(s_screenObj);
  lv_obj_set_scrollable(s_screenObj, false);
  lv_obj_set_clickable(s_screenObj, true);

  lv_obj_add_event_cb(s_screenObj, ScreenKlipper_EventCb, LV_EVENT_ALL, nullptr);

  s_timer = lv_timer_create(ScreenKlipper_TimerCb, 500, nullptr);
}
