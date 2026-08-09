// MeteoPlaneRadar - vyvoj / development: chiptron.cz
// =============================================================================
//  MeteoPlaneRadar - podkres map: obrys CR a mesta (GPS polygon + body).
//  Obrys i mesta jsou GPS souradnice, promitaji se stejnou projekci jako data
//  (radar/letadla) pres callback ProjectFn - vzdy tedy sedi s podkladem.
//  Vychozi z petus/Meteo-PlaneRadar, upraveno pro obdelnikovy displej ESPD-3.5.
// =============================================================================
#include "CzBorder.h"
#include "UI.h"
#include "Display_ST7701.h"   // LCD_WIDTH / LCD_HEIGHT
#include <string.h>

#include "CzBorderData.h"   // CZ_BORDER[] + CZ_BORDER_N

void CzBorder_Draw(ProjectFn project, uint16_t color) {
  // The outline is ~600 segments now. At the closest ranges almost all of them
  // land far outside the panel, and handing drawLine() coordinates in the tens
  // of thousands costs real time for nothing - so skip a segment when both of
  // its ends are outside the same edge of the screen. Segments that merely
  // cross the display still get drawn.
  const int M = 64;                       // margin, so line ends stay honest
  int px = 0, py = 0;
  bool have = false;
  for (int i = 0; i < CZ_BORDER_N; i++) {
    int sx, sy;
    project(CZ_BORDER[i][1], CZ_BORDER[i][0], &sx, &sy);   // lat, lon
    if (have) {
      bool skip = (px < -M && sx < -M) || (px > LCD_WIDTH  + M && sx > LCD_WIDTH  + M) ||
                  (py < -M && sy < -M) || (py > LCD_HEIGHT + M && sy > LCD_HEIGHT + M);
      if (!skip) gfx->drawLine(px, py, sx, sy, color);
    }
    px = sx; py = sy; have = true;
  }
}

// Mesta CR. tier 1 = velka/krajska (vzdy videt), tier 2 = stredni.
static const struct { const char* name; const char* abbr; float lon, lat; uint8_t tier; } CZ_CITIES[] = {
  {"Praha", "PHA", 14.4378f, 50.0755f, 1},
  {"Brno", "BRNO", 16.6068f, 49.1951f, 1},
  {"Ostrava", "OVA", 18.2625f, 49.8209f, 1},
  {"Plzen", "PLZ", 13.3776f, 49.7384f, 1},
  {"Liberec", "LBC", 15.0543f, 50.7663f, 1},
  {"Olomouc", "OLO", 17.2509f, 49.5938f, 1},
  {"Ceske Budejovice", "CB", 14.4747f, 48.9747f, 1},
  {"Hradec Kralove", "HK", 15.8327f, 50.2092f, 1},
  {"Usti nad Labem", "UNL", 14.0417f, 50.6607f, 1},
  {"Pardubice", "PCE", 15.7812f, 50.0343f, 1},
  {"Zlin", "ZLN", 17.6707f, 49.2265f, 1},
  {"Jihlava", "JIH", 15.5906f, 49.3961f, 1},
  {"Karlovy Vary", "KV", 12.8712f, 50.2306f, 1},
  {"Kladno", "KLD", 14.1017f, 50.1477f, 2},
  {"Most", "MOST", 13.6363f, 50.5031f, 2},
  {"Opava", "OPA", 17.9019f, 49.9387f, 2},
  {"Frydek-Mistek", "FM", 18.3505f, 49.6833f, 2},
  {"Karvina", "KAR", 18.5419f, 49.8540f, 2},
  {"Havirov", "HAV", 18.4364f, 49.7798f, 2},
  {"Teplice", "TEP", 13.8245f, 50.6404f, 2},
  {"Decin", "DEC", 14.2145f, 50.7821f, 2},
  {"Chomutov", "CHO", 13.4178f, 50.4605f, 2},
  {"Jablonec", "JBC", 15.1712f, 50.7243f, 2},
  {"Mlada Boleslav", "MB", 14.9038f, 50.4114f, 2},
  {"Prostejov", "PROS", 17.1118f, 49.4720f, 2},
  {"Prerov", "PRER", 17.4509f, 49.4554f, 2},
  {"Trebic", "TRB", 15.8814f, 49.2149f, 2},
  {"Ceska Lipa", "CL", 14.5376f, 50.6855f, 2},
  {"Trinec", "TRI", 18.6708f, 49.6776f, 2},
  {"Tabor", "TAB", 14.6578f, 49.4144f, 2},
  {"Znojmo", "ZNO", 16.0488f, 48.8555f, 2},
  {"Pribram", "PRI", 14.0104f, 49.6899f, 2},
  {"Cheb", "CHEB", 12.3740f, 50.0796f, 2},
  {"Trutnov", "TRU", 15.9124f, 50.5606f, 2},
  {"Kolin", "KOL", 15.2003f, 50.0281f, 2},
  {"Pisek", "PIS", 14.1476f, 49.3088f, 2},
  {"Kromeriz", "KRO", 17.3928f, 49.2979f, 2},
  {"Sumperk", "SUM", 16.9708f, 49.9653f, 2},
  {"Vsetin", "VSE", 17.9963f, 49.3387f, 2},
  {"Litomerice", "LTM", 14.1319f, 50.5344f, 2},
  {"Havlickuv Brod", "HB", 15.5800f, 49.6077f, 2},
  {"Strakonice", "STR", 13.9022f, 49.2619f, 2},
  {"Klatovy", "KLA", 13.2937f, 49.3955f, 2},
  {"Nachod", "NAC", 16.1655f, 50.4145f, 2},
  {"Zdar n. Sazavou", "ZDS", 15.9394f, 49.5628f, 2},
  {"Uherske Hradiste", "UH", 17.4597f, 49.0698f, 2},
  {"Hodonin", "HOD", 17.1300f, 48.8489f, 2},
  {"Breclav", "BRE", 16.8820f, 48.7589f, 2},
  {"Jindrichuv Hradec", "JH", 15.0030f, 49.1442f, 2},
  {"Sokolov", "SOK", 12.6400f, 50.1814f, 2},
  {"Bruntal", "BRU", 17.4647f, 49.9884f, 2},
  {"Krnov", "KRN", 17.7047f, 50.0899f, 2},
  {"Vyskov", "VYS", 16.9989f, 49.2775f, 2},
  {"Blansko", "BLA", 16.6440f, 49.3630f, 2},
  {"Beroun", "BER", 14.0720f, 49.9639f, 2},
  {"Melnik", "MEL", 14.4740f, 50.3505f, 2},
  {"Nymburk", "NYM", 15.0414f, 50.1861f, 2},
  {"Ceska Trebova", "CT", 16.4478f, 49.9038f, 2},
  {"Svitavy", "SVI", 16.4682f, 49.7554f, 2},
};
static const int CZ_CITIES_N = 59;

void CzBorder_DrawCities(ProjectFn project, int cx, int cy, int radius,
                         uint16_t dotColor, uint16_t textColor,
                         bool showFull, bool showSmall) {
  long r2 = (long)radius * radius;
  for (int i = 0; i < CZ_CITIES_N; i++) {
    // Pri velkem rozsahu kreslime jen velka mesta (tier 1).
    if (!showSmall && CZ_CITIES[i].tier > 1) continue;

    int sx, sy;
    project(CZ_CITIES[i].lat, CZ_CITIES[i].lon, &sx, &sy);
    // POJISTKA: mimo obrazovku nekreslime. gfx->fillCircle na canvasu neoreze
    // souradnice, takze bod mimo displej by zapsal ZA framebuffer -> pad
    // (StoreProhibited). Nutne zejmena u meteo s vetsim polomerem vyrezu.
    if (sx < 4 || sx >= LCD_WIDTH - 4 || sy < 4 || sy >= LCD_HEIGHT - 4) continue;
    // Mimo zvoleny kruh take nekreslime.
    long dx = sx - cx, dy = sy - cy;
    if (dx * dx + dy * dy > r2) continue;

    // Nazev/zkratka je VZDY vpravo od tecky. Kdyz se cely popis (tecka+nazev)
    // nevejde na obrazovku, mesto se vynecha - zadne presouvani doleva.
    const char* label = showFull ? CZ_CITIES[i].name : CZ_CITIES[i].abbr;
    int tw = strlen(label) * 6;
    int tx = sx + 9;
    int ty = sy - 4;
    if (tx + tw > LCD_WIDTH - 2 || ty < 1 || ty + 8 > LCD_HEIGHT - 1) continue;

    gfx->fillCircle(sx, sy, 3, dotColor);     // tecka mesta
    gfx->setTextSize(1);
    gfx->setTextColor(textColor);
    gfx->setCursor(tx, ty);
    gfx->print(label);
  }
}
