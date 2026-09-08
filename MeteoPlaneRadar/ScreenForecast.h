// =============================================================================
//  MeteoPlaneRadar
//  Screen: weather forecast - interface.
//
//  The next few hours on top, the next few days underneath, and a line of air
//  quality at the bottom. Everything from Open-Meteo: free, no key.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenForecast_Enter();
void ScreenForecast_Draw();
bool ScreenForecast_Tick();
