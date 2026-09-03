// =============================================================================
//  MeteoPlaneRadar
//  Shared UI helpers - colours, global gfx, interface.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#pragma once
#include <Arduino_GFX_Library.h>
#include "FontEngine.h"

// Colours (RGB565)
#define C_BLACK  0x0000
#define C_BLUE   0x001F
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_WHITE  0xFFFF
#define C_YELLOW 0xFFE0
#define C_GRAY   0x8410
#define C_LTGRAY 0xC618
#define C_DKGRAY 0x2124
#define C_CYAN   0x05FF
#define C_ORANGE 0xFC00   // altitude band 2-6 km

// Global display (defined in the .ino).
extern Arduino_GFX* gfx;

// Draw a WiFi QR code (for joining the AP). open=true -> open network.
void UI_DrawWifiQR(const char* ssid, const char* password, bool open,
                   int x, int y, int size_px);

// Horizontally centred text (size 1-4).
void UI_TextCentered(const char* text, int cy, uint16_t color, uint8_t size);

// Text centred inside the rectangle [x, x+w) - used for labels above the map.
void UI_TextCenteredIn(const char* text, int x, int w, int cy,
                       uint16_t color, uint8_t size);

// Render UTF-8 text at (x, y) with top-left anchor.
void UI_Text(const char* text, int x, int y, uint16_t color, uint8_t size = 1);

// Arduino String convenience overloads
inline void UI_TextCentered(const String& text, int cy, uint16_t color, uint8_t size) {
  UI_TextCentered(text.c_str(), cy, color, size);
}
inline void UI_TextCenteredIn(const String& text, int x, int w, int cy, uint16_t color, uint8_t size) {
  UI_TextCenteredIn(text.c_str(), x, w, cy, color, size);
}
inline void UI_Text(const String& text, int x, int y, uint16_t color, uint8_t size = 1) {
  UI_Text(text.c_str(), x, y, color, size);
}

// Half the width of the display circle at height y - i.e. how much room a line
// of text actually has there. On a round panel the usable width shrinks fast
// towards the top, so anything near the edge has to be measured, not assumed.

// The clock + outside temperature line, centred under the screen dots. Draws
// nothing when neither is known yet, and refuses to draw text that would not
// fit inside the circle rather than letting it run off the edge.
void UI_DrawStatusLine(int cy);

// Unified range / zoom indicator across all radar screens.
// Standardizes dot size (r=4), spacing (gap=20), positioning (LY_RANGE_DOTS),
// text backing, and visibility.
void UI_DrawRangeIndicator(const char* text, int activeIdx, int totalCount, bool showText = true);

// Unified home position marker (cyan outer ring, yellow inner ring, white center dot, crosshair)
void UI_DrawHomeMarker(int x, int y);

struct Aircraft;
struct RouteInfo;

// Unified, richly color-coded aircraft detail card for ScreenPlanes & ScreenTactical
void UI_DrawAircraftDetail(const Aircraft& ac, const RouteInfo* rt, int routeState, bool signalLost);
