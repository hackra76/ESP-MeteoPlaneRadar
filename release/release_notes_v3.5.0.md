# MeteoPlaneRadar v3.5.0 — Custom Screen Sequence & Klipper 3D Printer Overhaul

**MeteoPlaneRadar v3.5.0** introduces a comprehensive **Screen Sequence & Rotation Order Manager** built directly into the Tactile Field Instrument web portal, along with major fixes and telemetry enhancements for the **Klipper / Moonraker 3D Printer HUD**.

---

## 🌟 Highlights & New Features

### 📐 Custom Screen Ordering & Rotation Sequence Manager
- **Tactile Field Instrument Reordering Matrix**:
  - Embedded directly in Common Settings (`tCommon`) via `#scrOrderList`.
  - Supports full **HTML5 Drag & Drop** on desktops with real-time drop target indicator lines.
  - Features precision **Mechanical Step Buttons (`▲` / `▼`)** allowing seamless, single-tap reordering on smartphones and tablets.
  - Monospace `#01` to `#12` positional numbering and live status badges (`● AKTÍVNA` / `○ VYPNUTÁ`).
  - Direct jump navigation button (`Nastaviť` / `Config`) opening individual screen configuration tabs.
- **Non-Volatile Storage (NVS)**:
  - Custom sequence array `s_scrOrder[SCREEN_N]` is persisted in ESP32 flash memory (`"scrOrd"` key).
  - Preserves user sequence across power cycles, deep sleep, and reboots.
- **Unified Navigation Across Device & Web**:
  - **Touchscreen Swiping (`UI_SwitchScreenStep`)**: Left/Right edge swipe gestures traverse screens strictly in the user's customized sequence.
  - **Auto-Rotation Cycling (`UI_AutoRotate_Tick`)**: Screen timer advances through enabled screens in the custom order, automatically skipping disabled screens and the on-device Settings screen.
  - **Quick Screen Buttons (`#scrBtns`)**: Remote screen buttons and step navigation in the web portal dynamically mirror the custom sequence in real time.
- **Debounced Instant Auto-Save & Factory Reset**:
  - Reordering saves in `< 350ms` via `/api/config`.
  - One-click `↺ Predvolené poradie` / `↺ Default Order` button restores the factory sequence instantly.

### 🖨️ Klipper & Moonraker 3D Printer Screen Overhaul
- **Accurate Print ETA Calculation**:
  - Implemented Moonraker `print_duration` vs `total_duration` vs `estimated_time` and layer-time estimation fallbacks.
  - Eliminates the bug where remaining print time was stuck displaying `0s`.
- **Persistent Screen Activation**:
  - Fixed Moonraker active flag and screen enabled state persistence in NVS, ensuring the 3D printer screen remains enabled across reboots.
- **Current Layer / Total Layers Indicator**:
  - Extracted live layer count from Moonraker status (`print_stats.info.current_layer` / `total_layer`).
  - Rendered as a clean, high-contrast numerical readout: `L X/Y` (e.g. `L 142/350`) on the round LCD and in the web portal.
- **Enlarged Toolhead Telemetry & Homing Indicators**:
  - Increased font size and contrast for X, Y, Z homing status badges and coordinates.
  - Explicit distinction between current coordinate position and homing targets for quick glanceability on the round 480×480 screen.

### ⚡ Firmware PROGMEM & Memory Optimization
- Pre-compressed Gzip Level 9 payload (`WebPageGz.h`) packed to **65,831 bytes** (25.5% compression ratio, saving **187.8 KB** of flash storage).

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.5.0-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.5.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.5.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
