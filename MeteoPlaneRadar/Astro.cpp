// =============================================================================
//  MeteoPlaneRadar
//  Astro.cpp - Astronomical engine: Moon phase, solar position & twilight arcs.
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#include "Astro.h"
#include "Settings.h"
#include "Config.h"
#include "Lang.h"
#include "UI.h"
#include <Arduino_GFX_Library.h>
#include <math.h>

extern Arduino_GFX* gfx;

#define DEG2RAD 0.017453292519943295f
#define RAD2DEG 57.29577951308232f

static const char* MOON_NAMES_SK[8] = {
  "Nov", "Dorastajúci kosák", "Prvá štvrť", "Dorastajúci mesiac",
  "Spln", "Ubúdajúci mesiac", "Posledná štvrť", "Ubúdajúci kosák"
};

static const char* MOON_NAMES_CZ[8] = {
  "Nov", "Dorůstající srp", "První čtvrť", "Dorůstající měsíc",
  "Úplněk", "Ubývající měsíc", "Poslední čtvrť", "Ubývající srp"
};

static const char* MOON_NAMES_EN[8] = {
  "New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous",
  "Full Moon", "Waning Gibbous", "Last Quarter", "Waning Crescent"
};

MoonInfo Astro_GetMoon(time_t t) {
  MoonInfo info;
  // Julian Date
  double jd = (double)t / 86400.0 + 2440587.5;
  // Known new moon: Jan 6, 2000 18:14 UTC (JD 2451549.26)
  double d = jd - 2451549.26;
  const double synodicMonth = 29.530588853;
  double phase = fmod(d, synodicMonth) / synodicMonth;
  if (phase < 0.0) phase += 1.0;

  info.phase = (float)phase;
  // Illumination percentage: 0% at phase 0/1, 100% at phase 0.5
  info.illumination = (float)((1.0 - cos(phase * 2.0 * M_PI)) * 50.0);

  if (phase < 0.03 || phase >= 0.97)      info.phaseEnum = MOON_NEW;
  else if (phase < 0.22)                   info.phaseEnum = MOON_WAXING_CRESCENT;
  else if (phase < 0.28)                   info.phaseEnum = MOON_FIRST_QUARTER;
  else if (phase < 0.47)                   info.phaseEnum = MOON_WAXING_GIBBOUS;
  else if (phase < 0.53)                   info.phaseEnum = MOON_FULL;
  else if (phase < 0.72)                   info.phaseEnum = MOON_WANING_GIBBOUS;
  else if (phase < 0.78)                   info.phaseEnum = MOON_LAST_QUARTER;
  else                                     info.phaseEnum = MOON_WANING_CRESCENT;

  uint8_t lang = Settings_Language();
  if (lang == LANG_SK)      info.name = MOON_NAMES_SK[info.phaseEnum];
  else if (lang == LANG_EN) info.name = MOON_NAMES_EN[info.phaseEnum];
  else                      info.name = MOON_NAMES_CZ[info.phaseEnum];

  return info;
}

// NOAA Solar Calculation algorithm
static time_t calcSunEvent(double lat, double lon, time_t t, double zenith, bool sunrise) {
  struct tm tmDate;
  gmtime_r(&t, &tmDate);
  int year = tmDate.tm_year + 1900;
  int month = tmDate.tm_mon + 1;
  int day = tmDate.tm_mday;

  // Day of year N
  int N1 = floor(275 * month / 9);
  int N2 = floor((month + 9) / 12);
  int N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
  int N = N1 - (N2 * N3) + day - 30;

  double lngHour = lon / 15.0;
  double t_approx = sunrise ? (N + ((6.0 - lngHour) / 24.0)) : (N + ((18.0 - lngHour) / 24.0));

  // Sun mean anomaly
  double M = (0.9856 * t_approx) - 3.289;
  // Sun true longitude
  double L = M + (1.916 * sin(M * DEG2RAD)) + (0.020 * sin(2 * M * DEG2RAD)) + 282.634;
  while (L < 0.0) L += 360.0;
  while (L >= 360.0) L -= 360.0;

  // Right ascension
  double RA = RAD2DEG * atan(0.91764 * tan(L * DEG2RAD));
  while (RA < 0.0) RA += 360.0;
  while (RA >= 360.0) RA -= 360.0;

  // Right ascension value needs to be in the same quadrant as L
  double Lquadrant  = (floor(L / 90.0)) * 90.0;
  double RAquadrant = (floor(RA / 90.0)) * 90.0;
  RA = (RA + (Lquadrant - RAquadrant)) / 15.0;

  // Sun's declination
  double sinDec = 0.39782 * sin(L * DEG2RAD);
  double cosDec = cos(asin(sinDec));

  // Sun's local hour angle
  double cosH = (cos(zenith * DEG2RAD) - (sinDec * sin(lat * DEG2RAD))) / (cosDec * cos(lat * DEG2RAD));

  if (cosH > 1.0 || cosH < -1.0) return 0;   // Polar day or polar night

  double H = sunrise ? (360.0 - RAD2DEG * acos(cosH)) : (RAD2DEG * acos(cosH));
  H = H / 15.0;

  // Local mean time of event
  double T = H + RA - (0.06571 * t_approx) - 6.622;
  double UT = T - lngHour;
  while (UT < 0.0) UT += 24.0;
  while (UT >= 24.0) UT -= 24.0;

  // Exact UTC Unix timestamp calculation
  int y = year - 1970;
  int leapDays = (y + 1) / 4;
  time_t daysSinceEpoch = y * 365 + leapDays + N - 1;
  return (daysSinceEpoch * 86400) + (time_t)round(UT * 3600.0);
}

SolarTimes Astro_GetSolar(double lat, double lon, time_t t) {
  SolarTimes st = {0};
  st.sunrise         = calcSunEvent(lat, lon, t, 90.833, true);
  st.sunset          = calcSunEvent(lat, lon, t, 90.833, false);
  st.civilDawn       = calcSunEvent(lat, lon, t, 96.0, true);
  st.civilDusk       = calcSunEvent(lat, lon, t, 96.0, false);
  st.goldenHourStart = calcSunEvent(lat, lon, t, 84.0, false);
  st.goldenHourEnd   = calcSunEvent(lat, lon, t, 94.0, false);
  st.valid = (st.sunrise > 0 && st.sunset > 0);
  return st;
}

void Astro_DrawMoonIcon(int cx, int cy, int r, float phase) {
  if (!gfx || r <= 0) return;

  const uint16_t COL_DARK = 0x18E3;    // Dark slate gray (lunar night/shadow)
  const uint16_t COL_LIT  = 0xFFDF;    // Pearl white/cream (sunlit surface)
  const uint16_t COL_RING = 0x4A69;    // Medium slate outline ring

  // 1. Draw base dark circular silhouette
  gfx->fillCircle(cx, cy, r, COL_DARK);

  // Normalize phase to [0.0, 1.0)
  while (phase < 0.0f) phase += 1.0f;
  while (phase >= 1.0f) phase -= 1.0f;

  float cosAngle = cosf(phase * 2.0f * (float)M_PI);
  bool waxing = (phase <= 0.5f);

  for (int dy = -r; dy <= r; dy++) {
    int dxMax = (int)sqrtf((float)(r * r - dy * dy));
    if (dxMax <= 0) continue;

    int xLitStart, xLitEnd;
    if (waxing) {
      // Waxing (0.0 .. 0.5): Lit from terminator to +dxMax (Right side illuminated)
      int xTerm = (int)roundf(dxMax * cosAngle);
      xLitStart = xTerm;
      xLitEnd   = dxMax;
    } else {
      // Waning (0.5 .. 1.0): Lit from -dxMax to terminator (Left side illuminated)
      int xTerm = (int)roundf(-dxMax * cosAngle);
      xLitStart = -dxMax;
      xLitEnd   = xTerm;
    }

    if (xLitStart < -dxMax) xLitStart = -dxMax;
    if (xLitEnd > dxMax)    xLitEnd = dxMax;

    if (xLitEnd > xLitStart) {
      gfx->drawFastHLine(cx + xLitStart, cy + dy, xLitEnd - xLitStart + 1, COL_LIT);
    }
  }

  // 3. Crisp outline ring around the moon
  gfx->drawCircle(cx, cy, r, COL_RING);
}

void Astro_DrawSolarArc(int cx, int cy, int r, int thickness, double lat, double lon, time_t now) {
  if (!gfx || r <= 0 || thickness <= 0) return;

  SolarTimes st = Astro_GetSolar(lat, lon, now);
  if (!st.valid) return;

  struct tm localNow;
  localtime_r(&now, &localNow);
  int curMin = localNow.tm_hour * 60 + localNow.tm_min;

  struct tm sr, ss, cd, cs, gh1, gh2;
  localtime_r(&st.sunrise, &sr);
  localtime_r(&st.sunset, &ss);
  localtime_r(&st.civilDawn, &cd);
  localtime_r(&st.civilDusk, &cs);
  localtime_r(&st.goldenHourStart, &gh1);
  localtime_r(&st.goldenHourEnd, &gh2);

  int minSunrise = sr.tm_hour * 60 + sr.tm_min;
  int minSunset  = ss.tm_hour * 60 + ss.tm_min;
  int minDawn    = cd.tm_hour * 60 + cd.tm_min;
  int minDusk    = cs.tm_hour * 60 + cs.tm_min;
  int minGoldS   = gh1.tm_hour * 60 + gh1.tm_min;
  int minGoldE   = gh2.tm_hour * 60 + gh2.tm_min;

  // 24-hour arc: 0h (midnight) is at bottom (180 deg), 12h (noon) at top (0 deg).
  const uint16_t COL_DAY   = 0x04BF;  // Vibrant Sky Blue
  const uint16_t COL_GOLD  = 0xFD20;  // Warm Amber / Golden Hour
  const uint16_t COL_TWIL  = 0x4170;  // Twilight Indigo
  const uint16_t COL_NIGHT = 0x18C3;  // Deep Night Navy

  for (int m = 0; m < 1440; m += 4) {
    float deg = ((float)m / 1440.0f) * 360.0f + 180.0f;
    float rad = deg * DEG2RAD;

    uint16_t col;
    if (m >= minSunrise && m <= minGoldS) {
      col = COL_DAY;
    } else if ((m >= minGoldS && m <= minGoldE) || (m >= minDawn && m <= minSunrise)) {
      col = COL_GOLD;
    } else if (m >= minGoldE && m <= minDusk) {
      col = COL_TWIL;
    } else {
      col = COL_NIGHT;
    }

    for (int t = 0; t < thickness; t++) {
      int radDist = r - t;
      int px = cx + (int)(radDist * sinf(rad));
      int py = cy - (int)(radDist * cosf(rad));
      gfx->drawPixel(px, py, col);
    }
  }

  // Draw current Sun position dot on the 24h dial
  float curDeg = ((float)curMin / 1440.0f) * 360.0f + 180.0f;
  float curRad = curDeg * DEG2RAD;
  int sunX = cx + (int)(r * sinf(curRad));
  int sunY = cy - (int)(r * cosf(curRad));
  gfx->fillCircle(sunX, sunY, thickness + 1, C_YELLOW);
  gfx->drawCircle(sunX, sunY, thickness + 2, C_WHITE);
}
