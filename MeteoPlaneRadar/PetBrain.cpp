// =============================================================================
//  MeteoPlaneRadar
//  PetBrain.cpp - AI Pet intelligence and Google Gemini LLM integration.
// =============================================================================
#include "PetBrain.h"
#include "Display_ST7701.h"
#include <Preferences.h>
#include <Arduino.h>
#include "Settings.h"
#include "ADSB.h"
#include "Outside.h"
#include "NightMode.h"
#include "PrecipTracker.h"
#include "Net.h"
#include "AsyncCore.h"
#include "Lang.h"
#include "PetDrawer.h"
#include "Forecast.h"
#include "IssData.h"
#include "DisasterAlert.h"
#include <ArduinoJson.h>
#include "SpiRamAllocator.h"
#include <Preferences.h>
#include <math.h>

static PetMood  s_mood = PET_MOOD_IDLE;
static char     s_thought[192] = "MeteoPlaneRadar Pet AI online.";
static bool     s_fetchRequested = false;
static bool     s_isBusy = false;
static unsigned long s_lastAutoThought = 0;
static unsigned long s_moodResetMs = 0;

// Virtual Pet Stats (inspired by DigiCat under MIT License)
static PetStats s_stats = { 92, 18, 0, 0 }; // Initial: 92% happy, 18% hunger, stage 0
static unsigned long s_lastStatTick = 0;
static bool s_lastWasFeed = false;
static bool s_lastWasUserTap = false;
static unsigned long s_lastGeminiReqMs = 0;
static unsigned long s_rateLimitedUntilMs = 0;
static bool s_statsDirty = false;
static unsigned long s_statsDirtyMs = 0;


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
  float b = atan2f(y, x) * 180.0f / (float)M_PI;
  if (b < 0.0f) b += 360.0f;
  return b;
}

bool PetBrain_GetClosestPlaneTarget(float& bearingDeg, float& distKm, char* outCallsign, size_t callsignCap, SkyPlaneType& outType) {
  bearingDeg = 0.0f;
  distKm = 9999.0f;
  if (outCallsign && callsignCap > 0) outCallsign[0] = '\0';

  if (!Settings_HasLocation()) return false;
  const double myLat = Settings_Lat();
  const double myLon = Settings_Lon();

  Async_LockAdsb();
  const int count = ADSB_Count();
  const Aircraft* list = ADSB_List();
  if (count <= 0 || !list) {
    Async_UnlockAdsb();
    return false;
  }

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
    outType = PLANE_TYPE_JET;
    const Aircraft& ac = list[closestIdx];
    if (ac.isMilitary) {
      outType = PLANE_TYPE_FIGHTER;
    } else {
      String t = String(ac.type);
      String csStr = String(ac.callsign);
      t.toUpperCase();
      csStr.toUpperCase();
      
      bool isHeli = (t == "EC35" || t == "EC45" || t == "R44" || t == "A109" || t == "B429" || t == "H60" || t == "UH60" || t == "AS32");
      bool isRescue = isHeli && (csStr.indexOf("POL") >= 0 || csStr.indexOf("HEMS") >= 0 || csStr.indexOf("KRYSTOF") >= 0 || csStr.indexOf("RESCUE") >= 0);
      
      if (isRescue) {
        outType = PLANE_TYPE_RESCUE;
      } else if (isHeli) {
        outType = PLANE_TYPE_HELI;
      } else if (t.startsWith("B77") || t.startsWith("B78") || t.startsWith("A33") || t.startsWith("A35") || t.startsWith("A38") || t.startsWith("B74")) {
        outType = PLANE_TYPE_HEAVY;
      } else if (t.startsWith("C172") || t.startsWith("DA40") || t.startsWith("SR22") || t.startsWith("P28") || (ac.altFt > 0 && ac.altFt < 10000 && ac.gsKt > 0 && ac.gsKt < 150)) {
        outType = PLANE_TYPE_SMALL;
      }
    }
    Async_UnlockAdsb();
    return true;
  }
  Async_UnlockAdsb();
  return false;
}

// Generate an offline contextual thought without internet/LLM
static void generateOfflineThought(char* buf, size_t cap) {
  const uint8_t lang = Settings_Language();
  float bDeg = 0, dist = 9999.0f;
  char cs[16] = "";
  SkyPlaneType pType = PLANE_TYPE_JET;
  bool hasPlane = PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs), pType);

  const bool isNight = Settings_IsNight() && !PetDrawer_IsNightAwake();
  const PrecipAlert* alert = PrecipTracker_GetAlert();
  bool isApproaching = (alert && alert->status == PRECIP_STAT_APPROACHING);
  bool isActive = (alert && alert->status == PRECIP_STAT_CURRENTLY_ACTIVE);

  // 1. Seismic / Disaster Alert
  if (DisasterAlert_IsLocalTremorActive() || DisasterAlert_HasActiveAlert()) {
    s_mood = PET_MOOD_SCARED;
    if (lang == LANG_SK) snprintf(buf, cap, "Hiss! Cítim chvenie zeme v labkách! Seizmický poplach!");
    else if (lang == LANG_CZ) snprintf(buf, cap, "Hiss! Cítím chvění země v tlapkách! Seismický poplach!");
    else snprintf(buf, cap, "Hiss! My whiskers feel seismic vibrations! Tremor alert!");
    return;
  }

  // 2. Sleeping at night
  if (isNight) {
    s_mood = PET_MOOD_SLEEPY;
    if (lang == LANG_SK) snprintf(buf, cap, "Prrrr... DigiCat spinká pod hviezdami. Zzz...");
    else if (lang == LANG_CZ) snprintf(buf, cap, "Prrrr... DigiCat spinká pod hvězdami. Zzz...");
    else snprintf(buf, cap, "Purrr... DigiCat sleep time under the stars. Zzz...");
    return;
  }

  // 3. Hungry tummy check
  if (s_stats.hunger >= 75) {
    s_mood = PET_MOOD_IDLE;
    if (lang == LANG_SK) snprintf(buf, cap, "Mňau... bruško mi škvŕka (%d%% hlad)! Prosím chrumkavú rybičku!", s_stats.hunger);
    else if (lang == LANG_CZ) snprintf(buf, cap, "Mňau... v bříšku mi kručí (%d%% hlad)! Prosím křupavou rybičku!", s_stats.hunger);
    else snprintf(buf, cap, "Mrow... tummy rumbling (%d%% hunger)! Toss a crispy fish treat?", s_stats.hunger);
    return;
  }

  // 4. Precipitation / Storm alerts
  if (isApproaching) {
    s_mood = PET_MOOD_RAIN;
    if (lang == LANG_SK) snprintf(buf, cap, "Pozor! Búrka sa blíži! Podľa môjho výpočtu dorazí o %d minút!", alert->etaMin);
    else if (lang == LANG_CZ) snprintf(buf, cap, "Pozor! Bouřka se blíží! Podle mého výpočtu dorazí za %d minut!", alert->etaMin);
    else snprintf(buf, cap, "Alert! Storm approaching! My tactical tracking says ETA %d minutes!", alert->etaMin);
    return;
  }
  
  if (isActive) {
    s_mood = PET_MOOD_RAIN;
    if (lang == LANG_SK) snprintf(buf, cap, "Vonku prší! Dáždnik nad hlavou a teplé pradenie pri radare.");
    else if (lang == LANG_CZ) snprintf(buf, cap, "Venku prší! Deštník nad hlavou a teplé předení u radaru.");
    else snprintf(buf, cap, "Rain outside! Holding my umbrella with warm purrs inside.");
    return;
  }

  // 5. Military fighter intercept
  if (hasPlane && pType == PLANE_TYPE_FIGHTER && dist < 65.0f) {
    s_mood = PET_MOOD_MILITARY;
    if (lang == LANG_SK) snprintf(buf, cap, "Poplach! Stíhačka %s na radare (%.0fkm)! DigiCat nasadzuje letecké okuliare!", cs, dist);
    else if (lang == LANG_CZ) snprintf(buf, cap, "Poplach! Stíhačka %s na radaru (%.0fkm)! DigiCat nasazuje letecké brýle!", cs, dist);
    else snprintf(buf, cap, "Intercept alert! Fighter %s at %.0fkm! DigiCat locks tactical radar!", cs, dist);
    return;
  }

  // 6. Close overhead aircraft (with type awareness)
  if (hasPlane && dist <= 10.0f) {
    s_mood = PET_MOOD_EXCITED;
    if (pType == PLANE_TYPE_RESCUE || pType == PLANE_TYPE_HELI) {
      if (lang == LANG_SK) snprintf(buf, cap, "Vrtuľník %s bzučí priamo nad nami (%.0fkm)! Ušká v pozore! 🚁🐾", cs, dist);
      else if (lang == LANG_CZ) snprintf(buf, cap, "Vrtulník %s bzučí přímo nad námi (%.0fkm)! Ouška v pozoru! 🚁🐾", cs, dist);
      else snprintf(buf, cap, "Chopper %s buzzing right overhead (%.0fkm)! Ears swiveling! 🚁🐾", cs, dist);
    } else if (pType == PLANE_TYPE_HEAVY) {
      if (lang == LANG_SK) snprintf(buf, cap, "Obrie lietadlo %s priamo nad hlavou! Aký obrovský tieň! ✈️🐾", cs);
      else if (lang == LANG_CZ) snprintf(buf, cap, "Obří letadlo %s přímo nad hlavou! Jaký obrovský stín! ✈️🐾", cs);
      else snprintf(buf, cap, "Heavy jumbo %s roaring overhead! Look at those contrails! ✈️🐾", cs);
    } else {
      if (lang == LANG_SK) snprintf(buf, cap, "Mňau! Pozri hore! %s fičí nad nami! Skáčem a chytám ho labkou! ✈️🐾", cs);
      else if (lang == LANG_CZ) snprintf(buf, cap, "Mňau! Koukej nahoru! %s fičí nad námi! Skáču a chytám ho tlapkou! ✈️🐾", cs);
      else snprintf(buf, cap, "Mrow! Look up! %s zooming right above! Swatting paws at it! ✈️🐾", cs);
    }
    return;
  }

  // 7. ISS Space Station overhead pass
  const IssData* iss = Iss_GetDataPtr();
  if (iss && iss->valid && iss->inRange) {
    s_mood = PET_MOOD_EXCITED;
    if (lang == LANG_SK) snprintf(buf, cap, "Pozri na hviezdy! Vesmírna stanica ISS letí nad nami (%.0fkm)! 🛰️🐾", iss->distanceKm);
    else if (lang == LANG_CZ) snprintf(buf, cap, "Koukej na hvězdy! Vesmírná stanice ISS letí nad námi (%.0fkm)! 🛰️🐾", iss->distanceKm);
    else snprintf(buf, cap, "Look up! The ISS space station is passing overhead (%.0fkm)! 🛰️🐾", iss->distanceKm);
    return;
  }

  // 8. Medium-range aircraft on radar
  if (hasPlane && dist < 45.0f) {
    s_mood = PET_MOOD_HAPPY;
    if (lang == LANG_SK) snprintf(buf, cap, "%s na radare (%.0fkm). DigiCat sleduje jeho let! 📡🐾", cs, dist);
    else if (lang == LANG_CZ) snprintf(buf, cap, "%s na radaru (%.0fkm). DigiCat sleduje jeho let! 📡🐾", cs, dist);
    else snprintf(buf, cap, "%s on radar (%.0fkm). DigiCat tracking flight path! 📡🐾", cs, dist);
    return;
  }

  // 9. General idle with outdoor temperature awareness
  s_mood = PET_MOOD_IDLE;
  static uint8_t s_idleSeq = 0;
  s_idleSeq = (s_idleSeq + 1) % 8;

  if (s_idleSeq == 6 && Forecast_CurrentValid()) {
    float t = Forecast_CurrentTemp();
    if (t <= 2.0f) {
      if (lang == LANG_SK) snprintf(buf, cap, "Vonku je %.1f°C! Ešteže mám teplý červený šál a horúci radar!", t);
      else if (lang == LANG_CZ) snprintf(buf, cap, "Venku je %.1f°C! Ještě že mám teplou šálu a horký radar!", t);
      else snprintf(buf, cap, "Brrr, %.1f°C outside! Cozy knit scarf keeps my whiskers warm!", t);
      return;
    } else if (t >= 23.0f) {
      if (lang == LANG_SK) snprintf(buf, cap, "Slnečných %.1f°C! Vyhrievam si kožúšok na teplej dráhe!", t);
      else if (lang == LANG_CZ) snprintf(buf, cap, "Slunečných %.1f°C! Vyhřívám si kožíšek na teplé dráze!", t);
      else snprintf(buf, cap, "Sunny %.1f°C! Sunbathing my ginger fur on the warm tarmac!", t);
      return;
    }
  }

  const char* phrasesEN[8] = {
    "Purrrrr... DigiCat is scanning the skies with you!",
    "Watching airplanes zoom by... tail swishing happily!",
    "Drag your finger on the runway to shine my laser pointer!",
    "Cozy radar screen keeps me company. Purrr meow.",
    "Ears tuned to 1090 MHz ADS-B! Ready for the next jet!",
    "Paws clean, whiskers calibrated, radar sweeping smoothly!",
    "Tap my ears for scratches or toss a fish cracker!",
    "Stalking runway butterflies while airspace is quiet..."
  };
  const char* phrasesSK[8] = {
    "Prrrrr... DigiCat s tebou skenuje oblohu!",
    "Sledujem lietadlá na radare... chvostík vrtí radosťou!",
    "Potiahni prstom po dráhe a zasvieť mi laserovým ukazovátkom!",
    "Pracujem na radare a pradkám. Mňau!",
    "Ušká naladené na 1090 MHz ADS-B! Čakám na ďalšie lietadlo!",
    "Labky umyté, fúziky nakalibrované, radar beží hladko!",
    "Ťukni mi na uško pre poškrabkanie alebo hoď rybičku!",
    "Sledujem motýle pri dráhe, kým je na oblohe pokoj..."
  };
  const char* phrasesCZ[8] = {
    "Prrrrr... DigiCat s tebou skenuje oblohu!",
    "Sleduji letadla na radaru... ocásek vrtí radostí!",
    "Potáhni prstem po dráze a posviť mi laserovým ukazovátkem!",
    "Pracuji na radaru a předu. Mňau!",
    "Ouška naladěna na 1090 MHz ADS-B! Čekám na další letadlo!",
    "Tlapky umyté, vousky zkalibrované, radar běží hladce!",
    "Klepni mi na ouško pro podrbání nebo hoď rybičku!",
    "Pozoruji motýly u dráhy, dokud je na obloze klid..."
  };
  const char** pTable = (lang == LANG_SK) ? phrasesSK : ((lang == LANG_CZ) ? phrasesCZ : phrasesEN);
  snprintf(buf, cap, "%s", pTable[s_idleSeq]);
}

// Generate direct interactive dialogue when the user taps/pets the pet
static void generatePettingThought(char* buf, size_t cap) {
  const uint8_t lang = Settings_Language();
  s_mood = PET_MOOD_HAPPY;

  static uint8_t s_petSeq = 0;
  s_petSeq = (s_petSeq + 1) % 6;

  const char* phrasesEN[6] = {
    "Purrrrr! *happy head bonk* Best human co-pilot!",
    "Mrow! More chin scratches please! <3",
    "Kneading biscuits on the runway! DigiCat loves you!",
    "Purrr purrr! Tail vibrating like a turboprop! <3",
    "Slow-blinking at you with big happy cat eyes! <3",
    "So warm and cozy! Best radar crew in the world!"
  };
  const char* phrasesSK[6] = {
    "Prrrrrr! *šťastné štuchnutie hlávkou* Najlepší kopilot!",
    "Mňau! Ešte poškrabkať pod bradičkou, prosím! <3",
    "Prešľapujem labkami od radosti! DigiCat ťa ľúbi!",
    "Prrr prrr! Chvostík vibruje ako vrtuľa lietadla! <3",
    "Pomaly na teba žmurkám veľkými mačacími očkami! <3",
    "Také príjemné hladkanie! Najlepšia posádka radaru!"
  };
  const char* phrasesCZ[6] = {
    "Prrrrrr! *šťastné drcnutí hlavičkou* Nejlepší kopilot!",
    "Mňau! Ještě podrbat pod bradičkou, prosím! <3",
    "Přešlapuji pacičkami radostí! DigiCat tě má rád!",
    "Prrr prrr! Ocásek vibruje jako vrtule letadla! <3",
    "Pomalu na tebe mrkám velkýma kočičíma očima! <3",
    "Takové příjemné hlazení! Nejlepší posádka radaru!"
  };
  const char** pTable = (lang == LANG_SK) ? phrasesSK : ((lang == LANG_CZ) ? phrasesCZ : phrasesEN);
  snprintf(buf, cap, "%s", pTable[s_petSeq]);
}

PetStats PetBrain_GetStats() {
  return s_stats;
}

const char* PetBrain_GetStageTitle() {
  const uint8_t lang = Settings_Language();
  if (s_stats.stage == 0) {
    return (lang == LANG_SK) ? "Mačací kadet" : ((lang == LANG_CZ) ? "Kočičí kadet" : "Kitten Cadet");
  } else if (s_stats.stage == 1) {
    return (lang == LANG_SK || lang == LANG_CZ) ? "Radarový navigátor" : "Radar Navigator";
  } else {
    return (lang == LANG_SK || lang == LANG_CZ) ? "Letecké eso" : "Airspace Ace";
  }
}

void PetBrain_AwardXP(uint16_t pts) {
  s_stats.flightsTracked += pts;
  if (s_stats.flightsTracked >= 50) s_stats.stage = 2; // Airspace Ace
  else if (s_stats.flightsTracked >= 15) s_stats.stage = 1; // Radar Navigator
  else s_stats.stage = 0; // Cadet

  if (s_stats.happiness < 100) s_stats.happiness++;
  s_statsDirty = true;
  s_statsDirtyMs = millis();
}

// Generate feeding dialogue when user feeds the pet a treat
static void generateFeedingThought(char* buf, size_t cap) {
  const uint8_t lang = Settings_Language();
  s_mood = PET_MOOD_HAPPY;

  static uint8_t s_feedSeq = 0;
  s_feedSeq = (s_feedSeq + 1) % 6;

  const char* phrasesEN[6] = {
    "Nom nom nom! Crispy salmon cracker! Purrrrr!",
    "Crunch! Best aviation cat treat ever! Tank refueled!",
    "Mrow! Licking every crumb off my whiskers! So tasty!",
    "Purrr! Full belly, happy DigiCat ready to watch planes!",
    "Yum! Caught that fish snack mid-air! 10/10 landing!",
    "Crunchy perfection! My radar senses are supercharged!"
  };
  const char* phrasesSK[6] = {
    "Mňam mňam mňam! Chrumkavá lososová rybička! Prrrrrr!",
    "Chrum! Najlepšia letecká maškrta! Nádrž dotankovaná!",
    "Mňau! Oblizujem každú omrvinku z fúzikov! To bolo chutné!",
    "Prrr! Plné bruško, spokojná mačička pripravená na radary!",
    "Mňam! Chytil som tú rybičku na prvé skočenie! 10/10!",
    "Chrumkavá dokonalosť! Moje radarové zmysly sú nabité!"
  };
  const char* phrasesCZ[6] = {
    "Mňam mňam mňam! Křupavá lososová rybička! Prrrrrr!",
    "Křup! Nejlepší letecký pamlsek! Nádrž dotankována!",
    "Mňau! Olizuji každou drobku z vousků! To bylo skvělé!",
    "Prrr! Plné bříško, spokojená kočička připravena na radary!",
    "Mňam! Chytil jsem tu rybičku na první skočení! 10/10!",
    "Křupavá dokonalost! Moje radarové smysly jsou nabité!"
  };
  const char** pTable = (lang == LANG_SK) ? phrasesSK : ((lang == LANG_CZ) ? phrasesCZ : phrasesEN);
  snprintf(buf, cap, "%s", pTable[s_feedSeq]);
}

void PetBrain_Feed() {
  s_lastWasFeed = true;
  s_lastWasUserTap = false;
  s_mood = PET_MOOD_HAPPY;
  s_moodResetMs = millis() + 4500;

  // Stats update: reduce hunger, increase happiness
  s_stats.hunger = (s_stats.hunger > 30) ? (s_stats.hunger - 30) : 0;
  s_stats.happiness = (s_stats.happiness <= 85) ? (s_stats.happiness + 15) : 100;
  s_statsDirty = true;
  s_statsDirtyMs = millis();

  // Immediate feeding reaction in 0ms!
  generateFeedingThought(s_thought, sizeof(s_thought));

  const unsigned long now = millis();
  const char* apiKey = Settings_GeminiApiKey();
  const bool hasKey = (apiKey && strlen(apiKey) >= 10);

  if (!hasKey || now < s_rateLimitedUntilMs || (s_lastGeminiReqMs > 0 && (now - s_lastGeminiReqMs < 20000UL))) {
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

  // Restore persisted stats from NVS
  Preferences prefs;
  if (prefs.begin("digicat", true)) {
    s_stats.happiness = prefs.getUChar("happy", 92);
    s_stats.hunger = prefs.getUChar("hunger", 18);
    s_stats.stage = prefs.getUChar("stage", 0);
    s_stats.flightsTracked = prefs.getUShort("xp", 0);
    prefs.end();
  }

  generateOfflineThought(s_thought, sizeof(s_thought));
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
    s_statsDirty = true;
    s_statsDirtyMs = millis();
    // Always give an immediate, instant petting reaction in 0ms!
    generatePettingThought(s_thought, sizeof(s_thought));
  }

  const unsigned long now = millis();
  const char* apiKey = Settings_GeminiApiKey();
  const bool hasKey = (apiKey && strlen(apiKey) >= 10);

  // If no API key or currently rate-limited (HTTP 429) or in debounce cooldown (<20s),
  // offline thought was already generated synchronously in 0ms.
  if (!hasKey || now < s_rateLimitedUntilMs || (s_lastGeminiReqMs > 0 && (now - s_lastGeminiReqMs < 20000UL))) {
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

  // Reset temporary happy/scared mood after interaction or tilt
  if (s_moodResetMs > 0 && now > s_moodResetMs && (s_mood == PET_MOOD_HAPPY || s_mood == PET_MOOD_SCARED)) {
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
    SkyPlaneType dummyType;
    if (PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs), dummyType) && dist < 45.0f) {
      PetBrain_AwardXP(1);
    } else {
      s_statsDirty = true;
      s_statsDirtyMs = now;
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

  // Debounced persist of pet stats to NVS (15 minutes to reduce flash wear)
  if (s_statsDirty && (now - s_statsDirtyMs >= 900000UL)) {
    s_statsDirty = false;
    Preferences prefs;
    if (prefs.begin("digicat", false)) {
      prefs.putUChar("happy", s_stats.happiness);
      prefs.putUChar("hunger", s_stats.hunger);
      prefs.putUChar("stage", s_stats.stage);
      prefs.putUShort("xp", s_stats.flightsTracked);
      prefs.end();
    }
  }
}

static char s_geminiModel[48] = "";

static bool discoverModel(const char* apiKey) {
  if (s_geminiModel[0] != '\0') return true;

  String listUrl = "https://generativelanguage.googleapis.com/v1beta/models?key=";
  listUrl += apiKey;

  String body;
  if (!Net_GetString(listUrl.c_str(), body, "GEMINI_MODELS")) {
    Serial.println("PetBrain: Model discovery GET failed, will try candidate list");
    return false;
  }

  JsonDocument doc(SpiRamAllocator::instance());
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("PetBrain: Discovery JSON error: %s\n", err.c_str());
    return false;
  }

  JsonArray models = doc["models"].as<JsonArray>();
  // Look for models supporting generateContent in preferred priority:
  const char* preferredPatterns[] = { "3.8-flash", "3.5-flash-lite", "3.5-flash", "flash-lite", "flash" };
  for (const char* pat : preferredPatterns) {
    for (JsonObject m : models) {
      const char* name = m["name"] | "";
      if (strstr(name, pat) == nullptr) continue;

      JsonArray methods = m["supportedGenerationMethods"].as<JsonArray>();
      bool canGen = false;
      for (JsonVariant v : methods) {
        if (strcmp(v.as<const char*>(), "generateContent") == 0) {
          canGen = true;
          break;
        }
      }
      if (canGen) {
        const char* cleanName = (strncmp(name, "models/", 7) == 0) ? name + 7 : name;
        strncpy(s_geminiModel, cleanName, sizeof(s_geminiModel) - 1);
        s_geminiModel[sizeof(s_geminiModel) - 1] = '\0';
        Serial.printf("PetBrain: Discovered active Gemini model: %s\n", s_geminiModel);
        return true;
      }
    }
  }

  return false;
}

// Background worker called from Core 0
bool PetBrain_Step() {
  if (!s_fetchRequested) return false;
  s_fetchRequested = false;

  const unsigned long now = millis();
  const char* apiKey = Settings_GeminiApiKey();

  // If no Gemini API key configured, use the smart offline engine
  if (!apiKey || strlen(apiKey) < 10) {
    if (s_lastWasFeed) {
      generateFeedingThought(s_thought, sizeof(s_thought));
    } else if (s_lastWasUserTap) {
      generatePettingThought(s_thought, sizeof(s_thought));
    } else {
      generateOfflineThought(s_thought, sizeof(s_thought));
    }
    s_isBusy = false;
    return true;
  }

  // Rate-limit guard: if we got HTTP 429, don't spam Google servers
  if (now < s_rateLimitedUntilMs) {
    Serial.println("PetBrain: Rate-limited cooldown active, using offline thought");
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return true;
  }

  // 20s throttle guard to strictly obey Free Tier RPM limit
  if (s_lastGeminiReqMs > 0 && (now - s_lastGeminiReqMs < 20000UL)) {
    Serial.println("PetBrain: 20s throttle active, using offline thought");
    generateOfflineThought(s_thought, sizeof(s_thought));
    s_isBusy = false;
    return true;
  }

  // Ensure no lingering tile/weather session holds internal SRAM before making the LLM request
  Net_SessionEnd();
  delay(100);

  // Context preparation for Gemini
  const char* charName = "DigiCat";
  const uint8_t curLang = Settings_Language();
  const char* langDirective = (curLang == LANG_SK) ? "strictly in Slovak (slovenčina)" :
                              ((curLang == LANG_CZ) ? "strictly in Czech (čeština)" : "in English");

  float bDeg = 0, dist = 9999.0f;
  char cs[16] = "";
  SkyPlaneType dummyType;
  bool hasPlane = PetBrain_GetClosestPlaneTarget(bDeg, dist, cs, sizeof(cs), dummyType);

  char tempBuf[OUTSIDE_TEXT_MAX] = "";
  Outside_StatusText(tempBuf, sizeof(tempBuf));
  const bool isRaining = (PrecipTracker_IsApproaching() || PrecipTracker_IsCurrentlyActive());
  const bool isNight = Settings_IsNight() && !PetDrawer_IsNightAwake();

  // Construct prompt
  char prompt[420];
  if (s_lastWasFeed) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). "
      "The user just fed you a delicious treat! "
      "Say one very short, cute or funny reaction (max 10 words) %s. Plain text only, no quotes, no hashtags, no asterisks.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger, langDirective);
  } else if (s_lastWasUserTap) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). "
      "The user just lovingly petted and tapped you on the touchscreen! "
      "Say one very short, cute, loving or funny reaction (max 10 words) %s directly to the user. Plain text only, no quotes, no hashtags, no asterisks.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger, langDirective);
  } else if (hasPlane && dummyType == PLANE_TYPE_FIGHTER && dist < 65.0f) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an elite Top-Gun combat cat co-pilot (%s, Happiness: %d%%). "
      "A fast military supersonic fighter jet %s is intercepted at %.0fkm! "
      "Say one very short, badass, cute military co-pilot line (max 10 words) %s with aviator sunglasses. Plain text only, no quotes, no hashtags, no asterisks.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, cs, dist, langDirective);
  } else if (hasPlane && dist <= 10.0f) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation cat pet (%s, Happiness: %d%%, Hunger: %d%%). Ambient: %s, rain=%s, night=%s. "
      "Nearest flight is %s at %.0fkm flying very close overhead right above you! "
      "Say one very short, cute or witty cat remark (max 12 words) %s about chasing or swatting/scratching at it. Plain text only, no quotes, no hashtags, no asterisks.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger,
      tempBuf[0] ? tempBuf : "unknown",
      isRaining ? "yes" : "no", isNight ? "yes" : "no",
      cs, dist, langDirective);
  } else if (hasPlane && dist < 40.0f) {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation cat pet (%s, Happiness: %d%%, Hunger: %d%%). Ambient: %s, rain=%s, night=%s. "
      "Nearest flight is %s at %.0fkm distance on radar. "
      "Say one very short, cute or witty remark (max 12 words) %s about tracking it on radar. Plain text only, no quotes, no hashtags, no asterisks.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger,
      tempBuf[0] ? tempBuf : "unknown",
      isRaining ? "yes" : "no", isNight ? "yes" : "no",
      cs, dist, langDirective);
  } else {
    snprintf(prompt, sizeof(prompt),
      "You are %s, an aviation pet (%s, Happiness: %d%%, Hunger: %d%%). "
      "Ambient: %s, rain=%s, night=%s, skies clear. "
      "Say one very short, cute or witty remark (max 12 words) %s. Plain text only, no quotes, no hashtags, no asterisks.",
      charName, PetBrain_GetStageTitle(), s_stats.happiness, s_stats.hunger,
      tempBuf[0] ? tempBuf : "unknown",
      isRaining ? "yes" : "no", isNight ? "yes" : "no",
      langDirective);
  }

  // Build JSON request payload
  JsonDocument reqDoc(SpiRamAllocator::instance());
  JsonArray contents = reqDoc["contents"].to<JsonArray>();
  JsonObject part = contents.add<JsonObject>()["parts"].to<JsonArray>().add<JsonObject>();
  part["text"] = prompt;

  JsonObject genConfig = reqDoc["generationConfig"].to<JsonObject>();
  genConfig["temperature"] = 0.8;
  genConfig["maxOutputTokens"] = 45;

  String jsonBody;
  serializeJson(reqDoc, jsonBody);

  // Candidate models list: prioritize gemini-3.5-flash-lite for instant response and quota stability
  const char* candidates[6];
  int candCount = 0;
  if (s_geminiModel[0] != '\0') candidates[candCount++] = s_geminiModel;
  candidates[candCount++] = "gemini-3.5-flash-lite";
  candidates[candCount++] = "gemini-3.8-flash";
  candidates[candCount++] = "gemini-3.5-flash";
  candidates[candCount++] = "gemini-2.5-flash";

  bool success = false;
  String respBody;

  for (int i = 0; i < candCount; i++) {
    // Avoid re-trying identical model name twice
    if (i > 0 && strcmp(candidates[i], candidates[0]) == 0) continue;

    // Yield between candidate attempts to let mbedTLS buffers and socket close cleanly
    if (i > 0) {
      delay(300);
    }

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
        Serial.println("PetBrain: Quota exceeded (HTTP 429). Rate-limited for 60s, stopping candidate scan.");
        s_rateLimitedUntilMs = millis() + 60000UL;
        break; // Stop immediately, do not exhaust quota on subsequent candidate models!
      } else if (statusCode == 400 || statusCode == 403) {
        Serial.printf("PetBrain: Auth error (HTTP %d). Invalid API key or disabled API. Cooling down for 120s.\n", statusCode);
        s_rateLimitedUntilMs = millis() + 120000UL;
        break; // Invalid key or permission, retrying other models won't help!
      } else if (statusCode == 404) {
        Serial.printf("PetBrain: Model %s not found (HTTP 404), trying next candidate\n", candidates[i]);
      } else {
        Serial.printf("PetBrain: Model %s request failed (HTTP %d)\n", candidates[i], statusCode);
      }
    }
  }

  // If candidate models all failed, try discoverModel as a fallback
  if (!success && s_rateLimitedUntilMs <= millis()) {
    delay(300);
    if (discoverModel(apiKey)) {
      delay(200);
      String url = "https://generativelanguage.googleapis.com/v1beta/models/";
      url += s_geminiModel;
      url += ":generateContent?key=";
      url += apiKey;
      int statusCode = 0;
      if (Net_PostJson(url.c_str(), jsonBody.c_str(), respBody, "GEMINI", &statusCode)) {
        s_lastGeminiReqMs = millis();
        success = true;
      }
    }
  }

  if (!success) {
    Serial.println("PetBrain: All Gemini model requests failed, keeping current thought");
    s_isBusy = false;
    return false;
  }

  // Parse Gemini response
  JsonDocument respDoc(SpiRamAllocator::instance());
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
    Serial.printf("PetBrain: AI thought updated: \"%s\"\n", s_thought);
    s_isBusy = false;
    return true;
  }

  generateOfflineThought(s_thought, sizeof(s_thought));
  s_isBusy = false;
  return true;
}

