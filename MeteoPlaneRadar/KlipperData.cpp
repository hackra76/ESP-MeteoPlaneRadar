// =============================================================================
//  MeteoPlaneRadar
//  KlipperData.cpp - Moonraker / Klipper 3D Printer telemetry data provider.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1
// =============================================================================
#include "KlipperData.h"
#include "Net.h"
#include "AsyncCore.h"
#include "Settings.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "SpiRamAllocator.h"

static KlipperStatus s_klipperData;
static bool s_klipperBusy = false;
static bool s_forceFetch = true;
static unsigned long s_lastFetch = 0;

void Klipper_Init() {
  memset(&s_klipperData, 0, sizeof(s_klipperData));
  strncpy(s_klipperData.state, "offline", sizeof(s_klipperData.state) - 1);
  strncpy(s_klipperData.message, "Waiting for WiFi...", sizeof(s_klipperData.message) - 1);
  s_klipperData.detailedState = KLIPPER_STATE_OFFLINE;
  s_forceFetch = true;
}

void Klipper_RequestFetch() {
  s_forceFetch = true;
}

bool Klipper_IsBusy() {
  return s_klipperBusy;
}

unsigned long Klipper_LastUpdated() {
  return s_klipperData.lastUpdatedMs;
}

bool Klipper_GetData(KlipperStatus* out) {
  if (!out) return false;
  Async_LockKlipper();
  *out = s_klipperData;
  Async_UnlockKlipper();
  return s_klipperData.valid;
}

bool Klipper_IsConnected() {
  Async_LockKlipper();
  bool c = s_klipperData.connected;
  Async_UnlockKlipper();
  return c;
}

// Clean filename by stripping directory path prefix (e.g. "gcodes/sub/test.gcode" -> "test.gcode")
static void sanitizeFilename(const char* in, char* out, size_t outLen) {
  if (!in || !*in) {
    out[0] = '\0';
    return;
  }
  const char* p = strrchr(in, '/');
  if (!p) p = strrchr(in, '\\');
  const char* name = p ? p + 1 : in;
  strncpy(out, name, outLen - 1);
  out[outLen - 1] = '\0';
}

static KlipperDetailedState Klipper_EvaluateDetailedState(const KlipperStatus& k) {
  if (!k.connected) {
    if (strcmp(k.state, "no_host") == 0) return KLIPPER_STATE_NO_HOST;
    return KLIPPER_STATE_OFFLINE;
  }

  // 1. MCU Shutdown check (Klipper emergency stop or thermal protection)
  if (strcasecmp(k.webhookState, "shutdown") == 0) {
    return KLIPPER_STATE_MCU_SHUTDOWN;
  }
  if (strcasecmp(k.webhookState, "startup") == 0) {
    return KLIPPER_STATE_STARTUP;
  }
  if (strcasecmp(k.state, "error") == 0 || strcasecmp(k.webhookState, "error") == 0) {
    return KLIPPER_STATE_ERROR;
  }
  if (strcasecmp(k.state, "cancelled") == 0) {
    return KLIPPER_STATE_CANCELLED;
  }

  // Combine display_status message and print_stats message for keyword analysis
  String msg = String(k.message);
  msg.toLowerCase();

  // 2. Paused states (User pause vs Filament runout / M600)
  if (strcasecmp(k.state, "paused") == 0) {
    if (msg.indexOf("filament") >= 0 || msg.indexOf("runout") >= 0 ||
        msg.indexOf("m600") >= 0 || msg.indexOf("load") >= 0 || msg.indexOf("insert") >= 0) {
      return KLIPPER_STATE_FILAMENT_RUNOUT;
    }
    return KLIPPER_STATE_PAUSED;
  }

  // 3. Complete state
  if (strcasecmp(k.state, "complete") == 0) {
    return KLIPPER_STATE_COMPLETE;
  }

  // 4. Macro / Movement active check:
  // In Klipper, idle_timeout.state is "Printing" whenever G-code or a macro (homing, probing, QGL, mesh) is executing.
  // When idle_timeout.state is "Ready" or "Idle" and velocity is 0, the machine is at rest (standby).
  bool isPrintingFile = (strcasecmp(k.state, "printing") == 0);
  bool isCommandExecuting = (strcasecmp(k.idleState, "printing") == 0 || k.liveVelocity > 0.5f);

  if (isCommandExecuting) {
    if (msg.indexOf("home") >= 0 || msg.indexOf("homing") >= 0 || msg.indexOf("g28") >= 0) {
      return KLIPPER_STATE_HOMING;
    }
    // Physical motion during standby without all axes homed indicates active homing routine
    if (strcasecmp(k.state, "standby") == 0 && strlen(k.homedAxes) < 3 && k.liveVelocity > 0.5f) {
      return KLIPPER_STATE_HOMING;
    }
    if (msg.indexOf("mesh") >= 0 || msg.indexOf("prob") >= 0 || msg.indexOf("qgl") >= 0 ||
        msg.indexOf("level") >= 0 || msg.indexOf("z-tilt") >= 0 || msg.indexOf("tilt") >= 0 ||
        msg.indexOf("kamp") >= 0 || msg.indexOf("beacon") >= 0 || msg.indexOf("cartographer") >= 0) {
      return KLIPPER_STATE_PROBING;
    }
    if (msg.indexOf("clean") >= 0 || msg.indexOf("purg") >= 0 || msg.indexOf("wipe") >= 0 || msg.indexOf("brush") >= 0) {
      return KLIPPER_STATE_CLEANING;
    }
  }

  // 5. Heat soak
  if (msg.indexOf("soak") >= 0 || msg.indexOf("heatsoak") >= 0) {
    return KLIPPER_STATE_HEAT_SOAK;
  }

  // 6. Thermal analysis (Warming up bed or extruder)
  bool bedHeating = (k.bedTarget > 20.0f && (k.bedTarget - k.bedTemp) > 2.5f);
  bool extHeating = (k.extruderTarget > 40.0f && (k.extruderTarget - k.extruderTemp) > 3.0f);
  if (bedHeating || extHeating || (msg.indexOf("heat") >= 0 && isCommandExecuting)) {
    return KLIPPER_STATE_HEATING;
  }

  // 7. Active printing (virtual sdcard printing file)
  if (isPrintingFile) {
    return KLIPPER_STATE_PRINTING;
  }

  // 8. Standby / Cooling
  if (k.extruderTarget == 0.0f && k.bedTarget == 0.0f && (k.extruderTemp > 50.0f || k.bedTemp > 45.0f)) {
    return KLIPPER_STATE_COOLING;
  }

  return KLIPPER_STATE_STANDBY;
}

bool Klipper_Step() {
  if (WiFi.status() != WL_CONNECTED) return false;

  const char* rawHost = Settings_KlipperHost();
  if (!rawHost || strlen(rawHost) == 0) {
    Async_LockKlipper();
    s_klipperData.valid = false;
    s_klipperData.connected = false;
    s_klipperData.detailedState = KLIPPER_STATE_NO_HOST;
    strncpy(s_klipperData.state, "no_host", sizeof(s_klipperData.state) - 1);
    strncpy(s_klipperData.message, "Host not set in Web UI", sizeof(s_klipperData.message) - 1);
    Async_UnlockKlipper();
    return false;
  }

  // Sanitize host and extract scheme, host, and port
  String scheme = "http://";
  String hostStr = String(rawHost);
  hostStr.trim();

  if (hostStr.startsWith("http://") || hostStr.startsWith("HTTP://")) {
    scheme = "http://";
    hostStr.remove(0, 7);
  } else if (hostStr.startsWith("https://") || hostStr.startsWith("HTTPS://")) {
    scheme = "https://";
    hostStr.remove(0, 8);
  }

  int slashIdx = hostStr.indexOf('/');
  if (slashIdx >= 0) {
    hostStr = hostStr.substring(0, slashIdx);
  }
  hostStr.trim();

  uint16_t port = Settings_KlipperPort();
  if (port == 0) port = KLIPPER_DEFAULT_PORT;
  int colonIdx = hostStr.indexOf(':');
  if (colonIdx >= 0) {
    int p = hostStr.substring(colonIdx + 1).toInt();
    if (p > 0 && p <= 65535) port = (uint16_t)p;
    hostStr = hostStr.substring(0, colonIdx);
  }
  hostStr.trim();

  if (hostStr.length() == 0) {
    Async_LockKlipper();
    s_klipperData.valid = false;
    s_klipperData.connected = false;
    s_klipperData.detailedState = KLIPPER_STATE_NO_HOST;
    strncpy(s_klipperData.state, "no_host", sizeof(s_klipperData.state) - 1);
    strncpy(s_klipperData.message, "Invalid host in Web UI", sizeof(s_klipperData.message) - 1);
    Async_UnlockKlipper();
    return false;
  }

  // Resolve .local hostnames via mDNS if not directly an IP address (cached to avoid 1.5s multicast flooding)
  static char s_cachedHost[64] = "";
  static IPAddress s_cachedIp = INADDR_NONE;
  static unsigned long s_lastMdnsLookup = 0;

  IPAddress ip;
  if (!ip.fromString(hostStr) && hostStr.endsWith(".local")) {
    unsigned long nowMs = millis();
    if (strcmp(s_cachedHost, hostStr.c_str()) == 0 && s_cachedIp != INADDR_NONE && (nowMs - s_lastMdnsLookup < 60000UL)) {
      hostStr = s_cachedIp.toString();
    } else {
      String mdnsName = hostStr.substring(0, hostStr.length() - 6);
      IPAddress qip = MDNS.queryHost(mdnsName.c_str(), 1000);
      s_lastMdnsLookup = nowMs;
      if (qip != INADDR_NONE && qip != IPAddress(0, 0, 0, 0)) {
        strncpy(s_cachedHost, hostStr.c_str(), sizeof(s_cachedHost) - 1);
        s_cachedHost[sizeof(s_cachedHost) - 1] = '\0';
        s_cachedIp = qip;
        hostStr = qip.toString();
        Serial.printf("KLIPPER: mDNS %s.local -> %s\n", mdnsName.c_str(), hostStr.c_str());
      } else {
        s_cachedIp = INADDR_NONE;
        Serial.printf("KLIPPER: mDNS query for '%s' failed\n", mdnsName.c_str());
      }
    }
  }

  s_klipperBusy = true;
  s_klipperData.fetching = true;

  String url = scheme + hostStr + ":" + String(port) +
               "/printer/objects/query?print_stats&extruder&heater_bed&toolhead&display_status&virtual_sdcard&webhooks&motion_report&idle_timeout";

  const char* apiKey = Settings_KlipperApiKey();
  const char* hdrName = nullptr;
  const char* hdrVal = nullptr;
  if (apiKey && strlen(apiKey) > 0 && strcmp(apiKey, "***") != 0) {
    url += "&token=" + String(apiKey);
    hdrName = "X-Api-Key";
    hdrVal = apiKey;
  }

  const char* body = nullptr;
  size_t bodyLen = 0;
  if (!Net_GetCString(url.c_str(), &body, "KLIPPER", hdrName, hdrVal, 1500, 2500, &bodyLen) || !body) {
    Serial.printf("KLIPPER: Request failed (%s)\n", url.c_str());
    s_klipperBusy = false;
    s_klipperData.fetching = false;
    s_cachedIp = INADDR_NONE; // Invalidate cached mDNS IP so next retry can re-resolve
    Async_LockKlipper();
    s_klipperData.connected = false;
    s_klipperData.detailedState = KLIPPER_STATE_OFFLINE;
    strncpy(s_klipperData.state, "offline", sizeof(s_klipperData.state) - 1);
    strncpy(s_klipperData.message, "Printer unreachable", sizeof(s_klipperData.message) - 1);
    Async_UnlockKlipper();
    return false;
  }

  // Filter to minimize memory usage
  JsonDocument filter;
  JsonObject resStatus = filter["result"]["status"].to<JsonObject>();
  resStatus["print_stats"]["state"] = true;
  resStatus["print_stats"]["filename"] = true;
  resStatus["print_stats"]["print_duration"] = true;
  resStatus["print_stats"]["total_duration"] = true;
  resStatus["print_stats"]["filament_used"] = true;
  resStatus["print_stats"]["message"] = true;
  resStatus["print_stats"]["info"] = true;
  resStatus["extruder"]["temperature"] = true;
  resStatus["extruder"]["target"] = true;
  resStatus["heater_bed"]["temperature"] = true;
  resStatus["heater_bed"]["target"] = true;
  resStatus["toolhead"]["homed_axes"] = true;
  resStatus["toolhead"]["position"] = true;
  resStatus["motion_report"]["live_velocity"] = true;
  resStatus["idle_timeout"]["state"] = true;
  resStatus["display_status"]["progress"] = true;
  resStatus["display_status"]["message"] = true;
  resStatus["virtual_sdcard"]["progress"] = true;
  resStatus["webhooks"]["state"] = true;
  resStatus["webhooks"]["state_message"] = true;

  JsonDocument doc(SpiRamAllocator::instance());
  DeserializationError err = deserializeJson(doc, body, bodyLen, DeserializationOption::Filter(filter));
  if (err) {
    s_klipperBusy = false;
    s_klipperData.fetching = false;
    Async_LockKlipper();
    s_klipperData.connected = false;
    s_klipperData.detailedState = KLIPPER_STATE_ERROR;
    strncpy(s_klipperData.message, "JSON Parse Error", sizeof(s_klipperData.message) - 1);
    Async_UnlockKlipper();
    return false;
  }

  JsonObject status = doc["result"]["status"];
  if (status.isNull()) {
    s_klipperBusy = false;
    s_klipperData.fetching = false;
    Async_LockKlipper();
    s_klipperData.connected = false;
    s_klipperData.detailedState = KLIPPER_STATE_OFFLINE;
    strncpy(s_klipperData.state, "offline", sizeof(s_klipperData.state) - 1);
    strncpy(s_klipperData.message, "Klippy Not Ready", sizeof(s_klipperData.message) - 1);
    Async_UnlockKlipper();
    return false;
  }

  Async_LockKlipper();
  s_klipperData.connected = true;
  s_klipperData.valid = true;
  s_klipperData.fetching = false;
  s_klipperData.lastUpdatedMs = millis();

  // Print stats
  JsonObject printStats = status["print_stats"];
  if (!printStats.isNull()) {
    const char* st = printStats["state"] | "standby";
    strncpy(s_klipperData.state, st, sizeof(s_klipperData.state) - 1);
    s_klipperData.state[sizeof(s_klipperData.state) - 1] = '\0';

    const char* fn = printStats["filename"] | "";
    sanitizeFilename(fn, s_klipperData.filename, sizeof(s_klipperData.filename));

    s_klipperData.printDuration = (uint32_t)printStats["print_duration"].as<double>();
    s_klipperData.totalDuration = (uint32_t)printStats["total_duration"].as<double>();
    s_klipperData.filamentUsed  = printStats["filament_used"].as<float>();

    const char* msg = printStats["message"] | "";
    if (msg && *msg) {
      strncpy(s_klipperData.message, msg, sizeof(s_klipperData.message) - 1);
      s_klipperData.message[sizeof(s_klipperData.message) - 1] = '\0';
    }

    uint32_t curLayer = 0;
    uint32_t totLayer = 0;
    JsonObject info = printStats["info"];
    if (!info.isNull()) {
      curLayer = (uint32_t)info["current_layer"].as<double>();
      totLayer = (uint32_t)info["total_layer"].as<double>();
    }
    if (curLayer == 0) {
      curLayer = (uint32_t)printStats["current_layer"].as<double>();
    }
    if (totLayer == 0) {
      totLayer = (uint32_t)printStats["total_layer"].as<double>();
    }
    s_klipperData.currentLayer = curLayer;
    s_klipperData.totalLayers = totLayer;
  }

  // Temperatures
  JsonObject extruder = status["extruder"];
  if (!extruder.isNull()) {
    s_klipperData.extruderTemp   = extruder["temperature"] | 0.0f;
    s_klipperData.extruderTarget = extruder["target"] | 0.0f;
  }

  JsonObject heaterBed = status["heater_bed"];
  if (!heaterBed.isNull()) {
    s_klipperData.bedTemp   = heaterBed["temperature"] | 0.0f;
    s_klipperData.bedTarget = heaterBed["target"] | 0.0f;
  }

  // Toolhead
  JsonObject toolhead = status["toolhead"];
  if (!toolhead.isNull()) {
    const char* ha = toolhead["homed_axes"] | "";
    strncpy(s_klipperData.homedAxes, ha, sizeof(s_klipperData.homedAxes) - 1);
    s_klipperData.homedAxes[sizeof(s_klipperData.homedAxes) - 1] = '\0';

    JsonArray pos = toolhead["position"];
    if (!pos.isNull() && pos.size() >= 3) {
      s_klipperData.posX = pos[0] | 0.0f;
      s_klipperData.posY = pos[1] | 0.0f;
      s_klipperData.posZ = pos[2] | 0.0f;
    }
  }

  // Live Velocity
  JsonObject mr = status["motion_report"];
  if (!mr.isNull()) {
    s_klipperData.liveVelocity = mr["live_velocity"] | 0.0f;
  } else {
    s_klipperData.liveVelocity = 0.0f;
  }

  // Idle Timeout State
  JsonObject it = status["idle_timeout"];
  if (!it.isNull()) {
    const char* is = it["state"] | "Ready";
    strncpy(s_klipperData.idleState, is, sizeof(s_klipperData.idleState) - 1);
    s_klipperData.idleState[sizeof(s_klipperData.idleState) - 1] = '\0';
  } else {
    s_klipperData.idleState[0] = '\0';
  }

  // Progress
  float prog = 0.0f;
  JsonObject disp = status["display_status"];
  if (!disp.isNull()) {
    prog = disp["progress"] | 0.0f;
    const char* dmsg = disp["message"] | "";
    if (dmsg && *dmsg) {
      strncpy(s_klipperData.message, dmsg, sizeof(s_klipperData.message) - 1);
      s_klipperData.message[sizeof(s_klipperData.message) - 1] = '\0';
    } else {
      s_klipperData.message[0] = '\0';
    }
  }
  if (prog <= 0.0001f) {
    JsonObject vsd = status["virtual_sdcard"];
    if (!vsd.isNull()) {
      prog = vsd["progress"] | 0.0f;
    }
  }
  s_klipperData.progress = constrain(prog, 0.0f, 1.0f);

  // Fallback: If layer was not provided by SET_PRINT_STATS_INFO, parse from message (M117 / display_status)
  if (s_klipperData.currentLayer == 0 && s_klipperData.message[0] != '\0') {
    const char* p = s_klipperData.message;
    while (*p) {
      if (strncasecmp(p, "layer", 5) == 0 || strncasecmp(p, "vrstva", 6) == 0) {
        p += (strncasecmp(p, "layer", 5) == 0) ? 5 : 6;
        while (*p == ' ' || *p == ':' || *p == '#') p++;
        unsigned long cL = 0, tL = 0;
        if (sscanf(p, "%lu of %lu", &cL, &tL) == 2 ||
            sscanf(p, "%lu z %lu", &cL, &tL) == 2 ||
            sscanf(p, "%lu / %lu", &cL, &tL) == 2 ||
            sscanf(p, "%lu/%lu", &cL, &tL) == 2) {
          s_klipperData.currentLayer = cL;
          s_klipperData.totalLayers = tL;
          break;
        } else if (sscanf(p, "%lu", &cL) == 1 && cL > 0) {
          s_klipperData.currentLayer = cL;
          break;
        }
      } else {
        unsigned long cL = 0, tL = 0;
        if ((sscanf(p, "%lu/%lu", &cL, &tL) == 2 || sscanf(p, "%lu / %lu", &cL, &tL) == 2) && cL > 0 && tL >= cL) {
          s_klipperData.currentLayer = cL;
          s_klipperData.totalLayers = tL;
          break;
        }
      }
      p++;
    }
  }

  // Webhooks
  JsonObject webhooks = status["webhooks"];
  if (!webhooks.isNull()) {
    const char* ws = webhooks["state"] | "";
    strncpy(s_klipperData.webhookState, ws, sizeof(s_klipperData.webhookState) - 1);
    s_klipperData.webhookState[sizeof(s_klipperData.webhookState) - 1] = '\0';
    const char* wm = webhooks["state_message"] | "";
    strncpy(s_klipperData.webhookMsg, wm, sizeof(s_klipperData.webhookMsg) - 1);
    s_klipperData.webhookMsg[sizeof(s_klipperData.webhookMsg) - 1] = '\0';
  } else {
    s_klipperData.webhookState[0] = '\0';
    s_klipperData.webhookMsg[0] = '\0';
  }

  // Dynamic state evaluation
  s_klipperData.detailedState = Klipper_EvaluateDetailedState(s_klipperData);

  Serial.printf("KLIPPER: OK - %s, Tool: %.1fC, Bed: %.1fC, Prog: %.1f%%\n",
                s_klipperData.state, s_klipperData.extruderTemp, s_klipperData.bedTemp,
                s_klipperData.progress * 100.0f);

  Async_UnlockKlipper();

  s_klipperBusy = false;
  return true;
}
