// =============================================================================
//  MeteoPlaneRadar
//  Buzzer.h - Non-blocking acoustic alert & feedback engine.
//
//  Hardware: Active buzzer connected to TCA9554 EXIO8 on Waveshare
//            ESP32-S3-Touch-LCD-2.1.
// =============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

enum BuzzerTone : uint8_t {
  BEEP_NONE = 0,
  BEEP_CLICK,        // Short 10ms micro-click for touch / button feedback
  BEEP_HOURLY,       // Pleasant 40ms pulse for hourly chime
  BEEP_OVERHEAD,     // Pleasant double pulse for overhead flight arrival
  BEEP_PRECIP,       // Distinct 3-pulse chime for approaching precipitation alert
  BEEP_WATCHED,      // 2 pulses (50ms on, 70ms off, 50ms on) for watched flight
  BEEP_SONAR_PING,   // Acoustic double sonar ping (35ms on, 110ms off, 55ms on) for watched/rescue flight
  BEEP_EMERGENCY,    // 3 urgent pulses (90ms on, 70ms off, 90ms on, 70ms off, 120ms on)
  BEEP_MORSE_SOS,    // Authentic Morse code SOS (... --- ...) for squawk 7700 emergency
  BEEP_PET_PURR,     // Soft rhythmic purr pulses for petting
  BEEP_PET_CHIRP,    // Cheerful micro-chirp when happy / tickled
  BEEP_PET_SNEEZE,   // Playful sneeze pop (achoo!)
  BEEP_TEST          // Hardware test chirp sequence (bypasses all mutes/filters)
};

// Initialize buzzer (ensures pin is LOW)
void Buzzer_Init();

// Play a buzzer tone pattern (non-blocking). If force is true, bypasses master switch and night mute.
void Buzzer_Play(BuzzerTone tone, bool force = false);

// Explicit hardware test beep sequence (bypasses all filters and night mute)
void Buzzer_PlayTest();

// Stop any currently playing tone immediately
void Buzzer_Stop();

// Advance buzzer state machine - MUST be called frequently from loop()
void Buzzer_Tick();

// Returns true if a tone is currently active
bool Buzzer_IsPlaying();
