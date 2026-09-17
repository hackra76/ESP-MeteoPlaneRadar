// =============================================================================
//  MeteoPlaneRadar
//  PetDrawer.cpp - Full-screen Pet Companion overlay and animation engine.
// =============================================================================
#include "PetDrawer.h"
#include "PetBrain.h"
#include "Display_ST7701.h"
#include "Settings.h"
#include "UI.h"
#include "Buzzer.h"
#include "NightMode.h"
#include "Lang.h"
#include <math.h>

static bool s_isOpen = false;
static unsigned long s_lastAnimTick = 0;
static uint32_t s_frameCounter = 0;

// Blink state machine
static bool s_isBlinking = false;
static unsigned long s_nextBlinkMs = 3000;
static unsigned long s_blinkEndMs = 0;

bool PetDrawer_IsOpen() {
  return s_isOpen;
}

void PetDrawer_Open() {
  s_isOpen = true;
  s_nextBlinkMs = millis() + 2000;
  // Trigger thought update if none or idle
  PetBrain_RequestThought(false);
}

void PetDrawer_Close() {
  s_isOpen = false;
}

void PetDrawer_Toggle() {
  if (s_isOpen) PetDrawer_Close();
  else PetDrawer_Open();
}

bool PetDrawer_Tick() {
  if (!s_isOpen) return false;
  const unsigned long now = millis();

  // Animation frame rate cap (~30 FPS)
  if (now - s_lastAnimTick < 33) return false;
  s_lastAnimTick = now;
  s_frameCounter++;

  // Handle blinking
  if (!s_isBlinking && now >= s_nextBlinkMs) {
    s_isBlinking = true;
    s_blinkEndMs = now + 140; // 140ms blink
  } else if (s_isBlinking && now >= s_blinkEndMs) {
    s_isBlinking = false;
    s_nextBlinkMs = now + 2500 + (rand() % 3000);
  }

  PetBrain_Tick();
  return true;
}

// Word-wrap and draw text inside speech bubble
static void drawSpeechBubble(int bx, int by, int bw, int bh, const char* text) {
  // Bubble background with subtle glowing cyan/white border
  gfx->fillRoundRect(bx, by, bw, bh, 14, 0x0821);
  gfx->drawRoundRect(bx, by, bw, bh, 14, C_CYAN);

  // Bubble tail pointing down towards the pet's head
  const int tx = bx + bw / 2;
  const int ty = by + bh;
  gfx->fillTriangle(tx - 8, ty - 1, tx + 8, ty - 1, tx, ty + 10, 0x0821);
  gfx->drawLine(tx - 8, ty, tx, ty + 10, C_CYAN);
  gfx->drawLine(tx + 8, ty, tx, ty + 10, C_CYAN);

  if (!text || !*text) return;

  // Render text wrapped to max 3 lines (max ~32 chars per line)
  char line[36];
  const char* p = text;
  int ly = by + 10;
  int lineCount = 0;

  while (*p && lineCount < 3) {
    while (*p == ' ') p++;
    if (!*p) break;

    int len = 0;
    int lastSpace = -1;
    while (p[len] && len < 32) {
      if (p[len] == ' ') lastSpace = len;
      len++;
    }

    int copyLen = len;
    if (p[len] && lastSpace > 0) {
      copyLen = lastSpace;
    }

    strncpy(line, p, copyLen);
    line[copyLen] = '\0';
    UI_TextCenteredIn(line, bx, bw, ly, C_WHITE, 1);
    ly += 16;
    lineCount++;
    p += copyLen;
  }
}

// Helper to draw anti-aliased vector hearts
static void drawHeart(int cx, int cy, int size, uint16_t color) {
  if (size < 4) return;
  int r = size / 2;
  gfx->fillCircle(cx - r / 2, cy - r / 4, r / 2, color);
  gfx->fillCircle(cx + r / 2, cy - r / 4, r / 2, color);
  gfx->fillTriangle(cx - size / 2, cy - r / 4, cx + size / 2, cy - r / 4, cx, cy + size / 2 + 1, color);
}

// -----------------------------------------------------------------------------
// Character 0: Cyber Eyes (Vector / Cozmo style glowing robotics)
// -----------------------------------------------------------------------------
static void drawCyberEyes(int cx, int cy, PetMood mood, float bDeg, bool hasTarget) {
  uint16_t eyeCol = C_CYAN;
  if (mood == PET_MOOD_EXCITED) eyeCol = C_ORANGE;
  else if (mood == PET_MOOD_RAIN) eyeCol = 0x5D1F;
  else if (mood == PET_MOOD_HAPPY) eyeCol = 0xF814; // vibrant neon rose pink when loved
  else if (mood == PET_MOOD_SLEEPY) eyeCol = 0x3186;

  int eyeH = s_isBlinking ? 4 : (mood == PET_MOOD_SLEEPY ? 18 : 60);
  const int eyeW = 72;
  const int spacing = 46;

  // Pupil offsets
  int px = 0, py = 0;
  if (hasTarget && !s_isBlinking && mood != PET_MOOD_SLEEPY) {
    float rad = bDeg * (float)M_PI / 180.0f;
    px = (int)(sinf(rad) * 16.0f);
    py = (int)(-cosf(rad) * 12.0f);
  }

  // Left & Right eye positions
  int lx = cx - spacing - eyeW;
  int rx = cx + spacing;
  int ey = cy - eyeH / 2;

  // Outer ambient glow
  if (!s_isBlinking && eyeH > 30) {
    gfx->drawRoundRect(lx - 2, ey - 2, eyeW + 4, eyeH + 4, 18, 0x1084);
    gfx->drawRoundRect(rx - 2, ey - 2, eyeW + 4, eyeH + 4, 18, 0x1084);
  }

  // Draw eye backdrops
  gfx->fillRoundRect(lx, ey, eyeW, eyeH, s_isBlinking ? 2 : 16, eyeCol);
  gfx->fillRoundRect(rx, ey, eyeW, eyeH, s_isBlinking ? 2 : 16, eyeCol);

  // Pupils (dark cutout with glossy center, or glowing white hearts when happy)
  if (!s_isBlinking && eyeH > 24) {
    if (mood == PET_MOOD_HAPPY) {
      drawHeart(lx + eyeW / 2 + px, cy + py, 20, C_WHITE);
      drawHeart(rx + eyeW / 2 + px, cy + py, 20, C_WHITE);
    } else {
      gfx->fillCircle(lx + eyeW / 2 + px, cy + py, 14, C_BLACK);
      gfx->fillCircle(rx + eyeW / 2 + px, cy + py, 14, C_BLACK);
      gfx->fillCircle(lx + eyeW / 2 + px + 3, cy + py - 3, 5, C_WHITE);
      gfx->fillCircle(rx + eyeW / 2 + px + 3, cy + py - 3, 5, C_WHITE);
    }
  }
}

// =============================================================================
// Character 1: Aero Cat (Feline with aviator goggles & whiskers)
// Visual design, sleeping pose, and animations inspired by DigiCat
// (https://github.com/aquascape123/digicat) by aquascape123 under MIT License.
// =============================================================================
static void drawSleepingCat(int cx, int cy, uint16_t color) {
  // Main body (large cozy oval)
  gfx->fillEllipse(cx, cy + 10, 58, 40, color);
  // Head tucked into body
  gfx->fillCircle(cx - 25, cy - 4, 26, color);
  // Ears poking out
  gfx->fillTriangle(cx - 40, cy - 28, cx - 22, cy - 14, cx - 42, cy - 8, color);
  gfx->fillTriangle(cx - 16, cy - 30, cx + 2, cy - 18, cx - 20, cy - 14, color);
  // Inner ears (pink)
  gfx->fillTriangle(cx - 36, cy - 24, cx - 24, cy - 14, cx - 38, cy - 10, 0xFBEF);
  // Sleepy curved eyes
  gfx->drawCircle(cx - 30, cy - 3, 4, C_BLACK);
  gfx->drawLine(cx - 34, cy - 3, cx - 26, cy - 3, color);
  gfx->drawCircle(cx - 18, cy - 3, 4, C_BLACK);
  gfx->drawLine(cx - 22, cy - 3, cx - 14, cy - 3, color);
  // Tiny pink nose
  gfx->fillCircle(cx - 24, cy + 6, 3, 0xFBEF);
  // Curled tail wrapped around body
  for (int i = 0; i < 8; i++) {
    float angle = (float)i * 0.42f;
    int tailX = cx + (int)(cosf(angle) * 48.0f);
    int tailY = cy + 14 + (int)(sinf(angle) * 22.0f);
    gfx->fillCircle(tailX, tailY, 7 - i / 2, color);
  }
  // Soft tabby stripes
  gfx->drawLine(cx - 6, cy + 4, cx - 6, cy + 24, 0x6180);
  gfx->drawLine(cx + 8, cy + 4, cx + 8, cy + 26, 0x6180);
  gfx->drawLine(cx + 22, cy + 6, cx + 22, cy + 24, 0x6180);

  // Animated floating "Z z z"
  int zPhase = (millis() / 500) % 24;
  gfx->setTextSize(1);
  gfx->setTextColor(C_CYAN);
  gfx->setCursor(cx + 34, cy - 20 - zPhase);
  gfx->print("Z");
  gfx->setTextColor(0x5D1F);
  gfx->setCursor(cx + 48, cy - 30 - zPhase);
  gfx->print("z");
}

static void drawAeroCat(int cx, int cy, PetMood mood, float bDeg, bool hasTarget) {
  if (mood == PET_MOOD_SLEEPY) {
    drawSleepingCat(cx, cy, 0xBDD7);
    return;
  }

  // Tail curving and swaying behind cat (inspired by DigiCat)
  float tailSway = sinf((float)millis() * 0.0025f) * 8.0f;
  for (int i = 0; i < 7; i++) {
    int tx = cx + 42 + i * 4 + (int)(tailSway * (i / 7.0f));
    int ty = cy + 12 - i * 4;
    gfx->fillCircle(tx, ty, 8 - i, 0xBDD7);
  }

  // Ears
  gfx->fillTriangle(cx - 58, cy - 30, cx - 20, cy - 42, cx - 54, cy - 82, 0xBDD7);
  gfx->fillTriangle(cx + 20, cy - 42, cx + 58, cy - 30, cx + 54, cy - 82, 0xBDD7);
  gfx->fillTriangle(cx - 52, cy - 34, cx - 26, cy - 42, cx - 48, cy - 72, 0xFBEF); // inner pink
  gfx->fillTriangle(cx + 26, cy - 42, cx + 52, cy - 34, cx + 48, cy - 72, 0xFBEF);

  // Head base
  gfx->fillCircle(cx, cy, 56, 0xBDD7);

  // Classic Tabby "M" forehead marking (inspired by DigiCat)
  gfx->drawLine(cx - 10, cy - 24, cx, cy - 20, 0x6180);
  gfx->drawLine(cx, cy - 20, cx + 10, cy - 24, 0x6180);
  gfx->drawLine(cx, cy - 28, cx, cy - 20, 0x6180);

  // Aviator goggles on forehead
  gfx->fillRoundRect(cx - 50, cy - 52, 100, 20, 8, 0x6180); // brown leather strap
  gfx->fillCircle(cx - 22, cy - 42, 14, 0x3186);
  gfx->fillCircle(cx + 22, cy - 42, 14, 0x3186);
  gfx->drawCircle(cx - 22, cy - 42, 14, C_WHITE);
  gfx->drawCircle(cx + 22, cy - 42, 14, C_WHITE);
  gfx->fillCircle(cx - 20, cy - 45, 5, C_WHITE); // lens glare
  gfx->fillCircle(cx + 24, cy - 45, 5, C_WHITE);

  // Eyes
  if (s_isBlinking) {
    gfx->drawLine(cx - 32, cy - 6, cx - 14, cy - 6, C_BLACK);
    gfx->drawLine(cx + 14, cy - 6, cx + 32, cy - 6, C_BLACK);
  } else {
    int px = 0, py = 0;
    if (hasTarget) {
      float rad = bDeg * (float)M_PI / 180.0f;
      px = (int)(sinf(rad) * 6.0f);
      py = (int)(-cosf(rad) * 5.0f);
    }
    gfx->fillCircle(cx - 23, cy - 6, 12, C_WHITE);
    gfx->fillCircle(cx + 23, cy - 6, 12, C_WHITE);
    gfx->fillCircle(cx - 23 + px, cy - 6 + py, 7, 0x0400); // dark green cat pupil
    gfx->fillCircle(cx + 23 + px, cy - 6 + py, 7, 0x0400);
    gfx->fillCircle(cx - 25 + px, cy - 8 + py, 3, C_WHITE);
    gfx->fillCircle(cx + 21 + px, cy - 8 + py, 3, C_WHITE);

    // Sparkly eyes when very happy (inspired by DigiCat)
    if (PetBrain_GetStats().happiness > 70) {
      gfx->fillCircle(cx - 21 + px, cy - 4 + py, 2, C_WHITE);
      gfx->fillCircle(cx + 25 + px, cy - 4 + py, 2, C_WHITE);
    }
  }

  // Nose (pink triangle)
  gfx->fillTriangle(cx - 5, cy + 12, cx + 5, cy + 12, cx, cy + 18, 0xFBEF);

  // Blushing cheeks when happy
  if (mood == PET_MOOD_HAPPY) {
    gfx->fillCircle(cx - 36, cy + 10, 8, 0xF9D5);
    gfx->fillCircle(cx + 36, cy + 10, 8, 0xF9D5);
  }

  // Mouth & Whiskers
  gfx->drawLine(cx, cy + 18, cx, cy + 24, C_DKGRAY);
  gfx->drawLine(cx, cy + 24, cx - 10, cy + 29, C_DKGRAY);
  gfx->drawLine(cx, cy + 24, cx + 10, cy + 29, C_DKGRAY);

  // Whiskers (left)
  gfx->drawLine(cx - 30, cy + 14, cx - 60, cy + 10, C_WHITE);
  gfx->drawLine(cx - 30, cy + 20, cx - 62, cy + 22, C_WHITE);
  // Whiskers (right)
  gfx->drawLine(cx + 30, cy + 14, cx + 60, cy + 10, C_WHITE);
  gfx->drawLine(cx + 30, cy + 20, cx + 62, cy + 22, C_WHITE);

  // Front paws resting forward with pink paw pads (inspired by DigiCat)
  gfx->fillEllipse(cx - 18, cy + 34, 11, 7, 0xBDD7);
  gfx->fillEllipse(cx + 18, cy + 34, 11, 7, 0xBDD7);
  gfx->fillCircle(cx - 18, cy + 34, 3, 0xFBEF);
  gfx->fillCircle(cx + 18, cy + 34, 3, 0xFBEF);
}

// -----------------------------------------------------------------------------
// Character 2: Radar Dog (Canine with floppy ears & radar antenna cap)
// -----------------------------------------------------------------------------
static void drawRadarDog(int cx, int cy, PetMood mood, float bDeg, bool hasTarget) {
  // Floppy ears
  gfx->fillRoundRect(cx - 72, cy - 30, 28, 75, 12, 0x8200); // dark golden brown
  gfx->fillRoundRect(cx + 44, cy - 30, 28, 75, 12, 0x8200);

  // Head base
  gfx->fillCircle(cx, cy, 56, 0xDEFB); // light golden tan

  // Radar antenna cap
  gfx->fillRoundRect(cx - 28, cy - 60, 56, 16, 6, C_BLUE);
  gfx->drawLine(cx, cy - 60, cx, cy - 78, C_WHITE);
  gfx->fillCircle(cx, cy - 79, 5, (s_frameCounter % 30 < 15) ? C_RED : C_YELLOW); // blinking beacon

  // Eyes
  if (s_isBlinking || mood == PET_MOOD_SLEEPY) {
    gfx->drawLine(cx - 30, cy - 10, cx - 14, cy - 10, C_BLACK);
    gfx->drawLine(cx + 14, cy - 10, cx + 30, cy - 10, C_BLACK);
  } else {
    int px = 0, py = 0;
    if (hasTarget) {
      float rad = bDeg * (float)M_PI / 180.0f;
      px = (int)(sinf(rad) * 6.0f);
      py = (int)(-cosf(rad) * 5.0f);
    }
    gfx->fillCircle(cx - 21, cy - 10, 11, C_WHITE);
    gfx->fillCircle(cx + 21, cy - 10, 11, C_WHITE);
    gfx->fillCircle(cx - 21 + px, cy - 10 + py, 6, C_BLACK);
    gfx->fillCircle(cx + 21 + px, cy - 10 + py, 6, C_BLACK);
    gfx->fillCircle(cx - 23 + px, cy - 12 + py, 3, C_WHITE);
    gfx->fillCircle(cx + 19 + px, cy - 12 + py, 3, C_WHITE);
  }

  // Snout
  gfx->fillCircle(cx, cy + 20, 22, C_WHITE);
  gfx->fillCircle(cx, cy + 12, 8, C_BLACK); // nose

  // Blushing cheeks and tongue panting when excited or happy
  if (mood == PET_MOOD_HAPPY || mood == PET_MOOD_EXCITED) {
    gfx->fillCircle(cx - 40, cy + 10, 8, 0xF9D5);
    gfx->fillCircle(cx + 40, cy + 10, 8, 0xF9D5);
    gfx->fillRoundRect(cx - 6, cy + 27, 12, 18, 5, 0xF814); // pink tongue
  }
}

// -----------------------------------------------------------------------------
// Character 3: DigiCat (Authentic orange tabby virtual cat)
// Inspired by aquascape123/digicat under MIT License
// -----------------------------------------------------------------------------
static void drawDigiCat(int cx, int cy, PetMood mood, float bDeg, bool hasTarget) {
  const uint16_t CAT_BODY   = 0xFD20; // Vibrant orange
  const uint16_t CAT_STRIPE = 0x9260; // Dark warm tabby stripe
  const uint16_t CAT_EYES   = 0x07E0; // Emerald green
  const uint16_t CAT_NOSE   = 0xFBEF; // Soft pink

  if (mood == PET_MOOD_SLEEPY) {
    drawSleepingCat(cx, cy, CAT_BODY);
    return;
  }

  // 1. Curving, swaying tabby tail behind cat
  float tailSway = sinf((float)millis() * 0.003f) * 10.0f;
  for (int i = 0; i < 8; i++) {
    int tx = cx + 40 + i * 4 + (int)(tailSway * (i / 8.0f));
    int ty = cy + 14 - i * 4;
    uint16_t col = (i % 3 == 0) ? CAT_STRIPE : CAT_BODY;
    gfx->fillCircle(tx, ty, 8 - (i * 4 / 8), col);
  }

  // 2. Ears (triangles with pink inner ear)
  gfx->fillTriangle(cx - 56, cy - 28, cx - 18, cy - 40, cx - 52, cy - 80, CAT_BODY);
  gfx->fillTriangle(cx + 18, cy - 40, cx + 56, cy - 28, cx + 52, cy - 80, CAT_BODY);
  gfx->fillTriangle(cx - 50, cy - 32, cx - 24, cy - 40, cx - 46, cy - 70, CAT_NOSE);
  gfx->fillTriangle(cx + 24, cy - 40, cx + 50, cy - 32, cx + 46, cy - 70, CAT_NOSE);

  // 3. Head base & chubby cheeks
  gfx->fillCircle(cx, cy, 54, CAT_BODY);
  gfx->fillCircle(cx - 30, cy + 10, 20, CAT_BODY);
  gfx->fillCircle(cx + 30, cy + 10, 20, CAT_BODY);

  // 4. White chest/belly bib
  gfx->fillEllipse(cx, cy + 30, 26, 18, C_WHITE);

  // 5. Forehead Tabby "M" marking & tabby side stripes
  gfx->drawLine(cx - 12, cy - 28, cx - 5, cy - 18, CAT_STRIPE);
  gfx->drawLine(cx - 5, cy - 18, cx, cy - 26, CAT_STRIPE);
  gfx->drawLine(cx, cy - 26, cx + 5, cy - 18, CAT_STRIPE);
  gfx->drawLine(cx + 5, cy - 18, cx + 12, cy - 28, CAT_STRIPE);
  gfx->drawLine(cx, cy - 26, cx, cy - 38, CAT_STRIPE);
  gfx->drawLine(cx - 12, cy - 28, cx - 16, cy - 38, CAT_STRIPE);
  gfx->drawLine(cx + 12, cy - 28, cx + 16, cy - 38, CAT_STRIPE);

  // Cheeks side stripes
  gfx->drawLine(cx - 48, cy + 4, cx - 36, cy + 6, CAT_STRIPE);
  gfx->drawLine(cx - 48, cy + 12, cx - 34, cy + 13, CAT_STRIPE);
  gfx->drawLine(cx + 36, cy + 6, cx + 48, cy + 4, CAT_STRIPE);
  gfx->drawLine(cx + 34, cy + 13, cx + 48, cy + 12, CAT_STRIPE);

  // 6. Eyes (Big emerald green with aircraft tracking & dual sparkles)
  if (s_isBlinking) {
    gfx->drawLine(cx - 32, cy - 6, cx - 14, cy - 6, C_BLACK);
    gfx->drawLine(cx + 14, cy - 6, cx + 32, cy - 6, C_BLACK);
  } else {
    int px = 0, py = 0;
    if (hasTarget) {
      float rad = bDeg * (float)M_PI / 180.0f;
      px = (int)(sinf(rad) * 6.0f);
      py = (int)(-cosf(rad) * 5.0f);
    }
    // Sclera / eye white
    gfx->fillCircle(cx - 23, cy - 6, 13, C_WHITE);
    gfx->fillCircle(cx + 23, cy - 6, 13, C_WHITE);
    // Emerald green iris
    gfx->fillCircle(cx - 23, cy - 6, 11, CAT_EYES);
    gfx->fillCircle(cx + 23, cy - 6, 11, CAT_EYES);
    // Dark pupil (vertically elongated cat pupil)
    gfx->fillEllipse(cx - 23 + px, cy - 6 + py, 5, 8, C_BLACK);
    gfx->fillEllipse(cx + 23 + px, cy - 6 + py, 5, 8, C_BLACK);
    // Primary sparkle
    gfx->fillCircle(cx - 26 + px, cy - 9 + py, 3, C_WHITE);
    gfx->fillCircle(cx + 20 + px, cy - 9 + py, 3, C_WHITE);
    // Secondary cute glint
    gfx->fillCircle(cx - 20 + px, cy - 3 + py, 2, C_WHITE);
    gfx->fillCircle(cx + 26 + px, cy - 3 + py, 2, C_WHITE);
  }

  // 7. Blushing cheeks when happy or excited
  if (mood == PET_MOOD_HAPPY || mood == PET_MOOD_EXCITED) {
    gfx->fillCircle(cx - 36, cy + 12, 8, 0xF9D5);
    gfx->fillCircle(cx + 36, cy + 12, 8, 0xF9D5);
  }

  // 8. White muzzle pads & pink nose
  gfx->fillCircle(cx - 8, cy + 16, 10, C_WHITE);
  gfx->fillCircle(cx + 8, cy + 16, 10, C_WHITE);
  gfx->fillTriangle(cx - 6, cy + 10, cx + 6, cy + 10, cx, cy + 16, CAT_NOSE);

  // 9. Mouth line & tongue
  gfx->drawLine(cx, cy + 16, cx, cy + 20, C_BLACK);
  gfx->drawLine(cx, cy + 20, cx - 8, cy + 24, C_BLACK);
  gfx->drawLine(cx, cy + 20, cx + 8, cy + 24, C_BLACK);
  if (mood == PET_MOOD_EXCITED) {
    gfx->fillRoundRect(cx - 4, cy + 22, 8, 9, 3, 0xF814); // happy pink tongue
  }

  // 10. Whiskers (3 on each side)
  gfx->drawLine(cx - 26, cy + 14, cx - 60, cy + 10, C_WHITE);
  gfx->drawLine(cx - 26, cy + 19, cx - 62, cy + 19, C_WHITE);
  gfx->drawLine(cx - 26, cy + 24, cx - 58, cy + 28, C_WHITE);
  gfx->drawLine(cx + 26, cy + 14, cx + 60, cy + 10, C_WHITE);
  gfx->drawLine(cx + 26, cy + 19, cx + 62, cy + 19, C_WHITE);
  gfx->drawLine(cx + 26, cy + 24, cx + 58, cy + 28, C_WHITE);

  // 11. Front paws resting forward with pink paw pads
  gfx->fillEllipse(cx - 18, cy + 34, 11, 7, C_WHITE);
  gfx->fillEllipse(cx + 18, cy + 34, 11, 7, C_WHITE);
  gfx->fillCircle(cx - 18, cy + 34, 3, CAT_NOSE);
  gfx->fillCircle(cx + 18, cy + 34, 3, CAT_NOSE);
  gfx->fillCircle(cx - 22, cy + 32, 2, CAT_NOSE);
  gfx->fillCircle(cx - 14, cy + 32, 2, CAT_NOSE);
  gfx->fillCircle(cx + 14, cy + 32, 2, CAT_NOSE);
  gfx->fillCircle(cx + 22, cy + 32, 2, CAT_NOSE);
}

static unsigned long s_petBounceUntilMs = 0;
static unsigned long s_petFeedUntilMs = 0;

void PetDrawer_Draw() {
  if (!s_isOpen) return;

  // Clear full 480x480 circular screen to clean black background
  gfx->fillScreen(C_BLACK);

  // 1. Clock & Outside Temperature at the top (standard unified status pill)
  UI_DrawStatusLine(36);

  // 2. Character Name / Switcher pill with Aviation Rank (centered at Y = 58)
  const uint8_t ch = Settings_PetCharacter();
  const char* CHAR_TITLES[] = { "CYBER RADAR AI", "AERO CAT", "RADAR SHIBA", "DIGICAT" };
  char titleBuf[64];
  snprintf(titleBuf, sizeof(titleBuf), "🐾 %s [%s] ›",
           CHAR_TITLES[ch % 4], PetBrain_GetStageTitle());
  const int titleW = 280;
  const int titleX = (LCD_WIDTH - titleW) / 2;
  const int titleY = 58;
  gfx->fillRoundRect(titleX, titleY, titleW, 22, 11, 0x10A2);
  gfx->drawRoundRect(titleX, titleY, titleW, 22, 11, C_CYAN);
  UI_TextCenteredIn(titleBuf, titleX, titleW, titleY + 4, C_WHITE, 1);

  // 3. Virtual Pet Stats Badge (inspired by DigiCat under MIT License)
  PetStats stats = PetBrain_GetStats();
  char statBuf[64];
  snprintf(statBuf, sizeof(statBuf), "HAPPY: %d%%  •  HUNGER: %d%%  •  XP: %u",
           stats.happiness, stats.hunger, stats.flightsTracked);
  UI_TextCentered(statBuf, 84, 0x07E0, 1);

  // 4. Speech Bubble (Y = 98 .. 166)
  drawSpeechBubble(60, 98, 360, 68, PetBrain_GetThought());

  // 5. Pet Avatar (Centered at X = 240, Y = 282 with happy bounce)
  const unsigned long now = millis();
  int bounceY = 0;
  if (now < s_petBounceUntilMs) {
    float phase = (float)(s_petBounceUntilMs - now) / 1200.0f; // 1.0 down to 0.0
    bounceY = (int)(-fabsf(sinf(phase * (float)M_PI * 3.5f)) * 16.0f * phase);
  }
  const int petCenterX = LCD_WIDTH / 2;
  const int petCenterY = 282 + bounceY;

  float bDeg = 0.0f, dist = 9999.0f;
  char cs[16] = "";
  bool hasTarget = PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs));
  PetMood mood = PetBrain_GetMood();

  switch (ch) {
    case 1: drawAeroCat(petCenterX, petCenterY, mood, bDeg, hasTarget); break;
    case 2: drawRadarDog(petCenterX, petCenterY, mood, bDeg, hasTarget); break;
    case 3: drawDigiCat(petCenterX, petCenterY, mood, bDeg, hasTarget); break;
    default: drawCyberEyes(petCenterX, petCenterY, mood, bDeg, hasTarget); break;
  }

  // Treat snack animation when fed (inspired by DigiCat)
  if (now < s_petFeedUntilMs) {
    gfx->fillEllipse(petCenterX + 64, petCenterY + 18, 12, 7, 0xFDC0); // golden fish cracker
    gfx->fillTriangle(petCenterX + 74, petCenterY + 18, petCenterX + 82, petCenterY + 12, petCenterX + 82, petCenterY + 24, 0xFDC0);
    gfx->fillCircle(petCenterX + 58, petCenterY + 17, 2, C_BLACK);
    gfx->fillCircle(petCenterX + 62, petCenterY + 4, 2, C_YELLOW);
    gfx->fillCircle(petCenterX + 78, petCenterY + 6, 2, C_WHITE);
  }

  // Floating animated hearts when petted or happy
  if (now < s_petBounceUntilMs || mood == PET_MOOD_HAPPY) {
    float t1 = (float)(now % 1200) / 1200.0f;
    int h1_y = petCenterY - 45 - (int)(t1 * 50.0f);
    int h1_x = petCenterX - 75 + (int)(sinf(t1 * (float)M_PI * 2.0f) * 10.0f);
    drawHeart(h1_x, h1_y, 16, 0xF814); // bright pink-red

    float t2 = (float)((now + 400) % 1200) / 1200.0f;
    int h2_y = petCenterY - 55 - (int)(t2 * 55.0f);
    int h2_x = petCenterX + 75 + (int)(cosf(t2 * (float)M_PI * 2.0f) * 10.0f);
    drawHeart(h2_x, h2_y, 18, 0xFD20); // bright coral

    float t3 = (float)((now + 800) % 1200) / 1200.0f;
    int h3_y = petCenterY - 70 - (int)(t3 * 45.0f);
    int h3_x = petCenterX - 15 + (int)(sinf(t3 * (float)M_PI * 2.0f) * 8.0f);
    drawHeart(h3_x, h3_y, 13, 0xF814);
  }

  // 6. Dual Action Buttons (FEED TREAT & PET/TALK)
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

  // 7. Subtle return gesture hint at the bottom rim (Y = 442)
  const char* hintTxt = (Lang_Get() == LANG_EN) ? "v swipe down from top to close v"
                       : ((Lang_Get() == LANG_SK) ? "v potiahnutim zhora zatvorte v"
                                                  : "v potazenim shora zavrete v");
  UI_TextCentered(hintTxt, 442, 0x632C, 1);
}

bool PetDrawer_HandleTap(int x, int y) {
  if (!s_isOpen) return false;

  // Tap at the very top edge (Y <= 48) closes the pet screen
  if (y <= 48) {
    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
    PetDrawer_Close();
    return true;
  }

  // Tap on Pet Switcher button (Y = 52 .. 80)
  if (y >= 52 && y <= 80 && x >= 90 && x <= 390) {
    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
    uint8_t nextCh = (Settings_PetCharacter() + 1) % 4;
    Settings_SetPetCharacter(nextCh);
    PetBrain_RequestThought(false);
    return true;
  }

  // Tap on speech bubble (Y = 96 .. 170) refreshes thought
  if (y >= 96 && y <= 170) {
    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
    PetBrain_RequestThought(false);
    return true;
  }

  // Dual bottom buttons (Y = 385 .. 430)
  if (y >= 385 && y <= 430) {
    // Left: Feed Treat
    if (x >= 80 && x <= 235) {
      if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
      s_petFeedUntilMs = millis() + 1500;
      s_petBounceUntilMs = millis() + 1000;
      PetBrain_Feed();
      return true;
    }
    // Right: Pet / Talk
    if (x >= 245 && x <= 400) {
      if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_OVERHEAD);
      s_petBounceUntilMs = millis() + 1200;
      PetBrain_RequestThought(true);
      return true;
    }
  }

  // Tap on Pet avatar (Y = 175 .. 384) triggers petting & audio chime
  if (y >= 175 && y < 385) {
    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_OVERHEAD);
    s_petBounceUntilMs = millis() + 1200;
    PetBrain_RequestThought(true);
    return true;
  }

  return true;
}
