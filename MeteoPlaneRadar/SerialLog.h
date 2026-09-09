// =============================================================================
//  MeteoPlaneRadar
//  SerialLog.h - High-performance PSRAM ring buffer & Web Serial Monitor proxy.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <Stream.h>

void     SerialLog_Init();
void     SerialLog_Write(const uint8_t* data, size_t len);
size_t   SerialLog_Read(uint32_t sinceOffset, char* dest, size_t destCap, uint32_t* nextOffset, bool* overflow);
void     SerialLog_Clear();
uint32_t SerialLog_GetHead();

class WebSerialProxy : public Stream {
public:
  void begin(unsigned long baud = 0) {
    ::Serial.begin(baud);
    SerialLog_Init();
  }
  void end() { ::Serial.end(); }
  int available() override { return ::Serial.available(); }
  int peek() override { return ::Serial.peek(); }
  int read() override { return ::Serial.read(); }
  void flush() override { ::Serial.flush(); }

  size_t write(uint8_t c) override {
    ::Serial.write(c);
    SerialLog_Write(&c, 1);
    return 1;
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    ::Serial.write(buffer, size);
    SerialLog_Write(buffer, size);
    return size;
  }

  using Print::write;

  operator bool() const { return (bool)::Serial; }
  void updateBaudRate(unsigned long baud) { ::Serial.updateBaudRate(baud); }
  uint32_t baudRate() { return ::Serial.baudRate(); }
};

extern WebSerialProxy WebSerial;

#ifndef SERIAL_LOG_NO_MACRO
#ifdef Serial
#undef Serial
#endif
#define Serial WebSerial
#endif
