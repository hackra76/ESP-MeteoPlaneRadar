// =============================================================================
//  MeteoPlaneRadar
//  AircraftType.cpp - Aircraft type lookup table.
// =============================================================================
#include "AircraftType.h"
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

  // Military / Trainers / Gliders
  { "L39",  "Aero L-39 Albatros" },
  { "L159", "Aero L-159 Alca" },
  { "JAS3", "JAS 39 Gripen" },
  { "EUFI", "Eurofighter Typhoon" },
  { "F16",  "Lockheed F-16" },
  { "F35",  "Lockheed F-35" },
  { "C130", "Lockheed C-130" },
  { "A400", "Airbus A400M" },
  { "C17",  "Boeing C-17" },
  { "GLID", "Vetroň (Glider)" }
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
#include <stdio.h>

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

