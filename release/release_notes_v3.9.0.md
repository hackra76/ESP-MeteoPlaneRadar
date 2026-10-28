# MeteoPlaneRadar v3.9.0 — Full-System Audit Remediation, Concurrency & Bezel Geometry Overhaul

**MeteoPlaneRadar v3.9.0** delivers a comprehensive stability, thread-safety, touch responsiveness, and circular UI geometry overhaul across 43 modules, resolving every P0 and P1 finding from our full-system hardware and software audit.

---

## 🌟 Highlights & Improvements

### 🛡️ Core Concurrency, Memory Safety & Scheduler Hardening
- **`SerialLog` Fallback Heap Buffer Protection**:
  - Tracked actual allocated ring buffer capacity (`s_bufCap`) when falling back from 64 KB PSRAM to 16 KB internal heap allocation, preventing out-of-bounds heap corruption.
- **Strict Cross-Core Synchronization (`portMAX_DELAY`)**:
  - Enforced `portMAX_DELAY` across all `Async_Lock*()` mutexes (`s_mtxAdsb`, `s_mtxRadar`, `s_mtxForecast`, `s_mtxFinance`, `s_mtxIss`, `s_mtxYouTube`, `s_mtxKlipper`, `s_mtxSettings`, `s_mtxRoute`, `s_mtxI2C`).
  - Protected shared `Forecast`, `AirQuality`, `Route`, `PlaneTrail`, `PlanePhoto`, `MapTiles`, `CHMU`, `SHMU`, `PetBrain`, and `DisasterAlert` state across Core 0 async network tasks and Core 1 UI rendering.
- **`AsyncCore` Background Scheduler Anti-Starvation**:
  - Removed `YouTube_Reset()` from the 10 ms idle loop on non-YouTube screens (which previously trapped background polling in an infinite Step 1 loop and starved `iss-bg` and `disaster` tasks).
  - Marked `s_ytData.valid = true` immediately upon completing Step 1 (`YT_STATE_CHANNEL`) so subscriber counts appear without delay.
  - Interleaved `MapTiles` bursts (capping consecutive tile downloads at 3 steps before yielding to the main telemetry ladder) and decoupled `lastMtStep` from `lastStep`.
  - Fixed `Forecast_Tick()` return semantics so `s_lastFcAutoTryMs` only resets when an actual HTTP fetch executes.
- **lwIP Socket Reclamation (`SO_LINGER`)**:
  - Applied `setsockopt(fd, SOL_SOCKET, SO_LINGER, ...)` *before* `http.end()` / `client.stop()` in `Net.cpp` so TCP `TIME_WAIT` sockets are immediately reclaimed, preventing socket pool exhaustion under heavy TLS polling.
  - Guarded the shared `s_txtBuf` scratch buffer with `TxtLockGuard` across `Net_SessionBegin()`, `Net_SessionEnd()`, `Net_GetBinary()`, and `Net_TouchDate()`.
- **Eliminated Cross-Core LVGL Invalidations**:
  - Removed direct `lv_obj_invalidate()` and buffer free calls from Core 0 worker threads (`PlanePhoto`, `MapTiles`, `CHMU`, `SHMU`), delegating UI invalidation safely to Core 1 ticks.

### ⚙️ Peripherals, Watchdogs & Bus Hardening
- **I2C & Expander Thread Safety**:
  - Added `Wire.setTimeOut(50)` to prevent indefinite I2C bus hangs.
  - Protected `TCA9554` output shadow register (`s_output`) and I2C transactions with `Async_LockI2C()`.
  - Added `portMUX_TYPE` spinlocks in `Buzzer` and `QMI8658`, and made `BEEP_CLICK` non-blocking (`PATTERN_CLICK`).
- **Active Hardware Watchdogs & Scheduled Maintenance**:
  - Wired `FlightStats_Tick()` (midnight rollover), `ScreenSettings_WantsWifiReset()`, `TCA9554_Verify()` (`EXPANDER_CHECK_MS`), and `DISPLAY_WD` (`LCD_VsyncCount()`) recovery into the main loop.
  - Added a 3-minute periodic STA reconnect check when operating in fallback AP mode (`WiFiPortal.cpp`).
  - Rate-limited HTTP `Date:` header RTC synchronization (`PCF85063_SetTime`) to once per 30 minutes (`Outside.cpp`).
  - Persisted active screen index (`"scr"`) in NVS (`Settings.cpp`).

### 🎯 Typography, Touch Debounce & Circular Bezel Geometry ($R=240\text{ px}$)
- **Restored Font Hierarchy**:
  - Mapped `FONT_SMALL` (10pt), `FONT_MEDIUM` (14pt), and `FONT_LARGE` (20pt) to distinct LVGL glyphs in `FontEngine.cpp`.
- **Touch Responsiveness & Click Suppression**:
  - Reduced `TOUCH_IDLE_POLL_MS` to 200 ms, added a 60 ms touch release debounce (`TOUCH_RELEASE_MS`) in `Touch_CST820.cpp`, and suppressed underlying LVGL `CLICKED` events when overlay or double-tap handlers consume a gesture (`UI.cpp`).
  - Retained `CHMU`/`SHMU` radar frames in PSRAM across screen switches unless free PSRAM drops below 1 MB.
- **Round Bezel Safe-Zone Fixes**:
  - **`QuickControl` & Asset Picker**: Moved the QuickControl card (`CW_Y=76, CW_H=264`) and Asset Picker (`PK_Y=70, PK_H=324`) inside the circular display safe chord and wired Row 4 (`Update Forecast`).
  - **Emergency Telemetry HUD**: Repositioned the Emergency Squawk HUD card (`boxY=330, boxW=290, boxH=62`) above the range pill inside $R=240\text{ px}$, localized vertical rate labels, and scope-limited + alternated the special flight sub-header banner.
  - **`ScreenSonar`, `ScreenWeather`, `ScreenIss`, `ScreenClock` & `ScreenInfo`**: Clipped upper PPI inner rings in Sonar split view, lazily reallocated `s_wfBuffer`, fixed Weather loading modal draw order, added a contrast pill behind `"CTR: ..."` on ISS, optimized Clock sparkline vertical fills, and improved label contrast on Info.
  - **`PetDrawer`**: Adjusted visible-edge entrance bounds (`95..385`), centered `VOL`/`MUTE` text, and lowered downward laser drag threshold to prevent unintended drawer dismissal.

### 🌐 Web Portal Security, OTA Stability & Anti-Slop UI Compliance
- **Authentication & API Hardening**:
  - Enforced `authed()` on mutating POST routes (`/api/screen`, `/api/range`, `/api/toggle-legends`, `/api/pet/toggle`, `/api/display/resync`).
  - Accepted both `pass`/`password`/`oldPass` and `newPass`/`newPassword` JSON keys in `WebConfig.cpp`, and added HTTP `401`/`403`/`429` status checks in `WebPage.html`.
- **Safe OTA Memory Management**:
  - Removed destructive PSRAM cache clearing from `GET /update` and added a 60 ms Core 1 park synchronization barrier before freeing OTA buffers.
- **Web UI Fixes**:
  - Fixed `--acc` CSS variable, Info screen button index (`goScreen(10)`), `klipperPort` auto-save and `body()` serialization, and semver patch comparison (`l3 > c3`).

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.9.0-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.9.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## 🚀 How to Flash / Update

### On-Device OTA Update
Navigate to **Settings** (`Screen 11`) on the device or open the **Web Portal** at `http://<device-ip>/` and click **Check for Updates** under the System card.

### Web Flash Tool
1. Connect the ESP32-S3 board via USB.
2. Open Chrome/Edge and visit [ESP Web Flasher](https://espressif.github.io/esptool-js/).
3. Load `MeteoPlaneRadar-v3.9.0-factory.bin` at offset `0x0000`.
4. Click **Program**.

### Command Line (`esptool.py`)
```bash
esptool.py --chip esp32s3 -p COMx -b 921600 write_flash 0x0000 MeteoPlaneRadar-v3.9.0-factory.bin
```

