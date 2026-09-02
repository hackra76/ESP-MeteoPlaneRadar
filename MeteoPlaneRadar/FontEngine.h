// =============================================================================
//  MeteoPlaneRadar
//  FontEngine.h - High-quality smooth UTF-8 typography engine using U8g2 fonts.
//
//  Bridges Arduino_GFX (PSRAM canvas) with U8g2's UTF-8 text rendering engine.
//  Provides full Central/Eastern European Latin-2 support (Slovak, Czech, Polish).
// =============================================================================
#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <U8g2_for_Adafruit_GFX.h>

// Font size enumeration
enum FontSize : uint8_t {
  FONT_TINY   = 0,  // helvR08_te  (~8 px height)
  FONT_SMALL  = 1,  // helvB10_te  (~10 px height)  - maps to legacy size 1
  FONT_MEDIUM = 2,  // helvB12_te  (~12 px height)  - maps to legacy size 2
  FONT_LARGE  = 3,  // helvR14_te  (~14 px height)  - maps to legacy size 3
  FONT_TITLE  = 4,  // helvR18_te  (~18 px height)
  FONT_HUGE   = 5,  // helvB24_te  (~24 px height)
  FONT_CLOCK  = 8   // logisoso58_tn (~58 px height, large clock digits)
};

// Initialize FontEngine with the active Arduino_GFX canvas
void Font_Init(Arduino_GFX* gfx);

// Set typography state
void Font_SetSize(uint8_t size);
void Font_SetColor(uint16_t fg, uint16_t bg = 0, bool transparent = true);

// Drawing routines (coordinates use TOP-LEFT anchor, matching gfx->setCursor())
void Font_Draw(const char* str, int16_t x, int16_t y);
void Font_Draw(const char* str, int16_t x, int16_t y, uint16_t color, uint8_t size);
void Font_DrawCentered(const char* str, int16_t cx, int16_t y);
void Font_DrawCentered(const char* str, int16_t cx, int16_t y, uint16_t color, uint8_t size);
void Font_DrawCenteredIn(const char* str, int16_t x, int16_t w, int16_t y, uint16_t color, uint8_t size);

// Measurement
int16_t Font_TextWidth(const char* str, uint8_t size = 0);
int16_t Font_TextHeight(uint8_t size = 0);

// Access underlying U8g2 renderer
U8G2_FOR_ADAFRUIT_GFX& Font_GetU8g2();
