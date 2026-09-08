// =============================================================================
//  MeteoPlaneRadar
//  Daily aircraft flight statistics tracking and history.
//
// =============================================================================
#pragma once
#include <Arduino.h>
#include "ADSB.h"

void     FlightStats_Init();
void     FlightStats_Update(const Aircraft* list, int count);
void     FlightStats_CheckMidnight();

uint32_t FlightStats_TodayCount();
float    FlightStats_MaxDistKm();
float    FlightStats_MaxSpeedKt();
const char* FlightStats_MaxSpeedCallsign();
float    FlightStats_MaxAltFt();
float    FlightStats_MinAltFt();
uint32_t FlightStats_TotalSightings();

void     FlightStats_Reset();
