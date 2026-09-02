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
  // With the ring off there is nothing on this screen that changes faster than
  // once a minute, so do not spend a redraw per second on it.
  if (Settings_SecondsStyle() == SEC_STYLE_OFF) {
    if (lt.tm_min == s_lastMin) return false;
    s_lastMin = lt.tm_min;
    return true;
  }
  if (lt.tm_sec == s_lastSec) return false;
  s_lastSec = lt.tm_sec;
  s_lastMin = lt.tm_min;
  return true;
}

bool ScreenClock_HandleTap(int x, int y) {
  (void)x; (void)y;
  if (Settings_NightAuto()) return false;    // automatic mode owns the decision
  NightMode_Toggle();
  return true;
}

// Scale an RGB565 colour towards black. Used for the comet tail, where a fixed
// second colour would just be a bright block rather than a fading trail.
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
      // A continuous arc. Drawn as 60 short chords rather than with drawArc,
      // which the GFX build here does not have.
      int px = 0, py = 0;
      for (int i = 0; i <= 60; i++) {
        int x, y; secPos(i, R, &x, &y);
        if (i > 0) gfx->drawLine(px, py, x, y, (i <= sec) ? on : off);
        px = x; py = y;
      }
      // A little weight on the leading end so the arc has a visible head.
      int hx, hy; secPos(sec, R, &hx, &hy);
      gfx->fillCircle(hx, hy, 4, on);
      break;
    }

    case SEC_STYLE_COMET: {
      const int TAIL = 12;
      for (int i = 0; i < 60; i++) {          // faint track underneath
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
  }
}

void ScreenClock_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();

  // The screen dots at the top belong to the screen manager - on clock screen they sit at y=58
  Layout_ReserveBand(58 - 6, 12);

  if (!Outside_TimeValid()) {
    UI_TextCentered(T(S_WIFI_WAIT), CY - 8, C_YELLOW, 2);
    return;
  }

  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);

  // --- Seconds ring (outermost, drawn first) ---
  drawSecondsRing(lt.tm_sec);

  // --- 24h Solar Twilight Arc (day/golden hour/twilight/night ring) ---
  if (Settings_HasLocation()) {
    Astro_DrawSolarArc(CX, CY, 216, 3, Settings_Lat(), Settings_Lon(), now);
  }

  // --- Weekday and date (above the clock) ---
  char date[40];
  if (Lang_Get() == LANG_EN) {
    snprintf(date, sizeof(date), "%s %d %s",
             Lang_WeekdayShort(lt.tm_wday), lt.tm_mday, Lang_MonthName(lt.tm_mon));
  } else {
    snprintf(date, sizeof(date), "%s %d. %s",
             Lang_WeekdayShort(lt.tm_wday), lt.tm_mday, Lang_MonthName(lt.tm_mon));
  }
  // Long Czech month names ("listopadu") can outgrow the chord at size 2 - drop
  // to size 1 rather than letting it run off the rim.
  uint8_t dsize = 2;
  if (Layout_TextW(date, 2) > 2 * Layout_ChordHalf(DATE_Y + 16) - 16) dsize = 1;
  Layout_ReserveTextCentered(date, dsize, CX, DATE_Y);
  UI_TextCentered(date, DATE_Y, C_GRAY, dsize);

  // --- HH:MM ---
  // Size 8 is 48 px per character, so five characters come to 240 px - well
  // inside the chord at this height (about 470 px).
  char hhmm[8];
  snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt.tm_hour, lt.tm_min);
  Layout_ReserveTextCentered(hhmm, 8, CX, CLK_Y);
  UI_TextCentered(hhmm, CLK_Y, Settings_ClockColor(), 8);

  // --- Current conditions: icon, temperature and rain on one row ---
  // All of it comes from the forecast request, so the row simply stays empty
  // until the first one lands.
  if (Forecast_CurrentValid()) {
    char tbuf[16], pbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%d°C", (int)lroundf(Forecast_CurrentTemp()));

    // Precipitation shares the temperature's row
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
      // 1. Draw weather icon centered vertically at WX_Y
      WxIcon_Draw(x0 + WX_ICON_R, WX_Y, WX_ICON_R,
                  Forecast_CurrentCode(), Settings_IsNight());

      // 2. Draw temperature text vertically centered with the icon at WX_Y
      UI_Text(tbuf, x0 + iconW + gap, WX_Y - 7, C_WHITE, 3);

      // 3. Draw precipitation text vertically centered at WX_Y if raining
      if (hasRain) {
        UI_Text(pbuf, x0 + iconW + gap + tw + pgap, WX_Y - 5, C_CYAN, 2);
      }
    }

    // --- Wind, on its own line ---
    {
      char wbuf[16];
      snprintf(wbuf, sizeof(wbuf), "%d km/h", (int)lroundf(Forecast_CurrentWind()));
      const int ww = Layout_TextW(wbuf, 2);
      if (Layout_Claim(CX - ww / 2 - 6, WIND_Y - 3, ww + 12, LY_CHAR_H(2) + 6)) {
        UI_TextCentered(wbuf, WIND_Y, C_GRAY, 2);
      }
    }
  }

  // --- Moon Phase & Illumination Widget (2 lines: Icon above, Name + % below) ---
  {
    MoonInfo moon = Astro_GetMoon(now);
    const int mr = 16;
    Astro_DrawMoonIcon(CX, MOON_ICON_Y, mr, moon.phase);

    char mbuf[40];
    snprintf(mbuf, sizeof(mbuf), "%s  %.0f%%", moon.name, moon.illumination);
    uint8_t msize = 2;
    if (Layout_TextW(mbuf, 2) > 340) msize = 1;
    UI_TextCentered(mbuf, MOON_TEXT_Y, C_LTGRAY, msize);
  }

  // A quiet hint that a tap switches the look, but only when a tap actually
  // does something - with the automatic mode on it would be a lie.
  if (!Settings_NightAuto()) {
    const char* m = Settings_IsNight() ? "noc" : "den";
    if (Lang_Get() == LANG_EN) m = Settings_IsNight() ? "night" : "day";
    UI_TextCentered(m, LY_FOOTER, C_DKGRAY, 1);
  }
}
