// =============================================================================
//  MeteoPlaneRadar
//  Screen: settings - interface.
//
//  Deliberately short. Everything is configurable in the browser now, and a
//  round 480x480 panel with a finger on it is the worst place to type a
//  latitude or pick a colour. What stays here is what you want to change while
//  standing in front of the device - brightness, which way you are looking,
//  units, language - plus the address to reach the web UI at.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenSettings_Enter();
void ScreenSettings_Draw();
bool ScreenSettings_Tick();
bool ScreenSettings_HandleTap(int x, int y);

// Tells main that the user asked to forget the network and go back to the
// configuration access point.
bool ScreenSettings_WantsWifiReset();
void ScreenSettings_ClearWifiReset();
