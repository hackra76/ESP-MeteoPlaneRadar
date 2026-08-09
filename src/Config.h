// =============================================================================
//  MeteoPlaneRadar
//  Config.h - ALL user-tunable settings in one place.
//
//  This is the only file you normally need to touch when adapting the project:
//  time zone, default location, ranges, poll intervals, AP name, limits.
//  Everything here is a compile-time default; the location, brightness, units,
//  last screen and last range are also stored in NVS at runtime (Settings.*).
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#pragma once

// ---------------------------------------------------------------------------
//  Board pins / bus
// ---------------------------------------------------------------------------
#define I2C_SDA   15
#define I2C_SCL   7
#define BOOT_PIN  0        // hold at power-up (~3 s) = factory reset

// ---------------------------------------------------------------------------
//  Time zone (POSIX TZ string)
//
//  There is no NTP client and no system clock - see the note above setup() in
//  the .ino. This string is still needed: CHMU.cpp turns each frame's UTC
//  timestamp into local time through localtime_r(), and these rules are what
//  decide CET or CEST for the date of that particular frame.
// ---------------------------------------------------------------------------
#define TZ_INFO   "CET-1CEST,M3.5.0,M10.5.0/3"

// ---------------------------------------------------------------------------
//  Default location (Prague). Overwritten on first boot by IP geolocation, or
//  manually in the WiFi portal; the stored value always wins.
// ---------------------------------------------------------------------------
#define DEFAULT_LAT 50.0755
#define DEFAULT_LON 14.4378

// ---------------------------------------------------------------------------
//  Configuration access point (WiFi portal and OTA share this name)
// ---------------------------------------------------------------------------
#define AP_SSID     "MeteoPlaneRadar"
#define AP_PASSWORD ""     // "" = open network

// ---------------------------------------------------------------------------
//  Aircraft radar (adsb.fi)
// ---------------------------------------------------------------------------
#define ADSB_MAX 100       // max aircraft held/drawn (airborne only)

// Selectable ranges in km. Keep them ascending; the count is derived.
#define PLANE_RANGES_KM { 10.0f, 25.0f, 50.0f, 100.0f }

// Poll interval by range - larger areas return more data and are less
// time-critical, so they are polled less often (easier on the free API).
// After a failed fetch the interval is doubled.
#define ADSB_PERIOD_NEAR_MS  5000    // up to  ADSB_NEAR_KM
#define ADSB_PERIOD_MID_MS  10000    // up to  ADSB_MID_KM
#define ADSB_PERIOD_FAR_MS  15000    // beyond ADSB_MID_KM
#define ADSB_NEAR_KM 25.0f
#define ADSB_MID_KM  50.0f

// ---------------------------------------------------------------------------
//  Weather radar (CHMU)
// ---------------------------------------------------------------------------
// Radius in km around the user's position. The value 0 is special: it means
// "the whole country", a fixed view that ignores where the user is (see the
// CZ_VIEW_* box below). Keep it last - it is the widest of them all.
#define METEO_RANGES_KM { 25.0f, 50.0f, 100.0f, 200.0f, 0.0f }

// The fixed "whole country" view: centre of the republic and a radius wide
// enough to hold it. Expressed as centre + radius rather than a bounding box on
// purpose - the CHMU image has a different pixel-per-degree scale on each axis,
// so a box that looks square in pixels is NOT square on the ground and would
// stretch the country vertically. Going through the same radius maths as every
// other range keeps the scale honest (1.08 km per pixel in both directions).
//
// The country is 486 x 279 km, so 260 km of radius covers it with a margin that
// keeps the western tip clear of the round bezel. Costs ~480 kB of PSRAM per
// frame, ~2.9 MB for all six.
#define CZ_VIEW_LAT       49.805f
#define CZ_VIEW_LON       15.475f
#define CZ_VIEW_RADIUS_KM 260.0f

// ---------------------------------------------------------------------------
//  Clock and outside temperature (the line under the screen dots)
// ---------------------------------------------------------------------------
// Temperature comes from Open-Meteo: free, no key, no registration, a few
// hundred bytes per answer.
#define OUTSIDE_TEMP_URL "https://api.open-meteo.com/v1/forecast"
#define OUTSIDE_TEMP_PERIOD_MS 600000UL   // 10 min - it is a model value
#define OUTSIDE_TEMP_RETRY_MS   60000UL   // sooner while we have nothing yet

// Degree symbol. The built-in font is 7-bit ASCII, so a real "°" may come out
// as a random glyph; "degC" is the safe spelling. Flip to 1 only after checking
// on the actual display that the character renders.
#define OUTSIDE_DEG_SYMBOL 0
#if OUTSIDE_DEG_SYMBOL
  #define OUTSIDE_DEG_TEXT "\xB0C"
#else
  #define OUTSIDE_DEG_TEXT "degC"
#endif

// ---------------------------------------------------------------------------
//  Flight route lookup (adsbdb.com) - free, no key, no registration.
//  Asked only when an aircraft's detail is opened, one aircraft at a time.
// ---------------------------------------------------------------------------
#define ROUTE_API_BASE "https://api.adsbdb.com"
#define ROUTE_CACHE_N  8     // remembered answers (callsign or hex)

// ---------------------------------------------------------------------------
//  OTA (firmware update over WiFi)
// ---------------------------------------------------------------------------
#define OTA_IDLE_MS 300000UL   // leave OTA mode after this long with no upload

// ---------------------------------------------------------------------------
//  Map orientation
//  The user picks which compass bearing sits at the TOP of the aircraft radar,
//  i.e. the direction they are looking. The step must divide 90 evenly,
//  otherwise the exact cardinal directions (east / west) become unreachable.
// ---------------------------------------------------------------------------
#define MAP_ROT_STEP_DEG 45    // degrees per button press (45 -> 8 positions)

// ---------------------------------------------------------------------------
//  Touch
// ---------------------------------------------------------------------------
// The CST820 drops the odd sample in the middle of a drag, and a failed I2C
// read is thrown away for the same reason (see Touch_CST820.cpp). Ending the
// gesture on the first empty sample would turn one swipe into several bogus
// taps - so require this much continuous silence before accepting that the
// finger is really up. Real gestures last 40 ms and up, so 60 ms costs nothing.
#define TOUCH_RELEASE_MS 60

// 1 = only read the controller when it says it has something to report.
//
// The CST820 signals a touch event by pulling INT low; reading the registers at
// any other moment returns stale or garbage data, and the chip also puts itself
// into standby when nothing is happening, where it may not answer at all. We
// used to poll it every few milliseconds regardless - which is where the all
// 0xFF samples came from. The pin is watched by an interrupt (a level check
// would miss the pulse while a frame is being drawn), and there is still a slow
// fallback poll so a lost edge cannot leave the touch dead.
//
// Set to 0 to go back to unconditional polling if the touch ever feels
// unresponsive on some board revision.
#define TOUCH_USE_INT 1

// Fallback poll while idle - only a safety net for a missed interrupt.
#define TOUCH_IDLE_POLL_MS 250

// How often to read the I/O expander back and repair it if it does not match
// what we wrote (TCA9554_Verify). Cheap - one I2C register read.
#define EXPANDER_CHECK_MS 5000UL

// ---------------------------------------------------------------------------
//  Display watchdog
//
//  Reports of a screen that goes black after anywhere from ten minutes to two
//  hours, with the backlight still on and the board otherwise alive. The task
//  watchdog cannot catch that: loop() keeps running, so it keeps being fed.
//  This one watches the panel instead - the VSYNC interrupt counts frames, and
//  if that count stops moving, the display is gone.
// ---------------------------------------------------------------------------
#define DISPLAY_WD 1              // 0 = do not watch the panel at all

// No frame for this long = the panel is dead. One frame is ~34 ms, so anything
// above a second is already far outside normal.
#define DISPLAY_WD_DEAD_MS 3000UL

// First a repair is attempted (put the expander back). If the panel is still
// not scanning this long after that, reboot - the users are power-cycling the
// board by hand anyway, this just does it for them.
#define DISPLAY_WD_REBOOT_MS 8000UL

// ---------------------------------------------------------------------------
//  Network
// ---------------------------------------------------------------------------
// A TLS handshake needs roughly 45 kB of internal RAM. Starting one with less
// than this free fails deep inside mbedTLS and surfaces as a bare "HTTP -1",
// so skip the poll instead and try again later.
#define NET_MIN_HEAP 60000

// The WiFi portal blocks the whole sketch and the watchdog is suspended while
// it runs, so this timeout is what keeps it from blocking forever.
#define PORTAL_TIMEOUT_S 180

// ---------------------------------------------------------------------------
//  Aircraft detail
// ---------------------------------------------------------------------------
// adsb.fi occasionally drops an aircraft from a single poll and sends it again
// in the next one. Closing the detail panel on the first miss looks like the
// panel closes by itself, so tolerate this many consecutive misses first.
#define DETAIL_GRACE_POLLS 2

// ---------------------------------------------------------------------------
//  Diagnostics
//
//  The serial log comes out at 115200 Bd over the connector marked "USB" -
//  that is the ESP32-S3's native USB. Nothing shows up on the other USB-C
//  connector on the board.
// ---------------------------------------------------------------------------
// 1 = log touch gestures and every aircraft-selection change (with the reason
// why the detail closed) to the serial console at 115200 Bd.
#define TOUCH_DEBUG 0

// 1 = measure how long one full-screen flush takes and print min/last/max once
// per second. Use this to diagnose a flickering band: one frame lasts ~34 ms,
// so if the flush takes anywhere near that, the copy and the panel's scan-out
// run at the same speed and keep crossing each other. Set to 0 when done.
#define FLUSH_DEBUG 0

// ---------------------------------------------------------------------------
//  Watchdog
// ---------------------------------------------------------------------------
#define WDT_TIMEOUT_S 20       // reboot after this many seconds of being stuck
