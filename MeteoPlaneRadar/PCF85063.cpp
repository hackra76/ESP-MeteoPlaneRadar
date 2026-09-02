// =============================================================================
//  MeteoPlaneRadar
//  PCF85063 Real-Time Clock driver (I2C address 0x51).
// =============================================================================
#include "PCF85063.h"
#include <Wire.h>

#define PCF85063_REG_CTRL1 0x00
#define PCF85063_REG_CTRL2 0x01
#define PCF85063_REG_SEC   0x04

static inline uint8_t bcd2dec(uint8_t val) {
  return ((val >> 4) * 10) + (val & 0x0F);
}

static inline uint8_t dec2bcd(uint8_t val) {
  return ((val / 10) << 4) | (val % 10);
}

bool PCF85063_Init() {
  Wire.beginTransmission(PCF85063_ADDR);
  byte err = Wire.endTransmission();
  if (err != 0) {
    Serial.println("RTC: PCF85063 nenalezen na I2C (0x51)");
    return false;
  }
  Serial.println("RTC: PCF85063 detekovan na I2C (0x51)");
  return true;
}

bool PCF85063_ReadTime(struct tm* out) {
  if (!out) return false;
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(PCF85063_REG_SEC);
  if (Wire.endTransmission() != 0) return false;

  if (Wire.requestFrom((int)PCF85063_ADDR, 7) != 7) return false;

  uint8_t sec   = Wire.read();
  uint8_t min   = Wire.read();
  uint8_t hour  = Wire.read();
  uint8_t mday  = Wire.read();
  uint8_t wday  = Wire.read();
  uint8_t mon   = Wire.read();
  uint8_t year  = Wire.read();

  // Bit 7 of seconds register is OS (Oscillator Stop flag)
  if (sec & 0x80) {
    Serial.println("RTC: PCF85063 oscilator byl zastaven (ztrata napajeni)");
    return false;
  }

  int s = bcd2dec(sec & 0x7F);
  int m = bcd2dec(min & 0x7F);
  int h = bcd2dec(hour & 0x3F);
  int d = bcd2dec(mday & 0x3F);
  int mo = bcd2dec(mon & 0x1F);
  int y = bcd2dec(year) + 2000;

  if (y < 2025 || mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || m > 59 || s > 59) {
    return false;
  }

  out->tm_sec = s;
  out->tm_min = m;
  out->tm_hour = h;
  out->tm_mday = d;
  out->tm_mon = mo - 1;
  out->tm_year = y - 1900;
  out->tm_wday = wday & 0x07;
  out->tm_isdst = -1;
  return true;
}

bool PCF85063_SetTime(time_t epochUtc) {
  struct tm tm;
  gmtime_r(&epochUtc, &tm);

  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(PCF85063_REG_SEC);
  Wire.write(dec2bcd(tm.tm_sec)); // OS bit 7 is 0 (oscillator running)
  Wire.write(dec2bcd(tm.tm_min));
  Wire.write(dec2bcd(tm.tm_hour));
  Wire.write(dec2bcd(tm.tm_mday));
  Wire.write(tm.tm_wday & 0x07);
  Wire.write(dec2bcd(tm.tm_mon + 1));
  Wire.write(dec2bcd(tm.tm_year % 100));
  return Wire.endTransmission() == 0;
}
