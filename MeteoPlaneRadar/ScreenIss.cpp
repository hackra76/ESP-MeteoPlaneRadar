// =============================================================================
//  MeteoPlaneRadar
//  ScreenIss.cpp - International Space Station (ISS) mission control UI screen.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenIss.h"

static lv_obj_t* s_screenObj = nullptr;
static lv_timer_t* s_timer = nullptr;
#include "IssData.h"
#include "WorldMapData.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "PetDrawer.h"
#include "Config.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "AsyncCore.h"
#include "Settings.h"
#include <WiFi.h>
#include <math.h>

#define MAP_X 80
#define MAP_Y 138
#define MAP_W 320
#define MAP_H 160

static unsigned long s_lastDrawTick = 0;
static unsigned long s_lastTapTime = 0;

static inline bool getMapBit(int x, int y) {
  if (x < 0 || x >= WORLD_MAP_W || y < 0 || y >= WORLD_MAP_H) return false;
  int byteIdx = y * 45 + (x >> 3);
  return (pgm_read_byte(&WORLD_MAP_BITS[byteIdx]) >> (7 - (x & 7))) & 1;
}

static const char* getCompassDirection(float azDeg) {
  static const char* const DIRS[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };
  int idx = (int)((azDeg + 11.25f) / 22.5f) % 16;
  return DIRS[idx];
}


static void drawWorldMap(const IssData& iss) {
  // Outline card container
  gfx->fillRoundRect(MAP_X - 4, MAP_Y - 4, MAP_W + 8, MAP_H + 8, 8, RGB565(8, 16, 28));
  gfx->drawRoundRect(MAP_X - 4, MAP_Y - 4, MAP_W + 8, MAP_H + 8, 8, RGB565(35, 65, 95));

  // Solar sub-point for Day/Night terminator shading
  float solLatRad = iss.solarLat * (float)M_PI / 180.0f;
  float solLonRad = iss.solarLon * (float)M_PI / 180.0f;
  float sinSolLat = sinf(solLatRad);
  float cosSolLat = cosf(solLatRad);

  uint16_t lineBuf[MAP_W];

  for (int y = 0; y < MAP_H; y++) {
    float latDeg = 90.0f - (float)y / (float)(MAP_H - 1) * 180.0f;
    float latRad = latDeg * (float)M_PI / 180.0f;
    float sinLat = sinf(latRad);
    float cosLat = cosf(latRad);
    int srcY = (y * (WORLD_MAP_H - 1)) / (MAP_H - 1);

    for (int x = 0; x < MAP_W; x++) {
      float lonDeg = (float)x / (float)(MAP_W - 1) * 360.0f - 180.0f;
      float lonRad = lonDeg * (float)M_PI / 180.0f;

      float cosZenith = sinLat * sinSolLat + cosLat * cosSolLat * cosf(lonRad - solLonRad);
      bool isDay = (cosZenith > 0.0f);

      int srcX = (x * (WORLD_MAP_W - 1)) / (MAP_W - 1);
      bool isLand = getMapBit(srcX, srcY);

      if (isLand) {
        bool isCoast = (!getMapBit(srcX - 1, srcY) || !getMapBit(srcX + 1, srcY) ||
                        !getMapBit(srcX, srcY - 1) || !getMapBit(srcX, srcY + 1));
        if (isCoast) {
          lineBuf[x] = isDay ? RGB565(56, 189, 248) : RGB565(26, 95, 130);
        } else {
          lineBuf[x] = isDay ? RGB565(24, 52, 68) : RGB565(13, 26, 36);
        }
      } else {
        lineBuf[x] = isDay ? RGB565(10, 22, 38) : RGB565(4, 10, 18);
      }

      // Subtle dotted grid lines
      bool isEq = (y == (MAP_H / 2));
      bool isLatGrid = (y == (MAP_H / 4) || y == (3 * MAP_H / 4));
      bool isLonGrid = (x == (MAP_W / 6) || x == (2 * MAP_W / 6) || x == (3 * MAP_W / 6) ||
                        x == (4 * MAP_W / 6) || x == (5 * MAP_W / 6));

      if (isEq) {
        if ((x & 3) == 0) lineBuf[x] = RGB565(35, 65, 95);
      } else if (isLatGrid && (x & 7) == 0) {
        lineBuf[x] = RGB565(22, 40, 60);
      } else if (isLonGrid && (y & 7) == 0) {
        lineBuf[x] = RGB565(22, 40, 60);
      }
    }
    gfx->draw16bitRGBBitmap(MAP_X, MAP_Y + y, lineBuf, MAP_W, 1);
  }

  // Draw ISS orbit ground track (past 45 min to next 45 min)
  int prevX = -1, prevY = -1;
  for (int dt = -45; dt <= 45; dt += 2) {
    float o_lat = 0.0f, o_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &o_lat, &o_lon);

    int px = MAP_X + (int)((o_lon + 180.0f) / 360.0f * (MAP_W - 1));
    int py = MAP_Y + (int)((90.0f - o_lat) / 180.0f * (MAP_H - 1));

    if (prevX >= 0) {
      if (abs(px - prevX) < (MAP_W / 2)) {
        gfx->drawLine(prevX, prevY, px, py, RGB565(255, 180, 0));
        gfx->drawLine(prevX, prevY + 1, px, py + 1, RGB565(255, 180, 0));
      }
    }
    prevX = px;
    prevY = py;
  }

  // Next orbit pass (dashed forward track +45 to +90 min)
  prevX = -1; prevY = -1;
  for (int dt = 45; dt <= 92; dt += 2) {
    float o_lat = 0.0f, o_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &o_lat, &o_lon);

    int px = MAP_X + (int)((o_lon + 180.0f) / 360.0f * (MAP_W - 1));
    int py = MAP_Y + (int)((90.0f - o_lat) / 180.0f * (MAP_H - 1));

    if (prevX >= 0) {
      if (abs(px - prevX) < (MAP_W / 2) && ((dt / 2) & 1) == 0) {
        gfx->drawLine(prevX, prevY, px, py, RGB565(160, 115, 15));
      }
    }
    prevX = px;
    prevY = py;
  }

  // Current ISS Position
  int ix = MAP_X + (int)((iss.lon + 180.0f) / 360.0f * (MAP_W - 1));
  int iy = MAP_Y + (int)((90.0f - iss.lat) / 180.0f * (MAP_H - 1));

  // Ground Footprint Horizon Ring (~2200 km radius -> ~20 px)
  int fp_r = 20;
  uint16_t fp_col = iss.inRange ? RGB565(0, 255, 150) : RGB565(0, 200, 255);
  gfx->drawCircle(ix, iy, fp_r, fp_col);
  gfx->drawCircle(ix, iy, fp_r + 1, fp_col);

  // Observer Home Pin
  float homeLat = (float)Settings_Lat();
  float homeLon = (float)Settings_Lon();
  int hx = MAP_X + (int)((homeLon + 180.0f) / 360.0f * (MAP_W - 1));
  int hy = MAP_Y + (int)((90.0f - homeLat) / 180.0f * (MAP_H - 1));

  gfx->drawFastHLine(hx - 4, hy, 9, RGB565(255, 220, 0));
  gfx->drawFastVLine(hx, hy - 4, 9, RGB565(255, 220, 0));
  gfx->fillCircle(hx, hy, 2, C_RED);

  // ISS Satellite Icon (Solar arrays + Module)
  gfx->fillRect(ix - 6, iy - 2, 13, 5, C_WHITE);
  gfx->drawFastVLine(ix - 5, iy - 4, 9, RGB565(0, 220, 255));
  gfx->drawFastVLine(ix + 5, iy - 4, 9, RGB565(0, 220, 255));
  gfx->fillCircle(ix, iy, 2, C_RED);
}

struct Vec3f {
  float x, y, z;
};

static inline Vec3f latLonToVec3(float latDeg, float lonDeg) {
  float lr = latDeg * 0.0174532925f;
  float mr = lonDeg * 0.0174532925f;
  float cl = cosf(lr);
  return { cl * cosf(mr), cl * sinf(mr), sinf(lr) };
}

static void draw3DGlobe(const IssData& iss, bool centerHome) {
  // Determine globe center coordinates
  float cLat = centerHome ? (float)Settings_Lat() : iss.lat;
  float cLon = centerHome ? (float)Settings_Lon() : iss.lon;

  float t0 = cLat * 0.0174532925f;
  float p0 = cLon * 0.0174532925f;
  float ct = cosf(t0), st = sinf(t0);
  float cp = cosf(p0), sp = sinf(p0);

  Vec3f V_center = { ct * cp, ct * sp, st };
  Vec3f V_right  = { -sp,     cp,       0.0f };
  Vec3f V_up     = { -st * cp, -st * sp, ct };

  // Solar sub-point unit vector
  Vec3f S = latLonToVec3(iss.solarLat, iss.solarLon);

  const int X0 = 240;
  const int Y0 = 216;
  const int R  = 136;
  const float invR = 1.0f / (float)R;

  // Render Earth sphere line by line
  uint16_t lineBuf[310];

  for (int dy = -R; dy <= R; dy++) {
    int sy = Y0 + dy;
    float ny = -(float)dy * invR; // +ny = North
    float r2_max = 1.0f - ny * ny;
    if (r2_max <= 0.0f) continue;

    float maxNx = sqrtf(r2_max);
    int maxDx = (int)(maxNx * (float)R);
    if (maxDx > 140) maxDx = 140;
    int lineW = 2 * maxDx + 1;

    for (int dx = -maxDx; dx <= maxDx; dx++) {
      float nx = (float)dx * invR;
      float nz2 = r2_max - nx * nx;
      if (nz2 < 0.0f) {
        lineBuf[dx + maxDx] = C_BLACK;
        continue;
      }
      float nz = sqrtf(nz2);

      // Reconstruct surface point P
      float Px = nx * V_right.x + ny * V_up.x + nz * V_center.x;
      float Py = nx * V_right.y + ny * V_up.y + nz * V_center.y;
      float Pz = nx * V_right.z + ny * V_up.z + nz * V_center.z;

      // Latitude and Longitude
      float lat = asinf(fmaxf(-1.0f, fminf(1.0f, Pz))) * 57.2957795f;
      float lon = atan2f(Py, Px) * 57.2957795f;

      // Sample bitmap
      int srcX = (int)((lon + 180.0f) * (float)(WORLD_MAP_W - 1) / 360.0f);
      if (srcX < 0) srcX = 0; else if (srcX >= WORLD_MAP_W) srcX = WORLD_MAP_W - 1;
      int srcY = (int)((90.0f - lat) * (float)(WORLD_MAP_H - 1) / 180.0f);
      if (srcY < 0) srcY = 0; else if (srcY >= WORLD_MAP_H) srcY = WORLD_MAP_H - 1;

      bool isLand = getMapBit(srcX, srcY);

      // Solar angle: cos(zenith) = P . S
      float cosZenith = Px * S.x + Py * S.y + Pz * S.z;
      bool isDay = (cosZenith > 0.0f);
      float dayFactor = fmaxf(0.0f, fminf(1.0f, cosZenith * 3.5f + 0.35f));

      // Rayleigh atmospheric limb
      float limb = (1.0f - nz); // 0 at center, 1 at horizon edge

      uint16_t col;
      if (isLand) {
        bool isCoast = (!getMapBit(srcX - 1, srcY) || !getMapBit(srcX + 1, srcY) ||
                        !getMapBit(srcX, srcY - 1) || !getMapBit(srcX, srcY + 1));
        if (isCoast) {
          col = isDay ? RGB565(60, 200, 255) : RGB565(20, 80, 115);
        } else {
          uint8_t r = (uint8_t)(10 + 26 * dayFactor);
          uint8_t g = (uint8_t)(22 + 65 * dayFactor);
          uint8_t b = (uint8_t)(28 + 42 * dayFactor);
          col = RGB565(r, g, b);
        }
      } else {
        uint8_t r = (uint8_t)(4 + 14 * dayFactor);
        uint8_t g = (uint8_t)(10 + 32 * dayFactor);
        uint8_t b = (uint8_t)(20 + 70 * dayFactor);
        if (limb > 0.5f) {
          float lg = (limb - 0.5f) * 2.0f;
          g = (uint8_t)fminf(255.0f, g + 40.0f * lg);
          b = (uint8_t)fminf(255.0f, b + 80.0f * lg);
        }
        col = RGB565(r, g, b);
      }

      // Graticule: Equator and 30 deg parallels
      if (fabsf(lat) < 1.4f && (dx & 3) == 0) col = RGB565(45, 90, 130);
      else if ((fabsf(lat - 30.0f) < 1.0f || fabsf(lat + 30.0f) < 1.0f) && (dx & 7) == 0) col = RGB565(30, 60, 90);

      lineBuf[dx + maxDx] = col;
    }
    gfx->draw16bitRGBBitmap(X0 - maxDx, sy, lineBuf, lineW, 1);
  }

  // Multi-layer atmospheric limb glow rings
  gfx->drawCircle(X0, Y0, R + 1, RGB565(50, 150, 240));
  gfx->drawCircle(X0, Y0, R + 2, RGB565(25, 80, 170));
  gfx->drawCircle(X0, Y0, R + 3, RGB565(12, 40, 95));
  gfx->drawCircle(X0, Y0, R + 4, RGB565(4, 16, 45));

  // Projection helper for 3D sphere
  auto project3D = [&](float latDeg, float lonDeg, int& outX, int& outY, float& outZ) -> bool {
    Vec3f P = latLonToVec3(latDeg, lonDeg);
    outZ = P.x * V_center.x + P.y * V_center.y + P.z * V_center.z;
    float nx = P.x * V_right.x + P.y * V_right.y + P.z * V_right.z;
    float ny = P.x * V_up.x + P.y * V_up.y + P.z * V_up.z;
    outX = X0 + (int)(nx * (float)R);
    outY = Y0 - (int)(ny * (float)R);
    return (outZ > 0.0f);
  };

  // 1. Draw ISS Orbit Ground Track (-45 min to +45 min)
  int prevX = -1, prevY = -1;
  bool prevVis = false;
  for (int dt = -45; dt <= 45; dt += 2) {
    float o_lat = 0.0f, o_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &o_lat, &o_lon);
    int px, py;
    float pz;
    bool vis = project3D(o_lat, o_lon, px, py, pz);
    if (prevX >= 0 && vis && prevVis) {
      gfx->drawLine(prevX, prevY, px, py, RGB565(255, 180, 0));
      gfx->drawLine(prevX, prevY + 1, px, py + 1, RGB565(255, 180, 0));
    }
    prevX = px; prevY = py; prevVis = vis;
  }

  // 2. Next orbit pass (+45 to +90 min, dashed)
  prevX = -1; prevY = -1; prevVis = false;
  for (int dt = 45; dt <= 92; dt += 2) {
    float o_lat = 0.0f, o_lon = 0.0f;
    Iss_GetOrbitPoint((float)dt, &o_lat, &o_lon);
    int px, py;
    float pz;
    bool vis = project3D(o_lat, o_lon, px, py, pz);
    if (prevX >= 0 && vis && prevVis && ((dt / 2) & 1) == 0) {
      gfx->drawLine(prevX, prevY, px, py, RGB565(160, 115, 15));
    }
    prevX = px; prevY = py; prevVis = vis;
  }

  // 3. Observer Home Pin
  int hx, hy;
  float hz;
  bool hVis = project3D((float)Settings_Lat(), (float)Settings_Lon(), hx, hy, hz);
  if (hVis) {
    gfx->drawFastHLine(hx - 5, hy, 11, RGB565(255, 220, 0));
    gfx->drawFastVLine(hx, hy - 5, 11, RGB565(255, 220, 0));
    gfx->fillCircle(hx, hy, 2, C_RED);
    UI_Text("HOME", hx + 7, hy - 5, RGB565(255, 220, 0), 1);
  }

  // 4. ISS Satellite Position & Footprint Ring
  int ix, iy;
  float iz;
  bool iVis = project3D(iss.lat, iss.lon, ix, iy, iz);

  // Footprint ring (20 deg cone around ISS)
  Vec3f N = latLonToVec3(iss.lat, iss.lon);
  Vec3f ref = (fabsf(N.z) < 0.9f) ? Vec3f{ 0.0f, 0.0f, 1.0f } : Vec3f{ 1.0f, 0.0f, 0.0f };
  float Ux = ref.y * N.z - ref.z * N.y;
  float Uy = ref.z * N.x - ref.x * N.z;
  float Uz = ref.x * N.y - ref.y * N.x;
  float uLen = sqrtf(Ux * Ux + Uy * Uy + Uz * Uz);
  Ux /= uLen; Uy /= uLen; Uz /= uLen;
  float Wx = N.y * Uz - N.z * Uy;
  float Wy = N.z * Ux - N.x * Uz;
  float Wz = N.x * Uy - N.y * Ux;

  const float cosA = cosf(20.0f * 0.0174532925f);
  const float sinA = sinf(20.0f * 0.0174532925f);
  uint16_t fp_col = iss.inRange ? RGB565(0, 255, 150) : RGB565(0, 200, 255);

  int fPrevX = -1, fPrevY = -1;
  bool fPrevVis = false;
  int fFirstX = -1, fFirstY = -1;
  bool fFirstVis = false;

  for (int a = 0; a <= 36; a++) {
    float ang = (float)a * (2.0f * (float)M_PI / 36.0f);
    float ca = cosf(ang), sa = sinf(ang);
    Vec3f Q = {
      cosA * N.x + sinA * (ca * Ux + sa * Wx),
      cosA * N.y + sinA * (ca * Uy + sa * Wy),
      cosA * N.z + sinA * (ca * Uz + sa * Wz)
    };
    float qz = Q.x * V_center.x + Q.y * V_center.y + Q.z * V_center.z;
    float qnx = Q.x * V_right.x + Q.y * V_right.y + Q.z * V_right.z;
    float qny = Q.x * V_up.x + Q.y * V_up.y + Q.z * V_up.z;
    int qx = X0 + (int)(qnx * (float)R);
    int qy = Y0 - (int)(qny * (float)R);
    bool qVis = (qz > 0.0f);

    if (a == 0) { fFirstX = qx; fFirstY = qy; fFirstVis = qVis; }
    if (fPrevX >= 0 && qVis && fPrevVis) {
      gfx->drawLine(fPrevX, fPrevY, qx, qy, fp_col);
    }
    fPrevX = qx; fPrevY = qy; fPrevVis = qVis;
  }
  if (fFirstVis && fPrevVis && fPrevX >= 0) {
    gfx->drawLine(fPrevX, fPrevY, fFirstX, fFirstY, fp_col);
  }

  // Draw ISS Satellite Icon if on front hemisphere
  if (iVis) {
    gfx->fillRect(ix - 7, iy - 2, 15, 5, C_WHITE);
    gfx->drawFastVLine(ix - 6, iy - 5, 11, RGB565(0, 220, 255));
    gfx->drawFastVLine(ix + 6, iy - 5, 11, RGB565(0, 220, 255));
    gfx->fillCircle(ix, iy, 2, C_RED);
    UI_Text("ISS", ix + 10, iy - 6, RGB565(0, 255, 200), 1);
  } else {
    // Show back-side indicator badge inside top of globe
    UI_TextCentered("ISS ON FAR SIDE", Y0 - R + 20, RGB565(160, 195, 230), 1);
  }

  // Perspective Info Flanks (Center Lat/Lon coords) inside bottom of globe
  char cBuf[32];
  snprintf(cBuf, sizeof(cBuf), "CTR: %.1f°%c %.1f°%c",
           fabsf(cLat), cLat >= 0 ? 'N' : 'S',
           fabsf(cLon), cLon >= 0 ? 'E' : 'W');
  UI_TextCentered(cBuf, Y0 + R - 16, RGB565(120, 160, 200), 1);
}

static void ScreenIss_Draw() {
  gfx->fillScreen(C_BLACK);

  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  IssData iss;
  bool valid = Iss_GetData(&iss);

  uint8_t viewMode = Settings_IssViewMode();

  // 1. Draw World Map or Giant 3D Globe
  if (viewMode == ISS_VIEW_2D_MAP) {
    drawWorldMap(iss);
  } else {
    draw3DGlobe(iss, viewMode == ISS_VIEW_3D_HOME);
  }

  // 2. Outer Top Rim: Title & Status Dot
  const char* title = isEn ? "ISS ORBIT TRACKER" : (isSk ? "VESMÍRNA STANICA ISS" : "VESMÍRNÁ STANICE ISS");
  UI_TextCentered(title, 20, RGB565(0, 220, 255), 1);

  // Live status pulse dot at top center
  uint16_t dotCol = (valid && (millis() - iss.lastUpdatedMs < 60000))
                    ? (iss.inRange ? RGB565(0, 255, 120) : RGB565(0, 200, 255))
                    : RGB565(255, 180, 0);
  gfx->fillCircle(240, 8, 3, dotCol);

  // 3. Top-Left Peripheral Capsule: Altitude (hugging the 10:30 circular curve)
  char altBuf[24];
  if (valid) snprintf(altBuf, sizeof(altBuf), "%.0f km", iss.alt);
  else snprintf(altBuf, sizeof(altBuf), "---");
  int altX = 108, altY = 44, altW = 78, altH = 32;
  gfx->fillRoundRect(altX, altY, altW, altH, 6, RGB565(10, 20, 32));
  gfx->drawRoundRect(altX, altY, altW, altH, 6, RGB565(25, 50, 75));
  UI_TextCenteredIn(isEn ? "ALTITUDE" : "VÝŠKA", altX, altW, altY + 3, RGB565(120, 150, 180), 0);
  UI_TextCenteredIn(altBuf, altX, altW, altY + 15, RGB565(0, 220, 255), 1);

  // 4. Top-Right Peripheral Capsule: Velocity (inset to prevent round screen clipping)
  char velBuf[24];
  if (valid) snprintf(velBuf, sizeof(velBuf), "%.1fk", iss.velocity / 1000.0f);
  else snprintf(velBuf, sizeof(velBuf), "---");
  int velX = 294, velY = 44, velW = 78, velH = 32;
  gfx->fillRoundRect(velX, velY, velW, velH, 6, RGB565(10, 20, 32));
  gfx->drawRoundRect(velX, velY, velW, velH, 6, RGB565(25, 50, 75));
  UI_TextCenteredIn(isEn ? "VELOCITY" : (isSk ? "RÝCHLOSŤ" : "RYCHLOST"), velX, velW, velY + 3, RGB565(120, 150, 180), 0);
  UI_TextCenteredIn(velBuf, velX, velW, velY + 15, C_WHITE, 1);

  // 5. Left Flank Peripheral Capsule: Distance (hugging the 9:00 circular curve)
  char distBuf[24];
  if (valid) snprintf(distBuf, sizeof(distBuf), "%.0f km", iss.distanceKm);
  else snprintf(distBuf, sizeof(distBuf), "---");
  int distX = 6, distY = 206, distW = 80, distH = 44;
  gfx->fillRoundRect(distX, distY, distW, distH, 6, RGB565(8, 16, 28));
  gfx->drawRoundRect(distX, distY, distW, distH, 6, RGB565(25, 48, 72));
  UI_TextCenteredIn(isEn ? "DISTANCE" : (isSk ? "VZDIAL." : "VZDÁL."), distX, distW, distY + 4, RGB565(120, 145, 175), 0);
  UI_TextCenteredIn(distBuf, distX, distW, distY + 20, iss.inRange ? RGB565(0, 255, 180) : RGB565(0, 220, 255), 1);

  // 6. Right Flank Peripheral Capsule: View Mode Badge & Azimuth (hugging the 3:00 circular curve)
  const char* vmLabel = (viewMode == ISS_VIEW_3D_ISS) ? "3D ISS" :
                        (viewMode == ISS_VIEW_3D_HOME) ? "3D HOME" : "2D MAP";
  uint16_t vmBorder = (viewMode == ISS_VIEW_3D_ISS) ? RGB565(0, 200, 255) :
                      (viewMode == ISS_VIEW_3D_HOME) ? RGB565(255, 180, 0) : RGB565(70, 110, 150);

  int vmX = 394, vmY = 206, vmW = 80, vmH = 44;
  gfx->fillRoundRect(vmX, vmY, vmW, vmH, 6, RGB565(10, 20, 34));
  gfx->drawRoundRect(vmX, vmY, vmW, vmH, 6, vmBorder);
  UI_TextCenteredIn(vmLabel, vmX, vmW, vmY + 4, vmBorder, 1);

  char azBuf[24];
  snprintf(azBuf, sizeof(azBuf), "Az %.0f°", iss.azimuthDeg);
  UI_TextCenteredIn(azBuf, vmX, vmW, vmY + 24, RGB565(180, 210, 240), 0);

  // 7. Bottom Arc: In-Range Status Pill
  const char* stPill = "OUT OF RANGE";
  uint16_t stPillBg = RGB565(16, 24, 38);
  uint16_t stPillBorder = RGB565(35, 50, 70);
  uint16_t stPillFg = RGB565(140, 160, 180);
  if (iss.isOverhead) {
    stPill = isEn ? "OVERHEAD" : (isSk ? "PRIAMO NAD HLAVOU" : "PŘÍMO NAD HLAVOU");
    stPillBg = RGB565(160, 90, 0);
    stPillBorder = RGB565(255, 180, 0);
    stPillFg = C_WHITE;
  } else if (iss.inRange) {
    stPill = isEn ? "IN RANGE" : (isSk ? "V DOHĽADE" : "V DOHLEDU");
    stPillBg = RGB565(0, 110, 55);
    stPillBorder = RGB565(0, 220, 120);
    stPillFg = C_WHITE;
  } else {
    stPill = isEn ? "OUT OF RANGE" : (isSk ? "MIMO DOHĽAD" : "MIMO DOHLED");
    stPillBg = RGB565(14, 22, 34);
    stPillBorder = RGB565(35, 50, 70);
    stPillFg = RGB565(140, 160, 180);
  }

  int pillW = Layout_TextW(stPill, 1) + 16;
  int pillX = 240 - pillW / 2;
  int pillY = 358;
  int pillH = 20;
  gfx->fillRoundRect(pillX, pillY, pillW, pillH, 5, stPillBg);
  gfx->drawRoundRect(pillX, pillY, pillW, pillH, 5, stPillBorder);
  UI_TextCenteredBox(stPill, pillX, pillY, pillW, pillH, stPillFg, 1);

  // 8. Bottom Information: ISS Position (straight, aligned below status pill)
  char posBuf[48];
  if (valid) {
    snprintf(posBuf, sizeof(posBuf), isEn ? "ISS POS: %.1f°%c  %.1f°%c" : (isSk ? "POLOHA ISS: %.1f°%c  %.1f°%c" : "POLOHA ISS: %.1f°%c  %.1f°%c"),
             fabsf(iss.lat), iss.lat >= 0 ? 'N' : 'S',
             fabsf(iss.lon), iss.lon >= 0 ? 'E' : 'W');
  } else {
    snprintf(posBuf, sizeof(posBuf), isEn ? "ISS POS: ---" : (isSk ? "POLOHA ISS: ---" : "POLOHA ISS: ---"));
  }
  UI_TextCentered(posBuf, 384, RGB565(170, 210, 245), 1);

  // 9. Bottom Information: Straight Pass Prediction Line
  char passBuf[64];
  if (iss.inRange) {
    snprintf(passBuf, sizeof(passBuf), isEn ? "Pass in progress! Peak: %.0f°"
                                           : (isSk ? "Prelet prebieha! Max: %.0f°"
                                                   : "Přelet probíhá! Max: %.0f°"),
             iss.nextPassMaxEl > 0.0f ? iss.nextPassMaxEl : iss.elevationDeg);
  } else if (iss.nextPassMinutes > 0) {
    int hrs = iss.nextPassMinutes / 60;
    int mins = iss.nextPassMinutes % 60;
    if (hrs > 0) {
      snprintf(passBuf, sizeof(passBuf), isEn ? "Next pass in %dh %02dm (Max: %.0f°)"
                                             : (isSk ? "Najbližší prelet o %dh %02dm (Max: %.0f°)"
                                                     : "Nejbližší přelet za %dh %02dm (Max: %.0f°)"),
               hrs, mins, iss.nextPassMaxEl);
    } else {
      snprintf(passBuf, sizeof(passBuf), isEn ? "Next pass in %d min (Max: %.0f°)"
                                             : (isSk ? "Najbližší prelet o %d min (Max: %.0f°)"
                                                     : "Nejbližší přelet za %d min (Max: %.0f°)"),
               mins, iss.nextPassMaxEl);
    }
  } else {
    snprintf(passBuf, sizeof(passBuf), isEn ? "Calculating next orbital pass..."
                                           : (isSk ? "Výpočet najbližšieho preletu..."
                                                   : "Výpočet nejbližšího přeletu..."));
  }
  UI_TextCentered(passBuf, 408, RGB565(120, 180, 235), 1);

  // 10. Bottom Rim: Micro-hint
  const char* hint = isEn ? "Tap: 3D/2D View • Dbl-tap: Refresh"
                          : (isSk ? "Ťuk: 3D/2D Pohľad • Dvojklik: Obnova"
                                  : "Ťuk: 3D/2D Pohled • Dvojklik: Obnova");
  UI_TextCentered(hint, 432, RGB565(85, 115, 145), 0);
}

static bool ScreenIss_HandleTap(int x, int y) {
  unsigned long now = millis();
  if (now - s_lastTapTime < 350) {
    // Double tap -> force immediate refresh
    Buzzer_Play(BEEP_CLICK);
    Async_RequestIss();
    s_lastTapTime = 0;
    if (s_screenObj) lv_obj_invalidate(s_screenObj);
    return true;
  }
  s_lastTapTime = now;

  // Single tap on main globe area or view badge toggles view mode: 3D ISS -> 3D Home -> 2D Map
  if ((y >= 70 && y <= 375) || (x >= 380 && y >= 190 && y <= 270)) {
    uint8_t nextMode = (Settings_IssViewMode() + 1) % 3;
    Settings_SetIssViewMode(nextMode);
    Buzzer_Play(BEEP_CLICK);
    if (s_screenObj) lv_obj_invalidate(s_screenObj);
    return true;
  }
  return false;
}

static void ScreenIss_TimerCb(lv_timer_t* t) {
  if (UI_GetActiveScreen() != SCREEN_ISS_I) return;
  if (Async_TakeIssUpdated()) {
    lv_obj_invalidate(s_screenObj);
    return;
  }
  unsigned long now = millis();
  if (now - s_lastDrawTick >= 1000) {
    s_lastDrawTick = now;
    lv_obj_invalidate(s_screenObj);
  }
}

static void ScreenIss_EventCb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_DRAW_MAIN) {
    lv_layer_t* layer = lv_event_get_layer(e);
    gfx->setLayer(layer);
    ScreenIss_Draw();
    gfx->setLayer(nullptr);
  } else if (code == LV_EVENT_CLICKED) {
    if (UI_IsSwipeActive()) return;
    lv_indev_t* indev = lv_indev_active();
    if (indev) {
      lv_point_t pt;
      lv_indev_get_point(indev, &pt);
      ScreenIss_HandleTap(pt.x, pt.y);
    }
  } else if (code == LV_EVENT_SCREEN_LOAD_START) {
    Async_RequestIss();
  }
}

void ScreenIss_Init(lv_obj_t* parent) {
  s_screenObj = parent;
  lv_obj_set_size(s_screenObj, 480, 480);
  lv_obj_center(s_screenObj);
  lv_obj_set_scrollable(s_screenObj, false);
  lv_obj_set_clickable(s_screenObj, true);

  lv_obj_add_event_cb(s_screenObj, ScreenIss_EventCb, LV_EVENT_ALL, nullptr);

  s_timer = lv_timer_create(ScreenIss_TimerCb, 500, nullptr);
}
