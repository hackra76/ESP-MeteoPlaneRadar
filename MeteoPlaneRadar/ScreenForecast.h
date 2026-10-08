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

#include <lvgl.h>

void ScreenForecast_Init(lv_obj_t* parent);
