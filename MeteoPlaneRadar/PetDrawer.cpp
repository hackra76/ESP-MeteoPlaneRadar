// =============================================================================
//  MeteoPlaneRadar
//  PetDrawer.cpp - Full-screen Pixel Art DigiCat Autonomous Companion.
// =============================================================================
#include "PetDrawer.h"
#include "PetBrain.h"
#include "Display_ST7701.h"
#include "Settings.h"
#include "UI.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "Lang.h"
#include "QMI8658.h"
#include "Forecast.h"
#include "PrecipTracker.h"
#include "CatSprites.h"
#include <math.h>

static bool s_isOpen = false;
static unsigned long s_lastAnimTick = 0;
static uint32_t s_frameCounter = 0;

// DigiCat Autonomous Animation States
enum CatAnimState : uint8_t {
  CAT_STATE_ENTERING,     // Fast trot onto screen from left or right
  CAT_STATE_IDLE,         // Sitting alert / breathing / blinking / ear twitch
  CAT_STATE_HAPPY,        // Purring, blushing, floating hearts (petted / talked to)
  CAT_STATE_WATCH_PLANE,  // Looking high up and tracking aircraft in sky
  CAT_STATE_EATING,       // Munching crispy fish snack
  CAT_STATE_SLEEPING,     // Curled up sleeping loaf with Zzz
  CAT_STATE_PATROL,       // Strolling to a new spot on the runway deck
  CAT_STATE_JUMP,         // Playful crouch, launch, airborne apex, cushion land!
  CAT_STATE_GROOM,        // Sitting face wash & paw licking
  CAT_STATE_STRETCH,      // Big yoga stretch (front low, butt high, arched back)
  CAT_STATE_LEAVING,      // Decided to leave screen on airfield adventure
  CAT_STATE_AWAY          // Off-screen exploring airfield; returns on tap/call or timer
};

static CatAnimState  s_catState = CAT_STATE_IDLE;
static float         s_catX = 240.0f;
static float         s_catY = 282.0f;
static float         s_catTargetX = 240.0f;
static bool          s_catFlipX = false;
static uint8_t       s_animFrame = 0;
static unsigned long s_nextFrameMs = 0;
static unsigned long s_nextBrainDecisionMs = 12000;
static unsigned long s_autoReturnMs = 0;
static unsigned long s_watchPlaneUntilMs = 0;

// Dynamic Contextual Dialogue
static char          s_customThought[128] = "";
static bool          s_hasCustomThought = false;

// Tactile Petting & Stroking Interaction
static bool          s_isStroking = false;
static unsigned long s_petStrokeUntilMs = 0;
static float         s_petLeanX = 0.0f;
static unsigned long s_lastPurrBeepMs = 0;
static int           s_strokeHeartX = 0;
static int           s_strokeHeartY = 0;
static unsigned long s_lastUserInteractMs = 0;
static unsigned long s_petBounceUntilMs = 0;
static unsigned long s_petFeedUntilMs = 0;
static unsigned long s_nightWakeUntilMs = 0;
static bool          s_isDrowsy = false;
static const unsigned long NIGHT_WAKE_DURATION_MS = 120000; // 2 minutes awake after interaction
static const unsigned long NIGHT_DROWSY_DURATION_MS = 20000; // 20 seconds yawning / slow blinks before sleep

// IMU Reactive Inertia (Tilt)
static float s_imuTiltX = 0.0f;
static float s_imuTiltY = 0.0f;

// Forward declarations
static void callCatBack();

static void setThought(const char* en, const char* sk, const char* cz) {
  const uint8_t lang = Settings_Language();
  const char* src = (lang == LANG_SK) ? sk : ((lang == LANG_CZ) ? cz : en);
  strncpy(s_customThought, src, sizeof(s_customThought) - 1);
  s_customThought[sizeof(s_customThought) - 1] = '\0';
  s_hasCustomThought = true;
}

static const char* getThoughtText() {
  if (s_hasCustomThought && s_customThought[0]) return s_customThought;
  return PetBrain_GetThought();
}

// -----------------------------------------------------------------------------
// Fast Scaled (2x) Palette Blitter with Run-Length Span Acceleration
// -----------------------------------------------------------------------------
static void drawCatSprite2x(const uint8_t* frame, int topLeftX, int topLeftY, bool flipX) {
  if (!frame) return;

  for (int py = 0; py < CAT_SPRITE_H; py++) {
    int screenY = topLeftY + py * 2;
    if (screenY + 1 < 0 || screenY >= LCD_HEIGHT) continue;

    const uint8_t* row = frame + py * CAT_SPRITE_W;
    int px = 0;
    while (px < CAT_SPRITE_W) {
      uint8_t c = row[px];
      if (c == 0 || c >= 16) {
        px++;
        continue; // Transparent pixel
      }

      int startPx = px;
      while (px < CAT_SPRITE_W && row[px] == c) {
        px++;
      }
      int runLen = px - startPx;
      uint16_t color = CAT_PALETTE[c];
      int startX = flipX ? (topLeftX + (CAT_SPRITE_W - px) * 2) : (topLeftX + startPx * 2);
      int w = runLen * 2;

      // Screen clipping
      if (startX < 0) {
        w += startX;
        startX = 0;
      }
      if (startX + w > LCD_WIDTH) {
        w = LCD_WIDTH - startX;
      }

      if (w > 0) {
        gfx->fillRect(startX, screenY, w, 2, color);
      }
    }
  }
}

// -----------------------------------------------------------------------------
// Drawer Lifecycle & Touch Handlers
// -----------------------------------------------------------------------------
bool PetDrawer_IsOpen() {
  return s_isOpen;
}

bool PetDrawer_IsNightAwake() {
  return Settings_IsNight() && (millis() < s_nightWakeUntilMs);
}

void PetDrawer_Open() {
  s_isOpen = true;
  const unsigned long now = millis();
  s_lastUserInteractMs = now;
  s_nextBrainDecisionMs = now + 12000;
  s_hasCustomThought = false;

  bool isSleepingNow = Settings_IsNight() && (now >= s_nightWakeUntilMs);

  if (isSleepingNow) {
    // If it's night and not woken up, cat rests as a sleeping loaf directly in the center
    s_catX = 240.0f;
    s_catTargetX = 240.0f;
    s_catFlipX = false;
    s_catState = CAT_STATE_SLEEPING;
    s_animFrame = 0;
    s_nextFrameMs = now + 400;
  } else {
    // Coming from the sides animation! Random left or right entrance
    bool enterFromLeft = (rand() % 2 == 0);
    s_catX = enterFromLeft ? -70.0f : (LCD_WIDTH + 70.0f);
    s_catTargetX = 240.0f;
    s_catFlipX = !enterFromLeft; // facing towards center
    s_catState = CAT_STATE_ENTERING;
    s_animFrame = 0;
    s_nextFrameMs = now + 75;
  }

  // Contextual thought
  PetBrain_RequestThought(false);
}

void PetDrawer_Close() {
  s_isOpen = false;
  s_catX = 240.0f;
  s_catTargetX = 240.0f;
  s_catState = (Settings_IsNight() && millis() >= s_nightWakeUntilMs) ? CAT_STATE_SLEEPING : CAT_STATE_IDLE;
}

void PetDrawer_Toggle() {
  if (s_isOpen) PetDrawer_Close();
  else PetDrawer_Open();
}

static void callCatBack() {
  if (s_catState != CAT_STATE_AWAY && s_catState != CAT_STATE_LEAVING) return;
  const unsigned long now = millis();

  if (Settings_IsNight()) {
    s_nightWakeUntilMs = now + NIGHT_WAKE_DURATION_MS;
    s_isDrowsy = false;
  }

  if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) {
    Buzzer_Play(BEEP_PET_CHIRP);
  }

  // Choose entrance side: nearest edge
  if (s_catX <= 240.0f) {
    s_catX = -70.0f;
    s_catFlipX = false;
  } else {
    s_catX = LCD_WIDTH + 70.0f;
    s_catFlipX = true;
  }
  s_catTargetX = 240.0f;
  s_catState = CAT_STATE_ENTERING;
  s_animFrame = 0;
  s_nextFrameMs = now + 75; // fast trot zoomies back!
  s_nextBrainDecisionMs = now + 16000;

  setThought(
    "🐾 Mrow! You called? DigiCat zoomies back! <3",
    "🐾 Mňau! Volal si ma? DigiCat letí späť! <3",
    "🐾 Mňau! Volal jsi mě? DigiCat letí zpět! <3"
  );
}

void PetDrawer_HandleTouchMove(int x, int y) {
  if (!s_isOpen) return;
  const unsigned long now = millis();
  s_lastUserInteractMs = now;
  if (Settings_IsNight()) {
    s_nightWakeUntilMs = now + NIGHT_WAKE_DURATION_MS;
    s_isDrowsy = false;
  }

  if (s_catState == CAT_STATE_AWAY || s_catState == CAT_STATE_LEAVING) {
    callCatBack();
    return;
  }

  s_isStroking = true;
  s_petStrokeUntilMs = now + 700;

  // Lean gently toward user's finger
  float targetLean = constrain((float)(x - (int)s_catX) * 0.35f, -16.0f, 16.0f);
  s_petLeanX += (targetLean - s_petLeanX) * 0.35f;

  s_strokeHeartX = x;
  s_strokeHeartY = y - 18;

  // Purring sound while petting
  if (now - s_lastPurrBeepMs > 350) {
    s_lastPurrBeepMs = now;
    if (Settings_BuzzerEnabled()) {
      Buzzer_Play(BEEP_PET_PURR);
    }
  }
}

void PetDrawer_HandleTouchRelease() {
  s_isStroking = false;
}

// -----------------------------------------------------------------------------
// Autonomous Behavior State Machine Tick (~30 FPS)
// -----------------------------------------------------------------------------
bool PetDrawer_Tick() {
  if (!s_isOpen) return false;
  const unsigned long now = millis();

  if (now - s_lastAnimTick < 33) return false;
  s_lastAnimTick = now;
  s_frameCounter++;

  // IMU reading for physical inertia
  if (QMI8658_Available()) {
    QMI_Data imuData;
    QMI8658_GetData(&imuData);
    s_imuTiltX += (-imuData.ax * 12.0f - s_imuTiltX) * 0.15f;
    s_imuTiltY += (imuData.ay * 12.0f - s_imuTiltY) * 0.15f;
  }

  // Decay pet lean when touch released
  if (!s_isStroking && now >= s_petStrokeUntilMs) {
    s_petLeanX *= 0.82f;
  }

  // Airplane tracking check
  float bDeg = 0.0f, dist = 9999.0f;
  char cs[16] = "";
  bool hasPlane = PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs));
  const bool isNight = Settings_IsNight();
  const bool nightAwake = isNight && (now < s_nightWakeUntilMs);
  const bool nightDrowsy = isNight && nightAwake && (s_nightWakeUntilMs - now <= NIGHT_DROWSY_DURATION_MS);

  // Transition into drowsy state before falling asleep
  if (nightDrowsy && !s_isDrowsy) {
    s_isDrowsy = true;
    setThought(
      "🐾 *Yaaawn*... Getting sleepy... zzz",
      "🐾 *Zíííva*... Už sa mi zatvárajú očká... zzz",
      "🐾 *Zííívá*... Už se mi zavírají očka... zzz"
    );
    if (s_catState == CAT_STATE_IDLE) {
      s_catState = CAT_STATE_STRETCH;
      s_animFrame = 0;
      s_nextFrameMs = now + 450;
    }
  }

  // 1. Direct interactive overrides (Feeding & Petting)
  if (now < s_petFeedUntilMs) {
    s_catState = CAT_STATE_EATING;
    s_catTargetX = s_catX;
  } else if (s_isStroking || now < s_petStrokeUntilMs || now < s_petBounceUntilMs) {
    s_catState = CAT_STATE_HAPPY;
    s_catTargetX = s_catX;
  } else if (s_catState == CAT_STATE_ENTERING) {
    // Entering motion in progress: allow DigiCat to trot onto screen before sleeping or idling
  } else if (isNight && !nightAwake) {
    if (s_catState != CAT_STATE_SLEEPING) {
      s_catState = CAT_STATE_SLEEPING;
      s_animFrame = 0;
      s_nextFrameMs = now + 400;
      s_isDrowsy = false;
      setThought(
        "🐾 Purr... Time to sleep... Goodnight! 🌙💤",
        "🐾 Prrrr... Čas ísť spinkať... Dobrú noc! 🌙💤",
        "🐾 Prrrr... Čas jít spát... Dobrou noc! 🌙💤"
      );
    }
    s_catTargetX = s_catX;
  } else if (s_catState == CAT_STATE_AWAY) {
    // Away exploring airfield: auto return after timer if user hasn't called him back
    if (now >= s_autoReturnMs) {
      callCatBack();
    }
    return true;
  } else if (s_catState == CAT_STATE_LEAVING) {
    // Walking off-screen
    float dx = s_catTargetX - s_catX;
    float step = 3.6f;
    if (fabsf(dx) <= step || (s_catTargetX < 0 && s_catX <= -60.0f) || (s_catTargetX > LCD_WIDTH && s_catX >= LCD_WIDTH + 60.0f)) {
      s_catState = CAT_STATE_AWAY;
      s_autoReturnMs = now + 16000 + (rand() % 14000);
    } else {
      s_catX += (dx > 0) ? step : -step;
      s_catFlipX = (dx < 0);
    }
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 90;
      s_animFrame = (s_animFrame + 1) % 8;
    }
    return true;
  } else if (s_catState == CAT_STATE_JUMP) {
    // Jump animation step (8 frames)
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 80;
      s_animFrame++;
      if (s_animFrame >= 8) {
        s_catState = CAT_STATE_IDLE;
        s_animFrame = 0;
        s_nextBrainDecisionMs = now + 7000 + (rand() % 7000);
      }
    }
    return true;
  } else if (s_catState == CAT_STATE_GROOM) {
    // Groom animation step (8 frames)
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 220;
      s_animFrame++;
      if (s_animFrame >= 8) {
        s_catState = CAT_STATE_IDLE;
        s_animFrame = 0;
        s_nextBrainDecisionMs = now + 8000 + (rand() % 7000);
      }
    }
    return true;
  } else if (s_catState == CAT_STATE_STRETCH) {
    // Stretch animation step (6 frames)
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 450;
      s_animFrame++;
      if (s_animFrame >= 6) {
        s_catState = CAT_STATE_IDLE;
        s_animFrame = 0;
        s_nextBrainDecisionMs = now + 8000 + (rand() % 7000);
      }
    }
    return true;
  } else if (s_catState == CAT_STATE_WATCH_PLANE) {
    if (now >= s_watchPlaneUntilMs) {
      s_catState = CAT_STATE_IDLE;
      s_animFrame = 0;
      s_nextBrainDecisionMs = now + 6000 + (rand() % 6000);
    } else {
      if (now >= s_nextFrameMs) {
        s_nextFrameMs = now + 350;
        s_animFrame = (s_animFrame + 1) % 4;
      }
    }
    return true;
  } else if (s_catState == CAT_STATE_ENTERING || s_catState == CAT_STATE_PATROL) {
    // In-motion states
  } else {
    // Cat is idle: check if a new close aircraft suddenly passed over
    static char s_lastCloseCallsign[16] = "";
    if (hasPlane && dist < 18.0f && strcmp(cs, s_lastCloseCallsign) != 0 && (!isNight || nightAwake)) {
      strncpy(s_lastCloseCallsign, cs, sizeof(s_lastCloseCallsign) - 1);
      s_catState = CAT_STATE_WATCH_PLANE;
      s_catFlipX = (bDeg > 180.0f);
      s_watchPlaneUntilMs = now + 3800;
      s_animFrame = 0;
      s_nextFrameMs = now + 350;
      return true;
    }

    s_catState = CAT_STATE_IDLE;

    if (now >= s_nextBrainDecisionMs && now > s_lastUserInteractMs + 5000) {
      if (nightDrowsy) {
        // In drowsy phase, only do gentle wind-down actions (no jumping or leaving screen)
        int drowsyRoll = rand() % 100;
        if (drowsyRoll < 50) {
          s_catState = CAT_STATE_STRETCH;
          s_animFrame = 0;
          s_nextFrameMs = now + 450;
        } else {
          s_catState = CAT_STATE_GROOM;
          s_animFrame = 0;
          s_nextFrameMs = now + 240;
        }
        s_nextBrainDecisionMs = now + 7000;
      } else {
        int roll = rand() % 100;

        // If a plane is nearby, 20% chance to look up at it
        if (hasPlane && dist < 50.0f && roll < 20) {
          s_catState = CAT_STATE_WATCH_PLANE;
          s_catFlipX = (bDeg > 180.0f);
          s_watchPlaneUntilMs = now + 3500;
          s_animFrame = 0;
          s_nextFrameMs = now + 350;
        } else if (roll < 42) {
          // 1. Patrol to a new spot on the runway deck (160..320)
          float targetX = 160.0f + (float)(rand() % 160);
          s_catTargetX = targetX;
          s_catFlipX = (targetX < s_catX);
          s_catState = CAT_STATE_PATROL;
          s_animFrame = 0;
          s_nextFrameMs = now + 90;
        } else if (roll < 60) {
          // 2. Playful Jump / Pounce
          s_catState = CAT_STATE_JUMP;
          s_animFrame = 0;
          s_nextFrameMs = now + 80;
          if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) {
            Buzzer_Play(BEEP_PET_CHIRP);
          }
        } else if (roll < 74) {
          // 3. Groom / Face Wash
          s_catState = CAT_STATE_GROOM;
          s_animFrame = 0;
          s_nextFrameMs = now + 220;
        } else if (roll < 86) {
          // 4. Big Cat Stretch
          s_catState = CAT_STATE_STRETCH;
          s_animFrame = 0;
          s_nextFrameMs = now + 450;
        } else {
          // 5. Explore airfield! (Leaves screen)
          bool leaveRight = (s_catX > 240.0f);
          s_catTargetX = leaveRight ? (LCD_WIDTH + 70.0f) : -70.0f;
          s_catFlipX = (s_catTargetX < s_catX);
          s_catState = CAT_STATE_LEAVING;
          s_animFrame = 0;
          s_nextFrameMs = now + 90;

          int dep = rand() % 3;
          if (dep == 0) {
            setThought(
              "🐾 DigiCat went exploring the hangar... Tap anywhere to call!",
              "🐾 DigiCat išiel preskúmať hangár... Ťuknite pre privolanie!",
              "🐾 DigiCat šel prozkoumat hangár... Klepněte pro přivolání!"
            );
          } else if (dep == 1) {
            setThought(
              "🐾 Chasing a moth near runway 22... Tap anywhere to call!",
              "🐾 Naháňam moľa pri vzletovej dráhe... Ťukni pre privolanie!",
              "🐾 Honička za můrou u vzletové dráhy... Klepni pro přivolání!"
            );
          } else {
            setThought(
              "🐾 Gone to inspect the radar tower... Tap anywhere to call!",
              "🐾 Idem skontrolovať radarovú vežu... Ťukni pre privolanie!",
              "🐾 Jdu zkontrolovat radarovou věž... Klepni pro přivolání!"
            );
          }
        }
      }
    }
  }

  // Movement & Walk animation
  if (s_catState == CAT_STATE_ENTERING || s_catState == CAT_STATE_PATROL) {
    float dx = s_catTargetX - s_catX;
    float step = (s_catState == CAT_STATE_ENTERING) ? 5.2f : 3.2f;
    if (fabsf(dx) <= step) {
      s_catX = s_catTargetX;
      s_catState = (isNight && !nightAwake) ? CAT_STATE_SLEEPING : CAT_STATE_IDLE;
      s_animFrame = 0;
      s_nextBrainDecisionMs = now + 8000 + (rand() % 8000);
      if (s_hasCustomThought) {
        s_hasCustomThought = false; // resume normal thoughts
      }
    } else {
      s_catX += (dx > 0) ? step : -step;
      s_catFlipX = (dx < 0);
    }
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + (s_catState == CAT_STATE_ENTERING ? 75 : 90);
      s_animFrame = (s_animFrame + 1) % 8;
    }
  } else if (s_catState == CAT_STATE_EATING) {
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 140;
      s_animFrame = (s_animFrame + 1) % 8;
    }
  } else if (s_catState == CAT_STATE_HAPPY) {
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 110;
      s_animFrame = (s_animFrame + 1) % 8;
    }
  } else if (s_catState == CAT_STATE_SLEEPING) {
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 400;
      s_animFrame = (s_animFrame + 1) % 8;
    }
  } else { // CAT_STATE_IDLE
    if (now >= s_nextFrameMs) {
      if (s_animFrame >= 4 && s_animFrame <= 6) {
        s_animFrame = (s_animFrame == 6) ? 0 : s_animFrame + 1;
        s_nextFrameMs = now + (nightDrowsy ? 280 : 140);
      } else if (s_animFrame == 7) {
        s_animFrame = 0;
        s_nextFrameMs = now + 350;
      } else {
        s_animFrame = (s_animFrame + 1) % 4;
        if (s_animFrame == 0) {
          int roll = rand() % 100;
          if (nightDrowsy || roll < 25) {
            s_animFrame = 4; // slow blink
            s_nextFrameMs = now + (nightDrowsy ? 280 : 140);
          } else if (roll < 40) {
            s_animFrame = 7; // ear flick
            s_nextFrameMs = now + 260;
          } else {
            s_nextFrameMs = now + (nightDrowsy ? 500 : 350);
          }
        } else {
          s_nextFrameMs = now + (nightDrowsy ? 500 : 350);
        }
      }
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
// Visual Helpers
// -----------------------------------------------------------------------------
static void drawSpeechBubble(int bx, int by, int bw, int bh, const char* text) {
  gfx->fillRoundRect(bx, by, bw, bh, 14, 0x0821);
  gfx->drawRoundRect(bx, by, bw, bh, 14, C_CYAN);

  const int tx = bx + bw / 2;
  const int ty = by + bh;
  gfx->fillTriangle(tx - 8, ty - 1, tx + 8, ty - 1, tx, ty + 10, 0x0821);
  gfx->drawLine(tx - 8, ty, tx, ty + 10, C_CYAN);
  gfx->drawLine(tx + 8, ty, tx, ty + 10, C_CYAN);

  if (!text || !*text) return;

  char line[64];
  const char* p = text;
  int ly = by + 10;
  int lineCount = 0;

  while (*p && lineCount < 3) {
    while (*p == ' ') p++;
    if (!*p) break;

    int len = 0;
    int lastSpace = -1;
    while (p[len] && len < 34) {
      if (p[len] == ' ') lastSpace = len;
      len++;
    }

    int copyLen = len;
    if (p[len] && lastSpace > 0) {
      copyLen = lastSpace;
    } else {
      while (copyLen > 0 && ((unsigned char)p[copyLen] & 0xC0) == 0x80) {
        copyLen--;
      }
    }

    if (copyLen > (int)sizeof(line) - 1) copyLen = sizeof(line) - 1;
    strncpy(line, p, copyLen);
    line[copyLen] = '\0';
    UI_TextCenteredIn(line, bx, bw, ly, C_WHITE, 1);
    ly += 16;
    lineCount++;
    p += copyLen;
  }
}

static void drawHeart(int cx, int cy, int size, uint16_t color) {
  if (size < 4) return;
  int r = size / 2;
  gfx->fillCircle(cx - r / 2, cy - r / 4, r / 2, color);
  gfx->fillCircle(cx + r / 2, cy - r / 4, r / 2, color);
  gfx->fillTriangle(cx - size / 2, cy - r / 4, cx + size / 2, cy - r / 4, cx, cy + size / 2 + 1, color);
}

static void drawRainStreaks() {
  const unsigned long now = millis();
  for (int i = 0; i < 9; i++) {
    int rx = 42 + i * 44;
    int ry = ((int)(now / 2) + i * 61) % 460;
    gfx->drawLine(rx, ry, rx + 1, ry + 10, 0x2318);
  }
}

// -----------------------------------------------------------------------------
// Main Render Pass
// -----------------------------------------------------------------------------
void PetDrawer_Draw() {
  if (!s_isOpen) return;

  gfx->fillScreen(C_BLACK);

  // 1. Clock & Outside Temperature status line
  UI_DrawStatusLine(36);

  // 2. Pet Title Pill with Aviation Rank & Mute Toggle
  char titleBuf[64];
  snprintf(titleBuf, sizeof(titleBuf), "🐾 DIGICAT [%s]", PetBrain_GetStageTitle());
  const int titleW = 254;
  const int titleX = 85;
  const int titleY = 58;
  gfx->fillRoundRect(titleX, titleY, titleW, 22, 11, 0x10A2);
  gfx->drawRoundRect(titleX, titleY, titleW, 22, 11, C_CYAN);
  UI_TextCenteredIn(titleBuf, titleX, titleW, titleY + 4, C_WHITE, 1);

  // Mute / Sound Toggle Button
  const int muteX = 348;
  const int muteY = 58;
  const int muteW = 50;
  const int muteH = 22;
  bool petSound = Settings_BuzzerPet();
  gfx->fillRoundRect(muteX, muteY, muteW, muteH, 11, petSound ? 0x0821 : 0x4800);
  gfx->drawRoundRect(muteX, muteY, muteW, muteH, 11, petSound ? C_CYAN : C_RED);
  UI_TextCenteredIn(petSound ? "VOL" : "MUTE", muteX, muteW, muteY + 4, petSound ? C_WHITE : C_YELLOW, 1);

  // 3. Virtual Pet Stats Badge
  PetStats stats = PetBrain_GetStats();
  const uint8_t curLang = Settings_Language();
  char statBuf[64];
  if (curLang == LANG_SK) {
    snprintf(statBuf, sizeof(statBuf), "RADOSŤ: %d%%  •  HLAD: %d%%  •  XP: %u",
             stats.happiness, stats.hunger, stats.flightsTracked);
  } else if (curLang == LANG_CZ) {
    snprintf(statBuf, sizeof(statBuf), "RADOST: %d%%  •  HLAD: %d%%  •  XP: %u",
             stats.happiness, stats.hunger, stats.flightsTracked);
  } else {
    snprintf(statBuf, sizeof(statBuf), "HAPPY: %d%%  •  HUNGER: %d%%  •  XP: %u",
             stats.happiness, stats.hunger, stats.flightsTracked);
  }
  UI_TextCentered(statBuf, 84, 0x07E0, 1);

  // 4. Speech Bubble
  drawSpeechBubble(60, 98, 360, 68, getThoughtText());

  // 5. Environmental Rain Streaks
  bool hasRain = (Forecast_CurrentValid() && ((Forecast_CurrentCode() >= 50 && Forecast_CurrentCode() <= 69) ||
                                            (Forecast_CurrentCode() >= 80 && Forecast_CurrentCode() <= 82))) ||
                 PrecipTracker_IsApproaching() || PrecipTracker_IsCurrentlyActive();
  if (hasRain) {
    drawRainStreaks();
  }

  // 6. Draw DigiCat Pixel Art Sprite (or Away Radar Beacon & Call Button)
  const unsigned long now = millis();
  if (s_catState == CAT_STATE_AWAY) {
    // Concentric radar beacon pulse on deck
    int pulseR = 12 + (int)((now / 50) % 24);
    gfx->drawCircle(240, 250, pulseR, 0x18A2);
    gfx->drawCircle(240, 250, pulseR / 2, 0x2124);
    gfx->fillCircle(240, 250, 4, C_CYAN);

    // Call DigiCat button (prominent and clickable anywhere on deck)
    const int callX = 125;
    const int callY = 280;
    const int callW = 230;
    const int callH = 38;
    bool pulse = ((now / 450) % 2 == 0);
    gfx->fillRoundRect(callX, callY, callW, callH, 19, pulse ? 0x0320 : 0x0200);
    gfx->drawRoundRect(callX, callY, callW, callH, 19, pulse ? 0x07E0 : C_CYAN);
    const char* callTxt = (curLang == LANG_SK) ? "🐾 ZAVOLAŤ DIGICAT"
                        : ((curLang == LANG_CZ) ? "🐾 ZAVOLAT DIGICAT"
                                                : "🐾 CALL DIGICAT");
    UI_TextCenteredIn(callTxt, callX, callW, callY + 11, C_WHITE, 1);

    const char* awayHint = (curLang == LANG_SK) ? "( Ťuknite kdekoľvek pre privolanie )"
                          : ((curLang == LANG_CZ) ? "( Klepněte kdekoli pro přivolání )"
                                                  : "( Tap anywhere to call pet back )");
    UI_TextCentered(awayHint, 330, 0x632C, 1);
  } else {
    // Screen clamping only when sitting or patrolling on-screen
    if (s_catState != CAT_STATE_ENTERING && s_catState != CAT_STATE_LEAVING) {
      s_catX = constrain(s_catX, 140.0f, 340.0f);
    }

    const uint8_t* frameToDraw = nullptr;
    if (s_catState == CAT_STATE_ENTERING || s_catState == CAT_STATE_PATROL || s_catState == CAT_STATE_LEAVING) {
      frameToDraw = CAT_WALK_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_JUMP) {
      frameToDraw = CAT_JUMP_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_GROOM) {
      frameToDraw = CAT_GROOM_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_STRETCH) {
      frameToDraw = CAT_STRETCH_FRAMES[s_animFrame % 6];
    } else if (s_catState == CAT_STATE_EATING) {
      frameToDraw = CAT_EAT_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_HAPPY) {
      frameToDraw = CAT_HAPPY_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_SLEEPING) {
      frameToDraw = CAT_SLEEP_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_WATCH_PLANE) {
      frameToDraw = CAT_WATCH_FRAMES[s_animFrame % 4];
    } else {
      frameToDraw = CAT_IDLE_FRAMES[s_animFrame % 8];
    }

    if (!frameToDraw) {
      frameToDraw = CAT_IDLE_FRAMES[0];
    }

    int catDrawX = (int)(s_catX + s_imuTiltX * 0.5f + s_petLeanX) - 64;
    int catDrawY = (int)(s_catY + s_imuTiltY * 0.5f) - 64;

    // Jump vertical curve & walk bob
    if (s_catState == CAT_STATE_JUMP) {
      static const int8_t s_jumpY[8] = { 4, 6, -14, -26, -32, -20, -2, 4 };
      catDrawY += s_jumpY[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_ENTERING || s_catState == CAT_STATE_PATROL || s_catState == CAT_STATE_LEAVING) {
      static const int8_t s_walkBob[8] = { 0, -2, -1, 0, 0, -2, -1, 0 };
      catDrawY += s_walkBob[s_animFrame % 8];
    }

    drawCatSprite2x(frameToDraw, catDrawX, catDrawY, s_catFlipX);
  }

  // 7. Floating Hearts when Happy or Petted
  if (s_catState == CAT_STATE_HAPPY || s_isStroking || now < s_petStrokeUntilMs || now < s_petBounceUntilMs) {
    float t1 = (float)(now % 1200) / 1200.0f;
    int h1_y = (int)s_catY - 45 - (int)(t1 * 50.0f);
    int h1_x = (int)s_catX - 65 + (int)(sinf(t1 * (float)M_PI * 2.0f) * 10.0f);
    drawHeart(h1_x, h1_y, 14, 0xF814);

    float t2 = (float)((now + 400) % 1200) / 1200.0f;
    int h2_y = (int)s_catY - 55 - (int)(t2 * 55.0f);
    int h2_x = (int)s_catX + 65 + (int)(cosf(t2 * (float)M_PI * 2.0f) * 10.0f);
    drawHeart(h2_x, h2_y, 16, 0xFD20);

    if (s_isStroking && s_strokeHeartX > 0) {
      drawHeart(s_strokeHeartX, s_strokeHeartY, 14, 0xF814);
    }
  }

  // 8. Dual Action Buttons (FEED TREAT & PET/TALK)
  const int btnY = 394;
  const int btnH = 28;
  const int feedX = 90;
  const int feedW = 140;
  gfx->fillRoundRect(feedX, btnY, feedW, btnH, 14, (now < s_petFeedUntilMs) ? 0x0400 : 0x1084);
  gfx->drawRoundRect(feedX, btnY, feedW, btnH, 14, 0xFDC0);
  UI_TextCenteredIn("🍖 FEED TREAT", feedX, feedW, btnY + 7, 0xFDC0, 1);

  const int petBtnX = 250;
  const int petBtnW = 140;
  gfx->fillRoundRect(petBtnX, btnY, petBtnW, btnH, 14, (now < s_petBounceUntilMs) ? 0x8000 : 0x1084);
  gfx->drawRoundRect(petBtnX, btnY, petBtnW, btnH, 14, C_CYAN);
  UI_TextCenteredIn("👋 PET / TALK", petBtnX, petBtnW, btnY + 7, C_WHITE, 1);

  // 9. Gesture Close Hint
  const char* hintTxt = (Lang_Get() == LANG_EN) ? "v swipe down from top to close v"
                       : ((Lang_Get() == LANG_SK) ? "v potiahnutim zhora zatvorte v"
                                                  : "v potazenim shora zavrete v");
  UI_TextCentered(hintTxt, 442, 0x632C, 1);
}

// -----------------------------------------------------------------------------
// Tap Handlers
// -----------------------------------------------------------------------------
bool PetDrawer_HandleTap(int x, int y) {
  if (!s_isOpen) return false;
  const unsigned long now = millis();
  s_lastUserInteractMs = now;
  if (Settings_IsNight()) {
    s_nightWakeUntilMs = now + NIGHT_WAKE_DURATION_MS;
    s_isDrowsy = false;
  }

  // Tap at top edge closes drawer
  if (y <= 48) {
    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
    PetDrawer_Close();
    return true;
  }

  // If the cat is away or leaving, ANY tap summons him back immediately!
  if (s_catState == CAT_STATE_AWAY || s_catState == CAT_STATE_LEAVING) {
    callCatBack();
    return true;
  }

  // If the cat was sleeping, wake up with a stretch!
  if (s_catState == CAT_STATE_SLEEPING) {
    s_catState = CAT_STATE_STRETCH;
    s_animFrame = 0;
    s_nextFrameMs = now + 400;
    s_nextBrainDecisionMs = now + 7000;
    if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) {
      Buzzer_Play(BEEP_PET_PURR);
    }
    setThought(
      "🐾 *Yawn*... Oh, hello! DigiCat woke up! 😸",
      "🐾 *Zív*... Jéé, ahoj! DigiCat sa zobudil! 😸",
      "🐾 *Zív*... Jéé, ahoj! DigiCat se vzbudil! 😸"
    );
  }

  // Tap on Mute button
  if (y >= 50 && y <= 84 && x >= 344 && x <= 405) {
    bool newState = !Settings_BuzzerPet();
    Settings_SetBuzzerPet(newState);
    if (newState) {
      if (Settings_BuzzerEnabled()) Buzzer_Play(BEEP_PET_CHIRP);
      setThought(
        "🐾 Sound enabled! Purrrr! 😺",
        "🐾 Zvuky zapnuté! Prrrrr! 😺",
        "🐾 Zvuky zapnuty! Prrrrr! 😺"
      );
    } else {
      if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
      setThought(
        "🐾 Quiet mode active... Shhh! 🤫",
        "🐾 Tichý režim aktívny... Pst! 🤫",
        "🐾 Tichý režim aktivní... Pst! 🤫"
      );
    }
    return true;
  }

  // Tap on Title Pill refreshes thought & chirps
  if (y >= 52 && y <= 80 && x >= 80 && x < 344) {
    if (Settings_BuzzerTouch() && Settings_BuzzerPet()) Buzzer_Play(BEEP_PET_CHIRP);
    PetBrain_RequestThought(false);
    return true;
  }

  // Tap on speech bubble refreshes thought
  if (y >= 96 && y <= 170) {
    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
    PetBrain_RequestThought(false);
    return true;
  }

  // Bottom action buttons
  if (y >= 385 && y <= 430) {
    // Feed Treat
    if (x >= 80 && x <= 235) {
      if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
      s_petFeedUntilMs = now + 2400;
      s_catTargetX = s_catX; // halt walk if moving
      PetBrain_Feed();
      return true;
    }
    // Pet / Talk
    if (x >= 245 && x <= 400) {
      if (Settings_BuzzerEnabled()) Buzzer_Play(BEEP_PET_PURR);
      s_petBounceUntilMs = now + 1800;
      s_catTargetX = s_catX;
      PetBrain_RequestThought(true);
      return true;
    }
  }

  // Tap directly on Cat area
  if (y >= 175 && y < 385) {
    if (Settings_BuzzerEnabled()) Buzzer_Play(BEEP_PET_PURR);
    s_petBounceUntilMs = now + 1800;
    s_catTargetX = s_catX;
    PetBrain_RequestThought(true);
    return true;
  }

  return true;
}
