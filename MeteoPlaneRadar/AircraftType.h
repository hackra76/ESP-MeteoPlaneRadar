// =============================================================================
//  MeteoPlaneRadar
//  AircraftType.h - Human-readable aircraft type translation & special classification.
// =============================================================================
#pragma once
#include <stddef.h>
#include <stdint.h>

// Forward declaration
struct Aircraft;

// Returns human-friendly aircraft model name (e.g. "Airbus A320") from ICAO
// designator (e.g. "A320"). Returns icaoType itself if not found.
const char* AircraftType_Format(const char* icaoType);

enum SpecialCategory : uint8_t {
  SPEC_NONE = 0,
  SPEC_RESCUE,      // HEMS / Rescue helicopters (ATE, Kryštof, etc.)
  SPEC_GOVERNMENT,  // VIP / Government specials (SSG, CEF, etc.)
  SPEC_HEAVY_ICON,  // A380, B747, An-124, Beluga
  SPEC_MILITARY,    // Military fighters, transports, or dbFlags
  SPEC_WATCHED      // Custom user watchlist callsign match
};

// Classifies an aircraft into a special interest category.
// Populates a short badge/type label if labelOut is provided.
// Returns category and assigns recommended highlight color into colorOut.
SpecialCategory Aircraft_Classify(const Aircraft& ac, char* labelOut = nullptr, size_t labelCap = 0, uint16_t* colorOut = nullptr);

class Arduino_GFX;

enum AircraftIconType : uint8_t {
  ICON_AIRLINER = 0,    // Standard civil airliner (A320, B737, E190, etc.)
  ICON_LIGHT,           // Small GA propeller aircraft (Cessna 172, Piper, Ultralight, etc.)
  ICON_HELICOPTER,      // Rotorcraft / Helicopters (H145, R44, Mi-8, ATE, etc.)
  ICON_MILITARY_JET,    // Fighter jet / delta wing (F16, Gripen, L39, etc.)
  ICON_HEAVY,           // Heavy widebody giants (A380, B747, An-124, Beluga)
  ICON_GLIDER           // Sailplanes / gliders (GLID, etc.)
};

// Determines the icon shape based on airframe type, callsign, and flags
AircraftIconType Aircraft_GetIconType(const Aircraft& ac);

// Draws vector aircraft silhouette matching its category rotated by trackDeg
void Aircraft_DrawIcon(Arduino_GFX* g, int x, int y, float trackDeg, bool hasTrack, uint16_t col, AircraftIconType iconType);

