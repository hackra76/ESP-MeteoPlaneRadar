// =============================================================================
//  MeteoPlaneRadar
//  QMI8658.h - 6-Axis IMU (Accelerometer + Gyroscope) driver & gesture engine.
//
//  Hardware: QMI8658 6-axis IMU on Waveshare ESP32-S3-Touch-LCD-2.1 (I2C 0x6B/0x6A)
//  Features: Knock / Double-Tap detection, Tilt / Orientation (Pitch/Roll).
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#pragma once
#include <Arduino.h>

// Event callback types
typedef void (*QMI_DoubleTapCallback)();

// Initialize the QMI8658 sensor on the I2C bus.
// Returns true if sensor is detected and successfully configured.
bool QMI8658_Init();

// Check if QMI8658 sensor was detected during boot
bool QMI8658_Available();

// Periodic tick for sampling and double-tap gesture processing.
// Returns true if a physical double-tap was detected during this tick.
bool QMI8658_Tick();

// Set callback for double-tap event
void QMI8658_OnDoubleTap(QMI_DoubleTapCallback cb);

// Orientation and raw data structures
struct QMI_Data {
  float ax, ay, az;     // Acceleration in g
  float gx, gy, gz;     // Angular velocity in deg/s
  float pitch, roll;    // Tilt angles in degrees
};

// Retrieve latest sensor readings
void QMI8658_GetData(QMI_Data* out);
