// =============================================================================
//  MeteoPlaneRadar
//  Hardware watchdog - interface.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"   // WDT_TIMEOUT_S

void Watchdog_Begin();
void Watchdog_RegisterTask();
void Watchdog_Feed();
