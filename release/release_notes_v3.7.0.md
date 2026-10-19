# MeteoPlaneRadar v3.7.0 — Concurrency Hardening, Progressive JPEG Photo Engine & Rendering Overhaul

**MeteoPlaneRadar v3.7.0** delivers a major performance and stability upgrade across the entire firmware stack. It introduces **bounded $O(N)$ top-$K$ ADS-B flight parsing**, **robust Planespotters progressive JPEG thumbnail decoding**, **PSRAM anti-tearing bounce buffering**, **hardened Core 0/1 concurrency**, and **zero-allocation web streaming**.

---

## 🌟 Highlights & New Features

### ✈️ Planespotters Progressive JPEG Photo Engine
- **Full Support for Progressive JPEGs**:
  - Aircraft preview thumbnails from Planespotters CDN (`t.plnspttrs.net`) are frequently encoded as progressive JPEGs (`SOF2` / `0xFFC2`).
  - Integrated `stb_image.h` with custom PSRAM dynamic memory allocators (`ps_malloc` / `ps_realloc` / `free`) and an optimized box-filter downsampling stage directly into 200×133 RGB565 framebuffers.
  - Eliminates thumbnail decoding failures, image corruption, and memory fragmentation while browsing aircraft in the Aero Glass Flight Deck HUD.

### ⚡ Bounded $O(N)$ ADS-B Flight Parsing
- **Algorithmic Complexity Reduction**:
  - Replaced legacy $O(N^2)$ aircraft distance selection sort with an $O(N)$ bounded min-heap insertion algorithm during JSON stream parsing.
  - Hoisted global coordinate calculations, imperial/metric conversions, and unit scaling outside per-aircraft processing loops.
  - Replaced heap-allocating raw String parsing buffers with PSRAM streaming buffers, drastically reducing internal SRAM heap fragmentation under heavy airspace traffic (>150 aircraft).

### 🖥️ Display Pipeline & Anti-Tearing (Bounce Buffering)
- **Zero-Tearing RGB Display Driver**:
  - Enabled a dedicated hardware bounce buffer (`480 * 10` pixels) in `Arduino_ESP32RGBPanel`.
  - Prevents Octal PSRAM bus bandwidth starvation, flicker, and screen tearing during simultaneous DMA panel refresh, background Wi-Fi transfers, and flash I/O.

### 🛡️ Core Concurrency Hardening & Scheduler Overhaul
- **Deadlock & Starvation Elimination**:
  - Implemented an RAII `NetBusyGuard` to guarantee clean network state resets across all execution paths.
  - Fixed radar scheduler logic (`radarDue`) in the Core 0 network dispatch loop, preventing precipitation radar fetches from starving subsequent background telemetry tasks (ADS-B, Forecast, Markets, ISS, YouTube, Klipper, PetBrain, DisasterAlert).
  - Rate-limited complication weather retries with cache invalidation (`Forecast_Invalidate()`), eliminating HTTP 503 boot loops.

### 🎯 Radar & Sonar Trigonometric Acceleration
- **Pre-computed Trigonometric Lookups**:
  - Screen Sonar (`ScreenSonar.cpp`) and Screen Tactical (`ScreenTactical.cpp`) now utilize pre-computed sine and cosine lookup tables (`LUT_SinDeg` / `LUT_CosDeg`), removing per-frame `sinf()` and `cosf()` calls.
  - Cached dark-mode desaturated radar frames in PSRAM, avoiding repetitive pixel-by-pixel color space transformations on every render tick.
  - Fixed cross-dissolve blend bounds in `ScreenWeather.cpp` when operating at 200 km downsampled range.

### 🛰️ ISS Orbit Tracker Redraw Optimization
- **Path Downsampling & Redraw Dirty Flags**:
  - Downsampled ground-track path computation to 90 points with bounding-box culling.
  - Added redraw dirty-flag caching (`s_prevIssX`/`s_prevIssY`) to bypass unnecessary full-screen canvas rebuilds when the station position hasn't shifted display pixels.
  - Migrated 12 KB JSON processing buffer from task stack to PSRAM.

### 🌐 Embedded Web Server Chunked Streaming
- **Heap Pressure Reduction**:
  - Implemented `WebServerPrint` streaming adapter to stream large HTML templates directly in chunks, saving 6–14 KB heap reservations per web request.
  - Optimized `/api/screenshot.bmp` to stream line-by-line directly from display memory under the LVGL mutex, completely eliminating the 460.8 KB PSRAM snapshot allocation.
  - Enabled HTTP connection keep-alive reuse (`http.setReuse(true)`) across sequential Slippy Map tile fetches.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.7.0-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.7.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.7.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
