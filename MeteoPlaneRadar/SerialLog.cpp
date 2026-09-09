// =============================================================================
//  MeteoPlaneRadar
//  SerialLog.cpp - High-performance PSRAM ring buffer & Web Serial Monitor proxy.
// =============================================================================
#define SERIAL_LOG_NO_MACRO
#include "SerialLog.h"
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>

#define SERIAL_LOG_CAPACITY 65536 // 64 KB in PSRAM

WebSerialProxy WebSerial;

static char* s_buffer = nullptr;
static uint32_t s_totalWritten = 0;
static SemaphoreHandle_t s_mutex = nullptr;
static vprintf_like_t s_origVprintf = nullptr;

static int custom_vprintf(const char *format, va_list args) {
  if (s_origVprintf) {
    va_list copy1;
    va_copy(copy1, args);
    s_origVprintf(format, copy1);
    va_end(copy1);
  }
  char temp[256];
  va_list copy2;
  va_copy(copy2, args);
  int len = vsnprintf(temp, sizeof(temp), format, copy2);
  va_end(copy2);
  if (len > 0) {
    SerialLog_Write((const uint8_t*)temp, len < (int)sizeof(temp) ? len : sizeof(temp) - 1);
  }
  return len;
}

void SerialLog_Init() {
  if (!s_mutex) {
    s_mutex = xSemaphoreCreateMutex();
  }
  if (!s_buffer) {
    s_buffer = (char*)heap_caps_malloc(SERIAL_LOG_CAPACITY, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_buffer) {
      s_buffer = (char*)malloc(16384);
    }
  }
  if (!s_origVprintf) {
    s_origVprintf = esp_log_set_vprintf(custom_vprintf);
  }
}

void SerialLog_Write(const uint8_t* data, size_t len) {
  if (!data || len == 0) return;
  if (!s_buffer) SerialLog_Init();
  if (!s_buffer) return;

  if (s_mutex && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    const size_t cap = SERIAL_LOG_CAPACITY;
    for (size_t i = 0; i < len; i++) {
      s_buffer[s_totalWritten % cap] = (char)data[i];
      s_totalWritten++;
    }
    xSemaphoreGive(s_mutex);
  }
}

uint32_t SerialLog_GetHead() {
  return s_totalWritten;
}

void SerialLog_Clear() {
  if (s_mutex && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    s_totalWritten = 0;
    if (s_buffer) memset(s_buffer, 0, SERIAL_LOG_CAPACITY);
    xSemaphoreGive(s_mutex);
  }
}

size_t SerialLog_Read(uint32_t sinceOffset, char* dest, size_t destCap, uint32_t* nextOffset, bool* overflow) {
  if (!dest || destCap == 0) {
    if (nextOffset) *nextOffset = s_totalWritten;
    if (overflow) *overflow = false;
    return 0;
  }
  if (!s_buffer) {
    dest[0] = '\0';
    if (nextOffset) *nextOffset = 0;
    if (overflow) *overflow = false;
    return 0;
  }

  size_t bytesRead = 0;
  if (s_mutex && xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    const uint32_t curTotal = s_totalWritten;
    const uint32_t cap = SERIAL_LOG_CAPACITY;
    bool hasOverflow = false;

    uint32_t start = sinceOffset;
    if (start == 0) {
      // Initial load: return last ~8 KB of history if available
      uint32_t initChunk = 8192;
      if (initChunk > destCap - 1) initChunk = destCap - 1;
      if (curTotal > initChunk) {
        start = curTotal - initChunk;
      } else {
        start = 0;
      }
    } else if (curTotal > start + cap) {
      start = curTotal - cap;
      hasOverflow = true;
    }

    if (start > curTotal) start = curTotal;
    uint32_t toRead = curTotal - start;
    if (toRead > destCap - 1) {
      toRead = destCap - 1;
      hasOverflow = true;
    }

    for (uint32_t i = 0; i < toRead; i++) {
      dest[i] = s_buffer[(start + i) % cap];
    }
    dest[toRead] = '\0';
    bytesRead = toRead;

    if (nextOffset) *nextOffset = start + toRead;
    if (overflow) *overflow = hasOverflow;

    xSemaphoreGive(s_mutex);
  } else {
    if (nextOffset) *nextOffset = sinceOffset;
    if (overflow) *overflow = false;
    dest[0] = '\0';
  }
  return bytesRead;
}
