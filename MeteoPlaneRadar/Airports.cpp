// =============================================================================
//  MeteoPlaneRadar
//  Airports - draw airport symbols and IATA labels on the radar map.
//
//  Airport icons are a small stylised runway cross (4 px) with a cyan dot at
//  the centre. The IATA code is rendered in size 1 font to the right (or left
//  if it would spill outside the radar circle). Airports are rendered as
//  underlay - they do NOT call Layout_Claim, so aircraft callsigns always win.
//
// =============================================================================
#include "Airports.h"
#include "AirportsData.h"
#include "UI.h"
#include "Layout.h"
#include "Settings.h"
#include "Display_ST7701.h"

// Runway cross icon: two perpendicular lines through the dot.
static void drawAirportIcon(int sx, int sy) {
  // Runway cross in dark cyan
  gfx->drawFastHLine(sx - 5, sy, 11, C_CYAN);
  gfx->drawFastVLine(sx, sy - 5, 11, C_CYAN);
  // Centre dot brighter
  gfx->fillCircle(sx, sy, 2, C_CYAN);
}

void Airports_Draw(ProjectFn project, int cx, int cy, int radius,
                   float rangeKm,
                   float lat0, float lat1, float lon0, float lon1) {
  // Determine which tier is the least important to still show.
  uint8_t maxTier;
  if (rangeKm <= 100.0f)      maxTier = 3;
  else if (rangeKm <= 200.0f) maxTier = 2;
  else                        maxTier = 1;

  long r2 = (long)radius * radius;

  for (int i = 0; i < AIRPORT_COUNT; i++) {
    const AirportEntry& ap = AIRPORTS[i];
    if (ap.tier > maxTier) continue;

    // Geographic cull
    if (ap.lat < lat0 || ap.lat > lat1 || ap.lon < lon0 || ap.lon > lon1) continue;

    int sx, sy;
    project(ap.lat, ap.lon, &sx, &sy);

    // Skip if outside display circle
    long dx = sx - cx, dy = sy - cy;
    if (dx * dx + dy * dy > r2) continue;

    // Draw the airport icon
    drawAirportIcon(sx, sy);

    // IATA label to the right; flip left if it would exit the circle.
    int tw = strlen(ap.iata) * 6;   // size 1 char width = 6
    int tx = sx + 8;
    int ty = sy - 4;
    if ((long)(tx + tw - cx) * (tx + tw - cx) + dy * dy > r2) {
      tx = sx - 8 - tw;
    }

    if (Settings_ShowLegends()) {
      // Check collision: prevent airport labels from colliding with cities or each other
      if (Layout_Claim(tx - 2, ty - 2, tw + 4, 12)) {
        gfx->fillRoundRect(tx - 2, ty - 1, tw + 4, 11, 3, C_BLACK);
        UI_Text(ap.iata, tx, ty, C_CYAN, 1);
      } else {
        // Try opposite side if primary side collided
        int altX = (tx > sx) ? (sx - 8 - tw) : (sx + 8);
        if ((long)(altX + tw - cx) * (altX + tw - cx) + dy * dy <= r2) {
          if (Layout_Claim(altX - 2, ty - 2, tw + 4, 12)) {
            gfx->fillRoundRect(altX - 2, ty - 1, tw + 4, 11, 3, C_BLACK);
            UI_Text(ap.iata, altX, ty, C_CYAN, 1);
          }
        }
      }
    }
  }
}
