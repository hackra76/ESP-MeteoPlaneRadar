// MeteoPlaneRadar - vyvoj / development: chiptron.cz
// =============================================================================
//  MeteoPlaneRadar - meteoradar SHMU: stahovani srazkoveho kompozitu (Slovensko).
//  Rozhrani + kalibrace geografickych okraju + animace (vice ramcu).
// =============================================================================
#pragma once
#include <Arduino.h>

#define SHMU_API_URL  "https://www.shmu.sk/api/v1/meteo/getradardata"
#define SHMU_BASE_URL "https://www.shmu.sk/data/dataradary/data.cmax/"

// Geograficke ohraniceni CELEHO obrazku PNG (dle specifikace SHMU):
//   imageBounds = [[50.7, 23.79] , [46.05, 13.6]]
#define SHMU_LON_LEFT   13.600f
#define SHMU_LON_RIGHT  23.790f
#define SHMU_LAT_TOP    50.700f
#define SHMU_LAT_BOTTOM 46.050f

#define SHMU_MAX_PNG 131072      // max velikost jednoho PNG (~30-35 kB, rezerva)
#define SHMU_ANIM_MAX 6          // max poctu ramcu animace

void        SHMU_SetPollFn(void (*fn)());

// --- Animace: nejnovejsich wantN ramcu (5 min krok) ---
// Vraci pocet stazenych ramcu. Ramce jsou serazene 0 = nejstarsi ... N-1 = nyni.
int         SHMU_FetchAnim(int wantN);
int         SHMU_AnimCount();
uint8_t*    SHMU_AnimData(int i);
size_t      SHMU_AnimSize(int i);
String      SHMU_AnimTimeText(int i);   // HH:MM (lokalni cas snimku)
