// =============================================================================
//  MeteoPlaneRadar
//  Screen: what the Czech grid is generating right now - interface.
//
//  The companion to the price screen: that one says what electricity costs,
//  this one says why. A ring of sources with the renewable share in the middle,
//  the load under it and the cross-border balance at the foot.
//
//  No controls. There is one number per source and no range to change - a
//  screen that does nothing when touched is better than one that pretends to.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

void ScreenMix_Enter();
void ScreenMix_Draw();
bool ScreenMix_Tick();
