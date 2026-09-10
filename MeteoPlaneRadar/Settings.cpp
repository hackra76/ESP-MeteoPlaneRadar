// =============================================================================
//  MeteoPlaneRadar
//  Persisted settings - storage in NVS + JSON serialisation for the web UI.
//
// =============================================================================
#include "Settings.h"
#include "Lang.h"
#include <Preferences.h>
#include <string.h>
#include <ctype.h>

static Preferences prefs;
static const char* NS = "planeradar";

// --- WiFi ---
static char s_ssid[33] = "";
static char s_wpass[65] = "";
static WifiCredential s_wifiNets[MAX_WIFI_NETWORKS];
static int s_wifiNetCount = 0;
static char s_hostname[33] = WEB_HOSTNAME;

// --- Location ---
static double  s_lat = DEFAULT_LAT;
static double  s_lon = DEFAULT_LON;
static bool    s_hasLoc = false;

// --- Brightness / night mode ---
static uint8_t s_briDay   = 80;
static uint8_t s_briNight = 25;
static bool    s_nightAuto = true;
static int8_t  s_nightOff  = 0;
static bool    s_isNight   = false;

// --- Misc ---
static bool    s_metric = false;
static uint8_t s_lang   = LANG_EN;
static char    s_tz[64] = TZ_INFO;

// Bit per data screen (bit 0 = clock ... bit 6 = finance).
static uint8_t s_scrMask = (1 << SCREEN_CLOCK_I) | (1 << SCREEN_PLANES_I) |
                           (1 << SCREEN_METEO_I) | (1 << SCREEN_TACTICAL_I) |
                           (1 << SCREEN_FORECAST_I) | (1 << SCREEN_INFO_I) |
                           (1 << SCREEN_FINANCE_I);
static char    s_finTickers[128] = DEFAULT_FINANCE_TICKERS;
static uint16_t s_autoRot = 0;
static uint8_t s_radarSrc = RADAR_SRC_CHMU;
static bool    s_smoothRadar = true;

// --- Clock appearance ---
static uint8_t  s_secStyle   = SEC_STYLE_DOTS;
static uint8_t  s_clockStyle = CLOCK_STYLE_DIGITAL;
static uint16_t s_clockCol   = 0xFFFF;   // white
static uint16_t s_secCol     = 0x05FF;   // cyan
static bool     s_clkShowDate= true;
static bool     s_clkShowWx  = true;
static bool     s_clkShowWind= true;
static bool     s_clkShowMoon= true;
static bool     s_clkShowAstro= true;
static bool     s_nightClockOnly= false;
static bool     s_ultraNight    = false;
static bool     s_clkShowOverhead = true;
static float    s_overheadRadiusKm = 10.0f;

static bool     s_radShowTrails  = true;
static bool     s_radShowNearest = true;
static bool     s_radShowAirports= true;
static bool     s_radShowRings   = true;
static bool     s_radShowCompass = true;

// --- Aircraft filters ---
static uint16_t s_altMin = 0;
static uint16_t s_altMax = 60000;
static bool     s_onlyCs = false;
static bool     s_sqAlert = true;
static char     s_watch[10] = "";
static uint8_t  s_typeFilterMask = 0x3F;

// --- Buzzer / Audio alerts ---
static bool     s_bzOn     = true;
static bool     s_bzEm     = true;
static bool     s_bzWatch  = true;
static bool     s_bzOverhead = false;
static bool     s_bzPrecip = false;
static bool     s_bzTouch  = false;
static bool     s_bzHour   = false;
static bool     s_bzNMute  = true;
static bool     s_precipAlert = true;

// --- UI state ---
static uint8_t  s_rngP = 1;
static uint8_t  s_rngM = 1;
static uint8_t  s_rngT = 1;
static uint8_t  s_scr  = SCREEN_PLANES_I;
static uint16_t s_top  = 0;
static bool     s_showLegends = true;
static bool     s_autoRotateBearing = false;

// --- Admin password (see the note in Settings.h) ---
static char s_pw[33] = "";

static bool          s_uiDirty   = false;
static unsigned long s_uiDirtyAt = 0;
static void markDirty() { s_uiDirty = true; s_uiDirtyAt = millis(); }

// Immediate write for the settings that are changed rarely and deliberately
// (web UI, portal) rather than by dragging a finger.
static int s_batchDepth = 0;

void Settings_BatchBegin() {
  if (s_batchDepth == 0) {
    prefs.begin(NS, false);
  }
  s_batchDepth++;
}

void Settings_BatchEnd() {
  if (s_batchDepth > 0) {
    s_batchDepth--;
    if (s_batchDepth == 0) {
      prefs.end();
    }
  }
}

static void putU8(const char* k, uint8_t v) {
  if (s_batchDepth > 0) {
    prefs.putUChar(k, v);
  } else if (prefs.begin(NS, false)) {
    prefs.putUChar(k, v);
    prefs.end();
  }
}
static void putI8(const char* k, int8_t v) {
  if (s_batchDepth > 0) {
    prefs.putChar(k, v);
  } else if (prefs.begin(NS, false)) {
    prefs.putChar(k, v);
    prefs.end();
  }
}
static void putU16(const char* k, uint16_t v) {
  if (s_batchDepth > 0) {
    prefs.putUShort(k, v);
  } else if (prefs.begin(NS, false)) {
    prefs.putUShort(k, v);
    prefs.end();
  }
}
static void putBool(const char* k, bool v) {
  if (s_batchDepth > 0) {
    prefs.putBool(k, v);
  } else if (prefs.begin(NS, false)) {
    prefs.putBool(k, v);
    prefs.end();
  }
}
static void putStr(const char* k, const char* v) {
  if (s_batchDepth > 0) {
    prefs.putString(k, v);
  } else if (prefs.begin(NS, false)) {
    prefs.putString(k, v);
    prefs.end();
  }
}

void Settings_Begin() {
  bool migrateRotate = false;
  bool migrateInfoScr = false;
  bool migrateFinScr = false;
  if (prefs.begin(NS, true)) {
    s_lat    = prefs.getDouble("lat", DEFAULT_LAT);
    s_lon    = prefs.getDouble("lon", DEFAULT_LON);
    s_hasLoc = prefs.getBool("hasLoc", false);
    // "bl" was the single brightness value up to 0.5.5 - reuse it as the day
    // level so an updated device does not suddenly go dark.
    s_briDay   = prefs.getUChar("bl", 80);
    s_briNight = prefs.getUChar("blN", 25);
    s_nightAuto = prefs.getBool("nAuto", true);
    s_nightOff  = (int8_t)prefs.getChar("nOff", 0);
    s_metric = prefs.getBool("metric", false);
    s_lang   = prefs.getUChar("lang", LANG_EN);
    s_scrMask = prefs.getUChar("scrM", s_scrMask);
    if (!prefs.isKey("infoScrInit")) {
      s_scrMask |= (1 << SCREEN_INFO_I);
      migrateInfoScr = true;
    }
    if (!prefs.isKey("finScrInit")) {
      s_scrMask |= (1 << SCREEN_FINANCE_I);
      migrateFinScr = true;
    }
    if (prefs.isKey("finTk")) {
      prefs.getString("finTk", s_finTickers, sizeof(s_finTickers));
    }
    // Cycling interval moved from minutes to seconds - see Settings.h. The old
    // key is converted exactly once, so an updated device keeps its setting.
    if (prefs.isKey("autoRS")) {
      s_autoRot = prefs.getUShort("autoRS", 0);
    } else {
      s_autoRot = (uint16_t)prefs.getUChar("autoR", 0) * 60;
      migrateRotate = true;          // written below, the handle is read-only here
    }
    s_radarSrc = prefs.getUChar("radSrc", RADAR_SRC_CHMU);
    if (s_radarSrc != RADAR_SRC_RAINVIEWER && s_radarSrc != RADAR_SRC_SHMU) s_radarSrc = RADAR_SRC_CHMU;
    s_smoothRadar = prefs.getBool("smoothRad", true);
    s_secStyle = prefs.getUChar("secSt", SEC_STYLE_DOTS);
    s_clockStyle = prefs.getUChar("clkSt", CLOCK_STYLE_DIGITAL);
    s_clockCol = prefs.getUShort("clkC", 0xFFFF);
    s_secCol   = prefs.getUShort("secC", 0x05FF);
    s_clkShowDate = prefs.getBool("cDate", true);
    s_clkShowWx   = prefs.getBool("cWx", true);
    s_clkShowWind = prefs.getBool("cWnd", true);
    s_clkShowMoon = prefs.getBool("cMoon", true);
    s_clkShowAstro= prefs.getBool("cAstro", true);
    s_nightClockOnly = prefs.getBool("nClkOn", false);
    s_ultraNight     = prefs.getBool("uNight", false);
    s_clkShowOverhead = prefs.getBool("cOver", true);
    s_overheadRadiusKm = prefs.getFloat("ovRad", 10.0f);
    s_radShowTrails  = prefs.getBool("rTrl", true);
    s_radShowNearest = prefs.getBool("rNear", true);
    s_radShowAirports= prefs.getBool("rAirp", true);
    s_radShowRings   = prefs.getBool("rRng", true);
    s_radShowCompass = prefs.getBool("rCmp", true);
    s_altMin = prefs.getUShort("altLo", 0);
    s_altMax = prefs.getUShort("altHi", 60000);
    s_onlyCs = prefs.getBool("onlyCs", false);
    s_sqAlert = prefs.getBool("sqAl", true);
    s_typeFilterMask = prefs.getUChar("acMask", 0x3F);
    s_bzOn    = prefs.getBool("bzOn", true);
    s_bzEm    = prefs.getBool("bzEm", true);
    s_bzWatch = prefs.getBool("bzWatch", true);
    s_bzOverhead = prefs.getBool("bzOver", false);
    s_bzPrecip = prefs.getBool("bzPrecip", false);
    s_bzTouch = prefs.getBool("bzTouch", false);
    s_bzHour  = prefs.getBool("bzHour", false);
    s_bzNMute = prefs.getBool("bzNMute", true);
    s_precipAlert = prefs.getBool("cPrecip", true);
    if (prefs.isKey("watch")) prefs.getString("watch", s_watch, sizeof(s_watch));
    s_rngP   = prefs.getUChar("rngP", 1);
    s_rngM   = prefs.getUChar("rngM", 1);
    s_rngT   = prefs.getUChar("rngT", 1);
    s_scr    = prefs.getUChar("scr", SCREEN_PLANES_I);
    s_top    = prefs.getUShort("topb", 0);
    s_showLegends = prefs.getBool("sLeg", true);
    s_autoRotateBearing = prefs.getBool("autoRot", false);
    if (prefs.isKey("pw"))    prefs.getString("pw", s_pw, sizeof(s_pw));
    s_wifiNetCount = 0;
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
      char ks[8], kp[8];
      snprintf(ks, sizeof(ks), "w_s%d", i);
      snprintf(kp, sizeof(kp), "w_p%d", i);
      if (prefs.isKey(ks)) {
        prefs.getString(ks, s_wifiNets[s_wifiNetCount].ssid, sizeof(s_wifiNets[s_wifiNetCount].ssid));
        prefs.getString(kp, s_wifiNets[s_wifiNetCount].pass, sizeof(s_wifiNets[s_wifiNetCount].pass));
        if (s_wifiNets[s_wifiNetCount].ssid[0] != '\0') {
          s_wifiNetCount++;
        }
      }
    }
    // Migration from legacy single-profile "ssid" and "wpass"
    if (s_wifiNetCount == 0 && prefs.isKey("ssid")) {
      char legSsid[33] = "";
      char legPass[65] = "";
      prefs.getString("ssid", legSsid, sizeof(legSsid));
      prefs.getString("wpass", legPass, sizeof(legPass));
      if (legSsid[0] != '\0') {
        strncpy(s_wifiNets[0].ssid, legSsid, sizeof(s_wifiNets[0].ssid) - 1);
        strncpy(s_wifiNets[0].pass, legPass, sizeof(s_wifiNets[0].pass) - 1);
        s_wifiNetCount = 1;
      }
    }
    if (s_wifiNetCount > 0) {
      strncpy(s_ssid, s_wifiNets[0].ssid, sizeof(s_ssid) - 1);
      s_ssid[sizeof(s_ssid) - 1] = '\0';
      strncpy(s_wpass, s_wifiNets[0].pass, sizeof(s_wpass) - 1);
      s_wpass[sizeof(s_wpass) - 1] = '\0';
    } else {
      s_ssid[0] = '\0';
      s_wpass[0] = '\0';
    }
    if (prefs.isKey("tz"))    prefs.getString("tz", s_tz, sizeof(s_tz));
    if (prefs.isKey("host"))  prefs.getString("host", s_hostname, sizeof(s_hostname));
    prefs.end();
  }
  if (s_altMax == 0) s_altMax = 60000;
  if (s_autoRot > 3600) s_autoRot = 3600;
  if (migrateRotate && prefs.begin(NS, false)) {
    prefs.putUShort("autoRS", s_autoRot);
    prefs.remove("autoR");           // the old key would only confuse later
    prefs.end();
    if (s_autoRot) Serial.printf("Settings: auto-rotation migrated to %u s\n", s_autoRot);
  }
  if (migrateInfoScr && prefs.begin(NS, false)) {
    prefs.putBool("infoScrInit", true);
    prefs.putUChar("scrM", s_scrMask);
    prefs.end();
  }
  if (migrateFinScr && prefs.begin(NS, false)) {
    prefs.putBool("finScrInit", true);
    prefs.putUChar("scrM", s_scrMask);
    prefs.end();
  }
  Lang_Set(s_lang);
  Settings_ApplyTimezone();
}

// --- WiFi -------------------------------------------------------------------
static void saveWifiNets() {
  if (prefs.begin(NS, false)) {
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
      char ks[8], kp[8];
      snprintf(ks, sizeof(ks), "w_s%d", i);
      snprintf(kp, sizeof(kp), "w_p%d", i);
      if (i < s_wifiNetCount) {
        prefs.putString(ks, s_wifiNets[i].ssid);
        prefs.putString(kp, s_wifiNets[i].pass);
      } else {
        if (prefs.isKey(ks)) prefs.remove(ks);
        if (prefs.isKey(kp)) prefs.remove(kp);
      }
    }
    if (s_wifiNetCount > 0) {
      strncpy(s_ssid, s_wifiNets[0].ssid, sizeof(s_ssid) - 1);
      s_ssid[sizeof(s_ssid) - 1] = '\0';
      strncpy(s_wpass, s_wifiNets[0].pass, sizeof(s_wpass) - 1);
      s_wpass[sizeof(s_wpass) - 1] = '\0';
      prefs.putString("ssid", s_ssid);
      prefs.putString("wpass", s_wpass);
    } else {
      s_ssid[0] = '\0';
      s_wpass[0] = '\0';
      if (prefs.isKey("ssid")) prefs.remove("ssid");
      if (prefs.isKey("wpass")) prefs.remove("wpass");
    }
    prefs.end();
  }
}

const char* Settings_WifiSsid() {
  return (s_wifiNetCount > 0) ? s_wifiNets[0].ssid : "";
}

const char* Settings_WifiPass() {
  return (s_wifiNetCount > 0) ? s_wifiNets[0].pass : "";
}

bool Settings_HasWifi() {
  return s_wifiNetCount > 0 && s_wifiNets[0].ssid[0] != '\0';
}

void Settings_SetWifi(const char* ssid, const char* pass) {
  Settings_AddOrUpdateWifi(ssid, pass);
}

void Settings_ClearWifi() {
  s_wifiNetCount = 0;
  saveWifiNets();
}

int Settings_WifiNetworkCount() {
  return s_wifiNetCount;
}

bool Settings_GetWifiNetwork(int idx, WifiCredential* out) {
  if (idx < 0 || idx >= s_wifiNetCount || !out) return false;
  *out = s_wifiNets[idx];
  return true;
}

void Settings_AddOrUpdateWifi(const char* ssid, const char* pass) {
  if (!ssid || !*ssid) return;

  int foundIdx = -1;
  for (int i = 0; i < s_wifiNetCount; i++) {
    if (strcmp(s_wifiNets[i].ssid, ssid) == 0) {
      foundIdx = i;
      break;
    }
  }

  WifiCredential cred;
  memset(&cred, 0, sizeof(cred));
  strncpy(cred.ssid, ssid, sizeof(cred.ssid) - 1);
  if (pass && *pass) {
    strncpy(cred.pass, pass, sizeof(cred.pass) - 1);
  } else if (foundIdx >= 0) {
    strncpy(cred.pass, s_wifiNets[foundIdx].pass, sizeof(cred.pass) - 1);
  }

  if (foundIdx >= 0) {
    for (int i = foundIdx; i > 0; i--) {
      s_wifiNets[i] = s_wifiNets[i - 1];
    }
    s_wifiNets[0] = cred;
  } else {
    int limit = (s_wifiNetCount < MAX_WIFI_NETWORKS) ? s_wifiNetCount : (MAX_WIFI_NETWORKS - 1);
    for (int i = limit; i > 0; i--) {
      s_wifiNets[i] = s_wifiNets[i - 1];
    }
    s_wifiNets[0] = cred;
    if (s_wifiNetCount < MAX_WIFI_NETWORKS) s_wifiNetCount++;
  }
  saveWifiNets();
}

bool Settings_DeleteWifiNetwork(int idx) {
  if (idx < 0 || idx >= s_wifiNetCount) return false;
  for (int i = idx; i < s_wifiNetCount - 1; i++) {
    s_wifiNets[i] = s_wifiNets[i + 1];
  }
  s_wifiNetCount--;
  saveWifiNets();
  return true;
}

bool Settings_DeleteWifiNetworkBySsid(const char* ssid) {
  if (!ssid || !*ssid) return false;
  for (int i = 0; i < s_wifiNetCount; i++) {
    if (strcmp(s_wifiNets[i].ssid, ssid) == 0) {
      return Settings_DeleteWifiNetwork(i);
    }
  }
  return false;
}

void Settings_SetActiveWifi(int idx) {
  if (idx <= 0 || idx >= s_wifiNetCount) return;
  WifiCredential temp = s_wifiNets[idx];
  for (int i = idx; i > 0; i--) {
    s_wifiNets[i] = s_wifiNets[i - 1];
  }
  s_wifiNets[0] = temp;
  saveWifiNets();
}

const char* Settings_Hostname() {
  if (s_hostname[0] == '\0') return WEB_HOSTNAME;
  return s_hostname;
}

void Settings_SetHostname(const char* name) {
  if (!name || !*name) {
    strncpy(s_hostname, WEB_HOSTNAME, sizeof(s_hostname) - 1);
  } else {
    int j = 0;
    for (int i = 0; name[i] && j < (int)sizeof(s_hostname) - 1; i++) {
      char c = name[i];
      if (isalnum((unsigned char)c) || c == '-' || c == '_') {
        s_hostname[j++] = c;
      }
    }
    s_hostname[j] = '\0';
    if (j == 0) strncpy(s_hostname, WEB_HOSTNAME, sizeof(s_hostname) - 1);
  }
  s_hostname[sizeof(s_hostname) - 1] = '\0';
  putStr("host", s_hostname);
}

// --- Location ---------------------------------------------------------------
double Settings_Lat() { return s_lat; }
double Settings_Lon() { return s_lon; }
bool   Settings_HasLocation() { return s_hasLoc; }

void Settings_SetLocation(double lat, double lon) {
  s_lat = lat; s_lon = lon; s_hasLoc = true;
  if (s_batchDepth > 0) {
    prefs.putDouble("lat", lat);
    prefs.putDouble("lon", lon);
    prefs.putBool("hasLoc", true);
  } else if (prefs.begin(NS, false)) {
    prefs.putDouble("lat", lat);
    prefs.putDouble("lon", lon);
    prefs.putBool("hasLoc", true);
    prefs.end();
  }
}

// --- Brightness -------------------------------------------------------------
uint8_t Settings_Backlight()   { return s_isNight ? s_briNight : s_briDay; }
uint8_t Settings_BrightDay()   { return s_briDay; }
uint8_t Settings_BrightNight() { return s_briNight; }

// Dragging the slider used to write flash on every touch sample - dozens of
// erase/write cycles for one adjustment. It goes through the same debounce as
// the rest of the UI state.
void Settings_SetBacklight(uint8_t pct) {
  if (s_isNight) { if (pct == s_briNight) return; s_briNight = pct; }
  else           { if (pct == s_briDay)   return; s_briDay   = pct; }
  markDirty();
}
void Settings_SetBrightDay(uint8_t pct)   { if (pct != s_briDay)   { s_briDay = pct;   markDirty(); } }
void Settings_SetBrightNight(uint8_t pct) { if (pct != s_briNight) { s_briNight = pct; markDirty(); } }
bool Settings_NightAuto() { return s_nightAuto; }
void Settings_SetNightAuto(bool on) { s_nightAuto = on; putBool("nAuto", on); }
int8_t Settings_NightOffsetMin() { return s_nightOff; }
void   Settings_SetNightOffsetMin(int8_t m) {
  if (m >  NIGHT_OFFSET_MIN_LIMIT) m =  NIGHT_OFFSET_MIN_LIMIT;
  if (m < -NIGHT_OFFSET_MIN_LIMIT) m = -NIGHT_OFFSET_MIN_LIMIT;
  s_nightOff = m;
  putI8("nOff", m);
}
bool Settings_IsNight() { return s_isNight; }
void Settings_SetNight(bool night) { s_isNight = night; }

// --- Units, language --------------------------------------------------------
bool Settings_MetricUnits() { return s_metric; }
void Settings_SetMetricUnits(bool metric) { s_metric = metric; putBool("metric", metric); }
uint8_t Settings_Language() { return s_lang; }
void    Settings_SetLanguage(uint8_t l) {
  s_lang = (l == LANG_CZ || l == LANG_SK) ? l : LANG_EN;
  Lang_Set(s_lang);
  putU8("lang", s_lang);
}

const char* Settings_Timezone() { return s_tz; }
void Settings_SetTimezone(const char* tz) {
  if (!tz || !*tz) tz = TZ_INFO;
  strncpy(s_tz, tz, sizeof(s_tz) - 1);
  s_tz[sizeof(s_tz) - 1] = '\0';
  if (s_batchDepth > 0) {
    prefs.putString("tz", s_tz);
  } else if (prefs.begin(NS, false)) {
    prefs.putString("tz", s_tz);
    prefs.end();
  }
  Settings_ApplyTimezone();
}

void Settings_ApplyTimezone() {
  const char* tz = s_tz;
  if (!tz || !*tz) tz = TZ_INFO;
  setenv("TZ", tz, 1);
  tzset();
}

// --- Screens ----------------------------------------------------------------
bool Settings_ScreenEnabled(uint8_t idx) {
  if (idx == SCREEN_SETTINGS_I) return true;      // always reachable
  if (idx >= SCREEN_SETTINGS_I) return false;
  return (s_scrMask >> idx) & 1;
}

void Settings_SetScreenEnabled(uint8_t idx, bool on) {
  if (idx >= SCREEN_SETTINGS_I) return;
  uint8_t next = on ? (s_scrMask | (1 << idx)) : (s_scrMask & ~(1 << idx));
  // Never allow the last data screen to be turned off. With all of them gone
  // the device would boot into Settings and show nothing else - technically
  // recoverable, but it looks broken.
  if (next == 0) return;
  s_scrMask = next;
  putU8("scrM", s_scrMask);
  putBool("infoScrInit", true);
  putBool("finScrInit", true);
}

uint8_t Settings_EnabledCount() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < SCREEN_SETTINGS_I; i++) if (Settings_ScreenEnabled(i)) n++;
  return n;
}

uint16_t Settings_AutoRotateSec() { return s_autoRot; }
void     Settings_SetAutoRotateSec(uint16_t s) {
  if (s > 3600) s = 3600;
  s_autoRot = s;
  putU16("autoRS", s);
}

const char* Settings_FinanceTickers() { return s_finTickers; }
void Settings_SetFinanceTickers(const char* tickers) {
  if (!tickers || !*tickers) tickers = DEFAULT_FINANCE_TICKERS;
  strncpy(s_finTickers, tickers, sizeof(s_finTickers) - 1);
  s_finTickers[sizeof(s_finTickers) - 1] = '\0';
  putStr("finTk", s_finTickers);
}

// --- Weather radar ----------------------------------------------------------
uint8_t Settings_RadarSource() { return s_radarSrc; }
void    Settings_SetRadarSource(uint8_t s) {
  if (s != RADAR_SRC_RAINVIEWER && s != RADAR_SRC_SHMU) s = RADAR_SRC_CHMU;
  s_radarSrc = s;
  putU8("radSrc", s_radarSrc);
}
bool    Settings_SmoothRadar() { return s_smoothRadar; }
void    Settings_SetSmoothRadar(bool en) { s_smoothRadar = en; putBool("smoothRad", en); }

// --- Clock appearance -------------------------------------------------------
uint8_t  Settings_SecondsStyle() { return s_secStyle; }
void     Settings_SetSecondsStyle(uint8_t s) { if (s > SEC_STYLE_MAX) s = 0; s_secStyle = s; putU8("secSt", s); }
uint8_t  Settings_ClockStyle() { return s_clockStyle; }
void     Settings_SetClockStyle(uint8_t s) { if (s > CLOCK_STYLE_MAX) s = 0; s_clockStyle = s; putU8("clkSt", s); }
uint16_t Settings_ClockColor() { return s_clockCol; }
void     Settings_SetClockColor(uint16_t c) { s_clockCol = c; putU16("clkC", c); }
uint16_t Settings_SecondsColor() { return s_secCol; }
void     Settings_SetSecondsColor(uint16_t c) { s_secCol = c; putU16("secC", c); }

// --- Clock widget toggles ---
bool     Settings_ClockShowDate() { return s_clkShowDate; }
void     Settings_SetClockShowDate(bool on) { s_clkShowDate = on; putBool("cDate", on); }
bool     Settings_ClockShowWeather() { return s_clkShowWx; }
void     Settings_SetClockShowWeather(bool on) { s_clkShowWx = on; putBool("cWx", on); }
bool     Settings_ClockShowWind() { return s_clkShowWind; }
void     Settings_SetClockShowWind(bool on) { s_clkShowWind = on; putBool("cWnd", on); }
bool     Settings_ClockShowMoon() { return s_clkShowMoon; }
void     Settings_SetClockShowMoon(bool on) { s_clkShowMoon = on; putBool("cMoon", on); }
bool     Settings_ClockShowAstro() { return s_clkShowAstro; }
void     Settings_SetClockShowAstro(bool on) { s_clkShowAstro = on; putBool("cAstro", on); }
bool     Settings_NightClockOnly() { return s_nightClockOnly; }
void     Settings_SetNightClockOnly(bool on) { s_nightClockOnly = on; putBool("nClkOn", on); }
bool     Settings_UltraNight() { return s_ultraNight; }
void     Settings_SetUltraNight(bool on) { s_ultraNight = on; putBool("uNight", on); }
bool     Settings_ClockShowOverhead() { return s_clkShowOverhead; }
void     Settings_SetClockShowOverhead(bool on) { s_clkShowOverhead = on; putBool("cOver", on); }
float    Settings_OverheadRadiusKm() { return s_overheadRadiusKm; }
void     Settings_SetOverheadRadiusKm(float r) {
  if (r < 1.0f) r = 1.0f;
  if (r > 50.0f) r = 50.0f;
  s_overheadRadiusKm = r;
  if (s_batchDepth > 0) {
    prefs.putFloat("ovRad", r);
  } else if (prefs.begin(NS, false)) {
    prefs.putFloat("ovRad", r);
    prefs.end();
  }
}

// --- Radar widget toggles ---
bool     Settings_RadarShowTrails() { return s_radShowTrails; }
void     Settings_SetRadarShowTrails(bool on) { s_radShowTrails = on; putBool("rTrl", on); }
bool     Settings_RadarShowNearest() { return s_radShowNearest; }
void     Settings_SetRadarShowNearest(bool on) { s_radShowNearest = on; putBool("rNear", on); }
bool     Settings_RadarShowAirports() { return s_radShowAirports; }
void     Settings_SetRadarShowAirports(bool on) { s_radShowAirports = on; putBool("rAirp", on); }
bool     Settings_RadarShowRings() { return s_radShowRings; }
void     Settings_SetRadarShowRings(bool on) { s_radShowRings = on; putBool("rRng", on); }
bool     Settings_RadarShowCompass() { return s_radShowCompass; }
void     Settings_SetRadarShowCompass(bool on) { s_radShowCompass = on; putBool("rCmp", on); }

// --- Aircraft filters -------------------------------------------------------
uint16_t Settings_AltMinFt() { return s_altMin; }
uint16_t Settings_AltMaxFt() { return s_altMax; }
void     Settings_SetAltRangeFt(uint16_t lo, uint16_t hi) {
  if (hi <= lo) { lo = 0; hi = 60000; }           // nonsense range = no filter
  s_altMin = lo; s_altMax = hi;
  putU16("altLo", lo);
  putU16("altHi", hi);
}
bool Settings_OnlyWithCallsign() { return s_onlyCs; }
void Settings_SetOnlyWithCallsign(bool on) { s_onlyCs = on; putBool("onlyCs", on); }
bool Settings_SquawkAlert() { return s_sqAlert; }
void Settings_SetSquawkAlert(bool on) { s_sqAlert = on; putBool("sqAl", on); }
const char* Settings_WatchCallsign() { return s_watch; }
void Settings_SetWatchCallsign(const char* s) {
  if (!s) s = "";
  strncpy(s_watch, s, sizeof(s_watch) - 1);
  s_watch[sizeof(s_watch) - 1] = '\0';
  // Compared against callsigns and hex addresses, both of which arrive upper
  // case from adsb.fi - normalise here so the user does not have to.
  for (char* p = s_watch; *p; p++) *p = toupper((unsigned char)*p);
  putStr("watch", s_watch);
}
uint8_t Settings_PlaneTypeMask() { return s_typeFilterMask; }
void Settings_SetPlaneTypeMask(uint8_t mask) {
  s_typeFilterMask = mask;
  putU8("acMask", mask);
}
bool Settings_PlaneTypeEnabled(AircraftIconType type) {
  return (s_typeFilterMask & (1 << (uint8_t)type)) != 0;
}
void Settings_SetPlaneTypeEnabled(AircraftIconType type, bool on) {
  uint8_t bit = (1 << (uint8_t)type);
  if (on) s_typeFilterMask |= bit;
  else    s_typeFilterMask &= ~bit;
  putU8("acMask", s_typeFilterMask);
}

// --- Buzzer / Audio alerts --------------------------------------------------
bool Settings_BuzzerEnabled() { return s_bzOn; }
void Settings_SetBuzzerEnabled(bool on) { s_bzOn = on; putBool("bzOn", on); }
bool Settings_BuzzerEmergency() { return s_bzEm; }
void Settings_SetBuzzerEmergency(bool on) { s_bzEm = on; putBool("bzEm", on); }
bool Settings_BuzzerWatch() { return s_bzWatch; }
void Settings_SetBuzzerWatch(bool on) { s_bzWatch = on; putBool("bzWatch", on); }
bool Settings_BuzzerOverhead() { return s_bzOverhead; }
void Settings_SetBuzzerOverhead(bool on) { s_bzOverhead = on; putBool("bzOver", on); }
bool Settings_BuzzerPrecip() { return s_bzPrecip; }
void Settings_SetBuzzerPrecip(bool on) { s_bzPrecip = on; putBool("bzPrecip", on); }
bool Settings_BuzzerTouch() { return s_bzTouch; }
void Settings_SetBuzzerTouch(bool on) { s_bzTouch = on; putBool("bzTouch", on); }
bool Settings_BuzzerHourly() { return s_bzHour; }
void Settings_SetBuzzerHourly(bool on) { s_bzHour = on; putBool("bzHour", on); }
bool Settings_BuzzerNightMute() { return s_bzNMute; }
void Settings_SetBuzzerNightMute(bool on) { s_bzNMute = on; putBool("bzNMute", on); }

bool Settings_PrecipAlert() { return s_precipAlert; }
void Settings_SetPrecipAlert(bool on) { s_precipAlert = on; putBool("cPrecip", on); }

// --- UI state ---------------------------------------------------------------
uint8_t Settings_PlaneRange() { return s_rngP; }
void    Settings_SetPlaneRange(uint8_t idx) { if (idx != s_rngP) { s_rngP = idx; markDirty(); } }
uint8_t Settings_MeteoRange() { return s_rngM; }
void    Settings_SetMeteoRange(uint8_t idx) { if (idx != s_rngM) { s_rngM = idx; markDirty(); } }
uint8_t Settings_TacticalRange() { return s_rngT; }
void    Settings_SetTacticalRange(uint8_t idx) { if (idx != s_rngT) { s_rngT = idx; markDirty(); } }
uint16_t Settings_TopBearing() { return s_top; }
void     Settings_SetTopBearing(uint16_t deg) {
  deg %= 360;
  if (deg != s_top) { s_top = deg; markDirty(); }
}
uint8_t Settings_Screen() { return s_scr; }
void    Settings_SetScreen(uint8_t idx) { if (idx != s_scr) { s_scr = idx; markDirty(); } }
bool    Settings_ShowLegends() { return s_showLegends; }
void    Settings_SetShowLegends(bool show) {
  if (show != s_showLegends) {
    s_showLegends = show;
    putBool("sLeg", show);
  }
}
void    Settings_ToggleLegends() {
  Settings_SetShowLegends(!s_showLegends);
}
bool    Settings_AutoRotateBearing() { return s_autoRotateBearing; }
void    Settings_SetAutoRotateBearing(bool on) {
  if (on != s_autoRotateBearing) {
    s_autoRotateBearing = on;
    putBool("autoRot", on);
  }
}

// --- Admin password ---------------------------------------------------------
bool Settings_HasAdminPassword() { return s_pw[0] != '\0'; }
const char* Settings_AdminPassword() { return s_pw; }

void Settings_SetAdminPassword(const char* plain) {
  if (!plain) plain = "";
  // A single space is the agreed way to say "remove the password" from the web
  // form, where an empty field has to mean "leave it alone" - otherwise every
  // save without retyping it would silently switch the protection off.
  if (strcmp(plain, " ") == 0) plain = "";
  strncpy(s_pw, plain, sizeof(s_pw) - 1);
  s_pw[sizeof(s_pw) - 1] = '\0';
  putStr("pw", s_pw);
}

bool Settings_CheckAdminPassword(const char* plain) {
  if (!Settings_HasAdminPassword()) return true;    // protection disabled
  if (!plain) return false;
  // Constant time over the full buffer: comparing only up to the first
  // difference leaks how much of a guess was right.
  uint8_t diff = 0;
  size_t n = sizeof(s_pw);
  for (size_t i = 0; i < n; i++) {
    char a = (i < strlen(plain)) ? plain[i] : '\0';
    diff |= (uint8_t)(a ^ s_pw[i]);
  }
  return diff == 0;
}

// --- Serialisation ----------------------------------------------------------
void Settings_ToJson(JsonObject o) {
  o["lat"] = s_lat;
  o["lon"] = s_lon;
  o["hasLoc"] = s_hasLoc;
  o["lang"] = s_lang;
  o["metric"] = s_metric;
  o["briDay"] = s_briDay;
  o["briNight"] = s_briNight;
  o["nightAuto"] = s_nightAuto;
  o["nightOffset"] = s_nightOff;
  o["radarSrc"] = s_radarSrc;
  o["smoothRadar"] = s_smoothRadar;
  o["autoRotate"] = s_autoRot;   // seconds
  o["topBearing"] = s_top;
  o["secStyle"] = s_secStyle;
  o["clockStyle"] = s_clockStyle;
  o["clockColor"] = s_clockCol;
  o["secColor"] = s_secCol;
  o["cDate"] = s_clkShowDate;
  o["cWx"] = s_clkShowWx;
  o["cWind"] = s_clkShowWind;
  o["cMoon"] = s_clkShowMoon;
  o["cAstro"] = s_clkShowAstro;
  o["nightClockOnly"] = s_nightClockOnly;
  o["ultraNight"] = s_ultraNight;
  o["cOver"] = s_clkShowOverhead;
  o["ovRad"] = s_overheadRadiusKm;
  o["rTrails"] = s_radShowTrails;
  o["rNearest"] = s_radShowNearest;
  o["rAirports"] = s_radShowAirports;
  o["rRings"] = s_radShowRings;
  o["rCompass"] = s_radShowCompass;
  o["autoRotateBearing"] = s_autoRotateBearing;
  o["altMin"] = s_altMin;
  o["altMax"] = s_altMax;
  o["onlyCallsign"] = s_onlyCs;
  o["squawkAlert"] = s_sqAlert;
  o["watch"] = s_watch;
  o["typeAirliner"] = Settings_PlaneTypeEnabled(ICON_AIRLINER);
  o["typeLight"]    = Settings_PlaneTypeEnabled(ICON_LIGHT);
  o["typeHeli"]     = Settings_PlaneTypeEnabled(ICON_HELICOPTER);
  o["typeMilitary"] = Settings_PlaneTypeEnabled(ICON_MILITARY_JET);
  o["typeHeavy"]    = Settings_PlaneTypeEnabled(ICON_HEAVY);
  o["typeGlider"]   = Settings_PlaneTypeEnabled(ICON_GLIDER);
  o["buzzerOn"]        = s_bzOn;
  o["buzzerEmergency"] = s_bzEm;
  o["buzzerWatch"]     = s_bzWatch;
  o["buzzerOverhead"]  = s_bzOverhead;
  o["buzzerPrecip"]    = s_bzPrecip;
  o["buzzerTouch"]     = s_bzTouch;
  o["buzzerHourly"]    = s_bzHour;
  o["buzzerNightMute"] = s_bzNMute;
  o["cPrecip"]         = s_precipAlert;
  o["showLegends"] = s_showLegends;
  o["hasPassword"] = Settings_HasAdminPassword();
  o["timezone"] = s_tz;
  o["hostname"] = Settings_Hostname();

  JsonArray wArr = o["wifiNetworks"].to<JsonArray>();
  for (int i = 0; i < s_wifiNetCount; i++) {
    JsonObject w = wArr.add<JsonObject>();
    w["ssid"] = s_wifiNets[i].ssid;
    w["active"] = (i == 0);
  }

  JsonObject scr = o["screens"].to<JsonObject>();
  scr["clock"]    = Settings_ScreenEnabled(SCREEN_CLOCK_I);
  scr["planes"]   = Settings_ScreenEnabled(SCREEN_PLANES_I);
  scr["meteo"]    = Settings_ScreenEnabled(SCREEN_METEO_I);
  scr["tactical"] = Settings_ScreenEnabled(SCREEN_TACTICAL_I);
  scr["forecast"] = Settings_ScreenEnabled(SCREEN_FORECAST_I);
  scr["info"]     = Settings_ScreenEnabled(SCREEN_INFO_I);
  scr["finance"]  = Settings_ScreenEnabled(SCREEN_FINANCE_I);
  o["financeTickers"] = s_finTickers;
}

bool Settings_FromJson(JsonObjectConst in) {
  Settings_BatchBegin();
  bool changed = false;
  auto setIf = [&](const char* key, auto fn) {
    JsonVariantConst v = in[key];
    if (!v.isNull()) { fn(v); changed = true; }
  };

  // Location only counts when both halves are there and sane - a half-applied
  // position would put the radar in the Gulf of Guinea.
  if (!in["lat"].isNull() && !in["lon"].isNull()) {
    double la = in["lat"].as<double>(), lo = in["lon"].as<double>();
    if (la >= -90 && la <= 90 && lo >= -180 && lo <= 180 && (la != 0 || lo != 0)) {
      Settings_SetLocation(la, lo);
      changed = true;
    }
  }
  setIf("lang",         [](JsonVariantConst v){ Settings_SetLanguage(v.as<uint8_t>()); });
  setIf("metric",       [](JsonVariantConst v){ Settings_SetMetricUnits(v.as<bool>()); });
  setIf("briDay",       [](JsonVariantConst v){ Settings_SetBrightDay(v.as<uint8_t>()); });
  setIf("briNight",     [](JsonVariantConst v){ Settings_SetBrightNight(v.as<uint8_t>()); });
  setIf("nightAuto",    [](JsonVariantConst v){ Settings_SetNightAuto(v.as<bool>()); });
  setIf("nightOffset",  [](JsonVariantConst v){ Settings_SetNightOffsetMin(v.as<int8_t>()); });
  setIf("radarSrc",     [](JsonVariantConst v){ Settings_SetRadarSource(v.as<uint8_t>()); });
  setIf("smoothRadar",  [](JsonVariantConst v){ Settings_SetSmoothRadar(v.as<bool>()); });
  setIf("autoRotate",   [](JsonVariantConst v){ Settings_SetAutoRotateSec(v.as<uint16_t>()); });
  setIf("topBearing",   [](JsonVariantConst v){ Settings_SetTopBearing(v.as<uint16_t>()); });
  setIf("secStyle",     [](JsonVariantConst v){ Settings_SetSecondsStyle(v.as<uint8_t>()); });
  setIf("clockStyle",   [](JsonVariantConst v){ Settings_SetClockStyle(v.as<uint8_t>()); });
  setIf("clockColor",   [](JsonVariantConst v){ Settings_SetClockColor(v.as<uint16_t>()); });
  setIf("secColor",     [](JsonVariantConst v){ Settings_SetSecondsColor(v.as<uint16_t>()); });
  setIf("cDate",        [](JsonVariantConst v){ Settings_SetClockShowDate(v.as<bool>()); });
  setIf("cWx",          [](JsonVariantConst v){ Settings_SetClockShowWeather(v.as<bool>()); });
  setIf("cWind",        [](JsonVariantConst v){ Settings_SetClockShowWind(v.as<bool>()); });
  setIf("cMoon",        [](JsonVariantConst v){ Settings_SetClockShowMoon(v.as<bool>()); });
  setIf("cAstro",       [](JsonVariantConst v){ Settings_SetClockShowAstro(v.as<bool>()); });
  setIf("nightClockOnly",[](JsonVariantConst v){ Settings_SetNightClockOnly(v.as<bool>()); });
  setIf("ultraNight",    [](JsonVariantConst v){ Settings_SetUltraNight(v.as<bool>()); });
  setIf("cOver",         [](JsonVariantConst v){ Settings_SetClockShowOverhead(v.as<bool>()); });
  setIf("ovRad",         [](JsonVariantConst v){ Settings_SetOverheadRadiusKm(v.as<float>()); });
  setIf("rTrails",      [](JsonVariantConst v){ Settings_SetRadarShowTrails(v.as<bool>()); });
  setIf("rNearest",     [](JsonVariantConst v){ Settings_SetRadarShowNearest(v.as<bool>()); });
  setIf("rAirports",    [](JsonVariantConst v){ Settings_SetRadarShowAirports(v.as<bool>()); });
  setIf("hostname",     [](JsonVariantConst v){ Settings_SetHostname(v.as<const char*>()); });
  setIf("rRings",       [](JsonVariantConst v){ Settings_SetRadarShowRings(v.as<bool>()); });
  setIf("rCompass",     [](JsonVariantConst v){ Settings_SetRadarShowCompass(v.as<bool>()); });
  setIf("autoRotateBearing", [](JsonVariantConst v){ Settings_SetAutoRotateBearing(v.as<bool>()); });
  setIf("showLegends",  [](JsonVariantConst v){ Settings_SetShowLegends(v.as<bool>()); });
  setIf("onlyCallsign", [](JsonVariantConst v){ Settings_SetOnlyWithCallsign(v.as<bool>()); });
  setIf("squawkAlert",  [](JsonVariantConst v){ Settings_SetSquawkAlert(v.as<bool>()); });
  setIf("watch",        [](JsonVariantConst v){ Settings_SetWatchCallsign(v.as<const char*>()); });
  setIf("timezone",     [](JsonVariantConst v){ Settings_SetTimezone(v.as<const char*>()); });
  setIf("typeAirliner", [](JsonVariantConst v){ Settings_SetPlaneTypeEnabled(ICON_AIRLINER, v.as<bool>()); });
  setIf("typeLight",    [](JsonVariantConst v){ Settings_SetPlaneTypeEnabled(ICON_LIGHT, v.as<bool>()); });
  setIf("typeHeli",     [](JsonVariantConst v){ Settings_SetPlaneTypeEnabled(ICON_HELICOPTER, v.as<bool>()); });
  setIf("typeMilitary", [](JsonVariantConst v){ Settings_SetPlaneTypeEnabled(ICON_MILITARY_JET, v.as<bool>()); });
  setIf("typeHeavy",    [](JsonVariantConst v){ Settings_SetPlaneTypeEnabled(ICON_HEAVY, v.as<bool>()); });
  setIf("typeGlider",   [](JsonVariantConst v){ Settings_SetPlaneTypeEnabled(ICON_GLIDER, v.as<bool>()); });
  setIf("buzzerOn",        [](JsonVariantConst v){ Settings_SetBuzzerEnabled(v.as<bool>()); });
  setIf("buzzerEmergency", [](JsonVariantConst v){ Settings_SetBuzzerEmergency(v.as<bool>()); });
  setIf("buzzerWatch",     [](JsonVariantConst v){ Settings_SetBuzzerWatch(v.as<bool>()); });
  setIf("buzzerOverhead",  [](JsonVariantConst v){ Settings_SetBuzzerOverhead(v.as<bool>()); });
  setIf("buzzerPrecip",    [](JsonVariantConst v){ Settings_SetBuzzerPrecip(v.as<bool>()); });
  setIf("buzzerTouch",     [](JsonVariantConst v){ Settings_SetBuzzerTouch(v.as<bool>()); });
  setIf("buzzerHourly",    [](JsonVariantConst v){ Settings_SetBuzzerHourly(v.as<bool>()); });
  setIf("buzzerNightMute", [](JsonVariantConst v){ Settings_SetBuzzerNightMute(v.as<bool>()); });
  setIf("cPrecip",         [](JsonVariantConst v){ Settings_SetPrecipAlert(v.as<bool>()); });
  setIf("financeTickers",  [](JsonVariantConst v){ Settings_SetFinanceTickers(v.as<const char*>()); });

  if (!in["altMin"].isNull() || !in["altMax"].isNull()) {
    uint16_t lo = in["altMin"].isNull() ? s_altMin : in["altMin"].as<uint16_t>();
    uint16_t hi = in["altMax"].isNull() ? s_altMax : in["altMax"].as<uint16_t>();
    Settings_SetAltRangeFt(lo, hi);
    changed = true;
  }

  JsonObjectConst scr = in["screens"];
  if (!scr.isNull()) {
    struct { const char* key; uint8_t idx; } M[] = {
      { "clock",    SCREEN_CLOCK_I },
      { "planes",   SCREEN_PLANES_I },
      { "meteo",    SCREEN_METEO_I },
      { "tactical", SCREEN_TACTICAL_I },
      { "forecast", SCREEN_FORECAST_I },
      { "info",     SCREEN_INFO_I },
      { "finance",  SCREEN_FINANCE_I },
    };
    for (auto& m : M) {
      JsonVariantConst v = scr[m.key];
      if (!v.isNull()) { Settings_SetScreenEnabled(m.idx, v.as<bool>()); changed = true; }
    }
  }
  Settings_BatchEnd();
  return changed;
}

// --- Debounced flush --------------------------------------------------------
void Settings_Tick() {
  if (!s_uiDirty) return;
  if (millis() - s_uiDirtyAt < 2000) return;
  if (prefs.begin(NS, false)) {
    prefs.putUChar("rngP", s_rngP);
    prefs.putUChar("rngM", s_rngM);
    prefs.putUChar("rngT", s_rngT);
    prefs.putUChar("scr",  s_scr);
    prefs.putUShort("topb", s_top);
    prefs.putUChar("bl",   s_briDay);
    prefs.putUChar("blN",  s_briNight);
    prefs.end();
  }
  s_uiDirty = false;
}

void Settings_ClearAll() {
  if (prefs.begin(NS, false)) { prefs.clear(); prefs.end(); }
  s_lat = DEFAULT_LAT; s_lon = DEFAULT_LON; s_hasLoc = false;
  s_briDay = 80; s_briNight = 25; s_nightAuto = true; s_nightOff = 0; s_isNight = false;
  s_metric = false; s_lang = LANG_EN; Lang_Set(s_lang);
  s_scrMask = (1 << SCREEN_CLOCK_I) | (1 << SCREEN_PLANES_I) |
              (1 << SCREEN_METEO_I) | (1 << SCREEN_TACTICAL_I) |
              (1 << SCREEN_FORECAST_I) | (1 << SCREEN_INFO_I) |
              (1 << SCREEN_FINANCE_I);
  strncpy(s_finTickers, DEFAULT_FINANCE_TICKERS, sizeof(s_finTickers) - 1);
  s_autoRot = 0; s_radarSrc = RADAR_SRC_CHMU; s_smoothRadar = true;
  s_secStyle = SEC_STYLE_DOTS; s_clockCol = 0xFFFF; s_secCol = 0x05FF;
  s_clkShowOverhead = true; s_overheadRadiusKm = 10.0f;
  s_precipAlert = true;
  s_altMin = 0; s_altMax = 60000; s_onlyCs = false; s_sqAlert = true; s_watch[0] = '\0';
  s_typeFilterMask = 0x3F;
  s_bzOn = true; s_bzEm = true; s_bzWatch = true; s_bzOverhead = false; s_bzPrecip = false; s_bzTouch = false; s_bzHour = false; s_bzNMute = true;
  s_rngP = 1; s_rngM = 1; s_rngT = 1; s_scr = SCREEN_PLANES_I; s_top = 0;
  s_pw[0] = '\0';
  s_ssid[0] = '\0'; s_wpass[0] = '\0';
  s_wifiNetCount = 0;
  memset(s_wifiNets, 0, sizeof(s_wifiNets));
  strncpy(s_hostname, WEB_HOSTNAME, sizeof(s_hostname) - 1);
  s_uiDirty = false;
}
