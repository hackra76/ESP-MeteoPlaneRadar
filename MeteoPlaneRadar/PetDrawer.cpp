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
  CAT_STATE_SWAT,         // Swatting & scratching upward at overhead aircraft!
  CAT_STATE_LEAVING,      // Decided to leave screen on airfield adventure
  CAT_STATE_AWAY,         // Off-screen exploring airfield; returns on tap/call or timer
  CAT_STATE_SLIDE,        // Gentle tilt fun slide/surf across runway!
  CAT_STATE_TUMBLE,       // Steep tilt tumbling, scrambling claws & rolling!
  CAT_STATE_CLING         // Extreme tilt Option A: Clinging to circular bezel rim!
};

struct TrackedPlaneDisplay {
  bool active = false;
  float x = 0.0f;
  float y = 186.0f;
  float vx = 1.6f;
  float distKm = 0.0f;
  float bearingDeg = 0.0f;
  char callsign[16] = "";
  bool evasiveHop = false;
  unsigned long hopEndMs = 0;
};
static TrackedPlaneDisplay s_skyPlane;

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
static unsigned long s_planeChasePauseUntilMs = 0;

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

// IMU Reactive Inertia & Physics Tilt Interactions
static float         s_imuTiltX = 0.0f;
static float         s_imuTiltY = 0.0f;
static float         s_catVx = 0.0f;
static uint8_t       s_extremeOption = 0; // 0 = none, 1 = Option A Cling, 2 = Option B Slide Away
static bool          s_clingLeft = false;
static unsigned long s_lastTiltSfxMs = 0;
static unsigned long s_clingStartMs = 0;
static unsigned long s_tiltLevelTimeMs = 0;

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

static void drawAirplaneSprite2x(int topLeftX, int topLeftY, bool flipX) {
  for (int py = 0; py < CAT_AIRPLANE_H; py++) {
    int screenY = topLeftY + py * 2;
    if (screenY + 1 < 0 || screenY >= LCD_HEIGHT) continue;

    const uint8_t* row = CAT_AIRPLANE + py * CAT_AIRPLANE_W;
    int px = 0;
    while (px < CAT_AIRPLANE_W) {
      uint8_t c = row[px];
      if (c == 0 || c >= 16) {
        px++;
        continue;
      }

      int startPx = px;
      while (px < CAT_AIRPLANE_W && row[px] == c) {
        px++;
      }
      int runLen = px - startPx;
      uint16_t color = CAT_PALETTE[c];
      int startX = flipX ? (topLeftX + (CAT_AIRPLANE_W - px) * 2) : (topLeftX + startPx * 2);
      int w = runLen * 2;

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

  s_catVx = 0.0f;
  s_extremeOption = 0;
  s_tiltLevelTimeMs = 0;

  // Contextual thought
  PetBrain_RequestThought(false);
}

void PetDrawer_Close() {
  s_isOpen = false;
  s_catX = 240.0f;
  s_catTargetX = 240.0f;
  s_catVx = 0.0f;
  s_extremeOption = 0;
  s_tiltLevelTimeMs = 0;
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

  // IMU reading for physical inertia & dynamic tilt interactions
  float tiltG = 0.0f;
  float absTilt = 0.0f;
  if (QMI8658_Available()) {
    QMI_Data imuData;
    QMI8658_GetData(&imuData);
    tiltG = -imuData.ay; // Lateral roll: Negative = tilted left, Positive = tilted right
    absTilt = fabsf(tiltG);
    s_imuTiltX += (tiltG * 12.0f - s_imuTiltX) * 0.15f;
    s_imuTiltY += (imuData.ax * 12.0f - s_imuTiltY) * 0.15f; // Vertical pitch
  }

  // Dynamic IMU Tilt Physics & Reactions
  if (absTilt < 0.14f) {
    // Device held relatively level/flat
    if (s_tiltLevelTimeMs == 0) s_tiltLevelTimeMs = now;

    // Decay slide/tumble inertia
    if (s_catState == CAT_STATE_SLIDE || s_catState == CAT_STATE_TUMBLE) {
      s_catVx *= 0.78f;
      s_catX += s_catVx;
      s_catX = constrain(s_catX, 100.0f, 380.0f);
      if (fabsf(s_catVx) < 0.25f) {
        s_catVx = 0.0f;
        s_catState = CAT_STATE_IDLE;
        s_animFrame = 0;
        s_nextFrameMs = now + 140;
        s_nextBrainDecisionMs = now + 5000;
        s_extremeOption = 0;
        s_hasCustomThought = false;
      }
    } else if (s_catState == CAT_STATE_CLING) {
      // Option A recovery: held level for 600ms -> hoist back onto runway!
      if (now - s_tiltLevelTimeMs >= 600) {
        s_catState = CAT_STATE_JUMP;
        s_catX = s_clingLeft ? 150.0f : 330.0f;
        s_catVx = 0.0f;
        s_extremeOption = 0;
        s_animFrame = 0;
        s_nextFrameMs = now + 80;
        s_nextBrainDecisionMs = now + 6000;
        if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_PET_CHIRP);
        setThought(
          "🐾 *PHEW!* Leveled out! Hopped back onto the runway! 😸",
          "🐾 *UF!* Už je to rovno! Vyskočil som späť na dráhu! 😸",
          "🐾 *UF!* Už je to rovně! Vyskočil jsem zpět na dráhu! 😸"
        );
      }
    } else if (s_catState == CAT_STATE_AWAY && s_extremeOption == 2) {
      // Option B recovery: held level for 1800ms -> trot back onto deck!
      if (now - s_tiltLevelTimeMs >= 1800) {
        callCatBack();
        s_extremeOption = 0;
      }
    }
  } else {
    // Device is actively tilted
    s_tiltLevelTimeMs = 0;

    // Check for extreme tilt or falling off deck edge
    if (absTilt >= 0.68f || ((s_catX <= 75.0f || s_catX >= 405.0f) && absTilt >= 0.35f)) {
      if (s_catState != CAT_STATE_CLING && s_catState != CAT_STATE_AWAY && s_catState != CAT_STATE_LEAVING) {
        if (s_extremeOption == 0) {
          s_extremeOption = (rand() % 2) + 1; // 1 = Option A Cling, 2 = Option B Slide Away
        }
        if (s_extremeOption == 1) {
          // Option A: Cling to circular bezel rim by claws!
          s_catState = CAT_STATE_CLING;
          s_clingLeft = (s_catX < 240.0f);
          s_catX = s_clingLeft ? 52.0f : 428.0f;
          s_catVx = 0.0f;
          s_clingStartMs = now;
          if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_EMERGENCY);
          setThought(
            "🙀 *HOLD ME FLAT!* Clinging to the rim for dear life! Slipping! 🐾",
            "🙀 *DRŽ MA ROVNO!* Držím sa okraja! Šmýkam sa dole! 🐾",
            "🙀 *DRŽ MĚ ROVNĚ!* Držím se okraje! Kloužu dolů! 🐾"
          );
        } else {
          // Option B: Slide completely off the screen edge into AWAY
          s_catState = CAT_STATE_AWAY;
          s_autoReturnMs = now + 25000;
          s_catVx = 0.0f;
          if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_OVERHEAD);
          setThought(
            "🐾 *WHEEE-WHOOPS!* Slid right off the deck! Level device to call back! 🍂",
            "🐾 *FÍÍÍ-AU!* Skĺzol som z dráhy! Vyrovnaj displej pre privolanie! 🍂",
            "🐾 *FÍÍÍ-AU!* Sklouzl jsem z dráhy! Srovnej displej pro přivolání! 🍂"
          );
        }
      }
    } else if (absTilt >= 0.42f) {
      // Big Tilt: Scared tumbling & scrambling claws!
      if (s_catState != CAT_STATE_CLING && s_catState != CAT_STATE_AWAY && s_catState != CAT_STATE_LEAVING) {
        s_catState = CAT_STATE_TUMBLE;
        s_catFlipX = (tiltG < 0);
        s_catVx += tiltG * 2.8f;
        s_catVx = constrain(s_catVx, -9.5f, 9.5f);
        s_catX += s_catVx;
        s_catX = constrain(s_catX, 70.0f, 410.0f);
        PetBrain_SetMood(PET_MOOD_SCARED);
        if (now - s_lastTiltSfxMs >= 1600) {
          s_lastTiltSfxMs = now;
          if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_WATCHED);
          if (!s_hasCustomThought) {
            setThought(
              "🙀 *WHOAAAA!* Too steep! DigiCat is tumbling out of control! 🌪️🐾",
              "🙀 *ÁÁÁÁÁ!* Príliš strmé! Kotúľam sa a strácam rovnováhu! 🌪️🐾",
              "🙀 *ÁÁÁÁÁ!* Příliš strmé! Kutálím se a ztrácím rovnováhu! 🌪️🐾"
            );
          }
        }
      }
    } else {
      // Small Tilt (0.15f <= absTilt < 0.42f): Happy sliding / surfing!
      if (s_catState == CAT_STATE_SLEEPING) {
        s_catX += tiltG * 1.2f;
        s_catX = constrain(s_catX, 100.0f, 380.0f);
      } else if (s_catState != CAT_STATE_CLING && s_catState != CAT_STATE_AWAY && s_catState != CAT_STATE_LEAVING) {
        s_catState = CAT_STATE_SLIDE;
        s_catFlipX = (tiltG < 0);
        s_catVx += tiltG * 1.6f;
        s_catVx = constrain(s_catVx, -5.2f, 5.2f);
        s_catX += s_catVx;
        s_catX = constrain(s_catX, 70.0f, 410.0f);
        if (now - s_lastTiltSfxMs >= 2000) {
          s_lastTiltSfxMs = now;
          PetBrain_SetMood(PET_MOOD_HAPPY);
          if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_PET_PURR);
          if (!s_hasCustomThought) {
            setThought(
              "🐾 Wheee! Fun sliding! DigiCat is surfing the deck! 🏄‍♂️✨",
              "🐾 Jéééj! To sa parádne kĺže! DigiCat surfuje po dráhe! 🏄‍♂️✨",
              "🐾 Jéééj! To to hezky klouže! DigiCat surfuje po dráze! 🏄‍♂️✨"
            );
          }
        }
      }
    }
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

  // Tracked Aircraft simulation in pet screen: only when very close (<= 10km)
  if (hasPlane && dist <= 10.0f && (!isNight || nightAwake)) {
    s_skyPlane.active = true;
    s_skyPlane.distKm = dist;
    s_skyPlane.bearingDeg = bDeg;
    strncpy(s_skyPlane.callsign, cs, sizeof(s_skyPlane.callsign) - 1);
    s_skyPlane.callsign[sizeof(s_skyPlane.callsign) - 1] = '\0';

    if (s_skyPlane.vx == 0.0f) {
      if (bDeg > 180.0f) {
        s_skyPlane.x = 410.0f;
        s_skyPlane.vx = -1.6f;
      } else {
        s_skyPlane.x = 50.0f;
        s_skyPlane.vx = 1.6f;
      }
    }

    s_skyPlane.x += s_skyPlane.vx;
    if (s_skyPlane.vx > 0 && s_skyPlane.x > 410.0f) {
      s_skyPlane.vx = -1.6f;
    } else if (s_skyPlane.vx < 0 && s_skyPlane.x < 50.0f) {
      s_skyPlane.vx = 1.6f;
    }

    float baseY = 186.0f + sinf((float)now * 0.003f) * 3.5f;
    if (s_skyPlane.evasiveHop && now < s_skyPlane.hopEndMs) {
      baseY -= 10.0f;
    } else {
      s_skyPlane.evasiveHop = false;
    }
    s_skyPlane.y = baseY;

    // Cat reaction: chase or swat only when active, not in autonomous routine, and pause expired
    if (s_catState != CAT_STATE_SLEEPING && s_catState != CAT_STATE_AWAY &&
        s_catState != CAT_STATE_LEAVING && s_catState != CAT_STATE_EATING &&
        s_catState != CAT_STATE_HAPPY && s_catState != CAT_STATE_ENTERING &&
        s_catState != CAT_STATE_GROOM && s_catState != CAT_STATE_STRETCH &&
        s_catState != CAT_STATE_WATCH_PLANE &&
        s_catState != CAT_STATE_SLIDE && s_catState != CAT_STATE_TUMBLE &&
        s_catState != CAT_STATE_CLING &&
        now >= s_planeChasePauseUntilMs) {
      float catDist = fabsf(s_catX - (s_skyPlane.x + 28.0f));

      if (catDist <= 32.0f) {
        if (s_catState != CAT_STATE_SWAT && s_catState != CAT_STATE_JUMP) {
          s_catState = CAT_STATE_SWAT;
          s_catFlipX = (s_skyPlane.vx < 0);
          s_animFrame = 0;
          s_nextFrameMs = now + 110;
        }
      } else {
        if (s_catState != CAT_STATE_JUMP && s_catState != CAT_STATE_SWAT) {
          s_catTargetX = constrain(s_skyPlane.x + 28.0f, 140.0f, 340.0f);
          s_catFlipX = (s_catTargetX < s_catX);
          if (s_catState != CAT_STATE_PATROL) {
            s_catState = CAT_STATE_PATROL;
            s_animFrame = 0;
            s_nextFrameMs = now + 80;
          }
        }
      }
    }
  } else {
    s_skyPlane.active = false;
    s_skyPlane.vx = 0.0f;
    if (s_catState == CAT_STATE_SWAT) {
      s_catState = CAT_STATE_IDLE;
    }
  }

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
  } else if (s_catState == CAT_STATE_SWAT) {
    // Swatting & scratching overhead aircraft
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 110;
      s_animFrame++;

      // Claw apex swipe: trigger plane evasive hop
      if ((s_animFrame % 8 == 2 || s_animFrame % 8 == 5) && s_skyPlane.active) {
        s_skyPlane.evasiveHop = true;
        s_skyPlane.hopEndMs = now + 450;
        if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) {
          Buzzer_Play(BEEP_PET_CHIRP);
        }
      }

      // If plane drifted out of swat range, resume chase
      if (s_skyPlane.active && fabsf(s_catX - (s_skyPlane.x + 28.0f)) > 38.0f) {
        s_catTargetX = constrain(s_skyPlane.x + 28.0f, 140.0f, 340.0f);
        s_catFlipX = (s_catTargetX < s_catX);
        s_catState = CAT_STATE_PATROL;
        s_animFrame = 0;
        s_nextFrameMs = now + 80;
        return true;
      }

      // After 2 swat cycles (16 frames), chance for a playful jump at the aircraft or a rest
      if (s_animFrame >= 16) {
        PetBrain_AwardXP(1);
        if (rand() % 100 < 40 && s_skyPlane.active) {
          s_catState = CAT_STATE_JUMP;
          s_animFrame = 0;
          s_nextFrameMs = now + 80;
        } else {
          s_animFrame = 0;
          s_catState = CAT_STATE_IDLE;
          s_planeChasePauseUntilMs = now + 5000;
          s_nextBrainDecisionMs = now + 5000;
        }
      }
    }
    return true;
  } else if (s_catState == CAT_STATE_JUMP) {
    // Jump animation step (8 frames)
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 80;
      s_animFrame++;
      if ((s_animFrame == 3 || s_animFrame == 4) && s_skyPlane.active) {
        s_skyPlane.evasiveHop = true;
        s_skyPlane.hopEndMs = now + 500;
      }
      if (s_animFrame >= 8) {
        s_catState = CAT_STATE_IDLE;
        s_animFrame = 0;
        s_planeChasePauseUntilMs = now + 4500;
        s_nextBrainDecisionMs = now + 4500;
      }
    }
    return true;
  } else if (s_catState == CAT_STATE_GROOM) {
    // Groom animation step (8 frames)
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 220;
      s_animFrame++;
      if (s_animFrame == 4 && (rand() % 100 < 35)) {
        if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_PET_SNEEZE);
      }
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
  } else if (s_catState == CAT_STATE_SLIDE || s_catState == CAT_STATE_TUMBLE || s_catState == CAT_STATE_CLING) {
    return true; // Active tilt physical response
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

    if (now >= s_nextBrainDecisionMs && now > s_lastUserInteractMs + 5000 && absTilt < 0.15f) {
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
    float step = (s_catState == CAT_STATE_ENTERING) ? 5.2f : (s_skyPlane.active ? 4.5f : 3.2f);
    if (fabsf(dx) <= step) {
      s_catX = s_catTargetX;
      if (s_skyPlane.active) {
        s_catState = CAT_STATE_SWAT;
        s_animFrame = 0;
        s_nextFrameMs = now + 110;
      } else {
        s_catState = (isNight && !nightAwake) ? CAT_STATE_SLEEPING : CAT_STATE_IDLE;
        s_animFrame = 0;
        s_nextBrainDecisionMs = now + 8000 + (rand() % 8000);
      }
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
  } else if (s_catState == CAT_STATE_SLIDE) {
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 90;
      s_animFrame = (s_animFrame + 1) % 8;
    }
  } else if (s_catState == CAT_STATE_TUMBLE) {
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 65;
      s_animFrame = (s_animFrame + 1) % 4;
    }
  } else if (s_catState == CAT_STATE_CLING) {
    if (now >= s_nextFrameMs) {
      s_nextFrameMs = now + 120;
      s_animFrame = (s_animFrame + 1) % 4;
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
static void wrapSpeechText(const char* text, int maxW, uint8_t fontSize, int maxLines,
                           char lines[][64], int& lineCount, bool& allFit) {
  lineCount = 0;
  allFit = true;
  if (!text || !*text) return;

  const char* p = text;
  while (*p && lineCount < maxLines) {
    while (*p == ' ') p++;
    if (!*p) break;

    if (*p == '\n') {
      p++;
      continue;
    }

    const char* lineStart = p;
    const char* lastBreak = nullptr;
    const char* scan = p;

    char candidate[64];

    while (*scan && *scan != '\n') {
      while (*scan == ' ') scan++;
      if (!*scan || *scan == '\n') break;

      const char* wordEnd = scan;
      while (*wordEnd && *wordEnd != ' ' && *wordEnd != '\n') wordEnd++;

      int len = wordEnd - lineStart;
      if (len >= (int)sizeof(candidate)) {
        break;
      }

      memcpy(candidate, lineStart, len);
      candidate[len] = '\0';

      if (Font_TextWidth(candidate, fontSize) <= maxW) {
        lastBreak = wordEnd;
        scan = wordEnd;
      } else {
        break;
      }
    }

    if (!lastBreak) {
      const char* hardScan = lineStart;
      const char* lastChar = lineStart;
      while (*hardScan && *hardScan != ' ' && *hardScan != '\n') {
        const char* nextChar = hardScan + 1;
        while ((*nextChar & 0xC0) == 0x80) nextChar++;
        int len = nextChar - lineStart;
        if (len >= (int)sizeof(candidate)) break;
        memcpy(candidate, lineStart, len);
        candidate[len] = '\0';
        if (Font_TextWidth(candidate, fontSize) <= maxW) {
          lastChar = nextChar;
          hardScan = nextChar;
        } else {
          break;
        }
      }
      lastBreak = (lastChar > lineStart) ? lastChar : (lineStart + 1);
    }

    int copyLen = lastBreak - lineStart;
    if (copyLen >= (int)sizeof(lines[lineCount])) {
      copyLen = sizeof(lines[lineCount]) - 1;
    }
    memcpy(lines[lineCount], lineStart, copyLen);
    lines[lineCount][copyLen] = '\0';
    lineCount++;

    p = lastBreak;
    if (*p == ' ') p++;
    if (*p == '\n') p++;
  }

  while (*p == ' ') p++;
  if (*p) {
    allFit = false;
  }
}

static void drawSpeechBubble(int bx, int by, int bw, int bh, const char* text) {
  gfx->fillRoundRect(bx, by, bw, bh, 14, 0x0821);
  gfx->drawRoundRect(bx, by, bw, bh, 14, C_CYAN);

  const int tx = bx + bw / 2;
  const int ty = by + bh;
  gfx->fillTriangle(tx - 8, ty - 1, tx + 8, ty - 1, tx, ty + 10, 0x0821);
  gfx->drawLine(tx - 8, ty, tx, ty + 10, C_CYAN);
  gfx->drawLine(tx + 8, ty, tx, ty + 10, C_CYAN);

  if (!text || !*text) return;

  const int maxW = bw - 24;
  char lines[4][64];
  int lineCount = 0;
  bool allFit = false;
  uint8_t fontSize = FONT_MEDIUM;
  int fontH = 13;
  int lineSpacing = 18;

  // Prefer FONT_MEDIUM (bold, highly legible) with up to 3 lines
  wrapSpeechText(text, maxW, FONT_MEDIUM, 3, lines, lineCount, allFit);

  // Fall back to FONT_SMALL (up to 4 lines) if text is unusually long
  if (!allFit) {
    fontSize = FONT_SMALL;
    fontH = 10;
    lineSpacing = 14;
    wrapSpeechText(text, maxW, FONT_SMALL, 4, lines, lineCount, allFit);
  }

  if (lineCount <= 0) return;

  // Center text block vertically within the bubble
  int totalTextH = (lineCount - 1) * lineSpacing + fontH;
  int startY = by + (bh - totalTextH) / 2;

  for (int i = 0; i < lineCount; i++) {
    UI_TextCenteredIn(lines[i], bx, bw, startY + i * lineSpacing, C_WHITE, fontSize);
  }
}

static void drawHeart(int cx, int cy, int size, uint16_t color) {
  if (size < 4) return;
  int r = size / 2;
  gfx->fillCircle(cx - r / 2, cy - r / 4, r / 2, color);
  gfx->fillCircle(cx + r / 2, cy - r / 4, r / 2, color);
  gfx->fillTriangle(cx - size / 2, cy - r / 4, cx + size / 2, cy - r / 4, cx, cy + size / 2 + 1, color);
}

// -----------------------------------------------------------------------------
// Weather, Runway Deck & Accessory System
// -----------------------------------------------------------------------------
enum PetWeather : uint8_t {
  WEATHER_CLEAR = 0,
  WEATHER_RAIN,
  WEATHER_THUNDERSTORM,
  WEATHER_SNOW
};

static PetWeather getPetWeather() {
  // 1. Live radar nowcasting alert check
  const PrecipAlert* alert = PrecipTracker_GetAlert();
  if (alert && (alert->status == PRECIP_STAT_CURRENTLY_ACTIVE || alert->status == PRECIP_STAT_APPROACHING)) {
    if (alert->type == PRECIP_HAIL_STORM) return WEATHER_THUNDERSTORM;
    if (alert->type == PRECIP_SNOW) return WEATHER_SNOW;
    if (alert->type == PRECIP_SLEET) {
      if (Forecast_CurrentValid() && Forecast_CurrentTemp() <= 1.5f) return WEATHER_SNOW;
      return WEATHER_RAIN;
    }
    if (alert->type == PRECIP_RAIN) return WEATHER_RAIN;
  }

  // 2. Open-Meteo forecast current weather code
  if (Forecast_CurrentValid()) {
    int code = Forecast_CurrentCode();
    float temp = Forecast_CurrentTemp();
    if (code == 95 || code == 96 || code == 99) return WEATHER_THUNDERSTORM;
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) return WEATHER_SNOW;
    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82) || Forecast_CurrentPrecip() > 0.05f) {
      if (temp <= 0.5f) return WEATHER_SNOW;
      return WEATHER_RAIN;
    }
    if (temp <= -1.0f && (code == 45 || code == 48 || code == 3)) {
      return WEATHER_SNOW;
    }
  }

  return WEATHER_CLEAR;
}

static void drawWeatherBackdrop(PetWeather weather) {
  const unsigned long now = millis();

  // Clear night sky: twinkling stars
  if (Settings_IsNight() && weather == WEATHER_CLEAR) {
    static const struct { int16_t x, y; uint8_t period; } STARS[18] = {
      { 65, 80, 7 }, { 110, 70, 5 }, { 160, 48, 9 }, { 320, 48, 6 }, { 380, 74, 8 }, { 425, 90, 5 },
      { 48, 140, 6 }, { 75, 175, 8 }, { 405, 160, 7 }, { 430, 195, 9 }, { 55, 230, 5 }, { 415, 240, 8 },
      { 70, 275, 7 }, { 95, 305, 6 }, { 385, 290, 5 }, { 410, 315, 9 }, { 135, 325, 7 }, { 345, 325, 6 }
    };
    for (int i = 0; i < 18; i++) {
      uint8_t phase = (uint8_t)((now / (STARS[i].period * 60)) % 4);
      uint16_t starCol = (phase == 0) ? 0xFFFF : ((phase == 1) ? 0xCE79 : ((phase == 2) ? 0x8410 : 0x4208));
      gfx->drawPixel(STARS[i].x, STARS[i].y, starCol);
    }
    return;
  }

  // Thunderstorm with lightning flashes
  if (weather == WEATHER_THUNDERSTORM) {
    static unsigned long s_nextLightningMs = 0;
    static unsigned long s_lightningEndMs = 0;
    if (now >= s_nextLightningMs) {
      s_nextLightningMs = now + 8000 + (rand() % 9000);
      s_lightningEndMs = now + 70;
      if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) {
        Buzzer_Play(BEEP_PET_CHIRP);
      }
    }
    if (now < s_lightningEndMs) {
      gfx->fillRect(35, 40, 410, 302, 0x1946);
      gfx->drawLine(290, 42, 275, 110, 0xFFFF);
      gfx->drawLine(275, 110, 288, 165, 0xDEFB);
      gfx->drawLine(288, 165, 265, 225, 0xFFFF);
      gfx->drawLine(265, 225, 274, 295, 0x85FF);
    }
  }

  // Rain or Thunderstorm streaks
  if (weather == WEATHER_RAIN || weather == WEATHER_THUNDERSTORM) {
    int slant = (int)(s_imuTiltX * 0.3f);
    for (int i = 0; i < 22; i++) {
      int rx = (38 + i * 19 + (int)(now / 3) + slant) % 404 + 38;
      int ry = ((int)(now * 2) + i * 47) % 342;
      gfx->drawLine(rx, ry, rx + 1 + slant, ry + 8, (weather == WEATHER_THUNDERSTORM) ? 0x9E7F : 0x6DBF);
      if (ry > 333) {
        gfx->drawFastHLine(rx - 1, 342, 3, 0x9E7F); // splash ring on runway deck
      }
    }
    return;
  }

  // Drifting Snowflakes
  if (weather == WEATHER_SNOW) {
    for (int i = 0; i < 22; i++) {
      int sy = ((int)(now / 16) + i * 29) % 342;
      int drift = (int)(sinf((now * 0.0025f) + i) * 10.0f) + (int)(s_imuTiltX * 1.5f);
      int sx = 38 + ((i * 37 + drift) % 404);
      if (sx < 38) sx += 404;
      if (i % 3 == 0) {
        gfx->drawFastHLine(sx - 1, sy, 3, 0xFFFF);
        gfx->drawFastVLine(sx, sy - 1, 3, 0xFFFF);
      } else {
        gfx->drawPixel(sx, sy, (i % 2 == 0) ? 0xFFFF : 0xCE79);
      }
    }
  }
}

static void drawRunwayDeck(PetWeather weather) {
  // Runway Asphalt Surface (Y = 342 to 382)
  uint16_t tarmacCol = (weather == WEATHER_RAIN || weather == WEATHER_THUNDERSTORM) ? 0x10A2
                     : ((weather == WEATHER_SNOW) ? 0x2124 : 0x18C3);
  gfx->fillRect(35, 342, 410, 40, tarmacCol);

  // Runway Curb / Threshold line (Y = 342)
  uint16_t curbCol = (weather == WEATHER_SNOW) ? 0xFFFF
                   : ((weather == WEATHER_RAIN || weather == WEATHER_THUNDERSTORM) ? 0x4228 : 0x39E7);
  gfx->drawFastHLine(35, 342, 410, curbCol);

  // Snow accumulation layer on top of the curb
  if (weather == WEATHER_SNOW) {
    for (int x = 40; x < 440; x += 6) {
      gfx->drawPixel(x, 341, 0xCE79);
      gfx->drawPixel(x + 2, 341, 0xFFFF);
    }
  }

  // Centerline dashed markings (Y = 358)
  uint16_t dashCol = (weather == WEATHER_SNOW) ? 0x8410 : 0x632C;
  for (int x = 55; x < 420; x += 38) {
    gfx->fillRect(x, 358, 20, 3, dashCol);
  }

  // Runway Edge Lights
  // Left amber beacon at (54, 342)
  gfx->fillCircle(54, 342, 3, 0xFD20);
  gfx->drawCircle(54, 342, 5, 0x8400);

  // Right cyan/blue beacon at (426, 342)
  gfx->fillCircle(426, 342, 3, C_CYAN);
  gfx->drawCircle(426, 342, 5, 0x0210);
}

static void drawCatCastShadow(int cx, CatAnimState state, uint8_t frame) {
  if (state == CAT_STATE_CLING || state == CAT_STATE_AWAY) return;
  if (state == CAT_STATE_SLEEPING) {
    gfx->fillRoundRect(cx - 30, 340, 60, 5, 2, 0x0821);
    return;
  }

  int rx = 26;
  int ry = 4;
  uint16_t col = 0x0821;

  if (state == CAT_STATE_JUMP) {
    static const int8_t s_jumpY[8] = { 4, 6, -14, -26, -32, -20, -2, 4 };
    int8_t jy = s_jumpY[frame % 8];
    if (jy < -10) {
      rx = 14;
      ry = 2;
      col = 0x10A2; // higher jump -> smaller, fainter shadow on deck
    } else if (jy < 0) {
      rx = 20;
      ry = 3;
    }
  } else if (state == CAT_STATE_SWAT) {
    rx = 20;
    ry = 3;
  } else if (state == CAT_STATE_SLIDE) {
    rx = 30;
    ry = 3;
  } else if (state == CAT_STATE_TUMBLE) {
    rx = 22;
    ry = 3;
  }

  gfx->fillRoundRect(cx - rx, 340, rx * 2, ry, ry / 2, col);
}

static void drawUmbrellaCanopy(int cx, int cy, int r, uint16_t baseCol, uint16_t stripeCol) {
  // Render upper half of circle scanline-by-scanline without destructive black masks
  for (int dy = -r; dy <= 0; dy++) {
    int dx = (int)roundf(sqrtf((float)(r * r - dy * dy)));
    if (dx <= 0) continue;
    gfx->drawFastHLine(cx - dx, cy + dy, dx * 2 + 1, baseCol);
  }
  // Decorative contrasting ribs/stripes across canopy
  for (int dy = -r + 2; dy <= 0; dy++) {
    int dx = (int)roundf(sqrtf((float)(r * r - dy * dy)));
    if (dx > 4) {
      int s1 = dx / 2;
      gfx->drawPixel(cx - s1, cy + dy, stripeCol);
      gfx->drawPixel(cx + s1, cy + dy, stripeCol);
    }
  }
  // Apex tip ferrule & highlight
  gfx->drawFastVLine(cx, cy - r - 2, 3, baseCol);
  gfx->drawPixel(cx, cy - r - 3, 0xFFFF);
}

static void drawCatWeatherAccessories(int catDrawX, int catDrawY, bool flipX, CatAnimState state, PetWeather weather) {
  // 1. Rain & Thunderstorm: Umbrella!
  if (weather == WEATHER_RAIN || weather == WEATHER_THUNDERSTORM) {
    if (state == CAT_STATE_SLEEPING) {
      // Pitched beach/garden parasol sheltering the sleeping loaf from above
      int cx = (int)s_catX + 24;
      int cy = 265;
      int r = 26;
      int baseGroundX = cx + 24;
      int baseGroundY = 341;
      // Umbrella shaft and stand beside the loaf
      gfx->drawLine(cx, cy, baseGroundX, baseGroundY, 0x9482);
      gfx->drawFastHLine(baseGroundX - 6, baseGroundY, 13, 0x9482);
      // Clean non-destructive upper dome
      drawUmbrellaCanopy(cx, cy, r, 0xFDC0, 0xF800);
    } else {
      // Suppress umbrella during active acrobatic actions (jumping, swatting, eating, away, slide, tumble, cling)
      if (state == CAT_STATE_JUMP || state == CAT_STATE_SWAT || state == CAT_STATE_EATING || state == CAT_STATE_AWAY ||
          state == CAT_STATE_SLIDE || state == CAT_STATE_TUMBLE || state == CAT_STATE_CLING) {
        return;
      }
      // Handheld umbrella held upright in front paw
      int cx = flipX ? (catDrawX + 38) : (catDrawX + 90);
      int cy = catDrawY + 14;
      int r = 22;
      int pawX = flipX ? (catDrawX + 42) : (catDrawX + 86);
      int pawY = catDrawY + 48;
      // Draw shaft down to paw
      gfx->drawLine(cx, cy, pawX, pawY, 0x9482);
      gfx->drawPixel(pawX + (flipX ? -1 : 1), pawY, 0x9482);
      // Clean non-destructive upper dome
      drawUmbrellaCanopy(cx, cy, r, 0xFDC0, 0xF800);
    }
    return;
  }

  // 2. Snow / Freezing Weather: Cozy Knit Winter Scarf!
  bool isCold = (weather == WEATHER_SNOW) || (Forecast_CurrentValid() && Forecast_CurrentTemp() <= 2.0f);
  if (isCold && state != CAT_STATE_CLING && state != CAT_STATE_AWAY) {
    int neckX = flipX ? (catDrawX + 68) : (catDrawX + 48);
    int neckY = catDrawY + 54;
    gfx->fillRoundRect(neckX - 7, neckY, 15, 6, 2, 0xF800); // bright red scarf wrap
    gfx->drawFastVLine(neckX - 3, neckY, 6, 0xFFFF); // white knit pattern
    gfx->drawFastVLine(neckX + 2, neckY, 6, 0xFFFF);
    int tailX = flipX ? (neckX + 5) : (neckX - 5);
    gfx->fillRect(tailX, neckY + 4, 4, 10, 0xF800); // fluttering scarf tail
    gfx->drawFastHLine(tailX, neckY + 12, 4, 0xFFFF);
    gfx->drawFastHLine(tailX, neckY + 14, 4, 0xDEFB); // fringe
  }
}

static void drawTrackedSkyPlane() {
  if (!s_skyPlane.active) return;

  const unsigned long now = millis();
  int px = (int)s_skyPlane.x;
  int py = (int)s_skyPlane.y;
  bool flyingLeft = (s_skyPlane.vx < 0);

  // 1. Dual jet engine contrails trailing behind the aircraft
  int trailDir = flyingLeft ? 1 : -1;
  int tailOffset = flyingLeft ? 52 : 4;
  for (int i = 0; i < 7; i++) {
    int cx = px + tailOffset + i * trailDir * 7;
    if (cx < 40 || cx > 440) continue;
    uint16_t cCol = (i < 2) ? 0xFFFF : ((i < 4) ? 0xCE79 : ((i < 6) ? 0x8410 : 0x4208));
    gfx->drawFastHLine(cx, py + 10, 4, cCol);
    gfx->drawFastHLine(cx + 2, py + 18, 4, cCol);
  }

  // 2. Draw 2x scaled Pixel-Art Airplane Sprite (56x28 px)
  drawAirplaneSprite2x(px, py, flyingLeft);

  // 3. Strobe beacon light flash on fuselage/tailfin every 600ms
  if ((now / 300) % 2 == 0) {
    int strobeX = flyingLeft ? (px + 50) : (px + 6);
    gfx->fillCircle(strobeX, py + 2, 2, 0xFFFF);
  }

  // 4. Callsign & Distance Badge Pill floating right below plane
  char badge[32];
  if (s_skyPlane.callsign[0]) {
    snprintf(badge, sizeof(badge), "%s • %.0fkm", s_skyPlane.callsign, s_skyPlane.distKm);
  } else {
    snprintf(badge, sizeof(badge), "✈ %.0fkm", s_skyPlane.distKm);
  }
  int bw = Font_TextWidth(badge, FONT_TINY) + 12;
  int bx = px + 28 - bw / 2;
  bx = constrain(bx, 45, 435 - bw);
  int by = py + 30;

  gfx->fillRoundRect(bx, by, bw, 13, 4, 0x0821);
  gfx->drawRoundRect(bx, by, bw, 13, 4, 0x39E7);
  UI_TextCenteredIn(badge, bx, bw, by + 2, C_WHITE, FONT_TINY);

  // 5. Dynamic claw scratch marks / sparks when cat is actively swatting
  if (s_catState == CAT_STATE_SWAT && (s_animFrame % 8 == 2 || s_animFrame % 8 == 5)) {
    int clawX = (int)s_catX + (s_catFlipX ? -16 : 16);
    int clawY = py + 24;
    gfx->drawLine(clawX - 6, clawY - 4, clawX - 2, clawY + 5, 0xFFE0);
    gfx->drawLine(clawX - 1, clawY - 6, clawX + 3, clawY + 3, 0xFFFF);
    gfx->drawLine(clawX + 4, clawY - 5, clawX + 8, clawY + 4, 0xFFE0);
  }
}

// -----------------------------------------------------------------------------
// Dynamic Tilt Visual Effects (Skid trails, tumble sparks, bezel clinging)
// -----------------------------------------------------------------------------
static void drawSlideParticles(int catX, int catY, bool flipX, float speed) {
  int dir = flipX ? 1 : -1;
  int px = catX + dir * 20;
  unsigned long now = millis();
  for (int i = 0; i < 3; i++) {
    int ly = 340 + i * 2;
    int lx = px + dir * (i * 14 + (int)((now / 35) % 18));
    int len = 8 + (int)(fabsf(speed) * 1.6f);
    if (lx >= 40 && lx <= 440) {
      gfx->drawFastHLine(flipX ? lx : (lx - len), ly, len, 0xCE79);
    }
  }
}

static void drawTumbleEffects(int catX, int catY, bool flipX) {
  unsigned long now = millis();
  int sparkBaseX = catX + (flipX ? 22 : -22);
  for (int i = 0; i < 4; i++) {
    int sx = sparkBaseX + (int)(sinf((float)now * 0.025f + (float)i * 1.5f) * 16.0f);
    int sy = 338 - (int)(fabsf(cosf((float)now * 0.03f + (float)i * 1.2f)) * 14.0f);
    if (sx >= 40 && sx <= 440 && sy >= 300 && sy <= 342) {
      uint16_t col = (i % 2 == 0) ? 0xFFE0 : 0xFD06;
      gfx->drawPixel(sx, sy, col);
      gfx->drawPixel(sx + 1, sy, 0xFFFF);
    }
  }
  // Scared sweat droplet above head
  int sweatX = catX + (flipX ? 16 : -16);
  int sweatY = catY - 42 - (int)((now / 50) % 18);
  if (sweatX >= 40 && sweatX <= 440) {
    gfx->fillCircle(sweatX, sweatY, 3, 0x5DDF);
    gfx->drawPixel(sweatX, sweatY - 1, 0xFFFF);
  }
}

static void drawClingEffects(int catX, int catY, bool clingLeft) {
  int pawX1 = clingLeft ? 38 : 422;
  int pawX2 = clingLeft ? 52 : 436;
  // White/cream cat paws holding the rim
  gfx->fillRoundRect(pawX1, 339, 10, 6, 2, 0xFFFF);
  gfx->fillRoundRect(pawX2, 339, 10, 6, 2, 0xFFFF);
  // Claw lines gripping curb
  gfx->drawFastVLine(pawX1 + 3, 338, 3, 0x18A2);
  gfx->drawFastVLine(pawX1 + 7, 338, 3, 0x18A2);
  gfx->drawFastVLine(pawX2 + 3, 338, 3, 0x18A2);
  gfx->drawFastVLine(pawX2 + 7, 338, 3, 0x18A2);

  // Sweat drops splashing off forehead
  unsigned long now = millis();
  int swY = 320 + (int)((now / 60) % 16);
  gfx->fillCircle(clingLeft ? 68 : 412, swY, 3, 0x5DDF);
  gfx->drawPixel(clingLeft ? 68 : 412, swY - 1, 0xFFFF);
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

  // 5. Environmental Weather & Runway Deck
  PetWeather curWeather = getPetWeather();
  drawWeatherBackdrop(curWeather);
  drawRunwayDeck(curWeather);
  drawTrackedSkyPlane();

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
    // Screen clamping: slide and tumble can traverse wide deck [70, 410], cling is at rim [52, 428]
    if (s_catState != CAT_STATE_ENTERING && s_catState != CAT_STATE_LEAVING && s_catState != CAT_STATE_CLING) {
      if (s_catState == CAT_STATE_SLIDE || s_catState == CAT_STATE_TUMBLE) {
        s_catX = constrain(s_catX, 70.0f, 410.0f);
      } else {
        s_catX = constrain(s_catX, 140.0f, 340.0f);
      }
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
    } else if (s_catState == CAT_STATE_SWAT) {
      frameToDraw = CAT_SWAT_FRAMES[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_SLIDE) {
      frameToDraw = CAT_HAPPY_FRAMES[(now / 110) % 8];
    } else if (s_catState == CAT_STATE_TUMBLE) {
      static const uint8_t* const s_tumbleFrames[4] = {
        CAT_JUMP_FRAMES[1],
        CAT_SLEEP_FRAMES[0],
        CAT_STRETCH_FRAMES[2],
        CAT_JUMP_FRAMES[6]
      };
      frameToDraw = s_tumbleFrames[(now / 70) % 4];
    } else if (s_catState == CAT_STATE_CLING) {
      frameToDraw = CAT_SWAT_FRAMES[4];
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
    } else if (s_catState == CAT_STATE_SWAT) {
      static const int8_t s_swatBob[8] = { 0, -2, -4, -2, -5, -4, -2, 0 };
      catDrawY += s_swatBob[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_ENTERING || s_catState == CAT_STATE_PATROL || s_catState == CAT_STATE_LEAVING) {
      static const int8_t s_walkBob[8] = { 0, -2, -1, 0, 0, -2, -1, 0 };
      catDrawY += s_walkBob[s_animFrame % 8];
    } else if (s_catState == CAT_STATE_CLING) {
      catDrawY += 34; // Pinned down so paws grip the curb line
      if ((now / 140) % 2 == 0) catDrawY += 2; // struggling jitter
      s_catFlipX = !s_clingLeft;
    } else if (s_catState == CAT_STATE_TUMBLE) {
      catDrawY += (int)(sinf((float)now * 0.02f) * 4.0f);
    } else if (s_catState == CAT_STATE_SLIDE) {
      catDrawY += 2;
    }

    // Ground cast shadow directly under DigiCat's paws
    drawCatCastShadow((int)s_catX, s_catState, s_animFrame);

    // Render DigiCat 2x scaled sprite
    drawCatSprite2x(frameToDraw, catDrawX, catDrawY, s_catFlipX);

    // Dynamic particles & effects for tilt physics
    if (s_catState == CAT_STATE_SLIDE) {
      drawSlideParticles((int)s_catX, 340, s_catFlipX, s_catVx);
    } else if (s_catState == CAT_STATE_TUMBLE) {
      drawTumbleEffects((int)s_catX, 340, s_catFlipX);
    } else if (s_catState == CAT_STATE_CLING) {
      drawClingEffects((int)s_catX, 340, s_clingLeft);
    }

    // Weather-adaptive accessories (Umbrella in rain, Scarf in snow/cold, Aviator goggles in clear sky)
    drawCatWeatherAccessories(catDrawX, catDrawY, s_catFlipX, s_catState, curWeather);
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

  // If cat is clinging to rim, tap rescues it immediately!
  if (s_catState == CAT_STATE_CLING) {
    if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_PET_CHIRP);
    s_catState = CAT_STATE_JUMP;
    s_catX = s_clingLeft ? 150.0f : 330.0f;
    s_catVx = 0.0f;
    s_extremeOption = 0;
    s_animFrame = 0;
    s_nextFrameMs = now + 80;
    s_nextBrainDecisionMs = now + 6000;
    setThought(
      "🐾 *Hup!* Thanks for pulling me up! You saved me! 😸❤️",
      "🐾 *Hop!* Ďakujem za pomoc! Vytiahol si ma! 😸❤️",
      "🐾 *Hop!* Díky za pomoc! Vytáhl jsi mě! 😸❤️"
    );
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

  // Tap on the overhead tracked aircraft -> triggers evasive jet dash & cat jump/swat reaction!
  if (s_skyPlane.active && s_catState != CAT_STATE_AWAY && s_catState != CAT_STATE_LEAVING && s_catState != CAT_STATE_CLING) {
    int px = (int)(s_skyPlane.x + 28);
    int py = (int)(s_skyPlane.y + 14);
    if (abs(x - px) < 42 && abs(y - py) < 32) {
      if (Settings_BuzzerEnabled() && Settings_BuzzerPet()) Buzzer_Play(BEEP_PET_CHIRP);
      PetBrain_AwardXP(2);
      s_skyPlane.evasiveHop = true;
      s_skyPlane.hopEndMs = now + 500;
      s_planeChasePauseUntilMs = 0;
      // Cat jumps towards the plane
      s_catTargetX = (int)s_skyPlane.x;
      s_catState = CAT_STATE_JUMP;
      s_animFrame = 0;
      s_nextFrameMs = now + 80;
      s_nextBrainDecisionMs = now + 5000;
      setThought(
        "✈️ *SWIPEE!* Look at that jet zoom! Almost caught it! 🛩️🐾",
        "✈️ *CHŇAP!* Pozri ako to lietadielko fičí! Skoro som ho chytil! 🛩️🐾",
        "✈️ *CHŇAP!* Koukej jak to letadélko fičí! Málem jsem ho chytil! 🛩️🐾"
      );
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
