// =============================================================================
//  MeteoPlaneRadar
//  PetBrain.h - AI Pet intelligence, telemetry synthesis, and Gemini LLM thoughts.
//  Includes virtual pet mechanics & dynamics inspired by DigiCat
//  (https://github.com/aquascape123/digicat) by aquascape123 under MIT License.
// =============================================================================
#pragma once
#include <Arduino.h>

enum PetMood {
  PET_MOOD_IDLE = 0,
  PET_MOOD_HAPPY,
  PET_MOOD_EXCITED,    // Plane very close or high speed
  PET_MOOD_SLEEPY,     // Night mode or late hours
  PET_MOOD_RAIN,       // Rain / storm detected
  PET_MOOD_THINKING    // Fetching Gemini thought
};

struct PetStats {
  uint8_t happiness;       // 0 - 100%
  uint8_t hunger;          // 0 - 100% (0 = full, 100 = starving)
  uint16_t flightsTracked; // Total flights tracked with pet
  uint8_t stage;           // 0: Cadet, 1: Navigator, 2: Ace
};

void     PetBrain_Init();
void     PetBrain_Tick();                     // Core 1 periodic checks (mood & stat updates)
void     PetBrain_RequestThought(bool userTapped = false);
void     PetBrain_Feed();                     // Give treat / feed pet
PetStats PetBrain_GetStats();
const char* PetBrain_GetStageTitle();
bool     PetBrain_Step();                     // Core 0 background fetch via AsyncCore
bool     PetBrain_IsBusy();

PetMood  PetBrain_GetMood();
void     PetBrain_SetMood(PetMood mood);
const char* PetBrain_GetThought();
bool     PetBrain_GetClosestPlaneTarget(float& bearingDeg, float& distKm, char* outCallsign, size_t callsignCap);
