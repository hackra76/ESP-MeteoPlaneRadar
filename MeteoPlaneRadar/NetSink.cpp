// =============================================================================
//  MeteoPlaneRadar
//  See NetSink.h.
//
//  Direct, zero-allocation stream reader for HTTP/HTTPS responses.
//  Bypasses HTTPClient::writeToStreamDataBlock() to avoid intermediate 4KB
//  internal SRAM allocations and prevent unbudgeted delay(1) spins on Core 0.
// =============================================================================
#include "NetSink.h"
#include "Config.h"      // NET_BODY_BUDGET_MS, NET_MIN_HEAP, etc.
#include "Watchdog.h"
#include <HTTPClient.h>
#include <string.h>      // memcpy / memmove
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class AccessibleHTTPClient : public HTTPClient {
public:
  bool isChunked() const { return _transferEncoding == HTTPC_TE_CHUNKED; }
  int getRawSize() const { return _size; }
};

// Helper: read a single line terminated by LF (stripping CR) with strict timeout
static bool readChunkLine(NetworkClient* stream, HTTPClient& http, char* line, size_t maxLen,
                          uint32_t lineTimeoutMs, uint32_t budgetStartMs, uint32_t budgetMs,
                          void (*poll)()) {
  size_t idx = 0;
  uint32_t start = millis();
  while (millis() - start < lineTimeoutMs) {
    Watchdog_Feed();
    if (poll) poll();
    if (budgetMs && (millis() - budgetStartMs > budgetMs)) return false;

    if (stream->available() > 0) {
      char c = (char)stream->read();
      if (c == '\n') {
        line[idx] = '\0';
        return true;
      }
      if (c != '\r' && idx + 1 < maxLen) {
        line[idx++] = c;
      }
    } else {
      if (!http.connected() && stream->available() == 0) return false;
      vTaskDelay(pdMS_TO_TICKS(2));
    }
  }
  return false;
}

long Net_ReadBody(HTTPClient& http, uint8_t* buf, size_t cap, const char* tag,
                  void (*poll)()) {
  if (!buf || cap == 0) return -1;

  NetworkClient* stream = http.getStreamPtr();
  if (!stream) {
    Serial.printf("%s: null stream\n", tag);
    return -1;
  }

  const bool chunked = ((AccessibleHTTPClient&)http).isChunked();
  const int declared = http.getSize();
  const uint32_t budgetMs = NET_BODY_BUDGET_MS;
  const uint32_t startMs = millis();
  uint32_t lastDataMs = millis();
  size_t totalRead = 0;

  if (chunked) {
    // -------------------------------------------------------------------------
    // Chunked Transfer Encoding Stream Decoder
    // -------------------------------------------------------------------------
    while (true) {
      Watchdog_Feed();
      if (poll) poll();

      if (budgetMs && (millis() - startMs > budgetMs)) {
        Serial.printf("%s: transfer exceeded %lu ms, aborted\n", tag, (unsigned long)budgetMs);
        return -1;
      }

      char lineBuf[32];
      if (!readChunkLine(stream, http, lineBuf, sizeof(lineBuf), 5000, startMs, budgetMs, poll)) {
        Serial.printf("%s: chunk header read timeout\n", tag);
        return -1;
      }

      char* p = lineBuf;
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '\0') {
        // Empty CRLF line between chunks, continue to next chunk header
        continue;
      }

      // Strip optional chunk extensions (e.g. "1a4;ext=val")
      char* semi = strchr(p, ';');
      if (semi) *semi = '\0';

      long chunkSize = strtol(p, nullptr, 16);
      if (chunkSize <= 0) {
        // Terminal chunk (0): drain trailing CRLF if data is already available
        if (http.connected()) {
          uint32_t trailStart = millis();
          while (http.connected() && (millis() - trailStart < 50)) {
            if (stream->available() > 0) stream->read();
            else break;
          }
        }
        break; // Finished chunked transfer successfully
      }

      // Read chunk payload
      size_t chunkRead = 0;
      uint32_t chunkStartMs = millis();
      while (chunkRead < (size_t)chunkSize) {
        Watchdog_Feed();
        if (poll) poll();

        if (budgetMs && (millis() - startMs > budgetMs)) {
          Serial.printf("%s: transfer exceeded %lu ms, aborted\n", tag, (unsigned long)budgetMs);
          return -1;
        }

        int avail = stream->available();
        if (avail > 0) {
          size_t room = (cap > totalRead + 1) ? (cap - 1 - totalRead) : 0;
          if (room == 0) {
            Serial.printf("%s: response exceeded %u B, discarded\n", tag, (unsigned)cap);
            return -1;
          }
          size_t need = (size_t)chunkSize - chunkRead;
          size_t take = (need < (size_t)avail) ? need : (size_t)avail;
          if (take > room) take = room;
          int got = stream->read(buf + totalRead, take);
          if (got > 0) {
            totalRead += got;
            chunkRead += got;
            chunkStartMs = millis();
          }
        } else {
          if (!http.connected()) {
            Serial.printf("%s: connection lost mid-chunk\n", tag);
            return -1;
          }
          if (millis() - chunkStartMs > 5000) {
            Serial.printf("%s: chunk data stalled\n", tag);
            return -1;
          }
          vTaskDelay(pdMS_TO_TICKS(2));
        }
      }
    }
  } else {
    // -------------------------------------------------------------------------
    // Identity / Content-Length / Connection-Close Stream
    // -------------------------------------------------------------------------
    while (true) {
      Watchdog_Feed();
      if (poll) poll();

      if (budgetMs && (millis() - startMs > budgetMs)) {
        Serial.printf("%s: transfer exceeded %lu ms, aborted\n", tag, (unsigned long)budgetMs);
        return -1;
      }

      if (declared >= 0 && (int)totalRead >= declared) {
        break; // Complete Content-Length body received
      }

      int avail = stream->available();
      if (avail > 0) {
        lastDataMs = millis();
        size_t room = (cap > totalRead + 1) ? (cap - 1 - totalRead) : 0;
        if (room == 0) {
          Serial.printf("%s: response exceeded %u B, discarded\n", tag, (unsigned)cap);
          return -1;
        }
        size_t toRead = (size_t)avail;
        if (toRead > room) toRead = room;
        if (declared >= 0 && toRead > (size_t)(declared - totalRead)) {
          toRead = (size_t)(declared - totalRead);
        }
        int got = stream->read(buf + totalRead, toRead);
        if (got > 0) {
          totalRead += got;
        }
      } else {
        if (!http.connected()) {
          break; // Socket closed and buffer empty -> EOF reached cleanly
        }
        if (millis() - lastDataMs > 4000) {
          if (declared < 0 && totalRead > 0) {
            break; // No content-length, data received, 4s silence -> treat as complete
          }
          Serial.printf("%s: transfer stalled (4s silence), aborted\n", tag);
          return -1;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
      }
    }
  }

  // Guarantee NUL termination for text consumers
  buf[totalRead] = '\0';
  return (long)totalRead;
}

long Net_ScanBody(HTTPClient& http, NetScanFn cb, void* user, const char* tag,
                  void (*poll)()) {
  if (!cb) return -1;

  NetworkClient* stream = http.getStreamPtr();
  if (!stream) {
    Serial.printf("%s: null stream\n", tag);
    return -1;
  }

  const int declared = http.getSize();
  const uint32_t budgetMs = NET_BODY_BUDGET_MS;
  const uint32_t startMs = millis();
  uint32_t lastDataMs = millis();

  const size_t WIN = 512;
  const size_t OVERLAP = NET_SCAN_MAX_TOKEN;
  char win[WIN + 1];
  size_t fill = 0;
  size_t total = 0;
  bool stopped = false;

  auto feedWindow = [&](const uint8_t* data, size_t len) {
    size_t done = 0;
    while (done < len && !stopped) {
      size_t room = WIN - fill;
      size_t take = (len - done < room) ? (len - done) : room;
      memcpy(win + fill, data + done, take);
      fill += take;
      done += take;
      if (fill == WIN) {
        win[fill] = '\0';
        if (cb && !cb(win, user)) {
          stopped = true;
          break;
        }
        if (fill > OVERLAP) {
          memmove(win, win + fill - OVERLAP, OVERLAP);
          fill = OVERLAP;
        }
      }
    }
    total += done;
  };

  uint8_t readBlock[256];

  while (!stopped) {
    Watchdog_Feed();
    if (poll) poll();

    if (budgetMs && (millis() - startMs > budgetMs)) {
      Serial.printf("%s: transfer exceeded %lu ms, aborted\n", tag, (unsigned long)budgetMs);
      return -1;
    }

    if (declared >= 0 && (int)total >= declared) {
      break;
    }

    int avail = stream->available();
    if (avail > 0) {
      lastDataMs = millis();
      size_t take = (size_t)avail;
      if (take > sizeof(readBlock)) take = sizeof(readBlock);
      if (declared >= 0 && take > (size_t)(declared - total)) {
        take = (size_t)(declared - total);
      }
      int got = stream->read(readBlock, take);
      if (got > 0) {
        feedWindow(readBlock, got);
      }
    } else {
      if (!http.connected()) {
        break;
      }
      if (millis() - lastDataMs > 4000) {
        if (declared < 0 && total > 0) break;
        Serial.printf("%s: transfer stalled (4s silence), aborted\n", tag);
        return -1;
      }
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }

  // Scan whatever is left in the window
  if (fill > 0 && !stopped && cb) {
    win[fill] = '\0';
    cb(win, user);
  }

  return (long)total;
}

bool Net_HeapOk(const char* tag) {
  const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  size_t freeInt = heap_caps_get_free_size(caps);
  size_t maxBlock = heap_caps_get_largest_free_block(caps);
  bool ok = (freeInt >= NET_MIN_HEAP && maxBlock >= NET_MIN_BLOCK);
  if (ok) {
    // Fast-path: if a single contiguous block can hold all three TLS allocations
    // (2x record buffers + handshake state + allocator overhead), skip heap probing.
    const size_t combinedNeed = (2 * NET_TLS_BUF_BYTES) + NET_TLS_STATE_BYTES + 2048;
    if (maxBlock >= combinedNeed && freeInt >= NET_MIN_HEAP + 12288) {
      return true;
    }
    // Otherwise probe the real mbedTLS allocation pattern (in + out buffer + handshake
    // state) to verify fragmentation and give it straight back.
    void* in  = heap_caps_malloc(NET_TLS_BUF_BYTES, caps);
    void* out = in  ? heap_caps_malloc(NET_TLS_BUF_BYTES, caps) : nullptr;
    void* hs  = out ? heap_caps_malloc(NET_TLS_STATE_BYTES, caps) : nullptr;
    ok = (in && out && hs);
    if (hs)  heap_caps_free(hs);
    if (out) heap_caps_free(out);
    if (in)  heap_caps_free(in);
    if (ok) return true;
  }
  Serial.printf("%s: low memory (free %u B, max block %u B, need 2x%u B + %u B), fetch deferred\n",
                tag, (unsigned)freeInt, (unsigned)maxBlock,
                (unsigned)NET_TLS_BUF_BYTES, (unsigned)NET_TLS_STATE_BYTES);
  return false;
}
