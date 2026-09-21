// =============================================================================
//  MeteoPlaneRadar
//  QuickControl.cpp - Pull-down Control Center overlay implementation.
// =============================================================================
#include "QuickControl.h"
#include "Display_ST7701.h"
#include "Settings.h"
#include "NightMode.h"
#include "Lang.h"
#include "UI.h"
#include "Layout.h"
#include "AsyncCore.h"
#include "CHMU.h"
#include "SHMU.h"
#include "RainViewer.h"
#include "ScreenWeather.h"
#include "FinanceData.h"
#include "Buzzer.h"
#include "Config.h"

extern void gotoScreen(int idx);

static bool s_open = false;
static int  s_pickingSlot = -1; // 0..3 when asset picker is open, -1 when closed
static int  s_catIdx = 0;       // 0..5 active category in asset picker

bool QuickControl_IsOpen() { return s_open; }
void QuickControl_Open()   { s_open = true; s_pickingSlot = -1; }
void QuickControl_Close()  { s_open = false; s_pickingSlot = -1; }
void QuickControl_Toggle() { s_open = !s_open; s_pickingSlot = -1; }

static const int CW_W = 360;
static const int CW_H = 276;
static const int CW_X = (LCD_WIDTH - CW_W) / 2;
static const int CW_Y = 24;

// -----------------------------------------------------------------------------
//  Finance Presets & Categories (Mirrors Web Dashboard)
// -----------------------------------------------------------------------------
struct FinPresetItem {
  const char* symbol;
  const char* label;
};

struct FinPresetCat {
  const char* nameSk;
  const char* nameEn;
  const FinPresetItem* items;
  uint8_t count;
};

static const FinPresetItem PRESETS_ETF[] = {
  { "CW8.PA",  "Amundi World" },
  { "500.PA",  "Amundi S&P500" },
  { "C50.PA",  "Euro Stoxx 50" },
  { "C6E.PA",  "Stoxx Eur 600" },
  { "VWCE.DE", "Vanguard AllW" },
  { "SXR8.DE", "iShares S&P500" },
  { "EUNL.DE", "iShares World" },
  { "EQQQ.DE", "Invesco QQQ" }
};

static const FinPresetItem PRESETS_CRYPTO[] = {
  { "BTC-USD", "Bitcoin" },
  { "ETH-USD", "Ethereum" },
  { "SOL-USD", "Solana" },
  { "XRP-USD", "Ripple" },
  { "DOGE-USD","Dogecoin" }
};

static const FinPresetItem PRESETS_INDICES[] = {
  { "^GSPC",  "S&P 500" },
  { "^IXIC",  "Nasdaq Comp" },
  { "^DJI",   "Dow Jones" },
  { "^GDAXI", "DAX 40" },
  { "^FTSE",  "FTSE 100" }
};

static const FinPresetItem PRESETS_COMMODITIES[] = {
  { "GC=F", "Zlato/Gold" },
  { "SI=F", "Striebro/Silver" },
  { "CL=F", "Ropa WTI" },
  { "BZ=F", "Ropa Brent" },
  { "NG=F", "Zemny plyn" },
  { "HG=F", "Med/Copper" }
};

static const FinPresetItem PRESETS_STOCKS[] = {
  { "NVDA",  "Nvidia" },
  { "AAPL",  "Apple" },
  { "MSFT",  "Microsoft" },
  { "TSLA",  "Tesla" },
  { "AMZN",  "Amazon" },
  { "GOOGL", "Alphabet" },
  { "META",  "Meta" }
};

static const FinPresetItem PRESETS_FOREX[] = {
  { "EURUSD=X", "EUR / USD" },
  { "CZK=X",    "USD / CZK" },
  { "EURCZK=X", "EUR / CZK" }
};

static const FinPresetCat PRESET_CATS[] = {
  { "ETF",      "ETF",         PRESETS_ETF,         8 },
  { "Krypto",   "Crypto",      PRESETS_CRYPTO,      5 },
  { "Indexy",   "Indices",     PRESETS_INDICES,     5 },
  { "Komodity", "Commodities", PRESETS_COMMODITIES, 6 },
  { "Akcie",    "Stocks",      PRESETS_STOCKS,      7 },
  { "Forex",    "Forex",       PRESETS_FOREX,       3 }
};
static const int PRESET_CAT_COUNT = 6;

static void getFinanceSlotTickers(char slots[4][16]) {
  for (int i = 0; i < 4; i++) slots[i][0] = '\0';
  const char* csv = Settings_FinanceTickers();
  if (!csv || !*csv) csv = DEFAULT_FINANCE_TICKERS;
  char buf[128];
  strncpy(buf, csv, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char* tok = strtok(buf, ",; ");
  int idx = 0;
  while (tok && idx < 4) {
    while (*tok == ' ') tok++;
    if (*tok) {
      strncpy(slots[idx], tok, 15);
      slots[idx][15] = '\0';
      idx++;
    }
    tok = strtok(nullptr, ",; ");
  }
}

static void setFinanceSlotTicker(int slotIdx, const char* newTicker) {
  if (slotIdx < 0 || slotIdx >= 4) return;
  char slots[4][16];
  getFinanceSlotTickers(slots);

  if (newTicker && *newTicker) {
    strncpy(slots[slotIdx], newTicker, 15);
    slots[slotIdx][15] = '\0';
  } else {
    slots[slotIdx][0] = '\0';
  }

  char newCsv[128] = "";
  for (int i = 0; i < 4; i++) {
    if (slots[i][0] != '\0') {
      if (newCsv[0] != '\0') strncat(newCsv, ",", sizeof(newCsv) - strlen(newCsv) - 1);
      strncat(newCsv, slots[i], sizeof(newCsv) - strlen(newCsv) - 1);
    }
  }
  if (newCsv[0] == '\0') {
    strncpy(newCsv, DEFAULT_FINANCE_TICKERS, sizeof(newCsv) - 1);
  }

  Settings_SetFinanceTickers(newCsv);
  Finance_SetTickers(newCsv);
  Async_RequestFinance();
}

static int findCategoryForTicker(const char* ticker) {
  if (!ticker || !*ticker) return 0;
  for (int c = 0; c < PRESET_CAT_COUNT; c++) {
    for (int i = 0; i < PRESET_CATS[c].count; i++) {
      if (strcasecmp(PRESET_CATS[c].items[i].symbol, ticker) == 0) {
        return c;
      }
    }
  }
  return 0;
}

static void openAssetPicker(int slotIdx) {
  if (slotIdx < 0 || slotIdx >= 4) return;
  s_pickingSlot = slotIdx;
  char slots[4][16];
  getFinanceSlotTickers(slots);
  s_catIdx = findCategoryForTicker(slots[slotIdx]);
  Buzzer_Play(BEEP_CLICK);
}

static void drawSelectBox(int x, int y, int w, int h, int slotNum, const char* ticker, bool isHero, bool isActive) {
  uint16_t bg = isHero ? 0x0842 : 0x18C3;
  uint16_t border = isActive ? C_GREEN : (isHero ? C_CYAN : 0x39E7);
  gfx->fillRoundRect(x, y, w, h, 8, bg);
  gfx->drawRoundRect(x, y, w, h, 8, border);
  if (isActive || isHero) {
    gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, border);
  }

  // Draw slot number pill
  char numStr[12];
  if (isHero) snprintf(numStr, sizeof(numStr), "1*");
  else snprintf(numStr, sizeof(numStr), "%d", slotNum);
  uint16_t numBg = isHero ? 0x02EC : 0x31A6;
  gfx->fillRoundRect(x + 5, y + 5, 22, h - 10, 4, numBg);
  UI_TextCenteredIn(numStr, x + 5, 22, y + h / 2 - 4, C_WHITE, 1);

  // Draw ticker symbol
  char disp[20];
  if (ticker && *ticker) {
    strncpy(disp, ticker, sizeof(disp) - 1);
    disp[sizeof(disp) - 1] = '\0';
    if (disp[0] == '^') memmove(disp, disp + 1, strlen(disp));
  } else {
    strncpy(disp, (Lang_Get() == LANG_EN) ? "[Select]" : "[Vybrat]", sizeof(disp) - 1);
    disp[sizeof(disp) - 1] = '\0';
  }
  UI_Text(disp, x + 32, y + h / 2 - 4, isHero ? C_CYAN : C_WHITE, 1);

  // Draw drop-down triangle indicator
  int triX = x + w - 16;
  int triY = y + h / 2;
  gfx->fillTriangle(triX, triY - 3, triX + 8, triY - 3, triX + 4, triY + 3, isHero ? C_CYAN : C_GRAY);
}

static void drawAssetPickerModal() {
  const uint8_t lang = Lang_Get();
  const int PK_X = 75, PK_Y = 40, PK_W = 330, PK_H = 340;

  // Outer card
  gfx->fillRoundRect(PK_X, PK_Y, PK_W, PK_H, 16, 0x0821);
  gfx->drawRoundRect(PK_X, PK_Y, PK_W, PK_H, 16, C_CYAN);
  gfx->drawRoundRect(PK_X + 1, PK_Y + 1, PK_W - 2, PK_H - 2, 15, 0x1945);

  // Title
  char title[48];
  if (s_pickingSlot == 0) {
    snprintf(title, sizeof(title), (lang == LANG_EN) ? "Slot 1 (Hero): Choose Asset"
                                 : ((lang == LANG_SK) ? "Pozicia 1 (Hero): Vyber aktiva" : "Pozice 1 (Hero): Vyber aktiva"));
  } else {
    snprintf(title, sizeof(title), (lang == LANG_EN) ? "Slot %d: Choose Asset"
                                 : ((lang == LANG_SK) ? "Pozicia %d: Vyber aktiva" : "Pozice %d: Vyber aktiva"),
             s_pickingSlot + 1);
  }
  UI_TextCenteredIn(title, PK_X, PK_W, PK_Y + 10, C_WHITE, 1);

  // Close 'X' in top-right
  const int cx = PK_X + PK_W - 18, cy = PK_Y + 14;
  gfx->fillCircle(cx, cy, 10, 0x3000);
  gfx->drawCircle(cx, cy, 10, C_GRAY);
  gfx->drawLine(cx - 4, cy - 4, cx + 4, cy + 4, C_WHITE);
  gfx->drawLine(cx - 4, cy + 4, cx + 4, cy - 4, C_WHITE);

  // Category buttons (2 rows of 3)
  const int tabW = 98, tabH = 24;
  for (int c = 0; c < PRESET_CAT_COUNT; c++) {
    int col = c % 3;
    int row = c / 3;
    int tx = PK_X + 10 + col * (tabW + 8);
    int ty = PK_Y + 28 + row * (tabH + 4);
    bool active = (c == s_catIdx);

    uint16_t bg = active ? C_CYAN : 0x1945;
    uint16_t border = active ? C_WHITE : C_GRAY;
    uint16_t fg = active ? C_BLACK : C_WHITE;
    gfx->fillRoundRect(tx, ty, tabW, tabH, 6, bg);
    gfx->drawRoundRect(tx, ty, tabW, tabH, 6, border);

    const char* cName = (lang == LANG_EN) ? PRESET_CATS[c].nameEn : PRESET_CATS[c].nameSk;
    UI_TextCenteredIn(cName, tx, tabW, ty + tabH / 2 - 4, fg, 1);
  }

  // Divider
  gfx->drawFastHLine(PK_X + 10, PK_Y + 85, PK_W - 20, 0x31A6);

  // Current slot's ticker for checkmark comparison
  char slots[4][16];
  getFinanceSlotTickers(slots);
  const char* curTk = (s_pickingSlot >= 0 && s_pickingSlot < 4) ? slots[s_pickingSlot] : "";

  // Items grid for selected category (2 columns of up to 4 rows)
  const FinPresetCat& cat = PRESET_CATS[s_catIdx];
  const int itW = 148, itH = 36;
  for (int i = 0; i < cat.count && i < 8; i++) {
    int col = i % 2;
    int row = i / 2;
    int ix = PK_X + 10 + col * (itW + 14);
    int iy = PK_Y + 92 + row * (itH + 6);

    const FinPresetItem& it = cat.items[i];
    bool isSelected = (curTk[0] != '\0' && strcasecmp(curTk, it.symbol) == 0);

    uint16_t bg = isSelected ? 0x0922 : 0x10A3;
    uint16_t border = isSelected ? C_GREEN : 0x29E8;
    gfx->fillRoundRect(ix, iy, itW, itH, 6, bg);
    gfx->drawRoundRect(ix, iy, itW, itH, 6, border);
    if (isSelected) {
      gfx->drawRoundRect(ix + 1, iy + 1, itW - 2, itH - 2, 5, border);
    }

    // Symbol (top line)
    char symDisp[16];
    strncpy(symDisp, it.symbol, sizeof(symDisp) - 1);
    symDisp[sizeof(symDisp) - 1] = '\0';
    if (symDisp[0] == '^') memmove(symDisp, symDisp + 1, strlen(symDisp));
    UI_Text(symDisp, ix + 8, iy + 4, isSelected ? C_GREEN : C_CYAN, 1);

    // Checkmark if selected
    if (isSelected) {
      UI_Text("OK", ix + itW - 20, iy + 4, C_GREEN, 1);
    }

    // Label (bottom line)
    UI_Text(it.label, ix + 8, iy + 20, C_LTGRAY, 1);
  }

  // Bottom action buttons
  const int btnY = PK_Y + 266;
  const int btnH = 32;

  // Clear slot button
  gfx->fillRoundRect(PK_X + 10, btnY, itW, btnH, 6, 0x3000);
  gfx->drawRoundRect(PK_X + 10, btnY, itW, btnH, 6, 0x8183);
  const char* clrTxt = (lang == LANG_EN) ? "Clear Slot" : "Vymazat slot";
  UI_TextCenteredIn(clrTxt, PK_X + 10, itW, btnY + btnH / 2 - 4, 0xF986, 1);

  // Back button
  gfx->fillRoundRect(PK_X + 172, btnY, itW, btnH, 6, 0x2104);
  gfx->drawRoundRect(PK_X + 172, btnY, itW, btnH, 6, C_GRAY);
  const char* bckTxt = (lang == LANG_EN) ? "Back" : ((lang == LANG_SK) ? "Spat" : "Zpet");
  UI_TextCenteredIn(bckTxt, PK_X + 172, itW, btnY + btnH / 2 - 4, C_WHITE, 1);

  // Bottom swipe hint
  UI_TextCenteredIn((lang == LANG_EN) ? "^ swipe up to close ^" : "^ potiahnutim zatvorite ^",
                    PK_X, PK_W, PK_Y + PK_H - 18, C_GRAY, 1);
}

static bool handleAssetPickerTap(int x, int y) {
  const int PK_X = 75, PK_Y = 40, PK_W = 330, PK_H = 340;

  // Tap outside card -> close picker
  if (x < PK_X || x > PK_X + PK_W || y < PK_Y || y > PK_Y + PK_H) {
    s_pickingSlot = -1;
    return true;
  }

  // Close button tap
  const int cx = PK_X + PK_W - 18, cy = PK_Y + 14;
  if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= 14 * 14) {
    s_pickingSlot = -1;
    Buzzer_Play(BEEP_CLICK);
    return true;
  }

  // Category buttons (2 rows of 3)
  const int tabW = 98, tabH = 24;
  for (int c = 0; c < PRESET_CAT_COUNT; c++) {
    int col = c % 3;
    int row = c / 3;
    int tx = PK_X + 10 + col * (tabW + 8);
    int ty = PK_Y + 28 + row * (tabH + 4);
    if (x >= tx && x <= tx + tabW && y >= ty && y <= ty + tabH) {
      s_catIdx = c;
      Buzzer_Play(BEEP_CLICK);
      return true;
    }
  }

  // Items grid
  const FinPresetCat& cat = PRESET_CATS[s_catIdx];
  const int itW = 148, itH = 36;
  for (int i = 0; i < cat.count && i < 8; i++) {
    int col = i % 2;
    int row = i / 2;
    int ix = PK_X + 10 + col * (itW + 14);
    int iy = PK_Y + 92 + row * (itH + 6);
    if (x >= ix && x <= ix + itW && y >= iy && y <= iy + itH) {
      setFinanceSlotTicker(s_pickingSlot, cat.items[i].symbol);
      Buzzer_Play(BEEP_CLICK);
      s_pickingSlot = -1;
      return true;
    }
  }

  // Bottom action buttons
  const int btnY = PK_Y + 266;
  const int btnH = 32;

  // Clear slot button
  if (x >= PK_X + 10 && x <= PK_X + 10 + itW && y >= btnY && y <= btnY + btnH) {
    setFinanceSlotTicker(s_pickingSlot, "");
    Buzzer_Play(BEEP_CLICK);
    s_pickingSlot = -1;
    return true;
  }

  // Back button
  if (x >= PK_X + 172 && x <= PK_X + 172 + itW && y >= btnY && y <= btnY + btnH) {
    s_pickingSlot = -1;
    Buzzer_Play(BEEP_CLICK);
    return true;
  }

  return true;
}

static void drawButton(int x, int y, int w, int h, const char* label, bool active, uint16_t activeCol = C_CYAN) {
  uint16_t bg = active ? activeCol : 0x2104; // dark gray if inactive
  uint16_t fg = active ? C_BLACK   : C_WHITE;
  gfx->fillRoundRect(x, y, w, h, 8, bg);
  gfx->drawRoundRect(x, y, w, h, 8, active ? C_WHITE : C_GRAY);
  UI_TextCenteredIn(label, x, w, y + h / 2 - 7, fg, 1);
}


void QuickControl_Draw(int currentScreen) {
  if (!s_open) return;

  if (s_pickingSlot >= 0 && currentScreen == SCREEN_FINANCE_I) {
    drawAssetPickerModal();
    return;
  }

  // Dark backing card with rounded corners and glowing border
  gfx->fillRoundRect(CW_X, CW_Y, CW_W, CW_H, 16, 0x0821);
  gfx->drawRoundRect(CW_X, CW_Y, CW_W, CW_H, 16, C_CYAN);

  // Title
  const char* ccTitle = (Lang_Get() == LANG_EN) ? "CONTROL CENTER" : ((Lang_Get() == LANG_CZ) ? "OVLÁDACÍ CENTRUM" : "OVLÁDACIE CENTRUM");
  UI_TextCenteredIn(ccTitle, CW_X, CW_W, CW_Y + 10, C_WHITE, 1);

  // Close button 'X' in top-right
  const int cx = CW_X + CW_W - 20, cy = CW_Y + 16;
  gfx->fillCircle(cx, cy, 10, 0x3000);
  gfx->drawCircle(cx, cy, 10, C_GRAY);
  gfx->drawLine(cx - 4, cy - 4, cx + 4, cy + 4, C_WHITE);
  gfx->drawLine(cx - 4, cy + 4, cx + 4, cy - 4, C_WHITE);

  // --- Row 1: Brightness [-] [Jas: XX%] [+] ---
  const int y1 = CW_Y + 36;
  drawButton(CW_X + 16, y1, 44, 32, "-", false);
  drawButton(CW_X + CW_W - 16 - 44, y1, 44, 32, "+", false);

  char bTxt[32];
  snprintf(bTxt, sizeof(bTxt), "%s: %u%%", T(S_BRIGHTNESS), (unsigned)Settings_Backlight());
  UI_TextCenteredIn(bTxt, CW_X + 64, CW_W - 128, y1 + 7, C_YELLOW, 2);

  // --- Row 2: Night Mode & Full Settings ---
  const int y2 = CW_Y + 74;
  const char* nLabel = Settings_NightAuto() ? ((Lang_Get() == LANG_EN) ? "Night: AUTO" : "Noc: AUTO")
                     : (Settings_IsNight() ? ((Lang_Get() == LANG_EN) ? "Night: ON" : "Noc: ZAP") : ((Lang_Get() == LANG_EN) ? "Night: OFF" : "Noc: VYP"));
  drawButton(CW_X + 16, y2, 158, 32, nLabel, Settings_IsNight(), 0x3BDF);
  const char* allSettings = (Lang_Get() == LANG_EN) ? "All Settings" : ((Lang_Get() == LANG_CZ) ? "Vsechna nastaveni" : "Všetky nastavenia");
  drawButton(CW_X + 186, y2, 158, 32, allSettings, false);

  // Divider
  gfx->drawFastHLine(CW_X + 16, CW_Y + 114, CW_W - 32, 0x31A6);

  // --- Rows 3 & 4: Screen-specific quick toggles ---
  const int y3 = CW_Y + 122;
  const int y4 = CW_Y + 160;

  if (currentScreen == SCREEN_PLANES_I) {
    const char* airOn = (Lang_Get() == LANG_EN) ? "Airports: ON" : "Letiská: ZAP";
    const char* airOff = (Lang_Get() == LANG_EN) ? "Airports: OFF" : "Letiská: VYP";
    const char* ringOn = (Lang_Get() == LANG_EN) ? "Rings: ON" : "Okruhy: ZAP";
    const char* ringOff = (Lang_Get() == LANG_EN) ? "Rings: OFF" : "Okruhy: VYP";
    const char* legOn = (Lang_Get() == LANG_EN) ? "Labels: ON" : "Popisy: ZAP";
    const char* legOff = (Lang_Get() == LANG_EN) ? "Labels: OFF" : "Popisy: VYP";
    const char* compOn = (Lang_Get() == LANG_EN) ? "Compass: ON" : "Kompas: ZAP";
    const char* compOff = (Lang_Get() == LANG_EN) ? "Compass: OFF" : "Kompas: VYP";
    drawButton(CW_X + 16,  y3, 158, 34, Settings_RadarShowAirports() ? airOn : airOff, Settings_RadarShowAirports());
    drawButton(CW_X + 186, y3, 158, 34, Settings_RadarShowRings()    ? ringOn : ringOff,  Settings_RadarShowRings());
    drawButton(CW_X + 16,  y4, 158, 34, Settings_ShowLegends()       ? legOn : legOff,  Settings_ShowLegends());
    drawButton(CW_X + 186, y4, 158, 34, Settings_RadarShowCompass()  ? compOn : compOff,  Settings_RadarShowCompass());
  } else if (currentScreen == SCREEN_TACTICAL_I) {
    const char* trOn = (Lang_Get() == LANG_EN) ? "Trails: ON" : "Trasy: ZAP";
    const char* trOff = (Lang_Get() == LANG_EN) ? "Trails: OFF" : "Trasy: VYP";
    drawButton(CW_X + 16,  y3, 158, 34, Settings_RadarShowTrails()   ? trOn : trOff,   Settings_RadarShowTrails());
    const char* tSrc = (Settings_RadarSource() == RADAR_SRC_SHMU) ? ((Lang_Get() == LANG_EN) ? "Source: SHMU" : "Zdroj: SHMU") :
                       (Settings_RadarSource() == RADAR_SRC_RAINVIEWER) ? ((Lang_Get() == LANG_EN) ? "Source: RainViewer" : "Zdroj: RainViewer") : ((Lang_Get() == LANG_EN) ? "Source: CHMU" : "Zdroj: CHMU");
    drawButton(CW_X + 186, y3, 158, 34, tSrc, false);
    const char* smLabel = Settings_SmoothRadar() ? ((Lang_Get() == LANG_EN) ? "Smooth: ON" : "Vyhladenie: ZAP") : ((Lang_Get() == LANG_EN) ? "Smooth: OFF" : "Vyhladenie: VYP");
    drawButton(CW_X + 16,  y4, 158, 34, smLabel, Settings_SmoothRadar(), 0x07E0);
    const char* airOn = (Lang_Get() == LANG_EN) ? "Airports: ON" : "Letiská: ZAP";
    const char* airOff = (Lang_Get() == LANG_EN) ? "Airports: OFF" : "Letiská: VYP";
    drawButton(CW_X + 186, y4, 158, 34, Settings_RadarShowAirports() ? airOn : airOff, Settings_RadarShowAirports());
  } else if (currentScreen == SCREEN_METEO_I) {
    const char* mSrc = (Settings_RadarSource() == RADAR_SRC_SHMU) ? ((Lang_Get() == LANG_EN) ? "Source: SHMU" : "Zdroj: SHMU") :
                       (Settings_RadarSource() == RADAR_SRC_RAINVIEWER) ? ((Lang_Get() == LANG_EN) ? "Source: RainViewer" : "Zdroj: RainViewer") : ((Lang_Get() == LANG_EN) ? "Source: CHMU" : "Zdroj: CHMU");
    drawButton(CW_X + 16,  y3, 158, 34, mSrc, false);
    const char* smLabel = Settings_SmoothRadar() ? ((Lang_Get() == LANG_EN) ? "Smooth: ON" : "Vyhladenie: ZAP") : ((Lang_Get() == LANG_EN) ? "Smooth: OFF" : "Vyhladenie: VYP");
    drawButton(CW_X + 186, y3, 158, 34, smLabel, Settings_SmoothRadar(), 0x07E0);
    const char* swSrc = (Lang_Get() == LANG_EN) ? "Switch Radar Source" : ((Lang_Get() == LANG_CZ) ? "Prepnout radarovy zdroj" : "Prepnúť radarový zdroj");
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, swSrc, true, 0x07E0);
  } else if (currentScreen == SCREEN_CLOCK_I) {
    const char* cStyle = (Lang_Get() == LANG_EN) ? "Clock: Digital" : "Ciferník: Digitálny";
    switch (Settings_ClockStyle()) {
      case 0: cStyle = (Lang_Get() == LANG_EN) ? "Clock: Digital" : "Ciferník: Digitálny"; break;
      case 1: cStyle = (Lang_Get() == LANG_EN) ? "Clock: Modern Analog" : "Ciferník: Moderný Analógový"; break;
      case 2: cStyle = (Lang_Get() == LANG_EN) ? "Clock: Modern Digital" : "Ciferník: Moderný Digitálny"; break;
      case 3: cStyle = (Lang_Get() == LANG_EN) ? "Clock: Retro LCD" : "Ciferník: Retro LCD"; break;
    }
    drawButton(CW_X + 16,  y3, CW_W - 32, 34, cStyle, false);
    
    const char* sArc = (Lang_Get() == LANG_EN) ? 
        (Settings_ClockShowAstro() ? "Solar Arc: ON" : "Solar Arc: OFF") :
        (Settings_ClockShowAstro() ? "Solárny oblúk: ZAP" : "Solárny oblúk: VYP");
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, sArc, Settings_ClockShowAstro());
  } else if (currentScreen == SCREEN_FORECAST_I) {
    const char* uMet = (Lang_Get() == LANG_EN) ? "Units: Metric" : "Jednotky: Metrické";
    const char* uAv = (Lang_Get() == LANG_EN) ? "Units: Aviation" : "Jednotky: Letecké";
    drawButton(CW_X + 16,  y3, CW_W - 32, 34, Settings_MetricUnits() ? uMet : uAv, false);
    const char* upd = (Lang_Get() == LANG_EN) ? "Update Forecast" : ((Lang_Get() == LANG_CZ) ? "Aktualizovat predpoved" : "Aktualizovať predpoveď");
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, upd, true, 0x07E0);
  } else if (currentScreen == SCREEN_ISS_I) {
    const char* aLblOn = (Lang_Get() == LANG_EN) ? "Flyover Alert: ON" : "Výstraha preletu: ZAP";
    const char* aLblOff = (Lang_Get() == LANG_EN) ? "Flyover Alert: OFF" : "Výstraha preletu: VYP";
    drawButton(CW_X + 16,  y3, CW_W - 32, 34, Settings_IssAlert() ? aLblOn : aLblOff, Settings_IssAlert(), 0x07E0);
    const char* updIss = (Lang_Get() == LANG_EN) ? "Update ISS Location" : ((Lang_Get() == LANG_CZ) ? "Aktualizovat polohu ISS" : "Aktualizovať polohu ISS");
    drawButton(CW_X + 16,  y4, CW_W - 32, 34, updIss, true, 0x07E0);
  } else if (currentScreen == SCREEN_FINANCE_I) {
    char slots[4][16];
    getFinanceSlotTickers(slots);
    int actIdx = Finance_ActiveIndex();
    drawSelectBox(CW_X + 16,  y3, 158, 32, 1, slots[0], true,  actIdx == 0);
    drawSelectBox(CW_X + 186, y3, 158, 32, 2, slots[1], false, actIdx == 1);
    drawSelectBox(CW_X + 16,  y4, 158, 32, 3, slots[2], false, actIdx == 2);
    drawSelectBox(CW_X + 186, y4, 158, 32, 4, slots[3], false, actIdx == 3);

    const int y5 = CW_Y + 194;
    const bool isCandle = (Settings_FinanceGraphType() == FIN_GRAPH_CANDLESTICK);
    const char* grpLabel = isCandle ? "Graf: Svieckovy" : "Graf: Ciarovy";
    if (Lang_Get() == LANG_EN) {
      grpLabel = isCandle ? "Chart: Candlestick" : "Chart: Line";
    } else if (Lang_Get() == LANG_CZ) {
      grpLabel = isCandle ? "Graf: Svíckový" : "Graf: Cárový";
    }
    drawButton(CW_X + 16, y5, CW_W - 32, 32, grpLabel, isCandle, 0x07E0);
  }

  // Bottom pull-up hint
  const char* hint = (Lang_Get() == LANG_EN) ? "^ swipe up to close ^" : ((Lang_Get() == LANG_CZ) ? "^ potazenim nahoru zavrete ^" : "^ potiahnutím hore zatvoríte ^");
  UI_TextCenteredIn(hint, CW_X, CW_W, CW_Y + CW_H - 20, C_GRAY, 1);
}

static void switchRadarSource() {
  uint8_t curSrc = Settings_RadarSource();
  uint8_t nextSrc = (curSrc == RADAR_SRC_CHMU) ? RADAR_SRC_SHMU :
                    (curSrc == RADAR_SRC_SHMU) ? RADAR_SRC_RAINVIEWER : RADAR_SRC_CHMU;
  Settings_SetRadarSource(nextSrc);
  if (nextSrc == RADAR_SRC_RAINVIEWER) {
    CHMU_FreeBuffers();
    SHMU_FreeBuffers();
  } else if (nextSrc == RADAR_SRC_CHMU) {
    RainViewer_FreeBuffers();
    SHMU_FreeBuffers();
  } else if (nextSrc == RADAR_SRC_SHMU) {
    RainViewer_FreeBuffers();
    CHMU_FreeBuffers();
  }
  ScreenWeather_FreeBuffers();
  Async_RequestRadar();
}

bool QuickControl_HandleTap(int x, int y, int currentScreen) {
  if (!s_open) return false;

  if (s_pickingSlot >= 0 && currentScreen == SCREEN_FINANCE_I) {
    return handleAssetPickerTap(x, y);
  }

  // Tap outside card -> close
  if (x < CW_X || x > CW_X + CW_W || y < CW_Y || y > CW_Y + CW_H) {
    s_open = false;
    return true;
  }

  // Close button tap
  const int cx = CW_X + CW_W - 20, cy = CW_Y + 16;
  if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= 16 * 16) {
    s_open = false;
    return true;
  }

  // Row 1: Brightness [-] and [+]
  const int y1 = CW_Y + 36;
  if (y >= y1 && y <= y1 + 32) {
    int curB = (int)Settings_Backlight();
    if (x >= CW_X + 16 && x <= CW_X + 16 + 44) {
      curB -= 15;
      if (curB < 10) curB = 10;
      Settings_SetBacklight((uint8_t)curB);
      Set_Backlight((uint8_t)curB);
      return true;
    }
    if (x >= CW_X + CW_W - 16 - 44 && x <= CW_X + CW_W - 16) {
      curB += 15;
      if (curB > 100) curB = 100;
      Settings_SetBacklight((uint8_t)curB);
      Set_Backlight((uint8_t)curB);
      return true;
    }
  }

  // Row 2: Night Mode & Full Settings
  const int y2 = CW_Y + 74;
  if (y >= y2 && y <= y2 + 32) {
    if (x >= CW_X + 16 && x <= CW_X + 174) {
      bool autoN = Settings_NightAuto();
      if (autoN) {
        Settings_SetNightAuto(false);
        Settings_SetNight(true);
      } else if (Settings_IsNight()) {
        Settings_SetNight(false);
      } else {
        Settings_SetNightAuto(true);
      }
      NightMode_Apply();
      return true;
    }
    if (x >= CW_X + 186 && x <= CW_X + 344) {
      s_open = false;
      gotoScreen(SCREEN_SETTINGS_I);
      return true;
    }
  }

  // Rows 3 & 4: Screen-specific toggles
  const int y3 = CW_Y + 122;
  const int y4 = CW_Y + 160;

  if (currentScreen == SCREEN_PLANES_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetRadarShowAirports(!Settings_RadarShowAirports());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetRadarShowRings(!Settings_RadarShowRings());
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetShowLegends(!Settings_ShowLegends());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetRadarShowCompass(!Settings_RadarShowCompass());
        return true;
      }
    }
  } else if (currentScreen == SCREEN_TACTICAL_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetRadarShowTrails(!Settings_RadarShowTrails());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        switchRadarSource();
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        Settings_SetSmoothRadar(!Settings_SmoothRadar());
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetRadarShowAirports(!Settings_RadarShowAirports());
        return true;
      }
    }
  } else if (currentScreen == SCREEN_METEO_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        switchRadarSource();
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        Settings_SetSmoothRadar(!Settings_SmoothRadar());
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      switchRadarSource();
      return true;
    }
  } else if (currentScreen == SCREEN_CLOCK_I) {
    if (y >= y3 && y <= y3 + 34) {
      uint8_t nextSt = (Settings_ClockStyle() + 1) % (CLOCK_STYLE_MAX + 1);
      Settings_SetClockStyle(nextSt);
      return true;
    }
    if (y >= y4 && y <= y4 + 34) {
      Settings_SetClockShowAstro(!Settings_ClockShowAstro());
      return true;
    }
  } else if (currentScreen == SCREEN_FORECAST_I) {
    if (y >= y3 && y <= y3 + 34) {
      Settings_SetMetricUnits(!Settings_MetricUnits());
      return true;
    }
  } else if (currentScreen == SCREEN_ISS_I) {
    if (y >= y3 && y <= y3 + 34) {
      Settings_SetIssAlert(!Settings_IssAlert());
      return true;
    }
    if (y >= y4 && y <= y4 + 34) {
      Async_RequestIss();
      return true;
    }
  } else if (currentScreen == SCREEN_FINANCE_I) {
    if (y >= y3 && y <= y3 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        openAssetPicker(0);
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        openAssetPicker(1);
        return true;
      }
    }
    if (y >= y4 && y <= y4 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + 174) {
        openAssetPicker(2);
        return true;
      }
      if (x >= CW_X + 186 && x <= CW_X + 344) {
        openAssetPicker(3);
        return true;
      }
    }
    const int y5 = CW_Y + 194;
    if (y >= y5 && y <= y5 + 34) {
      if (x >= CW_X + 16 && x <= CW_X + CW_W - 16) {
        uint8_t cur = Settings_FinanceGraphType();
        Settings_SetFinanceGraphType(cur == FIN_GRAPH_CANDLESTICK ? FIN_GRAPH_LINE : FIN_GRAPH_CANDLESTICK);
        Buzzer_Play(BEEP_CLICK);
        return true;
      }
    }
  }

  return true;
}
