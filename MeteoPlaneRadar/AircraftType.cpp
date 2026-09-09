// =============================================================================
//  MeteoPlaneRadar
//  AircraftType.cpp - Aircraft type lookup table.
// =============================================================================
#include "AircraftType.h"
#include "Lang.h"
#include <string.h>
#include <strings.h>

struct TypeMapping {
  const char* icao;
  const char* name;
};

static const TypeMapping TYPE_TABLE[] = {
  // Airbus Airliners
  { "A318", "Airbus A318" },
  { "A319", "Airbus A319" },
  { "A320", "Airbus A320" },
  { "A20N", "Airbus A320neo" },
  { "A321", "Airbus A321" },
  { "A21N", "Airbus A321neo" },
  { "A332", "Airbus A330-200" },
  { "A333", "Airbus A330-300" },
  { "A338", "Airbus A330-800" },
  { "A339", "Airbus A330-900" },
  { "A342", "Airbus A340-200" },
  { "A343", "Airbus A340-300" },
  { "A345", "Airbus A340-500" },
  { "A346", "Airbus A340-600" },
  { "A359", "Airbus A350-900" },
  { "A35K", "Airbus A350-1000" },
  { "A388", "Airbus A380" },
  { "BCS1", "Airbus A220-100" },
  { "BCS3", "Airbus A220-300" },
  { "A220", "Airbus A220" },

  // Boeing Airliners
  { "B712", "Boeing 717" },
  { "B733", "Boeing 737-300" },
  { "B734", "Boeing 737-400" },
  { "B735", "Boeing 737-500" },
  { "B736", "Boeing 737-600" },
  { "B737", "Boeing 737-700" },
  { "B738", "Boeing 737-800" },
  { "B739", "Boeing 737-900" },
  { "B37M", "Boeing 737 MAX 7" },
  { "B38M", "Boeing 737 MAX 8" },
  { "B39M", "Boeing 737 MAX 9" },
  { "B3JM", "Boeing 737 MAX 10" },
  { "B744", "Boeing 747-400" },
  { "B748", "Boeing 747-8" },
  { "B752", "Boeing 757-200" },
  { "B753", "Boeing 757-300" },
  { "B762", "Boeing 767-200" },
  { "B763", "Boeing 767-300" },
  { "B764", "Boeing 767-400" },
  { "B772", "Boeing 777-200" },
  { "B77L", "Boeing 777-200LR" },
  { "B773", "Boeing 777-300" },
  { "B77W", "Boeing 777-300ER" },
  { "B788", "Boeing 787-8" },
  { "B789", "Boeing 787-9" },
  { "B78X", "Boeing 787-10" },

  // Embraer
  { "E170", "Embraer E170" },
  { "E175", "Embraer E175" },
  { "E75L", "Embraer E175" },
  { "E75S", "Embraer E175" },
  { "E190", "Embraer E190" },
  { "E195", "Embraer E195" },
  { "E290", "Embraer E190-E2" },
  { "E295", "Embraer E195-E2" },
  { "E135", "Embraer ERJ-135" },
  { "E145", "Embraer ERJ-145" },

  // Bombardier / Mitsubishi
  { "CRJ1", "Bombardier CRJ-100" },
  { "CRJ2", "Bombardier CRJ-200" },
  { "CRJ7", "Bombardier CRJ-700" },
  { "CRJ9", "Bombardier CRJ-900" },
  { "CRJX", "Bombardier CRJ-1000" },
  { "DH8A", "Dash 8-100" },
  { "DH8B", "Dash 8-200" },
  { "DH8C", "Dash 8-300" },
  { "DH8D", "Dash 8 Q400" },

  // ATR
  { "AT43", "ATR 42-300" },
  { "AT45", "ATR 42-500" },
  { "AT46", "ATR 42-600" },
  { "AT72", "ATR 72-200" },
  { "AT75", "ATR 72-500" },
  { "AT76", "ATR 72-600" },

  // General Aviation / Light
  { "C150", "Cessna 150" },
  { "C152", "Cessna 152" },
  { "C172", "Cessna 172" },
  { "C182", "Cessna 182" },
  { "C206", "Cessna 206" },
  { "C208", "Cessna Caravan" },
  { "C510", "Citation Mustang" },
  { "C525", "Cessna CitationJet" },
  { "C560", "Citation Excel" },
  { "C680", "Citation Sovereign" },
  { "PC12", "Pilatus PC-12" },
  { "PC24", "Pilatus PC-24" },
  { "PA28", "Piper PA-28" },
  { "PA34", "Piper Seneca" },
  { "PA44", "Piper Seminole" },
  { "SR20", "Cirrus SR20" },
  { "SR22", "Cirrus SR22" },
  { "SF50", "Cirrus Vision Jet" },
  { "BE20", "King Air 200" },
  { "BE30", "King Air 300" },
  { "B350", "King Air 350" },
  { "DA40", "Diamond DA40" },
  { "DA42", "Diamond DA42" },
  { "DA62", "Diamond DA62" },

  // Business Jets
  { "GLF4", "Gulfstream IV" },
  { "GLF5", "Gulfstream V" },
  { "GLF6", "Gulfstream G650" },
  { "GLEX", "Bombardier Global" },
  { "CL30", "Challenger 300" },
  { "CL35", "Challenger 350" },
  { "CL60", "Challenger 600" },
  { "FA50", "Falcon 50" },
  { "FA7X", "Falcon 7X" },
  { "FA8X", "Falcon 8X" },
  { "H25B", "Hawker 800" },
  { "LJ35", "Learjet 35" },
  { "LJ60", "Learjet 60" },

  // Helicopters
  { "EC35", "Eurocopter EC135" },
  { "EC45", "Eurocopter EC145" },
  { "H135", "Airbus H135" },
  { "H145", "Airbus H145" },
  { "AS50", "AS350 Ecureuil" },
  { "A109", "Agusta AW109" },
  { "A139", "Leonardo AW139" },
  { "B06",  "Bell 206 JetRanger" },
  { "B407", "Bell 407" },
  { "B429", "Bell 429" },
  { "R44",  "Robinson R44" },
  { "R66",  "Robinson R66" },
  { "MI8",  "Mil Mi-8" },
  { "MI17", "Mil Mi-17" },
  { "UH60", "Sikorsky UH-60" },

  // Regional / Turboprops / Czech & Slovak
  { "L410", "Let L-410 Turbolet" },
  { "C295", "CASA C-295M" },
  { "C27J", "Alenia C-27J Spartan" },
  { "AT45", "ATR 42-500" },
  { "AT75", "ATR 72-500" },
  { "AT76", "ATR 72-600" },
  { "EV97", "Evektor Eurostar" },
  { "WT9",  "Aerospool Dynamic WT9" },
  { "VL3",  "JMB VL-3 Evolution" },
  { "Z42",  "Zlin Z-42" },
  { "Z43",  "Zlin Z-43" },
  { "Z50",  "Zlin Z-50" },

  // Business & Heavy Transports
  { "PC24", "Pilatus PC-24" },
  { "TBM9", "Daher TBM 900" },
  { "P180", "Piaggio P.180 Avanti" },
  { "E55P", "Embraer Phenom 300" },
  { "E50P", "Embraer Phenom 100" },
  { "C700", "Cessna Citation Longitude" },
  { "E3TF", "Boeing E-3A Sentry (AWACS)" },
  { "K35R", "Boeing KC-135 Stratotanker" },
  { "IL76", "Ilyushin Il-76" },
  { "AN12", "Antonov An-12" },
  { "AN26", "Antonov An-26" },

  // Military / Trainers / Gliders
  { "L39",  "Aero L-39 Albatros" },
  { "L159", "Aero L-159 ALCA" },
  { "JAS3", "JAS 39 Gripen" },
  { "EUFI", "Eurofighter Typhoon" },
  { "F16",  "Lockheed F-16" },
  { "F35",  "Lockheed F-35" },
  { "C130", "Lockheed C-130 Hercules" },
  { "A400", "Airbus A400M Atlas" },
  { "C17",  "Boeing C-17 Globemaster" },
  { "GLID", "Vetron (Sailplane)" }
};

static const size_t TYPE_TABLE_COUNT = sizeof(TYPE_TABLE) / sizeof(TYPE_TABLE[0]);

const char* AircraftType_Format(const char* icaoType) {
  if (!icaoType || !icaoType[0]) return "";
  for (size_t i = 0; i < TYPE_TABLE_COUNT; i++) {
    if (strcasecmp(TYPE_TABLE[i].icao, icaoType) == 0) {
      return TYPE_TABLE[i].name;
    }
  }
  return icaoType;
}

#include "ADSB.h"
#include "Settings.h"
#include "UI.h"
#include <stdio.h>
#include <math.h>

#define COL_RESCUE      0x07E0   // Vibrant green
#define COL_GOVERNMENT  0xFDE0   // Gold / Amber
#define COL_HEAVY_ICON  0x07FF   // Cyan
#define COL_MILITARY    0xF800   // Crimson Red
#define COL_WATCHED     0xF81F   // Magenta

static bool startsWithCase(const char* str, const char* prefix) {
  if (!str || !prefix) return false;
  return strncasecmp(str, prefix, strlen(prefix)) == 0;
}

SpecialCategory Aircraft_Classify(const Aircraft& ac, char* labelOut, size_t labelCap, uint16_t* colorOut) {
  if (labelOut && labelCap > 0) labelOut[0] = '\0';

  // 1. User custom watchlist
  const char* w = Settings_WatchCallsign();
  if (w && w[0] && startsWithCase(ac.callsign, w)) {
    if (colorOut) *colorOut = COL_WATCHED;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Sledovany let");
    return SPEC_WATCHED;
  }

  // 2. Military (either dbFlags & 1, or military callsigns/types)
  if (ac.isMilitary || startsWithCase(ac.callsign, "NATO") || startsWithCase(ac.callsign, "JAS") ||
      startsWithCase(ac.callsign, "ALCA") || startsWithCase(ac.callsign, "TIGER") ||
      startsWithCase(ac.callsign, "VIPER") || startsWithCase(ac.callsign, "GHOST") ||
      strcasecmp(ac.type, "L39") == 0 || strcasecmp(ac.type, "L159") == 0 ||
      strcasecmp(ac.type, "JAS3") == 0 || strcasecmp(ac.type, "EUFI") == 0 ||
      strcasecmp(ac.type, "F16") == 0 || strcasecmp(ac.type, "F35") == 0 ||
      strcasecmp(ac.type, "C130") == 0 || strcasecmp(ac.type, "A400") == 0 ||
      strcasecmp(ac.type, "C17") == 0) {
    if (colorOut) *colorOut = COL_MILITARY;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Vojensky let");
    return SPEC_MILITARY;
  }

  // 3. Rescue / HEMS Helicopters
  // ATE (Air-Transport Europe / Kryštof in CZ / HZS / SAR / LZZ / Medic)
  if (startsWithCase(ac.callsign, "ATE") || startsWithCase(ac.callsign, "KRY") ||
      startsWithCase(ac.callsign, "HZS") || startsWithCase(ac.callsign, "SAR") ||
      startsWithCase(ac.callsign, "HEMS") || startsWithCase(ac.callsign, "LZZ") ||
      startsWithCase(ac.callsign, "MEDIC") || startsWithCase(ac.callsign, "RESCUE") ||
      startsWithCase(ac.reg, "OM-AT") || startsWithCase(ac.reg, "OK-AT") ||
      startsWithCase(ac.callsign, "POLICE") || startsWithCase(ac.callsign, "POLICIE") ||
      strcasecmp(ac.type, "EC35") == 0 || strcasecmp(ac.type, "EC45") == 0 ||
      strcasecmp(ac.type, "H135") == 0 || strcasecmp(ac.type, "H145") == 0 ||
      strcasecmp(ac.type, "A109") == 0 || strcasecmp(ac.type, "UH60") == 0) {
    if (colorOut) *colorOut = COL_RESCUE;
    if (labelOut && labelCap > 0) {
      if (startsWithCase(ac.callsign, "ATE") || startsWithCase(ac.callsign, "KRY")) {
        snprintf(labelOut, labelCap, "Zachranny vrtulnik");
      } else {
        snprintf(labelOut, labelCap, "Zachranny / Policia");
      }
    }
    return SPEC_RESCUE;
  }

  // 4. Government / VIP Specials
  // SSG (Slovak Government Flight Service), CEF (Czech Air Force VIP), GAF (German Gov), IAM (Italian Gov), COTAM (French Gov)
  if (startsWithCase(ac.callsign, "SSG") || startsWithCase(ac.callsign, "CEF") ||
      startsWithCase(ac.callsign, "IAM") || startsWithCase(ac.callsign, "GAF") ||
      startsWithCase(ac.callsign, "COTAM") || startsWithCase(ac.callsign, "PLF") ||
      startsWithCase(ac.callsign, "SVK0") || startsWithCase(ac.callsign, "CZE0")) {
    if (colorOut) *colorOut = COL_GOVERNMENT;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Vladny special");
    return SPEC_GOVERNMENT;
  }

  // 5. Heavy & Iconic Giants
  if (strcasecmp(ac.type, "A388") == 0) {
    if (colorOut) *colorOut = COL_HEAVY_ICON;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Airbus A380");
    return SPEC_HEAVY_ICON;
  }
  if (strcasecmp(ac.type, "B744") == 0 || strcasecmp(ac.type, "B748") == 0) {
    if (colorOut) *colorOut = COL_HEAVY_ICON;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Boeing 747");
    return SPEC_HEAVY_ICON;
  }
  if (strcasecmp(ac.type, "A124") == 0 || strcasecmp(ac.type, "A225") == 0) {
    if (colorOut) *colorOut = COL_HEAVY_ICON;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Antonov");
    return SPEC_HEAVY_ICON;
  }
  if (strcasecmp(ac.type, "A3ST") == 0 || strcasecmp(ac.type, "A337") == 0) {
    if (colorOut) *colorOut = COL_HEAVY_ICON;
    if (labelOut && labelCap > 0) snprintf(labelOut, labelCap, "Airbus Beluga");
    return SPEC_HEAVY_ICON;
  }

  return SPEC_NONE;
}

AircraftIconType Aircraft_GetIconType(const Aircraft& ac) {
  // 1. Military fighter jet
  if (ac.isMilitary || startsWithCase(ac.callsign, "NATO") || startsWithCase(ac.callsign, "JAS") ||
      startsWithCase(ac.callsign, "ALCA") || startsWithCase(ac.callsign, "TIGER") ||
      startsWithCase(ac.callsign, "VIPER") || startsWithCase(ac.callsign, "GHOST") ||
      strcasecmp(ac.type, "L39") == 0 || strcasecmp(ac.type, "L159") == 0 ||
      strcasecmp(ac.type, "JAS3") == 0 || strcasecmp(ac.type, "EUFI") == 0 ||
      strcasecmp(ac.type, "F16") == 0 || strcasecmp(ac.type, "F35") == 0 ||
      strcasecmp(ac.type, "F18") == 0 || strcasecmp(ac.type, "MIG29") == 0) {
    return ICON_MILITARY_JET;
  }

  // 2. Helicopters / Rotorcraft
  if (startsWithCase(ac.callsign, "ATE") || startsWithCase(ac.callsign, "KRY") ||
      startsWithCase(ac.callsign, "HZS") || startsWithCase(ac.callsign, "SAR") ||
      startsWithCase(ac.callsign, "HEMS") || startsWithCase(ac.callsign, "LZZ") ||
      startsWithCase(ac.callsign, "MEDIC") || startsWithCase(ac.callsign, "RESCUE") ||
      startsWithCase(ac.reg, "OM-AT") || startsWithCase(ac.reg, "OK-AT") ||
      strcasecmp(ac.type, "EC35") == 0 || strcasecmp(ac.type, "EC45") == 0 ||
      strcasecmp(ac.type, "EC20") == 0 || strcasecmp(ac.type, "EC30") == 0 ||
      strcasecmp(ac.type, "H135") == 0 || strcasecmp(ac.type, "H145") == 0 ||
      strcasecmp(ac.type, "H125") == 0 || strcasecmp(ac.type, "H130") == 0 ||
      strcasecmp(ac.type, "H160") == 0 || strcasecmp(ac.type, "H175") == 0 ||
      strcasecmp(ac.type, "AS50") == 0 || strcasecmp(ac.type, "AS55") == 0 ||
      strcasecmp(ac.type, "AS32") == 0 || strcasecmp(ac.type, "A109") == 0 ||
      strcasecmp(ac.type, "A119") == 0 || strcasecmp(ac.type, "A139") == 0 ||
      strcasecmp(ac.type, "A169") == 0 || strcasecmp(ac.type, "A189") == 0 ||
      strcasecmp(ac.type, "AW09") == 0 || strcasecmp(ac.type, "B06") == 0 ||
      strcasecmp(ac.type, "B407") == 0 || strcasecmp(ac.type, "B429") == 0 ||
      strcasecmp(ac.type, "B412") == 0 || strcasecmp(ac.type, "B212") == 0 ||
      strcasecmp(ac.type, "R22") == 0 || strcasecmp(ac.type, "R44") == 0 ||
      strcasecmp(ac.type, "R66") == 0 || strcasecmp(ac.type, "MI8") == 0 ||
      strcasecmp(ac.type, "MI17") == 0 || strcasecmp(ac.type, "MI24") == 0 ||
      strcasecmp(ac.type, "UH60") == 0 || strcasecmp(ac.type, "S76") == 0 ||
      strcasecmp(ac.type, "S92") == 0 || strcasecmp(ac.type, "MD52") == 0 ||
      strcasecmp(ac.type, "G2CA") == 0) {
    return ICON_HELICOPTER;
  }

  // 3. Gliders / Sailplanes / Balloons
  if (strcasecmp(ac.type, "GLID") == 0 || strcasecmp(ac.type, "BALL") == 0 ||
      strcasecmp(ac.type, "PARA") == 0 || strcasecmp(ac.type, "AS21") == 0 ||
      strcasecmp(ac.type, "DG10") == 0 || strcasecmp(ac.type, "DG50") == 0 ||
      strcasecmp(ac.type, "DG80") == 0 || strcasecmp(ac.type, "DISC") == 0 ||
      strcasecmp(ac.type, "DUOD") == 0 || strcasecmp(ac.type, "VENT") == 0 ||
      strcasecmp(ac.type, "LS4") == 0 || strcasecmp(ac.type, "LS8") == 0) {
    return ICON_GLIDER;
  }

  // 4. Heavy Giants
  if (strcasecmp(ac.type, "A388") == 0 || strcasecmp(ac.type, "B744") == 0 ||
      strcasecmp(ac.type, "B748") == 0 || strcasecmp(ac.type, "B742") == 0 ||
      strcasecmp(ac.type, "B741") == 0 || strcasecmp(ac.type, "B743") == 0 ||
      strcasecmp(ac.type, "A124") == 0 || strcasecmp(ac.type, "A225") == 0 ||
      strcasecmp(ac.type, "A3ST") == 0 || strcasecmp(ac.type, "A337") == 0 ||
      strcasecmp(ac.type, "C5") == 0) {
    return ICON_HEAVY;
  }

  // 5. Small / Light General Aviation & Ultralights
  if (strcasecmp(ac.type, "C150") == 0 || strcasecmp(ac.type, "C152") == 0 ||
      strcasecmp(ac.type, "C172") == 0 || strcasecmp(ac.type, "C182") == 0 ||
      strcasecmp(ac.type, "C206") == 0 || strcasecmp(ac.type, "C210") == 0 ||
      strcasecmp(ac.type, "C208") == 0 || strcasecmp(ac.type, "PA28") == 0 ||
      strcasecmp(ac.type, "P28A") == 0 || strcasecmp(ac.type, "P28R") == 0 ||
      strcasecmp(ac.type, "P28T") == 0 || strcasecmp(ac.type, "PA34") == 0 ||
      strcasecmp(ac.type, "PA44") == 0 || strcasecmp(ac.type, "PA32") == 0 ||
      strcasecmp(ac.type, "PA18") == 0 || strcasecmp(ac.type, "PA38") == 0 ||
      strcasecmp(ac.type, "SR20") == 0 || strcasecmp(ac.type, "SR22") == 0 ||
      strcasecmp(ac.type, "SF50") == 0 || strcasecmp(ac.type, "DA40") == 0 ||
      strcasecmp(ac.type, "DA42") == 0 || strcasecmp(ac.type, "DA62") == 0 ||
      strcasecmp(ac.type, "DA20") == 0 || strcasecmp(ac.type, "DV20") == 0 ||
      strcasecmp(ac.type, "PC12") == 0 || strcasecmp(ac.type, "BE36") == 0 ||
      strcasecmp(ac.type, "BE58") == 0 || strcasecmp(ac.type, "BE33") == 0 ||
      strcasecmp(ac.type, "AA5") == 0  || strcasecmp(ac.type, "AT3") == 0  ||
      strcasecmp(ac.type, "P92") == 0  || strcasecmp(ac.type, "P200") == 0 ||
      strcasecmp(ac.type, "P06T") == 0 || strcasecmp(ac.type, "Z42") == 0  ||
      strcasecmp(ac.type, "Z43") == 0  || strcasecmp(ac.type, "Z26") == 0  ||
      strcasecmp(ac.type, "Z50") == 0  || strcasecmp(ac.type, "WT9") == 0  ||
      strcasecmp(ac.type, "VL3") == 0  || strcasecmp(ac.type, "EV97") == 0 ||
      strcasecmp(ac.type, "TL20") == 0 || strcasecmp(ac.type, "TL30") == 0 ||
      strcasecmp(ac.type, "ULAC") == 0) {
    return ICON_LIGHT;
  }

  return ICON_AIRLINER;
}

void Aircraft_DrawIcon(Arduino_GFX* g, int x, int y, float trackDeg, bool hasTrack, uint16_t col, AircraftIconType iconType) {
  if (!g) return;

  if (!hasTrack) {
    // Track unknown - circle with center dot (orientation cannot be determined)
    uint16_t ringCol = (iconType == ICON_MILITARY_JET) ? C_RED : col;
    g->drawCircle(x, y, 7, ringCol);
    g->fillCircle(x, y, 2, ringCol);
    return;
  }

  float a = trackDeg * 0.0174532925f;
  float ca = cosf(a), sa = sinf(a);
  auto rot = [&](float right, float fwd, int* ox, int* oy) {
    *ox = x + (int)(right * ca + fwd * sa);
    *oy = y + (int)(right * sa - fwd * ca);
  };

  switch (iconType) {
    case ICON_HELICOPTER: {
      // --- Helicopter (Droplet cabin, tail boom, main & tail rotors) ---
      const float P_HELI[10][2] = {
        { 0.0f,   7.0f}, { 2.8f,  3.0f}, { 2.5f, -2.0f}, { 1.0f, -5.0f}, { 0.7f, -12.0f},
        { 0.0f, -13.0f}, {-0.7f, -12.0f}, {-1.0f, -5.0f}, {-2.5f, -2.0f}, {-2.8f,  3.0f}
      };
      int px[10], py[10];
      for (int i = 0; i < 10; i++) rot(P_HELI[i][0], P_HELI[i][1], &px[i], &py[i]);
      for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        g->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
      }
      // Main 2-blade rotor across fuselage
      int r1x, r1y, r2x, r2y;
      rot(-11.0f, 1.0f, &r1x, &r1y);
      rot( 11.0f, 1.0f, &r2x, &r2y);
      g->drawLine(r1x, r1y, r2x, r2y, col);
      // Cross rotor blade along fuselage
      int r3x, r3y, r4x, r4y;
      rot(0.0f,  11.0f, &r3x, &r3y);
      rot(0.0f,  -9.0f, &r4x, &r4y);
      g->drawLine(r3x, r3y, r4x, r4y, col);
      // Rotor mast hub
      int hx, hy;
      rot(0.0f, 1.0f, &hx, &hy);
      g->fillCircle(hx, hy, 2, col);
      g->fillCircle(hx, hy, 1, C_WHITE);
      // Tail rotor
      int t1x, t1y, t2x, t2y;
      rot(0.5f, -12.0f, &t1x, &t1y);
      rot(4.5f, -12.0f, &t2x, &t2y);
      g->drawLine(t1x, t1y, t2x, t2y, col);
      break;
    }

    case ICON_LIGHT: {
      // --- Light aircraft / GA prop (Cessna / Piper) ---
      // Shorter fuselage, straight wings, small nose spinner, compact tail
      const float P_LIGHT[14][2] = {
        { 0.0f,  8.0f}, { 1.5f,  3.0f}, { 9.0f,  2.0f}, { 9.0f, -0.5f}, { 1.5f,  0.0f},
        { 1.0f, -5.0f}, { 4.0f, -6.0f}, { 0.0f, -8.0f}, {-4.0f, -6.0f}, {-1.0f, -5.0f},
        {-1.5f,  0.0f}, {-9.0f, -0.5f}, {-9.0f,  2.0f}, {-1.5f,  3.0f}
      };
      int px[14], py[14];
      for (int i = 0; i < 14; i++) rot(P_LIGHT[i][0], P_LIGHT[i][1], &px[i], &py[i]);
      for (int i = 0; i < 14; i++) {
        int j = (i + 1) % 14;
        g->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
      }
      break;
    }

    case ICON_MILITARY_JET: {
      // --- Sharp delta-wing military fighter jet ---
      const float P_MIL[10][2] = {
        { 0.0f,  14.0f}, { 2.0f,  5.0f}, { 13.0f, -6.0f}, { 3.0f, -4.0f}, { 3.0f, -13.0f},
        { 0.0f, -10.0f}, {-3.0f, -13.0f}, {-3.0f, -4.0f}, {-13.0f, -6.0f}, {-2.0f,  5.0f}
      };
      int px[10], py[10];
      for (int i = 0; i < 10; i++) rot(P_MIL[i][0], P_MIL[i][1], &px[i], &py[i]);
      for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        g->fillTriangle(x, y, px[i], py[i], px[j], py[j], C_RED);
      }
      int cx, cy;
      rot(0.0f, 2.0f, &cx, &cy);
      g->fillCircle(cx, cy, 1, C_WHITE);
      break;
    }

    case ICON_HEAVY: {
      // --- Heavy Giant (A380, B747, Antonov) ---
      // Larger swept wings, wide fuselage, 4-engine markers
      const float P_HEAVY[14][2] = {
        { 0.0f,  16.0f}, { 4.0f,  2.0f}, { 16.0f, -9.0f}, { 16.0f, -12.0f}, { 4.0f, -7.0f},
        { 4.0f, -11.0f}, { 8.0f, -14.0f}, {  0.0f, -16.0f}, {-8.0f, -14.0f}, {-4.0f, -11.0f},
        {-4.0f,  -7.0f}, {-16.0f, -12.0f}, {-16.0f, -9.0f}, {-4.0f,  2.0f}
      };
      int px[14], py[14];
      for (int i = 0; i < 14; i++) rot(P_HEAVY[i][0], P_HEAVY[i][1], &px[i], &py[i]);
      for (int i = 0; i < 14; i++) {
        int j = (i + 1) % 14;
        g->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
      }
      // 4 white engine nacelle dots
      int e1x, e1y, e2x, e2y, e3x, e3y, e4x, e4y;
      rot( 7.0f, -4.5f, &e1x, &e1y);
      rot(11.5f, -7.5f, &e2x, &e2y);
      rot(-7.0f, -4.5f, &e3x, &e3y);
      rot(-11.5f, -7.5f, &e4x, &e4y);
      g->fillCircle(e1x, e1y, 1, C_WHITE);
      g->fillCircle(e2x, e2y, 1, C_WHITE);
      g->fillCircle(e3x, e3y, 1, C_WHITE);
      g->fillCircle(e4x, e4y, 1, C_WHITE);
      break;
    }

    case ICON_GLIDER: {
      // --- Sailplane / Glider (Extremely slender long wings, T-tail) ---
      const float P_GLID[14][2] = {
        { 0.0f,  10.0f}, { 1.0f,  2.0f}, { 15.0f,  1.5f}, { 15.0f, -0.5f}, { 1.0f,  0.0f},
        { 0.8f, -10.0f}, { 4.0f, -11.5f}, { 0.0f, -13.0f}, {-4.0f, -11.5f}, {-0.8f, -10.0f},
        {-1.0f,  0.0f}, {-15.0f, -0.5f}, {-15.0f,  1.5f}, {-1.0f,  2.0f}
      };
      int px[14], py[14];
      for (int i = 0; i < 14; i++) rot(P_GLID[i][0], P_GLID[i][1], &px[i], &py[i]);
      for (int i = 0; i < 14; i++) {
        int j = (i + 1) % 14;
        g->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
      }
      break;
    }

    case ICON_AIRLINER:
    default: {
      // --- Civil commercial airliner ---
      const float P[10][2] = {
        { 0.0f,  12.0f}, { 3.0f,  1.0f}, { 13.0f, -8.0f}, { 3.0f, -5.0f}, { 3.0f, -7.0f},
        { 0.0f, -12.0f}, {-3.0f, -7.0f}, {-3.0f, -5.0f}, {-13.0f, -8.0f}, {-3.0f,  1.0f}
      };
      int px[10], py[10];
      for (int i = 0; i < 10; i++) rot(P[i][0], P[i][1], &px[i], &py[i]);
      for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        g->fillTriangle(x, y, px[i], py[i], px[j], py[j], col);
      }
      break;
    }
  }
}

const char* Aircraft_GetCategoryName(AircraftIconType iconType) {
  uint8_t lang = Lang_Get();
  switch (iconType) {
    case ICON_HELICOPTER:
      return (lang == LANG_EN) ? "Rotorcraft / Helicopter"
           : ((lang == LANG_SK) ? "Zachranny / Vrtulnik" : "Zachranny / Vrtulnik");
    case ICON_MILITARY_JET:
      return (lang == LANG_EN) ? "Military Fighter Jet"
           : ((lang == LANG_SK) ? "Vojenska stihacka" : "Vojenska stihacka");
    case ICON_HEAVY:
      return (lang == LANG_EN) ? "Heavy Quad-Jet Giant"
           : ((lang == LANG_SK) ? "Stvormotorovy gigant" : "Ctyrmotorovy gigant");
    case ICON_LIGHT:
      return (lang == LANG_EN) ? "General Aviation Light"
           : ((lang == LANG_SK) ? "Vseobecne / Lahke GA" : "Vseobecne / Lehci GA");
    case ICON_GLIDER:
      return (lang == LANG_EN) ? "Sailplane / Glider"
           : ((lang == LANG_SK) ? "Bezmotorovy vetron" : "Bezmotorovy vetron");
    case ICON_AIRLINER:
    default:
      return (lang == LANG_EN) ? "Civil Airliner"
           : ((lang == LANG_SK) ? "Dopravne lietadlo" : "Dopravni letoun");
  }
}

const char* Aircraft_IdentifyModel(const Aircraft& ac, char* out, size_t outCap) {
  if (!out || outCap == 0) return "";
  out[0] = '\0';

  // 1. If explicit ICAO type is already in database, format it directly
  if (ac.type[0]) {
    const char* f = AircraftType_Format(ac.type);
    if (f && f[0] && strcmp(f, ac.type) != 0) {
      strncpy(out, f, outCap - 1);
      out[outCap - 1] = '\0';
      return out;
    }
  }

  // 2. Infer from callsign or registration for special flights
  if (startsWithCase(ac.callsign, "ATE") || startsWithCase(ac.callsign, "KRY") ||
      startsWithCase(ac.reg, "OM-AT") || startsWithCase(ac.reg, "OK-AT")) {
    snprintf(out, outCap, "Airbus H135 (HEMS Rescue)");
    return out;
  }
  if (startsWithCase(ac.callsign, "SSG") || startsWithCase(ac.callsign, "SVK0")) {
    snprintf(out, outCap, "Airbus A319 (SVK Government)");
    return out;
  }
  if (startsWithCase(ac.callsign, "CEF") || startsWithCase(ac.callsign, "CZE0")) {
    snprintf(out, outCap, "Airbus A319 / C-295 (Czech AF)");
    return out;
  }
  if (startsWithCase(ac.callsign, "NATO")) {
    snprintf(out, outCap, "Boeing E-3A / C-17 (NATO)");
    return out;
  }
  if (startsWithCase(ac.callsign, "JAS")) {
    snprintf(out, outCap, "Saab JAS 39 Gripen");
    return out;
  }
  if (startsWithCase(ac.callsign, "ALCA")) {
    snprintf(out, outCap, "Aero L-159 ALCA");
    return out;
  }

  // 3. Fallback to raw type if present
  if (ac.type[0]) {
    strncpy(out, ac.type, outCap - 1);
    out[outCap - 1] = '\0';
    return out;
  }

  // 4. Default to Category name
  AircraftIconType icon = Aircraft_GetIconType(ac);
  strncpy(out, Aircraft_GetCategoryName(icon), outCap - 1);
  out[outCap - 1] = '\0';
  return out;
}

void Aircraft_DrawDetailedSilhouette(Arduino_GFX* g, int cx, int cy, int maxW, int maxH, uint16_t col, AircraftIconType iconType) {
  if (!g) return;

  switch (iconType) {
    case ICON_HELICOPTER: {
      // Rotorcraft body
      g->fillRoundRect(cx - 14, cy - 14, 28, 32, 6, col);
      g->fillRoundRect(cx - 10, cy - 12, 20, 9, 3, 0x18C3);
      // Tail boom
      g->fillTriangle(cx - 3, cy + 18, cx + 3, cy + 18, cx, cy + 40, col);
      g->fillTriangle(cx - 2, cy + 34, cx + 11, cy + 40, cx - 2, cy + 40, col);
      g->drawLine(cx + 9, cy + 33, cx + 9, cy + 45, C_WHITE);
      // Landing skids
      g->drawFastVLine(cx - 18, cy - 12, 34, col);
      g->drawFastVLine(cx + 17, cy - 12, 34, col);
      g->drawFastHLine(cx - 18, cy - 2, 5, col);
      g->drawFastHLine(cx + 13, cy - 2, 5, col);
      g->drawFastHLine(cx - 18, cy + 12, 5, col);
      g->drawFastHLine(cx + 13, cy + 12, 5, col);
      // Rotor mast & 4 main rotor blades
      g->fillCircle(cx, cy - 2, 4, col);
      g->fillCircle(cx, cy - 2, 2, C_WHITE);
      g->drawLine(cx - 58, cy - 2, cx + 58, cy - 2, col);
      g->drawLine(cx - 58, cy - 1, cx + 58, cy - 1, col);
      g->drawLine(cx, cy - 50, cx, cy + 46, col);
      g->drawLine(cx + 1, cy - 50, cx + 1, cy + 46, col);
      break;
    }

    case ICON_MILITARY_JET: {
      // Delta-wing fighter jet
      g->fillTriangle(cx, cy - 42, cx - 7, cy + 22, cx + 7, cy + 22, col);
      g->fillRoundRect(cx - 5, cy - 12, 10, 38, 3, col);
      g->fillRoundRect(cx - 3, cy - 22, 6, 12, 2, 0x18C3);
      g->fillTriangle(cx - 5, cy - 8, cx - 52, cy + 18, cx - 5, cy + 20, col);
      g->fillTriangle(cx + 5, cy - 8, cx + 52, cy + 18, cx + 5, cy + 20, col);
      g->drawFastVLine(cx - 52, cy + 8, 14, col);
      g->drawFastVLine(cx + 52, cy + 8, 14, col);
      g->fillTriangle(cx - 8, cy + 14, cx - 15, cy + 30, cx - 6, cy + 28, col);
      g->fillTriangle(cx + 8, cy + 14, cx + 15, cy + 30, cx + 6, cy + 28, col);
      g->fillCircle(cx - 4, cy + 28, 2, C_RED);
      g->fillCircle(cx + 4, cy + 28, 2, C_RED);
      break;
    }

    case ICON_HEAVY: {
      // Quad-jet heavy widebody
      g->fillRoundRect(cx - 8, cy - 40, 16, 78, 7, col);
      g->drawFastHLine(cx - 4, cy - 32, 8, C_BLACK);
      g->fillTriangle(cx - 6, cy - 10, cx - 68, cy + 16, cx - 6, cy + 12, col);
      g->fillTriangle(cx + 6, cy - 10, cx + 68, cy + 16, cx + 6, cy + 12, col);
      g->drawLine(cx - 68, cy + 16, cx - 70, cy + 8, col);
      g->drawLine(cx + 68, cy + 16, cx + 70, cy + 8, col);
      // 4 engine nacelles
      g->fillRoundRect(cx - 30, cy + 1, 6, 14, 2, col);
      g->fillCircle(cx - 27, cy + 1, 1, C_WHITE);
      g->fillRoundRect(cx - 48, cy + 6, 6, 14, 2, col);
      g->fillCircle(cx - 45, cy + 6, 1, C_WHITE);
      g->fillRoundRect(cx + 25, cy + 1, 6, 14, 2, col);
      g->fillCircle(cx + 28, cy + 1, 1, C_WHITE);
      g->fillRoundRect(cx + 43, cy + 6, 6, 14, 2, col);
      g->fillCircle(cx + 46, cy + 6, 1, C_WHITE);
      g->fillTriangle(cx - 4, cy + 28, cx - 28, cy + 38, cx - 4, cy + 36, col);
      g->fillTriangle(cx + 4, cy + 28, cx + 28, cy + 38, cx + 4, cy + 36, col);
      g->drawFastVLine(cx, cy + 20, 16, C_WHITE);
      break;
    }

    case ICON_LIGHT: {
      // GA propeller aircraft
      g->fillCircle(cx, cy - 32, 3, C_WHITE);
      g->drawFastHLine(cx - 14, cy - 32, 28, C_WHITE);
      g->fillRoundRect(cx - 5, cy - 28, 10, 52, 4, col);
      g->drawFastHLine(cx - 3, cy - 14, 6, 0x18C3);
      g->drawFastHLine(cx - 3, cy - 8, 6, 0x18C3);
      g->fillRoundRect(cx - 50, cy - 14, 100, 7, 2, col);
      g->fillRoundRect(cx - 16, cy + 18, 32, 5, 2, col);
      g->drawFastVLine(cx, cy + 14, 10, C_WHITE);
      break;
    }

    case ICON_GLIDER: {
      // Slender sailplane / glider
      g->fillRoundRect(cx - 3, cy - 34, 6, 66, 3, col);
      g->fillRoundRect(cx - 2, cy - 18, 4, 8, 2, 0x18C3);
      g->drawLine(cx - 72, cy - 12, cx + 72, cy - 12, col);
      g->drawLine(cx - 72, cy - 11, cx + 72, cy - 11, col);
      g->fillTriangle(cx - 7, cy - 13, cx - 70, cy - 10, cx - 7, cy - 9, col);
      g->fillTriangle(cx + 7, cy - 13, cx + 70, cy - 10, cx + 7, cy - 9, col);
      g->drawLine(cx - 72, cy - 12, cx - 73, cy - 17, col);
      g->drawLine(cx + 72, cy - 12, cx + 73, cy - 17, col);
      g->drawFastHLine(cx - 15, cy + 30, 30, col);
      g->drawFastHLine(cx - 15, cy + 31, 30, col);
      break;
    }

    case ICON_AIRLINER:
    default: {
      // Civil airliner
      g->fillRoundRect(cx - 6, cy - 34, 12, 68, 5, col);
      g->drawFastHLine(cx - 3, cy - 28, 6, C_BLACK);
      g->fillTriangle(cx - 4, cy - 8, cx - 62, cy + 14, cx - 4, cy + 10, col);
      g->fillTriangle(cx + 4, cy - 8, cx + 62, cy + 14, cx + 4, cy + 10, col);
      g->drawLine(cx - 62, cy + 14, cx - 63, cy + 7, col);
      g->drawLine(cx + 62, cy + 14, cx + 63, cy + 7, col);
      // Twin underwing jet engines
      g->fillRoundRect(cx - 26, cy, 6, 16, 2, col);
      g->fillCircle(cx - 23, cy, 2, C_WHITE);
      g->fillRoundRect(cx + 20, cy, 6, 16, 2, col);
      g->fillCircle(cx + 23, cy, 2, C_WHITE);
      g->fillTriangle(cx - 3, cy + 22, cx - 24, cy + 31, cx - 3, cy + 29, col);
      g->fillTriangle(cx + 3, cy + 22, cx + 24, cy + 31, cx + 3, cy + 29, col);
      g->drawFastVLine(cx, cy + 15, 15, C_WHITE);
      break;
    }
  }
}


