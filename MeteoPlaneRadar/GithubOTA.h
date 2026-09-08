// =============================================================================
//  MeteoPlaneRadar
//  GithubOTA.h - GitHub Releases Over-The-Air firmware update engine.
//
//  Checks repository releases, parses semantic version tags, and performs
//  secure streaming updates from GitHub release assets with real-time progress.
//
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (ESP32-S3R8)
// =============================================================================
#pragma once
#include <Arduino.h>

enum GithubOtaState : uint8_t {
  GH_OTA_IDLE = 0,
  GH_OTA_CHECKING,
  GH_OTA_UP_TO_DATE,
  GH_OTA_AVAILABLE,
  GH_OTA_DOWNLOADING,
  GH_OTA_FLASHING,
  GH_OTA_SUCCESS,
  GH_OTA_ERROR
};

void GithubOTA_Init();

// Asynchronous background check against GitHub API
bool GithubOTA_CheckAsync();

// Synchronous check (used by WebConfig /api/ota/check endpoint)
bool GithubOTA_CheckSync();

// Start flashing update asynchronously
bool GithubOTA_StartUpdateAsync(const char* url = nullptr, const char* tag = nullptr);

// State & progress inspection
GithubOtaState GithubOTA_GetState();
const char*    GithubOTA_GetStateStr();
bool           GithubOTA_IsUpdateAvailable();
bool           GithubOTA_IsBusy();

int            GithubOTA_GetProgress();       // 0 - 100 %
size_t         GithubOTA_GetBytesWritten();
size_t         GithubOTA_GetTotalBytes();

const char*    GithubOTA_GetLatestVersion();
const char*    GithubOTA_GetReleaseTitle();
const char*    GithubOTA_GetReleaseBody();
const char*    GithubOTA_GetDownloadUrl();
const char*    GithubOTA_GetError();

// Reset state back to IDLE (or clear error)
void           GithubOTA_Reset();
