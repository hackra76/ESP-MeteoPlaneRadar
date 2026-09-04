// =============================================================================
//  MeteoPlaneRadar
//  Screen 1: aircraft radar (adsb.fi) + aircraft detail.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenPlanes.h"
#include "AsyncCore.h"
#include "PlaneTrail.h"
#include "ADSB.h"
#include "Settings.h"
#include "Config.h"
#include "Route.h"
#include "PlanePhoto.h"
#include "EuBorder.h"
#include "Airports.h"
#include "UI.h"
#include "Layout.h"
#include "Lang.h"
#include "Status.h"
#include "Display_ST7701.h"
#include "AircraftType.h"

#include <WiFi.h>
#include <math.h>
#include <string.h>   // strncpy / strcmp for the hex-based selection

// Round panel - radar centred on the middle of the screen.
// R_RADIUS = 230 px maps the selected range onto the radar circle.
#define R_CX (LCD_WIDTH / 2)
#define R_CY (LCD_HEIGHT / 2)
#define R_RADIUS 230

// Ranges (radius in km).
static const float RANGES_KM[] = PLANE_RANGES_KM;
static const int   RANGE_COUNT = sizeof(RANGES_KM) / sizeof(RANGES_KM[0]);
static int s_rangeIdx = 1;   // 25 km by default

static float currentRange() { return RANGES_KM[s_rangeIdx]; }

// Base poll interval by range. Larger areas return more data and are less
// time-critical, so they are polled less often - keeps us a light user of the
// free adsb.fi API.
static unsigned long basePeriodMs() {
  float r = currentRange();
  if (r <= ADSB_NEAR_KM) return ADSB_PERIOD_NEAR_MS;
  if (r <= ADSB_MID_KM)  return ADSB_PERIOD_MID_MS;
  return ADSB_PERIOD_FAR_MS;
}

static unsigned long s_nextFetch = 0;
static bool  s_dataOk = false;
static String s_status = "...";

// Aircraft detail - the selected aircraft is remembered by its ICAO hex address,
// NOT by its index in the list.
//
// The list is rebuilt from scratch on every fetch (every 5 s) and adsb.fi does
// not guarantee a stable order: one aircraft leaving the area shifts everything
// after it. Holding an index across a fetch therefore silently repoints the
// detail at a different aircraft. Keying on the hex means the panel either
// follows the aircraft you actually picked, or closes when it genuinely leaves.
//
// Empty string = nothing selected / detail closed.
static char s_selectedHex[8] = "";

// Grace period for the detail panel. adsb.fi sometimes omits an aircraft from a
// single poll and sends it again in the next one; closing the panel on the
// first miss looks like it closes by itself. We therefore keep the last known
// data and only give up after DETAIL_GRACE_POLLS consecutive misses.
static int      s_selMiss = 0;          // consecutive polls without the aircraft
static Aircraft s_selCache;             // last known data of the selected one
static bool     s_selCacheOk = false;

// Screen positions from the last draw, for tap-to-select. Parallel to the
// aircraft list, so s_planeHex[i] records which aircraft the point belongs to
// and the tap resolves to an identity rather than to a slot number.
static int  s_planeX[ADSB_MAX];
static int  s_planeY[ADSB_MAX];
static char s_planeHex[ADSB_MAX][8];
static int  s_planeN = 0;

bool ScreenPlanes_DetailOpen() { return s_selectedHex[0] != '\0'; }

// reason = short text for the serial log, so we can tell WHY the panel closed
// (user tap vs. the aircraft disappearing from the data).
static void selectNone(const char* reason) {
#if TOUCH_DEBUG
  if (s_selectedHex[0]) Serial.printf("SEL: zavreno (%s) hex=%s\n", reason, s_selectedHex);
#else
  (void)reason;
#endif
  s_selectedHex[0] = '\0';
  s_selMiss = 0;
  s_selCacheOk = false;
  Route_Clear();
  PlanePhoto_Clear();
}
static void selectHex(const char* hex) {
  strncpy(s_selectedHex, hex, sizeof(s_selectedHex) - 1);
  s_selectedHex[sizeof(s_selectedHex) - 1] = '\0';
  s_selMiss = 0;
  s_selCacheOk = false;
#if TOUCH_DEBUG
  Serial.printf("SEL: vybrano hex=%s\n", s_selectedHex);
#endif
}

// Convert an aircraft's lat/lon into screen coordinates (north up).
// --- Map orientation -------------------------------------------------------
// The user picks WHICH COMPASS BEARING IS AT THE TOP of the screen, i.e. the
// direction they are looking out of the window. Everything then follows:
//
//     screen angle of bearing b  =  b - topBearing      (0 = up, clockwise)
// Borders and cities go through the same project(), so they follow along for
// free; only the aircraft icons need their heading corrected separately.
static float s_rotSin = 0.0f, s_rotCos = 1.0f;   // cached sin/cos of the angle
static uint16_t s_topDeg = 0;                    // bearing shown at the top

static void refreshRotation() {
  uint16_t deg = Settings_TopBearing();
  if (deg == s_topDeg) return;
  s_topDeg = deg;
  float r = (float)deg * 0.0174532925f;
  s_rotSin = sinf(r);
  s_rotCos = cosf(r);
}

static void project(float lat, float lon, double clat, double clon,
                    float rangeKm, int* sx, int* sy) {
  float latr = clat * 0.0174532925f;
  float dxKm = (lon - clon) * 111.0f * cosf(latr);
  float dyKm = (lat - clat) * 111.0f;
  // Rotate the local east/north vector so that the chosen bearing ends up at
  // the top: with top = 90 (looking east) a target due east appears up.
  float rx = dxKm * s_rotCos - dyKm * s_rotSin;
  float ry = dxKm * s_rotSin + dyKm * s_rotCos;
  float scale = (float)R_RADIUS / rangeKm;   // px per km
  *sx = R_CX + (int)(rx * scale);
  *sy = R_CY - (int)(ry * scale);
}

static double s_viewLat = 0.0, s_viewLon = 0.0;
static void cityProject(float lat, float lon, int* sx, int* sy) {
  project(lat, lon, s_viewLat, s_viewLon, currentRange(), sx, sy);
}

// --- Altitude colour bands ---
// The colour of an aircraft encodes its barometric altitude, so a glance tells
// you whether something is on approach overhead or just transiting at cruise.
// The bands are contiguous - every altitude falls into exactly one.
//
//   < 2 km   red     approach/departure, helicopters, light aircraft
//   2-6 km   orange  climb/descent, regional traffic
//   6-10 km  yellow  lower cruise levels
//   >= 10 km blue    long-haul cruise
//
// An aircraft reporting no altitude at all (altFt == 0 and not on the ground)
// is drawn grey rather than being forced into the "low" band, which would
// otherwise paint every unknown target an alarming red.
static uint16_t altColor(float altFt, bool known) {
  return PlaneTrail_AltColor(altFt, known);
}

// Aircraft icon - a winged arrow, rotated to match the ground track.
// Filled polygon; the nose points "forward" (locally up, fwd positive).
// When the track is unknown (hasTrack=false), a circle is drawn instead.
static void drawPlane(int x, int y, float trackDeg, bool hasTrack, uint16_t col, bool isMilitary = false) {
  if (!hasTrack) {
    // Track unknown - circle with a dot (orientation cannot be determined).
    gfx->drawCircle(x, y, 7, isMilitary ? C_RED : col);
    gfx->fillCircle(x, y, 2, isMilitary ? C_RED : col);
    return;
  }
  float a = trackDeg * 0.0174532925f;
  float ca = cosf(a), sa = sinf(a);
  auto rot = [&](float right, float fwd, int* ox, int* oy) {
    *ox = x + (int)(right * ca + fwd * sa);
    *oy = y + (int)(right * sa - fwd * ca);
  };

  if (isMilitary) {
    // Sharp delta-wing military fighter jet silhouette
    const float P_MIL[10][2] = {
      { 0,  14}, { 2,  5}, { 13, -6}, { 3, -4}, { 3, -13},
      { 0, -10}, {-3, -13}, {-3, -4}, {-13, -6}, {-2,  5}
    };
    int px[10], py[10];
    for (int i = 0; i < 10; i++) rot(P_MIL[i][0], P_MIL[i][1], &px[i], &py[i]);
    for (int i = 0; i < 10; i++) {
      int j = (i + 1) % 10;
      gfx->fillTriangle(x, y, px[i], py[i], px[j], py[j], C_RED);
    }
    // High-contrast center point
    int cx, cy;
    rot(0, 2, &cx, &cy);
    gfx->fillCircle(cx, cy, 1, C_WHITE);
    return;
  }

  // Civil commercial airliner
  const float P[10][2] = {
    { 0,  12}, { 3,  1}, { 13, -8}, { 3, -5}, { 3, -7},
    { 0, -12}, {-3, -7}, {-3, -5}, {-13, -8}, {-3,  1}
  };
  int px[10], py[10];
  for (int i = 0; i < 10; i++) rot(P[i][0], P[i][1], &px[i], &py[i]);

  // Fill as a triangle fan from the centre of the icon.
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    gfx->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
  }
}

// --- Filters ----------------------------------------------------------------
// Set from the web UI. The point is not to hide traffic but to make a busy sky
// readable: over an approach path the interesting aircraft is the one at 2000
// ft, and at home under an airway it is the opposite.
//
// The filter applies to DRAWING only. The aircraft stays in the list, so the
// count, the emergency scan and a watched callsign all still see it - hiding an
// emergency because of an altitude filter would be a nasty surprise.
static bool passesFilter(const Aircraft& a) {
  if (Settings_OnlyWithCallsign() && !a.callsign[0]) return false;
  const uint16_t lo = Settings_AltMinFt(), hi = Settings_AltMaxFt();
  if (lo == 0 && hi >= 60000) return true;          // filter off
  if (a.altFt <= 0) return true;                    // unknown altitude - keep it
  if (a.altFt < (float)lo || a.altFt > (float)hi) return false;
  return true;
}

// Is this the aircraft the user asked to be told about? Matched against both
// the callsign and the ICAO address, because people quote whichever they have.
static bool isWatched(const Aircraft& a) {
  const char* w = Settings_WatchCallsign();
  if (!w || !*w) return false;
  if (a.callsign[0] && strcasecmp(a.callsign, w) == 0) return true;
  if (a.hex[0] && strcasecmp(a.hex, w) == 0) return true;
  return false;
}

void ScreenPlanes_Enter() {
  s_rangeIdx = Settings_PlaneRange();
  if (s_rangeIdx >= RANGE_COUNT) s_rangeIdx = 1;   // guard against a stale value
  Async_SetActiveScreen(SCREEN_PLANES_I);
  Async_SetAdsbTarget(Settings_Lat(), Settings_Lon(), currentRange());
  Async_RequestAdsb();
  s_nextFetch = 0;
}

bool ScreenPlanes_Tick() {
  if (WiFi.status() != WL_CONNECTED) { s_status = T(S_WIFI_WAIT); return false; }

  bool routeChanged = Async_TakeRouteUpdated() || Route_TakeChanged() || PlanePhoto_TakeChanged();
  bool adsbChanged  = Async_TakeAdsbUpdated();

  if (adsbChanged) {
    s_dataOk = (ADSB_Count() > 0);
    s_status = s_dataOk ? T(S_OK) : T(S_ERROR);

    if (ScreenPlanes_DetailOpen()) {
      if (ADSB_FindByHex(s_selectedHex) >= 0) {
        s_selMiss = 0;                       // still there
      } else if (++s_selMiss > DETAIL_GRACE_POLLS) {
        selectNone("letadlo zmizelo z dat");
      }
    }
    return true;   // new data -> redraw
  }
  return routeChanged;   // redraw if route arrived
}

// Short tap: with the detail open any tap closes it (the units toggle that used
// to sit at the bottom of the panel now lives on the settings screen).
// Otherwise select the nearest aircraft.
bool ScreenPlanes_HandleTap(int x, int y) {
  if (ScreenPlanes_DetailOpen()) {
    selectNone("tap mimo panel");
    return true;
  }
  // Find the aircraft nearest the tap (within 30 px), then remember *which
  // aircraft* it was, not where it happened to sit in the list.
  int best = -1;
  long bestD = 30L * 30L;
  for (int i = 0; i < s_planeN; i++) {
    long dx = s_planeX[i] - x, dy = s_planeY[i] - y;
    long d = dx * dx + dy * dy;
    if (d < bestD) { bestD = d; best = i; }
  }
  if (best >= 0 && s_planeHex[best][0]) {
    selectHex(s_planeHex[best]);
    // The callsign is only known from the data, so the lookup is kicked off
    // where the aircraft is drawn (below) - here we just have the hex.
    return true;
  }
  return false;
}

// Swipe: change the range (both directions). Re-fetch immediately at the new
// radius. Ignored while the detail panel is open (close it first).
void ScreenPlanes_ChangeRange(int dir) {
  if (ScreenPlanes_DetailOpen()) return;
  s_rangeIdx = (s_rangeIdx + dir + RANGE_COUNT) % RANGE_COUNT;
  Settings_SetPlaneRange(s_rangeIdx);   // remember across restarts (debounced)
  Serial.printf("ADSB range: %.0f km\n", currentRange());
  Async_SetAdsbTarget(Settings_Lat(), Settings_Lon(), currentRange());
  Async_RequestAdsb();
}

void ScreenPlanes_RangeText(char* out, size_t cap) {
  if (!out || !cap) return;
  snprintf(out, cap, "%.0f %s", currentRange(), T(S_KM));
}

// Close the detail panel (called on the long-press screen switch).
void ScreenPlanes_CloseDetail() { selectNone("dlouhy stisk / prepnuti obrazovky"); }

void ScreenPlanes_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  refreshRotation();           // pick up a changed rotation setting
  float range = currentRange();

  // Reserve the chrome BEFORE the map is drawn. City labels and callsigns are
  // placed by data rather than by design, so they have to lose the argument -
  // and they can only lose it if the space is already taken when they ask.
  Layout_ReserveBand(LY_DOTS - 6, 12);        // screen selector dots
  Layout_ReserveBand(LY_STATUS - 3, 22);      // clock + outside temperature
  // The alert banner is taller than the count it replaces, and the legend has
  // its boundary numbers under the colour bar - both bands are sized for the
  // taller variant so a city label can never creep into either.
  Layout_ReserveBand(LY_SUB - 6, 22);         // aircraft count / alert banner
  if (Settings_ShowLegends()) {
    Layout_ReserveBand(LY_LEGEND - 2, 24);    // altitude legend + numbers
  }
  Layout_ReserveBand(LY_RANGE - 2, 20);       // range readout
  Layout_ReserveBand(LY_RANGE_DOTS - 6, 12);  // range dots

  // --- Map underlay ---
  static Aircraft* s_drawList = nullptr;
  if (!s_drawList) {
    s_drawList = (Aircraft*)heap_caps_malloc(sizeof(Aircraft) * ADSB_MAX, MALLOC_CAP_SPIRAM);
    if (!s_drawList) s_drawList = (Aircraft*)malloc(sizeof(Aircraft) * ADSB_MAX);
  }
  Async_LockAdsb();
  int n = ADSB_Count();
  if (n > ADSB_MAX) n = ADSB_MAX;
  const Aircraft* liveList = ADSB_List();
  if (s_drawList && liveList) {
    for (int i = 0; i < n; i++) s_drawList[i] = liveList[i];
  }
  Async_UnlockAdsb();
  const Aircraft* list = s_drawList ? s_drawList : liveList;
  s_planeN = n;

  // Scan for any emergency aircraft (7500 / 7600 / 7700)
  int emergIdx = -1;
  const char* alertCode = nullptr;
  if (Settings_SquawkAlert()) {
    for (int i = 0; i < n; i++) {
      if (list[i].onGround) continue;
      const char* em = ADSB_EmergencyCode(list[i]);
      if (em) {
        emergIdx = i;
        alertCode = em;
        break;
      }
    }
  }

  // Camera center follows emergency aircraft if present, else user location
  s_viewLat = (emergIdx >= 0) ? (double)list[emergIdx].lat : Settings_Lat();
  s_viewLon = (emergIdx >= 0) ? (double)list[emergIdx].lon : Settings_Lon();
  const double clat = s_viewLat;
  const double clon = s_viewLon;
  const float  marginKm = range * 1.2f;
  const float  dLat = marginKm / 111.0f;
  const float  dLon = marginKm / (111.0f * cosf(clat * 0.0174532925f));
  const float  lat0 = clat - dLat, lat1 = clat + dLat;
  const float  lon0 = clon - dLon, lon1 = clon + dLon;

  EuBorder_Draw(cityProject, C_GRAY, lat0, lat1, lon0, lon1);

  // Cities as an underlay (below the aircraft, so it is clear where they are).
  {
    int rad = LCD_WIDTH / 2 - 4;
    bool showFull = (range <= 25.0f);    // full names only at close range
    uint8_t maxTier;
    if      (range <= 50.0f)  maxTier = 3;
    else if (range <= 150.0f) maxTier = 2;
    else                      maxTier = 1;
    EuBorder_DrawCities(cityProject, R_CX, R_CY, rad, C_WHITE, C_CYAN,
                        showFull, maxTier, lat0, lat1, lon0, lon1);
  }

  // Airports as underlay (below aircraft, above cities).
  if (Settings_RadarShowAirports()) {
    Airports_Draw(cityProject, R_CX, R_CY, LCD_WIDTH / 2 - 4, range,
                  lat0, lat1, lon0, lon1);
  }

  // Range rings and Home Base waypoint beacon
  if (Settings_RadarShowRings()) {
    gfx->drawCircle(R_CX, R_CY, LCD_WIDTH / 2 - 2, C_DKGRAY);
    gfx->drawCircle(R_CX, R_CY, LCD_WIDTH / 4, C_DKGRAY);
  }
  if (emergIdx < 0) {
    UI_DrawHomeMarker(R_CX, R_CY);
  } else {
    // In emergency mode, draw home base at its offset location
    int hx, hy;
    project(Settings_Lat(), Settings_Lon(), clat, clon, range, &hx, &hy);
    if ((long)(hx - R_CX) * (hx - R_CX) + (long)(hy - R_CY) * (hy - R_CY) <= (long)R_RADIUS * R_RADIUS) {
      gfx->drawCircle(hx, hy, 6, C_CYAN);
      gfx->fillCircle(hx, hy, 2, C_WHITE);
    }
  }

  // Aircraft.
  int shown = 0;
  int selIdx = ADSB_FindByHex(s_selectedHex);
  bool watchedSeen = false;

  int closestIdx = -1;
  float minDistKm = 999999.0f;
  int specialIdx = -1;
  float specialDistKm = 999999.0f;
  char specialLabel[32] = "";
  uint16_t specialCol = C_CYAN;
  SpecialCategory bestSpecialCat = SPEC_NONE;

  for (int i = 0; i < n; i++) {
    s_planeX[i] = -9999; s_planeY[i] = -9999;   // default: off-screen
    s_planeHex[i][0] = '\0';
    if (list[i].onGround) continue;

    // In Emergency mode: isolate and show ONLY the emergency aircraft!
    if (emergIdx >= 0 && i != emergIdx) continue;

    // Proximity calculation in 3D (ground distance + altitude)
    double dLatKm = (list[i].lat - clat) * 111.0;
    double dLonKm = (list[i].lon - clon) * (111.0 * cos(clat * 0.0174532925));
    float dGnd = sqrtf((float)(dLatKm * dLatKm + dLonKm * dLonKm));
    float dAlt = (list[i].altFt * 0.3048f) / 1000.0f;
    float d3D = sqrtf(dGnd * dGnd + dAlt * dAlt);
    if (d3D < minDistKm) {
      minDistKm = d3D;
      closestIdx = i;
    }

    // Classify special interest flights (rescue, government, icon, military)
    char sLbl[32]; uint16_t sCol = C_WHITE;
    SpecialCategory scat = Aircraft_Classify(list[i], sLbl, sizeof(sLbl), &sCol);
    if (scat != SPEC_NONE && scat >= bestSpecialCat) {
      bestSpecialCat = scat;
      specialIdx = i;
      specialDistKm = dGnd;
      strncpy(specialLabel, sLbl, sizeof(specialLabel) - 1);
      specialLabel[sizeof(specialLabel) - 1] = '\0';
      specialCol = sCol;
    }

    const char* em = Settings_SquawkAlert() ? ADSB_EmergencyCode(list[i]) : nullptr;
    const bool watched = isWatched(list[i]);
    if (em && !alertCode) alertCode = em;
    if (watched) watchedSeen = true;

    if (!em && !watched && !passesFilter(list[i])) continue;
    int sx, sy;
    project(list[i].lat, list[i].lon, clat, clon, range, &sx, &sy);
    int dx = sx - R_CX, dy = sy - R_CY;
    if (dx * dx + dy * dy > R_RADIUS * R_RADIUS) continue;   // outside the circle
    s_planeX[i] = sx; s_planeY[i] = sy;
    strncpy(s_planeHex[i], list[i].hex, sizeof(s_planeHex[i]) - 1);
    s_planeHex[i][sizeof(s_planeHex[i]) - 1] = '\0';

    if (i == selIdx) gfx->drawCircle(sx, sy, 16, C_WHITE);
    if (em) {
      gfx->drawCircle(sx, sy, 20, C_RED);
      gfx->drawCircle(sx, sy, 21, C_RED);
      gfx->drawCircle(sx, sy, 22, C_WHITE);
    } else if (scat != SPEC_NONE) {
      gfx->drawCircle(sx, sy, 15, sCol);
      gfx->drawCircle(sx, sy, 16, sCol);
    } else if (watched) {
      gfx->drawCircle(sx, sy, 15, C_GREEN);
      gfx->drawCircle(sx, sy, 16, C_GREEN);
    }

    bool altKnown = (list[i].altFt > 0.0f);
    bool isMil = list[i].isMilitary || (scat == SPEC_MILITARY);
    uint16_t col = (em || isMil) ? C_RED : altColor(list[i].altFt, altKnown);

    // Draw flight trajectory breadcrumb trail
    if (Settings_RadarShowTrails()) {
      PlaneTrail_Draw(list[i].hex, cityProject, col, sx, sy);
    }

    float screenTrack = list[i].track - (float)s_topDeg;
    while (screenTrack < 0.0f) screenTrack += 360.0f;
    drawPlane(sx, sy, screenTrack, list[i].hasTrack, col, isMil);

    // Label with smart multi-positioning and size fallback
    if (Settings_ShowLegends() && emergIdx < 0) {
      char label[24];
      const char* rawLabel = list[i].callsign[0] ? list[i].callsign : list[i].hex;
      if (rawLabel[0]) {
        if (list[i].baroRate > 300.0f) {
          snprintf(label, sizeof(label), "%s^", rawLabel);
        } else if (list[i].baroRate < -300.0f) {
          snprintf(label, sizeof(label), "%sv", rawLabel);
        } else {
          strncpy(label, rawLabel, sizeof(label) - 1);
          label[sizeof(label) - 1] = '\0';
        }

        uint16_t lCol = em ? C_RED : (watched ? C_GREEN : C_WHITE);
        // Smart placement: try below (size 2), above (size 2), right (size 2), then size 1 fallbacks
        struct Cand { int dx; int dy; uint8_t size; };
        const Cand cands[] = {
          { 0,  18, 2 },   // Centered below (Size 2)
          { 0, -20, 2 },   // Centered above (Size 2)
          { 14, -6, 2 },   // To the right (Size 2)
          { 0,  16, 1 },   // Centered below (Size 1)
          { 0, -14, 1 },   // Centered above (Size 1)
          { 12, -4, 1 }    // To the right (Size 1)
        };

        for (const auto& c : cands) {
          int tw = Layout_TextW(label, c.size);
          int th = LY_CHAR_H(c.size);
          int tx = (c.dx == 0) ? (sx - tw / 2) : (sx + c.dx);
          int ty = sy + c.dy;
          if (tx < 6 || tx + tw > LCD_WIDTH - 6 || ty < 6 || ty + th > LCD_HEIGHT - 6) continue;
          if (Layout_Claim(tx - 2, ty - 1, tw + 4, th + 2)) {
            UI_Text(label, tx, ty, lCol, c.size);

            // Route label (e.g. "VIE>LHR") on the line below the callsign
            if (list[i].callsign[0]) {
              const RouteInfo* rt = Route_GetCached(list[i].callsign);
              if (rt && rt->iataFrom[0] && rt->iataTo[0]) {
                char rl[12];
                snprintf(rl, sizeof(rl), "%s>%s", rt->iataFrom, rt->iataTo);
                int rw = Layout_TextW(rl, 1);
                int rh = LY_CHAR_H(1);
                int rx = (c.dx == 0) ? (sx - rw / 2) : tx;
                int ry = ty + th + 1;
                if (rx >= 6 && rx + rw <= LCD_WIDTH - 6 && ry + rh <= LCD_HEIGHT - 6) {
                  if (Layout_Claim(rx - 1, ry, rw + 2, rh + 1)) {
                    UI_Text(rl, rx, ry, C_YELLOW, 1);
                  }
                }
              } else if (!rt) {
                // Only queue background route for close aircraft (<= 35 km), the nearest one, or selected
                bool shouldQueue = (dGnd <= 35.0f) || (i == closestIdx) || (selIdx >= 0 && i == selIdx);
                if (shouldQueue) {
                  Route_Queue(list[i].callsign, list[i].lat, list[i].lon);
                }
              }
            }
            break;
          }
        }
      }
    }
    shown++;
  }

  // Draw Proximity Vector to the nearest overhead aircraft
  if (Settings_RadarShowNearest() && closestIdx >= 0 && s_planeX[closestIdx] > -9000 && !ScreenPlanes_DetailOpen() && minDistKm <= range) {
    int cx = s_planeX[closestIdx], cy = s_planeY[closestIdx];
    int steps = 10;
    for (int s = 2; s < steps; s += 2) {
      int px = R_CX + (cx - R_CX) * s / steps;
      int py = R_CY + (cy - R_CY) * s / steps;
      gfx->drawPixel(px, py, C_CYAN);
    }
    gfx->drawCircle(cx, cy, 14, C_CYAN);
  }

  // Top of the screen, under the screen-selector dots at y=18:
  //   y 30..45  clock + outside temperature (size 2, the same as the range)
  //   y 52..59  aircraft count (size 1 - it is a secondary number)
  //   y 74      altitude legend
  UI_DrawStatusLine(LY_STATUS);

  // The line under the clock: normally the aircraft count, but an emergency
  // squawk takes it over.
  char sub[48];
  if (emergIdx >= 0) {
    const Aircraft& emAc = list[emergIdx];
    const char* what = (strcmp(alertCode, SQUAWK_HIJACK) == 0) ? T(S_HIJACK)
                     : (strcmp(alertCode, SQUAWK_RADIO)  == 0) ? T(S_RADIO_FAIL)
                                                               : T(S_EMERGENCY);
    snprintf(sub, sizeof(sub), "! %s  %s !", alertCode, what);
    int tw = Layout_TextW(sub, 2);
    gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 8, LY_SUB - 4, tw + 16, 20, C_RED);
    UI_TextCentered(sub, LY_SUB - 1, C_WHITE, 2);

    // Emergency Telemetry HUD Card
    const int boxW = 320;
    const int boxH = 68;
    const int boxX = R_CX - boxW / 2;
    const int boxY = LCD_HEIGHT - boxH - 26;

    gfx->fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1800);
    gfx->drawRoundRect(boxX, boxY, boxW, boxH, 8, C_RED);

    char l1[36];
    const char* cname = emAc.callsign[0] ? emAc.callsign : emAc.hex;
    if (emAc.type[0]) {
      snprintf(l1, sizeof(l1), "%s  (%s)", cname, emAc.type);
    } else {
      snprintf(l1, sizeof(l1), "%s", cname);
    }
    UI_Text(l1, boxX + 12, boxY + 6, C_YELLOW, 2);

    char l2[48];
    snprintf(l2, sizeof(l2), "ALT: %.0f ft (%.0f m)   GS: %.0f km/h",
             emAc.altFt, emAc.altFt * 0.3048f, emAc.gsKt * 1.852f);
    UI_Text(l2, boxX + 12, boxY + 30, C_WHITE, 1);

    char l3[48];
    uint16_t l3Col = C_CYAN;
    if (emAc.baroRate < -300.0f) {
      snprintf(l3, sizeof(l3), "KLESANIE:  v %.0f ft/min (%.1f m/s)", emAc.baroRate, emAc.baroRate * 0.00508f);
      l3Col = C_RED;
    } else if (emAc.baroRate > 300.0f) {
      snprintf(l3, sizeof(l3), "STUPANIE:  ^ +%.0f ft/min (%.1f m/s)", emAc.baroRate, emAc.baroRate * 0.00508f);
      l3Col = C_GREEN;
    } else {
      snprintf(l3, sizeof(l3), "ROVNY LET: 0 ft/min   HDG: %.0f\xC2\xB0", emAc.track);
    }
    UI_Text(l3, boxX + 12, boxY + 48, l3Col, 1);
  } else if (alertCode) {
    const char* what = (strcmp(alertCode, SQUAWK_HIJACK) == 0) ? T(S_HIJACK)
                     : (strcmp(alertCode, SQUAWK_RADIO)  == 0) ? T(S_RADIO_FAIL)
                                                               : T(S_EMERGENCY);
    snprintf(sub, sizeof(sub), "%s  %s", alertCode, what);
    int tw = Layout_TextW(sub, 2);
    gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 6, LY_SUB - 4, tw + 12, 20, C_RED);
    UI_TextCentered(sub, LY_SUB - 1, C_WHITE, 2);
  } else if (WiFi.status() != WL_CONNECTED || !s_dataOk) {
    UI_TextCentered(s_status.c_str(), LY_SUB, C_YELLOW, 1);
  } else if (specialIdx >= 0 && emergIdx < 0) {
    const Aircraft& spAc = list[specialIdx];
    const char* cname = spAc.callsign[0] ? spAc.callsign : spAc.hex;
    snprintf(sub, sizeof(sub), "! %s: %s (%.0f km) !", specialLabel, cname, specialDistKm);
    UI_TextCentered(sub, LY_SUB, specialCol, 1);
  } else {
    if (closestIdx >= 0 && minDistKm <= range) {
      const char* cname = list[closestIdx].callsign[0] ? list[closestIdx].callsign : list[closestIdx].hex;
      snprintf(sub, sizeof(sub), "%s: %d  [%s: %.1f km]", T(S_AIRCRAFT), shown, cname, minDistKm);
    } else {
      snprintf(sub, sizeof(sub), "%s: %d%s", T(S_AIRCRAFT), shown,
               watchedSeen ? " *" : "");
    }
    UI_TextCentered(sub, LY_SUB, watchedSeen ? C_GREEN : C_CYAN, 1);
  }

  // --- Altitude legend ---
  // Five swatches in a continuous bar, with band boundaries (2 / 5 / 9 / 12 km).
  if (Settings_ShowLegends()) {
    const uint16_t cols[5] = {0x2FE6, 0xFDE0, 0x07FF, 0x3BDF, 0xF81F};
    const char*    bounds[4] = {"2", "5", "9", "12"};   // km boundaries
    const int sw = 22;                 // swatch width
    const int barW = 5 * sw;
    const int lx = R_CX - barW / 2;
    const int ly = LY_LEGEND;

    for (int i = 0; i < 5; i++) {
      gfx->fillRect(lx + i * sw, ly, sw, 6, cols[i]);
    }

    // Boundary numbers, centred on each internal edge of the bar.
    for (int i = 0; i < 4; i++) {
      int edge = lx + (i + 1) * sw;              // border between band i and i+1
      int tw = Layout_TextW(bounds[i], 1);
      UI_Text(bounds[i], edge - tw / 2, ly + 10, C_GRAY, 1);
    }
    // Unit, just past the right end of the bar.
    UI_Text("km", lx + barW + 4, ly + 10, C_GRAY, 1);
  }

  // --- Compass marks ---
  // Bearing b appears on screen at angle (b - rotation), 0 = up, clockwise.
  // --- Compass marks ---
  // Bearing b appears on screen at angle (b - rotation), 0 = up, clockwise.
  // Small size-1 letters at r = 205 slot in between the screen dots (y=18),
  // the status line (y=30), the aircraft count (y=52) and the range row.
  {
    const int   cr = 205;
    static const char* const LBL_EN[4] = { "N", "E", "S", "W" };
    static const char* const LBL_CZ[4] = { "S", "V", "J", "Z" };
    const char* const* lbl = (Lang_Get() == LANG_EN) ? LBL_EN : LBL_CZ;
    const int   brg[4] = { 0, 90, 180, 270 };
    for (int i = 0; i < 4; i++) {
      // Bearing b shows up at screen angle (b - topBearing).
      float a = (brg[i] - (int)s_topDeg) * 0.0174532925f;
      int cxp = R_CX + (int)(cr * sinf(a)) - 3;   // -3/-4 centres the glyph
      int cyp = R_CY - (int)(cr * cosf(a)) - 4;
      UI_Text(lbl[i], cxp, cyp, C_GRAY, 1);
    }
  }

  // Range indicator at the bottom
  char rbuf[16];
  snprintf(rbuf, sizeof(rbuf), "%.0f km", range);
  UI_DrawRangeIndicator(rbuf, s_rangeIdx, RANGE_COUNT, true);

  // --- Detail of the selected aircraft (overlay) ---
  // selIdx was resolved from the hex at the top of the frame, so this always
  // shows the aircraft that was actually tapped, with values refreshed from the
  // latest fetch (altitude, speed and climb rate update live while it is open).
  // Keep a copy of the selected aircraft so the panel can still be drawn while
  // the aircraft is temporarily missing from the data (grace period).
  if (selIdx >= 0 && selIdx < n) { s_selCache = list[selIdx]; s_selCacheOk = true; }
  const bool signalLost = (selIdx < 0) && s_selCacheOk;

  if (ScreenPlanes_DetailOpen() && s_selCacheOk) {
    Route_Select(s_selCache.callsign, s_selCache.lat, s_selCache.lon);
    PlanePhoto_Select(s_selCache.reg, s_selCache.hex);
    const RouteInfo* rt = Route_Get();
    UI_DrawAircraftDetail(s_selCache, rt, Route_GetState(), signalLost);
  }
}
