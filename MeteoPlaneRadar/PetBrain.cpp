// =============================================================================
//  MeteoPlaneRadar
//  PetBrain.cpp - AI Pet intelligence and Google Gemini LLM integration.
// =============================================================================
#include "PetBrain.h"
#include "Settings.h"
#include "ADSB.h"
#include "Outside.h"
#include "NightMode.h"
#include "PrecipTracker.h"
#include "Net.h"
#include "AsyncCore.h"
#include <ArduinoJson.h>
#include <math.h>

static PetMood  s_mood = PET_MOOD_IDLE;
static char     s_thought[160] = "Hi! Pull me up anytime to scan the skies.";
static bool     s_fetchRequested = false;
static bool     s_isBusy = false;
static unsigned long s_lastAutoThought = 0;
static unsigned long s_moodResetMs = 0;

static const char* CHAR_NAMES[] = {
  "Cyber Eyes",
  "Aero Cat",
  "Radar Dog",
  "DigiCat"
};

// Earth distance calculation in km
static float calcDistKm(double lat1, double lon1, double lat2, double lon2) {
  const float R = 6371.0f;
  float dLat = (float)(lat2 - lat1) * (float)M_PI / 180.0f;
  float dLon = (float)(lon2 - lon1) * (float)M_PI / 180.0f;
  float a = sinf(dLat / 2.0f) * sinf(dLat / 2.0f) +
            cosf((float)lat1 * (float)M_PI / 180.0f) * cosf((float)lat2 * (float)M_PI / 180.0f) *
            sinf(dLon / 2.0f) * sinf(dLon / 2.0f);
  float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
  return R * c;
}

// Earth bearing calculation in degrees (0 = North, clockwise)
static float calcBearingDeg(double lat1, double lon1, double lat2, double lon2) {
  float phi1 = (float)lat1 * (float)M_PI / 180.0f;
  float phi2 = (float)lat2 * (float)M_PI / 180.0f;
  float dLam = (float)(lon2 - lon1) * (float)M_PI / 180.0f;
  float y = sinf(dLam) * cosf(phi2);
  float x = cosf(phi1) * sinf(phi2) - sinf(phi1) * cosf(phi2) * cosf(dLam);
  float deg = atan2f(y, x) * 180.0f / (float)M_PI;
  if (deg < 0) deg += 360.0f;
  return deg;
}

bool PetBrain_GetClosestPlaneTarget(float& bearingDeg, float& distKm, char* outCallsign, size_t callsignCap) {
  bearingDeg = 0.0f;
  distKm = 9999.0f;
  if (outCallsign && callsignCap > 0) outCallsign[0] = '\0';

  if (!Settings_HasLocation()) return false;
  const double myLat = Settings_Lat();
  const double myLon = Settings_Lon();

  const int count = ADSB_Count();
  const Aircraft* list = ADSB_List();
  if (count <= 0 || !list) return false;

  int closestIdx = -1;
  float minD = 99999.0f;

  for (int i = 0; i < count; i++) {
    if (list[i].lat == 0.0f && list[i].lon == 0.0f) continue;
    float d = calcDistKm(myLat, myLon, list[i].lat, list[i].lon);
    if (d < minD) {
      minD = d;
      closestIdx = i;
    }
  }

  if (closestIdx >= 0) {
    distKm = minD;
    bearingDeg = calcBearingDeg(myLat, myLon, list[closestIdx].lat, list[closestIdx].lon);
    if (outCallsign && callsignCap > 0) {
      const char* cs = list[closestIdx].callsign[0] ? list[closestIdx].callsign : list[closestIdx].hex;
      strncpy(outCallsign, cs, callsignCap - 1);
      outCallsign[callsignCap - 1] = '\0';
    }
    return true;
  }
  return false;
}

// Generate an offline contextual thought without internet/LLM
static void generateOfflineThought(char* buf, size_t cap) {
  const uint8_t ch = Settings_PetCharacter();
  float bDeg = 0, dist = 9999.0f;
  char cs[16] = "";
  bool hasPlane = PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs));

  char tempBuf[OUTSIDE_TEXT_MAX] = "";
  Outside_StatusText(tempBuf, sizeof(tempBuf));

  const bool isNight = Settings_IsNight();
  const bool isRaining = (PrecipTracker_IsApproaching() || PrecipTracker_IsCurrentlyActive());

  if (isNight) {
    s_mood = PET_MOOD_SLEEPY;
    if (ch == 1) snprintf(buf, cap, "Purrr... Nap time under the stars. Zzz...");
    else if (ch == 2) snprintf(buf, cap, "Zzz... Guarding the runway in my dreams.");
    else if (ch == 3) snprintf(buf, cap, "Tucking my head under my wing. Goodnight!");
    else snprintf(buf, cap, "Power save mode active. Night radar standby.");
    return;
  }

  if (isRaining) {
    s_mood = PET_MOOD_RAIN;
    if (ch == 1) snprintf(buf, cap, "Rain on the radar! Keep my paws dry!");
    else if (ch == 2) snprintf(buf, cap, "Storm blips approaching! Ready for puddles!");
    else if (ch == 3) snprintf(buf, cap, "Rain outside! Warm purrs and cozy whiskers inside.");
    else snprintf(buf, cap, "Atmospheric precipitation detected nearby.");
    return;
  }

  if (hasPlane && dist < 12.0f) {
    s_mood = PET_MOOD_EXCITED;
    if (ch == 1) snprintf(buf, cap, "Look! %s is right above us! (~%.0fkm)", cs, dist);
    else if (ch == 2) snprintf(buf, cap, "Woof! %s zoomed right overhead!", cs);
    else if (ch == 3) snprintf(buf, cap, "Mrow! Look! %s zoomed right above our ears!", cs);
    else snprintf(buf, cap, "Proximity alert: %s at %.1f km.", cs, dist);
    return;
  }

  if (hasPlane && dist < 45.0f) {
    s_mood = PET_MOOD_HAPPY;
    if (ch == 1) snprintf(buf, cap, "Tracking %s inbound (~%.0fkm).", cs, dist);
    else if (ch == 2) snprintf(buf, cap, "Got eyes on %s on our radar!", cs);
    else if (ch == 3) snprintf(buf, cap, "%s spotted! DigiCat tracking the flight path!", cs);
    else snprintf(buf, cap, "Radar contact: %s bearing %.0f deg.", cs, bDeg);
    return;
  }

  // General idle
  s_mood = PET_MOOD_IDLE;
  static uint8_t s_idleSeq = 0;
  s_idleSeq = (s_idleSeq + 1) % 4;

  if (ch == 1) { // Cat
    const char* catPhrases[] = {
      "Watching the skies for flying birds and jets!",
      "Radar sweeps look so satisfying...",
      "Tap me again if you see a Boeing!",
      "Warm radar, cozy cat. Purrrr."
    };
    snprintf(buf, cap, "%s", catPhrases[s_idleSeq]);
  } else if (ch == 2) { // Dog
    const char* dogPhrases[] = {
      "All systems nominal! Ready to fetch planes!",
      "I'm keeping a sharp eye on our airspace!",
      "Air traffic clear! Tail wagging at 240MHz!",
      "Who's a good radar assistant? I am!"
    };
    snprintf(buf, cap, "%s", dogPhrases[s_idleSeq]);
  } else if (ch == 3) { // DigiCat
    const char* digiCatPhrases[] = {
      "Purrrr... DigiCat is scanning the skies with you!",
      "Watching airplanes zoom by... tail swishing happily!",
      "Virtual happiness high! Cozy radar warm on paws.",
      "Cozy radar screen keeps me company. Purrr meow."
    };
    snprintf(buf, cap, "%s", digiCatPhrases[s_idleSeq]);
  } else { // Cyber
    const char* cyberPhrases[] = {
      "ADS-B telemetry sweep complete. Sector clear.",
      "Dual-core processors locked on navigation beacon.",
      "Sensors active. Ready for airspace interrogation.",
      "All transponder decoders operating normally."
    };
    snprintf(buf, cap, "%s", cyberPhrases[s_idleSeq]);
  }
}

// Generate direct interactive dialogue when the user taps/pets the pet
static void generatePettingThought(char* buf, size_t cap) {
  const uint8_t ch = Settings_PetCharacter();
  s_mood = PET_MOOD_HAPPY;

  static uint8_t s_petSeq = 0;
  s_petSeq = (s_petSeq + 1) % 4;

  if (ch == 1) { // Cat
    const char* phrases[] = {
      "Purrrrr! That feels so good! *happy purrs*",
      "Purrr! Keep petting, the radar can wait!",
      "Meow! Gentle scratches behind my ears... bliss!",
      "Nuzzling your hand! Best co-pilot ever! <3"
    };
    snprintf(buf, cap, "%s", phrases[s_petSeq]);
  } else if (ch == 2) { // Dog
    const char* phrases[] = {
      "Woof! Tail wagging at maximum RPM! <3",
      "Belly rubs! I will guard this radar forever!",
      "Pant pant! You're the best human in the sky!",
      "Happy barks! Airspace clear, time for cuddles!"
    };
    snprintf(buf, cap, "%s", phrases[s_petSeq]);
  } else if (ch == 3) { // DigiCat
    const char* phrases[] = {
      "Purrrrr! *happy head bonk* Best human friend!",
      "Mrow! More chin scratches please! <3",
      "Kneading paws happily! DigiCat loves you!",
      "Purrr purrr! Tail vibrating with joy! <3"
    };
    snprintf(buf, cap, "%s", phrases[s_petSeq]);
  } else { // Cyber Eyes
    const char* phrases[] = {
      "Affection detected: Core temp warm and cozy. <3",
      "Dopamine protocol running at 100% efficiency.",
      "Sensory contact detected: Good human confirmed.",
      "System status: Maximum happiness overload!"
    };
    snprintf(buf, cap, "%s", phrases[s_petSeq]);
  }
}

// Virtual Pet Stats (inspired by DigiCat under MIT License)
static PetStats s_stats = { 92, 18, 0, 0 }; // Initial: 92% happy, 18% hunger, stage 0
static unsigned long s_lastStatTick = 0;
static bool s_lastWasFeed = false;
static bool s_lastWasUserTap = false;
static unsigned long s_lastGeminiReqMs = 0;
static unsigned long s_rateLimitedUntilMs = 0;

PetStats PetBrain_GetStats() {
  return s_stats;
}

const char* PetBrain_GetStageTitle() {
  const uint8_t ch = Settings_PetCharacter();
  if (s_stats.stage == 0) {
    return (ch == 1 || ch == 3) ? "Kitten Cadet" : "Flight Cadet";
  } else if (s_stats.stage == 1) {
    return "Radar Navigator";
  } else {
    return "Airspace Ace";
  }
}

// Generate feeding dialogue when user feeds the pet a treat
static void generateFeedingThought(char* buf, size_t cap) {
  const uint8_t ch = Settings_PetCharacter();
  s_mood = PET_MOOD_HAPPY;

  static uint8_t s_feedSeq = 0;
  s_feedSeq = (s_feedSeq + 1) % 4;

  if (ch == 1) { // Cat
    const char* phrases[] = {
      "Yummm! Radar fish treat devoured! *licks whiskers*",
      "Crunch crunch! Delicious salmon snack! Purrrr!",
      "Nom nom! Energy restored to 100%! <3",
      "Best treat ever! Ready to spot more Boeings!"
    };
    snprintf(buf, cap, "%s", phrases[s_feedSeq]);
  } else if (ch == 2) { // Dog
    const char* phrases[] = {
      "Chomp! Bacon snack caught mid-air! Tail wagging!",
      "Woof! Delicious! Best co-pilot snack ever!",
      "Gulp! That was tasty! Runway patrol energized!",
      "Nom nom nom! High-speed radar dog ready to go!"
    };
    snprintf(buf, cap, "%s", phrases[s_feedSeq]);
  } else if (ch == 3) { // DigiCat
    const char* phrases[] = {
      "Nom nom nom! Crispy fish cracker! Purrrrr!",
      "Crunch! Best cat treat ever! Energy recharged!",
      "Mrow! Licking my whiskers clean! So tasty!",
      "Purrr! Full belly, happy DigiCat ready to watch planes!"
    };
    snprintf(buf, cap, "%s", phrases[s_feedSeq]);
  } else { // Cyber Eyes
    const char* phrases[] = {
      "Energy cell replenished. Power level: 100%.",
      "Battery recharge protocol acknowledged: Yum. <3",
      "Thermal dissipation nominal. Snack accepted.",
      "Capacitor charge 100%. Processing efficiency maxed!"
    };
    snprintf(buf, cap, "%s", phrases[s_feedSeq]);
  }
}

void PetBrain_Feed() {
  s_lastWasFeed = true;
  s_lastWasUserTap = false;
  s_mood = PET_MOOD_HAPPY;
  s_moodResetMs = millis() + 4500;

  // Stats update: reduce hunger, increase happiness
  s_stats.hunger = (s_stats.hunger > 30) ? (s_stats.hunger - 30) : 0;
  s_stats.happiness = (s_stats.happiness <= 85) ? (s_stats.happiness + 15) : 100;

  generateFeedingThought(s_thought, sizeof(s_thought));

  const unsigned long now = millis();
  const char* apiKey = Settings_GeminiApiKey();
  const bool hasKey = (apiKey && strlen(apiKey) >= 10);

  if (!hasKey || now < s_rateLimitedUntilMs || (s_lastGeminiReqMs > 0 && (now - s_lastGeminiReqMs < 15000UL))) {
    return;
  }

  s_fetchRequested = true;
  Async_RequestPetThought();
}

void PetBrain_Init() {
  s_mood = PET_MOOD_IDLE;
  s_fetchRequested = false;
  s_isBusy = false;
  s_lastAutoThought = millis();
  s_lastStatTick = millis();
  s_lastGeminiReqMs = 0;
  s_rateLimitedUntilMs = 0;
  s_lastWasUserTap = false;
  s_lastWasFeed = false;
}

void PetBrain_RequestThought(bool userTapped) {
  s_lastWasUserTap = userTapped;
  s_lastWasFeed = false;
  if (userTapped) {
    s_mood = PET_MOOD_HAPPY;
    s_moodResetMs = millis() + 4500;
    // Tapping increases happiness
    if (s_stats.happiness <= 92) s_stats.happiness += 8;
    else s_stats.happiness = 100;
    // Always give an immediate, instant petting reaction in 0ms!
    generatePettingThought(s_thought, sizeof(s_thought));
  }

  const unsigned long now = millis();
  const char* apiKey = Settings_GeminiApiKey();
  const bool hasKey = (apiKey && strlen(apiKey) >= 10);

  // If no API key or currently rate-limited (HTTP 429) or in debounce cooldown (<15s),
  // return immediately: the user already sees the instant petting thought!
  if (!hasKey || now < s_rateLimitedUntilMs || (s_lastGeminiReqMs > 0 && (now - s_lastGeminiReqMs < 15000UL))) {
    if (!userTapped) {
      generateOfflineThought(s_thought, sizeof(s_thought));
    }
    return;
  }

  s_fetchRequested = true;
  Async_RequestPetThought();
}

PetMood PetBrain_GetMood() {
  return s_mood;
}

void PetBrain_SetMood(PetMood mood) {
  s_mood = mood;
}

const char* PetBrain_GetThought() {
  return s_thought;
}

bool PetBrain_IsBusy() {
  return s_isBusy;
}

void PetBrain_Tick() {
  const unsigned long now = millis();

  // Reset temporary happy mood after tap
  if (s_moodResetMs > 0 && now > s_moodResetMs && s_mood == PET_MOOD_HAPPY) {
    s_moodResetMs = 0;
    s_mood = PET_MOOD_IDLE;
  }

  // Gentle stat progression every 5 minutes (inspired by DigiCat)
  if (now - s_lastStatTick > 300000UL) {
    s_lastStatTick = now;
    if (s_stats.hunger < 100) s_stats.hunger++;
    if (s_stats.happiness > 10) s_stats.happiness--;

    // Check if planes are in airspace to increment experience & rank
    float bDeg = 0, dist = 9999.0f;
    char cs[16] = "";
    if (PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs)) && dist < 45.0f) {
      s_stats.flightsTracked++;
      if (s_stats.happiness < 100) s_stats.happiness++;
      if (s_stats.flightsTracked >= 50) s_stats.stage = 2; // Airspace Ace
      else if (s_stats.flightsTracked >= 15) s_stats.stage = 1; // Radar Navigator
      else s_stats.stage = 0; // Cadet
    }
  }

  // Periodic automatic thought refresh every 8 minutes
  if (now - s_lastAutoThought > 480000UL) {
    s_lastAutoThought = now;
    s_lastWasFeed = false;
    s_lastWasUserTap = false;
    s_fetchRequested = true;
    Async_RequestPetThought();
  }
}

static char s_geminiModel[48] = "gemini-3.8-flash";

static bool discoverModel(const char* apiKey) {
  if (s_geminiModel[0] != '\0') return true;

  String listUrl = "https://generativelanguage.googleapis.com/v1beta/models?key=";
  listUrl += apiKey;

  String body;
  if (!Net_GetString(listUrl.c_str(), body, "GEMINI_MODELS")) {
    Serial.println("PetBrain: Model discovery GET failed, will try candidate list");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("PetBrain: Discovery JSON error: %s\n", err.c_str());
    return false;
  }

  JsonArray models = doc["models"].as<JsonArray>();
  // 1. Look for gemini-3.8-flash first
  for (JsonObject m : models) {
    const char* name = m["name"] | "";
    if (strstr(name, "3.8-flash") != nullptr) {
      const char* cleanName = (strncmp(name, "models/", 7) == 0) ? name + 7 : name;
      strncpy(s_geminiModel, cleanName, sizeof(s_geminiModel) - 1);
      s_geminiModel[sizeof(s_geminiModel) - 1] = '\0';
      Serial.printf("PetBrain: Discovered active 3.8 flash model: %s\n", s_geminiModel);
      return true;
    }
  }

  // 2. Look for active v3 flash model (e.g. gemini-3.6-flash, gemini-3-flash)
  for (JsonObject m : models) {
    const char* name = m["name"] | "";
    JsonArray methods = m["supportedGenerationMethods"].as<JsonArray>();
    bool canGen = false;
    for (JsonVariant v : methods) {
      if (strcmp(v.as<const char*>(), "generateContent") == 0) {
        canGen = true;
        break;
      }
    }
    if (canGen && strstr(name, "flash") != nullptr && (strstr(name, "3.") != nullptr || strstr(name, "3-") != nullptr)) {
      const char* cleanName = (strncmp(name, "models/", 7) == 0) ? name + 7 : name;
      strncpy(s_geminiModel, cleanName, sizeof(s_geminiModel) - 1);
      s_geminiModel[sizeof(s_geminiModel) - 1] = '\0';
      Serial.printf("PetBrain: Discovered active v3 flash model: %s\n", s_geminiModel);
      return true;
    }
  }

  return false;
}

// Background worker called from Core 0
bool PetBrain_Step() {
  if (!s_fetchRequested) return false;
  s_fetchRequested = false;
  s_isBusy = true;

  const unsigned long now = millis();
  const char* apiKey = Settings_GeminiApiKey();

  // If no Gemini API key configured, use the smart offline engine
  if (!apiKey || strlen(apiKey) < 10) {
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return true;
  }

  // Rate-limit backoff guard (HTTP 429 cooldown)
  if (now < s_rateLimitedUntilMs) {
    Serial.printf("PetBrain: Rate-limit backoff (%lu s left), using offline thought\n",
                  (s_rateLimitedUntilMs - now) / 1000);
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return true;
  }

  // 15s throttle guard to strictly obey 5 RPM Free Tier limit
  if (s_lastGeminiReqMs > 0 && (now - s_lastGeminiReqMs < 15000UL)) {
    Serial.println("PetBrain: 15s throttle active, using offline thought");
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return true;
  }

  // Auto-discover the working model name for this specific key
  if (s_geminiModel[0] == '\0') {
    discoverModel(apiKey);
  }

  // Context preparation for Gemini
  const uint8_t ch = Settings_PetCharacter();
  const char* charName = (ch < 4) ? CHAR_NAMES[ch] : "Cyber Eyes";

  float bDeg = 0, dist = 9999.0f;
  char cs[16] = "";
  bool hasPlane = PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs));

  char tempBuf[OUTSIDE_TEXT_MAX] = "";
  Outside_StatusText(tempBuf, sizeof(tempBuf));
  const bool isRaining = (PrecipTracker_IsApproaching() || PrecipTracker_IsCurrentlyActive());
  const bool isNight = Settings_IsNight();

  // Construct prompt
  char prompt[384];
  if (s_lastWasFeed) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). "
      "The user just fed you a delicious treat! "
      "Say one very short, cute or funny reaction (max 10 words) in English. No quotes, no hashtags.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger);
  } else if (s_lastWasUserTap) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). "
      "The user just lovingly petted and tapped you on the touchscreen! "
      "Say one very short, cute, loving or funny reaction (max 10 words) in English directly to the user. No quotes, no hashtags.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger);
  } else if (hasPlane && dist < 30.0f) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). Ambient: %s, rain=%s, night=%s. "
      "Nearest flight is %s at %.0fkm distance. "
      "Say one very short, cute or witty sentence (max 12 words) in English about this. No quotes, no hashtags.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger,
      tempBuf[0] ? tempBuf : "unknown",
      isRaining ? "yes" : "no", isNight ? "yes" : "no",
      cs, dist);
  } else {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). "
      "Ambient: %s, rain=%s, night=%s, skies clear. "
      "Say one very short, cute or witty remark (max 12 words) in English. No quotes, no hashtags.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger,
      tempBuf[0] ? tempBuf : "unknown",
      isRaining ? "yes" : "no", isNight ? "yes" : "no");
  }

  // Build JSON request payload
  JsonDocument reqDoc;
  JsonArray contents = reqDoc["contents"].to<JsonArray>();
  JsonObject part = contents.add<JsonObject>()["parts"].to<JsonArray>().add<JsonObject>();
  part["text"] = prompt;

  JsonObject genConfig = reqDoc["generationConfig"].to<JsonObject>();
  genConfig["temperature"] = 0.8;
  genConfig["maxOutputTokens"] = 45;

  String jsonBody;
  serializeJson(reqDoc, jsonBody);

  // Candidate models list: prefer gemini-3.8-flash first
  const char* candidates[4];
  int candCount = 0;
  if (s_geminiModel[0] != '\0') candidates[candCount++] = s_geminiModel;
  candidates[candCount++] = "gemini-3.8-flash";
  candidates[candCount++] = "gemini-3.6-flash";
  candidates[candCount++] = "gemini-3-flash";

  bool success = false;
  String respBody;

  for (int i = 0; i < candCount; i++) {
    // Avoid re-trying identical model name twice
    if (i > 0 && strcmp(candidates[i], candidates[0]) == 0) continue;

    String url = "https://generativelanguage.googleapis.com/v1beta/models/";
    url += candidates[i];
    url += ":generateContent?key=";
    url += apiKey;

    int statusCode = 0;
    if (Net_PostJson(url.c_str(), jsonBody.c_str(), respBody, "GEMINI", &statusCode)) {
      s_lastGeminiReqMs = millis();
      // Remember working model for subsequent requests
      if (strcmp(s_geminiModel, candidates[i]) != 0) {
        strncpy(s_geminiModel, candidates[i], sizeof(s_geminiModel) - 1);
        s_geminiModel[sizeof(s_geminiModel) - 1] = '\0';
        Serial.printf("PetBrain: Successfully connected using model: %s\n", s_geminiModel);
      }
      success = true;
      break;
    } else {
      if (statusCode == 429) {
        Serial.println("PetBrain: Quota exceeded (HTTP 429). Rate-limited for 45s, stopping candidate scan.");
        s_rateLimitedUntilMs = millis() + 45000UL;
        break; // Stop immediately, do not exhaust quota on subsequent candidate models!
      }
    }
  }

  if (!success) {
    Serial.println("PetBrain: All Gemini model requests failed, using offline fallback");
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return false;
  }

  // Parse Gemini response
  JsonDocument respDoc;
  DeserializationError err = deserializeJson(respDoc, respBody);
  if (err) {
    Serial.printf("PetBrain: JSON error: %s\n", err.c_str());
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return false;
  }

  const char* reply = respDoc["candidates"][0]["content"]["parts"][0]["text"] | "";
  if (reply && strlen(reply) > 0) {
    // Strip leading/trailing quotes and newlines
    const char* src = reply;
    while (*src == ' ' || *src == '\"' || *src == '\'' || *src == '\n' || *src == '\r') src++;

    size_t len = strlen(src);
    while (len > 0 && (src[len - 1] == ' ' || src[len - 1] == '\"' || src[len - 1] == '\'' ||
                       src[len - 1] == '\n' || src[len - 1] == '\r')) {
      len--;
    }

    if (len > sizeof(s_thought) - 1) len = sizeof(s_thought) - 1;
    strncpy(s_thought, src, len);
    s_thought[len] = '\0';
    s_isBusy = false;
    return true;
  }

  generateOfflineThought(s_thought, sizeof(s_thought));
  s_isBusy = false;
  return true;
}
