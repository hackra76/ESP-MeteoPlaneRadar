// =============================================================================
//  MeteoPlaneRadar
//  Local time zone. See TimeZone.h.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "TimeZone.h"
#include "Config.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char s_tz[24] = "";
static bool s_fromNet = false;

const char* TimeZone_Text()    { return s_tz; }
bool        TimeZone_FromNetwork() { return s_fromNet; }

static void applyTz(const char* tz) {
  strncpy(s_tz, tz, sizeof(s_tz) - 1);
  s_tz[sizeof(s_tz) - 1] = '\0';
  setenv("TZ", s_tz, 1);
  tzset();
}

void TimeZone_Begin() {
  s_fromNet = false;
  applyTz(TZ_INFO);
}

int TimeZone_Offset() {
  // Ask the C library rather than keeping our own copy: with the compiled-in
  // rule in force the answer depends on the date, and only the library knows
  // whether today is summer time.
  //
  // The obvious way to read it is tm_gmtoff, but that field is a BSD/glibc
  // extension and the newlib shipped with the ESP32 core does not have it -
  // it compiles on a PC and fails on the device. So take the long way round:
  // break the SAME instant into a local and a UTC calendar and subtract them.
  // That works on every C library, because it uses nothing but the fields the
  // standard guarantees.
  const time_t now = time(nullptr);
  struct tm lt, gt;
  localtime_r(&now, &lt);
  gmtime_r(&now, &gt);

  // Day difference. tm_yday jumps by a whole year across New Year's Eve, so
  // the year fields decide the sign there and the result is clamped to the
  // one day an offset can ever span.
  int days = lt.tm_yday - gt.tm_yday;
  if (lt.tm_year != gt.tm_year) days = (lt.tm_year > gt.tm_year) ? 1 : -1;
  else if (days >  1)          days = -1;
  else if (days < -1)          days =  1;

  return days * 86400
       + (lt.tm_hour - gt.tm_hour) * 3600
       + (lt.tm_min  - gt.tm_min)  * 60
       + (lt.tm_sec  - gt.tm_sec);
}

void TimeZone_SetOffset(int seconds) {
  // Sanity. Real offsets run from -12:00 to +14:00; anything else is a parse
  // accident and must not be allowed to move the clock.
  if (seconds < -12 * 3600 || seconds > 14 * 3600) {
    Serial.printf("CAS: ignoruji nesmyslny posun %d s\n", seconds);
    return;
  }

  // Already right? Then say nothing and - importantly - leave the compiled-in
  // rule in place, so a Czech device keeps real DST dates instead of a frozen
  // offset that would be an hour out for a few weeks after each changeover.
  if (TimeZone_Offset() == seconds) return;

  // POSIX counts WEST as positive, which is the opposite of everyone else, so
  // the sign flips here exactly once. UTC+2 in Prague becomes the string
  // "UTC-2:00", and New York's UTC-5 becomes "UTC+5:00".
  const int west = -seconds;
  const char sign = (west < 0) ? '-' : '+';
  const int mag = (west < 0) ? -west : west;

  char tz[24];
  snprintf(tz, sizeof(tz), "UTC%c%d:%02d", sign, mag / 3600, (mag % 3600) / 60);
  applyTz(tz);
  s_fromNet = true;
  Serial.printf("CAS: posun %+d s, TZ=\"%s\"\n", seconds, s_tz);
}
