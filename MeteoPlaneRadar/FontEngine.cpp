// =============================================================================
//  MeteoPlaneRadar
//  FontEngine.cpp - High-quality smooth UTF-8 typography engine using U8g2 fonts.
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#include "FontEngine.h"
#include <Adafruit_GFX.h>

// Bridge adapter allowing U8g2_for_Adafruit_GFX to draw directly into Arduino_GFX
class ArduinoGfxAdapter : public Adafruit_GFX {
public:
  Arduino_GFX* target = nullptr;
  ArduinoGfxAdapter() : Adafruit_GFX(480, 480) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (target) target->drawPixel(x, y, color);
  }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
    if (target) target->drawFastHLine(x, y, w, color);
  }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
    if (target) target->drawFastVLine(x, y, h, color);
  }
};

static ArduinoGfxAdapter       s_adapter;
static U8G2_FOR_ADAFRUIT_GFX  s_u8g2;
static uint8_t                s_curSize = 1;
static uint16_t               s_curFg = 0xFFFF;
static uint16_t               s_curBg = 0x0000;
static bool                   s_initialized = false;

static const uint8_t* getFontForSize(uint8_t size) {
  switch (size) {
    case FONT_TINY:   return u8g2_font_helvR08_te;
    case FONT_SMALL:  return u8g2_font_helvB10_te;
    case FONT_MEDIUM: return u8g2_font_helvB12_te;
    case FONT_LARGE:  return u8g2_font_helvR14_te;
    case FONT_TITLE:  return u8g2_font_helvR18_te;
    case FONT_HUGE:   return u8g2_font_helvB24_te;
    case FONT_CLOCK:  return u8g2_font_logisoso58_tn;
    default:
      if (size >= 8) return u8g2_font_logisoso58_tn;
      if (size <= 1) return u8g2_font_helvB10_te;
      if (size == 2) return u8g2_font_helvB12_te;
      if (size == 3) return u8g2_font_helvR14_te;
      return u8g2_font_helvR18_te;
  }
}

void Font_Init(Arduino_GFX* gfx) {
  s_adapter.target = gfx;
  if (!s_initialized) {
    s_u8g2.begin(s_adapter);
    s_u8g2.setFontMode(1); // transparent
    Font_SetSize(FONT_SMALL);
    Font_SetColor(0xFFFF);
    s_initialized = true;
  }
}

void Font_SetSize(uint8_t size) {
  s_curSize = size;
  s_u8g2.setFont(getFontForSize(size));
}

void Font_SetColor(uint16_t fg, uint16_t bg, bool transparent) {
  s_curFg = fg;
  s_curBg = bg;
  s_u8g2.setForegroundColor(fg);
  if (!transparent) {
    s_u8g2.setBackgroundColor(bg);
    s_u8g2.setFontMode(0);
  } else {
    s_u8g2.setFontMode(1);
  }
}

void Font_Draw(const char* str, int16_t x, int16_t y) {
  if (!str || !*str) return;
  int16_t by = y + s_u8g2.getFontAscent();
  s_u8g2.drawUTF8(x, by, str);
}

void Font_Draw(const char* str, int16_t x, int16_t y, uint16_t color, uint8_t size) {
  if (!str || !*str) return;
  if (size != s_curSize) Font_SetSize(size);
  if (color != s_curFg) Font_SetColor(color);
  int16_t by = y + s_u8g2.getFontAscent();
  s_u8g2.drawUTF8(x, by, str);
}

void Font_DrawCentered(const char* str, int16_t cx, int16_t y) {
  if (!str || !*str) return;
  int16_t tw = s_u8g2.getUTF8Width(str);
  int16_t by = y + s_u8g2.getFontAscent();
  s_u8g2.drawUTF8(cx - tw / 2, by, str);
}

void Font_DrawCentered(const char* str, int16_t cx, int16_t y, uint16_t color, uint8_t size) {
  if (!str || !*str) return;
  if (size != s_curSize) Font_SetSize(size);
  if (color != s_curFg) Font_SetColor(color);
  int16_t tw = s_u8g2.getUTF8Width(str);
  int16_t by = y + s_u8g2.getFontAscent();
  s_u8g2.drawUTF8(cx - tw / 2, by, str);
}

void Font_DrawCenteredIn(const char* str, int16_t x, int16_t w, int16_t y, uint16_t color, uint8_t size) {
  if (!str || !*str) return;
  if (size != s_curSize) Font_SetSize(size);
  if (color != s_curFg) Font_SetColor(color);
  int16_t tw = s_u8g2.getUTF8Width(str);
  int16_t by = y + s_u8g2.getFontAscent();
  s_u8g2.drawUTF8(x + (w - tw) / 2, by, str);
}

int16_t Font_TextWidth(const char* str, uint8_t size) {
  if (!str || !*str) return 0;
  if (size != 0 && size != s_curSize) {
    const uint8_t* f = getFontForSize(size);
    const uint8_t* old = s_u8g2.u8g2.font;
    s_u8g2.setFont(f);
    int16_t tw = s_u8g2.getUTF8Width(str);
    s_u8g2.setFont(old);
    return tw;
  }
  return s_u8g2.getUTF8Width(str);
}

int16_t Font_TextHeight(uint8_t size) {
  if (size >= 8) return 58;
  switch (size) {
    case FONT_TINY:   return 8;
    case FONT_SMALL:  return 10;
    case FONT_MEDIUM: return 12;
    case FONT_LARGE:  return 14;
    case FONT_TITLE:  return 18;
    case FONT_HUGE:   return 24;
    default:
      if (size <= 1) return 10;
      if (size == 2) return 12;
      if (size == 3) return 14;
      return 18;
  }
}

U8G2_FOR_ADAFRUIT_GFX& Font_GetU8g2() {
  return s_u8g2;
}
