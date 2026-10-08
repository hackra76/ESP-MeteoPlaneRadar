#include "ScreenSonar.h"
#include "ADSB.h"
#include "Settings.h"
#include "Config.h"
#include "UI.h"
#include "PetDrawer.h"
#include "Display_ST7701.h"
#include "Buzzer.h"
#include "Layout.h"
#include "AsyncCore.h"
#include "esp_heap_caps.h"

#include <math.h>
#include <stdlib.h>

#define R_CX (LCD_WIDTH / 2)
#define R_CY (LCD_HEIGHT / 2)
#define R_RADIUS 218

#define WF_COLS 360
#define WF_ROWS 180

static lv_obj_t* s_screenObj = nullptr;
static lv_timer_t* s_tickTimer = nullptr;
static float s_sweepAngle = 0.0f;

static uint16_t* s_sonarDrawBuf = nullptr;
static size_t s_sonarDrawBufSize = 0;

static uint16_t* s_wfBuffer = nullptr;
static int s_wfHead = 0;
static unsigned long s_lastWfStepMs = 0;

static const float SONAR_RANGES[] = { 25.0f, 50.0f, 100.0f, 200.0f };
#define SONAR_RANGE_COUNT (sizeof(SONAR_RANGES) / sizeof(SONAR_RANGES[0]))
static uint8_t s_rangeIdx = 2; // Default 100 km

void ScreenSonar_RangeText(char* out, size_t cap) {
  if (!out || !cap) return;
  snprintf(out, cap, "%.0f km", SONAR_RANGES[s_rangeIdx]);
}

void ScreenSonar_ChangeRange(int dir) {
  if (dir < 0) {
    if (s_rangeIdx > 0) s_rangeIdx--;
    else s_rangeIdx = SONAR_RANGE_COUNT - 1;
  } else {
    if (s_rangeIdx + 1 < SONAR_RANGE_COUNT) s_rangeIdx++;
    else s_rangeIdx = 0;
  }
  Buzzer_Play(BEEP_CLICK);
  if (s_screenObj) lv_obj_invalidate(s_screenObj);
}

void ScreenSonar_Enter() {
  Async_SetActiveScreen(SCREEN_SONAR_I);
  Async_RequestAdsb();
}

// -----------------------------------------------------------------------------
//  1. Classic Circular PPI Radar CRT View
// -----------------------------------------------------------------------------
static void drawClassicPPI(float currentRangeKm) {
  // 1. Phosphor Persistence Decaying Shadow (CRT sweep trail)
  const int TRAIL_SLICES = 36;
  const float TRAIL_SPAN_DEG = 72.0f;
  const float SLICE_DEG = TRAIL_SPAN_DEG / (float)TRAIL_SLICES;

  float prevRad = (s_sweepAngle - 90.0f) * 0.0174532925f;
  int prevTx = R_CX + (int)roundf(cosf(prevRad) * R_RADIUS);
  int prevTy = R_CY + (int)roundf(sinf(prevRad) * R_RADIUS);

  for (int i = 1; i <= TRAIL_SLICES; i++) {
    float trailAngle = s_sweepAngle - (i * SLICE_DEG);
    if (trailAngle < 0.0f) trailAngle += 360.0f;
    float trad = (trailAngle - 90.0f) * 0.0174532925f;
    int tx = R_CX + (int)roundf(cosf(trad) * R_RADIUS);
    int ty = R_CY + (int)roundf(sinf(trad) * R_RADIUS);

    float frac = (float)i / (float)TRAIL_SLICES;
    float decay = powf(1.0f - frac, 1.8f);

    uint8_t gVal = (uint8_t)(190.0f * decay);
    uint8_t rVal = (uint8_t)(25.0f * decay * decay);
    uint8_t bVal = (uint8_t)(35.0f * decay * decay);

    if (gVal > 2) {
      gfx->fillTriangle(R_CX, R_CY, prevTx, prevTy, tx, ty, RGB565(rVal, gVal, bVal));
    }

    prevTx = tx;
    prevTy = ty;
  }

  // 2. Main Sweep Beam
  float sweepRad = (s_sweepAngle - 90.0f) * 0.0174532925f;
  int ex = R_CX + (int)roundf(cosf(sweepRad) * R_RADIUS);
  int ey = R_CY + (int)roundf(sinf(sweepRad) * R_RADIUS);
  gfx->drawLine(R_CX, R_CY, ex, ey, RGB565(160, 255, 160));

  // 3. Concentric Range Rings
  for (int i = 1; i <= 4; i++) {
    int r = (R_RADIUS * i) / 4;
    uint16_t ringCol = (i == 4) ? RGB565(0, 100, 0) : RGB565(0, 50, 0);
    gfx->drawCircle(R_CX, R_CY, r, ringCol);

    if (i < 4) {
      char rBuf[12];
      snprintf(rBuf, sizeof(rBuf), "%.0f", (currentRangeKm * i) / 4.0f);
      UI_Text(rBuf, R_CX + 4, R_CY - r - 8, RGB565(0, 140, 0), 1);
    }
  }

  // 4. Crosshairs & Cardinal Markings
  gfx->drawFastVLine(R_CX, R_CY - R_RADIUS, R_RADIUS * 2, RGB565(0, 60, 0));
  gfx->drawFastHLine(R_CX - R_RADIUS, R_CY, R_RADIUS * 2, RGB565(0, 60, 0));

  for (int d = -R_RADIUS + 20; d <= R_RADIUS - 20; d += 20) {
    if (d == 0) continue;
    gfx->drawFastHLine(R_CX - 3, R_CY + d, 7, RGB565(0, 90, 0));
    gfx->drawFastVLine(R_CX + d, R_CY - 3, 7, RGB565(0, 90, 0));
  }

  UI_TextCentered("N", R_CY - R_RADIUS + 14, RGB565(0, 220, 0), 1);
  UI_TextCentered("S", R_CY + R_RADIUS - 14, RGB565(0, 170, 0), 1);
  UI_Text("W", R_CX - R_RADIUS + 8, R_CY - 4, RGB565(0, 170, 0), 1);
  UI_Text("E", R_CX + R_RADIUS - 16, R_CY - 4, RGB565(0, 170, 0), 1);

  // 5. Live Aircraft Contacts
  int count = ADSB_Count();
  const Aircraft* list = ADSB_List();
  double clat = Settings_Lat();
  double clon = Settings_Lon();
  float latRad = (float)clat * 0.0174532925f;
  float cosLat = cosf(latRad);

  int inRangeCount = 0;
  for (int i = 0; i < count; i++) {
    float dxKm = (float)((list[i].lon - clon) * 111.32 * cosLat);
    float dyKm = (float)((list[i].lat - clat) * 110.57);
    float distKm = sqrtf(dxKm * dxKm + dyKm * dyKm);
    if (distKm > currentRangeKm) continue;

    inRangeCount++;

    float azimuthDeg = atan2f(dxKm, dyKm) * 57.2957795f;
    if (azimuthDeg < 0.0f) azimuthDeg += 360.0f;

    float diff = s_sweepAngle - azimuthDeg;
    if (diff < 0.0f) diff += 360.0f;

    if (diff < 140.0f) {
      float scale = (float)R_RADIUS / currentRangeKm;
      int sx = R_CX + (int)(dxKm * scale);
      int sy = R_CY - (int)(dyKm * scale);

      float fraction = 1.0f - (diff / 140.0f);
      uint8_t intensity = (uint8_t)(255.0f * fraction);

      gfx->fillCircle(sx, sy, 4, RGB565(0, intensity, 0));
      uint8_t coreW = (uint8_t)(200.0f * fraction);
      gfx->fillCircle(sx, sy, 2, RGB565(coreW, 255, coreW));

      if (Settings_ShowLegends() && fraction > 0.45f && list[i].callsign[0] && list[i].callsign[0] != ' ') {
        UI_Text(list[i].callsign, sx + 7, sy - 5, RGB565(0, intensity, 0), 1);
        if (list[i].altFt > 0) {
          char altBuf[12];
          snprintf(altBuf, sizeof(altBuf), "FL%d", list[i].altFt / 100);
          UI_Text(altBuf, sx + 7, sy + 4, RGB565(0, (uint8_t)(intensity * 0.7f), 0), 1);
        }
      }
    }
  }

  // 6. Home Location
  gfx->drawCircle(R_CX, R_CY, 3, RGB565(0, 255, 100));
  gfx->fillCircle(R_CX, R_CY, 1, C_WHITE);

  // 7. HUD Title & Target Count (positioned within round screen radius)
  UI_Text("SONAR", 145, 34, RGB565(0, 200, 0), 1);
  char countBuf[24];
  snprintf(countBuf, sizeof(countBuf), "%d TGT", inRangeCount);
  UI_Text(countBuf, 210, 34, RGB565(0, 220, 100), 1);

  // View Badge Pill
  int pillX = 275, pillY = 30, pillW = 48, pillH = 18;
  gfx->fillRoundRect(pillX, pillY, pillW, pillH, 4, RGB565(10, 30, 45));
  gfx->drawRoundRect(pillX, pillY, pillW, pillH, 4, RGB565(0, 180, 220));
  UI_TextCenteredBox("PPI", pillX, pillY, pillW, pillH, RGB565(0, 220, 255), 1);
}

// -----------------------------------------------------------------------------
//  2. Split View (Upper PPI Radar Arc + Lower Acoustic Waterfall BTR)
// -----------------------------------------------------------------------------
static void drawSplitView(float currentRangeKm) {
  const int S_CX = 240;
  const int S_CY = 210;
  const int S_R  = 168;

  // 1. Semi-circular PPI Sweep (top half)
  const int TRAIL_SLICES = 28;
  const float TRAIL_SPAN_DEG = 60.0f;
  const float SLICE_DEG = TRAIL_SPAN_DEG / (float)TRAIL_SLICES;

  float prevRad = (s_sweepAngle - 90.0f) * 0.0174532925f;
  int prevTx = S_CX + (int)roundf(cosf(prevRad) * S_R);
  int prevTy = S_CY + (int)roundf(sinf(prevRad) * S_R);

  for (int i = 1; i <= TRAIL_SLICES; i++) {
    float trailAngle = s_sweepAngle - (i * SLICE_DEG);
    if (trailAngle < 0.0f) trailAngle += 360.0f;
    float trad = (trailAngle - 90.0f) * 0.0174532925f;
    int tx = S_CX + (int)roundf(cosf(trad) * S_R);
    int ty = S_CY + (int)roundf(sinf(trad) * S_R);

    // Limit to radar card
    if (prevTy <= S_CY + 15 && ty <= S_CY + 15) {
      float frac = (float)i / (float)TRAIL_SLICES;
      float decay = powf(1.0f - frac, 1.8f);
      uint8_t gVal = (uint8_t)(170.0f * decay);
      if (gVal > 2) {
        gfx->fillTriangle(S_CX, S_CY, prevTx, prevTy, tx, ty, RGB565(15, gVal, 25));
      }
    }
    prevTx = tx; prevTy = ty;
  }

  // Sweep beam
  float sweepRad = (s_sweepAngle - 90.0f) * 0.0174532925f;
  int ex = S_CX + (int)roundf(cosf(sweepRad) * S_R);
  int ey = S_CY + (int)roundf(sinf(sweepRad) * S_R);
  if (ey <= S_CY + 15) {
    gfx->drawLine(S_CX, S_CY, ex, ey, RGB565(140, 255, 140));
  }

  // Semi-circular Rings
  for (int i = 1; i <= 3; i++) {
    int r = (S_R * i) / 3;
    gfx->drawCircle(S_CX, S_CY, r, RGB565(0, 60, 0));
  }
  gfx->drawFastHLine(S_CX - S_R, S_CY, S_R * 2, RGB565(0, 70, 0));
  gfx->drawFastVLine(S_CX, S_CY - S_R, S_R, RGB565(0, 70, 0));

  // Contacts on Arc
  int count = ADSB_Count();
  const Aircraft* list = ADSB_List();
  double clat = Settings_Lat();
  double clon = Settings_Lon();
  float latRad = (float)clat * 0.0174532925f;
  float cosLat = cosf(latRad);

  int inRangeCount = 0;
  for (int i = 0; i < count; i++) {
    float dxKm = (float)((list[i].lon - clon) * 111.32 * cosLat);
    float dyKm = (float)((list[i].lat - clat) * 110.57);
    float distKm = sqrtf(dxKm * dxKm + dyKm * dyKm);
    if (distKm > currentRangeKm) continue;

    inRangeCount++;

    float azimuthDeg = atan2f(dxKm, dyKm) * 57.2957795f;
    if (azimuthDeg < 0.0f) azimuthDeg += 360.0f;

    float scale = (float)S_R / currentRangeKm;
    int sx = S_CX + (int)(dxKm * scale);
    int sy = S_CY - (int)(dyKm * scale);

    if (sy < S_CY + 12 && sx >= S_CX - S_R && sx <= S_CX + S_R) {
      gfx->fillCircle(sx, sy, 3, RGB565(0, 255, 60));
      gfx->fillCircle(sx, sy, 1, C_WHITE);
    }
  }

  // Home marker
  gfx->fillCircle(S_CX, S_CY, 2, RGB565(0, 255, 120));

  // Top Title (inset within safe circular screen diameter)
  UI_Text("PPI RADAR", 125, 34, RGB565(0, 200, 0), 1);
  char cBuf[24];
  snprintf(cBuf, sizeof(cBuf), "%d TGT", inRangeCount);
  UI_Text(cBuf, 212, 34, RGB565(0, 220, 100), 1);

  // View Badge Pill
  int spPillX = 275, spPillY = 30, spPillW = 56, spPillH = 18;
  gfx->fillRoundRect(spPillX, spPillY, spPillW, spPillH, 4, RGB565(10, 30, 45));
  gfx->drawRoundRect(spPillX, spPillY, spPillW, spPillH, 4, RGB565(0, 180, 220));
  UI_TextCenteredBox("SPLIT", spPillX, spPillY, spPillW, spPillH, RGB565(0, 220, 255), 1);

  // 2. Lower Acoustic Waterfall Spectrogram (BTR)
  const int WF_X = 70;
  const int WF_Y = 248;
  const int WF_W = 340;
  const int WF_H = 142;

  gfx->fillRoundRect(WF_X - 3, WF_Y - 3, WF_W + 6, WF_H + 6, 6, RGB565(3, 8, 14));
  gfx->drawRoundRect(WF_X - 3, WF_Y - 3, WF_W + 6, WF_H + 6, 6, RGB565(25, 48, 70));

  // Bearing header line
  UI_Text("000°N", WF_X + 2,   WF_Y - 13, RGB565(0, 180, 100), 1);
  UI_Text("090°E", WF_X + 80,  WF_Y - 13, RGB565(0, 180, 100), 1);
  UI_Text("180°S", WF_X + 160, WF_Y - 13, RGB565(0, 180, 100), 1);
  UI_Text("270°W", WF_X + 240, WF_Y - 13, RGB565(0, 180, 100), 1);
  UI_Text("360°",  WF_X + 308, WF_Y - 13, RGB565(0, 180, 100), 1);

  // Render waterfall rows (340 columns centered from 360-column buffer)
  if (s_wfBuffer) {
    size_t needed = (size_t)WF_W * WF_H * sizeof(uint16_t);
    if (!s_sonarDrawBuf || s_sonarDrawBufSize < needed) {
      if (s_sonarDrawBuf) heap_caps_free(s_sonarDrawBuf);
      s_sonarDrawBuf = (uint16_t*)heap_caps_malloc(needed, MALLOC_CAP_SPIRAM);
      s_sonarDrawBufSize = needed;
    }
    if (s_sonarDrawBuf) {
      for (int r = 0; r < WF_H; r++) {
        int bufRow = (s_wfHead - r + WF_ROWS) % WF_ROWS;
        memcpy(&s_sonarDrawBuf[r * WF_W], &s_wfBuffer[bufRow * WF_COLS + 10], WF_W * sizeof(uint16_t));
      }
      gfx->draw16bitRGBBitmap(WF_X, WF_Y, s_sonarDrawBuf, WF_W, WF_H);
    }
  }

  // Doppler Legend flanking the range indicator (inset to prevent round screen clipping)
  UI_Text("< CLOSING", 95, 396, RGB565(0, 220, 255), 1);
  UI_Text("OPENING >", 325, 396, RGB565(255, 170, 0), 1);
}

// -----------------------------------------------------------------------------
//  3. Full Acoustic Waterfall Spectrogram View (Bearing-Time Record)
// -----------------------------------------------------------------------------
static void drawFullWaterfall(float currentRangeKm) {
  const int WF_X = 95;
  const int WF_Y = 66;
  const int WF_W = 290;
  const int WF_H = 328;

  int count = ADSB_Count();
  const Aircraft* list = ADSB_List();
  double clat = Settings_Lat();
  double clon = Settings_Lon();
  float latRad = (float)clat * 0.0174532925f;
  float cosLat = cosf(latRad);

  int inRangeCount = 0;
  for (int i = 0; i < count; i++) {
    float dxKm = (float)((list[i].lon - clon) * 111.32 * cosLat);
    float dyKm = (float)((list[i].lat - clat) * 110.57);
    if ((dxKm * dxKm + dyKm * dyKm) <= (currentRangeKm * currentRangeKm)) {
      inRangeCount++;
    }
  }

  // Top Title Bar (inset within safe circular screen diameter)
  UI_Text("SONAR BTR", 115, 34, RGB565(0, 220, 120), 1);
  char countBuf[24];
  snprintf(countBuf, sizeof(countBuf), "%d TGT", inRangeCount);
  UI_Text(countBuf, 202, 34, RGB565(0, 200, 220), 1);

  // View Badge Pill
  int wfPillX = 260, wfPillY = 30, wfPillW = 84, wfPillH = 18;
  gfx->fillRoundRect(wfPillX, wfPillY, wfPillW, wfPillH, 4, RGB565(10, 30, 50));
  gfx->drawRoundRect(wfPillX, wfPillY, wfPillW, wfPillH, 4, RGB565(0, 180, 255));
  UI_TextCenteredBox("WATERFALL", wfPillX, wfPillY, wfPillW, wfPillH, RGB565(0, 220, 255), 1);

  // Bearing Ruler / Graticule
  UI_Text("000°N", WF_X + 2,   50, RGB565(0, 190, 120), 1);
  UI_Text("090°E", WF_X + 68,  50, RGB565(0, 190, 120), 1);
  UI_Text("180°S", WF_X + 134, 50, RGB565(0, 190, 120), 1);
  UI_Text("270°W", WF_X + 200, 50, RGB565(0, 190, 120), 1);
  UI_Text("360°",  WF_X + 258, 50, RGB565(0, 190, 120), 1);

  // Container box
  gfx->fillRoundRect(WF_X - 3, WF_Y - 3, WF_W + 6, WF_H + 6, 6, RGB565(2, 6, 12));
  gfx->drawRoundRect(WF_X - 3, WF_Y - 3, WF_W + 6, WF_H + 6, 6, RGB565(25, 50, 75));

  // Time ticks inside left margin of waterfall
  UI_Text("NOW",  WF_X + 4, WF_Y + 4,   RGB565(0, 220, 120), 0);
  UI_Text("-1m",  WF_X + 4, WF_Y + 70,  RGB565(120, 160, 190), 0);
  UI_Text("-2m",  WF_X + 4, WF_Y + 136, RGB565(120, 160, 190), 0);
  UI_Text("-3m",  WF_X + 4, WF_Y + 202, RGB565(120, 160, 190), 0);
  UI_Text("-4m",  WF_X + 4, WF_Y + 268, RGB565(120, 160, 190), 0);

  // Render 164 history rows scaled x2 vertically -> 328 px (offset 35 cols centered)
  if (s_wfBuffer) {
    size_t needed = (size_t)WF_W * 328 * sizeof(uint16_t);
    if (!s_sonarDrawBuf || s_sonarDrawBufSize < needed) {
      if (s_sonarDrawBuf) heap_caps_free(s_sonarDrawBuf);
      s_sonarDrawBuf = (uint16_t*)heap_caps_malloc(needed, MALLOC_CAP_SPIRAM);
      s_sonarDrawBufSize = needed;
    }
    if (s_sonarDrawBuf) {
      for (int r = 0; r < 164; r++) {
        int bufRow = (s_wfHead - r + WF_ROWS) % WF_ROWS;
        memcpy(&s_sonarDrawBuf[(r * 2) * WF_W], &s_wfBuffer[bufRow * WF_COLS + 35], WF_W * sizeof(uint16_t));
        memcpy(&s_sonarDrawBuf[(r * 2 + 1) * WF_W], &s_wfBuffer[bufRow * WF_COLS + 35], WF_W * sizeof(uint16_t));
      }
      gfx->draw16bitRGBBitmap(WF_X, WF_Y, s_sonarDrawBuf, WF_W, 328);
    }
  }

  // Doppler Legend flanking the range indicator (inset to prevent round screen clipping)
  UI_Text("< CLOSING", 95, 400, RGB565(0, 220, 255), 1);
  UI_Text("OPENING >", 325, 400, RGB565(255, 170, 0), 1);
}

void ScreenSonar_Draw() {
  // Background is already filled by LVGL (LV_OPA_COVER)
  const float currentRangeKm = SONAR_RANGES[s_rangeIdx];
  const uint8_t viewMode = Settings_SonarView();

  if (viewMode == SONAR_VIEW_SPLIT) {
    drawSplitView(currentRangeKm);
  } else if (viewMode == SONAR_VIEW_WATERFALL) {
    drawFullWaterfall(currentRangeKm);
  } else {
    drawClassicPPI(currentRangeKm);
  }

  // Range Indicator (Bottom Center)
  char rbuf[24];
  ScreenSonar_RangeText(rbuf, sizeof(rbuf));
  UI_DrawRangeIndicator(rbuf, s_rangeIdx, SONAR_RANGE_COUNT, Settings_ShowLegends());
}

static void draw_event_cb(lv_event_t* e) {
  lv_layer_t* layer = lv_event_get_layer(e);
  gfx->setLayer(layer);
  ScreenSonar_Draw();
  gfx->setLayer(nullptr);
}

static void tick_timer_cb(lv_timer_t* timer) {
  if (UI_GetActiveScreen() != SCREEN_SONAR_I) return;

  float prevAngle = s_sweepAngle;
  s_sweepAngle += 4.2f; // ~105 deg/sec at 25 fps
  bool wrapped = false;
  if (s_sweepAngle >= 360.0f) {
    s_sweepAngle -= 360.0f;
    wrapped = true;
  }

  // Update Waterfall history row periodically (~1.5 s)
  unsigned long now = millis();
  if (now - s_lastWfStepMs >= 1500) {
    s_lastWfStepMs = now;
    if (s_wfBuffer) {
      s_wfHead = (s_wfHead + 1) % WF_ROWS;
      uint16_t* curRow = &s_wfBuffer[s_wfHead * WF_COLS];

      // Deep ocean acoustic noise floor
      for (int c = 0; c < WF_COLS; c++) {
        uint8_t noise = (uint8_t)(rand() % 16 + 4);
        curRow[c] = RGB565(0, noise, noise + 6);
      }

      int count = ADSB_Count();
      if (count > 0) {
        const Aircraft* list = ADSB_List();
        double clat = Settings_Lat();
        double clon = Settings_Lon();
        float latRad = (float)clat * 0.0174532925f;
        float cosLat = cosf(latRad);
        float currentRangeKm = SONAR_RANGES[s_rangeIdx];

        for (int i = 0; i < count; i++) {
          float dxKm = (float)((list[i].lon - clon) * 111.32 * cosLat);
          float dyKm = (float)((list[i].lat - clat) * 110.57);
          float distKm = sqrtf(dxKm * dxKm + dyKm * dyKm);
          if (distKm > currentRangeKm || distKm < 0.1f) continue;

          float azimuthDeg = atan2f(dxKm, dyKm) * 57.2957795f;
          if (azimuthDeg < 0.0f) azimuthDeg += 360.0f;
          int colIdx = (int)roundf(azimuthDeg) % 360;

          // Radial velocity: vr = -(v . r_hat)
          float speedKt = (float)list[i].gsKt;
          float trackRad = (float)list[i].track * 0.0174532925f;
          float vx = speedKt * sinf(trackRad);
          float vy = speedKt * cosf(trackRad);
          float rx = dxKm / distKm;
          float ry = dyKm / distKm;
          float vr = -(vx * rx + vy * ry); // positive = closing, negative = opening

          uint16_t sigCol;
          if (vr > 25.0f) {
            // Doppler closing: Cyan / electric blue
            uint8_t val = (uint8_t)fminf(255.0f, 180.0f + vr * 0.25f);
            sigCol = RGB565(0, val, 255);
          } else if (vr < -25.0f) {
            // Doppler opening: Amber / orange
            uint8_t val = (uint8_t)fmaxf(60.0f, 200.0f + vr * 0.25f);
            sigCol = RGB565(255, val, 0);
          } else {
            // Tangential / crossing: CRT phosphor green
            sigCol = RGB565(0, 240, 60);
          }

          for (int d = -1; d <= 1; d++) {
            int ci = (colIdx + d + WF_COLS) % WF_COLS;
            curRow[ci] = sigCol;
          }
        }
      }
    }
  }

  // Trigger a single short click whenever the sweep arm passes over an aircraft
  if (Settings_SonarPing()) {
    int count = ADSB_Count();
    if (count > 0) {
      const Aircraft* list = ADSB_List();
      double clat = Settings_Lat();
      double clon = Settings_Lon();
      float latRad = (float)clat * 0.0174532925f;
      float cosLat = cosf(latRad);
      float currentRangeKm = SONAR_RANGES[s_rangeIdx];
      float maxDistSq = currentRangeKm * currentRangeKm;
      bool hit = false;

      for (int i = 0; i < count; i++) {
        float dxKm = (float)((list[i].lon - clon) * 111.32 * cosLat);
        float dyKm = (float)((list[i].lat - clat) * 110.57);
        if ((dxKm * dxKm + dyKm * dyKm) > maxDistSq) continue;

        float azimuthDeg = atan2f(dxKm, dyKm) * 57.2957795f;
        if (azimuthDeg < 0.0f) azimuthDeg += 360.0f;

        if (!wrapped) {
          if (azimuthDeg >= prevAngle && azimuthDeg < s_sweepAngle) {
            hit = true;
            break;
          }
        } else {
          if (azimuthDeg >= prevAngle || azimuthDeg < s_sweepAngle) {
            hit = true;
            break;
          }
        }
      }

      if (hit) {
        Buzzer_Play(BEEP_SONAR_PING);
      }
    }
  }

  if (s_screenObj) {
    lv_obj_invalidate(s_screenObj);
  }
}

static void touch_event_cb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    if (UI_IsSwipeActive()) return;
    lv_indev_t* indev = lv_indev_active();
    if (indev) {
      lv_point_t pt;
      lv_indev_get_point(indev, &pt);
      // Tapping near bottom range bar (y >= 420) changes range
      if (pt.y >= 420) {
        ScreenSonar_ChangeRange(1);
      } else if (pt.x < 370 || pt.y <= 65) {
        // Tapping screen center or top view mode badge cycles view mode
        uint8_t nextMode = (Settings_SonarView() + 1) % 3;
        Settings_SetSonarView(nextMode);
        Buzzer_Play(BEEP_CLICK);
        if (s_screenObj) lv_obj_invalidate(s_screenObj);
      }
    }
  }
}

void ScreenSonar_Init(lv_obj_t* parent) {
  s_screenObj = parent;
  lv_obj_set_size(s_screenObj, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_set_scrollbar_mode(s_screenObj, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_bg_color(s_screenObj, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(s_screenObj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(s_screenObj, 0, 0);
  lv_obj_set_scrollable(s_screenObj, false);

  lv_obj_set_clickable(s_screenObj, true);

  lv_obj_add_event_cb(s_screenObj, draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);
  lv_obj_add_event_cb(s_screenObj, touch_event_cb, LV_EVENT_CLICKED, NULL);

  // Allocate waterfall buffer in Octal PSRAM (130 KB)
  if (!s_wfBuffer) {
    s_wfBuffer = (uint16_t*)heap_caps_malloc(WF_COLS * WF_ROWS * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_wfBuffer) {
      for (int i = 0; i < WF_COLS * WF_ROWS; i++) {
        uint8_t noise = (uint8_t)(rand() % 16 + 4);
        s_wfBuffer[i] = RGB565(0, noise, noise + 6);
      }
    }
  }

  // 25 FPS rotation timer
  s_tickTimer = lv_timer_create(tick_timer_cb, 40, NULL);
}

void ScreenSonar_FreeBuffers() {
  if (s_sonarDrawBuf) {
    heap_caps_free(s_sonarDrawBuf);
    s_sonarDrawBuf = nullptr;
    s_sonarDrawBufSize = 0;
  }
  if (s_wfBuffer) {
    heap_caps_free(s_wfBuffer);
    s_wfBuffer = nullptr;
  }
}
