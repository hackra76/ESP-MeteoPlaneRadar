// =============================================================================
//  MeteoPlaneRadar
//  Calendar helper shared by the weather frames and the clock.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

// Days since 1970-01-01 for a civil (proleptic Gregorian) date. Howard
// Hinnant's days_from_civil: exact, no loops, no library date support needed.
//
// Used to turn a UTC calendar date into an epoch WITHOUT consulting the system
// clock - which matters here, because the weather frame labels have to be right
// even before the clock has been set at all.
static inline long TimeUtil_DaysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (long)era * 146097 + (long)doe - 719468;
}

// UTC calendar -> epoch seconds.
static inline time_t TimeUtil_UtcToEpoch(int Y, int Mo, int D,
                                         int hh, int mm, int ss) {
  return (time_t)TimeUtil_DaysFromCivil(Y, (unsigned)Mo, (unsigned)D) * 86400L
       + (long)hh * 3600L + (long)mm * 60L + ss;
}
