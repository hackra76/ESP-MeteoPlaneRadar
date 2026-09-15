// =============================================================================
//  MeteoPlaneRadar
//  Screen: spot electricity price as a 24-hour dial - interface.
//
//  The round panel is the reason this screen exists. A day of prices is a
//  circle of twenty-four sectors that reads exactly like a clock face: the
//  needle is now, the green side of the ring is when to run the washing
//  machine, the red side is when not to. On a rectangular display the same
//  data is a bar chart nobody looks at twice.
//
//  Controls:
//    tap on the ring    - read one hour out: its price fills the middle
//    tap in the middle  - switch between today and tomorrow
//    swipe              - the same switch (the screen's "range")
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenPrice_Enter();
void ScreenPrice_Draw();
bool ScreenPrice_Tick();
bool ScreenPrice_HandleTap(int x, int y);

// Swipe: today <-> tomorrow. Named like the radar screens' range so the screen
// manager can treat it the same way.
void ScreenPrice_ChangeRange(int dir);

// "dnes" / "zitra" for the web remote control's range readout.
void ScreenPrice_RangeText(char* out, size_t cap);
