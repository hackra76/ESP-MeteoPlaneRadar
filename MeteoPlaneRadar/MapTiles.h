// =============================================================================
//  MeteoPlaneRadar
//  MapTiles.h - Online and offline map underlay engine for aircraft radar.
//
//  Providers:
//    0: Vector (offline EuBorder country outlines and cities)
//    1: Esri World Dark Gray Canvas (clean dark basemap, unwatermarked)
//    2: OpenStreetMap Standard (full color roads, terrain and settlements)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Arduino_GFX_Library.h"

enum MapProvider : uint8_t {
  MAP_PROV_VECTOR = 0,    // Offline vector borders + cities (EuBorder)
  MAP_PROV_ESRI_DARK = 1, // Esri World Dark Gray Base
  MAP_PROV_OSM = 2        // OpenStreetMap Standard
};

// Initialize MapTiles subsystem (allocates 480x480 PSRAM buffer)
void MapTiles_Init();

// Free all map buffers and decoders to release PSRAM for OTA
void MapTiles_FreeBuffers();

// Configure the view target. Non-blocking.
void MapTiles_Begin(uint8_t provider, double lat, double lon, float rangeKm);

// Single incremental network / decode step executed on Core 0.
// Returns true when a tile was completed and screen should be refreshed.
bool MapTiles_Step();

// Status queries
bool MapTiles_Busy();
bool MapTiles_IsReady();
bool MapTiles_Failed();
uint8_t MapTiles_CurrentProvider();

// Web Mercator coordinate projection for active map view
void MapTiles_Project(float lat, float lon, int* sx, int* sy);

// Map window bounds in lat/lon
void MapTiles_Window(float* lat0, float* lat1, float* lon0, float* lon1);

// Effective radar radius in km on display
float MapTiles_EffectiveRadiusKm();

// Current Web Mercator zoom level
int MapTiles_Zoom();

// Blit the map bitmap directly onto gfx (fast single call)
void MapTiles_Draw(Arduino_GFX* g);

// Direct pointer to 480x480 RGB565 buffer in PSRAM
const uint16_t* MapTiles_GetBuffer();
