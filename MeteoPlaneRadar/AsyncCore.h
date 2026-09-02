// =============================================================================
//  MeteoPlaneRadar
//  AsyncCore.h - Dual-Core FreeRTOS multitasking and cross-core synchronization.
//
//  Decouples the rendering and touch pipeline (Core 1) from all network I/O,
//  TLS handshakes, JSON parsing, PNG decoding, and WebServer serving (Core 0).
//
//  Author:  Petr / chiptron.cz & Antigravity
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (ESP32-S3R8, dual-core 240MHz)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Initialize and start the Core 0 background worker task.
void Async_Begin();

// Pause / resume the background worker (e.g. during OTA flash updates).
void Async_Pause();
void Async_Resume();
bool Async_IsPaused();

// Set active screen and parameters so Core 0 knows what to poll
void Async_SetActiveScreen(uint8_t screenIdx);
void Async_SetAdsbTarget(double lat, double lon, float rangeKm);

// Trigger on-demand fetches from Core 1
void Async_RequestAdsb();
void Async_RequestRadar();
void Async_RequestForecast();
void Async_RequestRoute(const char* callsign, float lat, float lon);

// Check and consume data-updated flags (called by UI screens on Core 1)
bool Async_TakeAdsbUpdated();
bool Async_TakeRadarUpdated();
bool Async_TakeForecastUpdated();
bool Async_TakeRouteUpdated();

// Global mutex helpers for thread-safe access to shared models
void Async_LockSettings();
void Async_UnlockSettings();

void Async_LockAdsb();
void Async_UnlockAdsb();

void Async_LockRadar();
void Async_UnlockRadar();

void Async_LockForecast();
void Async_UnlockForecast();

void Async_LockRoute();
void Async_UnlockRoute();
