// =============================================================================
//  MeteoPlaneRadar
//  PetDrawer.h - Pull-up Pet Companion drawer overlay and animation engine.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <lvgl.h>

void PetDrawer_Init();

bool PetDrawer_IsOpen();
void PetDrawer_Open();
// Debug (runtime-only): force weather 0=clear 1=rain 2=thunder 3=snow, -1 = live; toggle military accessories
void PetDrawer_DebugSetWeather(int w);
void PetDrawer_DebugToggleMilitary();
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

// Returns true if pet is currently in the active wake period during night hours
bool PetDrawer_IsNightAwake();
