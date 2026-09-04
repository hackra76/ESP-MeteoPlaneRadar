// =============================================================================
//  MeteoPlaneRadar
//  Screen: CHMU precipitation radar (meteoradar) with a 6-frame animation.
//  Adapted for MeteoPlaneRadar (chiptron.cz) and reworked to the ROUND 480x480 display:
//    - isotropic crop (aspect 1:1 instead of the rectangular 1.5:1),
//    - the decoded image is masked to the display circle (nothing is drawn
//      outside it),
//    - all overlays (legend, animation indicator, range) repositioned so they
//      stay inside the circle.
//
//  Coordinates: Web Mercator, square crop (isotropic scale).
//  Animation: up to 6 frames (5-min step), ~2 fps, ~5 s pause between cycles.
//  New frames are fetched only while the last frame is shown (in the pause) so
//  the animation never tears. "Nacitam animaci..." is shown while downloading.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenWeather.h"
#include "AsyncCore.h"
#include "CHMU.h"
#include "SHMU.h"
#include "Settings.h"
#include "Config.h"
#include "UI.h"
#include "Layout.h"
#include "Lang.h"
#include "Status.h"
#include "RainViewer.h"
#include "Display_ST7701.h"
#include "EuBorder.h"

#include <WiFi.h>
#include <PNGdec.h>
#include <math.h>
#include <string.h>
#include <esp_heap_caps.h>

#define ANIM_FRAMES  CHMU_ANIM_MAX   // 6

// --- Round-display geometry ---
static const int CX     = LCD_WIDTH  / 2;   // 240
static const int CY     = LCD_HEIGHT / 2;   // 240
static const int DISP_R = LCD_WIDTH  / 2 - 2;  // 238 - pixels beyond this are masked off

// Ranges (radius in km, mapped onto the display height).
static const float RANGES_KM[] = METEO_RANGES_KM;
static const int   RANGE_COUNT = sizeof(RANGES_KM) / sizeof(RANGES_KM[0]);
static int s_rangeIdx = 1;
static float currentRange() { return RANGES_KM[s_rangeIdx]; }

static PNG* s_png = nullptr;
static bool ensurePngDecoder() {
  if (!s_png) {
    s_png = (PNG*)heap_caps_malloc(sizeof(PNG), MALLOC_CAP_SPIRAM);
    if (!s_png) s_png = (PNG*)malloc(sizeof(PNG));
  }
  return s_png != nullptr;
}
static int s_imgW = 600, s_imgH = 480;
static int s_dataX1 = 100000, s_dataY0 = -1;   // data bounds (outside = title/scale in PNG)
static String s_status = "Start...";
static bool s_wide = false;                    // frame too wide for PNGdec

// Shown as a small note under the frame indicator instead of wiping the screen.
// Refreshing takes a few seconds and blanking the radar for that long - every
// five minutes - was the most annoying thing about this screen.
static bool s_loading   = false;   // a download is running right now
static bool s_lastFail  = false;   // the last attempt did not bring anything

struct Crop { int x1, y1, x2, y2; };
static Crop s_crop;
static uint16_t* s_lineBuf = nullptr;
static int       s_lineCap = 0;

// Decoded frame crops (RGB565) in PSRAM.
static uint16_t* s_frame565[ANIM_FRAMES] = {0};
static int       s_frameCap[ANIM_FRAMES] = {0};
static uint16_t* s_crop565 = nullptr;    // current target (pngDraw) / source (blit)
static int       s_frameCount = 0;

// Animation state.
static int  s_curFrame = 0;
static bool s_gap = false;
static bool s_needRebuild = false;
static unsigned long s_lastStep = 0, s_gapStart = 0, s_lastFetch = 0;

static int clampI(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// --- Sources, one screen -----------------------------------------------------
// CHMU and SHMU are national composite images that we crop; RainViewer is a tile
// service that builds a finished display-sized frame of its own. Everything below
// goes through these accessors.
static bool rvMode()   { return Settings_RadarSource() == RADAR_SRC_RAINVIEWER; }
static bool shmuMode() { return Settings_RadarSource() == RADAR_SRC_SHMU; }
static bool chmuMode() { return Settings_RadarSource() == RADAR_SRC_CHMU; }

static int srcCount() {
  return rvMode() ? RainViewer_Count() : s_frameCount;
}

static String srcTimeText(int i) {
  if (rvMode())   return RainViewer_TimeText(i);
  if (shmuMode()) return SHMU_AnimTimeText(i);
  return CHMU_AnimTimeText(i);
}

// How old the frame is, in minutes. CHMU and SHMU frames are a fixed five minutes apart
// so the position in the animation is enough; RainViewer publishes its own
// timestamps and the spacing can vary.
static int srcMinutesAgo(int i, int n) {
  if (rvMode()) return RainViewer_MinutesAgo(i);
  return (n - 1 - i) * 5;
}

// NOTE: the weather screen is deliberately NOT rotated by the map-rotation
// setting. A precipitation composite is read as a map and the outline is
// what anchors it, so north stays up here. The rotation only applies to the
// aircraft radar, which is a "view from where I stand".

// --- Geographic bounds based on active provider (CHMU vs SHMU) ---
static float radarLonLeft()   { return shmuMode() ? SHMU_LON_LEFT : CHMU_LON_LEFT; }
static float radarLonRight()  { return shmuMode() ? SHMU_LON_RIGHT : CHMU_LON_RIGHT; }
static float radarLatTop()    { return shmuMode() ? SHMU_LAT_TOP : CHMU_LAT_TOP; }
static float radarLatBottom() { return shmuMode() ? SHMU_LAT_BOTTOM : CHMU_LAT_BOTTOM; }

// --- Web Mercator ---
static float mercY(float latDeg) { float r = latDeg * 0.0174532925f; return logf(tanf(0.7853981634f + r * 0.5f)); }
static float mercTop()    { return mercY(radarLatTop()); }
static float mercBottom() { return mercY(radarLatBottom()); }
static int lonToX(float lon) { return lroundf((lon - radarLonLeft()) * (s_imgW - 1) / (radarLonRight() - radarLonLeft())); }
static int latToY(float lat) { float yt = mercTop(), yb = mercBottom(); return lroundf((yt - mercY(lat)) * (s_imgH - 1) / (yt - yb)); }

// Inverse of lonToX / latToY - the map layer needs the visible window in
// degrees, and the crop is in image pixels.
static float xToLon(int x) {
  return radarLonLeft() + (float)x * (radarLonRight() - radarLonLeft()) / (s_imgW - 1);
}
static float yToLat(int y) {
  float yt = mercTop(), yb = mercBottom();
  float m = yt - (float)y * (yt - yb) / (s_imgH - 1);
  return (2.0f * atanf(expf(m)) - 1.5707963268f) * 57.29577951f;
}

// Round display -> isotropic (square) crop.
static const float ASPECT = (float)LCD_WIDTH / (float)LCD_HEIGHT;   // 1.0

// Range 0 = the whole country: a fixed window that covers the country/region.
static bool isWholeCountry() { return currentRange() <= 0.0f; }

static bool isLocSK(double lat, double lon) {
  return (lat >= 47.5 && lat <= 49.9 && lon >= 16.6 && lon <= 22.8);
}
static bool isLocCZ(double lat, double lon) {
  return (lat >= 48.3 && lat <= 51.3 && lon >= 11.8 && lon <= 19.1);
}

static void getWholeCountryView(double* lat, double* lon, float* radiusKm, StrId* strId) {
  double ulat = Settings_Lat(), ulon = Settings_Lon();
  if (rvMode()) {
    if (isLocSK(ulat, ulon) || (!isLocCZ(ulat, ulon) && Lang_Get() == LANG_SK)) {
      if (lat) *lat = SK_VIEW_LAT;
      if (lon) *lon = SK_VIEW_LON;
      if (radiusKm) *radiusKm = SK_VIEW_RADIUS_KM;
      if (strId) *strId = S_WHOLE_SK;
    } else if (isLocCZ(ulat, ulon) || Lang_Get() == LANG_CZ) {
      if (lat) *lat = CZ_VIEW_LAT;
      if (lon) *lon = CZ_VIEW_LON;
      if (radiusKm) *radiusKm = CZ_VIEW_RADIUS_KM;
      if (strId) *strId = S_WHOLE_CZ;
    } else {
      if (lat) *lat = ulat;
      if (lon) *lon = ulon;
      if (radiusKm) *radiusKm = 250.0f;
      if (strId) *strId = S_WHOLE_COUNTRY;
    }
  } else if (shmuMode()) {
    // SHMU covers Slovakia
    if (lat) *lat = SK_VIEW_LAT;
    if (lon) *lon = SK_VIEW_LON;
    if (radiusKm) *radiusKm = SK_VIEW_RADIUS_KM;
    if (strId) *strId = S_WHOLE_SK;
  } else {
    // CHMU covers Czech Republic
    if (lat) *lat = CZ_VIEW_LAT;
    if (lon) *lon = CZ_VIEW_LON;
    if (radiusKm) *radiusKm = CZ_VIEW_RADIUS_KM;
    if (strId) *strId = S_WHOLE_CZ;
  }
}

// IMPORTANT: the crop is deliberately NOT clamped to the image.
//
// It used to be, and that was the bug behind "the position jumps somewhere else
// at 200 km". Clamping only moves the edge that sticks out, so the rectangle
// stops being centred on the user - while the crosshair was still drawn in the
// middle of the display. Near the edge of the CHMU coverage (Cheb, Ostrava at
// 200 km) the marker could end up tens of kilometres off. Letting the crop run
// past the image and filling those pixels with black keeps the geometry honest.
static void makeCrop(double lat, double lon, float radiusKm) {
  int x1, y1, x2, y2;

  if (radiusKm <= 0.0f) {          // whole country: fixed centre, fixed radius
    getWholeCountryView(&lat, &lon, &radiusKm, nullptr);
  }

  float degLat = radiusKm / 111.32f;
  float degLon = radiusKm * ASPECT / (111.32f * cosf(lat * 0.0174532925));
  x1 = lonToX(lon - degLon); x2 = lonToX(lon + degLon);
  y1 = latToY(lat + degLat); y2 = latToY(lat - degLat);
  if (x2 < x1) { int t = x1; x1 = x2; x2 = t; }
  if (y2 < y1) { int t = y1; y1 = y2; y2 = t; }

  s_crop.x1 = x1; s_crop.x2 = x2;
  s_crop.y1 = y1; s_crop.y2 = y2;
}
static int cropW() { return s_crop.x2 - s_crop.x1 + 1; }
static int cropH() { return s_crop.y2 - s_crop.y1 + 1; }

// Projection callback for the outline + cities. With CHMU it follows the crop
// of the source image; with RainViewer the module owns the projection, because
// the zoom level it picked is what decides the scale.
static void borderProject(float lat, float lon, int* sx, int* sy) {
  if (rvMode()) { RainViewer_Project(lat, lon, sx, sy); return; }
  int srcX = lonToX(lon), srcY = latToY(lat);
  *sx = (int)((int64_t)(srcX - s_crop.x1) * LCD_WIDTH  / cropW());
  *sy = (int)((int64_t)(srcY - s_crop.y1) * LCD_HEIGHT / cropH());
}

static int pngDraw(PNGDRAW* d) {
  int srcY = d->y;
  if (srcY < s_crop.y1 || srcY > s_crop.y2) return 1;
  if (!s_crop565 || !s_lineBuf || !s_png) return 1;
  s_png->getLineAsRGB565(d, s_lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);
  const int cw = cropW();
  uint16_t* row = s_crop565 + (int64_t)(srcY - s_crop.y1) * cw;
  const bool isShmu = shmuMode();
  for (int i = 0; i < cw; i++) {
    int srcX = s_crop.x1 + i;
    // Black for: past the edge of the image (the crop may now stick out), past
    // the data area on the right (colour scale), above the data area (title).
    bool have = (srcX >= 0 && srcX < s_imgW && srcX <= s_dataX1 && srcY >= s_dataY0);
    uint16_t col = have ? s_lineBuf[srcX] : 0x0000;
    if (isShmu) {
      // In SHMU PNG, the no-coverage areas outside radar range are light grey (RGB 216/230, indices 65/66).
      // Filter out these greys and pure white so they remain black on our dark display.
      if (col == 0xE71C || col == 0xD6DA || col == 0xE73C || col == 0xD69A || col == 0xFFFF) {
        col = 0x0000;
      }
    }
    row[i] = col;
  }
  return 1;
}

// RainViewer frames are already in display coordinates - the zoom was chosen so
// one world pixel is one display pixel - so this is a straight masked copy with
// no scaling at all.
static void blitRainViewer(const uint16_t* fb) {
  if (!fb) return;
  const long R2 = (long)DISP_R * DISP_R;
  for (int dy = 0; dy < LCD_HEIGHT; dy++) {
    long ddy = dy - CY;
    long room = R2 - ddy * ddy;
    if (room <= 0) continue;
    int half = (int)sqrtf((float)room);
    int x0 = CX - half, x1 = CX + half;
    if (x0 < 0) x0 = 0;
    if (x1 > LCD_WIDTH - 1) x1 = LCD_WIDTH - 1;
    const uint16_t* row = fb + (int32_t)dy * LCD_WIDTH;
    for (int dx = x0; dx <= x1; dx++) gfx->drawPixel(dx, dy, row[dx]);
  }
}

struct ScaleMapX {
  uint16_t x0;
  uint16_t x1;
  uint8_t  qx;
};
static ScaleMapX s_mapX[LCD_WIDTH];

// Stretch the current crop (s_crop565) over the display, masked to the circle.
// Pixels outside the round area are left black.
// When Settings_SmoothRadar() is enabled, performs bilinear interpolation.
static void blitCrop() {
  if (!s_crop565) return;
  const int cw = cropW(), ch = cropH();
  if (cw <= 0 || ch <= 0) return;
  const long R2 = (long)DISP_R * DISP_R;
  const bool smooth = Settings_SmoothRadar() && (cw > 1) && (ch > 1);

  if (!smooth) {
    for (int dy = 0; dy < LCD_HEIGHT; dy++) {
      long ddy = dy - CY;
      long room = R2 - ddy * ddy;
      if (room <= 0) continue;
      int half = (int)sqrtf((float)room);
      int x0 = CX - half, x1 = CX + half;
      if (x0 < 0) x0 = 0;
      if (x1 >= LCD_WIDTH) x1 = LCD_WIDTH - 1;

      int srcRow = clampI((int)((int64_t)dy * ch / LCD_HEIGHT), 0, ch - 1);
      const uint16_t* row = s_crop565 + (int64_t)srcRow * cw;
      for (int dx = x0; dx <= x1; dx++) {
        int srcCol = clampI((int)((int64_t)dx * cw / LCD_WIDTH), 0, cw - 1);
        gfx->drawPixel(dx, dy, row[srcCol]);
      }
    }
    return;
  }

  // Bilinear interpolation
  for (int dx = 0; dx < LCD_WIDTH; dx++) {
    int fx = (int)(((int64_t)dx * (cw - 1) * 256 + (LCD_WIDTH / 2)) / (LCD_WIDTH - 1));
    int x0 = fx >> 8;
    if (x0 < 0) x0 = 0;
    if (x0 >= cw) x0 = cw - 1;
    int x1 = (x0 < cw - 1) ? x0 + 1 : x0;
    s_mapX[dx].x0 = (uint16_t)x0;
    s_mapX[dx].x1 = (uint16_t)x1;
    s_mapX[dx].qx = (uint8_t)(fx & 0xFF);
  }

  for (int dy = 0; dy < LCD_HEIGHT; dy++) {
    long ddy = dy - CY;
    long room = R2 - ddy * ddy;
    if (room <= 0) continue;
    int half = (int)sqrtf((float)room);
    int sx0 = CX - half, sx1 = CX + half;
    if (sx0 < 0) sx0 = 0;
    if (sx1 >= LCD_WIDTH) sx1 = LCD_WIDTH - 1;

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
      int x0 = s_mapX[dx].x0;
      int x1 = s_mapX[dx].x1;
      int qx = s_mapX[dx].qx;

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
      gfx->drawPixel(dx, dy, outCol);
    }
  }
}

// -----------------------------------------------------------------------------
//  Build the crop of every frame (decode PNG -> s_frame565[f]).
// -----------------------------------------------------------------------------
static bool rebuildCrops() {
  int cnt = shmuMode() ? SHMU_AnimCount() : CHMU_AnimCount();
  if (cnt <= 0) { s_frameCount = 0; return false; }
  if (cnt > ANIM_FRAMES) cnt = ANIM_FRAMES;

  uint8_t* frame0Data = shmuMode() ? SHMU_AnimData(0) : CHMU_AnimData(0);
  size_t   frame0Size = shmuMode() ? SHMU_AnimSize(0) : CHMU_AnimSize(0);
  if (!frame0Data || frame0Size == 0) { s_frameCount = 0; return false; }

  if (!ensurePngDecoder()) { s_frameCount = 0; return false; }

  // Dimensions + width sanity check from the first frame.
  if (s_png->openRAM(frame0Data, frame0Size, pngDraw) != PNG_SUCCESS) { s_frameCount = 0; return false; }
  s_imgW = s_png->getWidth(); s_imgH = s_png->getHeight();
  if (shmuMode()) {
    s_dataX1 = s_imgW - 1;
    s_dataY0 = 0;
  } else {
    s_dataX1 = lonToX(CHMU_LON_DATA_RIGHT);   // right of this x = scale/edge
    s_dataY0 = latToY(CHMU_LAT_DATA_TOP);     // above this y = title
  }
  int bufSize = s_png->getBufferSize();
  int pitch = (s_imgH > 0) ? bufSize / s_imgH : bufSize;
  s_png->close();
  if (2 * (pitch + 1) > (int)PNG_MAX_BUFFERED_PIXELS) { s_wide = true; s_frameCount = 0; return false; }
  s_wide = false;

  if (s_imgW > s_lineCap) {
    if (s_lineBuf) free(s_lineBuf);
    s_lineBuf = (uint16_t*)heap_caps_malloc((size_t)s_imgW * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!s_lineBuf) s_lineBuf = (uint16_t*)malloc((size_t)s_imgW * sizeof(uint16_t));
    s_lineCap = s_lineBuf ? s_imgW : 0;
  }
  if (!s_lineBuf) { s_frameCount = 0; return false; }

  // Size the buffers for the WIDEST range there is, not for the current one.
  int need = 0;
  for (int r = 0; r < RANGE_COUNT; r++) {
    makeCrop(Settings_Lat(), Settings_Lon(), RANGES_KM[r]);
    int sz = cropW() * cropH();
    if (sz > need) need = sz;
  }
  makeCrop(Settings_Lat(), Settings_Lon(), currentRange());   // back to the real one

  int okc = 0;
  for (int f = 0; f < cnt; f++) {
    if (s_frameCap[f] < need) {
      if (s_frame565[f]) free(s_frame565[f]);
      s_frame565[f] = (uint16_t*)heap_caps_malloc((size_t)need * 2, MALLOC_CAP_SPIRAM);
      if (!s_frame565[f]) s_frame565[f] = (uint16_t*)malloc((size_t)need * 2);
      s_frameCap[f] = s_frame565[f] ? need : 0;
    }
    if (!s_frame565[f]) break;
    s_crop565 = s_frame565[f];
    memset(s_crop565, 0, (size_t)need * 2);
    uint8_t* fData = shmuMode() ? SHMU_AnimData(f) : CHMU_AnimData(f);
    size_t   fSize = shmuMode() ? SHMU_AnimSize(f) : CHMU_AnimSize(f);
    if (!fData || fSize == 0) break;
    if (s_png->openRAM(fData, fSize, pngDraw) != PNG_SUCCESS) { s_png->close(); break; }
    s_png->decode(nullptr, 0);
    s_png->close();
    okc++;
  }
  s_frameCount = okc;
  return okc > 0;
}

// Download frames + build crops. The fetch blocks for a few seconds, so the
// screen is repainted first WITH the "Nacitam" note on it - the last good
// animation stays visible the whole time instead of going black.
static void loadAndBuild() {
  s_loading = true;
  ScreenWeather_Draw();          // last good frame + the note
  gfx->flush();

  const int prevCount = s_frameCount;   // what we already have on screen
  int n = shmuMode() ? SHMU_FetchAnim(ANIM_FRAMES) : CHMU_FetchAnim(ANIM_FRAMES);
  bool ok = (n > 0) && rebuildCrops();

  // rebuildCrops() zeroes the frame count when it gives up. If we had frames
  // before, put them back rather than leaving the screen empty.
  if (!ok && s_frameCount == 0 && prevCount > 0) s_frameCount = prevCount;

  s_loading  = false;
  s_lastFail = !ok;
  if (ok) s_status = T(S_OK);
  else if (s_wide) s_status = T(S_FRAME_WIDE);
  else s_status = (s_frameCount > 0) ? T(S_OK) : T(S_ERROR);

  if (shmuMode()) {
    Status_Set(ST_RADAR, ok ? "SHMU: %d snimkov" : "SHMU: chyba", s_frameCount);
  } else {
    Status_Set(ST_RADAR, ok ? "CHMU: %d snimku" : "CHMU: chyba", s_frameCount);
  }
}

// -----------------------------------------------------------------------------
//  Overlays (round layout).
// -----------------------------------------------------------------------------
static void drawOverlay() {
  // Crosshair on the user's position - PROJECTED, not simply the middle of the
  // screen. With the "whole country" view the user is off-centre by definition,
  // and even a circular range can end up off-centre once the crop runs past the
  // edge of the image. Drawing it in the middle regardless is what made the
  // marker point at the wrong place.
  {
    int px, py;
    borderProject(Settings_Lat(), Settings_Lon(), &px, &py);
    long ddx = px - CX, ddy = py - CY;
    if (ddx * ddx + ddy * ddy <= (long)(DISP_R - 10) * (DISP_R - 10)) {
      UI_DrawHomeMarker(px, py);
    }
  }

  // Precipitation-intensity legend - dBZ / mm/h, on the left and vertically
  // centred so it stays inside the circle.
  //
  // The palette must match the source on screen - the same yellow means 40 dBZ
  // on one scale and 35 on the other - so each brings its own table and says
  // which one you are reading. RainViewer's is "Universal Blue", the only
  // scheme their API offers, from their published dBZ table; the mm/h column is
  // the Marshall-Palmer conversion (Z = 200 R^1.6) that the CHMU scale also
  // uses, so both read alike.
  if (Settings_ShowLegends()) {
    static const uint16_t COL_CHMU[6] = { 0xA000, 0xF800, 0xFC20, 0xE6E0, 0x05E0, 0x001F };
    static const char*    LBL_CHMU[6] = { ">56 / >100", "52 / 65", "46 / 27",
                                          "40 / 12", "32 / 3.6", "20 / <1" };

    static const uint16_t COL_SHMU[6] = { 0xFC7F, 0xF040, 0xFEA0, 0x0626, 0x12B5, 0xA61A };
    static const char*    LBL_SHMU[6] = { ">55 / >100", "50 / 49", "40 / 12",
                                          "30 / 2.7",   "20 / 0.7", "10 / <0.2" };

    // #ffaaff  #c10000  #ff4400  #ffaa00  #005588  #00a3e0
    static const uint16_t COL_RV[6]   = { 0xFD5F, 0xC000, 0xFA20, 0xFD40, 0x02B1, 0x051C };
    static const char*    LBL_RV[6]   = { ">55 / >100", "50 / 49", "45 / 24",
                                          "40 / 12", "30 / 2.7", "20 / 0.7" };

    const bool rv   = rvMode();
    const bool shmu = shmuMode();
    const uint16_t*    col = rv ? COL_RV : (shmu ? COL_SHMU : COL_CHMU);
    const char* const* lbl = rv ? LBL_RV : (shmu ? LBL_SHMU : LBL_CHMU);

    const int lx = 30, ly = 142, boxW = 96, boxH = 22 + 6 * 13 + 2;
    gfx->fillRect(lx - 2, ly - 2, boxW, boxH, C_BLACK);   // readability backing
    const char* srcText = rv ? "RainViewer" : (shmu ? "SHMU" : "CHMU");
    uint16_t    srcCol  = rv ? C_CYAN : (shmu ? C_WHITE : C_GREEN);
    UI_Text(srcText, lx, ly, srcCol, 1);
    UI_Text("dBZ / mm/h", lx, ly + 10, C_GRAY, 1);
    for (int i = 0; i < 6; i++) {
      int ry = ly + 22 + i * 13;
      gfx->fillRect(lx, ry, 9, 9, col[i]);
      gfx->drawRect(lx, ry, 9, 9, C_DKGRAY);
      UI_Text(lbl[i], lx + 13, ry + 1, C_WHITE, 1);
    }
  }

  // Clock + outside temperature, directly under the screen-selector dots.
  UI_DrawStatusLine(LY_STATUS);

  // Animation indicator - dots + the frame's time label. Moved down to make
  // room for the status line above it: dots at y=62, label at y=74.
  {
    int have = srcCount();
    int n = have > 0 ? have : 1;
    int minAgo = srcMinutesAgo(s_curFrame, n);
    char lbl[28];
    String hhmm = (have > 0) ? srcTimeText(s_curFrame) : String("");
    if (minAgo <= 0) snprintf(lbl, sizeof(lbl), "%s  %s", T(S_NOW), hhmm.c_str());
    else             snprintf(lbl, sizeof(lbl), "-%d %s  %s", minAgo, T(S_MIN), hhmm.c_str());
    gfx->fillRect(CX - 70, LY_SUB, 140, 34, C_BLACK);   // backing
    int gap = 16, totalW = (n - 1) * gap, sx0 = CX - totalW / 2, dy = LY_SUB + 10;
    for (int i = 0; i < n; i++) {
      int x = sx0 + i * gap;
      if (i == s_curFrame) gfx->fillCircle(x, dy, 4, C_YELLOW);
      else                 gfx->drawCircle(x, dy, 4, C_GRAY);
    }
    UI_TextCenteredIn(lbl, 0, LCD_WIDTH, LY_LEGEND, C_CYAN, 1);

    // Small note under the frame indicator: a refresh in progress, or the fact
    // that the last one failed and what you are looking at is the older data.
    const char* note = s_loading  ? T(S_LOADING_NEWER)
                     : s_lastFail ? T(S_OLD_DATA)
                                  : nullptr;
    if (note) {
      gfx->fillRect(CX - 160, LY_NOTE, 320, 10, C_BLACK);
      UI_TextCenteredIn(note, 0, LCD_WIDTH, LY_NOTE, C_YELLOW, 1);
    }
  }

  // Range + indicator dots (bottom centre)
  char rbuf[24] = "";
  if (!isWholeCountry()) {
    if (rvMode()) snprintf(rbuf, sizeof(rbuf), "%.0f %s",
                           RainViewer_EffectiveRadiusKm(), T(S_KM));
    else          snprintf(rbuf, sizeof(rbuf), "%.0f %s", currentRange(), T(S_KM));
  }
  UI_DrawRangeIndicator(rbuf, s_rangeIdx, RANGE_COUNT, Settings_ShowLegends());
}

void ScreenWeather_RangeText(char* out, size_t cap) {
  if (!out || !cap) return;
  if (isWholeCountry()) {
    StrId s = shmuMode() ? S_WHOLE_SK : S_WHOLE_CZ;
    getWholeCountryView(nullptr, nullptr, nullptr, &s);
    snprintf(out, cap, "%s", T(s));
  }
  else if (rvMode()) snprintf(out, cap, "%.0f %s", RainViewer_EffectiveRadiusKm(), T(S_KM));
  else               snprintf(out, cap, "%.0f %s", currentRange(), T(S_KM));
}

void ScreenWeather_Enter() {
  Async_SetActiveScreen(SCREEN_METEO_I);
  // Invalidate frames if the radar provider has changed
  static uint8_t s_enterLastSrc = 255;
  uint8_t curSrc = Settings_RadarSource();
  if (curSrc != s_enterLastSrc) {
    s_enterLastSrc = curSrc;
    s_frameCount = 0;
    s_needRebuild = false;
    s_lastFetch = 0;
  }
  uint8_t saved = Settings_MeteoRange();
  if (saved >= RANGE_COUNT) saved = 1;             // guard against a stale value
  if (saved != s_rangeIdx) {                       // only when it actually differs
    s_rangeIdx = saved;
    s_needRebuild = true;
  }
  s_lastStep = millis();
  s_gap = false;
  s_curFrame = 0;
}

static float s_lastRng = -999.0f;
static double s_lastLat = -999.0, s_lastLon = -999.0;

void ScreenWeather_ChangeRange(int dir) {
  s_rangeIdx = (s_rangeIdx + dir + RANGE_COUNT) % RANGE_COUNT;
  Settings_SetMeteoRange(s_rangeIdx);   // remember across restarts (debounced)
  s_needRebuild = true;   // re-crop the frames on the next tick
  if (rvMode()) {
    // A different range means a different zoom, so every tile has to come down
    // again. Begin() notices the change and restarts the incremental fetch.
    s_curFrame = 0;
    s_gap = false;
    double vlat = Settings_Lat(), vlon = Settings_Lon();
    float vrng = currentRange();
    s_lastRng = vrng;
    s_lastLat = vlat;
    s_lastLon = vlon;
    if (vrng <= 0.0f) {
      getWholeCountryView(&vlat, &vlon, &vrng, nullptr);
      RainViewer_Begin(vlat, vlon, -vrng, CHMU_ANIM_MAX);
    } else {
      RainViewer_Begin(vlat, vlon, vrng, CHMU_ANIM_MAX);
    }
  }
}

// RainViewer fetches incrementally - one tile per call - so the loop stays
// responsive and the newest frame appears within a couple of seconds instead of
// after the whole animation has downloaded.
static bool tickRainViewer() {
  unsigned long now = millis();
  float rng = currentRange();
  double vlat = Settings_Lat(), vlon = Settings_Lon();

  bool viewChanged = (rng != s_lastRng) || (fabs(vlat - s_lastLat) > 1e-5) || (fabs(vlon - s_lastLon) > 1e-5);
  if (viewChanged) {
    s_lastRng = rng; s_lastLat = vlat; s_lastLon = vlon;
    if (rng <= 0.0f) {
      getWholeCountryView(&vlat, &vlon, &rng, nullptr);
      RainViewer_Begin(vlat, vlon, -rng, CHMU_ANIM_MAX);
    } else {
      RainViewer_Begin(vlat, vlon, rng, CHMU_ANIM_MAX);
    }
  }

  if (RainViewer_Busy()) {
    s_loading = true;
    return Async_TakeRadarUpdated();
  }
  if (s_loading) {
    s_loading = false;
    s_lastFail = RainViewer_Failed() || RainViewer_Count() == 0;
    s_lastFetch = now;
    s_curFrame = 0;
    s_gap = false;
    s_lastStep = now;
    Status_Set(ST_RADAR, s_lastFail ? "RainViewer: chyba"
                                    : "RainViewer: %d snimku", RainViewer_Count());
    return true;
  }

  // Nothing came back last time. RainViewer_Begin() will not re-arm on its own
  // - the view has not changed - so without this the radar stayed empty until
  // the range was changed or the device rebooted.
  if (s_lastFail && RainViewer_Count() == 0 && (now - s_lastFetch >= RADAR_RETRY_MS)) {
    RainViewer_Refresh();
    s_loading   = true;
    s_lastFetch = now;
    return true;
  }

  const int n = RainViewer_Count();
  if (n <= 0) return false;

  // Refresh only during the pause at the end of a cycle, so the animation never
  // changes under itself halfway through.
  // Poll every 60 s so newly published 5-minute frames appear without lag (Issue #6).
  if (s_gap && (now - s_lastFetch >= 60000UL)) {
    RainViewer_Refresh();
    s_loading = true;
    return true;
  }
  if (s_gap) {
    if (now - s_gapStart >= 5000UL) { s_gap = false; s_curFrame = 0; s_lastStep = now; return true; }
    return false;
  }
  if (now - s_lastStep >= 500UL) {
    s_lastStep = now;
    s_curFrame++;
    if (s_curFrame >= n) { s_curFrame = n - 1; s_gap = true; s_gapStart = now; }
    return true;
  }
  return false;
}

bool ScreenWeather_Tick() {
  if (WiFi.status() != WL_CONNECTED) { s_status = T(S_WIFI_WAIT); return false; }

  // Detect radar source changes dynamically (e.g. changed in QuickControl or WebUI)
  static uint8_t s_lastSrc = 255;
  uint8_t curSrc = Settings_RadarSource();
  if (curSrc != s_lastSrc) {
    s_lastSrc = curSrc;
    s_frameCount = 0;
    s_needRebuild = false;
    s_curFrame = 0;
    s_gap = false;
    s_lastFetch = 0;
    if (rvMode()) {
      double vlat = Settings_Lat(), vlon = Settings_Lon();
      float vrng = currentRange();
      s_lastRng = vrng; s_lastLat = vlat; s_lastLon = vlon;
      if (vrng <= 0.0f) {
        getWholeCountryView(&vlat, &vlon, &vrng, nullptr);
        RainViewer_Begin(vlat, vlon, -vrng, CHMU_ANIM_MAX);
      } else {
        RainViewer_Begin(vlat, vlon, vrng, CHMU_ANIM_MAX);
      }
    }
    return true;
  }

  static bool s_lastSmooth = true;
  bool curSmooth = Settings_SmoothRadar();
  if (curSmooth != s_lastSmooth) {
    s_lastSmooth = curSmooth;
    return true;
  }

  if (rvMode()) return tickRainViewer();
  unsigned long now = millis();

  // Range changed -> just re-crop (frames already downloaded).
  int curCount = shmuMode() ? SHMU_AnimCount() : CHMU_AnimCount();
  if (s_needRebuild && curCount > 0) {
    rebuildCrops();
    s_needRebuild = false; s_curFrame = 0; s_gap = false; s_lastStep = now;
    return true;
  }

  // First load. A failing index fetch leaves s_frameCount at 0, so without the
  // delay the whole listing was re-fetched on the very next tick - once a
  // second, every second.
  if (s_frameCount == 0) {
    if (s_lastFail && (now - s_lastFetch < RADAR_RETRY_MS)) return false;
    loadAndBuild();
    s_lastFetch = now; s_curFrame = 0; s_gap = false; s_lastStep = now;
    return true;
  }

  // Periodic refresh - only during the pause (last frame shown).
  // Poll every 60 s so newly published 5-minute frames appear without lag (Issue #6).
  if (s_gap && (now - s_lastFetch >= 60000UL)) {
    loadAndBuild();
    s_lastFetch = now; s_curFrame = 0; s_gap = false; s_lastStep = now;
    return true;
  }

  // Animation step.
  if (s_gap) {
    if (now - s_gapStart >= 5000UL) { s_gap = false; s_curFrame = 0; s_lastStep = now; return true; }
    return false;
  }
  if (now - s_lastStep >= 500UL) {
    s_lastStep = now;
    s_curFrame++;
    if (s_curFrame >= s_frameCount) { s_curFrame = s_frameCount - 1; s_gap = true; s_gapStart = now; }
    return true;
  }
  return false;
}

void ScreenWeather_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();

  // Chrome owns its lines before the map draws a single city label.
  Layout_ReserveBand(LY_DOTS - 6, 12);
  Layout_ReserveBand(LY_STATUS - 3, 22);
  Layout_ReserveBand(LY_SUB, 22);            // frame dots
  Layout_ReserveBand(LY_LEGEND - 2, 12);     // frame time label
  Layout_ReserveBand(LY_NOTE - 2, 12);       // loading / stale note
  if (!isWholeCountry()) {
    Layout_ReserveBand(LY_RANGE - 4, 24);
  }
  Layout_ReserveBand(LY_RANGE_DOTS - 6, 12);
  // The precipitation legend down the left-hand side.
  if (Settings_ShowLegends()) {
    Layout_Reserve(28, 140, 100, 22 + 6 * 13 + 6);
  }

  const int have = srcCount();
  if (have > 0) {
    int f = clampI(s_curFrame, 0, have - 1);
    if (rvMode()) {
      blitRainViewer(RainViewer_Frame(f));
    } else {
      s_crop565 = s_frame565[f];
      blitCrop();
    }
  }

  // Same map data as the aircraft radar: European outlines and cities, drawn
  // through this screen's projection.
  {
    float lat0, lat1, lon0, lon1;
    if (rvMode()) {
      RainViewer_Window(&lat0, &lat1, &lon0, &lon1);
    } else {
      lat1 = yToLat(s_crop.y1); lat0 = yToLat(s_crop.y2);   // y grows south
      lon0 = xToLon(s_crop.x1); lon1 = xToLon(s_crop.x2);
    }
    float mLat = (lat1 - lat0) * 0.1f, mLon = (lon1 - lon0) * 0.1f;
    lat0 -= mLat; lat1 += mLat; lon0 -= mLon; lon1 += mLon;

    EuBorder_Draw(borderProject, C_GRAY, lat0, lat1, lon0, lon1);

    float rng = rvMode() ? RainViewer_EffectiveRadiusKm() : currentRange();
    bool    showFull = !isWholeCountry() && (rng <= 50.0f);
    uint8_t maxTier;
    if      (isWholeCountry() || rng > 180.0f) maxTier = 1;  // cele Slovensko / najvacsi zoom: iba krajske mesta (Tier 1)
    else if (rng <= 80.0f)                     maxTier = 3;  // velky detail: vsetky mesta (Tier 1-3)
    else                                       maxTier = 2;  // stredny zoom: krajske + regionalne centra (Tier 1-2)
    EuBorder_DrawCities(borderProject, CX, CY, DISP_R, C_WHITE, C_CYAN,
                        showFull, maxTier, lat0, lat1, lon0, lon1);
  }

  // Thin ring marking the edge of the round display.
  gfx->drawCircle(CX, CY, DISP_R, C_DKGRAY);

  // If no radar frames are ready yet, show an overlay badge rather than a black screen
  if (have == 0) {
    const int bw = 260, bh = 54;
    gfx->fillRoundRect(CX - bw / 2, CY - bh / 2, bw, bh, 12, C_DKGRAY);
    gfx->drawRoundRect(CX - bw / 2, CY - bh / 2, bw, bh, 12, C_CYAN);
    UI_TextCentered(T(S_METEORADAR), CY - 16, C_WHITE, 1);
    const char* msg = (s_loading || RainViewer_Busy()) ? T(S_LOADING)
                    : s_wide ? T(S_FRAME_WIDE)
                             : s_status.c_str();
    UI_TextCentered(msg, CY + 2, C_YELLOW, 2);
  }

  drawOverlay();
}
