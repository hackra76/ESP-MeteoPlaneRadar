// =============================================================================
//  MeteoPlaneRadar
//  PetDrawer.h - Pull-up Pet Companion drawer overlay and animation engine.
// =============================================================================
#pragma once
#include <Arduino.h>

bool PetDrawer_IsOpen();
void PetDrawer_Open();
void PetDrawer_Close();
void PetDrawer_Toggle();

// Periodic tick for animation frame pacing (blinking, pupil tracking)
bool PetDrawer_Tick();

// Returns true if tap was handled inside the drawer
bool PetDrawer_HandleTap(int x, int y);

// Continuous touch / petting drag support
void PetDrawer_HandleTouchMove(int x, int y);
void PetDrawer_HandleTouchRelease();

// Draws the Pet Drawer overlay on top of active canvas
void PetDrawer_Draw();
