# MeteoPlaneRadar v3.1.0 — Global Vector Borders, Solar Engine & Stability Overhaul

**MeteoPlaneRadar v3.1.0** delivers worldwide vector mapping with sub-millisecond clipping, an offline mathematical astronomical solar engine for real-time day/night brightness control, rock-solid weather radar zoom stability, and key user interface refinements across avionics and companion screens.

---

## 🌟 Highlights & New Features

### 🗺️ Natural Earth Worldwide Vector Borders
- **Global Offline Mapping**: Replaced the legacy regional European vector dataset with a high-performance global vector dataset (Natural Earth 110m clipped coastline and sovereign boundaries).
- **Universal Home Location Support**: Border vectors now render accurately anywhere in the world, matching the user's custom geographic coordinates without hardware degradation or memory strain.
- **Cross-Screen Integration**: Enabled across all radar display modes:
  - **Glass Cockpit Planes Radar** (Screen 1)
  - **Precipitation Weather Radar** (Screen 2)
  - **Combined Tactical Airspace Radar** (Screen 3)
  - **Cold War Tactical Sonar** (Screen 4)
- **Sub-1ms Clipping**: Highly optimized coordinate-space line clipper ensures instantaneous rendering without dropping frames.

### ☀️ Astronomical Solar Engine & Dynamic Brightness Control
- **NOAA Solar Algorithm**: Real-time mathematical calculation of astronomical sunrise and sunset directly from station latitude/longitude and UTC epoch time. Operates completely offline without requiring internet access or waiting for Open-Meteo API responses.
- **Automatic Day/Night Transitions**: Dynamic brightness control seamlessly transitions between configured Day and Night brightness levels.
- **Ultra Night Mode with Physical Wake**:
  - Deep-red sleep monochrome watchface at 2% backlight brightness to preserve dark adaptation.
  - Temporary 15-second brightness restoration upon touchscreen tap (`UI_NotifyInteraction`) or physical double-tap on the board (`QMI8658_OnDoubleTap`).
- **Night Screen Auto-Lock**: Enforces `nightClockOnly` and `ultraNight` to automatically lock display auto-rotation to Screen 0 (Clock) during nighttime hours.

### 🌧️ Meteoradar Zoom & PSRAM Memory Stability
- **Zero-Crash Zoom Switching**: Fixed display driver crashes and PSRAM tearing during meteoradar zoom level switching (25 km to 200 km).
- **Optimized Memory Lifecycle**: Removed unstable full-country composite buffers, stabilized asynchronous frame queues, and eliminated DMA bus saturation.

### 🐾 DigiCat Companion & Avionics Polish
- **Aircraft Chase Badge Fix**: Resolved corrupt glyph/box rendered between callsign and distance on the active DigiCat aircraft chasing overlay.
- **ISS Orbit Tracker**: Polished hint text and status indicators on Screen 7.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.1.0-factory.bin` | ~4.8 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.1.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.1.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
