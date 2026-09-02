// =============================================================================
//  MeteoPlaneRadar
//  Screen 3: Tactical / Combined radar (live aircraft + live weather radar).
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenTactical.h"
#include "AsyncCore.h"
#include "PlaneTrail.h"
#include "ADSB.h"
#include "CHMU.h"
#include "RainViewer.h"
#include "Settings.h"
#include "Config.h"
#include "Route.h"
#include "EuBorder.h"
#include "Airports.h"
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
static unsigned long s_lastRadarFetch = 0;
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

static double s_tacViewLat = 0.0, s_tacViewLon = 0.0;
static float  s_tacViewRng = 50.0f;
static void cityProject(float lat, float lon, int* sx, int* sy) {
  project(lat, lon, s_tacViewLat, s_tacViewLon, s_tacViewRng, sx, sy);
}

static uint16_t altColor(float altFt, bool known) {
  return PlaneTrail_AltColor(altFt, known);
}

static void drawPlane(int x, int y, float trackDeg, bool hasTrack, uint16_t col, bool isMilitary = false) {
  if (!hasTrack) {
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
    int cx, cy;
    rot(0, 2, &cx, &cy);
    gfx->fillCircle(cx, cy, 1, C_WHITE);
    return;
  }

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
  const Aircraft* list = ADSB_List();
  int n = ADSB_Count();
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

  double clat = Settings_Lat(), clon = Settings_Lon();
  float crng = currentRange();
  if (emergIdx >= 0) {
    clat = (double)list[emergIdx].lat;
    clon = (double)list[emergIdx].lon;
  } else if (isWholeCountry()) {
    getWholeCountryView(&clat, &clon, &crng, nullptr);
  }
  s_tacViewLat = clat;
  s_tacViewLon = clon;
  s_tacViewRng = crng;

  float dLat = (crng * 1.05f) / 111.0f;
  float dLon = dLat / cosf(clat * 0.0174532925f);
  EuBorder_Draw(cityProject, C_GRAY, clat - dLat, clat + dLat, clon - dLon, clon + dLon);

  bool showFull = (!isWholeCountry() && crng <= 50.0f);
  uint8_t maxTier = (isWholeCountry() || crng > 100.0f) ? 1 : 2;
  EuBorder_DrawCities(cityProject, R_CX, R_CY, R_RADIUS, C_WHITE, C_CYAN,
                      showFull, maxTier, clat - dLat, clat + dLat, clon - dLon, clon + dLon);

  // Airports as underlay
  if (Settings_RadarShowAirports()) {
    Airports_Draw(cityProject, R_CX, R_CY, R_RADIUS, crng,
                  clat - dLat, clat + dLat, clon - dLon, clon + dLon);
  }

  // 3. Range rings & Home Base waypoint beacon
  if (Settings_RadarShowRings()) {
    gfx->drawCircle(R_CX, R_CY, R_RADIUS, C_DKGRAY);
    gfx->drawCircle(R_CX, R_CY, R_RADIUS / 2, C_DKGRAY);
  }
  if (emergIdx < 0 && !isWholeCountry()) {
    gfx->drawCircle(R_CX, R_CY, 8, C_CYAN);
    gfx->drawCircle(R_CX, R_CY, 4, C_YELLOW);
    gfx->fillCircle(R_CX, R_CY, 2, C_WHITE);
    gfx->drawFastHLine(R_CX - 12, R_CY, 24, C_DKGRAY);
    gfx->drawFastVLine(R_CX, R_CY - 12, 24, C_DKGRAY);
  } else if (emergIdx >= 0) {
    int hx, hy;
    project(Settings_Lat(), Settings_Lon(), clat, clon, crng, &hx, &hy);
    if ((long)(hx - R_CX) * (hx - R_CX) + (long)(hy - R_CY) * (hy - R_CY) <= (long)R_RADIUS * R_RADIUS) {
      gfx->drawCircle(hx, hy, 6, C_CYAN);
      gfx->fillCircle(hx, hy, 2, C_WHITE);
    }
  }

  // 4. Aircraft overlay
  int selIdx = ADSB_FindByHex(s_selectedHex);
  bool watchedSeen = false;
  int drawnCount = 0;

  int closestIdx = -1;
  float minDistKm = 999999.0f;

  for (int i = 0; i < n; i++) {
    s_planeX[i] = -9999; s_planeY[i] = -9999;
    s_planeHex[i][0] = '\0';
    if (list[i].onGround) continue;

    // In Emergency mode: isolate and show ONLY the emergency aircraft!
    if (emergIdx >= 0 && i != emergIdx) continue;

    // 3D Proximity distance
    double dLatKm = (list[i].lat - clat) * 111.0;
    double dLonKm = (list[i].lon - clon) * (111.0 * cos(clat * 0.0174532925));
    float dGnd = sqrtf((float)(dLatKm * dLatKm + dLonKm * dLonKm));
    float dAlt = (list[i].altFt * 0.3048f) / 1000.0f;
    float d3D = sqrtf(dGnd * dGnd + dAlt * dAlt);
    if (d3D < minDistKm) {
      minDistKm = d3D;
      closestIdx = i;
    }

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
    if (em) {
      gfx->drawCircle(sx, sy, 20, C_RED);
      gfx->drawCircle(sx, sy, 21, C_RED);
      gfx->drawCircle(sx, sy, 22, C_WHITE);
    } else if (watched) {
      gfx->drawCircle(sx, sy, 20, C_GREEN);
      gfx->drawCircle(sx, sy, 21, C_GREEN);
    }

    bool altKnown = (list[i].altFt > 0.0f);
    bool isMil = list[i].isMilitary;
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
            gfx->setTextSize(c.size);
            gfx->setTextColor(lCol);
            gfx->setCursor(tx, ty);
            gfx->print(label);

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
                    gfx->setTextSize(1);
                    gfx->setTextColor(C_YELLOW);
                    gfx->setCursor(rx, ry);
                    gfx->print(rl);
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
    drawnCount++;
  }

  // Draw Proximity Vector to nearest aircraft
  if (Settings_RadarShowNearest() && closestIdx >= 0 && s_planeX[closestIdx] > -9000 && !ScreenTactical_DetailOpen() && minDistKm <= crng) {
    int cx = s_planeX[closestIdx], cy = s_planeY[closestIdx];
    int steps = 10;
    for (int s = 2; s < steps; s += 2) {
      int px = R_CX + (cx - R_CX) * s / steps;
      int py = R_CY + (cy - R_CY) * s / steps;
      gfx->drawPixel(px, py, C_CYAN);
    }
    gfx->drawCircle(cx, cy, 14, C_CYAN);
  }

  // 5. Header status
  UI_DrawStatusLine(LY_STATUS);

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
    gfx->setTextSize(2);
    gfx->setTextColor(C_YELLOW);
    gfx->setCursor(boxX + 12, boxY + 6);
    gfx->print(l1);

    char l2[48];
    snprintf(l2, sizeof(l2), "ALT: %.0f ft (%.0f m)   GS: %.0f km/h",
             emAc.altFt, emAc.altFt * 0.3048f, emAc.gsKt * 1.852f);
    gfx->setTextSize(1);
    gfx->setTextColor(C_WHITE);
    gfx->setCursor(boxX + 12, boxY + 30);
    gfx->print(l2);

    char l3[48];
    if (emAc.baroRate < -300.0f) {
      snprintf(l3, sizeof(l3), "KLESANIE:  v %.0f ft/min (%.1f m/s)", emAc.baroRate, emAc.baroRate * 0.00508f);
      gfx->setTextColor(C_RED);
    } else if (emAc.baroRate > 300.0f) {
      snprintf(l3, sizeof(l3), "STUPANIE:  ^ +%.0f ft/min (%.1f m/s)", emAc.baroRate, emAc.baroRate * 0.00508f);
      gfx->setTextColor(C_GREEN);
    } else {
      snprintf(l3, sizeof(l3), "ROVNY LET: 0 ft/min   HDG: %.0f deg", emAc.track);
      gfx->setTextColor(C_CYAN);
    }
    gfx->setTextSize(1);
    gfx->setCursor(boxX + 12, boxY + 48);
    gfx->print(l3);
  } else if (alertCode) {
    const char* what = (strcmp(alertCode, SQUAWK_HIJACK) == 0) ? T(S_HIJACK)
                     : (strcmp(alertCode, SQUAWK_RADIO)  == 0) ? T(S_RADIO_FAIL)
                                                               : T(S_EMERGENCY);
    snprintf(sub, sizeof(sub), "%s  %s", alertCode, what);
    int tw = Layout_TextW(sub, 2);
    gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 6, LY_SUB - 3, tw + 12, 20, C_RED);
    UI_TextCentered(sub, LY_SUB, C_WHITE, 2);
  } else {
    if (closestIdx >= 0 && minDistKm <= crng) {
      const char* cname = list[closestIdx].callsign[0] ? list[closestIdx].callsign : list[closestIdx].hex;
      snprintf(sub, sizeof(sub), "%s: %d  [%s: %.1f km]", T(S_TACTICAL), drawnCount, cname, minDistKm);
    } else {
      snprintf(sub, sizeof(sub), "%s: %d", T(S_TACTICAL), drawnCount);
    }
    int tw = Layout_TextW(sub, 1);
    gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 6, LY_SUB - 2, tw + 12, 12, C_BLACK);
    UI_TextCentered(sub, LY_SUB, C_CYAN, 1);
  }

  // 6. Range indicator at bottom
  if (Settings_ShowLegends() && emergIdx < 0) {
    char rbuf[32];
    if (isWholeCountry()) {
      const char* lbl = nullptr;
      getWholeCountryView(nullptr, nullptr, nullptr, &lbl);
      snprintf(rbuf, sizeof(rbuf), "%s", lbl ? lbl : T(S_WHOLE_COUNTRY));
    } else {
      snprintf(rbuf, sizeof(rbuf), "%.0f km", crng);
    }
    int rw = Layout_TextW(rbuf, 2);
    gfx->fillRect(LCD_WIDTH / 2 - rw / 2 - 6, LY_RANGE - 3, rw + 12, 22, C_BLACK);
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

    UI_Text(ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?"),
            px + 18, py + 16, C_YELLOW, 3);

    char line[44];
    int ty = py + 56;

    if (metric) snprintf(line, sizeof(line), "%s: %.0f m", T(S_ALTITUDE), ac.altFt * 0.3048f);
    else        snprintf(line, sizeof(line), "%s: %.0f ft", T(S_ALTITUDE), ac.altFt);
    UI_Text(line, px + 18, ty, C_WHITE, 2); ty += 26;

    if (metric) snprintf(line, sizeof(line), "%s: %.0f km/h", T(S_SPEED), ac.gsKt * 1.852f);
    else        snprintf(line, sizeof(line), "%s: %.0f kt", T(S_SPEED), ac.gsKt);
    UI_Text(line, px + 18, ty, C_WHITE, 2); ty += 26;

    if (ac.hasTrack) snprintf(line, sizeof(line), "%s: %.0f deg", T(S_TRACK), ac.track);
    else             snprintf(line, sizeof(line), "%s: %s", T(S_TRACK), T(S_UNKNOWN));
    UI_Text(line, px + 18, ty, C_WHITE, 2); ty += 26;

    const char* ar = ac.baroRate > 100 ? "^" : (ac.baroRate < -100 ? "v" : "-");
    if (metric) snprintf(line, sizeof(line), "%s: %.1f m/s %s", T(S_CLIMB), ac.baroRate * 0.00508f, ar);
    else        snprintf(line, sizeof(line), "%s: %.0f ft/m %s", T(S_CLIMB), ac.baroRate, ar);
    UI_Text(line, px + 18, ty, C_WHITE, 2); ty += 26;

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
      UI_Text(line, px + 18, ty, C_WHITE, 2);
      ty += 26;
    }

    if (rt && (rt->from[0] || rt->to[0])) {
      snprintf(line, sizeof(line), "%s", rt->from);
      UI_Text(line, px + 18, ty, C_YELLOW, 2);
      ty += 22;
      snprintf(line, sizeof(line), "-> %s", rt->to);
      UI_Text(line, px + 18, ty, C_YELLOW, 2);
    }
  }
}

void ScreenTactical_Enter() {
  selectNone("enter");
  double clat = Settings_Lat(), clon = Settings_Lon();
  float crng = currentRange();
  if (isWholeCountry()) {
    getWholeCountryView(&clat, &clon, &crng, nullptr);
  }
  Async_SetActiveScreen(SCREEN_TACTICAL_I);
  Async_SetAdsbTarget(clat, clon, crng);
  Async_RequestAdsb();
  if (rvMode()) {
    if (isWholeCountry()) {
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
  double clat = Settings_Lat(), clon = Settings_Lon();
  float crng = currentRange();
  if (isWholeCountry()) {
    getWholeCountryView(&clat, &clon, &crng, nullptr);
  }
  Async_SetAdsbTarget(clat, clon, crng);
  Async_RequestAdsb();
  if (rvMode()) {
    if (isWholeCountry()) {
      RainViewer_Begin(clat, clon, -crng, 1);
    } else {
      RainViewer_Begin(clat, clon, crng, 1);
    }
  }
}

bool ScreenTactical_Tick() {
  if (WiFi.status() != WL_CONNECTED) return false;

  bool routeChanged = Async_TakeRouteUpdated() || Route_TakeChanged();
  bool adsbChanged  = Async_TakeAdsbUpdated();
  bool radarChanged = Async_TakeRadarUpdated();

  if (adsbChanged) {
    s_dataOk = (ADSB_Count() > 0);
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

  return routeChanged || radarChanged;
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
