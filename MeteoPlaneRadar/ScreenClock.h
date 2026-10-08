// =============================================================================
//  MeteoPlaneRadar
//  Screen: clock - interface.
//
//  A large digital clock with the date, the current temperature and a seconds
//  ring around the rim. Everything it shows comes from sources the device polls
//  anyway (the HTTP Date header for the time, Open-Meteo for the weather), so
//  the screen adds no new dependency - and specifically no Home Assistant.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
// =============================================================================
#pragma once
#include <lvgl.h>

void ScreenClock_Init(lv_obj_t* parent);
void ScreenClock_FreeBuffers();

