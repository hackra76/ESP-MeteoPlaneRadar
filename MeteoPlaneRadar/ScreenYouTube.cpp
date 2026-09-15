// =============================================================================
//  MeteoPlaneRadar
//  ScreenYouTube.cpp - YouTube Channel Analytics UI screen.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenYouTube.h"
#include "YouTubeData.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Config.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "AsyncCore.h"
#include "Settings.h"
#include "FontEngine.h"
#include <WiFi.h>

static unsigned long s_lastDrawTick = 0;
static unsigned long s_lastTapTime = 0;

void ScreenYouTube_Enter() {
  Async_RequestYouTube();
}

bool ScreenYouTube_Tick() {
  if (Async_TakeYouTubeUpdated()) {
    return true;
  }
  unsigned long now = millis();
  if (now - s_lastDrawTick >= 1000) {
    s_lastDrawTick = now;
    return true;
  }
  return false;
}

bool ScreenYouTube_HandleTap(int x, int y) {
  (void)x; (void)y;
  unsigned long now = millis();
  if (now - s_lastTapTime < 1000) return false;
  s_lastTapTime = now;

  if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
  YouTube_RequestFetch();
  Async_RequestYouTube();
  return true;
}

// Format counts nicely e.g. "1.25M", "85.4K", "837"
static void formatMetric(uint64_t val, char* buf, size_t bufLen) {
  if (val >= 1000000000ULL) {
    snprintf(buf, bufLen, "%.2fB", (double)val / 1e9);
  } else if (val >= 10000000ULL) {
    snprintf(buf, bufLen, "%.1fM", (double)val / 1e6);
  } else if (val >= 1000000ULL) {
    snprintf(buf, bufLen, "%.2fM", (double)val / 1e6);
  } else if (val >= 10000ULL) {
    snprintf(buf, bufLen, "%.1fK", (double)val / 1e3);
  } else if (val >= 1000ULL) {
    snprintf(buf, bufLen, "%.2fK", (double)val / 1e3);
  } else {
    snprintf(buf, bufLen, "%llu", (unsigned long long)val);
  }
}

// Format space-separated integer for exact view count
static void formatSeparators(uint64_t val, char* buf, size_t bufLen) {
  char temp[32];
  snprintf(temp, sizeof(temp), "%llu", (unsigned long long)val);
  int len = strlen(temp);
  int outIdx = 0;
  for (int i = 0; i < len && outIdx < (int)bufLen - 2; i++) {
    if (i > 0 && (len - i) % 3 == 0) {
      buf[outIdx++] = ' ';
    }
    buf[outIdx++] = temp[i];
  }
  buf[outIdx] = '\0';
}

// Smart 2-line word wrapper for latest video title to prevent any card overflow
static void drawWrappedTitle(const char* title, int x, int w, int y) {
  if (!title || !*title) {
    UI_TextCenteredIn("No uploads", x, w, y + 6, RGB565(120, 135, 150), FONT_TINY);
    return;
  }
  int len = strlen(title);
  if (len <= 20) {
    UI_TextCenteredIn(title, x, w, y + 6, C_WHITE, FONT_TINY);
    return;
  }

  char line1[32] = {0};
  char line2[32] = {0};

  // Find optimal word boundary before 20 characters
  int splitIdx = -1;
  for (int i = 0; i < len && i <= 20; i++) {
    if (title[i] == ' ') splitIdx = i;
  }

  if (splitIdx >= 8) {
    strncpy(line1, title, splitIdx);
    line1[splitIdx] = '\0';
    const char* rest = title + splitIdx + 1;
    strncpy(line2, rest, sizeof(line2) - 1);
    line2[sizeof(line2) - 1] = '\0';
  } else {
    strncpy(line1, title, 18);
    line1[18] = '\0';
    strncpy(line2, title + 18, sizeof(line2) - 1);
    line2[sizeof(line2) - 1] = '\0';
  }

  if (strlen(line2) > 20) {
    line2[17] = '.'; line2[18] = '.'; line2[19] = '.'; line2[20] = '\0';
  }

  UI_TextCenteredIn(line1, x, w, y, C_WHITE, FONT_TINY);
  UI_TextCenteredIn(line2, x, w, y + 13, C_WHITE, FONT_TINY);
}

void ScreenYouTube_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  Layout_ReserveBand(LY_DOTS - 6, 12);

  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  YouTubeStats yt;
  bool valid = YouTube_GetData(&yt);

  // 1. YouTube Red Pill Badge at top (y=36, h=22, w=114)
  const int badgeW = 114;
  const int badgeH = 22;
  const int badgeX = (LCD_WIDTH - badgeW) / 2;
  const int badgeY = 36;
  gfx->fillRoundRect(badgeX, badgeY, badgeW, badgeH, 11, RGB565(230, 33, 23)); // YouTube Red
  // White Play triangle icon
  const int triX = badgeX + 14;
  const int triY = badgeY + 5;
  gfx->fillTriangle(triX, triY, triX, triY + 12, triX + 9, triY + 6, C_WHITE);
  UI_TextCenteredIn("YOUTUBE", badgeX + 16, badgeW - 16, badgeY + 4, C_WHITE, FONT_SMALL);

  // 2. Channel Title (y=64)
  const char* chTitle = (valid && strlen(yt.channelTitle) > 0) ? yt.channelTitle
                      : (Settings_YouTubeChannel()[0] ? Settings_YouTubeChannel() : "No Channel Configured");
  char dispTitle[32];
  strncpy(dispTitle, chTitle, sizeof(dispTitle) - 1);
  dispTitle[sizeof(dispTitle) - 1] = '\0';
  if (strlen(dispTitle) > 24) {
    dispTitle[21] = '.'; dispTitle[22] = '.'; dispTitle[23] = '.'; dispTitle[24] = '\0';
  }
  UI_TextCentered(dispTitle, 64, C_WHITE, FONT_TITLE);

  // Missing API key or error message
  if (!valid) {
    const char* hint = yt.statusMsg;
    if (!Settings_YouTubeApiKey() || strlen(Settings_YouTubeApiKey()) == 0) {
      hint = isEn ? "Configure API Key in Web UI" : (isSk ? "Nastavte API kluc vo Web UI" : "Nastavte API klic ve Web UI");
    }
    UI_TextCentered(hint, 84, RGB565(255, 185, 0), FONT_SMALL);
  }

  char strBuf[64];

  // 3. Hero Card: SUBSCRIBERS (Center y=92..218, H=126, W=380)
  const int heroW = 380;
  const int heroH = 126;
  const int heroX = (LCD_WIDTH - heroW) / 2; // 50
  const int heroY = 92;

  // Outer rounded container
  gfx->fillRoundRect(heroX, heroY, heroW, heroH, 14, 0x0842);
  gfx->drawRoundRect(heroX, heroY, heroW, heroH, 14, 0x29E8);

  // Red accent indicator line at top of hero card
  gfx->fillRoundRect(heroX + (heroW - 120) / 2, heroY, 120, 3, 2, RGB565(230, 33, 23));

  const char* subLabel = isEn ? "SUBSCRIBERS" : (isSk ? "ODBERATELIA" : "ODBĚRATELÉ");
  UI_TextCenteredIn(subLabel, heroX, heroW, heroY + 12, RGB565(150, 170, 195), FONT_SMALL);

  if (valid) {
    formatMetric(yt.subscriberCount, strBuf, sizeof(strBuf));
  } else {
    snprintf(strBuf, sizeof(strBuf), "---");
  }
  // Prominent, large bold subscriber metric
  UI_TextCenteredIn(strBuf, heroX, heroW, heroY + 34, C_WHITE, FONT_HERO);

  // Exact subscriber count subtitle in subtle slate gray
  if (valid && yt.subscriberCount > 0) {
    char exactBuf[32];
    formatSeparators(yt.subscriberCount, exactBuf, sizeof(exactBuf));
    snprintf(strBuf, sizeof(strBuf), "%s %s", exactBuf, isEn ? "total" : "spolu");
    UI_TextCenteredIn(strBuf, heroX, heroW, heroY + 92, RGB565(130, 150, 175), FONT_SMALL);
  }

  // 4. Lower Cards: Total Views (Left) & Latest Video (Right)
  // Perfectly aligned with hero card width: 184 + 12 + 184 = 380
  const int cardW = 184;
  const int cardH = 148;
  const int gapCards = 12;
  const int card1X = heroX;                     // 50
  const int card2X = card1X + cardW + gapCards; // 246
  const int cardY  = 228;

  // --- Card A: Total Channel Views ---
  gfx->fillRoundRect(card1X, cardY, cardW, cardH, 14, 0x0842);
  gfx->drawRoundRect(card1X, cardY, cardW, cardH, 14, 0x2187);

  UI_TextCenteredIn(isEn ? "TOTAL VIEWS" : "POZRETIA", card1X, cardW, cardY + 12, RGB565(140, 160, 185), FONT_SMALL);

  if (valid) {
    formatMetric(yt.totalViews, strBuf, sizeof(strBuf));
  } else {
    snprintf(strBuf, sizeof(strBuf), "---");
  }
  UI_TextCenteredIn(strBuf, card1X, cardW, cardY + 36, RGB565(0, 225, 255), FONT_HUGE);

  // Exact views or subtitle
  if (valid && yt.totalViews > 0) {
    char exactViews[32];
    formatSeparators(yt.totalViews, exactViews, sizeof(exactViews));
    UI_TextCenteredIn(exactViews, card1X, cardW, cardY + 74, RGB565(150, 165, 180), FONT_TINY);
  }

  // Total Videos Chip at bottom of Card A
  if (valid && yt.videoCount > 0) {
    const int chipW = 120, chipH = 22;
    const int chipX = card1X + (cardW - chipW) / 2;
    const int chipY = cardY + 106;
    gfx->fillRoundRect(chipX, chipY, chipW, chipH, 5, 0x1146);
    gfx->drawRoundRect(chipX, chipY, chipW, chipH, 5, 0x29E8);
    snprintf(strBuf, sizeof(strBuf), "%u %s", yt.videoCount, isEn ? "VIDEOS" : "VIDEÍ");
    UI_TextCenteredIn(strBuf, chipX, chipW, chipY + 4, RGB565(180, 205, 230), FONT_TINY);
  }

  // --- Card B: Latest Video Views & Title ---
  gfx->fillRoundRect(card2X, cardY, cardW, cardH, 14, 0x0842);
  gfx->drawRoundRect(card2X, cardY, cardW, cardH, 14, 0x2187);

  // Red "NEW" tag + header label without any overlap
  const int tagW = 32, tagH = 15;
  const int tagX = card2X + 12;
  const int tagY = cardY + 11;
  gfx->fillRoundRect(tagX, tagY, tagW, tagH, 3, RGB565(230, 33, 23));
  UI_TextCenteredIn("NEW", tagX, tagW, tagY + 2, C_WHITE, FONT_TINY);

  const char* vHeadLabel = isEn ? "LATEST VIDEO" : "NOVÉ VIDEO";
  UI_Text(vHeadLabel, card2X + 50, cardY + 12, RGB565(140, 160, 185), FONT_SMALL);

  // Latest Video Views counter
  if (valid && yt.latestVideoViews > 0) {
    formatMetric(yt.latestVideoViews, strBuf, sizeof(strBuf));
  } else if (valid) {
    snprintf(strBuf, sizeof(strBuf), "0");
  } else {
    snprintf(strBuf, sizeof(strBuf), "---");
  }
  UI_TextCenteredIn(strBuf, card2X, cardW, cardY + 36, RGB565(255, 215, 0), FONT_HUGE);

  // "views" label
  UI_TextCenteredIn(isEn ? "views" : "pozretí", card2X, cardW, cardY + 74, RGB565(150, 165, 180), FONT_TINY);

  // Latest Video Title with 2-line clean wrap contained strictly inside Card B
  if (valid && strlen(yt.latestVideoTitle) > 0) {
    drawWrappedTitle(yt.latestVideoTitle, card2X + 8, cardW - 16, cardY + 102);
  } else {
    UI_TextCenteredIn(isEn ? "No uploads" : "Žiadne videá", card2X + 8, cardW - 16, cardY + 108, RGB565(120, 135, 150), FONT_TINY);
  }

  // 5. Bottom Status / Timestamp (y=394)
  if (valid && yt.lastUpdatedMs > 0) {
    unsigned long ageSec = (millis() - yt.lastUpdatedMs) / 1000;
    if (ageSec < 60) {
      snprintf(strBuf, sizeof(strBuf), "%s", isEn ? "Updated just now" : (isSk ? "Aktualizované práve teraz" : "Aktualizováno právě teď"));
    } else {
      snprintf(strBuf, sizeof(strBuf), "%s: %lum ago", isEn ? "Updated" : (isSk ? "Aktualizované" : "Aktualizováno"), ageSec / 60);
    }
    // Small live green status dot
    int tw = Font_TextWidth(strBuf, FONT_SMALL);
    gfx->fillCircle(LCD_WIDTH / 2 - tw / 2 - 10, 399, 3, C_GREEN);
    UI_TextCentered(strBuf, 394, RGB565(120, 140, 160), FONT_SMALL);
  } else {
    UI_TextCentered(isEn ? "Tap screen to refresh" : (isSk ? "Ťuknite pre obnovenie" : "Klepněte pro obnovení"), 394, RGB565(120, 140, 160), FONT_SMALL);
  }
}
