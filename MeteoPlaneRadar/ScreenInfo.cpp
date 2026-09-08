// =============================================================================
//  MeteoPlaneRadar
//  Screen: Info & Statistics (flight traffic summary, system & network info).
//
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenInfo.h"
#include "FlightStats.h"
#include "Settings.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Display_ST7701.h"
#include "Config.h"
#include "Version.h"
#include "Buzzer.h"

#include <WiFi.h>
#include <esp_system.h>
#include <time.h>

#define CX (LCD_WIDTH / 2)
#define CY (LCD_HEIGHT / 2)

static unsigned long s_lastTick = 0;
static bool s_resetFlash = false;
static unsigned long s_resetFlashTime = 0;

void ScreenInfo_Enter() {
  FlightStats_CheckMidnight();
}

bool ScreenInfo_Tick() {
  unsigned long now = millis();
  if (s_resetFlash && now - s_resetFlashTime > 1500) {
    s_resetFlash = false;
    return true;
  }
  // Refresh every second for uptime and stats
  if (now - s_lastTick >= 1000) {
    s_lastTick = now;
    return true;
  }
  return false;
}

bool ScreenInfo_HandleTap(int x, int y) {
  // Tap bottom button area (Y in [395, 440], X in [135, 345]) to reset stats
  if (y >= 395 && y <= 440 && x >= 135 && x <= 345) {
    FlightStats_Reset();
    s_resetFlash = true;
    s_resetFlashTime = millis();
    Buzzer_Play(BEEP_CLICK);
    return true;
  }
  return false;
}

static void drawWifiStrength(int x, int y, int rssi) {
  // 4 signal bars
  const int barW = 3;
  const int gap = 2;
  int level = 0;
  if (rssi > -60) level = 4;
  else if (rssi > -70) level = 3;
  else if (rssi > -80) level = 2;
  else if (rssi > -90) level = 1;

  uint16_t col = (level >= 3) ? C_GREEN : (level == 2 ? C_YELLOW : C_RED);
  for (int i = 0; i < 4; i++) {
    int h = (i + 1) * 3;
    uint16_t c = (i < level) ? col : 0x31A6;
    gfx->fillRect(x + i * (barW + gap), y + 12 - h, barW, h, c);
  }
}

void ScreenInfo_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  Layout_ReserveBand(LY_DOTS - 6, 12);

  FlightStats_CheckMidnight();

  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  // Screen Title (Y=32 in size 1, sits cleanly between dots at Y=18 and Card 1 at Y=52)
  const char* title = isEn ? "INFO & STATISTICS" : (isSk ? "INFO & STATISTIKY" : "INFO & STATISTIKY");
  UI_TextCentered(title, 32, C_WHITE, 1);

  // Card layout parameters:
  // On a 480x480 circle, W=290 (X: 95..385) fits safely inside the circle bounds
  // at all heights from Y=52 to Y=390 with zero corner clipping.
  const int cardW = 290;
  const int cardX = CX - cardW / 2; // 95
  const int padX  = 12;

  // -------------------------------------------------------------------------
  // CARD 1: Today's Flight Traffic (Y: 52..216, H: 164)
  // -------------------------------------------------------------------------
  const int c1Y = 52;
  const int c1H = 164;
  gfx->fillRoundRect(cardX, c1Y, cardW, c1H, 10, 0x0821);
  gfx->drawRoundRect(cardX, c1Y, cardW, c1H, 10, 0x2187);

  // Header banner inside Card 1
  gfx->fillRoundRect(cardX, c1Y, cardW, 22, 10, 0x10A2);
  gfx->fillRect(cardX, c1Y + 11, cardW, 11, 0x10A2);
  gfx->drawLine(cardX, c1Y + 22, cardX + cardW - 1, c1Y + 22, 0x29E8);

  const char* trafficTitle = isEn ? "AIR TRAFFIC TODAY" : (isSk ? "PREMAVKA DNES" : "PROVOZ DNES");
  UI_TextCenteredIn(trafficTitle, cardX, cardW, c1Y + 4, C_CYAN, 2);

  // Row 1: Unique Aircraft Count
  const int r1Y = c1Y + 30;
  uint32_t count = FlightStats_TodayCount();
  char countBuf[16];
  snprintf(countBuf, sizeof(countBuf), "%u", count);

  const char* unqLbl = isEn ? "Unique aircraft:" : (isSk ? "Unikatne stroje:" : "Unikatni letadla:");
  UI_Text(unqLbl, cardX + padX, r1Y + 4, C_GRAY, 1);
  UI_Text(countBuf, cardX + cardW - Layout_TextW(countBuf, 2) - padX, r1Y, C_WHITE, 2);

  // Row 2: Top Ground Speed
  const int r2Y = c1Y + 56;
  float topKt = FlightStats_MaxSpeedKt();
  const char* topCs = FlightStats_MaxSpeedCallsign();
  const char* spdLbl = isEn ? "Max speed:" : (isSk ? "Max. rychlost:" : "Max. rychlost:");
  UI_Text(spdLbl, cardX + padX, r2Y + 4, C_GRAY, 1);

  if (topKt > 0.0f) {
    char spdVal[24];
    if (Settings_MetricUnits()) {
      snprintf(spdVal, sizeof(spdVal), "%d km/h", (int)(topKt * 1.852f));
    } else {
      snprintf(spdVal, sizeof(spdVal), "%d kt", (int)topKt);
    }
    int spdW = Layout_TextW(spdVal, 2);

    if (topCs && topCs[0]) {
      char csBuf[16];
      snprintf(csBuf, sizeof(csBuf), "(%s)", topCs);
      int csW = Layout_TextW(csBuf, 1);
      int rightX = cardX + cardW - padX;
      UI_Text(csBuf, rightX - csW, r2Y + 4, C_CYAN, 1);
      UI_Text(spdVal, rightX - csW - 6 - spdW, r2Y, C_YELLOW, 2);
    } else {
      UI_Text(spdVal, cardX + cardW - spdW - padX, r2Y, C_YELLOW, 2);
    }
  } else {
    UI_Text("---", cardX + cardW - Layout_TextW("---", 2) - padX, r2Y, C_GRAY, 2);
  }

  // Row 3: Max Distance
  const int r3Y = c1Y + 82;
  float maxDist = FlightStats_MaxDistKm();
  const char* dstLbl = isEn ? "Max distance:" : (isSk ? "Max. vzdialenost:" : "Max. vzdalenost:");
  UI_Text(dstLbl, cardX + padX, r3Y + 4, C_GRAY, 1);

  char distBuf[24];
  if (maxDist > 0.0f) {
    snprintf(distBuf, sizeof(distBuf), "%.1f km", maxDist);
  } else {
    snprintf(distBuf, sizeof(distBuf), "---");
  }
  UI_Text(distBuf, cardX + cardW - Layout_TextW(distBuf, 2) - padX, r3Y, C_WHITE, 2);

  // Row 4: Altitude Range
  const int r4Y = c1Y + 108;
  float minAlt = FlightStats_MinAltFt();
  float maxAlt = FlightStats_MaxAltFt();
  const char* altLbl = isEn ? "Altitude span:" : (isSk ? "Rozpatie vysky:" : "Rozpeti vysek:");
  UI_Text(altLbl, cardX + padX, r4Y + 4, C_GRAY, 1);

  char altBuf[32];
  if (maxAlt > 0.0f && minAlt < 999990.0f) {
    if (Settings_MetricUnits()) {
      snprintf(altBuf, sizeof(altBuf), "%.0f - %.0f m", minAlt * 0.3048f, maxAlt * 0.3048f);
    } else {
      snprintf(altBuf, sizeof(altBuf), "FL%d - FL%d", (int)(minAlt / 100), (int)(maxAlt / 100));
    }
  } else {
    snprintf(altBuf, sizeof(altBuf), "---");
  }
  UI_Text(altBuf, cardX + cardW - Layout_TextW(altBuf, 1) - padX, r4Y + 4, C_CYAN, 1);

  // Row 5: Total Position Reports
  const int r5Y = c1Y + 134;
  uint32_t totalSightings = FlightStats_TotalSightings();
  const char* repLbl = isEn ? "ADS-B reports:" : (isSk ? "Celkovo sprav:" : "Celkem zprav:");
  UI_Text(repLbl, cardX + padX, r5Y + 4, C_DKGRAY, 1);
  char repBuf[16];
  snprintf(repBuf, sizeof(repBuf), "%u", totalSightings);
  UI_Text(repBuf, cardX + cardW - Layout_TextW(repBuf, 1) - padX, r5Y + 4, C_GRAY, 1);

  // -------------------------------------------------------------------------
  // CARD 2: System & Network Status (Y: 224..388, H: 164)
  // -------------------------------------------------------------------------
  const int c2Y = 224;
  const int c2H = 164;
  gfx->fillRoundRect(cardX, c2Y, cardW, c2H, 10, 0x0821);
  gfx->drawRoundRect(cardX, c2Y, cardW, c2H, 10, 0x2187);

  // Header banner inside Card 2
  gfx->fillRoundRect(cardX, c2Y, cardW, 22, 10, 0x0A65);
  gfx->fillRect(cardX, c2Y + 11, cardW, 11, 0x0A65);
  gfx->drawLine(cardX, c2Y + 22, cardX + cardW - 1, c2Y + 22, 0x1AE9);

  const char* sysTitle = isEn ? "SYSTEM & NETWORK" : (isSk ? "SYSTEM & SIET" : "SYSTEM & SIT");
  UI_TextCenteredIn(sysTitle, cardX, cardW, c2Y + 4, C_GREEN, 2);

  // Row 1: WiFi row: SSID + RSSI dBm + Signal bars
  const int cr1Y = c2Y + 30;
  UI_Text("WiFi:", cardX + padX, cr1Y + 4, C_GRAY, 1);
  if (WiFi.status() == WL_CONNECTED) {
    char ssidBuf[18];
    String rawSsid = WiFi.SSID();
    if (rawSsid.length() > 14) {
      rawSsid = rawSsid.substring(0, 13) + "..";
    }
    strncpy(ssidBuf, rawSsid.c_str(), sizeof(ssidBuf) - 1);
    ssidBuf[sizeof(ssidBuf) - 1] = '\0';
    UI_Text(ssidBuf, cardX + 50, cr1Y, C_WHITE, 2);

    int rssi = WiFi.RSSI();
    char rssiBuf[16];
    snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", rssi);
    UI_Text(rssiBuf, cardX + cardW - Layout_TextW(rssiBuf, 1) - 30 - padX, cr1Y + 4, C_LTGRAY, 1);
    drawWifiStrength(cardX + cardW - 24 - padX, cr1Y + 4, rssi);
  } else {
    UI_Text(T(S_NOT_CONNECTED), cardX + 50, cr1Y, C_YELLOW, 2);
  }

  // Row 2: IP Address
  const int cr2Y = c2Y + 56;
  UI_Text("IP:", cardX + padX, cr2Y + 4, C_GRAY, 1);
  char ipBuf[24];
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(ipBuf, sizeof(ipBuf), "%s", WiFi.localIP().toString().c_str());
  } else {
    snprintf(ipBuf, sizeof(ipBuf), "---");
  }
  UI_Text(ipBuf, cardX + cardW - Layout_TextW(ipBuf, 2) - padX, cr2Y, C_CYAN, 2);

  // Row 3: Uptime
  const int cr3Y = c2Y + 82;
  UI_Text("Uptime:", cardX + padX, cr3Y + 4, C_GRAY, 1);
  uint32_t upSec = (uint32_t)(millis() / 1000UL);
  uint32_t upDays = upSec / 86400UL;
  uint32_t upHours = (upSec % 86400UL) / 3600UL;
  uint32_t upMin = (upSec % 3600UL) / 60UL;
  uint32_t upS = upSec % 60UL;
  char upBuf[32];
  if (upDays > 0) {
    snprintf(upBuf, sizeof(upBuf), "%ud %02uh %02um", upDays, upHours, upMin);
  } else {
    snprintf(upBuf, sizeof(upBuf), "%02uh %02um %02us", upHours, upMin, upS);
  }
  UI_Text(upBuf, cardX + cardW - Layout_TextW(upBuf, 2) - padX, cr3Y, C_WHITE, 2);

  // Row 4: Free RAM / PSRAM
  const int cr4Y = c2Y + 108;
  const char* memLbl = isEn ? "Memory:" : (isSk ? "Pamat:" : "Pamet:");
  UI_Text(memLbl, cardX + padX, cr4Y + 4, C_GRAY, 1);
  char memBuf[48];
  uint32_t freeHeap = esp_get_free_heap_size() / 1024;
  uint32_t freePsram = ESP.getFreePsram() / 1024;
  snprintf(memBuf, sizeof(memBuf), "Heap: %uk | PSRAM: %.1fM", freeHeap, freePsram / 1024.0f);
  UI_Text(memBuf, cardX + cardW - Layout_TextW(memBuf, 1) - padX, cr4Y + 4, C_LTGRAY, 1);

  // Row 5: Firmware version
  const int cr5Y = c2Y + 134;
  const char* fwLbl = isEn ? "Firmware:" : (isSk ? "Firmver:" : "Firmware:");
  UI_Text(fwLbl, cardX + padX, cr5Y + 4, C_DKGRAY, 1);
  char fwBuf[32];
  snprintf(fwBuf, sizeof(fwBuf), "v%s", FW_VERSION);
  UI_Text(fwBuf, cardX + cardW - Layout_TextW(fwBuf, 1) - padX, cr5Y + 4, C_GRAY, 1);

  // -------------------------------------------------------------------------
  // Footer: Reset Statistics button / hint (Y: 402..430, H: 28, W: 180)
  // Sits safely within the bottom chord (chord width at Y=430 is ~293 px)
  // -------------------------------------------------------------------------
  const int btnW = 180, btnH = 28;
  const int btnX = CX - btnW / 2;
  const int btnY = 402;

  if (s_resetFlash) {
    gfx->fillRoundRect(btnX, btnY, btnW, btnH, 8, C_GREEN);
    const char* rstOk = isEn ? "Reset Complete!" : (isSk ? "Reset uspesny!" : "Reset uspesny!");
    UI_TextCenteredIn(rstOk, btnX, btnW, btnY + 8, C_BLACK, 1);
  } else {
    gfx->fillRoundRect(btnX, btnY, btnW, btnH, 8, 0x18C3);
    gfx->drawRoundRect(btnX, btnY, btnW, btnH, 8, 0x31A6);
    const char* rstText = isEn ? "[ Reset Statistics ]" : (isSk ? "[ Resetovat statistiky ]" : "[ Resetovat statistiky ]");
    UI_TextCenteredIn(rstText, btnX, btnW, btnY + 8, C_GRAY, 1);
  }
}
