// =============================================================================
//  MeteoPlaneRadar
//  MapTiles.cpp - Online and offline map underlay engine for aircraft radar.
// =============================================================================
#include "MapTiles.h"
#include "Config.h"
#include "Net.h"
#include "Display_ST7701.h"
#include "AsyncCore.h"

#include <PNGdec.h>
#include <JPEGDEC.h>
#include <esp_heap_caps.h>
#include <math.h>

#define MAP_TILE_SIZE 256
#define MAP_MAX_TILE_PAYLOAD (96 * 1024)

static uint8_t   s_provider = MAP_PROV_VECTOR;
static double    s_centerLat = 0.0;
static double    s_centerLon = 0.0;
static float     s_reqRadiusKm = 25.0f;
static float     s_effRadiusKm = 25.0f;
static int       s_zoom = 10;
static double    s_originX = 0.0;
static double    s_originY = 0.0;

static int       s_tx0 = 0, s_ty0 = 0;
static int       s_txN = 0, s_tyN = 0;
static int       s_totalTiles = 0;
static int       s_currentTileIdx = 0;
static int       s_tileTry = 0;
static unsigned long s_retryAt = 0;

static bool      s_busy = false;
static bool      s_ready = false;
static bool      s_failed = false;

static uint16_t* s_mapBuf = nullptr;
static uint16_t* s_lineBuf = nullptr;
static uint8_t*  s_tilePayload = nullptr;
static size_t    s_tilePayloadLen = 0;

static int       s_tileDstX = 0;
static int       s_tileDstY = 0;

static PNG*     s_png = nullptr;
static JPEGDEC* s_jpeg = nullptr;
static uint32_t s_jobId = 0;

// --- Web Mercator Math -------------------------------------------------------
static inline double worldSize(int z) {
  return (double)MAP_TILE_SIZE * (double)(1UL << z);
}

static void lonLatToWorld(double lat, double lon, int z, double* wx, double* wy) {
  double s = worldSize(z);
  double la = lat * 0.017453292519943295;
  if (la >  1.4835) la =  1.4835;
  if (la < -1.4835) la = -1.4835;
  *wx = (lon + 180.0) / 360.0 * s;
  *wy = (1.0 - log(tan(0.7853981633974483 + la * 0.5)) / M_PI) * 0.5 * s;
}

static void worldToLonLat(double wx, double wy, int z, double* lat, double* lon) {
  double s = worldSize(z);
  *lon = wx / s * 360.0 - 180.0;
  double n = M_PI * (1.0 - 2.0 * wy / s);
  *lat = atan(sinh(n)) * 57.29577951308232;
}

// Pick the ideal zoom level for the given radar radius in km.
static void chooseZoom(double lat, float radiusKm) {
  const double wantRes = (double)radiusKm * 1000.0 / 230.0; // 230px radar radius
  const double baseRes = 156543.03392 * cos(lat * 0.017453292519943295);
  int z = (int)lround(log(baseRes / wantRes) / log(2.0));

  int minZ = 4, maxZ = 16;
  if (s_provider == MAP_PROV_OSM) maxZ = 17;
  if (z < minZ) z = minZ;
  if (z > maxZ) z = maxZ;

  s_zoom = z;
  s_effRadiusKm = (float)(230.0 * baseRes / (double)(1UL << z) / 1000.0);
}

// --- Decoder Callbacks -------------------------------------------------------
static int jpegDrawCallback(JPEGDRAW *pDraw) {
  if (!s_mapBuf) return 1;
  const int rowW = pDraw->iWidth;
  const int rowH = pDraw->iHeight;

  for (int r = 0; r < rowH; r++) {
    int screenY = s_tileDstY + pDraw->y + r;
    if (screenY < 0 || screenY >= LCD_HEIGHT) continue;

    int screenX = s_tileDstX + pDraw->x;
    int sx0 = 0;
    int sx1 = rowW - 1;
    if (screenX < 0) {
      sx0 = -screenX;
    }
    if (screenX + sx1 >= LCD_WIDTH) {
      sx1 = (LCD_WIDTH - 1) - screenX;
    }
    if (sx0 <= sx1) {
      uint16_t* dst = s_mapBuf + (size_t)screenY * LCD_WIDTH + (screenX + sx0);
      const uint16_t* src = pDraw->pPixels + r * rowW + sx0;
      memcpy(dst, src, (size_t)(sx1 - sx0 + 1) * sizeof(uint16_t));
    }
  }
  return 1;
}

static int pngDrawCallback(PNGDRAW *pDraw) {
  if (!s_mapBuf || !s_lineBuf || !s_png) return 0;
  int screenY = s_tileDstY + pDraw->y;
  if (screenY < 0 || screenY >= LCD_HEIGHT) return 1;

  s_png->getLineAsRGB565(pDraw, s_lineBuf, PNG_RGB565_LITTLE_ENDIAN, 0x00000000);

  int screenX = s_tileDstX;
  int sx0 = 0;
  int sx1 = pDraw->iWidth - 1;
  if (screenX < 0) {
    sx0 = -screenX;
  }
  if (screenX + sx1 >= LCD_WIDTH) {
    sx1 = (LCD_WIDTH - 1) - screenX;
  }
  if (sx0 <= sx1) {
    uint16_t* dst = s_mapBuf + (size_t)screenY * LCD_WIDTH + (screenX + sx0);
    const uint16_t* src = s_lineBuf + sx0;
    memcpy(dst, src, (size_t)(sx1 - sx0 + 1) * sizeof(uint16_t));
  }
  return 1;
}

static bool decodeTilePayload(const uint8_t* data, size_t len) {
  if (!data || len < 4) return false;

  // Check magic header: JPEG vs PNG
  if (data[0] == 0xFF && data[1] == 0xD8) {
    if (s_jpeg && s_jpeg->openRAM((uint8_t*)data, (int)len, jpegDrawCallback)) {
      s_jpeg->setPixelType(RGB565_LITTLE_ENDIAN);
      s_jpeg->decode(0, 0, 0);
      s_jpeg->close();
      return true;
    }
  } else if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
    if (s_png && s_png->openRAM((uint8_t*)data, len, pngDrawCallback) == PNG_SUCCESS) {
      s_png->decode(nullptr, 0);
      s_png->close();
      return true;
    }
  }
  return false;
}

// --- Public Subsystem Implementation -----------------------------------------
void MapTiles_Init() {
  if (!s_mapBuf) {
    s_mapBuf = (uint16_t*)heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (s_mapBuf) {
      memset(s_mapBuf, 0, LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t));
      Serial.println("MapTiles: Allocated 480x480 RGB565 map buffer in PSRAM");
    } else {
      Serial.println("MapTiles: Failed to allocate PSRAM map buffer!");
    }
  }

  if (!s_lineBuf) {
    s_lineBuf = (uint16_t*)heap_caps_malloc(MAP_TILE_SIZE * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_lineBuf) s_lineBuf = (uint16_t*)malloc(MAP_TILE_SIZE * sizeof(uint16_t));
  }

  if (!s_tilePayload) {
    s_tilePayload = (uint8_t*)heap_caps_malloc(MAP_MAX_TILE_PAYLOAD, MALLOC_CAP_SPIRAM);
    if (!s_tilePayload) s_tilePayload = (uint8_t*)malloc(MAP_MAX_TILE_PAYLOAD);
  }

  if (!s_png) {
    s_png = (PNG*)heap_caps_malloc(sizeof(PNG), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_png) memset(s_png, 0, sizeof(PNG));
  }

  if (!s_jpeg) {
    s_jpeg = (JPEGDEC*)heap_caps_malloc(sizeof(JPEGDEC), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_jpeg) memset(s_jpeg, 0, sizeof(JPEGDEC));
  }
}

void MapTiles_FreeBuffers() {
  s_busy = false;
  s_ready = false;
  s_failed = false;

  if (s_mapBuf) {
    heap_caps_free(s_mapBuf);
    s_mapBuf = nullptr;
  }
  if (s_lineBuf) {
    if (esp_ptr_external_ram(s_lineBuf)) heap_caps_free(s_lineBuf); else free(s_lineBuf);
    s_lineBuf = nullptr;
  }
  if (s_tilePayload) {
    if (esp_ptr_external_ram(s_tilePayload)) heap_caps_free(s_tilePayload); else free(s_tilePayload);
    s_tilePayload = nullptr;
    s_tilePayloadLen = 0;
  }
  if (s_png) {
    heap_caps_free(s_png);
    s_png = nullptr;
  }
  if (s_jpeg) {
    heap_caps_free(s_jpeg);
    s_jpeg = nullptr;
  }
}

void MapTiles_Begin(uint8_t provider, double lat, double lon, float rangeKm) {
  s_jobId++;
  MapTiles_Init();

  if (provider == MAP_PROV_VECTOR) {
    if (s_provider != MAP_PROV_VECTOR) {
      Net_SessionEnd();
    }
    s_provider = MAP_PROV_VECTOR;
    s_busy = false;
    s_ready = true;
    s_failed = false;
    return;
  }

  bool sameView = (s_provider == provider) &&
                  (fabs(lat - s_centerLat) < 1e-5) &&
                  (fabs(lon - s_centerLon) < 1e-5) &&
                  (fabsf(rangeKm - s_reqRadiusKm) < 0.1f);

  if (sameView && (s_ready || s_busy)) {
    return;
  }

  if (s_provider != provider) {
    Net_SessionEnd();
  }
  Net_SessionBegin();

  s_provider = provider;
  s_centerLat = lat;
  s_centerLon = lon;
  s_reqRadiusKm = rangeKm;

  chooseZoom(lat, rangeKm);

  double cx, cy;
  lonLatToWorld(lat, lon, s_zoom, &cx, &cy);
  s_originX = cx - (double)(LCD_WIDTH / 2);
  s_originY = cy - (double)(LCD_HEIGHT / 2);

  s_tx0 = (int)floor(s_originX / (double)MAP_TILE_SIZE);
  s_ty0 = (int)floor(s_originY / (double)MAP_TILE_SIZE);
  int tx1 = (int)floor((s_originX + (double)LCD_WIDTH - 1.0) / (double)MAP_TILE_SIZE);
  int ty1 = (int)floor((s_originY + (double)LCD_HEIGHT - 1.0) / (double)MAP_TILE_SIZE);

  s_txN = tx1 - s_tx0 + 1;
  s_tyN = ty1 - s_ty0 + 1;
  if (s_txN < 1) s_txN = 1;
  if (s_tyN < 1) s_tyN = 1;

  s_totalTiles = s_txN * s_tyN;
  s_currentTileIdx = 0;
  s_tileTry = 0;
  s_retryAt = 0;
  s_busy = true;
  s_ready = false;
  s_failed = false;

  // Clear map buffer with subtle dark tint for dark gray basemap, or black
  if (s_mapBuf) {
    uint16_t fillCol = (provider == MAP_PROV_ESRI_DARK) ? RGB565(30, 32, 34) : RGB565(0, 0, 0);
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
      s_mapBuf[i] = fillCol;
    }
  }

  Serial.printf("MapTiles: Provider %d, zoom %d, grid %dx%d (%d tiles), effRadius %.1f km\n",
                s_provider, s_zoom, s_txN, s_tyN, s_totalTiles, s_effRadiusKm);
}

bool MapTiles_Step() {
  if (!s_busy || s_provider == MAP_PROV_VECTOR) return false;
  if (s_currentTileIdx >= s_totalTiles) {
    s_busy = false;
    s_ready = true;
    Net_SessionEnd();
    return true;
  }

  // Backoff timer after failure
  if (s_retryAt && millis() < s_retryAt) {
    return false;
  }

  uint32_t myJob = s_jobId;
  uint8_t myProv = s_provider;
  int myZoom = s_zoom;
  int myTileIdx = s_currentTileIdx;

  const int gx = myTileIdx % s_txN;
  const int gy = myTileIdx / s_txN;
  const int tx = s_tx0 + gx;
  const int ty = s_ty0 + gy;

  const int worldDim = 1 << myZoom;
  int wrapX = tx % worldDim;
  if (wrapX < 0) wrapX += worldDim;

  if (ty < 0 || ty >= worldDim) {
    s_currentTileIdx++;
    return false;
  }

  s_tileDstX = (int)lround((double)tx * (double)MAP_TILE_SIZE - s_originX);
  s_tileDstY = (int)lround((double)ty * (double)MAP_TILE_SIZE - s_originY);

  char url[192];
  if (myProv == MAP_PROV_ESRI_DARK) {
    // Esri ArcGIS REST endpoint format: /tile/{z}/{y}/{x}
    snprintf(url, sizeof(url),
             "https://server.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Dark_Gray_Base/MapServer/tile/%d/%d/%d",
             myZoom, ty, wrapX);
  } else {
    // OpenStreetMap standard endpoint format: /{z}/{x}/{y}.png
    snprintf(url, sizeof(url),
             "https://tile.openstreetmap.org/%d/%d/%d.png",
             myZoom, wrapX, ty);
  }

  s_tilePayloadLen = 0;
  bool ok = Net_GetBinary(url, s_tilePayload, MAP_MAX_TILE_PAYLOAD, &s_tilePayloadLen, "MAPTILES");

  // If a new job was started while this download was in flight, abort cleanly
  if (myJob != s_jobId) {
    return false;
  }

  if (!ok || s_tilePayloadLen < 4) {
    s_tileTry++;
    s_retryAt = millis() + 1000;
    if (s_tileTry >= 2) {
      Serial.printf("MapTiles: Failed tile (%d/%d/%d) after retry\n", myZoom, wrapX, ty);
      s_currentTileIdx++;
      s_tileTry = 0;
      s_retryAt = 0;
      if (s_currentTileIdx >= s_totalTiles) {
        s_busy = false;
        s_ready = true;
        Net_SessionEnd();
      }
    }
    return false;
  }

  s_retryAt = 0;
  bool decoded = decodeTilePayload(s_tilePayload, s_tilePayloadLen);

  if (myJob != s_jobId) {
    return false;
  }

  Serial.printf("MapTiles: Tile %d/%d (%d/%d/%d) %u B decoded=%d\n",
                s_currentTileIdx + 1, s_totalTiles, myZoom, wrapX, ty,
                (unsigned)s_tilePayloadLen, (int)decoded);
  s_currentTileIdx++;
  s_tileTry = 0;
  if (s_currentTileIdx >= s_totalTiles) {
    s_busy = false;
    s_ready = true;
    Net_SessionEnd();
  }
  return decoded;
}

bool MapTiles_Busy()       { return s_busy; }
bool MapTiles_IsReady()    { return s_ready || (s_provider == MAP_PROV_VECTOR); }
bool MapTiles_Failed()     { return s_failed; }
uint8_t MapTiles_CurrentProvider() { return s_provider; }
float MapTiles_EffectiveRadiusKm() { return s_effRadiusKm; }
int MapTiles_Zoom()        { return s_zoom; }

void MapTiles_Project(float lat, float lon, int* sx, int* sy) {
  double wx, wy;
  lonLatToWorld(lat, lon, s_zoom, &wx, &wy);
  if (sx) *sx = (int)lround(wx - s_originX);
  if (sy) *sy = (int)lround(wy - s_originY);
}

void MapTiles_Window(float* lat0, float* lat1, float* lon0, float* lon1) {
  double laTL, loTL, laBR, loBR;
  worldToLonLat(s_originX, s_originY, s_zoom, &laTL, &loTL);
  worldToLonLat(s_originX + (double)LCD_WIDTH, s_originY + (double)LCD_HEIGHT, s_zoom, &laBR, &loBR);
  if (lat0) *lat0 = (float)laBR;
  if (lat1) *lat1 = (float)laTL;
  if (lon0) *lon0 = (float)loTL;
  if (lon1) *lon1 = (float)loBR;
}

void MapTiles_Draw(Arduino_GFX* g) {
  if (!g || !s_mapBuf) return;
  g->draw16bitRGBBitmap(0, 0, s_mapBuf, LCD_WIDTH, LCD_HEIGHT);
}

const uint16_t* MapTiles_GetBuffer() {
  return s_mapBuf;
}
