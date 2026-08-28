// =============================================================================
//  MeteoPlaneRadar
//  Screen 3: Tactical / Combined view (live aircraft + live weather radar).
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenTactical_Draw();
void ScreenTactical_Enter();
bool ScreenTactical_Tick();
bool ScreenTactical_HandleTap(int x, int y);
void ScreenTactical_ChangeRange(int dir);
void ScreenTactical_RangeText(char* out, size_t cap);
bool ScreenTactical_DetailOpen();
void ScreenTactical_CloseDetail();
