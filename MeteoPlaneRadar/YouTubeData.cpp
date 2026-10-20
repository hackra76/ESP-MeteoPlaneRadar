// =============================================================================
//  MeteoPlaneRadar
//  YouTubeData.cpp - YouTube Channel Analytics API client.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1
// =============================================================================
#include "YouTubeData.h"
#include "Net.h"
#include "AsyncCore.h"
#include "Settings.h"
#include "Watchdog.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include "SpiRamAllocator.h"

enum YouTubeStepState {
  YT_STATE_IDLE = 0,
  YT_STATE_CHANNEL,
  YT_STATE_VIDEO,
  YT_STATE_VIEWS
};

static YouTubeStats s_ytData;
static YouTubeStepState s_stepState = YT_STATE_IDLE;
static unsigned long s_lastStepTime = 0;

void YouTube_Init() {
  Async_LockYouTube();
  memset(&s_ytData, 0, sizeof(s_ytData));
  strncpy(s_ytData.statusMsg, "Waiting for WiFi...", sizeof(s_ytData.statusMsg) - 1);
  s_ytData.statusMsg[sizeof(s_ytData.statusMsg) - 1] = '\0';
  Async_UnlockYouTube();
  s_stepState = YT_STATE_CHANNEL;
  s_lastStepTime = 0;
}

void YouTube_RequestFetch() {
  s_stepState = YT_STATE_CHANNEL;
  s_lastStepTime = 0;
}

bool YouTube_IsBusy() {
  return (s_stepState != YT_STATE_IDLE);
}

bool YouTube_ReadyToStep() {
  return (s_stepState != YT_STATE_IDLE && (millis() - s_lastStepTime >= 350));
}

void YouTube_Reset() {
  s_stepState = YT_STATE_IDLE;
  s_lastStepTime = 0;
  Async_LockYouTube();
  s_ytData.fetching = false;
  Async_UnlockYouTube();
}

unsigned long YouTube_LastUpdated() {
  return s_ytData.lastUpdatedMs;
}

bool YouTube_GetData(YouTubeStats* out) {
  if (!out) return false;
  Async_LockYouTube();
  *out = s_ytData;
  bool valid = s_ytData.valid;
  Async_UnlockYouTube();
  return valid;
}

// URL encode helper for channel handle or ID
static String urlEncode(const char* str) {
  String encoded = "";
  if (!str) return encoded;
  char c;
  while ((c = *str++)) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

static String decodeHtmlEntities(const String& in) {
  String out = in;
  out.replace("&amp;", "&");
  out.replace("&#39;", "'");
  out.replace("&quot;", "\"");
  out.replace("&lt;", "<");
  out.replace("&gt;", ">");
  out.replace("&apos;", "'");
  return out;
}

bool YouTube_Step() {
  if (WiFi.status() != WL_CONNECTED) {
    s_stepState = YT_STATE_IDLE;
    return false;
  }

  const char* apiKey = Settings_YouTubeApiKey();
  const char* channel = Settings_YouTubeChannel();

  if (!apiKey || strlen(apiKey) == 0 || !channel || strlen(channel) == 0) {
    Async_LockYouTube();
    s_ytData.valid = false;
    s_ytData.fetching = false;
    strncpy(s_ytData.statusMsg, "Missing API Key", sizeof(s_ytData.statusMsg) - 1);
    s_ytData.statusMsg[sizeof(s_ytData.statusMsg) - 1] = '\0';
    Async_UnlockYouTube();
    s_stepState = YT_STATE_IDLE;
    return false;
  }

  if (s_stepState == YT_STATE_IDLE) {
    s_stepState = YT_STATE_CHANNEL;
  }

  switch (s_stepState) {
    // -------------------------------------------------------------------------
    // Step 1: Channel Statistics & Uploads Playlist ID
    // -------------------------------------------------------------------------
    case YT_STATE_CHANNEL: {
      s_lastStepTime = millis();
      Async_LockYouTube();
      s_ytData.fetching = true;
      Async_UnlockYouTube();

      String channelParam;
      if (channel[0] == '@') {
        channelParam = "forHandle=" + urlEncode(channel);
      } else if (strncmp(channel, "UC", 2) == 0) {
        channelParam = "id=" + urlEncode(channel);
      } else {
        channelParam = "forHandle=%40" + urlEncode(channel);
      }

      String url = "https://www.googleapis.com/youtube/v3/channels?part=snippet,statistics,contentDetails&" +
                   channelParam + "&key=" + urlEncode(apiKey);

      const char* body = nullptr;
      size_t bodyLen = 0;
      if (!Net_GetCString(url.c_str(), &body, "YouTube-Ch", nullptr, nullptr, 8000, 10000, &bodyLen) || !body) {
        Async_LockYouTube();
        s_ytData.fetching = false;
        strncpy(s_ytData.statusMsg, "HTTP Fetch Error", sizeof(s_ytData.statusMsg) - 1);
        s_ytData.statusMsg[sizeof(s_ytData.statusMsg) - 1] = '\0';
        Async_UnlockYouTube();
        s_stepState = YT_STATE_IDLE;
        return false;
      }

      JsonDocument chFilter;
      chFilter["items"][0]["id"] = true;
      chFilter["items"][0]["snippet"]["title"] = true;
      chFilter["items"][0]["statistics"]["subscriberCount"] = true;
      chFilter["items"][0]["statistics"]["viewCount"] = true;
      chFilter["items"][0]["statistics"]["videoCount"] = true;
      chFilter["items"][0]["contentDetails"]["relatedPlaylists"]["uploads"] = true;

      JsonDocument chDoc(SpiRamAllocator::instance());
      DeserializationError err = deserializeJson(chDoc, body, bodyLen, DeserializationOption::Filter(chFilter));
      if (err || !chDoc["items"] || chDoc["items"].size() == 0) {
        Serial.printf("YouTube: JSON error or empty channel items: %s\n", err ? err.c_str() : "No channel items");
        Async_LockYouTube();
        s_ytData.fetching = false;
        strncpy(s_ytData.statusMsg, "Channel Not Found", sizeof(s_ytData.statusMsg) - 1);
        s_ytData.statusMsg[sizeof(s_ytData.statusMsg) - 1] = '\0';
        Async_UnlockYouTube();
        s_stepState = YT_STATE_IDLE;
        return false;
      }

      JsonObject item = chDoc["items"][0];
      const char* chId = item["id"] | "";
      const char* title = item["snippet"]["title"] | "";
      const char* subsStr = item["statistics"]["subscriberCount"] | "0";
      const char* viewsStr = item["statistics"]["viewCount"] | "0";
      uint32_t vCount = item["statistics"]["videoCount"] | 0;
      const char* uploadsId = item["contentDetails"]["relatedPlaylists"]["uploads"] | "";
      const uint8_t vMode = Settings_YouTubeVideoMode();

      Async_LockYouTube();
      strncpy(s_ytData.channelId, chId, sizeof(s_ytData.channelId) - 1);
      s_ytData.channelId[sizeof(s_ytData.channelId) - 1] = '\0';
      String cleanTitle = decodeHtmlEntities(title);
      strncpy(s_ytData.channelTitle, cleanTitle.c_str(), sizeof(s_ytData.channelTitle) - 1);
      s_ytData.channelTitle[sizeof(s_ytData.channelTitle) - 1] = '\0';
      s_ytData.subscriberCount = strtoull(subsStr, nullptr, 10);
      s_ytData.totalViews = strtoull(viewsStr, nullptr, 10);
      s_ytData.videoCount = vCount;
      strncpy(s_ytData.uploadsPlaylistId, uploadsId, sizeof(s_ytData.uploadsPlaylistId) - 1);
      s_ytData.uploadsPlaylistId[sizeof(s_ytData.uploadsPlaylistId) - 1] = '\0';
      s_ytData.videoMode = vMode;
      Async_UnlockYouTube();

      // Channel data acquired. Advance to video step.
      s_stepState = YT_STATE_VIDEO;
      return false; // Intermediate step completed
    }

    // -------------------------------------------------------------------------
    // Step 2: Fetch Video ID & Title (Search or Uploads Playlist)
    // -------------------------------------------------------------------------
    case YT_STATE_VIDEO: {
      s_lastStepTime = millis();
      char chIdBuf[32];
      char uploadsIdBuf[48];
      uint8_t vMode;

      Async_LockYouTube();
      strncpy(chIdBuf, s_ytData.channelId, sizeof(chIdBuf) - 1);
      chIdBuf[sizeof(chIdBuf) - 1] = '\0';
      strncpy(uploadsIdBuf, s_ytData.uploadsPlaylistId, sizeof(uploadsIdBuf) - 1);
      uploadsIdBuf[sizeof(uploadsIdBuf) - 1] = '\0';
      vMode = s_ytData.videoMode;
      Async_UnlockYouTube();

      bool videoFound = false;
      String targetVideoId = "";
      String targetVideoTitle = "";

      // 1. Most viewed video via search API if selected and channel ID known
      if (vMode == YT_MODE_MOST_VIEWED && strlen(chIdBuf) > 0) {
        String searchUrl = "https://www.googleapis.com/youtube/v3/search?part=snippet&channelId=" +
                           urlEncode(chIdBuf) + "&order=viewCount&type=video&maxResults=1&key=" + urlEncode(apiKey);
        const char* searchBody = nullptr;
        size_t searchLen = 0;
        if (Net_GetCString(searchUrl.c_str(), &searchBody, "YouTube-Search", nullptr, nullptr, 8000, 10000, &searchLen) && searchBody) {
          JsonDocument sFilter;
          sFilter["items"][0]["snippet"]["title"] = true;
          sFilter["items"][0]["id"]["videoId"] = true;

          JsonDocument sDoc(SpiRamAllocator::instance());
          if (!deserializeJson(sDoc, searchBody, searchLen, DeserializationOption::Filter(sFilter))) {
            if (sDoc["items"] && sDoc["items"].size() > 0) {
              JsonObject vidItem = sDoc["items"][0];
              targetVideoTitle = (const char*)(vidItem["snippet"]["title"] | "");
              targetVideoId = (const char*)(vidItem["id"]["videoId"] | "");
              if (targetVideoId.length() > 0) {
                videoFound = true;
              }
            }
          }
        }
      }

      // 2. Fallback to latest video via uploads playlist
      if (!videoFound && strlen(uploadsIdBuf) > 0) {
        String plUrl = "https://www.googleapis.com/youtube/v3/playlistItems?part=snippet&playlistId=" +
                       urlEncode(uploadsIdBuf) + "&maxResults=1&key=" + urlEncode(apiKey);
        const char* plBody = nullptr;
        size_t plLen = 0;
        if (Net_GetCString(plUrl.c_str(), &plBody, "YouTube-PL", nullptr, nullptr, 8000, 10000, &plLen) && plBody) {
          JsonDocument plFilter;
          plFilter["items"][0]["snippet"]["title"] = true;
          plFilter["items"][0]["snippet"]["resourceId"]["videoId"] = true;

          JsonDocument plDoc(SpiRamAllocator::instance());
          if (!deserializeJson(plDoc, plBody, plLen, DeserializationOption::Filter(plFilter))) {
            if (plDoc["items"] && plDoc["items"].size() > 0) {
              JsonObject vidItem = plDoc["items"][0];
              targetVideoTitle = (const char*)(vidItem["snippet"]["title"] | "");
              targetVideoId = (const char*)(vidItem["snippet"]["resourceId"]["videoId"] | "");
              if (targetVideoId.length() > 0) {
                videoFound = true;
              }
            }
          }
        }
      }

      if (videoFound && targetVideoId.length() > 0) {
        Async_LockYouTube();
        String cleanVidTitle = decodeHtmlEntities(targetVideoTitle);
        strncpy(s_ytData.latestVideoTitle, cleanVidTitle.c_str(), sizeof(s_ytData.latestVideoTitle) - 1);
        s_ytData.latestVideoTitle[sizeof(s_ytData.latestVideoTitle) - 1] = '\0';
        strncpy(s_ytData.latestVideoId, targetVideoId.c_str(), sizeof(s_ytData.latestVideoId) - 1);
        s_ytData.latestVideoId[sizeof(s_ytData.latestVideoId) - 1] = '\0';
        Async_UnlockYouTube();

        // Video located. Advance to views step.
        s_stepState = YT_STATE_VIEWS;
        return false; // Intermediate step completed
      } else {
        // No video found, but channel stats are already valid
        Async_LockYouTube();
        s_ytData.latestVideoTitle[0] = '\0';
        s_ytData.latestVideoId[0] = '\0';
        s_ytData.latestVideoViews = 0;
        s_ytData.lastUpdatedMs = millis();
        s_ytData.valid = true;
        s_ytData.fetching = false;
        strncpy(s_ytData.statusMsg, "OK (No Videos)", sizeof(s_ytData.statusMsg) - 1);
        s_ytData.statusMsg[sizeof(s_ytData.statusMsg) - 1] = '\0';
        Async_UnlockYouTube();

        s_stepState = YT_STATE_IDLE;
        return true; // Channel data complete
      }
    }

    // -------------------------------------------------------------------------
    // Step 3: Fetch Video View Count
    // -------------------------------------------------------------------------
    case YT_STATE_VIEWS: {
      s_lastStepTime = millis();
      char targetVidId[24];
      Async_LockYouTube();
      strncpy(targetVidId, s_ytData.latestVideoId, sizeof(targetVidId) - 1);
      targetVidId[sizeof(targetVidId) - 1] = '\0';
      Async_UnlockYouTube();

      if (strlen(targetVidId) > 0) {
        String vidUrl = "https://www.googleapis.com/youtube/v3/videos?part=statistics&id=" +
                        urlEncode(targetVidId) + "&key=" + urlEncode(apiKey);
        const char* vidBody = nullptr;
        size_t vidLen = 0;
        if (Net_GetCString(vidUrl.c_str(), &vidBody, "YouTube-Vid", nullptr, nullptr, 8000, 10000, &vidLen) && vidBody) {
          JsonDocument vidFilter;
          vidFilter["items"][0]["statistics"]["viewCount"] = true;
          JsonDocument vidDoc(SpiRamAllocator::instance());
          if (!deserializeJson(vidDoc, vidBody, vidLen, DeserializationOption::Filter(vidFilter))) {
            if (vidDoc["items"] && vidDoc["items"].size() > 0) {
              const char* vViewsStr = vidDoc["items"][0]["statistics"]["viewCount"] | "0";
              Async_LockYouTube();
              s_ytData.latestVideoViews = strtoull(vViewsStr, nullptr, 10);
              Async_UnlockYouTube();
            }
          }
        }
      }

      Async_LockYouTube();
      s_ytData.lastUpdatedMs = millis();
      s_ytData.valid = true;
      s_ytData.fetching = false;
      strncpy(s_ytData.statusMsg, "OK", sizeof(s_ytData.statusMsg) - 1);
      s_ytData.statusMsg[sizeof(s_ytData.statusMsg) - 1] = '\0';
      Async_UnlockYouTube();

      s_stepState = YT_STATE_IDLE;

      Serial.printf("YouTube: Updated '%s' - Subs: %llu, Views: %llu, Latest: '%s' (%llu views)\n",
                    s_ytData.channelTitle, (unsigned long long)s_ytData.subscriberCount,
                    (unsigned long long)s_ytData.totalViews, s_ytData.latestVideoTitle,
                    (unsigned long long)s_ytData.latestVideoViews);
      return true; // Complete fetch cycle finished
    }

    default:
      s_stepState = YT_STATE_IDLE;
      return false;
  }
}
