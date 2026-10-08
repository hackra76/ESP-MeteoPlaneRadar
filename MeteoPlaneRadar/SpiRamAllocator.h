#pragma once
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

// Custom allocator directing ArduinoJson memory allocations directly to PSRAM,
// preserving internal SRAM for network buffers and mbedTLS handshakes.
class SpiRamAllocator : public ArduinoJson::Allocator {
 public:
  void* allocate(size_t size) override {
    void* p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : malloc(size);
  }
  void deallocate(void* ptr) override {
    free(ptr);
  }
  void* reallocate(void* ptr, size_t new_size) override {
    if (new_size == 0) {
      heap_caps_free(ptr);
      return nullptr;
    }
    if (ptr == nullptr) {
      return heap_caps_malloc(new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    
    // Bypass ESP-IDF TLSF heap_caps_realloc bugs (in-place merge corruption)
    // by manually allocating a new block, copying the data, and freeing the old one.
    void* new_ptr = heap_caps_malloc(new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!new_ptr) {
      // If PSRAM is completely full, we fail safely without touching the old block.
      // (ArduinoJson handles NULL gracefully).
      return nullptr;
    }
    
    size_t old_size = heap_caps_get_allocated_size(ptr);
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    heap_caps_free(ptr);
    
    return new_ptr;
  }
  static SpiRamAllocator* instance() {
    static SpiRamAllocator s_alloc;
    return &s_alloc;
  }
};
