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

static const ToneStep PATTERN_SONAR_PING[] = {
  { 35,  true  },
  { 110, false },
  { 55,  true  },
  { 0,   false }
};

static const ToneStep PATTERN_EMERGENCY[] = {
  { 90, true },
  { 70, false },
  { 90, true },
  { 70, false },
  { 120, true },
  { 0,  false }
};

// Morse Code SOS: ... --- ...
// S: dit (70ms) dit (70ms) dit (70ms) with 60ms gaps, 180ms inter-letter pause
// O: dah (210ms) dah (210ms) dah (210ms) with 60ms gaps, 180ms inter-letter pause
// S: dit (70ms) dit (70ms) dit (70ms)
static const ToneStep PATTERN_MORSE_SOS[] = {
  // S: ...
  { 70,  true  }, { 60, false },
  { 70,  true  }, { 60, false },
  { 70,  true  }, { 180, false },
  // O: ---
  { 210, true  }, { 60, false },
  { 210, true  }, { 60, false },
  { 210, true  }, { 180, false },
  // S: ...
  { 70,  true  }, { 60, false },
  { 70,  true  }, { 60, false },
  { 70,  true  }, { 0,   false }
};

static const ToneStep PATTERN_PET_PURR[] = {
  { 12, true }, { 35, false },
  { 14, true }, { 35, false },
  { 12, true }, { 35, false },
  { 14, true }, { 0,  false }
};

static const ToneStep PATTERN_PET_CHIRP[] = {
  { 18, true }, { 35, false },
  { 32, true }, { 0,  false }
};

static const ToneStep PATTERN_PET_SNEEZE[] = {
  { 14, true }, { 30, false },
  { 65, true }, { 0,  false }
};

static const ToneStep PATTERN_TEST[] = {
  { 60, true },
  { 60, false },
  { 60, true },
  { 60, false },
  { 100, true },
  { 0,  false }
};

static volatile BuzzerTone      s_activeTone = BEEP_NONE;
static const ToneStep* volatile s_currPattern = nullptr;
static volatile uint8_t         s_stepIdx = 0;
static volatile unsigned long   s_stepStart = 0;

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

void Buzzer_Play(BuzzerTone tone, bool force) {
  if (tone == BEEP_NONE) {
    Buzzer_Stop();
    return;
  }

  // Force flag or test tone bypasses master switch, night mute, and category filters
  if (!force && tone != BEEP_TEST) {
    // Master switch
    if (!Settings_BuzzerEnabled()) return;

    // Night mode mute (mute everything except emergency squawk, or all if configured)
    if (Settings_BuzzerNightMute() && Settings_IsNight()) {
      if (tone != BEEP_EMERGENCY && tone != BEEP_MORSE_SOS) return;
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
      case BEEP_SONAR_PING:
        if (!Settings_BuzzerWatch()) return;
        break;
      case BEEP_EMERGENCY:
      case BEEP_MORSE_SOS:
        if (!Settings_BuzzerEmergency()) return;
        break;
      case BEEP_PET_PURR:
      case BEEP_PET_CHIRP:
      case BEEP_PET_SNEEZE:
        if (!Settings_BuzzerPet()) return;
        break;
      default:
        break;
    }
  }

  // Priority check: lower priority tones do not override higher ones (unless forced)
  if (!force && tone != BEEP_TEST && s_activeTone > tone && s_activeTone != BEEP_NONE) {
    return;
  }

  // BEEP_CLICK is a tactile click (12 ms). Generating it synchronously ensures
  // it is crisp and never gets extended by screen transitions or frame rendering.
  if (tone == BEEP_CLICK) {
    TCA9554_SetPin(EXIO_BUZZER, true);
    delay(12);
    TCA9554_SetPin(EXIO_BUZZER, false);
    return;
  }

  const ToneStep* pat = nullptr;
  switch (tone) {
    case BEEP_HOURLY:     pat = PATTERN_HOURLY;     break;
    case BEEP_OVERHEAD:   pat = PATTERN_OVERHEAD;   break;
    case BEEP_PRECIP:     pat = PATTERN_PRECIP;     break;
    case BEEP_WATCHED:    pat = PATTERN_WATCHED;    break;
    case BEEP_SONAR_PING: pat = PATTERN_SONAR_PING; break;
    case BEEP_EMERGENCY:  pat = PATTERN_EMERGENCY;  break;
    case BEEP_MORSE_SOS:  pat = PATTERN_MORSE_SOS;  break;
    case BEEP_PET_PURR:   pat = PATTERN_PET_PURR;   break;
    case BEEP_PET_CHIRP:  pat = PATTERN_PET_CHIRP;  break;
    case BEEP_PET_SNEEZE: pat = PATTERN_PET_SNEEZE; break;
    case BEEP_TEST:       pat = PATTERN_TEST;       break;
    default: return;
  }

  Serial.printf("[Buzzer] Playing tone %u (forced=%d)\n", (unsigned)tone, (int)(force || tone == BEEP_TEST));

  s_activeTone = tone;
  s_currPattern = pat;
  s_stepIdx = 0;
  s_stepStart = millis();

  TCA9554_SetPin(EXIO_BUZZER, s_currPattern[0].state);
}

void Buzzer_PlayTest() {
  Buzzer_Play(BEEP_TEST, true);
}

void Buzzer_Tick() {
  if (s_activeTone == BEEP_NONE || !s_currPattern) return;

  unsigned long now = millis();
  if (now - s_stepStart >= s_currPattern[s_stepIdx].ms) {
    s_stepIdx = s_stepIdx + 1;
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
