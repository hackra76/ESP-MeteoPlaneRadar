// =============================================================================
//  MeteoPlaneRadar
//  The configuration web server - interface.
//
//  One HTTP server serves two roles, which is why they live together:
//
//    ACCESS POINT (no WiFi credentials yet). The device puts up an open network
//    and a captive portal. It stays up until a network is actually entered -
//    there is no timeout, because without WiFi the device has nothing else to
//    do. The old build gave up after three minutes and went back to a radar
//    with no data on it, which helped nobody.
//
//    CONNECTED. The same server runs permanently on the home network and serves
//    the full settings page at http://meteoplaneradar.local/ (or the IP). It is
//    not on a timer either: a settings page you have to go and re-enable on the
//    device first is a settings page you stop using.
//
//  Firmware updates now happen here too, at /update, so the separate update
//  access point and its screen are gone.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

// Start (or restart) the server for the current WiFi role.
void WebConfig_Begin(bool apMode);

// Pump the server and the captive-portal DNS. Call from loop().
void WebConfig_Loop();

// True while a firmware upload is running - the main loop stops drawing and
// stops fetching so the flash write gets the bus to itself.
bool WebConfig_UpdateBusy();

// Credentials arrived from the portal and a connection attempt is wanted.
bool WebConfig_WantsWifiConnect();
void WebConfig_ClearWifiConnect();

// Something changed that only takes effect from a clean start (the set of
// screens, the radar source, the location).
bool WebConfig_WantsRestart();

// --- Remote control ---------------------------------------------------------
// The web UI can drive the device without anyone touching the glass. Requests
// are QUEUED here and carried out by loop(), for the same reason a touch is:
// switching screens or redrawing from inside a request handler would re-enter
// the drawing code while the canvas is mid-frame.
//
// Each call returns the pending request and clears it.
int WebConfig_TakeScreen();       // screen index, or -1 when nothing is pending
int WebConfig_TakeScreenStep();   // -1 / +1, or 0
int WebConfig_TakeRangeStep();    // -1 / +1, or 0
