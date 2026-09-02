// =============================================================================
//  MeteoPlaneRadar
//  TCA9554 I/O expander (LCD reset / CS / power control).
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#include "TCA9554.h"
#include "AsyncCore.h"

// Shadow copy of the output register (we flip individual bits).
static uint8_t s_output = 0xFF;

static bool writeReg(uint8_t reg, uint8_t val) {
  Async_LockI2C();
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(reg);
  Wire.write(val);
  bool ok = (Wire.endTransmission() == 0);
  Async_UnlockI2C();
  return ok;
}

// ok = false means the expander did not answer; the value is then meaningless
// (a failed read used to come back as 0xFF, which looks like a valid register).
static uint8_t readReg(uint8_t reg, bool* ok = nullptr) {
  if (ok) *ok = false;
  Async_LockI2C();
  Wire.beginTransmission(TCA9554_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) {
    Async_UnlockI2C();
    return 0xFF;
  }
  if (Wire.requestFrom((int)TCA9554_ADDR, 1) != 1) {
    Async_UnlockI2C();
    return 0xFF;
  }
  if (!Wire.available()) {
    Async_UnlockI2C();
    return 0xFF;
  }
  if (ok) *ok = true;
  uint8_t val = Wire.read();
  Async_UnlockI2C();
  return val;
}

void TCA9554_Init() {
  // 0 = output for every pin
  writeReg(TCA9554_CONFIG_REG, 0x00);
  s_output = 0xFF;
  writeReg(TCA9554_OUTPUT_REG, s_output);
}

bool TCA9554_SetPin(uint8_t pin, bool high) {
  // pin 1-8 -> bit 0-7
  uint8_t bit = pin - 1;
  if (high) s_output |= (1 << bit);
  else      s_output &= ~(1 << bit);
  // Two tries: this register carries the display's reset and power lines, so a
  // dropped write is not something to shrug off.
  if (writeReg(TCA9554_OUTPUT_REG, s_output)) return true;
  delay(2);
  if (writeReg(TCA9554_OUTPUT_REG, s_output)) return true;
  Serial.println("TCA9554: zapis selhal");
  return false;
}

bool TCA9554_Verify() {
  bool ok = false;
  uint8_t now = readReg(TCA9554_OUTPUT_REG, &ok);
  if (!ok) {
    Serial.println("TCA9554: expander neodpovida");
    return false;
  }
  if (now == s_output) return true;
  // The register drifted from what we last wrote. Either a write got corrupted
  // or the chip reset itself. Whatever the cause, some of those bits are the
  // display's power and reset - put them back.
  Serial.printf("TCA9554: registr se rozesel (cteno 0x%02X, ocekavano 0x%02X), opravuji\n",
                now, s_output);
  writeReg(TCA9554_CONFIG_REG, 0x00);      // all pins outputs again
  writeReg(TCA9554_OUTPUT_REG, s_output);
  return false;
}
