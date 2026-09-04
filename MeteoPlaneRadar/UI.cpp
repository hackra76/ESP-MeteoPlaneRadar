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
#include "PlanePhoto.h"
#include "AircraftType.h"
#include "Settings.h"
#include "Lang.h"
static float calcGeoDistKm(double lat1, double lon1, double lat2, double lon2) {
  const float R = 6371.0f, D = 0.017453293f;
  float dLat = (float)(lat2 - lat1) * D;
  float dLon = (float)(lon2 - lon1) * D;
  float a = sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
            cosf((float)lat1 * D) * cosf((float)lat2 * D) * sinf(dLon * 0.5f) * sinf(dLon * 0.5f);
  if (a < 0) a = 0; else if (a > 1) a = 1;
  return 2.0f * R * asinf(sqrtf(a));
}

static bool s_photoFullscreen = false;

bool UI_IsPhotoFullscreen() {
  return s_photoFullscreen;
}

void UI_SetPhotoFullscreen(bool en) {
  s_photoFullscreen = en;
}

static void drawScaledPhoto(const uint16_t* pixels, int iw, int ih, int targetX, int targetY, int tw, int th) {
  if (!pixels || iw <= 0 || ih <= 0 || tw <= 0 || th <= 0) return;
  for (int y = 0; y < th; y++) {
    int srcY = (y * ih) / th;
    if (srcY >= ih) srcY = ih - 1;
    const uint16_t* srcRow = pixels + (int32_t)srcY * iw;
    int dstY = targetY + y;
    if (dstY < 0 || dstY >= LCD_HEIGHT) continue;

    for (int x = 0; x < tw; x++) {
      int srcX = (x * iw) / tw;
      if (srcX >= iw) srcX = iw - 1;
      int dstX = targetX + x;
      if (dstX >= 0 && dstX < LCD_WIDTH) {
        gfx->drawPixel(dstX, dstY, srcRow[srcX]);
      }
    }
  }
}

static void UI_DrawAircraftPhotoFullscreen(const Aircraft& ac) {
  gfx->fillScreen(C_BLACK);

  int iw = 0, ih = 0;
  const uint16_t* pixels = PlanePhoto_GetRgb565(&iw, &ih);
  if (!pixels || iw <= 0 || ih <= 0) {
    s_photoFullscreen = false;
    return;
  }

  // Fit photo within circular display (diameter 480, safe radius R=232)
  float aspect = (float)iw / (float)ih;
  float maxTh = sqrtf(4.0f * 53824.0f / (aspect * aspect + 1.0f));
  int th = (int)maxTh;
  int tw = (int)(th * aspect);
  if (tw > 440) { tw = 440; th = (int)(tw / aspect); }
  if (th > 350) { th = 350; tw = (int)(th * aspect); }

  int targetX = (LCD_WIDTH - tw) / 2;
  int targetY = (LCD_HEIGHT - th) / 2;

  // Frame around photo
  gfx->drawRoundRect(targetX - 2, targetY - 2, tw + 4, th + 4, 6, ac.isMilitary ? C_RED : C_CYAN);
  drawScaledPhoto(pixels, iw, ih, targetX, targetY, tw, th);

  // Top header: Callsign, Type, Reg
  const char* cs = ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?");
  char topTitle[64];
  const char* fullType = AircraftType_Format(ac.type);
  if (ac.reg[0] && fullType[0]) {
    snprintf(topTitle, sizeof(topTitle), "%s  ·  %s [%s]", cs, fullType, ac.reg);
  } else if (ac.reg[0]) {
    snprintf(topTitle, sizeof(topTitle), "%s  ·  %s", cs, ac.reg);
  } else {
    snprintf(topTitle, sizeof(topTitle), "%s  ·  %s", cs, ac.type[0] ? ac.type : "");
  }
  UI_TextCentered(topTitle, 45, ac.isMilitary ? C_RED : C_YELLOW, 2);

  // Bottom info: Photographer credit
  const char* photog = PlanePhoto_GetPhotographer();
  char credit[64];
  snprintf(credit, sizeof(credit), "Foto: \xC2\xA9 %s (Planespotters.net)", (photog && photog[0]) ? photog : "Planespotters.net");
  UI_TextCentered(credit, targetY + th + 10, C_WHITE, 1);

  // Bottom hint: tap anywhere to return
  UI_TextCentered(T(S_TAP_TO_RETURN), LCD_HEIGHT - 38, C_GRAY, 1);
}

void UI_DrawAircraftDetail(const Aircraft& ac, const RouteInfo* rt, int routeState, bool signalLost) {
  if (s_photoFullscreen) {
    UI_DrawAircraftPhotoFullscreen(ac);
    return;
  }

  const bool metric = Settings_MetricUnits();

  // --- 1. Aircraft Photo Box (Top section, 200x133) ---
  const int photoW = 200, photoH = 133;
  const int photoX = (LCD_WIDTH - photoW) / 2;
  const int photoY = 50;

  gfx->fillRoundRect(photoX - 2, photoY - 2, photoW + 4, photoH + 4, 8, 0x0821);
  gfx->drawRoundRect(photoX - 2, photoY - 2, photoW + 4, photoH + 4, 8, ac.isMilitary ? C_RED : C_CYAN);

  PhotoState pState = PlanePhoto_GetState();
  if (pState == PHOTO_OK) {
    int iw = 0, ih = 0;
    const uint16_t* pixels = PlanePhoto_GetRgb565(&iw, &ih);
    if (pixels && iw > 0 && ih > 0) {
      float aspect = (float)iw / (float)ih;
      int tw = photoW;
      int th = (int)(tw / aspect);
      if (th > photoH) {
        th = photoH;
        tw = (int)(th * aspect);
      }
      int dx = photoX + (photoW - tw) / 2;
      int dy = photoY + (photoH - th) / 2;
      drawScaledPhoto(pixels, iw, ih, dx, dy, tw, th);

      // Tap hint badge in bottom-right corner of photo box
      gfx->fillRoundRect(photoX + photoW - 24, photoY + photoH - 17, 22, 14, 3, 0x18C3);
      UI_Text("+", photoX + photoW - 16, photoY + photoH - 15, C_WHITE, 1);
    }
    const char* photog = PlanePhoto_GetPhotographer();
    char credit[48];
    snprintf(credit, sizeof(credit), "Foto: %s", (photog && photog[0]) ? photog : "Planespotters.net");
    UI_TextCentered(credit, photoY + photoH + 4, C_GRAY, 1);
  } else if (pState == PHOTO_WAIT) {
    UI_TextCentered(T(S_PHOTO_WAIT), photoY + photoH / 2 - 4, C_GRAY, 1);
  } else if (pState == PHOTO_NONE) {
    UI_TextCentered(T(S_NO_PHOTO), photoY + photoH / 2 - 4, 0x52AA, 1);
  }

  // --- 2. Telemetry Card (Middle section, 310x126) ---
  const int cw = 310, ch = 126;
  const int cx = (LCD_WIDTH - cw) / 2;
  const int cy = 202;

  gfx->fillRoundRect(cx, cy, cw, ch, 12, 0x0821);
  gfx->drawRoundRect(cx, cy, cw, ch, 12, ac.isMilitary ? C_RED : 0x2FE6);

  // Close button in top right
  const int bx = cx + cw - 16, by = cy + 14;
  gfx->fillCircle(bx, by, 9, 0x3000);
  gfx->drawCircle(bx, by, 9, C_GRAY);
  gfx->drawLine(bx - 3, by - 3, bx + 3, by + 3, C_WHITE);
  gfx->drawLine(bx - 3, by + 3, bx + 3, by - 3, C_WHITE);

  int ty = cy + 10;

  // Title: Callsign & Type
  const char* cs = ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?");
  UI_Text(cs, cx + 14, ty, ac.isMilitary ? C_RED : C_YELLOW, 2);
  int csw = Layout_TextW(cs, 2);
  if (ac.isMilitary) {
    UI_Text("MIL", cx + 14 + csw + 8, ty + 2, C_RED, 1);
    csw += 28;
  }
  if (ac.type[0]) {
    UI_Text(ac.type, cx + 14 + csw + 10, ty, C_WHITE, 2);
  }
  ty += 23;

  // 2-Column Telemetry Rows
  const int col1X = cx + 14;
  const int col2X = cx + 160;

  // Row 1: ALT & V/S
  char altStr[32];
  if (metric) snprintf(altStr, sizeof(altStr), "ALT %.0f m", ac.altFt * 0.3048f);
  else        snprintf(altStr, sizeof(altStr), "ALT %.0f ft", ac.altFt);
  UI_Text(altStr, col1X, ty, C_WHITE, 1);

  char vsStr[32];
  uint16_t vsCol = C_GRAY;
  if (ac.baroRate > 100.0f)       vsCol = C_GREEN;
  else if (ac.baroRate < -100.0f) vsCol = C_RED;
  if (metric) snprintf(vsStr, sizeof(vsStr), "V/S %+.1f m/s", ac.baroRate * 0.00508f);
  else        snprintf(vsStr, sizeof(vsStr), "V/S %+.0f", ac.baroRate);
  UI_Text(vsStr, col2X, ty, vsCol, 1);
  ty += 18;

  // Row 2: SPD & HDG
  char spdStr[32];
  if (metric) snprintf(spdStr, sizeof(spdStr), "SPD %.0f km/h", ac.gsKt * 1.852f);
  else        snprintf(spdStr, sizeof(spdStr), "SPD %.0f kt", ac.gsKt);
  UI_Text(spdStr, col1X, ty, C_WHITE, 1);

  char hdgStr[32];
  if (ac.hasTrack) snprintf(hdgStr, sizeof(hdgStr), "HDG %03.0f\xC2\xB0", ac.track);
  else             snprintf(hdgStr, sizeof(hdgStr), "HDG ---");
  UI_Text(hdgStr, col2X, ty, C_WHITE, 1);
  ty += 18;

  // Row 3: DIST & SQK
  float distKm = calcGeoDistKm(Settings_Lat(), Settings_Lon(), ac.lat, ac.lon);
  char distStr[32];
  if (metric) snprintf(distStr, sizeof(distStr), "DIST %.1f km", distKm);
  else        snprintf(distStr, sizeof(distStr), "DIST %.1f NM", distKm * 0.539957f);
  UI_Text(distStr, col1X, ty, C_WHITE, 1);

  char sqkStr[32];
  if (ac.squawk[0]) snprintf(sqkStr, sizeof(sqkStr), "SQK %s", ac.squawk);
  else              snprintf(sqkStr, sizeof(sqkStr), "SQK ----");
  UI_Text(sqkStr, col2X, ty, C_WHITE, 1);
  ty += 20;

  // Row 4: Route (or Type / Reg)
  const int availW = cw - 28;
  if (routeState == ROUTE_WAIT) {
    UI_Text(T(S_ROUTE_WAIT), col1X, ty, C_GRAY, 1);
  } else if (rt && (rt->from[0] || rt->to[0])) {
    char rtLine[64];
    snprintf(rtLine, sizeof(rtLine), "%s -> %s", rt->from[0] ? rt->from : "?", rt->to[0] ? rt->to : "?");
    int rw = Layout_TextW(rtLine, 2);
    uint8_t fSize = (rw <= availW) ? 2 : 1;
    if (fSize == 1 && Layout_TextW(rtLine, 1) > availW) {
      int maxCh = availW / 6;
      if ((int)strlen(rtLine) > maxCh) { rtLine[maxCh - 2] = '.'; rtLine[maxCh - 1] = '.'; rtLine[maxCh] = '\0'; }
    }
    UI_Text(rtLine, col1X, ty, 0x56E0 /* Bright soft green */, fSize);
  } else {
    char typeLine[64] = "";
    const char* fullType = AircraftType_Format(ac.type);
    if (fullType[0] && ac.reg[0]) snprintf(typeLine, sizeof(typeLine), "%s [%s]", fullType, ac.reg);
    else if (ac.reg[0])           snprintf(typeLine, sizeof(typeLine), "Reg: %s", ac.reg);
    else if (fullType[0])         snprintf(typeLine, sizeof(typeLine), "%s", fullType);
    if (typeLine[0]) {
      if (Layout_TextW(typeLine, 1) > availW) {
        int maxCh = availW / 6;
        if ((int)strlen(typeLine) > maxCh) { typeLine[maxCh - 2] = '.'; typeLine[maxCh - 1] = '.'; typeLine[maxCh] = '\0'; }
      }
      UI_Text(typeLine, col1X, ty, 0xFDE0 /* Soft amber */, 1);
    }
  }

  // Signal lost indicator
  if (signalLost) {
    const char* lost = T(S_SIGNAL_LOST);
    UI_Text(lost, cx + cw - 14 - Layout_TextW(lost, 1), cy + ch - 12, C_YELLOW, 1);
  }
}
