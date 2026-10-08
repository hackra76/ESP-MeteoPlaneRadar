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
#include "PetDrawer.h"
#include "Display_ST7701.h"
#include "Config.h"
#include "ADSB.h"
#include "Route.h"
#include "Buzzer.h"
#include "ScreenPlanes.h"
#include "PrecipTracker.h"
#include "QMI8658.h"
#include "FontEngine.h"
#include "MoonTexture.h"
#include <esp_heap_caps.h>

static lv_obj_t* s_screenObj = nullptr;
static lv_timer_t* s_timer = nullptr;

#include <time.h>
#include <math.h>
#include <stdio.h>



#define CX (LCD_WIDTH / 2)
#define CY (LCD_HEIGHT / 2)

// Vertical stack, read top to bottom: date, time, conditions, wind. The date
// goes above the clock so the 64 px glyphs sit in the widest part of the
// circle. Rows are evenly spaced and the block is centred on the panel.
#define DATE_Y      108     // weekday + date                     (size 2, 16 px)
#define CLK_Y       146     // top of the HH:MM glyphs             (size 8, 76 px)
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





static bool ScreenClock_HandleTap(int x, int y) {
  if (s_overheadActive && x >= s_cardX && x <= s_cardX + s_cardW && y >= s_cardY && y <= s_cardY + s_cardH) {
    UI_SwitchScreen(SCREEN_PLANES_I);
    return true;
  }
  if (s_precipActive && x >= s_precipCardX && x <= s_precipCardX + s_precipCardW &&
      y >= s_precipCardY && y <= s_precipCardY + s_precipCardH) {
    UI_SwitchScreen(SCREEN_METEO_I);
    return true;
  }
  // Tap bottom weather sub-dial to jump to weather forecast
  int dx = x - CX, dy = y - 356;
  if (dx * dx + dy * dy <= 58 * 58) {
    UI_SwitchScreen(SCREEN_FORECAST_I);
    return true;
  }
  if (Settings_NightAuto()) return false;
  NightMode_Toggle();
  if (s_screenObj) lv_obj_invalidate(s_screenObj);
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

    const bool showAstro = Settings_ClockShowAstro() && Settings_HasLocation();
    const int wxY   = showAstro ? 248 : WX_Y;
    const int windY = showAstro ? 280 : WIND_Y;
    const int pillY = showAstro ? 322 : 338;

    const int iconW = 2 * WX_ICON_R;
    const int tw    = Layout_TextW(tbuf, 3);
    const int pw    = hasRain ? Layout_TextW(pbuf, 2) : 0;
    const int gap   = 14;
    const int pgap  = hasRain ? 18 : 0;
    const int totalW = iconW + gap + tw + pgap + pw;
    const int x0 = CX - totalW / 2;

    if (Layout_Claim(x0 - 6, wxY - WX_ICON_R - 3, totalW + 12, 2 * WX_ICON_R + 6)) {
      WxIcon_Draw(x0 + WX_ICON_R, wxY, WX_ICON_R,
                  Forecast_CurrentCode(), Settings_IsNight());
      UI_Text(tbuf, x0 + iconW + gap, wxY - 7, C_WHITE, 3);
      if (hasRain) {
        UI_Text(pbuf, x0 + iconW + gap + tw + pgap, wxY - 5, C_CYAN, 2);
      }
    }

    // --- Wind, on its own line ---
    if (Settings_ClockShowWind()) {
      char wbuf[16];
      snprintf(wbuf, sizeof(wbuf), "%d km/h", (int)lroundf(Forecast_CurrentWind()));
      const int ww = Layout_TextW(wbuf, 2);
      if (Layout_Claim(CX - ww / 2 - 6, windY - 3, ww + 12, LY_CHAR_H(2) + 6)) {
        UI_TextCentered(wbuf, windY, C_GRAY, 2);
      }
    }

    // --- 3-Hour Mini-Forecast Pills ---
    drawHourlyForecastPills(pillY);
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

static void drawClockFooter(const struct tm* lt, time_t now) {
  const bool showMoon  = Settings_ClockShowMoon();
  const bool showAstro = Settings_ClockShowAstro() && Settings_HasLocation();
  const bool hasPills  = Settings_ClockShowWeather() && (Forecast_HourCount() >= 4);

  SolarTimes st = {0};
  bool hasSolar = false;
  char sunBuf[32] = "";
  char cntBuf[48] = "";
  uint16_t evtCol = C_GRAY;

  if (showAstro) {
    st = Astro_GetSolar(Settings_Lat(), Settings_Lon(), now);
    if (st.valid) {
      hasSolar = true;
      struct tm sr, ss, gh;
      localtime_r(&st.sunrise,         &sr);
      localtime_r(&st.sunset,          &ss);
      localtime_r(&st.goldenHourStart, &gh);

      bool isEn = (Lang_Get() == LANG_EN);
      snprintf(sunBuf, sizeof(sunBuf), "^ %02d:%02d  |  v %02d:%02d",
               sr.tm_hour, sr.tm_min, ss.tm_hour, ss.tm_min);

      int nowMin = lt->tm_hour * 60 + lt->tm_min;
      int minSR  = sr.tm_hour * 60 + sr.tm_min;
      int minSS  = ss.tm_hour * 60 + ss.tm_min;
      int minGH  = gh.tm_hour * 60 + gh.tm_min;

      bool beforeSR = (nowMin < minSR);
      bool inGolden = (!beforeSR && nowMin >= minGH && nowMin < minSS);
      bool afterSS  = (nowMin >= minSS);
      bool inDay    = (!beforeSR && !inGolden && !afterSS);

      int diffMin = 0;
      if (beforeSR) {
        diffMin = minSR - nowMin;
        evtCol  = 0x7BEF;
        if (diffMin >= 60) snprintf(cntBuf, sizeof(cntBuf), isEn ? "Sunrise in %dh %02dm" : "Vychod o %dh %02dm", diffMin/60, diffMin%60);
        else               snprintf(cntBuf, sizeof(cntBuf), isEn ? "Sunrise in %dm"       : "Vychod o %dm",       diffMin);
      } else if (inDay) {
        diffMin = minGH - nowMin;  if (diffMin < 0) diffMin = 0;
        evtCol  = C_YELLOW;
        if (diffMin >= 60) snprintf(cntBuf, sizeof(cntBuf), isEn ? "Golden hr in %dh %02dm" : "Zlata h. o %dh %02dm", diffMin/60, diffMin%60);
        else               snprintf(cntBuf, sizeof(cntBuf), isEn ? "Golden hr in %dm"        : "Zlata h. o %dm",        diffMin);
      } else if (inGolden) {
        diffMin = minSS - nowMin;  if (diffMin < 0) diffMin = 0;
        evtCol  = 0xFEA0;
        if (diffMin >= 60) snprintf(cntBuf, sizeof(cntBuf), isEn ? "Golden Hour! Sunset %dh %02dm" : "Zlata hodina! %dh %02dm", diffMin/60, diffMin%60);
        else               snprintf(cntBuf, sizeof(cntBuf), isEn ? "Golden Hour! Sunset %dm"       : "Zlata hodina! %dm",       diffMin);
      } else {  // afterSS
        diffMin = (1440 - nowMin) + minSR;
        evtCol  = 0x4208;
        if (diffMin >= 60) snprintf(cntBuf, sizeof(cntBuf), isEn ? "Sunrise in %dh %02dm" : "Vychod o %dh %02dm", diffMin/60, diffMin%60);
        else               snprintf(cntBuf, sizeof(cntBuf), isEn ? "Sunrise in %dm"       : "Vychod o %dm",       diffMin);
      }
    }
  }

  // Render non-overlapping layouts based on active features
  if (showMoon && hasSolar) {
    // Both Moon & Solar: enlarged moon phase indicator centered above sun & event info
    MoonInfo moon = Astro_GetMoon(now);
    char mbuf[40];
    snprintf(mbuf, sizeof(mbuf), "%s  %.0f%%", moon.name, moon.illumination);

    const int iconR = hasPills ? 13 : 16;
    const int moonIconY = hasPills ? 362 : 348;
    const int moonTextY = hasPills ? 382 : 372;
    const int sunY      = hasPills ? 401 : 394;
    const int cntY      = hasPills ? 419 : 413;

    Astro_DrawMoonIcon(CX, moonIconY, iconR, moon.phase);
    UI_TextCentered(mbuf, moonTextY, C_LTGRAY, 1);

    UI_TextCentered(sunBuf, sunY, C_GRAY, 1);
    UI_TextCentered(cntBuf, cntY, evtCol, 1);

  } else if (showMoon && !hasSolar) {
    // Moon only
    MoonInfo moon = Astro_GetMoon(now);
    const int mr = hasPills ? 14 : 16;
    const int moonY = hasPills ? 384 : MOON_ICON_Y;
    const int textY = hasPills ? 410 : MOON_TEXT_Y;
    Astro_DrawMoonIcon(CX, moonY, mr, moon.phase);

    char mbuf[40];
    snprintf(mbuf, sizeof(mbuf), "%s  %.0f%%", moon.name, moon.illumination);
    uint8_t msize = 2;
    if (Layout_TextW(mbuf, 2) > 340) msize = 1;
    UI_TextCentered(mbuf, textY, C_LTGRAY, msize);

  } else if (!showMoon && hasSolar) {
    // Solar only
    int sunY = hasPills ? 392 : 378;
    int cntY = hasPills ? 412 : 400;
    UI_TextCentered(sunBuf, sunY, C_GRAY, 1);
    UI_TextCentered(cntBuf, cntY, evtCol, 1);

  } else if (!Settings_NightAuto()) {
    // Fallback night/day indicator
    const char* m = Settings_IsNight() ? "noc" : "den";
    if (Lang_Get() == LANG_EN) m = Settings_IsNight() ? "night" : "day";
    UI_TextCentered(m, LY_FOOTER, C_DKGRAY, 1);
  }
}

// =============================================================================
//  Luxury Astronomical Chronograph (Design Proposal Implementation)
// =============================================================================

static uint16_t* s_luxuryBg = nullptr;
static uint16_t* s_frameBg  = nullptr;
static float s_lastBgMoonPhase = -999.0f;
static int s_lastBgDay = -1;
static bool s_bgDirty = true;

static inline uint16_t pack565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline void unpack565(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = (uint8_t)((c >> 11) & 0x1F); r = (uint8_t)((r << 3) | (r >> 2));
  g = (uint8_t)((c >> 5) & 0x3F);  g = (uint8_t)((g << 2) | (g >> 4));
  b = (uint8_t)(c & 0x1F);         b = (uint8_t)((b << 3) | (b >> 2));
}

static inline uint16_t blendColor(uint16_t c1, uint16_t c2, float a) {
  if (a <= 0.0f) return c1;
  if (a >= 1.0f) return c2;
  uint8_t r1, g1, b1, r2, g2, b2;
  unpack565(c1, r1, g1, b1);
  unpack565(c2, r2, g2, b2);
  uint8_t r = (uint8_t)(r1 + (r2 - r1) * a);
  uint8_t g = (uint8_t)(g1 + (g2 - g1) * a);
  uint8_t b = (uint8_t)(b1 + (b2 - b1) * a);
  return pack565(r, g, b);
}

static inline uint16_t addGlow(uint16_t base, uint8_t gr, uint8_t gg, uint8_t gb, float intensity) {
  if (intensity <= 0.001f) return base;
  uint8_t r, g, b;
  unpack565(base, r, g, b);
  int nr = r + (int)(gr * intensity);
  int ng = g + (int)(gg * intensity);
  int nb = b + (int)(gb * intensity);
  return pack565((uint8_t)(nr > 255 ? 255 : nr),
                 (uint8_t)(ng > 255 ? 255 : ng),
                 (uint8_t)(nb > 255 ? 255 : nb));
}

static void drawCapsuleToBuf(uint16_t* buf, float x0, float y0, float x1, float y1, float radius, uint16_t col) {
  float l2 = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
  if (l2 < 0.001f) return;
  int minX = (int)floorf(fminf(x0, x1) - radius - 2.0f);
  int maxX = (int)ceilf(fmaxf(x0, x1) + radius + 2.0f);
  int minY = (int)floorf(fminf(y0, y1) - radius - 2.0f);
  int maxY = (int)ceilf(fmaxf(y0, y1) + radius + 2.0f);
  if (minX < 0) minX = 0; if (maxX >= LCD_WIDTH) maxX = LCD_WIDTH - 1;
  if (minY < 0) minY = 0; if (maxY >= LCD_HEIGHT) maxY = LCD_HEIGHT - 1;

  for (int py = minY; py <= maxY; py++) {
    for (int px = minX; px <= maxX; px++) {
      float t = ((px - x0) * (x1 - x0) + (py - y0) * (y1 - y0)) / l2;
      if (t < 0.0f) t = 0.0f;
      else if (t > 1.0f) t = 1.0f;
      float projX = x0 + t * (x1 - x0);
      float projY = y0 + t * (y1 - y0);
      float d = sqrtf((px - projX) * (px - projX) + (py - projY) * (py - projY));
      if (d <= radius + 1.0f) {
        float alpha = 1.0f - (d - (radius - 0.5f));
        if (alpha < 0.0f) alpha = 0.0f;
        else if (alpha > 1.0f) alpha = 1.0f;
        int idx = py * LCD_WIDTH + px;
        buf[idx] = blendColor(buf[idx], col, alpha);
      }
    }
  }
}

static void drawSunGlyphToBuf(uint16_t* buf, float sx, float sy, uint8_t type) {
  if (!buf) return;
  int isx = (int)roundf(sx);
  int isy = (int)roundf(sy);
  for (int py = isy - 6; py <= isy + 6; py++) {
    for (int px = isx - 6; px <= isx + 6; px++) {
      if (px >= 0 && px < LCD_WIDTH && py >= 0 && py < LCD_HEIGHT) {
        float d = sqrtf((px - sx) * (px - sx) + (py - sy) * (py - sy));
        float aHalo = 4.7f - d;
        if (aHalo <= 0.0f) continue;
        if (aHalo > 1.0f) aHalo = 1.0f;
        uint16_t& p = buf[py * LCD_WIDTH + px];
        p = blendColor(p, pack565(245, 175, 40), aHalo * 0.7f);
        float aCore = 3.5f - d;
        if (aCore > 0.0f) p = blendColor(p, pack565(255, 235, 160), aCore > 1.0f ? 1.0f : aCore);
      }
    }
  }

  if (type == 0) { // Active radiating sun: 8 golden rays
    for (int rAng = 0; rAng < 8; rAng++) {
      float ra = (float)rAng * (float)(M_PI / 4.0);
      drawCapsuleToBuf(buf, sx + 4.5f * cosf(ra), sy + 4.5f * sinf(ra),
                            sx + 8.0f * cosf(ra), sy + 8.0f * sinf(ra), 0.6f, pack565(255, 215, 110));
    }
  } else if (type == 1) { // Sunrise: upper rays + rising horizon bar
    for (int rAng = 0; rAng < 5; rAng++) {
      float ra = -(float)M_PI + (float)rAng * (float)(M_PI / 4.0);
      drawCapsuleToBuf(buf, sx + 4.5f * cosf(ra), sy + 4.5f * sinf(ra),
                            sx + 7.5f * cosf(ra), sy + 7.5f * sinf(ra), 0.6f, pack565(255, 220, 110));
    }
    drawCapsuleToBuf(buf, sx - 7.0f, sy + 5.0f, sx + 7.0f, sy + 5.0f, 0.7f, pack565(255, 195, 60));
  } else { // Sunset: setting sun dipping into horizon bar
    for (int rAng = 0; rAng < 5; rAng++) {
      float ra = -(float)M_PI + (float)rAng * (float)(M_PI / 4.0);
      drawCapsuleToBuf(buf, sx + 4.5f * cosf(ra), sy + 4.5f * sinf(ra),
                            sx + 7.5f * cosf(ra), sy + 7.5f * sinf(ra), 0.6f, pack565(255, 170, 50));
    }
    drawCapsuleToBuf(buf, sx - 7.0f, sy + 5.0f, sx + 7.0f, sy + 5.0f, 0.7f, pack565(245, 140, 40));
  }
}

static void renderLuxuryDialBackground(uint16_t* buf, float moonPhase, time_t now) {
  if (!buf) return;

  SolarTimes st = {0};
  int minSunrise = 6 * 60, minSunset = 20 * 60;
  int minDawn = 5 * 60 + 30, minDusk = 20 * 60 + 30;

  if (Settings_HasLocation()) {
    st = Astro_GetSolar(Settings_Lat(), Settings_Lon(), now);
    if (st.valid) {
      struct tm sr, ss, cd, cs;
      localtime_r(&st.sunrise, &sr);
      localtime_r(&st.sunset, &ss);
      localtime_r(&st.civilDawn, &cd);
      localtime_r(&st.civilDusk, &cs);
      minSunrise = sr.tm_hour * 60 + sr.tm_min;
      minSunset  = ss.tm_hour * 60 + ss.tm_min;
      minDawn    = cd.tm_hour * 60 + cd.tm_min;
      minDusk    = cs.tm_hour * 60 + cs.tm_min;
    }
  }

  int minGoldMorningS = minSunrise;
  int minGoldMorningE = minSunrise + 45;
  int minGoldS = minSunset - 45;
  int minGoldE = minSunset;

  const uint16_t COL_DAY   = pack565(140, 105, 70);   // Warm brushed bronze daytime
  const uint16_t COL_GOLD  = pack565(255, 195, 40);   // Radiant 24K luminous gold (golden hour)
  const uint16_t COL_TWIL  = pack565(32, 60, 115);    // Deep cobalt twilight
  const uint16_t COL_NIGHT = pack565(10, 16, 28);     // Deep midnight navy

  const float r_apt_m = 48.0f;
  const float r_moon  = 46.0f;
  const float cy_m    = 96.0f;

  const float r_apt_w = 56.0f;
  const float cy_w    = 356.0f;

  // Pass 1: Pixel shader for dial face, vignette, unlit ring, bevels, apertures
  auto shade = [&](float fx, float fy) -> uint16_t {
    float dy = fy - (float)CY;
    {
      float dx = fx - (float)CX;
      float dist = sqrtf(dx * dx + dy * dy);


      if (dist > 239.0f) {
        return 0x0000;
      }

      uint16_t baseColor;

      // Base dial tone with radial vignette
      if (dist <= 216.0f) {
        float vign = dist / 216.0f;
        float vFactor = 1.0f - vign * vign * 0.65f;
        baseColor = pack565((uint8_t)(22.0f * vFactor), (uint8_t)(26.0f * vFactor), (uint8_t)(33.0f * vFactor));
      } else if (dist <= 220.0f) {
        baseColor = pack565(12, 14, 18);
      } else if (dist <= 228.0f) {
        // Solar Twilight Track: Top (deg=0) is 12:00 NOON (720 min)! Bottom (deg=180) is 00:00 MIDNIGHT!
        float ang = atan2f(dx, -dy);
        float deg = ang * 57.2957795f;
        if (deg < 0.0f) deg += 360.0f;
        int m24 = (int)(((deg / 360.0f) * 1440.0f) + 720.0f) % 1440;

        // Morning twilight & golden hour
        if (m24 >= minDawn && m24 < minSunrise) {
          float f = (float)(m24 - minDawn) / (float)(minSunrise - minDawn);
          baseColor = blendColor(COL_TWIL, COL_GOLD, f);
        } else if (m24 >= minGoldMorningS && m24 < minGoldMorningE) {
          float f = (float)(m24 - minGoldMorningS) / (float)(minGoldMorningE - minGoldMorningS);
          baseColor = blendColor(COL_GOLD, COL_DAY, f);
        } else if (m24 >= minGoldMorningE && m24 < minGoldS) {
          baseColor = COL_DAY;
        } else if (m24 >= minGoldS && m24 < minSunset) {
          float f = (float)(m24 - minGoldS) / (float)(minSunset - minGoldS);
          baseColor = blendColor(COL_DAY, COL_GOLD, f);
        } else if (m24 >= minSunset && m24 <= minDusk) {
          float f = (float)(m24 - minSunset) / (float)(minDusk - minSunset);
          baseColor = blendColor(COL_GOLD, COL_TWIL, f);
        } else {
          // Night
          const int TRANS = 25;
          if (m24 > minDusk && m24 < minDusk + TRANS) {
            float f = (float)(m24 - minDusk) / (float)TRANS;
            baseColor = blendColor(COL_TWIL, COL_NIGHT, f);
          } else if (m24 < minDawn && m24 > minDawn - TRANS) {
            float f = (float)(minDawn - m24) / (float)TRANS;
            baseColor = blendColor(COL_TWIL, COL_NIGHT, f);
          } else {
            baseColor = COL_NIGHT;
          }
        }
      } else if (dist <= 235.0f) {
        float bevel = (dist - 228.0f) / 7.0f;
        baseColor = blendColor(pack565(16, 20, 26), pack565(36, 44, 56), bevel);
      } else { // 235..239
        float normY = -dy / dist;
        float glint = normY * 0.4f + 0.6f;
        if (glint < 0.2f) glint = 0.2f;
        baseColor = blendColor(pack565(25, 30, 40), pack565(85, 105, 130), glint);
      }

      // Concentric Seconds Ring Track (R=186):
      // In base background, render the muted unlit dark slate track
      float deltaRing = fabsf(dist - 186.0f);
      if (deltaRing <= 1.5f) {
        baseColor = blendColor(baseColor, pack565(16, 30, 50), 0.75f);
      } else if (deltaRing <= 4.0f) {
        baseColor = blendColor(baseColor, pack565(12, 20, 35), 0.35f);
      }

      // Top Moon Aperture (cx=240, cy=96, r=48)
      float dx_m = fx - 240.0f;
      float dy_m = fy - cy_m;
      float dm = sqrtf(dx_m * dx_m + dy_m * dy_m);

      if (dm >= r_apt_m && dm <= r_apt_m + 4.0f) {
        float ang_m = atan2f(dy_m, dx_m);
        float glint_m = (-sinf(ang_m) * 0.5f - cosf(ang_m) * 0.35f) + 0.5f;
        if (glint_m < 0.0f) glint_m = 0.0f;
        if (dm >= r_apt_m + 3.0f) {
          baseColor = pack565(10, 13, 17);
        } else {
          baseColor = blendColor(pack565(25, 32, 42), pack565(85, 105, 135), glint_m);
        }
      } else if (dm < r_apt_m) {
        if (dm <= r_moon) {
          float nx = dx_m / r_moon;
          float ny = dy_m / r_moon;
          float nz2 = 1.0f - nx * nx - ny * ny;
          float nz = sqrtf(nz2 > 0.0f ? nz2 : 0.0f);

          // Photographic NASA Lunar Albedo Map lookup (92x92)
          int um = (int)floorf((nx + 1.0f) * 0.5f * 91.0f);
          int vm = (int)floorf((ny + 1.0f) * 0.5f * 91.0f);
          if (um < 0) um = 0; if (um > 91) um = 91;
          if (vm < 0) vm = 0; if (vm > 91) vm = 91;
          float albedo = (float)pgm_read_byte(&s_moonAlbedo92[vm * 92 + um]) / 255.0f;

          float phaseRad = moonPhase * 2.0f * (float)M_PI;
          float lx = -sinf(phaseRad);
          float lz = -cosf(phaseRad);
          float dot = nx * lx + nz * lz;
          float limb = powf(nz, 0.35f);

          float lit = 1.0f / (1.0f + expf(-dot * 26.0f));
          float diffuse = (dot > 0.0f) ? (dot * 0.88f + 0.12f) : 0.0f;
          float intensity = lit * diffuse * limb * albedo;

          uint16_t moonCol;
          if (intensity > 0.02f) {
            uint8_t mr = (uint8_t)fminf(255.0f, 238.0f * intensity * 1.35f);
            uint8_t mg = (uint8_t)fminf(255.0f, 242.0f * intensity * 1.35f);
            uint8_t mb = (uint8_t)fminf(255.0f, 250.0f * intensity * 1.35f);
            moonCol = pack565(mr, mg, mb);
          } else {
            float earth = 0.07f * limb * albedo;
            moonCol = pack565((uint8_t)(145.0f * earth), (uint8_t)(175.0f * earth), (uint8_t)(225.0f * earth));
          }

          float topDist = fy - (cy_m - r_apt_m);
          float aoShadow = (topDist < 14.0f) ? (1.0f - (topDist / 14.0f)) * 0.65f : 0.0f;
          float rimDist = r_moon - dm;
          if (rimDist < 3.0f) {
            float rimS = (1.0f - (rimDist / 3.0f)) * 0.75f;
            if (rimS > aoShadow) aoShadow = rimS;
          }
          baseColor = blendColor(moonCol, pack565(0, 0, 0), aoShadow);
        } else {
          baseColor = pack565(4, 5, 7);
        }
      }

      // Bottom Weather Aperture (cx=240, cy=356, r=56)
      float dx_w = fx - 240.0f;
      float dy_w = fy - cy_w;
      float dw = sqrtf(dx_w * dx_w + dy_w * dy_w);

      if (dw >= r_apt_w && dw <= r_apt_w + 4.0f) {
        float ang_w = atan2f(dy_w, dx_w);
        float glint_w = (-sinf(ang_w) * 0.5f - cosf(ang_w) * 0.35f) + 0.5f;
        if (glint_w < 0.0f) glint_w = 0.0f;
        if (dw >= r_apt_w + 3.0f) {
          baseColor = pack565(10, 13, 17);
        } else {
          baseColor = blendColor(pack565(25, 32, 42), pack565(80, 100, 130), glint_w);
        }
      } else if (dw < r_apt_w) {
        uint16_t subdialFloor = pack565(10, 12, 16);
        float topDistW = fy - (cy_w - r_apt_w);
        float ao_w = (topDistW < 18.0f) ? (1.0f - (topDistW / 18.0f)) : 0.0f;
        float rimDistW = r_apt_w - dw;
        if (rimDistW < 5.0f) {
          float rimS = 1.0f - (rimDistW / 5.0f);
          if (rimS > ao_w) ao_w = rimS;
        }
        baseColor = blendColor(subdialFloor, pack565(0, 0, 0), ao_w * 0.85f);
      }

      return baseColor;
    }
  };

  // Edge radii (centre-relative for the dial, aperture-relative for the subdials)
  static const float kDialEdges[] = {216.0f, 220.0f, 228.0f, 235.0f, 239.0f};
  static const float kMoonEdges[] = {46.0f, 48.0f, 51.0f, 52.0f};
  static const float kWxEdges[]   = {56.0f, 59.0f, 60.0f};
  auto nearEdge = [](float d, const float* e, int n) {
    for (int i = 0; i < n; i++) if (fabsf(d - e[i]) < 1.5f) return true;
    return false;
  };
  for (int y = 0; y < LCD_HEIGHT; y++) {
    for (int x = 0; x < LCD_WIDTH; x++) {
      const float fx = (float)x, fy = (float)y;
      const float d0 = sqrtf((fx - CX) * (fx - CX) + (fy - CY) * (fy - CY));
      const float dm0 = sqrtf((fx - 240.0f) * (fx - 240.0f) + (fy - cy_m) * (fy - cy_m));
      const float dw0 = sqrtf((fx - 240.0f) * (fx - 240.0f) + (fy - cy_w) * (fy - cy_w));
      uint16_t col;
      if (nearEdge(d0, kDialEdges, 5) || nearEdge(dm0, kMoonEdges, 4) || nearEdge(dw0, kWxEdges, 3)) {
        int sr = 0, sg = 0, sb = 0;
        for (int sy = 0; sy < 4; sy++) for (int sx = 0; sx < 4; sx++) {
          uint8_t r, g, b;
          unpack565(shade(fx - 0.375f + sx * 0.25f, fy - 0.375f + sy * 0.25f), r, g, b);
          sr += r; sg += g; sb += b;
        }
        col = pack565((uint8_t)(sr >> 4), (uint8_t)(sg >> 4), (uint8_t)(sb >> 4));
      } else {
        col = shade(fx, fy);
      }
      buf[y * LCD_WIDTH + x] = col;
    }
  }

  // Pass 2: Draw anti-aliased capsules for batons, ticks, and sun symbols
  const uint16_t colGold = pack565(235, 185, 135);
  const uint16_t colCyan = pack565(60, 205, 255);
  const uint16_t colSlate = pack565(65, 78, 95);

  for (int m = 0; m < 60; m++) {
    float a = (float)m * 6.0f * (float)(M_PI / 180.0);
    float sinA = sinf(a);
    float cosA = cosf(a);

    if (m % 5 == 0) {
      if (m == 0) { // 12 o'clock double baton
        float offset = 0.022f;
        drawCapsuleToBuf(buf, CX + 200.0f * sinf(a - offset), CY - 200.0f * cosf(a - offset),
                              CX + 214.0f * sinf(a - offset), CY - 214.0f * cosf(a - offset), 1.5f, colGold);
        drawCapsuleToBuf(buf, CX + 200.0f * sinf(a + offset), CY - 200.0f * cosf(a + offset),
                              CX + 214.0f * sinf(a + offset), CY - 214.0f * cosf(a + offset), 1.5f, colGold);
        drawCapsuleToBuf(buf, CX + 217.0f * sinA, CY - 217.0f * cosA, CX + 227.0f * sinA, CY - 227.0f * cosA, 1.0f, colCyan);
      } else if (m == 15 || m == 30 || m == 45) {
        drawCapsuleToBuf(buf, CX + 196.0f * sinA, CY - 196.0f * cosA, CX + 214.0f * sinA, CY - 214.0f * cosA, 1.8f, colGold);
        drawCapsuleToBuf(buf, CX + 217.0f * sinA, CY - 217.0f * cosA, CX + 227.0f * sinA, CY - 227.0f * cosA, 1.0f, colCyan);
      } else {
        drawCapsuleToBuf(buf, CX + 202.0f * sinA, CY - 202.0f * cosA, CX + 214.0f * sinA, CY - 214.0f * cosA, 1.5f, colGold);
      }
    } else {
      drawCapsuleToBuf(buf, CX + 209.0f * sinA, CY - 209.0f * cosA, CX + 214.0f * sinA, CY - 214.0f * cosA, 0.6f, colSlate);
    }
  }

  // 12 ticks in bottom subdial (r = 47 to 52)
  for (int i = 0; i < 12; i++) {
    float th = (float)i * (float)(M_PI / 6.0);
    float sx = sinf(th);
    float cxVal = cosf(th);
    drawCapsuleToBuf(buf, 240.0f + 47.0f * sx, cy_w - 47.0f * cxVal,
                          240.0f + 52.0f * sx, cy_w - 52.0f * cxVal, 0.7f, colGold);
  }

  // Noon index indicator on 24h solar track (12 o'clock / 0 deg)
  drawCapsuleToBuf(buf, (float)CX, (float)CY - 220.0f, (float)CX, (float)CY - 227.0f, 0.9f, pack565(255, 215, 110));

  // Sunrise and Sunset glyphs positioned on the 24h solar arc
  float degSunrise = (float)(((minSunrise - 720 + 1440) % 1440) * 360) / 1440.0f;
  float degSunset  = (float)(((minSunset  - 720 + 1440) % 1440) * 360) / 1440.0f;
  float radRise = degSunrise * (float)(M_PI / 180.0);
  float radSet  = degSunset  * (float)(M_PI / 180.0);

  drawSunGlyphToBuf(buf, (float)CX + 224.0f * sinf(radRise), (float)CY - 224.0f * cosf(radRise), 1);
  drawSunGlyphToBuf(buf, (float)CX + 224.0f * sinf(radSet),  (float)CY - 224.0f * cosf(radSet),  2);
}

static void drawLuxuryWeatherDynamic() {
  // Temperature text at top of bottom sub-dial
  char tbuf[16];
  if (Forecast_CurrentValid()) {
    snprintf(tbuf, sizeof(tbuf), "%.1f\xC2\xB0""C", Forecast_CurrentTemp());
  } else {
    snprintf(tbuf, sizeof(tbuf), "--\xC2\xB0""C");
  }
  Font_DrawCentered(tbuf, CX, 326, C_WHITE, FONT_TITLE);

  // Sparkline trend curve
  const int spW = 70;
  const int spH = 22;
  const int spX = CX - spW / 2;
  const int spY = 356 + 2;

  int hCount = Forecast_HourCount();
  const FcHour* hours = Forecast_Hours();
  if (hCount >= 6 && hours) {
    const int pts = (hCount < 12) ? hCount : 12;
    float minT = hours[0].temp, maxT = hours[0].temp;
    for (int i = 1; i < pts; i++) {
      if (hours[i].temp < minT) minT = hours[i].temp;
      if (hours[i].temp > maxT) maxT = hours[i].temp;
    }
    float rng = maxT - minT;
    if (rng < 1.0f) rng = 1.0f;

    struct Pt { int x, y; };
    Pt coords[16];
    for (int i = 0; i < pts; i++) {
      coords[i].x = spX + (i * (spW - 1)) / (pts - 1);
      coords[i].y = spY + spH - 1 - (int)(((hours[i].temp - minT) / rng) * (spH - 6) + 3);
    }

    int prevX = -1, prevY = -1;
    for (int i = 0; i < pts - 1; i++) {
      const Pt& p0 = coords[i > 0 ? i - 1 : 0];
      const Pt& p1 = coords[i];
      const Pt& p2 = coords[i + 1];
      const Pt& p3 = coords[i + 2 < pts ? i + 2 : pts - 1];

      const int steps = 8;
      for (int s = 0; s < steps; s++) {
        float t = (float)s / (float)steps;
        float t2 = t * t;
        float t3 = t2 * t;
        float cx_pt = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
        float cy_pt = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
        int curX = (int)roundf(cx_pt);
        int curY = (int)roundf(cy_pt);

        // Vertical gradient fill under curve
        for (int fy = curY + 1; fy <= spY + spH + 2; fy++) {
          float frac = (float)(fy - curY) / (float)(spH + 4);
          if (frac > 1.0f) frac = 1.0f;
          uint16_t c = blendColor(pack565(20, 65, 110), pack565(10, 12, 16), frac);
          gfx->drawPixel(curX, fy, c);
        }

        if (prevX >= 0) {
          gfx->drawLine(prevX, prevY, curX, curY, RGB565(56, 189, 248));
          gfx->drawLine(prevX, prevY + 1, curX, curY + 1, RGB565(25, 110, 170));
        }
        prevX = curX;
        prevY = curY;
      }
    }
    if (prevX >= 0) {
      gfx->drawLine(prevX, prevY, coords[pts - 1].x, coords[pts - 1].y, RGB565(56, 189, 248));
      int lx = coords[pts - 1].x, ly = coords[pts - 1].y;
      gfx->fillCircle(lx, ly, 4, RGB565(18, 80, 150));
      gfx->fillCircle(lx, ly, 2, RGB565(56, 189, 248));
      gfx->fillCircle(lx, ly, 1, C_WHITE);
    }
  } else {
    gfx->drawFastHLine(CX - 24, 356 + 12, 48, RGB565(25, 45, 65));
  }
}

static void ScreenClock_Draw() {
  if (!s_luxuryBg) {
    s_luxuryBg = (uint16_t*)heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_bgDirty = true;
  }
  if (!s_frameBg) {
    s_frameBg = (uint16_t*)heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }

  Layout_Begin();

  if (!Outside_TimeValid()) {
    gfx->fillScreen(C_BLACK);
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

  MoonInfo moon = Astro_GetMoon(now);

  // Background Cache Regeneration Check
  if (s_bgDirty || s_lastBgDay != lt.tm_yday || fabsf(moon.phase - s_lastBgMoonPhase) >= 0.01f) {
    if (s_luxuryBg) {
      renderLuxuryDialBackground(s_luxuryBg, moon.phase, now);
      s_lastBgMoonPhase = moon.phase;
      s_lastBgDay = lt.tm_yday;
      s_bgDirty = false;
    }
  }

  uint16_t* displayBuf = s_luxuryBg;
  if (s_frameBg && s_luxuryBg) {
    memcpy(s_frameBg, s_luxuryBg, LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
    displayBuf = s_frameBg;
  }

  // Render dynamic active glowing seconds arc from 12 o'clock clockwise to current second
  if (displayBuf) {
    static const float s_radialGlow[33] = {
      1.400f, 1.348f, 1.211f, 1.026f, 0.832f, 0.655f, 0.510f, 0.398f,
      0.315f, 0.252f, 0.205f, 0.168f, 0.138f, 0.114f, 0.095f, 0.079f,
      0.065f, 0.054f, 0.045f, 0.038f, 0.031f, 0.026f, 0.022f, 0.018f,
      0.015f, 0.013f, 0.010f, 0.009f, 0.007f, 0.006f, 0.005f, 0.004f, 0.000f
    };

    float targetSecDeg = (float)lt.tm_sec * 6.0f;
    for (int y = CY - 202; y <= CY + 202; y++) {
      float dy = (float)y - (float)CY;
      float dy2 = dy * dy;
      float xOuter2 = 202.0f * 202.0f - dy2;
      if (xOuter2 < 0.0f) continue;
      float xOuter = sqrtf(xOuter2);

      float xInner = 0.0f;
      float xInner2 = 170.0f * 170.0f - dy2;
      if (xInner2 > 0.0f) xInner = sqrtf(xInner2);

      int spans[2][2] = {
        { (int)floorf((float)CX - xOuter), (int)ceilf((float)CX - xInner) },
        { (int)floorf((float)CX + xInner), (int)ceilf((float)CX + xOuter) }
      };
      if (xInner <= 0.0f) {
        spans[0][1] = spans[1][1];
        spans[1][0] = 1; spans[1][1] = 0;
      }

      for (int sp = 0; sp < 2; sp++) {
        int xStart = spans[sp][0];
        int xEnd   = spans[sp][1];
        if (xStart > xEnd) continue;
        if (xStart < 0) xStart = 0;
        if (xEnd >= LCD_WIDTH) xEnd = LCD_WIDTH - 1;

        for (int x = xStart; x <= xEnd; x++) {
          float dx = (float)x - (float)CX;
          float dist = sqrtf(dx * dx + dy2);
          if (dist < 170.0f || dist > 202.0f) continue;

          // Occlude behind raised moon aperture (cy=96, r=52) and weather complication (cy=356, r=60)
          float dy_m = (float)y - 96.0f;
          if (dx * dx + dy_m * dy_m <= 52.0f * 52.0f) continue;
          float dy_w = (float)y - 356.0f;
          if (dx * dx + dy_w * dy_w <= 60.0f * 60.0f) continue;

          float angSec = atan2f(dx, -dy) * 57.2957795f;
          if (angSec < 0.0f) angSec += 360.0f;

          if (angSec <= targetSecDeg) {
            float deltaRing = fabsf(dist - 186.0f);
            float edgeA = 1.0f;
            if (angSec > targetSecDeg - 1.5f) {
              edgeA = (targetSecDeg - angSec) / 1.5f;
              if (edgeA < 0.0f) edgeA = 0.0f;
            }

            int gIdx = (int)(deltaRing * 2.0f);
            float glow = (gIdx < 32 ? s_radialGlow[gIdx] : 0.0f) * edgeA;

            int idx = y * LCD_WIDTH + x;
            uint16_t c = displayBuf[idx];
            if (deltaRing <= 1.0f) {
              float coreA = (1.0f - deltaRing) * edgeA;
              c = blendColor(c, pack565(80, 215, 255), coreA * 0.95f);
            }
            displayBuf[idx] = addGlow(c, 40, 165, 255, glow);
          }
        }
      }
    }
  }

  // Live Sun glyph orbiting along 24h perimeter solar ring according to current local time
  if (displayBuf) {
    float minNowF = (float)lt.tm_hour * 60.0f + (float)lt.tm_min + (float)lt.tm_sec / 60.0f;
    float degNow = (float)(fmodf(minNowF - 720.0f + 1440.0f, 1440.0f) * 360.0f) / 1440.0f;
    float radNow = degNow * (float)(M_PI / 180.0);
    float sunX = (float)CX + 224.0f * sinf(radNow);
    float sunY = (float)CY - 224.0f * cosf(radNow);
    drawSunGlyphToBuf(displayBuf, sunX, sunY, 0);
  }

  // 1. Blit the pre-rendered luxury chronograph background (bloom arc, vignette, chamfers, shadows, moon, batons)
  if (displayBuf) {
    gfx->draw16bitRGBBitmap(0, 0, displayBuf, LCD_WIDTH, LCD_HEIGHT);
  } else {
    gfx->fillScreen(C_BLACK);
  }

  // 2. Moon Illumination percentage under top complication
  char mbuf[32];
  snprintf(mbuf, sizeof(mbuf), "%.0f%%", moon.illumination);
  Font_DrawCentered(mbuf, CX, 146, RGB565(145, 155, 170), FONT_TINY);

  // Date complication placed between Moon phase percentage and Time
  if (Settings_ClockShowDate()) {
    char dateBuf[48];
    if (Lang_Get() == LANG_EN) {
      snprintf(dateBuf, sizeof(dateBuf), "%s, %d %s",
               Lang_WeekdayShort(lt.tm_wday), lt.tm_mday, Lang_MonthName(lt.tm_mon));
    } else {
      snprintf(dateBuf, sizeof(dateBuf), "%s, %d. %s",
               Lang_WeekdayShort(lt.tm_wday), lt.tm_mday, Lang_MonthName(lt.tm_mon));
    }
    Font_DrawCentered(dateBuf, CX, 168, RGB565(195, 205, 220), FONT_TINY);
  }

  // 3. Center Swiss Digital Clock
  char hhmm[8];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt.tm_hour, lt.tm_min);
  Font_DrawCenteredBox(hhmm, 0, 192, LCD_WIDTH, 80, C_WHITE, FONT_CLOCK);

  // 4. Bottom Complication (Temperature & Sparkline)
  drawLuxuryWeatherDynamic();

  // 5. Orbiting Electric-Blue Seconds Bead at leading edge of active arc (occluded behind moon bezel)
  float sRad = (lt.tm_sec * 6.0f - 90.0f) * 0.0174532925f;
  int bx = CX + (int)roundf(186.0f * cosf(sRad));
  int by = CY + (int)roundf(186.0f * sinf(sRad));
  float dmb = sqrtf((float)((bx - 240)*(bx - 240) + (by - 96)*(by - 96)));
  if (dmb > 52.0f) {
    gfx->fillCircle(bx, by, 8, RGB565(10, 45, 90));
    gfx->fillCircle(bx, by, 5, RGB565(24, 110, 190));
    gfx->fillCircle(bx, by, 3, RGB565(56, 189, 248));
    gfx->fillCircle(bx, by, 1, C_WHITE);
  }

  // 6. Draw Overhead Flight Banner Overlay if detected
  if (s_overheadActive && overheadAc) {
    drawOverheadWidget(overheadAc, overheadDist);
  }

  // 7. Draw Approaching Precipitation Banner Overlay if detected
  if (s_precipActive && precipAlert) {
    drawPrecipWidget(precipAlert);
  }
}

static void ScreenClock_TimerCb(lv_timer_t* t) {
  if (UI_GetActiveScreen() != SCREEN_CLOCK_I) return;
  static unsigned long s_lastTick = 0;
  unsigned long now = millis();
  if (now - s_lastTick >= 1000) {
    s_lastTick = now;
    lv_obj_invalidate(s_screenObj);
  }
}

static void ScreenClock_EventCb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_DRAW_MAIN) {
    lv_layer_t* layer = lv_event_get_layer(e);
    gfx->setLayer(layer);
    ScreenClock_Draw();
    gfx->setLayer(nullptr);
  } else if (code == LV_EVENT_CLICKED) {
    if (UI_IsSwipeActive()) return;
    lv_indev_t* indev = lv_indev_active();
    if (indev) {
      lv_point_t pt;
      lv_indev_get_point(indev, &pt);
      ScreenClock_HandleTap(pt.x, pt.y);
    }
  }
}

void ScreenClock_Init(lv_obj_t* parent) {
  s_screenObj = parent;
  lv_obj_set_size(s_screenObj, 480, 480);
  lv_obj_center(s_screenObj);
  lv_obj_set_scrollable(s_screenObj, false);
  lv_obj_set_clickable(s_screenObj, true);

  lv_obj_add_event_cb(s_screenObj, ScreenClock_EventCb, LV_EVENT_ALL, nullptr);

  if (!s_luxuryBg) {
    s_luxuryBg = (uint16_t*)heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (!s_frameBg) {
    s_frameBg = (uint16_t*)heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  s_bgDirty = true;

  s_timer = lv_timer_create(ScreenClock_TimerCb, 500, nullptr);
}

void ScreenClock_FreeBuffers() {
  if (s_luxuryBg) {
    heap_caps_free(s_luxuryBg);
    s_luxuryBg = nullptr;
  }
  if (s_frameBg) {
    heap_caps_free(s_frameBg);
    s_frameBg = nullptr;
  }
  s_bgDirty = true;
}

