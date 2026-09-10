// =============================================================================
//  MeteoPlaneRadar
//  ScreenFinance.cpp - Financial Markets, Stocks, Crypto & Commodities Screen.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "ScreenFinance.h"
#include "FinanceData.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include "Lang.h"
#include "UI.h"
#include "Config.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "AsyncCore.h"
#include <WiFi.h>

#define CX (LCD_WIDTH / 2)
#define CY (LCD_HEIGHT / 2)

static unsigned long s_lastDrawTick = 0;
static unsigned long s_lastTapTime = 0;

void ScreenFinance_Enter() {
  Async_RequestFinance();
}

bool ScreenFinance_Tick() {
  if (Async_TakeFinanceUpdated()) {
    return true;
  }
  unsigned long now = millis();
  if (now - s_lastDrawTick >= 2000) {
    s_lastDrawTick = now;
    return true;
  }
  return false;
}

static void formatPrice(float val, const char* cur, char* out, size_t cap) {
  if (val <= 0.00001f) {
    snprintf(out, cap, "---");
    return;
  }
  const char* sym = "$";
  if (strcmp(cur, "EUR") == 0) sym = "€";
  else if (strcmp(cur, "GBP") == 0) sym = "£";
  else if (strcmp(cur, "CZK") == 0) sym = "Kc";

  if (val >= 10000.0f) {
    int whole = (int)val;
    int k = whole / 1000;
    int rem = whole % 1000;
    int dec = (int)((val - whole) * 100.0f + 0.5f);
    snprintf(out, cap, "%s%d,%03d.%02d", sym, k, rem, dec);
  } else if (val >= 100.0f) {
    snprintf(out, cap, "%s%.2f", sym, val);
  } else if (val >= 1.0f) {
    snprintf(out, cap, "%s%.3f", sym, val);
  } else {
    snprintf(out, cap, "%s%.4f", sym, val);
  }
}

static void drawSparkline(int x, int y, int w, int h, const float* data, int count, float prevClose, bool positive) {
  if (!data || count < 2) {
    gfx->drawLine(x, y + h / 2, x + w - 1, y + h / 2, 0x31A6);
    return;
  }

  float minVal = data[0];
  float maxVal = data[0];
  for (int i = 1; i < count; i++) {
    if (data[i] < minVal) minVal = data[i];
    if (data[i] > maxVal) maxVal = data[i];
  }
  if (prevClose > 0.0f) {
    if (prevClose < minVal) minVal = prevClose;
    if (prevClose > maxVal) maxVal = prevClose;
  }

  float range = maxVal - minVal;
  if (range <= 0.00001f) range = 1.0f;

  // Draw dashed baseline for previous close if within bounds
  if (prevClose >= minVal && prevClose <= maxVal) {
    int basePy = y + h - 1 - (int)(((prevClose - minVal) / range) * (h - 10) + 5);
    for (int bx = x; bx < x + w - 1; bx += 6) {
      gfx->drawFastHLine(bx, basePy, 3, 0x39E7);
    }
  }

  uint16_t lineCol = positive ? 0x27E8 : 0xF986; // Vibrant emerald vs vibrant coral
  uint16_t fillCol = positive ? 0x0A82 : 0x48A1;

  int prevX = x;
  int prevY = y + h - 1 - (int)(((data[0] - minVal) / range) * (h - 10) + 5);

  for (int i = 1; i < count; i++) {
    int curX = x + (i * (w - 1)) / (count - 1);
    int curY = y + h - 1 - (int)(((data[i] - minVal) / range) * (h - 10) + 5);

    // Soft gradient glow / vertical shade under curve
    gfx->drawLine(curX, curY + 1, curX, y + h - 1, fillCol);

    // Thick crisp trendline
    gfx->drawLine(prevX, prevY, curX, curY, lineCol);
    gfx->drawLine(prevX, prevY + 1, curX, curY + 1, lineCol);

    prevX = curX;
    prevY = curY;
  }

  // End dot
  gfx->fillCircle(prevX, prevY, 3, C_WHITE);
  gfx->fillCircle(prevX, prevY, 2, lineCol);
}

void ScreenFinance_Draw() {
  gfx->fillScreen(C_BLACK);
  Layout_Begin();
  Layout_ReserveBand(LY_DOTS - 6, 12);

  const bool isEn = (Lang_Get() == LANG_EN);
  const bool isSk = (Lang_Get() == LANG_SK);

  // Top screen title
  char titleBuf[48];
  int count = Finance_Count();
  int actIdx = Finance_ActiveIndex();
  const char* baseTitle = isEn ? "MARKETS & CRYPTO" : (isSk ? "TRHY & KRYPTO" : "TRHY & KRYPTO");
  if (count > 0) {
    snprintf(titleBuf, sizeof(titleBuf), "%s  (%d/%d)", baseTitle, actIdx + 1, count);
  } else {
    snprintf(titleBuf, sizeof(titleBuf), "%s", baseTitle);
  }
  UI_TextCentered(titleBuf, 32, C_WHITE, 1);

  // Live status indicator dot
  bool busy = Finance_IsBusy();
  uint16_t dotCol = busy ? C_YELLOW : (WiFi.status() == WL_CONNECTED ? C_GREEN : C_RED);
  gfx->fillCircle(CX + Layout_TextW(titleBuf, 1) / 2 + 10, 35, 3, dotCol);

  if (count == 0) {
    UI_TextCentered("No tickers configured", CY, C_GRAY, 2);
    return;
  }

  const FinanceItem* activeItem = Finance_GetItemPtr(actIdx);
  if (!activeItem) return;

  const int cardW = 310;
  const int cardX = CX - cardW / 2; // 85

  // ---------------------------------------------------------------------------
  // HERO CARD: Focused Asset (Y: 48 .. 224, H: 176)
  // ---------------------------------------------------------------------------
  const int hY = 48;
  const int hH = 176;

  gfx->fillRoundRect(cardX, hY, cardW, hH, 10, 0x0842);
  gfx->drawRoundRect(cardX, hY, cardW, hH, 10, 0x2187);

  // Row 1: Symbol and Category Tag
  char symDisplay[24];
  strncpy(symDisplay, activeItem->symbol, sizeof(symDisplay) - 1);
  symDisplay[sizeof(symDisplay) - 1] = '\0';
  if (symDisplay[0] == '^') memmove(symDisplay, symDisplay + 1, strlen(symDisplay));

  UI_Text(symDisplay, cardX + 12, hY + 10, C_CYAN, 2);

  // Category badge
  const char* cat = "ASSET";
  if (strstr(activeItem->symbol, "-USD") || strstr(activeItem->symbol, "-EUR")) cat = "CRYPTO";
  else if (activeItem->symbol[0] == '^') cat = "INDEX";
  else if (strstr(activeItem->symbol, "=F")) cat = "COMMODITY";
  else if (strstr(activeItem->symbol, "=X")) cat = "FOREX";
  else cat = "EQUITY";

  int catW = Layout_TextW(cat, 1) + 10;
  int catX = cardX + cardW - 12 - catW;
  gfx->fillRoundRect(catX, hY + 10, catW, 16, 4, 0x1146);
  gfx->drawRoundRect(catX, hY + 10, catW, 16, 4, 0x29E8);
  UI_Text(cat, catX + 5, hY + 14, 0x863F, 1);

  // Short Name
  if (activeItem->shortName[0]) {
    char nameShort[24];
    strncpy(nameShort, activeItem->shortName, sizeof(nameShort) - 1);
    nameShort[sizeof(nameShort) - 1] = '\0';
    if (strlen(nameShort) > 18) {
      nameShort[16] = '.'; nameShort[17] = '.'; nameShort[18] = '\0';
    }
    UI_Text(nameShort, cardX + 12, hY + 30, C_GRAY, 1);
  }

  // Row 2: Price & Change Pill
  char priceBuf[24];
  formatPrice(activeItem->price, activeItem->currency, priceBuf, sizeof(priceBuf));
  UI_Text(priceBuf, cardX + 12, hY + 46, C_WHITE, 2);

  // Percentage badge
  char chgBuf[16];
  bool positive = (activeItem->changePct >= 0.0f);
  snprintf(chgBuf, sizeof(chgBuf), "%s%.2f%%", positive ? "+" : "", activeItem->changePct);

  int chgW = Layout_TextW(chgBuf, 1) + 12;
  int chgX = cardX + cardW - 12 - chgW;
  uint16_t pillBg = positive ? 0x0B22 : 0x4861;
  uint16_t pillBorder = positive ? 0x1CE6 : 0x8183;
  uint16_t pillFg = positive ? C_GREEN : 0xF986;

  gfx->fillRoundRect(chgX, hY + 46, chgW, 18, 5, pillBg);
  gfx->drawRoundRect(chgX, hY + 46, chgW, 18, 5, pillBorder);
  UI_Text(chgBuf, chgX + 6, hY + 51, pillFg, 1);

  // Divider line
  gfx->drawLine(cardX + 10, hY + 70, cardX + cardW - 10, hY + 70, 0x1945);

  // Row 3: Sparkline Chart (Y: hY + 76 .. hY + 166, H: 90)
  const int sparkX = cardX + 12;
  const int sparkY = hY + 78;
  const int sparkW = cardW - 24;
  const int sparkH = 88;

  drawSparkline(sparkX, sparkY, sparkW, sparkH, activeItem->sparkline, activeItem->sparkCount, activeItem->prevClose, positive);

  // ---------------------------------------------------------------------------
  // WATCHLIST MINI-CARDS (Y: 232 .. 396)
  // Shows next tickers in watchlist with one-touch direct selection
  // ---------------------------------------------------------------------------
  const int rowH = 48;
  const int startRowY = 232;
  int shownCount = 0;

  for (int i = 0; i < count && shownCount < 3; i++) {
    int itemIdx = (actIdx + 1 + i) % count;
    if (itemIdx == actIdx && count > 1) continue;

    const FinanceItem* it = Finance_GetItemPtr(itemIdx);
    if (!it) continue;

    int rY = startRowY + shownCount * (rowH + 6);
    gfx->fillRoundRect(cardX, rY, cardW, rowH, 8, 0x0821);
    gfx->drawRoundRect(cardX, rY, cardW, rowH, 8, 0x1945);

    // Ticker symbol
    char rSym[16];
    strncpy(rSym, it->symbol, sizeof(rSym) - 1);
    rSym[sizeof(rSym) - 1] = '\0';
    if (rSym[0] == '^') memmove(rSym, rSym + 1, strlen(rSym));

    UI_Text(rSym, cardX + 12, rY + 8, C_WHITE, 2);

    if (it->shortName[0]) {
      char rName[18];
      strncpy(rName, it->shortName, sizeof(rName) - 1);
      rName[sizeof(rName) - 1] = '\0';
      if (strlen(rName) > 13) { rName[11] = '.'; rName[12] = '.'; rName[13] = '\0'; }
      UI_Text(rName, cardX + 12, rY + 28, C_GRAY, 1);
    }

    // Mini Price
    char rPrice[20];
    formatPrice(it->price, it->currency, rPrice, sizeof(rPrice));
    int prW = Layout_TextW(rPrice, 1);

    // Mini Change Pill
    char rChg[16];
    bool rPos = (it->changePct >= 0.0f);
    snprintf(rChg, sizeof(rChg), "%s%.1f%%", rPos ? "+" : "", it->changePct);
    int rChgW = Layout_TextW(rChg, 1) + 8;
    int rChgX = cardX + cardW - 10 - rChgW;

    uint16_t rPillBg = rPos ? 0x0B22 : 0x4861;
    uint16_t rPillFg = rPos ? C_GREEN : 0xF986;

    gfx->fillRoundRect(rChgX, rY + 14, rChgW, 18, 4, rPillBg);
    UI_Text(rChg, rChgX + 4, rY + 19, rPillFg, 1);

    UI_Text(rPrice, rChgX - prW - 8, rY + 19, C_LTGRAY, 1);

    shownCount++;
  }

  // Bottom footer touch hint
  const char* hint = isEn ? "Tap to switch asset  *  Dbl-tap refresh" :
                     (isSk ? "Dotyk prepne aktivum  *  Dvojklik obnovi" :
                             "Dotyk prepne aktivum  *  Dvojklik obnovi");
  UI_TextCentered(hint, 420, C_DKGRAY, 1);
}

bool ScreenFinance_HandleTap(int x, int y) {
  unsigned long now = millis();

  // Double tap (within 350ms) forces refresh
  if (now - s_lastTapTime < 350) {
    Finance_RequestFetch();
    Buzzer_Play(BEEP_CLICK);
    s_lastTapTime = 0;
    return true;
  }
  s_lastTapTime = now;

  int count = Finance_Count();
  if (count <= 1) {
    Finance_RequestFetch();
    Buzzer_Play(BEEP_CLICK);
    return true;
  }

  const int cardW = 310;
  const int cardX = CX - cardW / 2;
  const int rowH = 48;
  const int startRowY = 232;
  int actIdx = Finance_ActiveIndex();

  // Check tap on watchlist mini-cards
  if (x >= cardX && x <= cardX + cardW) {
    for (int i = 0; i < 3; i++) {
      int rY = startRowY + i * (rowH + 6);
      if (y >= rY && y <= rY + rowH) {
        int targetIdx = (actIdx + 1 + i) % count;
        Finance_SetActiveIndex(targetIdx);
        Buzzer_Play(BEEP_CLICK);
        return true;
      }
    }
  }

  // Tap anywhere else cycles to next ticker
  Finance_NextActive();
  Buzzer_Play(BEEP_CLICK);
  return true;
}
