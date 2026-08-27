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
#include <math.h>
#include "qrcode.h"
#include "Display_ST7701.h"

void UI_TextCenteredIn(const char* text, int x, int w, int cy,
                       uint16_t color, uint8_t size) {
  int16_t x1, y1; uint16_t tw, th;
  gfx->setTextSize(size);
  gfx->getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
  gfx->setTextColor(color);
  gfx->setCursor(x + (w - (int)tw) / 2, cy);
  gfx->print(text);
}

void UI_TextCentered(const char* text, int cy, uint16_t color, uint8_t size) {
  UI_TextCenteredIn(text, 0, LCD_WIDTH, cy, color, size);
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

  // Measure against the circle at the LOWER edge of the text, which is the
  // narrower end. The longest string this can produce is "23:59   -12 degC"
  // (~192 px at size 2) and the chord here is around 265 px, so it fits - but
  // check anyway: a wider font or a lower line would silently overflow.
  int16_t x1, y1; uint16_t tw, th;
  gfx->setTextSize(2);
  gfx->getTextBounds(txt, 0, 0, &x1, &y1, &tw, &th);
  int room = 2 * UI_ChordHalfWidth(cy + 16) - 8;
  if ((int)tw > room) return;

  // Dark backing - on the weather screen this sits straight on top of the radar
  // image, where white on yellow rain would be unreadable.
  gfx->fillRect(LCD_WIDTH / 2 - (int)tw / 2 - 6, cy - 3, (int)tw + 12, 22, C_BLACK);
  UI_TextCentered(txt, cy, C_WHITE, 2);
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
