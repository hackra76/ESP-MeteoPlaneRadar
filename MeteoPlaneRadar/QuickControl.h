// =============================================================================
//  MeteoPlaneRadar
//  QuickControl.h - Pull-down Control Center overlay for rapid settings & toggles.
// =============================================================================
#pragma once
#include <Arduino.h>

bool QuickControl_IsOpen();
void QuickControl_Open();
void QuickControl_Close();
void QuickControl_Toggle();

// Returns true if the tap was handled inside QuickControl (including close)
bool QuickControl_HandleTap(int x, int y, int currentScreen);

// Draws the Control Center overlay on top of the active screen
void QuickControl_Draw(int currentScreen);
