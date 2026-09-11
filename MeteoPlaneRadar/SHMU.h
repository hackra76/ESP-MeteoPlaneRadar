// =============================================================================
//  MeteoPlaneRadar
//  SHMU weather radar: composite download (Slovakia).
//  Interface, geographic bounds calibration, and multi-frame animation.
// =============================================================================
#pragma once
#include <Arduino.h>

#define SHMU_API_URL  "https://www.shmu.sk/api/v1/meteo/getradardata"
#define SHMU_BASE_URL "https://www.shmu.sk/data/dataradary/data.cmax/"

// Geographic bounds of the full composite PNG (per SHMU specification):
//   imageBounds = [[50.7, 23.79], [46.05, 13.6]]
#define SHMU_LON_LEFT   13.600f
#define SHMU_LON_RIGHT  23.790f
#define SHMU_LAT_TOP    50.700f
#define SHMU_LAT_BOTTOM 46.050f

#define SHMU_MAX_PNG 262144      // Max buffer for one PNG (typically ~30-70 kB, up to ~180 kB during heavy rain across SK)
#define SHMU_ANIM_MAX 6          // Max animation frames kept

void        SHMU_SetPollFn(void (*fn)());

// --- Animation: newest wantN frames (5 min intervals) ---
// Returns count of successfully loaded frames (0 = oldest, N-1 = newest).
int         SHMU_FetchAnim(int wantN);
int         SHMU_AnimCount();
uint8_t*    SHMU_AnimData(int i);
size_t      SHMU_AnimSize(int i);
String      SHMU_AnimTimeText(int i);   // HH:MM (local frame time)
void        SHMU_FreeBuffers();         // Free PSRAM buffers
