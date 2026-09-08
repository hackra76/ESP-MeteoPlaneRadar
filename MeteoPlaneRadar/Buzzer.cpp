// =============================================================================
//  MeteoPlaneRadar
//  Buzzer.cpp - Non-blocking acoustic alert & feedback engine.
//
//  Hardware: Active buzzer connected to TCA9554 EXIO8 on Waveshare
//            ESP32-S3-Touch-LCD-2.1.
// =============================================================================
#include "Buzzer.h"
#include "TCA9554.h"
#include "Settings.h"
#include <Arduino.h>

struct ToneStep {
  uint16_t ms;
  bool     state;
};

static const ToneStep PATTERN_CLICK[] = {
  { 10, true },
  { 0,  false }
};

static const ToneStep PATTERN_HOURLY[] = {
  { 40, true },
  { 0,  false }
};

static const ToneStep PATTERN_OVERHEAD[] = {
  { 35, true },
  { 50, false },
  { 35, true },
  { 0,  false }
};

static const ToneStep PATTERN_PRECIP[] = {
  { 40, true },
  { 50, false },
  { 40, true },
  { 50, false },
  { 60, true },
  { 0,  false }
};

static const ToneStep PATTERN_WATCHED[] = {
  { 50, true },
  { 70, false },
  { 50, true },
  { 0,  false }
};

static const ToneStep PATTERN_EMERGENCY[] = {
  { 90, true },
  { 70, false },
  { 90, true },
  { 70, false },
  { 120, true },
  { 0,  false }
};

static BuzzerTone      s_activeTone = BEEP_NONE;
static const ToneStep* s_currPattern = nullptr;
static uint8_t         s_stepIdx = 0;
static unsigned long   s_stepStart = 0;

void Buzzer_Init() {
  s_activeTone = BEEP_NONE;
  s_currPattern = nullptr;
  s_stepIdx = 0;
  TCA9554_SetPin(EXIO_BUZZER, false);
}

void Buzzer_Stop() {
  if (s_activeTone != BEEP_NONE) {
    TCA9554_SetPin(EXIO_BUZZER, false);
    s_activeTone = BEEP_NONE;
    s_currPattern = nullptr;
  }
}

bool Buzzer_IsPlaying() {
  return s_activeTone != BEEP_NONE;
}

void Buzzer_Play(BuzzerTone tone) {
  if (tone == BEEP_NONE) {
    Buzzer_Stop();
    return;
  }

  // Master switch
  if (!Settings_BuzzerEnabled()) return;

  // Night mode mute (mute everything except emergency squawk, or all if configured)
  if (Settings_BuzzerNightMute() && Settings_IsNight()) {
    if (tone != BEEP_EMERGENCY) return;
  }

  // Individual category filters
  switch (tone) {
    case BEEP_CLICK:
      if (!Settings_BuzzerTouch()) return;
      break;
    case BEEP_HOURLY:
      if (!Settings_BuzzerHourly()) return;
      break;
    case BEEP_OVERHEAD:
      if (!Settings_BuzzerOverhead()) return;
      break;
    case BEEP_PRECIP:
      if (!Settings_BuzzerPrecip()) return;
      break;
    case BEEP_WATCHED:
      if (!Settings_BuzzerWatch()) return;
      break;
    case BEEP_EMERGENCY:
      if (!Settings_BuzzerEmergency()) return;
      break;
    default:
      break;
  }

  // Priority check: lower priority tones do not override higher ones
  // BEEP_CLICK (1) < BEEP_HOURLY (2) < BEEP_PRECIP (3) < BEEP_OVERHEAD (4) < BEEP_WATCHED (5) < BEEP_EMERGENCY (6)
  if (s_activeTone > tone && s_activeTone != BEEP_NONE) {
    return;
  }

  // BEEP_CLICK is a tactile micro-tick (2.5 ms). Generating it synchronously ensures
  // it is crisp and never gets extended by screen transitions or frame rendering.
  if (tone == BEEP_CLICK) {
    TCA9554_SetPin(EXIO_BUZZER, true);
    delayMicroseconds(2500);
    TCA9554_SetPin(EXIO_BUZZER, false);
    return;
  }

  const ToneStep* pat = nullptr;
  switch (tone) {
    case BEEP_HOURLY:    pat = PATTERN_HOURLY;    break;
    case BEEP_OVERHEAD:  pat = PATTERN_OVERHEAD;  break;
    case BEEP_PRECIP:    pat = PATTERN_PRECIP;    break;
    case BEEP_WATCHED:   pat = PATTERN_WATCHED;   break;
    case BEEP_EMERGENCY: pat = PATTERN_EMERGENCY; break;
    default: return;
  }

  s_activeTone = tone;
  s_currPattern = pat;
  s_stepIdx = 0;
  s_stepStart = millis();

  TCA9554_SetPin(EXIO_BUZZER, s_currPattern[0].state);
}

void Buzzer_Tick() {
  if (s_activeTone == BEEP_NONE || !s_currPattern) return;

  unsigned long now = millis();
  if (now - s_stepStart >= s_currPattern[s_stepIdx].ms) {
    s_stepIdx++;
    if (s_currPattern[s_stepIdx].ms == 0) {
      // Finished
      TCA9554_SetPin(EXIO_BUZZER, false);
      s_activeTone = BEEP_NONE;
      s_currPattern = nullptr;
    } else {
      s_stepStart = now;
      TCA9554_SetPin(EXIO_BUZZER, s_currPattern[s_stepIdx].state);
    }
  }
}
