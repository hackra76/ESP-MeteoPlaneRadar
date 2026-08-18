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
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenClock_Enter();
void ScreenClock_Draw();
bool ScreenClock_Tick();                  // true = needs a redraw
bool ScreenClock_HandleTap(int x, int y); // toggles night mode when auto is off
