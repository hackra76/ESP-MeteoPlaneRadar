// =============================================================================
//  MeteoPlaneRadar
//  PlanePhoto.h - Aircraft thumbnail photos fetched from Planespotters.net API
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#pragma once
#include <Arduino.h>

enum PhotoState : uint8_t {
  PHOTO_IDLE = 0,   // no aircraft selected
  PHOTO_WAIT,       // queued / downloading / decoding
  PHOTO_OK,         // decoded RGB565 image available in PSRAM
  PHOTO_NONE        // query completed, but no photo exists on Planespotters
};

void            PlanePhoto_Select(const char* reg, const char* hex);
void            PlanePhoto_Clear();
void            PlanePhoto_Tick();
bool            PlanePhoto_HasPending();
bool            PlanePhoto_TakeChanged();
PhotoState      PlanePhoto_GetState();
const char*     PlanePhoto_GetPhotographer();
const uint16_t* PlanePhoto_GetRgb565(int* outW, int* outH);
