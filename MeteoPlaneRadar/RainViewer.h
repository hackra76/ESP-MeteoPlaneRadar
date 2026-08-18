// =============================================================================
//  MeteoPlaneRadar
//  RainViewer - the international precipitation radar.
//
//  Why a second source at all: the CHMU composite is sharper and it is the
//  right choice over the Czech Republic, but it simply has no data anywhere
//  else. Set the location to Vienna and the weather screen goes black. So the
//  user can switch to RainViewer, which is European/global, free and needs no
//  key - at the cost of a slightly coarser picture.
//
//  How it works, and why it looks nothing like CHMU.cpp:
//
//  RainViewer serves standard Web Mercator TILES (z/x/y, 256 px). Rather than
//  download a big image and crop it, we pick the zoom level at which one world
//  pixel IS one display pixel, so the tiles are copied in without any
//  resampling - sharper, and far cheaper than scaling every frame. The zoom is
//  a power of two, so the range you get is the nearest one available rather
//  than exactly the number on the screen; the effective radius is reported back
//  so the range readout can stay honest.
//
//  Fetching is INCREMENTAL. A six-frame animation over a 3x3 tile grid is 54
//  HTTPS requests - well over a minute if done in one go, with the screen
//  frozen throughout. Instead Step() fetches exactly one tile per call and the
//  newest frame is built first, so a usable picture appears within a couple of
//  seconds and the animation fills in behind it.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

// Point the module at a view. Cheap when nothing changed; otherwise it drops
// what it has and starts fetching the new one on the next Step().
// radiusKm <= 0 means the fixed whole-country view (CZ_VIEW_* in Config.h).
void RainViewer_Begin(double lat, double lon, float radiusKm, int wantFrames);

// Fetch the newest frames again for the SAME view. Used for the periodic
// refresh - nudging the requested radius to force Begin() to notice would work
// once and then drift, a hundredth of a kilometre at a time.
void RainViewer_Refresh();

// Do a little work: at most one HTTP request. Returns true when something
// changed on screen. Call from the weather screen's tick.
bool RainViewer_Step();

// Frames that are complete and safe to display, oldest first.
int  RainViewer_Count();

// A finished frame: LCD_WIDTH * LCD_HEIGHT RGB565 pixels, already in display
// coordinates. nullptr when that frame is not ready.
const uint16_t* RainViewer_Frame(int i);

// Local HH:MM of that frame, and how many minutes old it is.
String RainViewer_TimeText(int i);
int    RainViewer_MinutesAgo(int i);

// Projection matching the current view - for the borders and city labels.
void RainViewer_Project(float lat, float lon, int* sx, int* sy);

// The visible window in degrees, for culling the map data.
void RainViewer_Window(float* lat0, float* lat1, float* lon0, float* lon1);

// The radius the chosen zoom actually gives, in km. Differs from the requested
// one because zoom levels are powers of two.
float RainViewer_EffectiveRadiusKm();

bool RainViewer_Busy();      // still fetching
bool RainViewer_Failed();    // the last attempt brought nothing

void RainViewer_SetPollFn(void (*fn)());
