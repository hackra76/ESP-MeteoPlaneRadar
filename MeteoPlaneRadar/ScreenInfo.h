// =============================================================================
//  MeteoPlaneRadar
//  Screen: Info & Statistics (flight traffic summary, system & network info).
//
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenInfo_Enter();
void ScreenInfo_Draw();
bool ScreenInfo_Tick();
bool ScreenInfo_HandleTap(int x, int y);
