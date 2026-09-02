// =============================================================================
//  MeteoPlaneRadar
//  Airports - draw airport symbols and IATA labels on the radar map.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "EuBorder.h"   // ProjectFn

// Draw airport waypoints on the radar map. Airports are rendered as underlay
// (they do NOT claim Layout collision slots) so aircraft labels always win.
//
// rangeKm controls which tiers are drawn:
//   <= 100 km: tiers 1-3 (all airports)
//   <= 200 km: tiers 1-2 (hubs + regional)
//   >  200 km: tier 1 only (major hubs)
void Airports_Draw(ProjectFn project, int cx, int cy, int radius,
                   float rangeKm,
                   float lat0, float lat1, float lon0, float lon1);
