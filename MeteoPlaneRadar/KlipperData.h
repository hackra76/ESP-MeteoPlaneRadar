// =============================================================================
//  MeteoPlaneRadar
//  KlipperData.h - Moonraker / Klipper 3D Printer telemetry data provider.
//
//  Communicates with Moonraker REST API on the local LAN/WiFi:
//  GET /printer/objects/query?print_stats&extruder&heater_bed&toolhead&display_status&virtual_sdcard
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1
// =============================================================================
#pragma once
#include <Arduino.h>

enum KlipperDetailedState : uint8_t {
  KLIPPER_STATE_OFFLINE = 0,
  KLIPPER_STATE_NO_HOST,
  KLIPPER_STATE_MCU_SHUTDOWN,
  KLIPPER_STATE_STARTUP,
  KLIPPER_STATE_STANDBY,
  KLIPPER_STATE_HOMING,
  KLIPPER_STATE_PROBING,        // Bed mesh / QGL / Leveling
  KLIPPER_STATE_HEATING,        // Warming up bed or extruder
  KLIPPER_STATE_HEAT_SOAK,      // Heat soak countdown/chamber
  KLIPPER_STATE_CLEANING,       // Nozzle purge / brush line
  KLIPPER_STATE_PRINTING,       // Active extrusion/printing
  KLIPPER_STATE_PAUSED,         // User paused
  KLIPPER_STATE_FILAMENT_RUNOUT,// Runout sensor / M600 change
  KLIPPER_STATE_COOLING,        // Heaters turned off but > 45-50°C
  KLIPPER_STATE_COMPLETE,       // Print finished
  KLIPPER_STATE_CANCELLED,      // Aborted/cancelled
  KLIPPER_STATE_ERROR           // General error
};

struct KlipperStatus {
  bool valid;
  bool connected;
  bool fetching;
  char state[24];         // Raw Klipper print_stats.state
  char message[64];       // display_status.message or state message
  char filename[64];      // print_stats.filename
  float extruderTemp;     // actual temperature in °C
  float extruderTarget;   // target temperature in °C
  float bedTemp;          // actual bed temperature in °C
  float bedTarget;        // target bed temperature in °C
  char homedAxes[8];      // "xyz", "xy", etc.
  float posX, posY, posZ; // toolhead position in mm
  float liveVelocity;     // current toolhead velocity in mm/s from motion_report
  float progress;         // 0.0 .. 1.0 (0% .. 100%)
  uint32_t printDuration; // elapsed printing duration in seconds
  uint32_t totalDuration; // total job duration in seconds
  float filamentUsed;     // filament used in mm
  uint32_t currentLayer;  // current print layer (from print_stats.info or message)
  uint32_t totalLayers;   // total print layers (from print_stats.info or message)
  char webhookState[16];  // "ready", "startup", "shutdown", "error"
  char webhookMsg[96];    // shutdown or error details
  char idleState[16];     // "Idle", "Ready", "Printing" from idle_timeout
  KlipperDetailedState detailedState;
  unsigned long lastUpdatedMs;
};

void Klipper_Init();
bool Klipper_Step();
bool Klipper_GetData(KlipperStatus* out);
bool Klipper_IsConnected();
void Klipper_RequestFetch();
bool Klipper_IsBusy();
unsigned long Klipper_LastUpdated();
