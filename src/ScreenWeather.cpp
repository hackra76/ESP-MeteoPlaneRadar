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
#include "CHMU.h"
#include "Settings.h"
#include "Config.h"
#include "UI.h"
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

static PNG png;
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

// NOTE: the weather screen is deliberately NOT rotated by the map-rotation
// setting. A precipitation composite is read as a map and the CR outline is
// what anchors it, so north stays up here. The rotation only applies to the
// aircraft radar, which is a "view from where I stand".

// --- Web Mercator ---
static float mercY(float latDeg) { float r = latDeg * 0.0174532925f; return logf(tanf(0.7853981634f + r * 0.5f)); }
static float mercTop()    { return mercY(CHMU_LAT_TOP); }
static float mercBottom() { return mercY(CHMU_LAT_BOTTOM); }
static int lonToX(float lon) { return lroundf((lon - CHMU_LON_LEFT) * (s_imgW - 1) / (CHMU_LON_RIGHT - CHMU_LON_LEFT)); }
static int latToY(float lat) { float yt = mercTop(), yb = mercBottom(); return lroundf((yt - mercY(lat)) * (s_imgH - 1) / (yt - yb)); }

// Inverse of lonToX / latToY - the map layer needs the visible window in
// degrees, and the crop is in image pixels.
static float xToLon(int x) {
  return CHMU_LON_LEFT + (float)x * (CHMU_LON_RIGHT - CHMU_LON_LEFT) / (s_imgW - 1);
}
static float yToLat(int y) {
  float yt = mercTop(), yb = mercBottom();
  float m = yt - (float)y * (yt - yb) / (s_imgH - 1);
  return (2.0f * atanf(expf(m)) - 1.5707963268f) * 57.29577951f;
}

// Round display -> isotropic (square) crop.
static const float ASPECT = (float)LCD_WIDTH / (float)LCD_HEIGHT;   // 1.0

// Range 0 = the whole country: a fixed window that ignores the user's position.
static bool isWholeCz() { return currentRange() <= 0.0f; }

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
    lat = CZ_VIEW_LAT;
    lon = CZ_VIEW_LON;
    radiusKm = CZ_VIEW_RADIUS_KM;
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

// Projection callback for the CR outline + cities (same crop as the image).
static void borderProject(float lat, float lon, int* sx, int* sy) {
  int srcX = lonToX(lon), srcY = latToY(lat);
  *sx = (int)((int64_t)(srcX - s_crop.x1) * LCD_WIDTH  / cropW());
  *sy = (int)((int64_t)(srcY - s_crop.y1) * LCD_HEIGHT / cropH());
}

static int pngDraw(PNGDRAW* d) {
  int srcY = d->y;
  if (srcY < s_crop.y1 || srcY > s_crop.y2) return 1;
  if (!s_crop565 || !s_lineBuf) return 1;
  png.getLineAsRGB565(d, s_lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff);
  const int cw = cropW();
  uint16_t* row = s_crop565 + (int64_t)(srcY - s_crop.y1) * cw;
  for (int i = 0; i < cw; i++) {
    int srcX = s_crop.x1 + i;
    // Black for: past the edge of the image (the crop may now stick out), past
    // the data area on the right (colour scale), above the data area (title).
    bool have = (srcX >= 0 && srcX < s_imgW && srcX <= s_dataX1 && srcY >= s_dataY0);
    row[i] = have ? s_lineBuf[srcX] : 0x0000;
  }
  return 1;
}

// Stretch the current crop (s_crop565) over the display, masked to the circle
// (nearest neighbour). Pixels outside the round area are left black.
static void blitCrop() {
  if (!s_crop565) return;
  const int cw = cropW(), ch = cropH();
  const long R2 = (long)DISP_R * DISP_R;
  for (int dy = 0; dy < LCD_HEIGHT; dy++) {
    long ddy = dy - CY;
    int srcRow = clampI((int)((int64_t)dy * ch / LCD_HEIGHT), 0, ch - 1);
    const uint16_t* row = s_crop565 + (int64_t)srcRow * cw;
    for (int dx = 0; dx < LCD_WIDTH; dx++) {
      long ddx = dx - CX;
      if (ddx * ddx + ddy * ddy > R2) continue;   // outside the round display
      int srcCol = clampI((int)((int64_t)dx * cw / LCD_WIDTH), 0, cw - 1);
      gfx->drawPixel(dx, dy, row[srcCol]);
    }
  }
}

// -----------------------------------------------------------------------------
//  Build the crop of every frame (decode PNG -> s_frame565[f]).
// -----------------------------------------------------------------------------
static bool rebuildCrops() {
  int cnt = CHMU_AnimCount();
  if (cnt <= 0) { s_frameCount = 0; return false; }
  if (cnt > ANIM_FRAMES) cnt = ANIM_FRAMES;

  // Dimensions + width sanity check from the first frame.
  if (png.openRAM(CHMU_AnimData(0), CHMU_AnimSize(0), pngDraw) != PNG_SUCCESS) { s_frameCount = 0; return false; }
  s_imgW = png.getWidth(); s_imgH = png.getHeight();
  s_dataX1 = lonToX(CHMU_LON_DATA_RIGHT);   // right of this x = scale/edge
  s_dataY0 = latToY(CHMU_LAT_DATA_TOP);     // above this y = title
  int bufSize = png.getBufferSize();
  int pitch = (s_imgH > 0) ? bufSize / s_imgH : bufSize;
  png.close();
  if (2 * (pitch + 1) > (int)PNG_MAX_BUFFERED_PIXELS) { s_wide = true; s_frameCount = 0; return false; }
  s_wide = false;

  if (s_imgW > s_lineCap) {
    if (s_lineBuf) free(s_lineBuf);
    s_lineBuf = (uint16_t*)malloc((size_t)s_imgW * sizeof(uint16_t));
    s_lineCap = s_lineBuf ? s_imgW : 0;
  }
  if (!s_lineBuf) { s_frameCount = 0; return false; }

  // Size the buffers for the WIDEST range there is, not for the current one.
  // Switching from 25 km to the whole country used to mean freeing six small
  // blocks and asking for six of ~480 kB - a fine way to lose to PSRAM
  // fragmentation. Allocated once at the maximum, they are never touched again.
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
    // Rows of the crop that fall outside the image are never delivered by the
    // decoder at all, so they have to start out black rather than holding the
    // previous frame's pixels.
    memset(s_crop565, 0, (size_t)need * 2);
    if (png.openRAM(CHMU_AnimData(f), CHMU_AnimSize(f), pngDraw) != PNG_SUCCESS) { png.close(); break; }
    png.decode(nullptr, 0);
    png.close();
    okc++;
  }
  s_frameCount = okc;
  return okc > 0;
}

// Download frames + build crops. The fetch blocks for a few seconds, so the
// screen is repainted first WITH the "Nacitam" note on it - the last good
// animation stays visible the whole time instead of going black.
//
// If the download brings nothing (no link, server down, half-received frames),
// the previous frames are kept and only a small note appears. Blanking the
// screen would throw away data that is at most five minutes old and still
// perfectly readable.
static void loadAndBuild() {
  s_loading = true;
  ScreenWeather_Draw();          // last good frame + the note
  gfx->flush();

  const int prevCount = s_frameCount;   // what we already have on screen
  int n = CHMU_FetchAnim(ANIM_FRAMES);
  bool ok = (n > 0) && rebuildCrops();

  // rebuildCrops() zeroes the frame count when it gives up. If we had frames
  // before, put them back rather than leaving the screen empty.
  if (!ok && s_frameCount == 0 && prevCount > 0) s_frameCount = prevCount;

  s_loading  = false;
  s_lastFail = !ok;
  if (ok) s_status = "OK";
  else if (s_wide) s_status = "snimek moc siroky";
  else s_status = (s_frameCount > 0) ? "OK" : "Chyba stahovani";
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
      gfx->drawFastHLine(px - 8, py, 16, C_WHITE);
      gfx->drawFastVLine(px, py - 8, 16, C_WHITE);
    }
  }

  // Precipitation-intensity legend - dBZ / mm/h. Placed on the left, vertically
  // centred, so it stays inside the circle. Colours taken from the real CHMU
  // palette so they match the map.
  {
    static const uint16_t COL[6] = { 0xA000, 0xF800, 0xFC20, 0xE6E0, 0x05E0, 0x001F };
    static const char*    LBL[6] = { ">56 / >100", "52 / 65", "46 / 27", "40 / 12", "32 / 3.6", "20 / <1" };
    const int lx = 26, ly = 150, boxW = 96, boxH = 12 + 6 * 13 + 2;
    gfx->fillRect(lx - 2, ly - 2, boxW, boxH, C_BLACK);   // readability backing
    gfx->setTextSize(1); gfx->setTextColor(C_GRAY);
    gfx->setCursor(lx, ly); gfx->print("dBZ / mm/h");
    for (int i = 0; i < 6; i++) {
      int ry = ly + 12 + i * 13;
      gfx->fillRect(lx, ry, 9, 9, COL[i]);
      gfx->drawRect(lx, ry, 9, 9, C_DKGRAY);
      gfx->setTextColor(C_WHITE);
      gfx->setCursor(lx + 13, ry + 1); gfx->print(LBL[i]);
    }
  }

  // Clock + outside temperature, directly under the screen-selector dots.
  UI_DrawStatusLine(30);

  // Animation indicator - dots + the frame's time label. Moved down to make
  // room for the status line above it: dots at y=62, label at y=74.
  {
    int n = s_frameCount > 0 ? s_frameCount : 1;
    int minAgo = (n - 1 - s_curFrame) * 5;
    char lbl[20];
    String hhmm = (s_frameCount > 0) ? CHMU_AnimTimeText(s_curFrame) : String("");
    if (minAgo <= 0) snprintf(lbl, sizeof(lbl), "nyni  %s", hhmm.c_str());
    else             snprintf(lbl, sizeof(lbl), "-%d min  %s", minAgo, hhmm.c_str());
    gfx->fillRect(CX - 70, 52, 140, 34, C_BLACK);   // backing
    int gap = 16, totalW = (n - 1) * gap, sx0 = CX - totalW / 2, dy = 62;
    for (int i = 0; i < n; i++) {
      int x = sx0 + i * gap;
      if (i == s_curFrame) gfx->fillCircle(x, dy, 4, C_YELLOW);
      else                 gfx->drawCircle(x, dy, 4, C_GRAY);
    }
    UI_TextCenteredIn(lbl, 0, LCD_WIDTH, 74, C_CYAN, 1);

    // Small note under the frame indicator: a refresh in progress, or the fact
    // that the last one failed and what you are looking at is the older data.
    const char* note = s_loading  ? "nacitam novejsi snimky..."
                     : s_lastFail ? "bez spojeni, zobrazena starsi data"
                                  : nullptr;
    if (note) {
      gfx->fillRect(CX - 110, 86, 220, 10, C_BLACK);
      UI_TextCenteredIn(note, 0, LCD_WIDTH, 86, C_YELLOW, 1);
    }
  }

  // Range + indicator dots (bottom centre) with a backing.
  char rbuf[16];
  if (isWholeCz()) snprintf(rbuf, sizeof(rbuf), "cela CR");
  else             snprintf(rbuf, sizeof(rbuf), "%.0f km", currentRange());
  gfx->fillRect(CX - 46, LCD_HEIGHT - 74, 92, 40, C_BLACK);
  UI_TextCenteredIn(rbuf, 0, LCD_WIDTH, LCD_HEIGHT - 70, C_YELLOW, 2);
  int dotGap = 16, dotY = LCD_HEIGHT - 42, totalW = (RANGE_COUNT - 1) * dotGap;
  int startX = CX - totalW / 2;
  for (int i = 0; i < RANGE_COUNT; i++) {
    int x = startX + i * dotGap;
    if (i == s_rangeIdx) gfx->fillCircle(x, dotY, 3, C_YELLOW);
    else                 gfx->drawCircle(x, dotY, 3, C_GRAY);
  }
}

void ScreenWeather_Enter() {
  // Restore the last range. Do NOT force a rebuild here: a range change already
  // sets s_needRebuild in ScreenWeather_ChangeRange(), and the very first load
  // is handled by the s_frameCount == 0 branch in Tick(). Forcing it would
  // re-decode all six PNGs on every visit to this screen.
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

void ScreenWeather_ChangeRange(int dir) {
  s_rangeIdx = (s_rangeIdx + dir + RANGE_COUNT) % RANGE_COUNT;
  Settings_SetMeteoRange(s_rangeIdx);   // remember across restarts (debounced)
  s_needRebuild = true;   // re-crop the frames on the next tick
}

bool ScreenWeather_Tick() {
  if (WiFi.status() != WL_CONNECTED) { s_status = "Ceka na WiFi"; return false; }
  unsigned long now = millis();

  // Range changed -> just re-crop (frames already downloaded).
  if (s_needRebuild && CHMU_AnimCount() > 0) {
    rebuildCrops();
    s_needRebuild = false; s_curFrame = 0; s_gap = false; s_lastStep = now;
    return true;
  }

  // First load.
  if (s_frameCount == 0) {
    loadAndBuild();
    s_lastFetch = now; s_curFrame = 0; s_gap = false; s_lastStep = now;
    return true;
  }

  // Periodic refresh - only during the pause (last frame shown).
  if (s_gap && (now - s_lastFetch >= 300000UL)) {
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

  if (s_frameCount == 0) {
    // Nothing to show yet - this is the very first load, or every attempt so
    // far has failed. Once there is a frame we never come back here.
    UI_TextCentered("Meteoradar CHMU", LCD_HEIGHT / 2 - 20, C_WHITE, 2);
    const char* msg = s_loading ? "Nacitam animaci..."
                    : s_wide    ? "snimek moc siroky"
                                : s_status.c_str();
    UI_TextCentered(msg, LCD_HEIGHT / 2 + 6, C_YELLOW, 2);
    return;
  }

  int f = clampI(s_curFrame, 0, s_frameCount - 1);
  s_crop565 = s_frame565[f];
  blitCrop();

  // Same map data as the aircraft radar: European outlines and cities, drawn
  // through this screen's projection. The weather screen used to have its own
  // Czech-only outline, which was both redundant (the European set contains the
  // Czech border at the same fidelity) and misleading - at the widest range the
  // country hung in an empty void with no neighbours around it.
  //
  // EuBorder culls against a lat/lon window, so hand it the crop converted back
  // into degrees, with a small margin for lines that only clip the edge.
  {
    float lat1 = yToLat(s_crop.y1), lat0 = yToLat(s_crop.y2);   // y grows south
    float lon0 = xToLon(s_crop.x1), lon1 = xToLon(s_crop.x2);
    float mLat = (lat1 - lat0) * 0.1f, mLon = (lon1 - lon0) * 0.1f;
    lat0 -= mLat; lat1 += mLat; lon0 -= mLon; lon1 += mLon;

    EuBorder_Draw(borderProject, C_GRAY, lat0, lat1, lon0, lon1);

    float rng = currentRange();
    // Careful: range 0 means the WHOLE COUNTRY, not a tiny one - comparing it
    // numerically would light up every village on the widest view of all.
    bool    showFull = !isWholeCz() && (rng <= 50.0f);
    uint8_t maxTier;
    // Tier 3 pulls in the Czech district towns, which is what makes the map
    // over the republic look like it used to. Wider views drop back to tier 2,
    // otherwise the country turns into a wall of labels.
    if      (isWholeCz())   maxTier = 2;
    else if (rng <= 100.0f) maxTier = 3;
    else                    maxTier = 2;
    EuBorder_DrawCities(borderProject, CX, CY, DISP_R, C_WHITE, C_CYAN,
                        showFull, maxTier, lat0, lat1, lon0, lon1);
  }

  // Thin ring marking the edge of the round display.
  gfx->drawCircle(CX, CY, DISP_R, C_DKGRAY);

  drawOverlay();
}
