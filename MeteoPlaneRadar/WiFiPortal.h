// =============================================================================
//  MeteoPlaneRadar
//  WiFi - connect, or put up an access point until someone tells us a network.
//
//  WiFiManager is gone as of 0.6.0. Two reasons: its portal is English-only and
//  cannot be translated, and it blocks the whole sketch while it runs - which
//  meant suspending the watchdog and freezing the display. The replacement is a
//  plain access point plus the project's own web page, so the portal speaks the
//  same two languages as the rest of the device and the main loop keeps
//  running the whole time.
//
//  The access point has NO timeout. Without a network the device cannot fetch
//  anything, so there is nothing to go back to - the old three-minute timeout
//  just dropped the user onto an empty radar.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"   // AP_SSID / AP_PASSWORD
#define PORTAL_IP "192.168.4.1"

// Connect with the stored credentials, or bring up the access point. Blocks
// only for the initial connection attempt; everything after that is in Loop().
void WiFi_Begin();

// Connection upkeep and the handover from the portal. Call from loop().
void WiFi_Loop();

bool   WiFi_IsConnected();
bool   WiFi_IsAP();
String WiFi_SSID();
String WiFi_IP();

// Forget the network and go back to the access point.
void WiFi_Reset();

// Redraw the access-point instructions (SSID, QR code, address).
void WiFi_DrawApScreen();
