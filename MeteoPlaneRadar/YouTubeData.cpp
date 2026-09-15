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
#include <WiFi.h>
#include <ArduinoJson.h>

static YouTubeStats s_ytData;
static bool s_ytBusy = false;
static bool s_forceFetch = true;
static unsigned long s_lastFetch = 0;

void YouTube_Init() {
  memset(&s_ytData, 0, sizeof(s_ytData));
  strncpy(s_ytData.statusMsg, "Waiting for WiFi...", sizeof(s_ytData.statusMsg));
  s_forceFetch = true;
}

void YouTube_RequestFetch() {
  s_forceFetch = true;
}

bool YouTube_IsBusy() {
  return s_ytBusy;
}

unsigned long YouTube_LastUpdated() {
  return s_ytData.lastUpdatedMs;
}

bool YouTube_GetData(YouTubeStats* out) {
  if (!out) return false;
  Async_LockSettings(); // Re-use settings mutex or safe local copy
  *out = s_ytData;
  Async_UnlockSettings();
  return s_ytData.valid;
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

bool YouTube_Step() {
  if (WiFi.status() != WL_CONNECTED) return false;

  const char* apiKey = Settings_YouTubeApiKey();
  const char* channel = Settings_YouTubeChannel();

  if (!apiKey || strlen(apiKey) == 0 || !channel || strlen(channel) == 0) {
    s_ytData.valid = false;
    strncpy(s_ytData.statusMsg, "Missing API Key", sizeof(s_ytData.statusMsg));
    return false;
  }

  s_ytBusy = true;
  s_ytData.fetching = true;

  // 1. Channel Statistics & Uploads Playlist ID
  String channelParam;
  if (channel[0] == '@') {
    channelParam = "forHandle=" + urlEncode(channel);
  } else if (strncmp(channel, "UC", 2) == 0) {
    channelParam = "id=" + urlEncode(channel);
  } else {
    // If entered without '@', prepend '@' for handle lookup or pass directly
    channelParam = "forHandle=%40" + urlEncode(channel);
  }

  String url = "https://www.googleapis.com/youtube/v3/channels?part=snippet,statistics,contentDetails&" +
               channelParam + "&key=" + urlEncode(apiKey);

  String body;
  if (!Net_GetString(url.c_str(), body, "YouTube")) {
    s_ytBusy = false;
    s_ytData.fetching = false;
    strncpy(s_ytData.statusMsg, "HTTP Fetch Error", sizeof(s_ytData.statusMsg));
    return false;
  }

  JsonDocument chFilter;
  chFilter["items"][0]["snippet"]["title"] = true;
  chFilter["items"][0]["statistics"]["subscriberCount"] = true;
  chFilter["items"][0]["statistics"]["viewCount"] = true;
  chFilter["items"][0]["statistics"]["videoCount"] = true;
  chFilter["items"][0]["contentDetails"]["relatedPlaylists"]["uploads"] = true;

  JsonDocument chDoc;
  DeserializationError err = deserializeJson(chDoc, body, DeserializationOption::Filter(chFilter));
  if (err || !chDoc["items"] || chDoc["items"].size() == 0) {
    Serial.printf("YouTube: JSON error or empty items: %s\n", err ? err.c_str() : "No channel items");
    s_ytBusy = false;
    s_ytData.fetching = false;
    strncpy(s_ytData.statusMsg, "Channel Not Found", sizeof(s_ytData.statusMsg));
    return false;
  }

  JsonObject item = chDoc["items"][0];
  const char* title = item["snippet"]["title"] | "";
  const char* subsStr = item["statistics"]["subscriberCount"] | "0";
  const char* viewsStr = item["statistics"]["viewCount"] | "0";
  uint32_t vCount = item["statistics"]["videoCount"] | 0;
  const char* uploadsId = item["contentDetails"]["relatedPlaylists"]["uploads"] | "";

  Async_LockSettings();
  strncpy(s_ytData.channelTitle, title, sizeof(s_ytData.channelTitle) - 1);
  s_ytData.channelTitle[sizeof(s_ytData.channelTitle) - 1] = '\0';
  s_ytData.subscriberCount = strtoull(subsStr, nullptr, 10);
  s_ytData.totalViews = strtoull(viewsStr, nullptr, 10);
  s_ytData.videoCount = vCount;
  strncpy(s_ytData.uploadsPlaylistId, uploadsId, sizeof(s_ytData.uploadsPlaylistId) - 1);
  s_ytData.uploadsPlaylistId[sizeof(s_ytData.uploadsPlaylistId) - 1] = '\0';
  Async_UnlockSettings();

  // 2. Fetch Latest Uploaded Video (if uploads playlist exists)
  if (strlen(uploadsId) > 0) {
    String plUrl = "https://www.googleapis.com/youtube/v3/playlistItems?part=snippet&playlistId=" +
                   urlEncode(uploadsId) + "&maxResults=1&key=" + urlEncode(apiKey);
    String plBody;
    if (Net_GetString(plUrl.c_str(), plBody, "YouTube-PL")) {
      JsonDocument plFilter;
      plFilter["items"][0]["snippet"]["title"] = true;
      plFilter["items"][0]["snippet"]["resourceId"]["videoId"] = true;

      JsonDocument plDoc;
      if (!deserializeJson(plDoc, plBody, DeserializationOption::Filter(plFilter))) {
        if (plDoc["items"] && plDoc["items"].size() > 0) {
          JsonObject vidItem = plDoc["items"][0];
          const char* vTitle = vidItem["snippet"]["title"] | "";
          const char* vId = vidItem["snippet"]["resourceId"]["videoId"] | "";

          Async_LockSettings();
          strncpy(s_ytData.latestVideoTitle, vTitle, sizeof(s_ytData.latestVideoTitle) - 1);
          s_ytData.latestVideoTitle[sizeof(s_ytData.latestVideoTitle) - 1] = '\0';
          strncpy(s_ytData.latestVideoId, vId, sizeof(s_ytData.latestVideoId) - 1);
          s_ytData.latestVideoId[sizeof(s_ytData.latestVideoId) - 1] = '\0';
          Async_UnlockSettings();

          // 3. Fetch Views for this latest video
          if (strlen(vId) > 0) {
            String vidUrl = "https://www.googleapis.com/youtube/v3/videos?part=statistics&id=" +
                            urlEncode(vId) + "&key=" + urlEncode(apiKey);
            String vidBody;
            if (Net_GetString(vidUrl.c_str(), vidBody, "YouTube-Vid")) {
              JsonDocument vidFilter;
              vidFilter["items"][0]["statistics"]["viewCount"] = true;
              JsonDocument vidDoc;
              if (!deserializeJson(vidDoc, vidBody, DeserializationOption::Filter(vidFilter))) {
                if (vidDoc["items"] && vidDoc["items"].size() > 0) {
                  const char* vViewsStr = vidDoc["items"][0]["statistics"]["viewCount"] | "0";
                  Async_LockSettings();
                  s_ytData.latestVideoViews = strtoull(vViewsStr, nullptr, 10);
                  Async_UnlockSettings();
                }
              }
            }
          }
        }
      }
    }
  }

  s_ytData.lastUpdatedMs = millis();
  s_ytData.valid = true;
  s_ytData.fetching = false;
  s_ytBusy = false;
  s_forceFetch = false;
  strncpy(s_ytData.statusMsg, "OK", sizeof(s_ytData.statusMsg));

  Serial.printf("YouTube: Updated '%s' - Subs: %llu, Views: %llu, Latest: '%s' (%llu views)\n",
                s_ytData.channelTitle, (unsigned long long)s_ytData.subscriberCount,
                (unsigned long long)s_ytData.totalViews, s_ytData.latestVideoTitle,
                (unsigned long long)s_ytData.latestVideoViews);
  return true;
}
