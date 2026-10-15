# MeteoPlaneRadar v3.4.0 — Tactile Field Instrument Industrial UI & Anti-Slop Web Portal

**MeteoPlaneRadar v3.4.0** delivers a comprehensive visual and architectural overhaul of the embedded web interface, transforming it from generic SaaS dashboard aesthetics into an authentic, precision **Tactile Field Instrument** inspired by Braun, Teenage Engineering, and classic Swiss Architectural Grids.

---

## 🌟 Highlights & New Features

### 🎛️ Tactile Field Instrument & Swiss Grid Architecture
- **Anti-Slop Ban List Enforcement**:
  - Eliminated all default AI tropes: slate/zinc blue dark mode backgrounds (`#0f172a`, `#1e293b`), purple/cyan neon gradients, radial glow blurs, glassmorphism (`backdrop-filter: blur`), floating soft cards (`border-radius: 8px–20px`), generic pill badges (`border-radius: 9999px`), and generic sans data fonts.
- **Matte Industrial Palette**:
  - Surfaces grounded in deep matte carbon (`#121211`), milled chassis plate (`#171716`), and recessed instrument bays (`#0B0B0A`).
  - Structural division via visible 1px hairlines (`#2A2A28`).
  - Primary ink in high-contrast off-white bone (`#F4F3EF`) with secondary technical annotations in `#B2B1AC` and `#6A6965`.
  - Strictly rationed functional accents: International Signal Orange (`#FF4F00`) for warning/active actuators, Lab Ochre (`#E5A93C`) for standby channels, and Phosphor Green (`#00D062`) for live radar sweeps and carrier detect.
- **Exposed Architectural Swiss Grid**:
  - Zero-gap structural topology (`gap: 0; 1px solid #2A2A28` dividers).
  - Technical corner crosshair glyphs (`+`) positioned at intersecting grid coordinates.
  - Asymmetric layout: Dense fixed-width technical sidebar (`380px`) paired with a high-density telemetry canvas.
- **Extreme Typographic Hierarchy**:
  - Technical grotesque headers (`Space Grotesk`) paired with strict tabular monospace (`JetBrains Mono`, `tabular-nums`) for all numbers, coordinates, altitudes, voltages, and logs.
  - Dramatic scale contrast: Micro `10px` uppercase tracked labels (`letter-spacing: 0.12em`) paired with raw oversized `42px` hero readouts.
- **Bespoke Tactile Controls & Inline SVGs**:
  - Rotary-style SVG circular arc gauges with calibrated tick marks for core die temperature and PSRAM utilization.
  - Segmented mechanical rocker switches with recessed channels and Signal Orange tactile indicators.
  - Phosphor LED matrix status diodes for RF 1090MHz carrier, meteo sync, ISS tracking, and network link.
  - Integrated standalone field instrument simulator (`portal.html`) featuring an animated 480×480 CRT radar scope with sweeping beam, persistent phosphor decay, and aircraft telemetry blips.

### ⚡ Firmware PROGMEM & Memory Optimization
- Pre-compressed Gzip Level 9 payload (`WebPageGz.h`): Packed down to **62,776 bytes** (25.4% compression ratio, saving **184.8 KB** of flash storage).
- Clean separation between embedded API hooks and clientside fallback simulation.

### 📜 Permanent Anti-Slop Rule Codification
- Permanent workspace rule codified in `.agents/rules/anti-slop-industrial-ui.md` and repository guidelines in `GEMINI.md`.
- Knowledge graph entity `GlobalDesignRule_AntiSlopIndustrialUI` persisted in local memory to enforce this standard across all future UI workflows.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.4.0-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.4.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.4.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
