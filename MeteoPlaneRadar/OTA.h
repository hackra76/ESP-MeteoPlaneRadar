// =============================================================================
//  MeteoPlaneRadar
//  Firmware updates moved in 0.6.0.
//
//  Up to 0.5.5 an update meant leaving normal operation, bringing up a separate
//  access point and serving one page from it. Now that the configuration web
//  server runs permanently there is no reason for any of that: the update page
//  is simply /update on the same server, at the device's normal address.
//
//  This header is kept so the change is discoverable rather than a file that
//  silently vanished. There is nothing to include.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
// See WebConfig.cpp - /update is served by the main web server there, on top of
// the Update class from the ESP32 core.
