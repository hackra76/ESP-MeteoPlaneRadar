// =============================================================================
//  MeteoPlaneRadar
//  Local time zone - interface.
//
//  The clock does not use NTP (see Outside.h): every HTTPS response carries a
//  "Date" header in GMT, so time() is a correct UTC epoch anywhere in the
//  world. Turning that into LOCAL time is a separate problem, and until 0.7.0
//  it was solved with a constant compiled into Config.h - which is right in
//  Prague and wrong everywhere else. A device carried to New York showed Czech
//  time on the clock screen and labelled the forecast with Czech hours: not a
//  blank screen, a confidently wrong one.
//
//  The offset now comes from the forecast response. Open-Meteo answers with
//  utc_offset_seconds for the coordinates it was asked about, the forecast is
//  fetched every half hour anyway, and it costs no extra request.
//
//  WHY AN OFFSET AND NOT A ZONE NAME: the newlib that ships with ESP-IDF has no
//  IANA database, so setenv("TZ", "America/New_York") does nothing useful. TZ
//  has to be a POSIX string, either a full rule with DST dates or a plain
//  offset. We use the offset, and because the forecast refreshes every thirty
//  minutes, the spring and autumn clock changes correct themselves within half
//  an hour of happening.
//
//  The compile-time TZ_INFO is NOT thrown away. It carries proper Czech DST
//  rules, so as long as the network offset agrees with what it already
//  produces, it is left alone and a Czech device keeps the exact rule. Only a
//  disagreement switches to the fixed offset.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

// Apply the compile-time default. Call once in setup(), before anything
// converts a timestamp for display.
void TimeZone_Begin();

// Offset east of UTC in seconds, as the forecast reported it for the user's
// coordinates. Does nothing when it matches what the active rule already gives,
// which is the normal case in Czechia.
void TimeZone_SetOffset(int seconds);

// What is in force right now, in seconds east of UTC. Reads it back from the C
// library rather than remembering it, so it is true even before the network has
// said anything.
int TimeZone_Offset();

// True once a network offset has actually been applied - i.e. the device is
// somewhere the compiled-in rule does not describe. Shown on the status page.
bool TimeZone_FromNetwork();

// The POSIX string currently in TZ, for the status page and the serial log.
const char* TimeZone_Text();
