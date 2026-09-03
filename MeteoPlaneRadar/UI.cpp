// =============================================================================
//  MeteoPlaneRadar
//  Shared UI helpers (text, QR code).
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "UI.h"
#include "Outside.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include <math.h>
#include "qrcode.h"
#include "Display_ST7701.h"

void UI_TextCenteredIn(const char* text, int x, int w, int cy,
                       uint16_t color, uint8_t size) {
  Font_DrawCenteredIn(text, x, w, cy, color, size);
}

void UI_TextCentered(const char* text, int cy, uint16_t color, uint8_t size) {
  Font_DrawCentered(text, LCD_WIDTH / 2, cy, color, size);
}

void UI_Text(const char* text, int x, int y, uint16_t color, uint8_t size) {
  Font_Draw(text, x, y, color, size);
}

static int UI_ChordHalfWidth(int y) {
  const int R = LCD_WIDTH / 2 - 2;          // same margin as the screens use
  long dy = (long)y - LCD_HEIGHT / 2;
  long d2 = (long)R * R - dy * dy;
  if (d2 <= 0) return 0;
  return (int)sqrtf((float)d2);
}

void UI_DrawStatusLine(int cy) {
  char txt[OUTSIDE_TEXT_MAX];
  Outside_StatusText(txt, sizeof(txt));
  if (!txt[0]) return;                      // nothing known yet - leave it empty

  int16_t tw = Font_TextWidth(txt, 2);
  int room = 2 * UI_ChordHalfWidth(cy + 16) - 8;
  if (tw > room) return;

  // Dark backing - on the weather screen this sits straight on top of the radar
  // image, where white on yellow rain would be unreadable.
  gfx->fillRect(LCD_WIDTH / 2 - tw / 2 - 6, cy - 3, tw + 12, 22, C_BLACK);
  Font_DrawCentered(txt, LCD_WIDTH / 2, cy, C_WHITE, 2);
}

void UI_DrawWifiQR(const char* ssid, const char* password, bool open,
                   int x, int y, int size_px) {
  // WiFi QR payload: WIFI:T:nopass;S:<ssid>;; or ...WPA;...P:<password>;;
  String payload = "WIFI:T:";
  payload += open ? "nopass" : "WPA";
  payload += ";S:"; payload += ssid; payload += ";";
  if (!open) { payload += "P:"; payload += password; payload += ";"; }
  payload += ";";

  uint8_t version = 3;
  if (payload.length() > 60) version = 5;
  if (payload.length() > 100) version = 7;

  QRCode qr;
  uint8_t buf[qrcode_getBufferSize(7)];
  if (qrcode_initText(&qr, buf, version, ECC_MEDIUM, payload.c_str()) != 0) return;

  int modules = qr.size;
  int scale = size_px / (modules + 2);
  if (scale < 1) return;
  int qrPix = (modules + 2) * scale;

  // White background including the quiet zone.
  gfx->fillRect(x, y, qrPix, qrPix, C_WHITE);
  int off = x + scale, offY = y + scale;
  for (int my = 0; my < modules; my++) {
    for (int mx = 0; mx < modules; mx++) {
      if (qrcode_getModule(&qr, mx, my)) {
        gfx->fillRect(off + mx * scale, offY + my * scale, scale, scale, C_BLACK);
      }
    }
  }
}

void UI_DrawRangeIndicator(const char* text, int activeIdx, int totalCount, bool showText) {
  if (totalCount <= 0) return;

  // 1. Text readout (e.g. "50 km") with dark backing
  if (showText && text && text[0]) {
    int tw = Font_TextWidth(text, 2);
    gfx->fillRoundRect(LCD_WIDTH / 2 - tw / 2 - 8, LY_RANGE - 3, tw + 16, 22, 4, C_BLACK);
    Font_DrawCentered(text, LCD_WIDTH / 2, LY_RANGE, C_YELLOW, 2);
  }

  // 2. Standardized zoom dots: radius 4 px, gap 20 px, dark backing
  const int dotGap = 20;
  const int dotR = 4;
  const int dotY = LY_RANGE_DOTS;
  const int totalW = (totalCount - 1) * dotGap;
  const int startX = LCD_WIDTH / 2 - totalW / 2;

  // Clean dark backing so radar clouds or plane tracks don't cross under dots
  gfx->fillRoundRect(startX - dotR - 4, dotY - dotR - 3, totalW + 2 * (dotR + 4), 2 * dotR + 6, 6, C_BLACK);

  for (int i = 0; i < totalCount; i++) {
    int x = startX + i * dotGap;
    if (i == activeIdx) {
      gfx->fillCircle(x, dotY, dotR, C_YELLOW);
    } else {
      gfx->drawCircle(x, dotY, dotR, C_GRAY);
    }
  }
}

void UI_DrawHomeMarker(int x, int y) {
  gfx->drawCircle(x, y, 8, C_CYAN);
  gfx->drawCircle(x, y, 4, C_YELLOW);
  gfx->fillCircle(x, y, 2, C_WHITE);
  gfx->drawFastHLine(x - 12, y, 24, C_DKGRAY);
  gfx->drawFastVLine(x, y - 12, 24, C_DKGRAY);
}

#include "ADSB.h"
#include "Route.h"
#include "AircraftType.h"
#include "Settings.h"
#include "Lang.h"

void UI_DrawAircraftDetail(const Aircraft& ac, const RouteInfo* rt, int routeState, bool signalLost) {
  const bool metric = Settings_MetricUnits();

  const int pw = 300, ph = 224;
  const int px = (LCD_WIDTH - pw) / 2;
  const int py = (LCD_HEIGHT - ph) / 2;

  // Modern glassmorphic dark card with rounded corners and cyan accent border
  gfx->fillRoundRect(px, py, pw, ph, 14, 0x0821);
  gfx->drawRoundRect(px, py, pw, ph, 14, ac.isMilitary ? C_RED : C_CYAN);

  // Close button in top right: small red/dark circular badge with 'X'
  const int cx = px + pw - 20, cy = py + 18;
  gfx->fillCircle(cx, cy, 11, 0x3000);
  gfx->drawCircle(cx, cy, 11, C_GRAY);
  gfx->drawLine(cx - 4, cy - 4, cx + 4, cy + 4, C_WHITE);
  gfx->drawLine(cx - 4, cy + 4, cx + 4, cy - 4, C_WHITE);

  int ty = py + 12;
  char line[64];

  // 1. Callsign (header in bright yellow, or red if military)
  const char* cs = ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?");
  UI_Text(cs, px + 18, ty, ac.isMilitary ? C_RED : C_YELLOW, 2);
  if (ac.isMilitary) {
    UI_Text("MIL", px + 18 + Layout_TextW(cs, 2) + 12, ty + 2, C_RED, 1);
  }
  ty += 25;

  // 2. Altitude: ft or m - in C_CYAN
  if (metric) snprintf(line, sizeof(line), "%s: %.0f m", T(S_ALTITUDE), ac.altFt * 0.3048f);
  else        snprintf(line, sizeof(line), "%s: %.0f ft", T(S_ALTITUDE), ac.altFt);
  UI_Text(line, px + 18, ty, C_CYAN, 2);
  ty += 24;

  // 3. Speed: kt or km/h - in C_WHITE
  if (metric) snprintf(line, sizeof(line), "%s: %.0f km/h", T(S_SPEED), ac.gsKt * 1.852f);
  else        snprintf(line, sizeof(line), "%s: %.0f kt", T(S_SPEED), ac.gsKt);
  UI_Text(line, px + 18, ty, C_WHITE, 2);
  ty += 24;

  // 4. Track (Heading) - in C_WHITE with degree symbol °
  if (ac.hasTrack) snprintf(line, sizeof(line), "%s: %.0f\xC2\xB0", T(S_TRACK), ac.track);
  else             snprintf(line, sizeof(line), "%s: %s", T(S_TRACK), T(S_UNKNOWN));
  UI_Text(line, px + 18, ty, C_WHITE, 2);
  ty += 24;

  // 5. Climb / Descent - dynamic color: C_GREEN (climb) / C_RED (descent) / C_GRAY (level)
  uint16_t clCol = C_GRAY;
  const char* ar = "-";
  if (ac.baroRate > 100.0f) {
    clCol = C_GREEN;
    ar = "^";
  } else if (ac.baroRate < -100.0f) {
    clCol = C_RED;
    ar = "v";
  }
  if (metric) snprintf(line, sizeof(line), "%s: %.1f m/s %s", T(S_CLIMB), ac.baroRate * 0.00508f, ar);
  else        snprintf(line, sizeof(line), "%s: %.0f ft/m %s", T(S_CLIMB), ac.baroRate, ar);
  UI_Text(line, px + 18, ty, clCol, 2);
  ty += 24;

  // 6. Type & Reg - in soft amber with translated human name!
  const char* fullType = AircraftType_Format(ac.type);
  if (fullType[0] || ac.reg[0]) {
    if (fullType[0] && ac.reg[0]) {
      snprintf(line, sizeof(line), "%s [%s]", fullType, ac.reg);
    } else if (fullType[0]) {
      snprintf(line, sizeof(line), "%s", fullType);
    } else {
      snprintf(line, sizeof(line), "%s: %s", T(S_TYPE), ac.reg);
    }
    int avail = pw - 36;
    int tw = Layout_TextW(line, 2);
    int size = 2;
    if (tw > avail) {
      size = 1;
      tw = Layout_TextW(line, 1);
      if (tw > avail) {
        int maxCh = avail / 6;
        if ((int)strlen(line) > maxCh) { line[maxCh - 2] = '.'; line[maxCh - 1] = '.'; line[maxCh] = '\0'; }
      }
    }
    UI_Text(line, px + 18, ty + (size == 1 ? 4 : 0), 0xFDE0 /* Soft amber */, size);
  }
  ty += 26;

  // 7. Route: From -> To (or wait message)
  const int avail = pw - 36;
  if (routeState == ROUTE_WAIT) {
    UI_Text(T(S_ROUTE_WAIT), px + 18, ty, C_GRAY, 1);
  } else if (rt && (rt->from[0] || rt->to[0])) {
    char l1[48], l2[48];
    snprintf(l1, sizeof(l1), "%s: %s", T(S_FROM), rt->from[0] ? rt->from : "?");
    snprintf(l2, sizeof(l2), "%s: %s", T(S_TO), rt->to[0] ? rt->to : "?");

    int s1 = (Layout_TextW(l1, 2) > avail) ? 1 : 2;
    int s2 = (Layout_TextW(l2, 2) > avail) ? 1 : 2;
    int finalSize = (s1 < s2) ? s1 : s2;

    int maxCh = avail / (6 * finalSize);
    if ((int)strlen(l1) > maxCh) { l1[maxCh - 2] = '.'; l1[maxCh - 1] = '.'; l1[maxCh] = '\0'; }
    if ((int)strlen(l2) > maxCh) { l2[maxCh - 2] = '.'; l2[maxCh - 1] = '.'; l2[maxCh] = '\0'; }

    UI_Text(l1, px + 18, ty, 0x56E0 /* Bright soft green for departure */, finalSize);
    ty += (finalSize == 2 ? 22 : 14);
    UI_Text(l2, px + 18, ty, 0xFDE0 /* Soft amber for destination */, finalSize);
  }

  // Signal lost note if missing from current frame
  if (signalLost) {
    const char* lost = T(S_SIGNAL_LOST);
    UI_Text(lost, px + pw - 18 - Layout_TextW(lost, 1), py + ph - 16, C_YELLOW, 1);
  }
}
