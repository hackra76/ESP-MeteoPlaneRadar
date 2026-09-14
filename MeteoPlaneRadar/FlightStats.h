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

uint8_t  FlightStats_ScopeCount();
float    FlightStats_ScopeRangeKm(uint8_t scope);

uint32_t FlightStats_TodayCount(int scope = -1);
float    FlightStats_MaxDistKm(int scope = -1);
float    FlightStats_MaxSpeedKt(int scope = -1);
const char* FlightStats_MaxSpeedCallsign(int scope = -1);
float    FlightStats_MaxAltFt(int scope = -1);
float    FlightStats_MinAltFt(int scope = -1);
uint32_t FlightStats_TotalSightings(int scope = -1);

void     FlightStats_Reset(int scope = -1);

