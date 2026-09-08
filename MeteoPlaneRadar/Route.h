// =============================================================================
//  MeteoPlaneRadar
//  Flight route lookup from adsb.lol (position + plausibility check).
//
//  adsb.fi provides aircraft positions, but not routes, so routes must come
//  from elsewhere. Previously, a purely static database of planned routes was
//  used: one row per callsign, without dates or link to an actual flight.
//  Callsigns are recycled between rotations and seasons, so an aircraft over
//  Prague could easily get an Athens -> Istanbul route with no way to tell it
//  was wrong.
//
//  adsb.lol adds a critical feature: along with the callsign, the aircraft's
//  current position is sent, and the server returns a "plausible" flag.
//  It computes the perpendicular distance of the position from the great circle
//  track between the route airports with a tolerance of max(50 NM, 20% of route length).
//  A route that does not fit the position is filtered out directly on the server.
//
//  Endpoint (free, no API key, no registration):
//    GET https://api.adsb.lol/api/0/route/{callsign}/{lat}/{lon}
//
//  Queried ONLY when opening the detail view of an aircraft - one query per
//  aircraft, never for the entire list - and the answer is cached, so toggling
//  between aircraft does not burden the API. Many flights have no route
//  (general aviation, military, helicopters) and many transmit no callsign;
//  both are expected conditions, not errors, and simply result in no route shown.
//
//  Registration and aircraft type are handled separately - both arrive from adsb.fi
//  in the position payload ("r" and "t" fields), see ADSB.h.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
// =============================================================================
#pragma once
#include <Arduino.h>

enum RouteState : uint8_t {
  ROUTE_IDLE = 0,   // Nothing requested
  ROUTE_WAIT,       // In queue / downloading
  ROUTE_OK,         // Route found and server marked it plausible
  ROUTE_NONE        // Query finished, but no usable route found
};

struct RouteInfo {
  char from[20]     = "";   // "Prague", or "PRG" if city is missing
  char to[20]       = "";
  char iataFrom[5]  = "";   // 3-letter IATA code (e.g. "PRG")
  char iataTo[5]    = "";   // 3-letter IATA code (e.g. "LHR")
};

// Query route for this aircraft. Cheap and idempotent: repeated calls with
// the same callsign do nothing once cached. An empty callsign does not trigger
// any query. Hex is not used as a fallback because normalized hex can match
// valid airline codes (e.g. "a31234" -> "A31234" = Aegean Airlines).
// Current aircraft position is sent along so the server can evaluate plausibility.
void       Route_Select(const char* callsign, float lat, float lon);

// Clear selection - cancels pending query.
void       Route_Clear();

// Process pending query. Called from loop(); does nothing if idle.
void       Route_Tick();

// Yield + watchdog reset, called during network stream processing.
void       Route_SetPollFn(void (*fn)());

RouteState Route_GetState();
const RouteInfo* Route_Get();   // Valid while state is ROUTE_OK

// Returns true once when Route_Tick() finishes a result, clearing the flag.
bool       Route_TakeChanged();

// Look up a cached route by callsign without changing the "selected" aircraft.
// Returns a pointer to cached RouteInfo if found and state is ROUTE_OK, else
// nullptr. Used by the radar screen to show IATA route labels on the map.
const RouteInfo* Route_GetCached(const char* callsign);

// Queue a background route lookup for an aircraft visible on the radar.
// Like Route_Select but does not change the "selected" aircraft.
void       Route_Queue(const char* callsign, float lat, float lon);
void       Route_ClearQueue();
bool       Route_HasPending();
