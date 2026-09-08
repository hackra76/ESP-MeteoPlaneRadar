// =============================================================================
//  MeteoPlaneRadar
//  CHMU weather radar: composite download (Czech Republic).
//  Interface, geographic bounds calibration, and multi-frame animation.
// =============================================================================
#pragma once
#include <Arduino.h>

#define CHMU_INDEX_URL "https://opendata.chmi.cz/meteorology/weather/radar/composite/maxz/png/"

// Geographic bounds of the full PNG image (per CHMU documentation):
//   lon 11.267 - 20.770 ; lat 48.047 - 52.167
#define CHMU_LON_LEFT   11.267f
#define CHMU_LON_RIGHT  20.770f
#define CHMU_LAT_TOP    52.167f
#define CHMU_LAT_BOTTOM 48.047f

// Bounds of actual radar DATA (narrower than full image).
// The title bar on top and color scale on the right are masked out to avoid false echoes.
#define CHMU_LON_DATA_RIGHT 19.624f
#define CHMU_LAT_DATA_TOP   51.458f

#define CHMU_MAX_PNG 131072      // Max buffer for one PNG (~15-30 kB with margin)
#define CHMU_ANIM_MAX 6          // Max animation frames kept

void        CHMU_SetPollFn(void (*fn)());

// --- Animation: newest wantN frames (5 min intervals) ---
// Returns count of successfully loaded frames (0 = oldest, N-1 = newest).
int         CHMU_FetchAnim(int wantN);
int         CHMU_AnimCount();
uint8_t*    CHMU_AnimData(int i);
size_t      CHMU_AnimSize(int i);
String      CHMU_AnimTimeText(int i);   // HH:MM (local frame time)
void        CHMU_FreeBuffers();         // Free PSRAM buffers
