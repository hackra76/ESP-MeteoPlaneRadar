// =============================================================================
//  MeteoPlaneRadar
//  Screen: clock, date, current weather and a seconds ring.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenClock.h"
#include "Settings.h"
#include "Outside.h"
#include "Forecast.h"
#include "Astro.h"
#include "WxIcon.h"
#include "NightMode.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Config.h"

#include <time.h>
#include <math.h>
#include <stdio.h>

#define CX (LCD_WIDTH / 2)
#define CY (LCD_HEIGHT / 2)

// Vertical stack, read top to bottom: date, time, conditions, wind. The date
// goes above the clock so the 64 px glyphs sit in the widest part of the
// circle. Rows are evenly spaced and the block is centred on the panel.
#define DATE_Y      108     // weekday + date                     (size 2, 16 px)
#define CLK_Y       152     // top of the HH:MM glyphs             (size 8, 64 px)
#define WX_Y        256     // CENTRE of the weather icon row      (icon + temp + rain)
#define WX_ICON_R    20
#define WIND_Y      296     // wind speed on its own line          (size 2)
#define MOON_ICON_Y 344     // CENTRE of large Moon Icon           (R=16, 32 px diameter)
#define MOON_TEXT_Y 376     // Moon name + illumination            (size 2)

static int s_lastMin = -1;
static int s_lastSec = -1;

void ScreenClock_Enter() { s_lastMin = -1; s_lastSec = -1; }

bool ScreenClock_Tick() {
  if (!Outside_TimeValid()) return false;
  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);
  // In analog, orbital, hud, regulator modes or when seconds ring is active, update every second
  bool secTick = (Settings_ClockStyle() == CLOCK_STYLE_ANALOG ||
                  Settings_ClockStyle() == CLOCK_STYLE_ORBITAL ||
                  Settings_ClockStyle() == CLOCK_STYLE_HUD ||
                  Settings_ClockStyle() == CLOCK_STYLE_REGULATOR ||
                  Settings_SecondsStyle() != SEC_STYLE_OFF);
  if (secTick) {
    if (lt.tm_sec == s_lastSec) return false;
    s_lastSec = lt.tm_sec;
    s_lastMin = lt.tm_min;
    return true;
  }
  if (lt.tm_min == s_lastMin) return false;
  s_lastMin = lt.tm_min;
  return true;
}

bool ScreenClock_HandleTap(int x, int y) {
  (void)x; (void)y;
  if (Settings_NightAuto()) return false;    // automatic mode owns the decision
  NightMode_Toggle();
  return true;
}

// Scale an RGB565 colour towards black.
static uint16_t dim(uint16_t c, uint8_t num, uint8_t den) {
  if (den == 0) return 0;
  uint16_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
  r = (uint16_t)((uint32_t)r * num / den);
  g = (uint16_t)((uint32_t)g * num / den);
  b = (uint16_t)((uint32_t)b * num / den);
  return (uint16_t)((r << 11) | (g << 5) | b);
}

// Position of second `s` on the ring. 0 at the top, clockwise.
static void secPos(int s, int r, int* x, int* y) {
  float a = (s * 6.0f - 90.0f) * 0.0174532925f;
  *x = CX + (int)(r * cosf(a));
  *y = CY + (int)(r * sinf(a));
}

static void drawSecondsRing(int sec) {
  const uint8_t style = Settings_SecondsStyle();
  if (style == SEC_STYLE_OFF) return;
  const int R = LY_SEC_RING_R;
  const uint16_t on  = Settings_SecondsColor();
  const uint16_t off = C_DKGRAY;

  switch (style) {
    case SEC_STYLE_DOTS:
      for (int i = 0; i < 60; i++) {
        int x, y; secPos(i, R, &x, &y);
        if (i <= sec) gfx->fillCircle(x, y, 3, on);
        else          gfx->fillCircle(x, y, 2, off);
      }
      break;

    case SEC_STYLE_LINE: {
      int px = 0, py = 0;
      for (int i = 0; i <= 60; i++) {
        int x, y; secPos(i, R, &x, &y);
        if (i > 0) gfx->drawLine(px, py, x, y, (i <= sec) ? on : off);
        px = x; py = y;
      }
      int hx, hy; secPos(sec, R, &hx, &hy);
      gfx->fillCircle(hx, hy, 4, on);
      break;
    }

    case SEC_STYLE_COMET: {
      const int TAIL = 12;
      for (int i = 0; i < 60; i++) {
        int x, y; secPos(i, R, &x, &y);
        gfx->drawPixel(x, y, off);
      }
      for (int k = TAIL; k >= 0; k--) {
        int i = sec - k;
        if (i < 0) i += 60;
        int x, y; secPos(i, R, &x, &y);
        uint16_t c = dim(on, (uint8_t)(TAIL + 1 - k), (uint8_t)(TAIL + 1));
        gfx->fillCircle(x, y, (k == 0) ? 4 : (k < 4 ? 3 : 2), c);
      }
      break;
    }

    case SEC_STYLE_RADAR: {
      // Rotating radar sweep beam with trailing gradient spokes
      const int r0 = R - 28, r1 = R + 2;
      const int SPOKES = 8;
      for (int k = SPOKES; k >= 0; k--) {
        int s = sec - k;
        if (s < 0) s += 60;
        float a = (s * 6.0f - 90.0f) * 0.0174532925f;
        float ca = cosf(a), sa = sinf(a);
        int x0 = CX + (int)(r0 * ca), y0 = CY + (int)(r0 * sa);
        int x1 = CX + (int)(r1 * ca), y1 = CY + (int)(r1 * sa);
        uint16_t c = (k == 0) ? on : dim(on, (uint8_t)(SPOKES + 1 - k), (uint8_t)(SPOKES + 2));
        gfx->drawLine(x0, y0, x1, y1, c);
      }
      int hx, hy; secPos(sec, r1, &hx, &hy);
      gfx->fillCircle(hx, hy, 3, C_WHITE);
      break;
    }

    case SEC_STYLE_TICKS: {
      // Swiss chronometer tick marks
      for (int i = 0; i < 60; i++) {
        bool isMajor = (i % 5 == 0);
        int len = isMajor ? 12 : 6;
        float a = (i * 6.0f - 90.0f) * 0.0174532925f;
        float ca = cosf(a), sa = sinf(a);
        int x0 = CX + (int)((R - len) * ca), y0 = CY + (int)((R - len) * sa);
        int x1 = CX + (int)((R + 2) * ca),   y1 = CY + (int)((R + 2) * sa);
        uint16_t c = (i <= sec) ? on : off;
        gfx->drawLine(x0, y0, x1, y1, c);
        if (isMajor && i <= sec) {
          gfx->drawPixel(x0 + 1, y0, c);
          gfx->drawPixel(x1 + 1, y1, c);
        }
      }
      int tx, ty; secPos(sec, R - 2, &tx, &ty);
      gfx->fillCircle(tx, ty, 3, C_WHITE);
      break;
    }

    case SEC_STYLE_ORBIT: {
      gfx->drawCircle(CX, CY, R, off);
      float a = (sec * 6.0f - 90.0f) * 0.0174532925f;
      float ca = cosf(a), sa = sinf(a);
      int sx = CX + (int)(R * ca), sy = CY + (int)(R * sa);
      float pa = a + 1.5707963f;
      float pca = cosf(pa), psa = sinf(pa);
      int w1x = sx + (int)(5.0f * pca), w1y = sy + (int)(5.0f * psa);
      int w2x = sx - (int)(5.0f * pca), w2y = sy - (int)(5.0f * psa);
      gfx->drawLine(w1x, w1y, w2x, w2y, C_CYAN);
      gfx->fillCircle(sx, sy, 3, on);
      gfx->fillCircle(sx, sy, 1, C_WHITE);
      break;
    }
  }
}

// Draw a tapered luminous hand for analog clock
static void drawHand(float angleDeg, float len, float backLen, float w, uint16_t color) {
  float rad = (angleDeg - 90.0f) * 0.0174532925f;
  float perp = rad + 1.5707963f;
  float cr = cosf(rad), sr = sinf(rad);
  float cp = cosf(perp), sp = sinf(perp);

  int xTip = CX + (int)(len * cr);
  int yTip = CY + (int)(len * sr);
  int xTail = CX - (int)(backLen * cr);
  int yTail = CY - (int)(backLen * sr);

  int xL = CX + (int)(w * cp);
  int yL = CY + (int)(w * sp);
  int xR = CX - (int)(w * cp);
  int yR = CY - (int)(w * sp);

  gfx->fillTriangle(xTail, yTail, xL, yL, xTip, yTip, color);
  gfx->fillTriangle(xTail, yTail, xR, yR, xTip, yTip, color);
}

// Draw a slender seconds needle with counterbalance ring
static void drawSecondHand(float angleDeg, uint16_t color) {
  float rad = (angleDeg - 90.0f) * 0.0174532925f;
  float cr = cosf(rad), sr = sinf(rad);
  int xTip = CX + (int)(175.0f * cr);
  int yTip = CY + (int)(175.0f * sr);
  int xTail = CX - (int)(36.0f * cr);
  int yTail = CY - (int)(36.0f * sr);
  gfx->drawLine(xTail, yTail, xTip, yTip, color);

  // Counterbalance ring
  int xRing = CX - (int)(22.0f * cr);
  int yRing = CY - (int)(22.0f * sr);
  gfx->fillCircle(xRing, yRing, 5, color);
  gfx->fillCircle(xRing, yRing, 2, C_BLACK);
}

static void drawAnalogClock(const struct tm* lt, time_t now) {
  // 1. Hour numerals (12, 3, 6, 9)
  UI_TextCentered("12", 52, C_WHITE, 3);
  UI_TextCentered("3", CY - 10, C_WHITE, 3);
  Font_DrawCentered("3", CX + 165, CY - 10, C_WHITE, 3);
  UI_TextCentered("6", LCD_HEIGHT - 72, C_WHITE, 3);
  Font_DrawCentered("9", CX - 165, CY - 10, C_WHITE, 3);

  // Hour tick marks for other hours
  for (int h = 1; h <= 12; h++) {
    if (h % 3 == 0) continue;
    float a = (h * 30.0f - 90.0f) * 0.0174532925f;
    float ca = cosf(a), sa = sinf(a);
    int x0 = CX + (int)(168.0f * ca), y0 = CY + (int)(168.0f * sa);
    int x1 = CX + (int)(182.0f * ca), y1 = CY + (int)(182.0f * sa);
    gfx->drawLine(x0, y0, x1, y1, C_WHITE);
  }

  // 2. Complications (sub-dials)
  // Left sub-dial: Weather icon + temperature
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    const int subX = CX - 80, subY = CY;
    gfx->drawCircle(subX, subY, 36, C_DKGRAY);
    WxIcon_Draw(subX, subY - 9, 13, Forecast_CurrentCode(), Settings_IsNight());
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));
    Font_DrawCentered(tbuf, subX, subY + 9, C_WHITE, 2);
  }

  // Right sub-dial: Moon phase icon & % illumination
  if (Settings_ClockShowMoon()) {
    const int subX = CX + 80, subY = CY;
    gfx->drawCircle(subX, subY, 36, C_DKGRAY);
    MoonInfo moon = Astro_GetMoon(now);
    Astro_DrawMoonIcon(subX, subY - 9, 13, moon.phase);
    char mbuf[16];
    snprintf(mbuf, sizeof(mbuf), "%.0f%%", moon.illumination);
    Font_DrawCentered(mbuf, subX, subY + 10, C_LTGRAY, 1);
  }

  // Date window (top center)
  if (Settings_ClockShowDate()) {
    char date[40];
    if (Lang_Get() == LANG_EN) {
      snprintf(date, sizeof(date), "%s %d %s",
               Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    } else {
      snprintf(date, sizeof(date), "%s %d. %s",
               Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    }
    UI_TextCentered(date, 115, C_GRAY, 2);
  }

  // Wind at bottom center
  if (Settings_ClockShowWind() && Forecast_CurrentValid()) {
    char wbuf[16];
    snprintf(wbuf, sizeof(wbuf), "%d km/h", (int)lroundf(Forecast_CurrentWind()));
    UI_TextCentered(wbuf, CY + 85, C_GRAY, 2);
  }

  // 3. Hands
  float hourAngle = (lt->tm_hour % 12 + lt->tm_min / 60.0f) * 30.0f;
  float minAngle  = (lt->tm_min + lt->tm_sec / 60.0f) * 6.0f;
  float secAngle  = lt->tm_sec * 6.0f;

  // Luminous hour hand
  drawHand(hourAngle, 85.0f, 18.0f, 7.0f, C_WHITE);
  drawHand(hourAngle, 72.0f, 12.0f, 3.0f, Settings_ClockColor());

  // Luminous minute hand
  drawHand(minAngle, 135.0f, 22.0f, 5.0f, C_WHITE);
  drawHand(minAngle, 122.0f, 15.0f, 2.5f, Settings_ClockColor());

  // Second hand
  drawSecondHand(secAngle, Settings_SecondsColor());

  // Center hub cap
  gfx->fillCircle(CX, CY, 9, C_DKGRAY);
  gfx->fillCircle(CX, CY, 6, C_WHITE);
  gfx->fillCircle(CX, CY, 3, Settings_SecondsColor());
}

static void drawGaugeArc(int cx, int cy, int rIn, int rOut, float startDeg, float endDeg, uint16_t color) {
  if (endDeg <= startDeg) return;
  for (float deg = startDeg; deg <= endDeg; deg += 1.0f) {
    float rad = (deg - 90.0f) * 0.0174532925f;
    float c = cosf(rad), s = sinf(rad);
    int x0 = cx + (int)(rIn * c), y0 = cy + (int)(rIn * s);
    int x1 = cx + (int)(rOut * c), y1 = cy + (int)(rOut * s);
    gfx->drawLine(x0, y0, x1, y1, color);
  }
}

// 1. Orbital Gauges (Concentric Planetary Arc Rings)
static void drawOrbitalClock(const struct tm* lt, time_t now) {
  // Outer track: Minutes (0..60 min)
  float minDeg = (lt->tm_min + lt->tm_sec / 60.0f) * 6.0f;
  drawGaugeArc(CX, CY, 198, 210, 0.0f, 360.0f, 0x18C3);
  drawGaugeArc(CX, CY, 198, 210, 0.0f, minDeg, Settings_ClockColor());
  float mRad = (minDeg - 90.0f) * 0.0174532925f;
  gfx->fillCircle(CX + (int)(204.0f * cosf(mRad)), CY + (int)(204.0f * sinf(mRad)), 5, C_WHITE);

  // Middle track: Hours (0..12 h)
  float hourDeg = (lt->tm_hour % 12 + lt->tm_min / 60.0f) * 30.0f;
  drawGaugeArc(CX, CY, 170, 182, 0.0f, 360.0f, 0x18C3);
  drawGaugeArc(CX, CY, 170, 182, 0.0f, hourDeg, C_CYAN);
  float hRad = (hourDeg - 90.0f) * 0.0174532925f;
  gfx->fillCircle(CX + (int)(176.0f * cosf(hRad)), CY + (int)(176.0f * sinf(hRad)), 5, C_WHITE);

  // Inner track: Seconds (0..60 s)
  float secDeg = lt->tm_sec * 6.0f;
  drawGaugeArc(CX, CY, 144, 154, 0.0f, 360.0f, 0x1082);
  drawGaugeArc(CX, CY, 144, 154, 0.0f, secDeg, Settings_SecondsColor());
  float sRad = (secDeg - 90.0f) * 0.0174532925f;
  gfx->fillCircle(CX + (int)(149.0f * cosf(sRad)), CY + (int)(149.0f * sinf(sRad)), 4, C_WHITE);

  // Central Core Display
  if (Settings_ClockShowDate()) {
    char date[32];
    snprintf(date, sizeof(date), "%s %d.%d.", Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, lt->tm_mon + 1);
    UI_TextCentered(date, CY - 60, C_LTGRAY, 2);
  }

  // Large digital time in center
  char hhmm[8];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt->tm_hour, lt->tm_min);
  UI_TextCentered(hhmm, CY - 24, C_WHITE, 4);

  // Seconds small badge next to time
  char ssec[8];
  snprintf(ssec, sizeof(ssec), ":%02d", lt->tm_sec);
  UI_Text(ssec, CX + 62, CY - 18, Settings_SecondsColor(), 2);

  // Conditions row below center (Weather & Temp)
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));
    int tw = Layout_TextW(tbuf, 2);
    int totalW = 24 + 8 + tw;
    int x0 = CX - totalW / 2;
    WxIcon_Draw(x0 + 12, CY + 36, 12, Forecast_CurrentCode(), Settings_IsNight());
    UI_Text(tbuf, x0 + 30, CY + 30, C_LTGRAY, 2);
  }

  // Moon phase at bottom
  if (Settings_ClockShowMoon()) {
    MoonInfo moon = Astro_GetMoon(now);
    char mbuf[24];
    snprintf(mbuf, sizeof(mbuf), "%.0f%%", moon.illumination);
    Astro_DrawMoonIcon(CX - 18, CY + 72, 10, moon.phase);
    UI_Text(mbuf, CX + 4, CY + 66, C_GRAY, 1);
  }
}

// 2. Fighter HUD (Cockpit Head-Up Display)
static void drawHudClock(const struct tm* lt, time_t now) {
  uint16_t hudCol = Settings_ClockColor();
  uint16_t dimHud = dim(hudCol, 1, 3);

  // Top Compass Heading Tape
  gfx->drawFastHLine(CX - 140, 68, 280, dimHud);
  for (int h = -40; h <= 40; h += 10) {
    int x = CX + h * 3;
    gfx->drawFastVLine(x, 64, 8, hudCol);
  }
  gfx->fillTriangle(CX - 5, 58, CX + 5, 58, CX, 66, hudCol);
  UI_TextCentered("HDG 360", 42, hudCol, 1);

  // Pitch Ladder
  gfx->drawFastHLine(CX - 130, CY, 60, hudCol);
  gfx->drawFastHLine(CX + 70, CY, 60, hudCol);
  gfx->drawFastVLine(CX - 130, CY, 10, hudCol);
  gfx->drawFastVLine(CX + 130, CY, 10, hudCol);

  gfx->drawFastHLine(CX - 80, CY - 55, 40, dimHud);
  gfx->drawFastVLine(CX - 80, CY - 55, 6, dimHud);
  gfx->drawFastHLine(CX + 40, CY - 55, 40, dimHud);
  gfx->drawFastVLine(CX + 80, CY - 55, 6, dimHud);
  UI_Text("10", CX - 100, CY - 60, dimHud, 1);
  UI_Text("10", CX + 88, CY - 60, dimHud, 1);

  for (int dx = 0; dx < 40; dx += 8) {
    gfx->drawFastHLine(CX - 80 + dx, CY + 55, 4, dimHud);
    gfx->drawFastHLine(CX + 40 + dx, CY + 55, 4, dimHud);
  }
  gfx->drawFastVLine(CX - 80, CY + 49, 6, dimHud);
  gfx->drawFastVLine(CX + 80, CY + 49, 6, dimHud);
  UI_Text("-10", CX - 104, CY + 50, dimHud, 1);
  UI_Text("-10", CX + 88, CY + 50, dimHud, 1);

  // Center Aircraft Reticle
  gfx->drawCircle(CX, CY, 8, hudCol);
  gfx->drawPixel(CX, CY, hudCol);
  gfx->drawFastHLine(CX - 18, CY, 7, hudCol);
  gfx->drawFastHLine(CX + 12, CY, 7, hudCol);

  // Target Lock Box (Locked on time)
  char hhmmss[16];
  snprintf(hhmmss, sizeof(hhmmss), "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
  int tw = Layout_TextW(hhmmss, 3);
  int bx = CX - tw / 2 - 12, by = CY - 18, bw = tw + 24, bh = 36;
  int cLen = 8;
  gfx->drawFastHLine(bx, by, cLen, hudCol);
  gfx->drawFastVLine(bx, by, cLen, hudCol);
  gfx->drawFastHLine(bx + bw - cLen, by, cLen, hudCol);
  gfx->drawFastVLine(bx + bw, by, cLen, hudCol);
  gfx->drawFastHLine(bx, by + bh, cLen, hudCol);
  gfx->drawFastVLine(bx, by + bh - cLen, cLen, hudCol);
  gfx->drawFastHLine(bx + bw - cLen, by + bh, cLen, hudCol);
  gfx->drawFastVLine(bx + bw, by + bh - cLen, cLen, hudCol);
  UI_TextCentered(hhmmss, CY - 11, hudCol, 3);
  UI_Text("SYS·TGT·LOCK", CX - 36, by - 12, dimHud, 1);

  // Left Tape (Weather / Temp)
  gfx->drawFastVLine(55, 120, 240, dimHud);
  for (int y = 120; y <= 360; y += 24) {
    gfx->drawFastHLine(50, y, 6, dimHud);
  }
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));
    gfx->drawRect(18, CY - 14, 46, 28, hudCol);
    UI_Text(tbuf, 24, CY - 6, hudCol, 2);
    WxIcon_Draw(40, CY - 36, 12, Forecast_CurrentCode(), Settings_IsNight());
  }

  // Right Tape (Wind speed)
  gfx->drawFastVLine(425, 120, 240, dimHud);
  for (int y = 120; y <= 360; y += 24) {
    gfx->drawFastHLine(425, y, 6, dimHud);
  }
  if (Settings_ClockShowWind() && Forecast_CurrentValid()) {
    char wbuf[16];
    snprintf(wbuf, sizeof(wbuf), "%d", (int)lroundf(Forecast_CurrentWind()));
    gfx->drawRect(416, CY - 14, 46, 28, hudCol);
    UI_Text(wbuf, 424, CY - 6, hudCol, 2);
    UI_Text("WND", 422, CY - 26, dimHud, 1);
    UI_Text("KMH", 422, CY + 18, dimHud, 1);
  }

  // Tactical Bottom Info Line
  if (Settings_ClockShowDate()) {
    char dLine[48];
    snprintf(dLine, sizeof(dLine), "UTC %02d:%02d · %s %02d",
             lt->tm_hour, lt->tm_min, Lang_MonthName(lt->tm_mon), lt->tm_mday);
    UI_TextCentered(dLine, 410, dimHud, 1);
  }
}

// 3. Observatory Régulateur (Decoupled Axes Chronometer)
static void drawRegulatorClock(const struct tm* lt, time_t now) {
  // Outer 60-Minute Chapter Ring
  for (int m = 0; m < 60; m++) {
    float a = (m * 6.0f - 90.0f) * 0.0174532925f;
    float ca = cosf(a), sa = sinf(a);
    bool major = (m % 5 == 0);
    int r0 = major ? 200 : 208;
    gfx->drawLine(CX + (int)(r0 * ca), CY + (int)(r0 * sa),
                  CX + (int)(216.0f * ca), CY + (int)(216.0f * sa),
                  major ? C_WHITE : C_DKGRAY);
  }

  // Upper Sub-dial: Hours (Center CX, 138, radius 58)
  const int H_CY = 138, H_R = 58;
  gfx->drawCircle(CX, H_CY, H_R, C_DKGRAY);
  gfx->drawCircle(CX, H_CY, H_R - 2, 0x18C3);
  UI_TextCentered("12", H_CY - 50, C_WHITE, 1);
  UI_Text("3", CX + 44, H_CY - 4, C_WHITE, 1);
  UI_TextCentered("6", H_CY + 40, C_WHITE, 1);
  UI_Text("9", CX - 50, H_CY - 4, C_WHITE, 1);

  // Hour hand
  float hourAngle = (lt->tm_hour % 12 + lt->tm_min / 60.0f) * 30.0f;
  float hRad = (hourAngle - 90.0f) * 0.0174532925f;
  float hPerp = hourAngle * 0.0174532925f;
  int hTipX = CX + (int)(42.0f * cosf(hRad));
  int hTipY = H_CY + (int)(42.0f * sinf(hRad));
  int hTailX = CX - (int)(10.0f * cosf(hRad));
  int hTailY = H_CY - (int)(10.0f * sinf(hRad));
  int hLx = CX + (int)(4.0f * cosf(hPerp));
  int hLy = H_CY + (int)(4.0f * sinf(hPerp));
  int hRx = CX - (int)(4.0f * cosf(hPerp));
  int hRy = H_CY - (int)(4.0f * sinf(hPerp));
  gfx->fillTriangle(hTailX, hTailY, hLx, hLy, hTipX, hTipY, Settings_ClockColor());
  gfx->fillTriangle(hTailX, hTailY, hRx, hRy, hTipX, hTipY, Settings_ClockColor());
  gfx->fillCircle(CX, H_CY, 4, C_WHITE);

  // Lower Sub-dial: Seconds (Center CX, 342, radius 58)
  const int S_CY = 342, S_R = 58;
  gfx->drawCircle(CX, S_CY, S_R, C_DKGRAY);
  gfx->drawCircle(CX, S_CY, S_R - 2, 0x18C3);
  UI_TextCentered("60", S_CY - 50, Settings_SecondsColor(), 1);
  UI_Text("15", CX + 42, S_CY - 4, Settings_SecondsColor(), 1);
  UI_TextCentered("30", S_CY + 40, Settings_SecondsColor(), 1);
  UI_Text("45", CX - 50, S_CY - 4, Settings_SecondsColor(), 1);

  // Seconds hand
  float secAngle = lt->tm_sec * 6.0f;
  float sRad = (secAngle - 90.0f) * 0.0174532925f;
  int sTipX = CX + (int)(46.0f * cosf(sRad));
  int sTipY = S_CY + (int)(46.0f * sinf(sRad));
  int sTailX = CX - (int)(12.0f * cosf(sRad));
  int sTailY = S_CY - (int)(12.0f * sinf(sRad));
  gfx->drawLine(sTailX, sTailY, sTipX, sTipY, Settings_SecondsColor());
  gfx->fillCircle(CX, S_CY, 4, Settings_SecondsColor());

  // Complications at 9 o'clock (Weather) and 3 o'clock (Moon)
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    gfx->drawCircle(72, CY, 38, C_DKGRAY);
    WxIcon_Draw(72, CY - 8, 14, Forecast_CurrentCode(), Settings_IsNight());
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));
    UI_TextCentered(tbuf, CY + 14, C_WHITE, 1);
  }

  if (Settings_ClockShowMoon()) {
    gfx->drawCircle(408, CY, 38, C_DKGRAY);
    MoonInfo moon = Astro_GetMoon(now);
    Astro_DrawMoonIcon(408, CY - 8, 12, moon.phase);
    char mbuf[16];
    snprintf(mbuf, sizeof(mbuf), "%.0f%%", moon.illumination);
    UI_TextCentered(mbuf, CY + 14, C_LTGRAY, 1);
  }

  // Large Master Minute Hand (Pivoting at CX, CY)
  float minAngle = (lt->tm_min + lt->tm_sec / 60.0f) * 6.0f;
  drawHand(minAngle, 192.0f, 28.0f, 6.0f, C_WHITE);
  drawHand(minAngle, 178.0f, 20.0f, 3.0f, Settings_ClockColor());
  gfx->fillCircle(CX, CY, 8, C_WHITE);
  gfx->fillCircle(CX, CY, 4, Settings_ClockColor());

  // Date banner
  if (Settings_ClockShowDate()) {
    char date[32];
    snprintf(date, sizeof(date), "%s %d", Lang_MonthName(lt->tm_mon), lt->tm_mday);
    UI_TextCentered(date, CY + 24, C_LTGRAY, 1);
  }
}

// 4. Stacked Bold Typography
static void drawStackedClock(const struct tm* lt, time_t now) {
  // Top giant hours
  char hbuf[4];
  snprintf(hbuf, sizeof(hbuf), "%02d", lt->tm_hour);
  UI_TextCentered(hbuf, 95, Settings_ClockColor(), 9);

  // Bottom giant minutes
  char mbuf[4];
  snprintf(mbuf, sizeof(mbuf), "%02d", lt->tm_min);
  UI_TextCentered(mbuf, 215, Settings_SecondsColor(), 9);

  // Top Date Pill
  if (Settings_ClockShowDate()) {
    char date[40];
    snprintf(date, sizeof(date), "%s %d. %s",
             Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    int dw = Layout_TextW(date, 2);
    gfx->fillRoundRect(CX - dw / 2 - 12, 60, dw + 24, 26, 12, 0x18C3);
    UI_TextCentered(date, 65, C_WHITE, 2);
  }

  // Left Weather Pill
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    gfx->fillRoundRect(36, CY - 40, 56, 80, 16, 0x18C3);
    gfx->drawRoundRect(36, CY - 40, 56, 80, 16, 0x31A6);
    WxIcon_Draw(64, CY - 18, 14, Forecast_CurrentCode(), Settings_IsNight());
    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°", (int)lroundf(Forecast_CurrentTemp()));
    UI_TextCentered(tbuf, CY + 10, C_WHITE, 2);
  }

  // Right Moon Pill
  if (Settings_ClockShowMoon()) {
    gfx->fillRoundRect(388, CY - 40, 56, 80, 16, 0x18C3);
    gfx->drawRoundRect(388, CY - 40, 56, 80, 16, 0x31A6);
    MoonInfo moon = Astro_GetMoon(now);
    Astro_DrawMoonIcon(416, CY - 18, 12, moon.phase);
    char pbuf[16];
    snprintf(pbuf, sizeof(pbuf), "%.0f%%", moon.illumination);
    UI_TextCentered(pbuf, CY + 12, C_LTGRAY, 1);
  }

  // Bottom Wind Pill
  if (Settings_ClockShowWind() && Forecast_CurrentValid()) {
    char wbuf[24];
    snprintf(wbuf, sizeof(wbuf), "%d km/h", (int)lroundf(Forecast_CurrentWind()));
    int ww = Layout_TextW(wbuf, 2);
    gfx->fillRoundRect(CX - ww / 2 - 12, 385, ww + 24, 24, 12, 0x18C3);
    UI_TextCentered(wbuf, 389, C_GRAY, 2);
  }
}

static void drawMinimalClock(const struct tm* lt, time_t now) {
  // Date above
  if (Settings_ClockShowDate()) {
    char date[40];
    if (Lang_Get() == LANG_EN) {
      snprintf(date, sizeof(date), "%s %d %s",
               Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    } else {
      snprintf(date, sizeof(date), "%s %d. %s",
               Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    }
    UI_TextCentered(date, 135, C_GRAY, 2);
  }

  // Giant clean time centered at CY - 18
  char hhmm[8];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt->tm_hour, lt->tm_min);
  UI_TextCentered(hhmm, 180, Settings_ClockColor(), 8);

  // Weather row below
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    char tbuf[32];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));
    int tw = Layout_TextW(tbuf, 3);
    int totalW = 32 + 10 + tw;
    int x0 = CX - totalW / 2;
    WxIcon_Draw(x0 + 16, 290, 16, Forecast_CurrentCode(), Settings_IsNight());
    UI_Text(tbuf, x0 + 38, 290 - 7, C_WHITE, 3);
  }

  // Moon phase below
  if (Settings_ClockShowMoon()) {
    MoonInfo moon = Astro_GetMoon(now);
    char mbuf[40];
    snprintf(mbuf, sizeof(mbuf), "%s  %.0f%%", moon.name, moon.illumination);
    UI_TextCentered(mbuf, 345, C_LTGRAY, 2);
  }
}

static void drawDigitalClock(const struct tm* lt, time_t now) {
  // --- Weekday and date (above the clock) ---
  if (Settings_ClockShowDate()) {
    char date[40];
    if (Lang_Get() == LANG_EN) {
      snprintf(date, sizeof(date), "%s %d %s",
               Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    } else {
      snprintf(date, sizeof(date), "%s %d. %s",
               Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
    }
    uint8_t dsize = 2;
    if (Layout_TextW(date, 2) > 2 * Layout_ChordHalf(DATE_Y + 16) - 16) dsize = 1;
    Layout_ReserveTextCentered(date, dsize, CX, DATE_Y);
    UI_TextCentered(date, DATE_Y, C_GRAY, dsize);
  }

  // --- HH:MM ---
  char hhmm[8];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt->tm_hour, lt->tm_min);
  Layout_ReserveTextCentered(hhmm, 8, CX, CLK_Y);
  UI_TextCentered(hhmm, CLK_Y, Settings_ClockColor(), 8);

  // --- Current conditions: icon, temperature and rain on one row ---
  if (Settings_ClockShowWeather() && Forecast_CurrentValid()) {
    char tbuf[16], pbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));

    const float p = Forecast_CurrentPrecip();
    const bool hasRain = (p >= 0.05f);
    if (hasRain) snprintf(pbuf, sizeof(pbuf), "%.1f mm", p);

    const int iconW = 2 * WX_ICON_R;
    const int tw    = Layout_TextW(tbuf, 3);
    const int pw    = hasRain ? Layout_TextW(pbuf, 2) : 0;
    const int gap   = 14;
    const int pgap  = hasRain ? 18 : 0;
    const int totalW = iconW + gap + tw + pgap + pw;
    const int x0 = CX - totalW / 2;

    if (Layout_Claim(x0 - 6, WX_Y - WX_ICON_R - 3, totalW + 12, 2 * WX_ICON_R + 6)) {
      WxIcon_Draw(x0 + WX_ICON_R, WX_Y, WX_ICON_R,
                  Forecast_CurrentCode(), Settings_IsNight());
      UI_Text(tbuf, x0 + iconW + gap, WX_Y - 7, C_WHITE, 3);
      if (hasRain) {
        UI_Text(pbuf, x0 + iconW + gap + tw + pgap, WX_Y - 5, C_CYAN, 2);
      }
    }

    // --- Wind, on its own line ---
    if (Settings_ClockShowWind()) {
      char wbuf[16];
      snprintf(wbuf, sizeof(wbuf), "%d km/h", (int)lroundf(Forecast_CurrentWind()));
      const int ww = Layout_TextW(wbuf, 2);
      if (Layout_Claim(CX - ww / 2 - 6, WIND_Y - 3, ww + 12, LY_CHAR_H(2) + 6)) {
        UI_TextCentered(wbuf, WIND_Y, C_GRAY, 2);
      }
    }
  }

  // --- Moon Phase & Illumination Widget ---
  if (Settings_ClockShowMoon()) {
    MoonInfo moon = Astro_GetMoon(now);
    const int mr = 16;
    Astro_DrawMoonIcon(CX, MOON_ICON_Y, mr, moon.phase);

    char mbuf[40];
    snprintf(mbuf, sizeof(mbuf), "%s  %.0f%%", moon.name, moon.illumination);
    uint8_t msize = 2;
    if (Layout_TextW(mbuf, 2) > 340) msize = 1;
    UI_TextCentered(mbuf, MOON_TEXT_Y, C_LTGRAY, msize);
  }
}

void ScreenClock_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();

  // The screen dots at the top belong to the screen manager
  Layout_ReserveBand(58 - 6, 12);

  if (!Outside_TimeValid()) {
    UI_TextCentered(T(S_WIFI_WAIT), CY - 8, C_YELLOW, 2);
    return;
  }

  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);

  // --- Seconds ring (outermost, drawn first) ---
  drawSecondsRing(lt.tm_sec);

  // --- 24h Solar Twilight Arc ---
  if (Settings_ClockShowAstro() && Settings_HasLocation()) {
    Astro_DrawSolarArc(CX, CY, 216, 3, Settings_Lat(), Settings_Lon(), now);
  }

  // --- Render clock face by selected style ---
  switch (Settings_ClockStyle()) {
    case CLOCK_STYLE_ANALOG:
      drawAnalogClock(&lt, now);
      break;
    case CLOCK_STYLE_ORBITAL:
      drawOrbitalClock(&lt, now);
      break;
    case CLOCK_STYLE_HUD:
      drawHudClock(&lt, now);
      break;
    case CLOCK_STYLE_REGULATOR:
      drawRegulatorClock(&lt, now);
      break;
    case CLOCK_STYLE_STACKED:
      drawStackedClock(&lt, now);
      break;
    case CLOCK_STYLE_MINIMAL:
      drawMinimalClock(&lt, now);
      break;
    case CLOCK_STYLE_DIGITAL:
    default:
      drawDigitalClock(&lt, now);
      break;
  }

  // Night hint footer
  if (!Settings_NightAuto()) {
    const char* m = Settings_IsNight() ? "noc" : "den";
    if (Lang_Get() == LANG_EN) m = Settings_IsNight() ? "night" : "day";
    UI_TextCentered(m, LY_FOOTER, C_DKGRAY, 1);
  }
}
