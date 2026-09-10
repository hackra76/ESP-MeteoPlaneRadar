// =============================================================================
//  MeteoPlaneRadar
//  ScreenIss.h - International Space Station (ISS) mission control UI screen.
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenIss_Enter();
void ScreenIss_Draw();
bool ScreenIss_Tick();
bool ScreenIss_HandleTap(int x, int y);
