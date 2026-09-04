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
#include "SHMU.h"
#include "RainViewer.h"
#include <PNGdec.h>
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

static bool rvMode()   { return Settings_RadarSource() == RADAR_SRC_RAINVIEWER; }
static bool shmuMode() { return Settings_RadarSource() == RADAR_SRC_SHMU; }
static bool chmuMode() { return Settings_RadarSource() == RADAR_SRC_CHMU; }
static inline int clampI(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool ScreenTactical_DetailOpen() { return s_selectedHex[0] != '\0'; }

static void selectNone(const char* reason) {
  (void)reason;
  s_selectedHex[0] = '\0';
  s_selMiss = 0;
  s_selCacheOk = false;
  UI_SetPhotoFullscreen(false);
  Route_Clear();
  PlanePhoto_Clear();
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

// --- Composite Radar Background (CHMU / SHMU) for Tactical Screen ---
static uint16_t* s_radarFb = nullptr;
static uint16_t* s_crop565 = nullptr;
static int       s_cropCap = 0;
static uint16_t* s_lineBuf = nullptr;
static int       s_lineCap = 0;
static PNG*      s_png = nullptr;

struct TacticalCrop {
  int x1, y1, x2, y2;
  int cw, ch;
  int imgW, imgH;
  int dataX1, dataY0;
  bool isShmu;
};
static TacticalCrop s_tc;

static float tacRadarLonLeft(bool isShmu)   { return isShmu ? SHMU_LON_LEFT : CHMU_LON_LEFT; }
static float tacRadarLonRight(bool isShmu)  { return isShmu ? SHMU_LON_RIGHT : CHMU_LON_RIGHT; }
static float tacRadarLatTop(bool isShmu)    { return isShmu ? SHMU_LAT_TOP : CHMU_LAT_TOP; }
static float tacRadarLatBottom(bool isShmu) { return isShmu ? SHMU_LAT_BOTTOM : CHMU_LAT_BOTTOM; }

static float tacMercY(float latDeg) {
  float r = latDeg * 0.0174532925f;
  return logf(tanf(0.7853981634f + r * 0.5f));
}

static int tacLonToX(float lon, bool isShmu, int imgW) {
  float left = tacRadarLonLeft(isShmu), right = tacRadarLonRight(isShmu);
  return lroundf((lon - left) * (imgW - 1) / (right - left));
}

static int tacLatToY(float lat, bool isShmu, int imgH) {
  float yt = tacMercY(tacRadarLatTop(isShmu)), yb = tacMercY(tacRadarLatBottom(isShmu));
  return lroundf((yt - tacMercY(lat)) * (imgH - 1) / (yt - yb));
}

static int pngDrawTactical(PNGDRAW* d) {
  int srcY = d->y;
  if (srcY < s_tc.y1 || srcY > s_tc.y2) return 1;
  if (!s_crop565 || !s_lineBuf || !s_png) return 1;
  s_png->getLineAsRGB565(d, s_lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  const int cw = s_tc.cw;
  uint16_t* row = s_crop565 + (int64_t)(srcY - s_tc.y1) * cw;
  const bool isShmu = s_tc.isShmu;
  for (int i = 0; i < cw; i++) {
    int srcX = s_tc.x1 + i;
    bool have = (srcX >= 0 && srcX < s_tc.imgW && srcX <= s_tc.dataX1 && srcY >= s_tc.dataY0);
    uint16_t col = have ? s_lineBuf[srcX] : 0x0000;
    if (isShmu) {
      if (col == 0xE71C || col == 0xD6DA || col == 0xE73C || col == 0xD69A || col == 0xFFFF) {
        col = 0x0000;
      }
    }
    row[i] = col;
  }
  return 1;
}

static bool ensureTacticalRadarBuffers(int needCropPx) {
  if (!s_png) {
    s_png = (PNG*)heap_caps_malloc(sizeof(PNG), MALLOC_CAP_SPIRAM);
    if (!s_png) s_png = (PNG*)malloc(sizeof(PNG));
  }
  if (!s_radarFb) {
    s_radarFb = (uint16_t*)heap_caps_malloc((size_t)LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (s_radarFb) memset(s_radarFb, 0, (size_t)LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
  }
  if (needCropPx > 0 && (!s_crop565 || s_cropCap < needCropPx)) {
    if (s_crop565) free(s_crop565);
    s_crop565 = (uint16_t*)heap_caps_malloc((size_t)needCropPx * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    s_cropCap = s_crop565 ? needCropPx : 0;
  }
  return (s_png != nullptr && s_radarFb != nullptr && (needCropPx == 0 || s_crop565 != nullptr));
}

static void buildTacticalComposite() {
  if (rvMode()) return;
  bool isShmu = shmuMode();
  int cnt = isShmu ? SHMU_AnimCount() : CHMU_AnimCount();
  if (cnt <= 0) return;

  int newestIdx = cnt - 1;
  uint8_t* pngData = isShmu ? SHMU_AnimData(newestIdx) : CHMU_AnimData(newestIdx);
  size_t   pngSize = isShmu ? SHMU_AnimSize(newestIdx) : CHMU_AnimSize(newestIdx);
  if (!pngData || pngSize == 0) return;

  if (!ensureTacticalRadarBuffers(0)) return;
  if (s_png->openRAM(pngData, pngSize, pngDrawTactical) != PNG_SUCCESS) {
    return;
  }
  s_tc.imgW = s_png->getWidth();
  s_tc.imgH = s_png->getHeight();
  s_tc.isShmu = isShmu;
  if (isShmu) {
    s_tc.dataX1 = s_tc.imgW - 1;
    s_tc.dataY0 = 0;
  } else {
    s_tc.dataX1 = tacLonToX(CHMU_LON_DATA_RIGHT, false, s_tc.imgW);
    s_tc.dataY0 = tacLatToY(CHMU_LAT_DATA_TOP, false, s_tc.imgH);
  }

  if (s_tc.imgW > s_lineCap) {
    if (s_lineBuf) free(s_lineBuf);
    s_lineBuf = (uint16_t*)heap_caps_malloc((size_t)s_tc.imgW * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!s_lineBuf) s_lineBuf = (uint16_t*)malloc((size_t)s_tc.imgW * sizeof(uint16_t));
    s_lineCap = s_lineBuf ? s_tc.imgW : 0;
  }
  if (!s_lineBuf) { s_png->close(); return; }

  // Vypocet bounding boxu orezu (lat, lon, radiusKm)
  double clat = Settings_Lat(), clon = Settings_Lon();
  float crng = currentRange();
  if (isWholeCountry()) {
    getWholeCountryView(&clat, &clon, &crng, nullptr);
  }
  float degLat = crng / 111.32f;
  float degLon = crng / (111.32f * cosf(clat * 0.0174532925f));
  int x1 = tacLonToX(clon - degLon, isShmu, s_tc.imgW);
  int x2 = tacLonToX(clon + degLon, isShmu, s_tc.imgW);
  int y1 = tacLatToY(clat + degLat, isShmu, s_tc.imgH);
  int y2 = tacLatToY(clat - degLat, isShmu, s_tc.imgH);
  if (x2 < x1) { int t = x1; x1 = x2; x2 = t; }
  if (y2 < y1) { int t = y1; y1 = y2; y2 = t; }

  s_tc.x1 = x1; s_tc.x2 = x2;
  s_tc.y1 = y1; s_tc.y2 = y2;
  s_tc.cw = x2 - x1 + 1;
  s_tc.ch = y2 - y1 + 1;
  if (s_tc.cw <= 0 || s_tc.ch <= 0) { s_png->close(); return; }

  int needCropPx = s_tc.cw * s_tc.ch;
  if (!ensureTacticalRadarBuffers(needCropPx)) { s_png->close(); return; }

  memset(s_crop565, 0, (size_t)needCropPx * sizeof(uint16_t));
  s_png->decode(nullptr, 0);
  s_png->close();

  // Skalovanie do 480x480 kruhoveho framebufferu s_radarFb
  const int cw = s_tc.cw, ch = s_tc.ch;
  if (cw <= 0 || ch <= 0) return;
  const long R2 = (long)DISP_R * DISP_R;
  const bool smooth = Settings_SmoothRadar() && (cw > 1) && (ch > 1);

  if (!smooth) {
    for (int dy = 0; dy < LCD_HEIGHT; dy++) {
      long ddy = dy - R_CY;
      uint16_t* dst = s_radarFb + (int32_t)dy * LCD_WIDTH;
      long room = R2 - ddy * ddy;
      if (room <= 0) {
        memset(dst, 0, LCD_WIDTH * sizeof(uint16_t));
        continue;
      }
      int half = (int)sqrtf((float)room);
      int sx0 = R_CX - half, sx1 = R_CX + half;
      if (sx0 < 0) sx0 = 0;
      if (sx1 >= LCD_WIDTH) sx1 = LCD_WIDTH - 1;
      if (sx0 > 0) memset(dst, 0, sx0 * sizeof(uint16_t));
      if (sx1 < LCD_WIDTH - 1) memset(dst + sx1 + 1, 0, (LCD_WIDTH - 1 - sx1) * sizeof(uint16_t));

      int srcRow = clampI((int)((int64_t)dy * ch / LCD_HEIGHT), 0, ch - 1);
      uint16_t* row = s_crop565 + (int64_t)srcRow * cw;
      for (int dx = sx0; dx <= sx1; dx++) {
        int srcCol = clampI((int)((int64_t)dx * cw / LCD_WIDTH), 0, cw - 1);
        dst[dx] = row[srcCol];
      }
    }
    return;
  }

  // Bilinear interpolation
  struct TacScaleMapX {
    uint16_t x0;
    uint16_t x1;
    uint8_t  qx;
  };
  static TacScaleMapX s_tacMapX[LCD_WIDTH];

  for (int dx = 0; dx < LCD_WIDTH; dx++) {
    int fx = (int)(((int64_t)dx * (cw - 1) * 256 + (LCD_WIDTH / 2)) / (LCD_WIDTH - 1));
    int x0 = fx >> 8;
    if (x0 < 0) x0 = 0;
    if (x0 >= cw) x0 = cw - 1;
    int x1 = (x0 < cw - 1) ? x0 + 1 : x0;
    s_tacMapX[dx].x0 = (uint16_t)x0;
    s_tacMapX[dx].x1 = (uint16_t)x1;
    s_tacMapX[dx].qx = (uint8_t)(fx & 0xFF);
  }

  for (int dy = 0; dy < LCD_HEIGHT; dy++) {
    long ddy = dy - R_CY;
    uint16_t* dst = s_radarFb + (int32_t)dy * LCD_WIDTH;
    long room = R2 - ddy * ddy;
    if (room <= 0) {
      memset(dst, 0, LCD_WIDTH * sizeof(uint16_t));
      continue;
    }
    int half = (int)sqrtf((float)room);
    int sx0 = R_CX - half, sx1 = R_CX + half;
    if (sx0 < 0) sx0 = 0;
    if (sx1 >= LCD_WIDTH) sx1 = LCD_WIDTH - 1;
    if (sx0 > 0) memset(dst, 0, sx0 * sizeof(uint16_t));
    if (sx1 < LCD_WIDTH - 1) memset(dst + sx1 + 1, 0, (LCD_WIDTH - 1 - sx1) * sizeof(uint16_t));

    int fy = (int)(((int64_t)dy * (ch - 1) * 256 + (LCD_HEIGHT / 2)) / (LCD_HEIGHT - 1));
    int y0 = fy >> 8;
    if (y0 < 0) y0 = 0;
    if (y0 >= ch) y0 = ch - 1;
    int y1 = (y0 < ch - 1) ? y0 + 1 : y0;
    int qy = fy & 0xFF;
    int wy0 = 256 - qy;
    int wy1 = qy;

    const uint16_t* r0 = s_crop565 + (int64_t)y0 * cw;
    const uint16_t* r1 = s_crop565 + (int64_t)y1 * cw;

    for (int dx = sx0; dx <= sx1; dx++) {
      int x0 = s_tacMapX[dx].x0;
      int x1 = s_tacMapX[dx].x1;
      int qx = s_tacMapX[dx].qx;

      uint16_t c00 = r0[x0];
      uint16_t c10 = r0[x1];
      uint16_t c01 = r1[x0];
      uint16_t c11 = r1[x1];

      uint16_t outCol;
      if ((c00 | c10 | c01 | c11) == 0) {
        outCol = 0x0000;
      } else if (c00 == c10 && c00 == c01 && c00 == c11) {
        outCol = c00;
      } else {
        int w00 = (wy0 * (256 - qx)) >> 8;
        int w10 = (wy0 * qx) >> 8;
        int w01 = (wy1 * (256 - qx)) >> 8;
        int w11 = (wy1 * qx) >> 8;

        uint32_t r = ((c00 >> 11) & 0x1F) * w00 +
                     ((c10 >> 11) & 0x1F) * w10 +
                     ((c01 >> 11) & 0x1F) * w01 +
                     ((c11 >> 11) & 0x1F) * w11;

        uint32_t g = ((c00 >> 5) & 0x3F) * w00 +
                     ((c10 >> 5) & 0x3F) * w10 +
                     ((c01 >> 5) & 0x3F) * w01 +
                     ((c11 >> 5) & 0x3F) * w11;

        uint32_t b = (c00 & 0x1F) * w00 +
                     (c10 & 0x1F) * w10 +
                     (c01 & 0x1F) * w01 +
                     (c11 & 0x1F) * w11;

        r >>= 8;
        g >>= 8;
        b >>= 8;
        if (r < 1 && g < 2 && b < 1) outCol = 0x0000;
        else outCol = (uint16_t)((r << 11) | (g << 5) | b);
      }
      dst[dx] = outCol;
    }
  }
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
  } else if (s_radarFb) {
    blitRainViewer(s_radarFb);
  }

  // 2. Borders & Cities
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
    UI_DrawHomeMarker(R_CX, R_CY);
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
  int specialIdx = -1;
  float specialDistKm = 999999.0f;
  char specialLabel[32] = "";
  uint16_t specialCol = C_CYAN;
  SpecialCategory bestSpecialCat = SPEC_NONE;

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

    // Classify special interest flights
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
    gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 6, LY_SUB - 3, tw + 12, 20, C_RED);
    UI_TextCentered(sub, LY_SUB, C_WHITE, 2);
  } else if (specialIdx >= 0 && emergIdx < 0) {
    const Aircraft& spAc = list[specialIdx];
    const char* cname = spAc.callsign[0] ? spAc.callsign : spAc.hex;
    snprintf(sub, sizeof(sub), "! %s: %s (%.0f km) !", specialLabel, cname, specialDistKm);
    UI_TextCentered(sub, LY_SUB, specialCol, 1);
  } else {
    if (closestIdx >= 0 && minDistKm <= crng) {
      const char* cname = list[closestIdx].callsign[0] ? list[closestIdx].callsign : list[closestIdx].hex;
      snprintf(sub, sizeof(sub), "%s: %d  [%s: %.1f km]", T(S_AIRCRAFT), drawnCount, cname, minDistKm);
    } else {
      snprintf(sub, sizeof(sub), "%s: %d%s", T(S_AIRCRAFT), drawnCount,
               watchedSeen ? " *" : "");
    }
    UI_TextCentered(sub, LY_SUB, watchedSeen ? C_GREEN : C_CYAN, 1);
  }

  // 5. Range indicator at the bottom
  // Standardized UI indicator matching ScreenWeather and ScreenPlanes
  if (isWholeCountry()) {
    UI_DrawRangeIndicator(nullptr, s_rangeIdx, RANGE_COUNT, false);
  } else {
    char rbuf[16];
    snprintf(rbuf, sizeof(rbuf), "%.0f km", crng);
    UI_DrawRangeIndicator(rbuf, s_rangeIdx, RANGE_COUNT, true);
  }

  // 6. Detail overlay
  if (selIdx >= 0 && selIdx < n) { s_selCache = list[selIdx]; s_selCacheOk = true; }
  const bool signalLost = (selIdx < 0) && s_selCacheOk;

  if (ScreenTactical_DetailOpen() && s_selCacheOk) {
    Route_Select(s_selCache.callsign, s_selCache.lat, s_selCache.lon);
    PlanePhoto_Select(s_selCache.reg, s_selCache.hex);
    const RouteInfo* rt = Route_Get();
    UI_DrawAircraftDetail(s_selCache, rt, Route_GetState(), signalLost);
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
  s_lastRadarFetch = millis();
  if (rvMode()) {
    if (isWholeCountry()) {
      RainViewer_Begin(clat, clon, -crng, 1);
    } else {
      RainViewer_Begin(clat, clon, crng, 1);
    }
  } else {
    int cnt = shmuMode() ? SHMU_AnimCount() : CHMU_AnimCount();
    if (cnt > 0) {
      buildTacticalComposite();
    }
    Async_RequestRadar();
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
  s_lastRadarFetch = millis();
  if (rvMode()) {
    if (isWholeCountry()) {
      RainViewer_Begin(clat, clon, -crng, 1);
    } else {
      RainViewer_Begin(clat, clon, crng, 1);
    }
  } else {
    int cnt = shmuMode() ? SHMU_AnimCount() : CHMU_AnimCount();
    if (cnt > 0) {
      buildTacticalComposite();
    }
  }
}

bool ScreenTactical_Tick() {
  if (WiFi.status() != WL_CONNECTED) return false;

  unsigned long now = millis();
  uint8_t curSrc = Settings_RadarSource();
  static uint8_t s_lastSrc = 255;
  bool srcChanged = false;
  if (curSrc != s_lastSrc) {
    s_lastSrc = curSrc;
    srcChanged = true;
    double clat = Settings_Lat(), clon = Settings_Lon();
    float crng = currentRange();
    if (isWholeCountry()) {
      getWholeCountryView(&clat, &clon, &crng, nullptr);
    }
    s_lastRadarFetch = now;
    if (rvMode()) {
      if (isWholeCountry()) {
        RainViewer_Begin(clat, clon, -crng, 1);
      } else {
        RainViewer_Begin(clat, clon, crng, 1);
      }
    } else {
      int cnt = shmuMode() ? SHMU_AnimCount() : CHMU_AnimCount();
      if (cnt > 0) {
        buildTacticalComposite();
      }
      Async_RequestRadar();
    }
  }

  // Periodicka obnova zrazkoveho radaru (RainViewer) kazdych 5 minut alebo retry pri chybe
  if (rvMode() && !RainViewer_Busy()) {
    bool failed = (RainViewer_Failed() || RainViewer_Count() == 0);
    if ((failed && (now - s_lastRadarFetch >= RADAR_RETRY_MS)) ||
        (!failed && (now - s_lastRadarFetch >= TACTICAL_RADAR_PERIOD_MS))) {
      s_lastRadarFetch = now;
      RainViewer_Refresh();
    }
  } else if (!rvMode()) {
    if (now - s_lastRadarFetch >= TACTICAL_RADAR_PERIOD_MS) {
      s_lastRadarFetch = now;
      Async_RequestRadar();
    }
  }

  bool curSmooth = Settings_SmoothRadar();
  static bool s_lastSmooth = true;
  bool smoothChanged = false;
  if (curSmooth != s_lastSmooth) {
    s_lastSmooth = curSmooth;
    smoothChanged = true;
    if (!rvMode()) {
      buildTacticalComposite();
    }
  }

  bool routeChanged = Async_TakeRouteUpdated() || Route_TakeChanged() || PlanePhoto_TakeChanged();
  bool adsbChanged  = Async_TakeAdsbUpdated();
  bool radarChanged = Async_TakeRadarUpdated();

  if (radarChanged && !rvMode()) {
    buildTacticalComposite();
  }

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

  return routeChanged || radarChanged || srcChanged || smoothChanged;
}

bool ScreenTactical_HandleTap(int x, int y) {
  if (UI_IsPhotoFullscreen()) {
    UI_SetPhotoFullscreen(false);
    return true;
  }
  if (ScreenTactical_DetailOpen()) {
    // If photo is loaded and tap is on the photo box -> open fullscreen photo
    if (PlanePhoto_GetState() == PHOTO_OK && x >= 130 && x <= 350 && y >= 40 && y <= 195) {
      UI_SetPhotoFullscreen(true);
      return true;
    }
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
