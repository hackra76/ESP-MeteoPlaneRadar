// =============================================================================
//  MeteoPlaneRadar
//  Screen 3: Tactical / Combined radar (live aircraft + live weather radar).
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenTactical.h"
#include "ADSB.h"
#include "CHMU.h"
#include "RainViewer.h"
#include "Settings.h"
#include "Config.h"
#include "Route.h"
#include "EuBorder.h"
#include "UI.h"
#include "Layout.h"
#include "Lang.h"
#include "Status.h"
#include "Display_ST7701.h"

#include <WiFi.h>
#include <math.h>
#include <string.h>

#define R_CX (LCD_WIDTH / 2)
#define R_CY (LCD_HEIGHT / 2)
#define R_RADIUS 230
#define DISP_R (LCD_WIDTH / 2 - 2)

// Ranges for tactical combined radar (km): 25, 50, 100, 200 km, 0 = Whole Country
static const float RANGES_KM[] = TACTICAL_RANGES_KM;
static const int   RANGE_COUNT = sizeof(RANGES_KM) / sizeof(RANGES_KM[0]);
static int s_rangeIdx = 1; // 50 km default

static float currentRange() { return RANGES_KM[s_rangeIdx]; }
static bool  isWholeCountry() { return currentRange() <= 0.0f; }

static unsigned long s_nextFetch = 0;
static bool  s_dataOk = false;
static String s_status = "...";

static char  s_selectedHex[8] = "";
static int   s_selMiss = 0;
static Aircraft s_selCache;
static bool  s_selCacheOk = false;

static int   s_planeX[ADSB_MAX];
static int   s_planeY[ADSB_MAX];
static char  s_planeHex[ADSB_MAX][8];
static int   s_planeN = 0;

static float s_rotSin = 0.0f, s_rotCos = 1.0f;
static uint16_t s_topDeg = 0;

static bool rvMode() { return Settings_RadarSource() == RADAR_SRC_RAINVIEWER; }

bool ScreenTactical_DetailOpen() { return s_selectedHex[0] != '\0'; }

static void selectNone(const char* reason) {
  (void)reason;
  s_selectedHex[0] = '\0';
  s_selMiss = 0;
  s_selCacheOk = false;
  Route_Clear();
}

static void selectHex(const char* hex) {
  strncpy(s_selectedHex, hex, sizeof(s_selectedHex) - 1);
  s_selectedHex[sizeof(s_selectedHex) - 1] = '\0';
  s_selMiss = 0;
  s_selCacheOk = false;
}

void ScreenTactical_CloseDetail() { selectNone("manual"); }

static void refreshRotation() {
  uint16_t deg = Settings_TopBearing();
  if (deg == s_topDeg) return;
  s_topDeg = deg;
  float r = (float)deg * 0.0174532925f;
  s_rotSin = sinf(r);
  s_rotCos = cosf(r);
}

// Compute whole country center and radius
static void getWholeCountryView(double* outLat, double* outLon, float* outRadius, const char** outLabel) {
  double ulat = Settings_Lat(), ulon = Settings_Lon();
  bool inSlovakia = (ulat >= 47.7 && ulat <= 49.7 && ulon >= 16.8 && ulon <= 22.6);
  if (inSlovakia || Settings_Language() == LANG_SK) {
    if (outLat) *outLat = SK_VIEW_LAT;
    if (outLon) *outLon = SK_VIEW_LON;
    if (outRadius) *outRadius = SK_VIEW_RADIUS_KM;
    if (outLabel) *outLabel = T(S_WHOLE_SK);
  } else {
    if (outLat) *outLat = 49.800;
    if (outLon) *outLon = 15.500;
    if (outRadius) *outRadius = 240.0f;
    if (outLabel) *outLabel = T(S_WHOLE_CZ);
  }
}

// Map projection: lat/lon to display coordinates
static void project(float lat, float lon, double clat, double clon,
                    float rangeKm, int* sx, int* sy) {
  float latr = clat * 0.0174532925f;
  float dxKm = (lon - clon) * 111.0f * cosf(latr);
  float dyKm = (lat - clat) * 111.0f;
  float rx = dxKm * s_rotCos - dyKm * s_rotSin;
  float ry = dxKm * s_rotSin + dyKm * s_rotCos;
  float scale = (float)R_RADIUS / rangeKm;
  *sx = R_CX + (int)(rx * scale);
  *sy = R_CY - (int)(ry * scale);
}

static void cityProject(float lat, float lon, int* sx, int* sy) {
  double clat = Settings_Lat(), clon = Settings_Lon();
  float crng = currentRange();
  if (isWholeCountry()) {
    getWholeCountryView(&clat, &clon, &crng, nullptr);
  }
  project(lat, lon, clat, clon, crng, sx, sy);
}

static uint16_t altColor(float altFt, bool known) {
  if (!known) return C_GRAY;
  float km = altFt * 0.0003048f;
  if (km <  2.0f) return C_RED;
  if (km <  6.0f) return C_ORANGE;
  if (km < 10.0f) return C_YELLOW;
  return C_BLUE;
}

static void drawPlane(int x, int y, float trackDeg, bool hasTrack, uint16_t col) {
  if (!hasTrack) {
    gfx->drawCircle(x, y, 7, col);
    gfx->fillCircle(x, y, 2, col);
    return;
  }
  float a = trackDeg * 0.0174532925f;
  float ca = cosf(a), sa = sinf(a);
  auto rot = [&](float right, float fwd, int* ox, int* oy) {
    *ox = x + (int)(right * ca + fwd * sa);
    *oy = y + (int)(right * sa - fwd * ca);
  };
  const float P[10][2] = {
    { 0,  12}, { 3,  1}, { 13, -8}, { 3, -5}, { 3, -7},
    { 0, -12}, {-3, -7}, {-3, -5}, {-13, -8}, {-3,  1}
  };
  int px[10], py[10];
  for (int i = 0; i < 10; i++) rot(P[i][0], P[i][1], &px[i], &py[i]);

  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    gfx->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
  }
}

static bool isWatched(const Aircraft& ac) {
  const char* w = Settings_WatchCallsign();
  if (!w || !w[0]) return false;
  if (ac.callsign[0] && strcasecmp(ac.callsign, w) == 0) return true;
  if (ac.hex[0] && strcasecmp(ac.hex, w) == 0) return true;
  return false;
}

static bool passesFilter(const Aircraft& ac) {
  if (Settings_OnlyWithCallsign() && !ac.callsign[0]) return false;
  if (ac.altFt > 0.0f) {
    if (ac.altFt < (float)Settings_AltMinFt()) return false;
    if (ac.altFt > (float)Settings_AltMaxFt()) return false;
  }
  return true;
}

static void blitRainViewer(const uint16_t* fb) {
  if (!fb) return;
  const long R2 = (long)DISP_R * DISP_R;
  for (int dy = 0; dy < LCD_HEIGHT; dy++) {
    long ddy = dy - R_CY;
    long room = R2 - ddy * ddy;
    if (room <= 0) continue;
    int half = (int)sqrtf((float)room);
    int x0 = R_CX - half, x1 = R_CX + half;
    if (x0 < 0) x0 = 0;
    if (x1 >= LCD_WIDTH) x1 = LCD_WIDTH - 1;
    const uint16_t* src = fb + (int32_t)dy * LCD_WIDTH;
    for (int dx = x0; dx <= x1; dx++) {
      uint16_t c = src[dx];
      if (c != 0x0000) {
        gfx->drawPixel(dx, dy, c);
      }
    }
  }
}

void ScreenTactical_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  refreshRotation();

  // 1. Weather background
  if (rvMode()) {
    int frameCount = RainViewer_Count();
    if (frameCount > 0) {
      blitRainViewer(RainViewer_Frame(frameCount - 1));
    }
  }

  // 2. Borders & Cities
  double clat = Settings_Lat(), clon = Settings_Lon();
  float crng = currentRange();
  if (isWholeCountry()) {
    getWholeCountryView(&clat, &clon, &crng, nullptr);
  }
  float dLat = (crng * 1.05f) / 111.0f;
  float dLon = dLat / cosf(clat * 0.0174532925f);
  EuBorder_Draw(cityProject, C_GRAY, clat - dLat, clat + dLat, clon - dLon, clon + dLon);

  bool showFull = (!isWholeCountry() && crng <= 50.0f);
  uint8_t maxTier = (isWholeCountry() || crng > 100.0f) ? 1 : 2;
  EuBorder_DrawCities(cityProject, R_CX, R_CY, R_RADIUS, C_WHITE, C_CYAN,
                      showFull, maxTier, clat - dLat, clat + dLat, clon - dLon, clon + dLon);

  // 3. Range rings & crosshair
  gfx->drawCircle(R_CX, R_CY, R_RADIUS, C_DKGRAY);
  gfx->drawCircle(R_CX, R_CY, R_RADIUS / 2, C_DKGRAY);
  gfx->drawFastHLine(R_CX - 8, R_CY, 16, C_WHITE);
  gfx->drawFastVLine(R_CX, R_CY - 8, 16, C_WHITE);

  // 4. Aircraft overlay
  const Aircraft* list = ADSB_List();
  int n = ADSB_Count();
  s_planeN = n;
  int selIdx = ADSB_FindByHex(s_selectedHex);

  const char* alertCode = nullptr;
  bool watchedSeen = false;
  int drawnCount = 0;

  for (int i = 0; i < n; i++) {
    s_planeX[i] = -9999; s_planeY[i] = -9999;
    s_planeHex[i][0] = '\0';
    if (list[i].onGround) continue;

    const char* em = Settings_SquawkAlert() ? ADSB_EmergencyCode(list[i]) : nullptr;
    const bool watched = isWatched(list[i]);
    if (em && !alertCode) alertCode = em;
    if (watched) watchedSeen = true;

    if (!em && !watched && !passesFilter(list[i])) continue;
    int sx, sy;
    project(list[i].lat, list[i].lon, clat, clon, crng, &sx, &sy);
    int dx = sx - R_CX, dy = sy - R_CY;
    if (dx * dx + dy * dy > R_RADIUS * R_RADIUS) continue;

    s_planeX[i] = sx; s_planeY[i] = sy;
    strncpy(s_planeHex[i], list[i].hex, sizeof(s_planeHex[i]) - 1);
    s_planeHex[i][sizeof(s_planeHex[i]) - 1] = '\0';

    if (i == selIdx) gfx->drawCircle(sx, sy, 16, C_WHITE);
    if (em)      { gfx->drawCircle(sx, sy, 20, C_RED);   gfx->drawCircle(sx, sy, 21, C_RED); }
    else if (watched) { gfx->drawCircle(sx, sy, 20, C_GREEN); gfx->drawCircle(sx, sy, 21, C_GREEN); }

    bool altKnown = (list[i].altFt > 0.0f);
    float screenTrack = list[i].track - (float)s_topDeg;
    while (screenTrack < 0.0f) screenTrack += 360.0f;
    drawPlane(sx, sy, screenTrack, list[i].hasTrack, altColor(list[i].altFt, altKnown));

    const char* label = list[i].callsign[0] ? list[i].callsign : list[i].hex;
    if (label[0]) {
      int tw = Layout_TextW(label, 2);
      int tx = sx - tw / 2, ty = sy + 22;
      if (Layout_Claim(tx - 2, ty - 2, tw + 4, 20)) {
        gfx->setTextSize(2);
        gfx->setTextColor(em ? C_RED : (watched ? C_GREEN : C_WHITE));
        gfx->setCursor(tx, ty);
        gfx->print(label);
      }
    }
    drawnCount++;
  }

  // 5. Header status
  UI_DrawStatusLine(LY_STATUS);

  char sub[40];
  if (alertCode) {
    const char* what = (strcmp(alertCode, SQUAWK_HIJACK) == 0) ? T(S_HIJACK)
                     : (strcmp(alertCode, SQUAWK_RADIO)  == 0) ? T(S_RADIO_FAIL)
                                                               : T(S_EMERGENCY);
    snprintf(sub, sizeof(sub), "%s  %s", alertCode, what);
    int tw = Layout_TextW(sub, 2);
    gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 6, LY_SUB - 4, tw + 12, 20, C_RED);
    UI_TextCentered(sub, LY_SUB, C_WHITE, 2);
  } else {
    snprintf(sub, sizeof(sub), "%s (%d)", T(S_TACTICAL), drawnCount);
    UI_TextCentered(sub, LY_SUB, C_CYAN, 2);
  }

  // 6. Range indicator at bottom
  char rbuf[32];
  if (isWholeCountry()) {
    const char* lbl = nullptr;
    getWholeCountryView(nullptr, nullptr, nullptr, &lbl);
    snprintf(rbuf, sizeof(rbuf), "%s", lbl ? lbl : T(S_WHOLE_COUNTRY));
  } else {
    snprintf(rbuf, sizeof(rbuf), "%.0f km", crng);
  }
  UI_TextCentered(rbuf, LY_RANGE, C_YELLOW, 2);

  int dotGap = 20;
  int totalW = (RANGE_COUNT - 1) * dotGap;
  int startX = R_CX - totalW / 2;
  int dotY = LY_RANGE_DOTS;
  for (int i = 0; i < RANGE_COUNT; i++) {
    int x = startX + i * dotGap;
    if (i == s_rangeIdx) gfx->fillCircle(x, dotY, 4, C_YELLOW);
    else                 gfx->drawCircle(x, dotY, 4, C_GRAY);
  }

  // 7. Aircraft detail panel if selected
  if (selIdx >= 0 && selIdx < n) { s_selCache = list[selIdx]; s_selCacheOk = true; }

  if (ScreenTactical_DetailOpen() && s_selCacheOk) {
    const Aircraft& ac = s_selCache;
    const bool metric = Settings_MetricUnits();

    const int pw = 320, ph = 260;
    const int px = R_CX - pw / 2;
    const int py = R_CY - ph / 2;
    gfx->fillRoundRect(px, py, pw, ph, 14, C_DKGRAY);
    gfx->drawRoundRect(px, py, pw, ph, 14, C_CYAN);

    int cxx = px + pw - 24, cyy = py + 22;
    gfx->fillCircle(cxx, cyy, 15, C_RED);
    gfx->drawLine(cxx - 6, cyy - 6, cxx + 6, cyy + 6, C_WHITE);
    gfx->drawLine(cxx - 6, cyy + 6, cxx + 6, cyy - 6, C_WHITE);

    gfx->setTextSize(3); gfx->setTextColor(C_YELLOW);
    gfx->setCursor(px + 18, py + 16);
    gfx->print(ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?"));

    char line[44];
    int ty = py + 56;
    gfx->setTextSize(2); gfx->setTextColor(C_WHITE);

    if (metric) snprintf(line, sizeof(line), "%s: %.0f m", T(S_ALTITUDE), ac.altFt * 0.3048f);
    else        snprintf(line, sizeof(line), "%s: %.0f ft", T(S_ALTITUDE), ac.altFt);
    gfx->setCursor(px + 18, ty); gfx->print(line); ty += 26;

    if (metric) snprintf(line, sizeof(line), "%s: %.0f km/h", T(S_SPEED), ac.gsKt * 1.852f);
    else        snprintf(line, sizeof(line), "%s: %.0f kt", T(S_SPEED), ac.gsKt);
    gfx->setCursor(px + 18, ty); gfx->print(line); ty += 26;

    if (ac.hasTrack) snprintf(line, sizeof(line), "%s: %.0f deg", T(S_TRACK), ac.track);
    else             snprintf(line, sizeof(line), "%s: %s", T(S_TRACK), T(S_UNKNOWN));
    gfx->setCursor(px + 18, ty); gfx->print(line); ty += 26;

    const char* ar = ac.baroRate > 100 ? "^" : (ac.baroRate < -100 ? "v" : "-");
    if (metric) snprintf(line, sizeof(line), "%s: %.1f m/s %s", T(S_CLIMB), ac.baroRate * 0.00508f, ar);
    else        snprintf(line, sizeof(line), "%s: %.0f ft/m %s", T(S_CLIMB), ac.baroRate, ar);
    gfx->setCursor(px + 18, ty); gfx->print(line); ty += 26;

    Route_Select(ac.callsign, ac.lat, ac.lon);
    const RouteInfo* rt = Route_Get();

    const char* typ = ac.type[0] ? ac.type : nullptr;
    if (typ || ac.reg[0]) {
      snprintf(line, sizeof(line), "%s: %s%s%s", T(S_TYPE),
               typ ? typ : "?",
               ac.reg[0] ? "  " : "",
               ac.reg[0] ? ac.reg : "");
      int maxCh = (pw - 36) / 12;
      if ((int)strlen(line) > maxCh) { line[maxCh - 1] = '.'; line[maxCh] = '\0'; }
      gfx->setCursor(px + 18, ty); gfx->print(line);
      ty += 26;
    }

    if (rt && (rt->from[0] || rt->to[0])) {
      gfx->setTextColor(C_YELLOW);
      snprintf(line, sizeof(line), "%s", rt->from);
      gfx->setCursor(px + 18, ty); gfx->print(line);
      ty += 22;
      snprintf(line, sizeof(line), "-> %s", rt->to);
      gfx->setCursor(px + 18, ty); gfx->print(line);
    }
  }
}

void ScreenTactical_Enter() {
  selectNone("enter");
  s_nextFetch = 0;
  if (rvMode()) {
    double clat = Settings_Lat(), clon = Settings_Lon();
    float crng = currentRange();
    if (isWholeCountry()) {
      getWholeCountryView(&clat, &clon, &crng, nullptr);
      RainViewer_Begin(clat, clon, -crng, 1);
    } else {
      RainViewer_Begin(clat, clon, crng, 1);
    }
  }
}

void ScreenTactical_RangeText(char* out, size_t cap) {
  if (!out || !cap) return;
  if (isWholeCountry()) {
    const char* lbl = nullptr;
    getWholeCountryView(nullptr, nullptr, nullptr, &lbl);
    snprintf(out, cap, "%s", lbl ? lbl : T(S_WHOLE_COUNTRY));
  } else {
    snprintf(out, cap, "%.0f %s", currentRange(), T(S_KM));
  }
}

void ScreenTactical_ChangeRange(int dir) {
  if (ScreenTactical_DetailOpen()) return;
  s_rangeIdx = (s_rangeIdx + dir + RANGE_COUNT) % RANGE_COUNT;
  selectNone("range_change");
  if (rvMode()) {
    double clat = Settings_Lat(), clon = Settings_Lon();
    float crng = currentRange();
    if (isWholeCountry()) {
      getWholeCountryView(&clat, &clon, &crng, nullptr);
      RainViewer_Begin(clat, clon, -crng, 1);
    } else {
      RainViewer_Begin(clat, clon, crng, 1);
    }
  }
  s_nextFetch = 0;
}

bool ScreenTactical_Tick() {
  if (WiFi.status() != WL_CONNECTED) return false;
  unsigned long now = millis();

  // Tick RainViewer background
  if (rvMode() && RainViewer_Busy()) {
    RainViewer_Step();
  }

  Route_Tick();
  bool routeChanged = Route_TakeChanged();

  if (now >= s_nextFetch) {
    s_nextFetch = now + 5000;
    double clat = Settings_Lat(), clon = Settings_Lon();
    float crng = currentRange();
    if (isWholeCountry()) {
      getWholeCountryView(&clat, &clon, &crng, nullptr);
    }
    s_dataOk = ADSB_Fetch(clat, clon, crng);
    s_status = s_dataOk ? "OK" : "ERR";

    if (ScreenTactical_DetailOpen()) {
      if (ADSB_FindByHex(s_selectedHex) >= 0) {
        s_selMiss = 0;
      } else if (++s_selMiss > DETAIL_GRACE_POLLS) {
        selectNone("plane lost");
      }
    }
    return true;
  }

  return routeChanged;
}

bool ScreenTactical_HandleTap(int x, int y) {
  if (ScreenTactical_DetailOpen()) {
    selectNone("tap_outside");
    return true;
  }

  int best = -1;
  long bestD = 30L * 30L;
  for (int i = 0; i < s_planeN; i++) {
    long dx = s_planeX[i] - x, dy = s_planeY[i] - y;
    long d = dx * dx + dy * dy;
    if (d < bestD) { bestD = d; best = i; }
  }

  if (best >= 0 && s_planeHex[best][0]) {
    selectHex(s_planeHex[best]);
    return true;
  }

  return false;
}
