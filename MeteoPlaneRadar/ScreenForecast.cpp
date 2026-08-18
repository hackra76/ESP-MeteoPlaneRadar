// =============================================================================
//  MeteoPlaneRadar
//  Screen: weather forecast (Open-Meteo).
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenForecast.h"
#include "Forecast.h"
#include "Settings.h"
#include "WxIcon.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Config.h"

#include <WiFi.h>
#include <time.h>
#include <math.h>

#define CX (LCD_WIDTH / 2)

// Vertical stack. The hourly block is denser than the daily one - six rows have
// to fit where the circle is still narrowing, so they get less height each.
//
// Every number is size 2 with its unit in size 1 beside it: full-size units
// would force the numbers down to an unreadable size, and bare numbers at the
// end of a row mean nothing to the reader.
#define HOUR_Y0    58
#define HOUR_H     27
#define SEP_Y      (HOUR_Y0 + FORECAST_HOURS * HOUR_H + 6)
#define DAY_Y0     (SEP_Y + 8)
#define DAY_H      37
#define AQ_Y0      (DAY_Y0 + FORECAST_DAYS * DAY_H + 10)
#define AQ_H       24     // line spacing in the air-quality block

// Columns, measured from the centre. The narrowest row is the top one, where
// the chord is about 320 px, so everything has to live inside +/-160.
//
// The temperature-to-precipitation gap fits the worst case on a DAY row:
// "-12/-19" is 84 px. Any tighter and the precipitation would be dropped on
// exactly the cold days you want it.
#define COL_LABEL  (CX - 150)
#define COL_ICON   (CX - 104)
#define COL_TEMP   (CX -  80)
#define COL_PRECIP (CX +  16)
#define COL_WIND   (CX +  92)

static unsigned long s_lastSeen = 0;

void ScreenForecast_Enter() {
  // Nudge the fetcher: entering the screen is exactly when a stale forecast is
  // most annoying. Forecast_Tick() decides whether anything actually happens.
  s_lastSeen = 0;
}

bool ScreenForecast_Tick() {
  // The data itself is fetched centrally (Forecast_Tick in the main loop), so
  // all this has to do is notice when it changed and ask for a redraw.
  static bool lastValid = false;
  static int  lastHour  = -1;
  bool valid = Forecast_Valid();
  time_t now = time(nullptr);
  struct tm lt; localtime_r(&now, &lt);
  if (valid != lastValid || lt.tm_hour != lastHour) {
    lastValid = valid;
    lastHour = lt.tm_hour;
    return true;
  }
  return false;
}

// One text field, claimed before it is drawn so it can never sit on top of a
// neighbour that turned out wider than expected.
static void field(const char* s, int x, int y, uint8_t size, uint16_t col) {
  if (!s || !*s) return;
  int w = Layout_TextW(s, size);
  if (!Layout_Claim(x - 2, y - 2, w + 4, LY_CHAR_H(size) + 4)) return;
  gfx->setTextSize(size);
  gfx->setTextColor(col);
  gfx->setCursor(x, y);
  gfx->print(s);
}

// A number with its unit beside it. The pair claims its space together, so a
// unit can never be drawn without the number it belongs to.
static void valueWithUnit(const char* val, const char* unit,
                          int x, int y, uint16_t col) {
  if (!val || !*val) return;
  const int vw  = Layout_TextW(val, 2);
  const int uw  = (unit && *unit) ? Layout_TextW(unit, 1) : 0;
  const int gap = uw ? 3 : 0;
  const int tot = vw + gap + uw;

  if (!Layout_Claim(x - 2, y - 2, tot + 4, LY_CHAR_H(2) + 4)) return;

  gfx->setTextSize(2);
  gfx->setTextColor(col);
  gfx->setCursor(x, y);
  gfx->print(val);

  if (uw) {
    gfx->setTextSize(1);
    gfx->setTextColor(C_GRAY);
    gfx->setCursor(x + vw + gap, y + 8);   // sits on the baseline of the digits
    gfx->print(unit);
  }
}

static void drawHourRow(const FcHour& h, int y) {
  char buf[16];
  struct tm lt; localtime_r(&h.t, &lt);

  snprintf(buf, sizeof(buf), "%02d", lt.tm_hour);
  field(buf, COL_LABEL, y + 5, 2, C_GRAY);

  if (Layout_Claim(COL_ICON - 11, y + 1, 22, 22))
    WxIcon_Draw(COL_ICON, y + 12, 11, h.code, false);

  snprintf(buf, sizeof(buf), "%d", (int)lroundf(h.temp));
  valueWithUnit(buf, OUTSIDE_DEG_TEXT, COL_TEMP, y + 5, WxIcon_Color(h.code));

  // Precipitation is only interesting when there is some - a column of "0.0"
  // is noise, and the space is better spent on nothing at all.
  if (h.precip >= 0.05f) {
    snprintf(buf, sizeof(buf), "%.1f", h.precip);
    valueWithUnit(buf, "mm", COL_PRECIP, y + 5, C_CYAN);
  }

  snprintf(buf, sizeof(buf), "%d", (int)lroundf(h.wind));
  valueWithUnit(buf, "km/h", COL_WIND, y + 5, C_GRAY);
}

static void drawDayRow(const FcDay& d, int y) {
  char buf[24];
  struct tm lt; localtime_r(&d.t, &lt);

  field(Lang_WeekdayShort(lt.tm_wday), COL_LABEL, y + 9, 2, C_WHITE);

  if (Layout_Claim(COL_ICON - 14, y + 2, 28, 28))
    WxIcon_Draw(COL_ICON, y + 16, 14, d.code, false);

  // Maximum and minimum share the temperature column. No unit here: "-12/-19"
  // plus a suffix would run into the precipitation column, and the hourly rows
  // directly above already label this column.
  snprintf(buf, sizeof(buf), "%d/%d", (int)lroundf(d.tmax), (int)lroundf(d.tmin));
  field(buf, COL_TEMP, y + 9, 2, WxIcon_Color(d.code));

  if (d.precip >= 0.05f) {
    snprintf(buf, sizeof(buf), "%.1f", d.precip);
    valueWithUnit(buf, "mm", COL_PRECIP, y + 9, C_CYAN);
  }

  snprintf(buf, sizeof(buf), "%d", (int)lroundf(d.wind));
  valueWithUnit(buf, "km/h", COL_WIND, y + 9, C_GRAY);
}

// European AQI bands, coloured the way the index itself is published.
static uint16_t aqiColor(int aqi) {
  if (aqi < 0)  return C_GRAY;
  if (aqi <= 20) return C_GREEN;
  if (aqi <= 40) return 0x87E0;    // yellow-green
  if (aqi <= 60) return C_YELLOW;
  if (aqi <= 80) return C_ORANGE;
  return C_RED;
}

// Particulates, on the WHO daily guidance rather than the AQI bands - the two
// do not line up, and PM2.5 is the number people actually recognise.
static uint16_t pmColor(float pm25) {
  if (pm25 <= 15) return C_GREEN;
  if (pm25 <= 25) return C_YELLOW;
  if (pm25 <= 50) return C_ORANGE;
  return C_RED;
}

// Pollen counts are grains per cubic metre. The thresholds differ per species,
// so this is a deliberately coarse "is it worth knowing about" scale.
static uint16_t pollenColor(float p) {
  if (p < 10)  return C_GRAY;
  if (p < 50)  return C_GREEN;
  if (p < 100) return C_YELLOW;
  return C_ORANGE;
}

// One line of the air-quality block: grey label, coloured value, centred as a
// pair.
static void aqRow(int y, const char* label, const char* value, uint16_t valCol) {
  if (!label || !value) return;
  const int lw = Layout_TextW(label, 2);
  const int vw = Layout_TextW(value, 2);
  const int gap = 12;
  const int total = lw + gap + vw;
  const int x0 = CX - total / 2;

  if (!Layout_Claim(x0 - 4, y - 3, total + 8, LY_CHAR_H(2) + 6)) return;

  gfx->setTextSize(2);
  gfx->setTextColor(C_GRAY);
  gfx->setCursor(x0, y);
  gfx->print(label);
  gfx->setTextColor(valCol);
  gfx->setCursor(x0 + lw + gap, y);
  gfx->print(value);
}

void ScreenForecast_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();

  // Chrome first: the screen dots and the status line own their bands, so a
  // long row below can never creep into them.
  Layout_ReserveBand(LY_DOTS - 6, 12);
  Layout_ReserveBand(LY_STATUS - 3, 22);
  UI_DrawStatusLine(LY_STATUS);

  if (!Settings_HasLocation()) {
    UI_TextCentered(T(S_NO_LOCATION), LCD_HEIGHT / 2, C_YELLOW, 2);
    return;
  }
  if (!Forecast_Valid()) {
    UI_TextCentered(T(S_FORECAST), LCD_HEIGHT / 2 - 20, C_WHITE, 2);
    UI_TextCentered(WiFi.status() == WL_CONNECTED ? T(S_LOADING) : T(S_WIFI_WAIT),
                    LCD_HEIGHT / 2 + 10, C_YELLOW, 2);
    return;
  }

  // --- Hours ---
  const FcHour* hrs = Forecast_Hours();
  int hn = Forecast_HourCount();
  if (hn > FORECAST_HOURS) hn = FORECAST_HOURS;
  for (int i = 0; i < hn; i++) drawHourRow(hrs[i], HOUR_Y0 + i * HOUR_H);

  // --- Separator ---
  {
    int half = Layout_ChordHalf(SEP_Y) - 30;
    if (half > 20) {
      Layout_Reserve(CX - half, SEP_Y - 1, 2 * half, 3);
      gfx->drawFastHLine(CX - half, SEP_Y, 2 * half, C_DKGRAY);
    }
  }

  // --- Days ---
  const FcDay* dys = Forecast_Days();
  int dn = Forecast_DayCount();
  if (dn > FORECAST_DAYS) dn = FORECAST_DAYS;
  for (int i = 0; i < dn; i++) drawDayRow(dys[i], DAY_Y0 + i * DAY_H);

  // --- Air quality ---
  // Three stacked lines rather than one crowded row: each gets the full width
  // of the circle, so the label can be spelled out and the value drawn large.
  // Only the parts that are actually known appear - pollen is a European
  // product that returns nothing elsewhere, and "no data" must never be shown
  // as a reassuring zero.
  if (AirQuality_Valid()) {
    char val[24], lbl[24];
    int line = 0;

    int aqi = AirQuality_Aqi();
    if (aqi >= 0) {
      snprintf(val, sizeof(val), "%d", aqi);
      aqRow(AQ_Y0 + line * AQ_H, "AQI", val, aqiColor(aqi));
      line++;
    }

    float pm = AirQuality_Pm25();
    if (pm > 0) {
      // The unit goes with the value rather than in a header: this block has no
      // columns, and a bare "8" next to "AQI 42" reads as a second index rather
      // than a concentration. Spelled "ug/m3" - the built-in font is 7-bit
      // ASCII, so the real symbols would come out as random glyphs.
      snprintf(val, sizeof(val), "%.0f ug/m3", pm);
      aqRow(AQ_Y0 + line * AQ_H, "PM2.5", val, pmColor(pm));
      line++;
    }

    float pollen = AirQuality_PollenMax();
    if (pollen >= 0) {
      // The species that is worst right now, so the number means something -
      // "pollen 120" without knowing it is birch is not actionable.
      const char* worst = AirQuality_PollenWorst();
      if (worst && *worst) snprintf(lbl, sizeof(lbl), "%s %s", T(S_POLLEN), worst);
      else                 snprintf(lbl, sizeof(lbl), "%s", T(S_POLLEN));
      snprintf(val, sizeof(val), "%.0f", pollen);
      aqRow(AQ_Y0 + line * AQ_H, lbl, val, pollenColor(pollen));
    }
  }
}
