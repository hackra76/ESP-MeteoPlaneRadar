// =============================================================================
//  MeteoPlaneRadar
//  Screen: clock, date, current weather and a seconds ring.
//
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
#include "ADSB.h"
#include "Route.h"
#include "Buzzer.h"
#include "ScreenPlanes.h"
#include "PrecipTracker.h"
#include "QMI8658.h"

#include <time.h>
#include <math.h>
#include <stdio.h>

extern void gotoScreen(int idx);

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
static bool s_overheadActive = false;
static char s_overheadHex[8] = "";
static char s_lastChirpHex[8] = "";
static int  s_cardX = 75, s_cardY = 70, s_cardW = 330, s_cardH = 44;
static bool s_precipActive = false;
static int  s_precipCardX = 85, s_precipCardY = 66, s_precipCardW = 310, s_precipCardH = 40;

void ScreenClock_Enter() { s_lastMin = -1; s_lastSec = -1; }

void ScreenClock_ChangeStyle(int dir) {
  const int total = CLOCK_STYLE_MAX + 1;
  int cur = (int)Settings_ClockStyle();
  int next = (cur + dir) % total;
  if (next < 0) next += total;
  Settings_SetClockStyle((uint8_t)next);
  ScreenClock_Enter();
  Serial.printf("Watchface changed: %d\n", next);
}

bool ScreenClock_Tick() {
  if (!Outside_TimeValid()) return false;
  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);
  if (NightMode_IsUltraNightActive()) {
    if (lt.tm_min == s_lastMin) return false;
    s_lastMin = lt.tm_min;
    return true;
  }
  // Update every second if seconds ring is active
  bool secTick = (Settings_SecondsStyle() != SEC_STYLE_OFF);
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
  if (s_overheadActive && x >= s_cardX && x <= s_cardX + s_cardW && y >= s_cardY && y <= s_cardY + s_cardH) {
    ScreenPlanes_SelectHex(s_overheadHex);
    gotoScreen(SCREEN_PLANES_I);
    return true;
  }
  if (s_precipActive && x >= s_precipCardX && x <= s_precipCardX + s_precipCardW &&
      y >= s_precipCardY && y <= s_precipCardY + s_precipCardH) {
    gotoScreen(SCREEN_METEO_I);
    return true;
  }
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




static void drawHourlyForecastPills(int cy) {
  if (Forecast_HourCount() < 4) return;
  const FcHour* hrs = Forecast_Hours();

  const int n = 3;
  const int pillW = 82, pillH = 44, pillGap = 12;
  const int startX = CX - (n * pillW + (n - 1) * pillGap) / 2;
  const int py = cy - pillH / 2;

  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);

  for (int i = 1; i <= n; i++) {
    int px = startX + (i - 1) * (pillW + pillGap);
    gfx->fillRoundRect(px, py, pillW, pillH, 8, 0x0821);
    gfx->drawRoundRect(px, py, pillW, pillH, 8, 0x2965);

    // Time: e.g. "15:00"
    char hbuf[10];
    snprintf(hbuf, sizeof(hbuf), "%02d:00", (lt.tm_hour + i) % 24);
    UI_TextCenteredIn(hbuf, px, pillW, py + 4, C_GRAY, 1);

    // Weather Icon
    WxIcon_Draw(px + 18, py + 28, 9, hrs[i].code, Settings_IsNight());

    // Temp
    char tbuf[12];
    snprintf(tbuf, sizeof(tbuf), "%.0f\xC2\xB0", hrs[i].temp);
    uint16_t tcol = (hrs[i].precip > 0.1f) ? C_CYAN : C_WHITE;
    UI_Text(tbuf, px + 36, py + 22, tcol, 1);
  }
}

static void drawDigitalClock(const struct tm* lt, time_t now) {
  // --- Weekday and date (above the clock) ---
  if (!s_overheadActive && !s_precipActive && Settings_ClockShowDate()) {
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
    snprintf(tbuf, sizeof(tbuf), "%d\xC2\xB0""C", (int)lroundf(Forecast_CurrentTemp()));

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

    // --- 3-Hour Mini-Forecast Pills ---
    drawHourlyForecastPills(338);
  }

  // --- Moon Phase & Illumination Widget ---
  if (Settings_ClockShowMoon()) {
    MoonInfo moon = Astro_GetMoon(now);
    const int mr = 16;
    const int moonY = (Settings_ClockShowWeather() && Forecast_HourCount() >= 4) ? 388 : MOON_ICON_Y;
    const int textY = (Settings_ClockShowWeather() && Forecast_HourCount() >= 4) ? 412 : MOON_TEXT_Y;
    Astro_DrawMoonIcon(CX, moonY, mr, moon.phase);

    char mbuf[40];
    snprintf(mbuf, sizeof(mbuf), "%s  %.0f%%", moon.name, moon.illumination);
    uint8_t msize = 2;
    if (Layout_TextW(mbuf, 2) > 340) msize = 1;
    UI_TextCentered(mbuf, textY, C_LTGRAY, msize);
  }
}

static void drawUltraNightClock(const struct tm* lt, time_t now) {
  // Pure black background with soothing monochrome deep-crimson digits
  const uint16_t cRed = 0xC800;     // Soft deep red
  const uint16_t cDimRed = 0x6000;  // Dim dark red

  char hhmm[8];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt->tm_hour, lt->tm_min);
  UI_TextCentered(hhmm, CY - 40, cRed, 8);

  char date[40];
  if (Lang_Get() == LANG_EN) {
    snprintf(date, sizeof(date), "%s, %s %d",
             Lang_WeekdayShort(lt->tm_wday), Lang_MonthName(lt->tm_mon), lt->tm_mday);
  } else {
    snprintf(date, sizeof(date), "%s, %d. %s",
             Lang_WeekdayShort(lt->tm_wday), lt->tm_mday, Lang_MonthName(lt->tm_mon));
  }
  UI_TextCentered(date, CY + 48, cDimRed, 2);

  if (Settings_ClockShowMoon()) {
    MoonInfo moon = Astro_GetMoon(now);
    Astro_DrawMoonIcon(CX, CY + 110, 12, moon.phase);
  }
}

static void drawOverheadWidget(const Aircraft* ac, float distKm) {
  if (!ac) return;
  const int w = 330;
  const int h = 44;
  const int x = CX - w / 2;
  const int y = 70;
  s_cardX = x; s_cardY = y; s_cardW = w; s_cardH = h;

  // Background glassmorphism pill with subtle border
  gfx->fillRoundRect(x, y, w, h, 10, 0x0948); // deep navy
  gfx->drawRoundRect(x, y, w, h, 10, 0x2CF4); // cyan border

  // Airplane icon
  const int px = x + 18, py = y + 22;
  gfx->fillTriangle(px - 7, py - 4, px + 7, py, px - 7, py + 4, C_CYAN);
  gfx->fillTriangle(px - 11, py, px + 8, py, px - 7, py - 2, C_WHITE);
  gfx->fillCircle(px + 8, py, 2, C_WHITE);

  // Callsign or Hex
  const char* cs = (ac->callsign[0] != '\0') ? ac->callsign : ac->hex;
  UI_Text(cs, x + 36, y + 6, C_WHITE, 2);

  // Route or Aircraft type
  const RouteInfo* rt = Route_GetCached(ac->callsign);
  char routeBuf[32];
  if (rt && rt->iataFrom[0] && rt->iataTo[0]) {
    snprintf(routeBuf, sizeof(routeBuf), "%s > %s", rt->iataFrom, rt->iataTo);
  } else if (ac->type[0] != '\0') {
    snprintf(routeBuf, sizeof(routeBuf), "%s", ac->type);
  } else {
    snprintf(routeBuf, sizeof(routeBuf), "OVERHEAD");
  }
  UI_Text(routeBuf, x + 36, y + 26, C_YELLOW, 1);

  // Right side: Altitude and Distance
  char altBuf[24];
  if (Settings_MetricUnits()) {
    snprintf(altBuf, sizeof(altBuf), "%.0f m", ac->altFt * 0.3048f);
  } else {
    if (ac->altFt >= 10000) snprintf(altBuf, sizeof(altBuf), "FL%d", (int)(ac->altFt / 100));
    else snprintf(altBuf, sizeof(altBuf), "%d ft", (int)ac->altFt);
  }
  char distBuf[24];
  snprintf(distBuf, sizeof(distBuf), "%.1f km", distKm);

  int aw = Layout_TextW(altBuf, 2);
  int dw = Layout_TextW(distBuf, 1);
  UI_Text(altBuf, x + w - aw - 12, y + 6, C_CYAN, 2);
  UI_Text(distBuf, x + w - dw - 12, y + 26, C_LTGRAY, 1);
}

static void drawPrecipWidget(const PrecipAlert* pa) {
  if (!pa) return;
  const int w = 310;
  const int h = 40;
  const int x = CX - w / 2;
  const int y = s_overheadActive ? 370 : 80;
  s_precipCardX = x; s_precipCardY = y; s_precipCardW = w; s_precipCardH = h;

  uint16_t bg = (pa->type == PRECIP_HAIL_STORM) ? 0x2800 : ((pa->type == PRECIP_SNOW) ? 0x0113 : 0x0842);
  uint16_t bcol = (pa->type == PRECIP_HAIL_STORM) ? C_RED : ((pa->type == PRECIP_SNOW) ? 0x7FFF : C_CYAN);

  gfx->fillRoundRect(x, y, w, h, 10, bg);
  gfx->drawRoundRect(x, y, w, h, 10, bcol);

  // Weather symbol / icon indicator
  int iconX = x + 18, iconY = y + 20;
  if (pa->type == PRECIP_SNOW) {
    gfx->fillCircle(iconX, iconY, 4, 0x7FFF);
    gfx->drawFastHLine(iconX - 6, iconY, 13, C_WHITE);
    gfx->drawFastVLine(iconX, iconY - 6, 13, C_WHITE);
  } else if (pa->type == PRECIP_HAIL_STORM) {
    gfx->fillTriangle(iconX, iconY - 8, iconX - 7, iconY + 6, iconX + 7, iconY + 6, C_YELLOW);
    gfx->fillCircle(iconX, iconY + 1, 2, C_RED);
  } else {
    // Rain droplets
    gfx->fillCircle(iconX - 3, iconY + 2, 3, C_CYAN);
    gfx->fillCircle(iconX + 4, iconY - 1, 3, C_CYAN);
  }

  char pbuf[48];
  PrecipTracker_GetStatusText(pbuf, sizeof(pbuf));
  UI_Text(pbuf, x + 34, y + 12, C_WHITE, 2);
}

void ScreenClock_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();

  if (!Outside_TimeValid()) {
    UI_TextCentered(T(S_WIFI_WAIT), CY - 8, C_YELLOW, 2);
    return;
  }

  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);

  if (NightMode_IsUltraNightActive()) {
    drawUltraNightClock(&lt, now);
    return;
  }

  // --- Check Overhead Flight ---
  const Aircraft* overheadAc = nullptr;
  float overheadDist = 0.0f;
  if (Settings_ClockShowOverhead() && Settings_HasLocation()) {
    overheadAc = ADSB_GetOverheadAircraft(Settings_OverheadRadiusKm(), &overheadDist);
  }
  if (overheadAc) {
    s_overheadActive = true;
    strncpy(s_overheadHex, overheadAc->hex, sizeof(s_overheadHex) - 1);
    s_overheadHex[sizeof(s_overheadHex) - 1] = '\0';
    if (strcmp(s_lastChirpHex, overheadAc->hex) != 0) {
      strncpy(s_lastChirpHex, overheadAc->hex, sizeof(s_lastChirpHex) - 1);
      s_lastChirpHex[sizeof(s_lastChirpHex) - 1] = '\0';
      if (Settings_BuzzerOverhead() && ADSB_IsFresh()) {
        Buzzer_Play(BEEP_OVERHEAD);
      }
    }
    Route_Queue(overheadAc->callsign, overheadAc->lat, overheadAc->lon);
  } else {
    s_overheadActive = false;
    s_overheadHex[0] = '\0';
    s_lastChirpHex[0] = '\0';
  }

  // --- Check Approaching Precipitation ---
  const PrecipAlert* precipAlert = nullptr;
  if (Settings_PrecipAlert() && Settings_HasLocation()) {
    const PrecipAlert* pa = PrecipTracker_GetAlert();
    if (pa && (pa->status == PRECIP_STAT_APPROACHING || pa->status == PRECIP_STAT_CURRENTLY_ACTIVE)) {
      precipAlert = pa;
      s_precipActive = true;
      if (pa->alertArmed) {
        PrecipTracker_DismissAlert();
        if (Settings_BuzzerPrecip()) {
          Buzzer_Play(BEEP_PRECIP);
        }
      }
    } else {
      s_precipActive = false;
    }
  } else {
    s_precipActive = false;
  }

  // The screen dots at the top belong to the screen manager
  Layout_ReserveBand(58 - 6, 12);

  // --- Seconds ring (outermost, drawn first) ---
  drawSecondsRing(lt.tm_sec);

  // --- 24h Solar Twilight Arc ---
  if (Settings_ClockShowAstro() && Settings_HasLocation()) {
    Astro_DrawSolarArc(CX, CY, 216, 3, Settings_Lat(), Settings_Lon(), now);
  }

  // --- Render clock face ---
  drawDigitalClock(&lt, now);

  // --- Draw Overhead Flight Banner Overlay if detected ---
  if (s_overheadActive && overheadAc) {
    drawOverheadWidget(overheadAc, overheadDist);
  }

  // --- Draw Approaching Precipitation Banner Overlay if detected ---
  if (s_precipActive && precipAlert) {
    drawPrecipWidget(precipAlert);
  }

  // Night hint footer
  if (!Settings_NightAuto()) {
    const char* m = Settings_IsNight() ? "noc" : "den";
    if (Lang_Get() == LANG_EN) m = Settings_IsNight() ? "night" : "day";
    UI_TextCentered(m, LY_FOOTER, C_DKGRAY, 1);
  }
}
