// =============================================================================
//  MeteoPlaneRadar
//  QMI8658.cpp - 6-Axis IMU (Accelerometer + Gyroscope) driver & gesture engine.
//
//  Hardware: QMI8658 on Waveshare ESP32-S3-Touch-LCD-2.1 (I2C 0x6B or 0x6A)
//  Features: Knock / Double-Tap detection, Tilt / Orientation (Pitch/Roll).
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#include "QMI8658.h"
#include "Config.h"
#include <Wire.h>
#include <math.h>

#define QMI_ADDR_PRIMARY   0x6B
#define QMI_ADDR_SECONDARY 0x6A

// Register map
#define REG_WHO_AM_I   0x00
#define REG_CTRL1      0x02
#define REG_CTRL2      0x03
#define REG_CTRL3      0x04
#define REG_CTRL5      0x06
#define REG_CTRL7      0x08
#define REG_STATUS0    0x2E
#define REG_AX_L       0x35

static uint8_t s_i2cAddr = 0;
static bool    s_available = false;
static QMI_Data s_data = {0};
static QMI_DoubleTapCallback s_tapCallback = nullptr;

// Gesture state machine variables
static float s_lastMag = 1.0f;
static unsigned long s_lastTapTime = 0;
static unsigned long s_lockoutUntil = 0;
static uint8_t s_tapCount = 0;

// Tap detection parameters
#define TAP_THRESHOLD_DIFF  0.42f   // Peak jerk difference in g
#define TAP_MIN_INTERVAL_MS 70      // Min time between tap 1 and tap 2 (debounces initial vibration)
#define TAP_MAX_INTERVAL_MS 480     // Max window for second tap
#define DOUBLE_TAP_LOCKOUT  550     // Lockout window after double-tap trigger

static bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(s_i2cAddr);
  Wire.write(reg);
  Wire.write(val);
  return (Wire.endTransmission() == 0);
}

static uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(s_i2cAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom((int)s_i2cAddr, 1);
  return Wire.available() ? Wire.read() : 0;
}

static bool readBurst(uint8_t startReg, uint8_t* buf, size_t len) {
  Wire.beginTransmission(s_i2cAddr);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom((int)s_i2cAddr, (int)len);
  for (size_t i = 0; i < len; i++) {
    if (!Wire.available()) return false;
    buf[i] = Wire.read();
  }
  return true;
}

bool QMI8658_Init() {
  // Test primary address first, then secondary
  s_i2cAddr = QMI_ADDR_PRIMARY;
  uint8_t id = readReg(REG_WHO_AM_I);
  if (id != 0x05) {
    s_i2cAddr = QMI_ADDR_SECONDARY;
    id = readReg(REG_WHO_AM_I);
  }

  if (id != 0x05) {
    Serial.printf("QMI8658: Sensor not detected (WHO_AM_I = 0x%02X)\n", id);
    s_available = false;
    return false;
  }

  Serial.printf("QMI8658: IMU detected at I2C 0x%02X (WHO_AM_I = 0x05)\n", s_i2cAddr);

  // 1. CTRL1: Auto-increment address enable, 50MHz SPI disable, 400kHz I2C
  writeReg(REG_CTRL1, 0x60);

  // 2. CTRL2: Accelerometer: Range +/-4g (0x10), ODR 100Hz (0x03) -> 0x13
  writeReg(REG_CTRL2, 0x13);

  // 3. CTRL3: Gyroscope: Range +/-512dps (0x40), ODR 100Hz (0x03) -> 0x43
  writeReg(REG_CTRL3, 0x43);

  // 4. CTRL5: Low pass filter configuration (LPF enabled)
  writeReg(REG_CTRL5, 0x00);

  // 5. CTRL7: Enable both Accelerometer (bit 0) and Gyroscope (bit 1) -> 0x03
  writeReg(REG_CTRL7, 0x03);

  s_available = true;
  s_lastMag = 1.0f;
  s_tapCount = 0;
  return true;
}

bool QMI8658_Available() {
  return s_available;
}

void QMI8658_OnDoubleTap(QMI_DoubleTapCallback cb) {
  s_tapCallback = cb;
}

void QMI8658_GetData(QMI_Data* out) {
  if (!out) return;
  *out = s_data;
}

bool QMI8658_Tick() {
  if (!s_available) return false;

  unsigned long now = millis();

  // Read 12 bytes: 6 accel bytes (0x35..0x3A) + 6 gyro bytes (0x3B..0x40)
  uint8_t raw[12];
  if (!readBurst(REG_AX_L, raw, 12)) {
    return false;
  }

  int16_t ax_raw = (int16_t)(raw[0] | (raw[1] << 8));
  int16_t ay_raw = (int16_t)(raw[2] | (raw[3] << 8));
  int16_t az_raw = (int16_t)(raw[4] | (raw[5] << 8));

  int16_t gx_raw = (int16_t)(raw[6] | (raw[7] << 8));
  int16_t gy_raw = (int16_t)(raw[8] | (raw[9] << 8));
  int16_t gz_raw = (int16_t)(raw[10] | (raw[11] << 8));

  // Scale factors for +/-4g (8192 LSB/g) and +/-512dps (64 LSB/dps)
  s_data.ax = (float)ax_raw / 8192.0f;
  s_data.ay = (float)ay_raw / 8192.0f;
  s_data.az = (float)az_raw / 8192.0f;

  s_data.gx = (float)gx_raw / 64.0f;
  s_data.gy = (float)gy_raw / 64.0f;
  s_data.gz = (float)gz_raw / 64.0f;

  // Calculate tilt angles
  float pitch = atan2f(-s_data.ax, sqrtf(s_data.ay * s_data.ay + s_data.az * s_data.az)) * 57.29578f;
  float roll  = atan2f(s_data.ay, s_data.az) * 57.29578f;
  s_data.pitch = pitch;
  s_data.roll  = roll;

  // Total acceleration magnitude
  float mag = sqrtf(s_data.ax * s_data.ax + s_data.ay * s_data.ay + s_data.az * s_data.az);
  float diff = fabsf(mag - s_lastMag);
  s_lastMag = mag;

  // Double-tap processing
  bool triggered = false;

  if (now > s_lockoutUntil) {
    if (diff >= TAP_THRESHOLD_DIFF) {
      if (s_tapCount == 0) {
        s_tapCount = 1;
        s_lastTapTime = now;
      } else if (s_tapCount == 1) {
        unsigned long dt = now - s_lastTapTime;
        if (dt >= TAP_MIN_INTERVAL_MS && dt <= TAP_MAX_INTERVAL_MS) {
          triggered = true;
          s_tapCount = 0;
          s_lockoutUntil = now + DOUBLE_TAP_LOCKOUT;
          Serial.printf("QMI8658: Double-tap detected! (dt = %lu ms, jerk = %.2fg)\n", dt, diff);
          if (s_tapCallback) {
            s_tapCallback();
          }
        } else if (dt > TAP_MAX_INTERVAL_MS) {
          // Window expired, treat this tap as a new first tap
          s_tapCount = 1;
          s_lastTapTime = now;
        }
      }
    } else {
      // Timeout single tap
      if (s_tapCount == 1 && (now - s_lastTapTime > TAP_MAX_INTERVAL_MS)) {
        s_tapCount = 0;
      }
    }
  }

  return triggered;
}
