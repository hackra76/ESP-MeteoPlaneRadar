# MeteoPlaneRadar v3.6.0 — Luxury Dial Complications, Astronomical Moon Lighting & Network Isolation

**MeteoPlaneRadar v3.6.0** introduces fully configurable **Dual Circular Complications** on Screen 0 (Luxury Astronomical Chronograph), physical **Astrophysical Moon Phase Lighting Corrections**, **Automatic Subdial Telemetry Supervision**, and strict **Klipper Network Isolation** for offline 3D printers.

---

## 🌟 Highlights & New Features

### ⏱️ Configurable Dual Circular Dial Complications (Screen 0)
- **Top and Bottom Subdial Apertures**:
  - Both circular apertures (Top: $r=48$ at $y=96$; Bottom: $r=56$ at $y=356$) can now be independently assigned any of the 7 supported complication modules.
- **7 Interchangeable Complication Modules**:
  - `0`: **3D Photorealistic Moon Phase** — NASA textured lunar sphere with dynamic lighting.
  - `1`: **Temperature Trend & Sparkline** — High-contrast 24h temperature curve with min/max bounds.
  - `2`: **Current Weather & Condition Icon** — Real-time temperature (°C) and wind speed (km/h) with weather glyph.
  - `3`: **Aircraft in 10 km Radius** — ADS-B proximity count with aviation cyan jet silhouette.
  - `4`: **YouTube Subscribers Count** — Branded badge with formatted subscriber total (`SUBS`).
  - `5`: **Primary Market Ticker** — Real-time price and percentage change for your configured financial symbol.
  - `6`: **Klipper 3D Printer Progress** — Real-time print completion percentage, extruder/bed temperatures, and layer progress.
- **Dynamic Luxury Subdial Dial Rendering**:
  - High-resolution PSRAM background buffer (`s_luxuryBg`) dynamically renders either the 3D spherical lunar aperture or an engraved mechanical subdial floor with radial ambient occlusion and 12 calibrated gold tick marks.
- **Direct Touch Navigation**:
  - Tapping either complication aperture directly jumps to its full telemetry screen (Forecast, Planes, YouTube, Finance, or Klipper).
- **Quick Control Center & Web Configuration**:
  - Added dedicated Top and Bottom complication cycler rocker buttons in the on-screen Quick Control panel with buzzer feedback.
  - Added "Dial Complications Configuration" card in the Web Portal with full i18n (SK, CS, EN) and instant NVS persistence (`clkTopS`, `clkBotS`).

### 🌕 Astrophysical Moon Phase Lighting Correction
- **Waxing Illumination Orientation**:
  - Corrected light vector normal calculation in `ScreenClock.cpp` from `float lx = -sinf(phaseRad);` to `float lx = sinf(phaseRad);`.
  - In sphere coordinate space ($+x$ is right), waxing crescent and quarter phases ($0 < \text{phase} < 0.5$) now correctly illuminate the **right** hemisphere, matching northern hemisphere physical observation.

### 🔄 Automatic Complication Telemetry Supervision
- **Zero-Touch Live Telemetry**:
  - Subdial complications now automatically fetch and refresh background data (YouTube statistics, stock/crypto prices, ADS-B proximity, and weather) directly while dwelling on the clock screen without requiring manual rotation through the carousel.
  - Integrated a 4-second supervisor in `ScreenClock_TimerCb` to maintain fresh telemetry across all active complications.

### 🖨️ Klipper 3D Printer Network Isolation & Fast LAN Timeouts
- **Strict On-Screen Display Restriction**:
  - Klipper / Moonraker network queries are now strictly confined to when the Klipper screen is actively displayed (`curScr == SCREEN_KLIPPER_I`).
  - Completely eliminated background polling when on the Clock screen or any other screen, preventing socket exhaustion, network stalls, and serial error spam when the printer is powered off.
- **Fast Local LAN Timeouts**:
  - Extended `Net_GetString` with configurable connect and request timeouts. Klipper uses 1500 ms connect and 2500 ms request timeouts on LAN instead of blocking the network thread for 8000 ms.
- **Offline Polling Backoff**:
  - When actively viewing the Klipper screen while the printer is off, polling automatically backs off to 15 seconds (`KLIPPER_PERIOD_OFFLINE_MS`) instead of 1.5 seconds, while retaining instant refresh on tap.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.6.0-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.6.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.6.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
