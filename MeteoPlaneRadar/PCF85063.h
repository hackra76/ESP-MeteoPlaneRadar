// =============================================================================
//  MeteoPlaneRadar
//  PCF85063 Real-Time Clock (I2C address 0x51)
// =============================================================================
#pragma once
#include <Arduino.h>
#include <time.h>

#define PCF85063_ADDR 0x51

bool PCF85063_Init();
bool PCF85063_ReadTime(struct tm* out);
bool PCF85063_SetTime(time_t epochUtc);
