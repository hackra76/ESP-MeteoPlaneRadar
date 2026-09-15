// =============================================================================
//  MeteoPlaneRadar
//  YouTubeData.h - YouTube Channel Analytics data model & API client.
//
//  Fetches subscriber count, total channel views, latest video title,
//  and latest video view count via YouTube Data API v3.
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

struct YouTubeStats {
  char     channelTitle[64];
  uint64_t subscriberCount;
  uint64_t totalViews;
  uint32_t videoCount;
  char     latestVideoTitle[96];
  uint64_t latestVideoViews;
  char     uploadsPlaylistId[48];
  char     latestVideoId[24];
  unsigned long lastUpdatedMs;
  bool     valid;
  bool     fetching;
  char     statusMsg[32];
};

void YouTube_Init();
bool YouTube_Step();
void YouTube_RequestFetch();
bool YouTube_IsBusy();
unsigned long YouTube_LastUpdated();
bool YouTube_GetData(YouTubeStats* out);
