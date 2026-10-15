# MeteoPlaneRadar v3.3.0 — Klipper & Moonraker 3D Printer Telemetry HUD

**MeteoPlaneRadar v3.3.0** introduces a high-tech, circular 3D printer telemetry screen inspired by KNOMI and modern cyberpunk HUDs, communicating with Moonraker over local Wi-Fi. Featuring custom hardware-drawn vector micro-icons, dynamic state detection across 16 operational states with illuminated LED indicators, dual glassmorphism thermal cards, real-time toolhead coordinates, and full web management.

---

## 🌟 Highlights & New Features

### 🖨️ Cyberpunk / KNOMI-Inspired Circular 3D Printer HUD
- **Dedicated Telemetry Screen (`SCREEN_KLIPPER_I`)**: Added as Screen 9 in the carousel rotation sequence. Communicates with Moonraker over local Wi-Fi with asynchronous JSON parsing.
- **Hardware-Rendered Vector Micro-Icons**:
  - *Isometric 3D Shaded Calibration Cube*: Shaded faces with neon cyan wireframe edges for print progress.
  - *Extruder Hotend Nozzle*: Angular brass nozzle with horizontal heater block.
  - *Heated Bed Plate*: Isometric build surface with rising thermal wave curves.
  - *Aviation Homing Crosshair*: Bullseye reticle with 4 cardinal hash ticks.
  - *Bed Mesh Grid*: 3D tilted calibration mesh grid.
  - *G-Code Document*: File sheet with folded corner and simulated G-code lines.
- **Dynamic State Badge & Status LED Indicator**: Real-time operational state detection with illuminated LED glow across 16 states:
  - `STANDBY`, `HOMING`, `PROBING / MESH`, `HEATING`, `HEAT SOAKING`, `PRINTING`, `PAUSED`, `FILAMENT RUNOUT`, `PURGING / CLEAN`, `COOLING DOWN`, `COMPLETED`, `CANCELLED`, `ERROR`, `MCU SHUTDOWN`, `STARTING UP`, and `DISCONNECTED`.
- **Intelligent Macro Lifecycle Gating**: Binds transient status messages (such as bed leveling, tilt adjustment, or meshing) to Moonraker's `idle_timeout` (`state: "Printing"` vs `"Ready"`/`"Idle"`) and `motion_report` (`live_velocity`), preventing displays from getting stuck on lingering LCD strings like `"Z tilt adjusting...."`.
- **Dual Glassmorphism Thermal Cards**: Side-by-side cards for Hotend and Heated Bed with neon accent strips, high-precision temperatures, and target pill badges (`OFF` / `TGT XX°C`).
- **Real-Time Toolhead Elevation & Telemetry**: Dynamic X/Y/Z homed status badges, Z-height elevation pill, and volumetric flow / filament usage chip.
- **Interactive Touch Refresh**: Tap anywhere on the Klipper screen to trigger an immediate live status fetch.

### 🌐 Web Interface Management
- **Dedicated 3D Printer Settings Card**: Configure custom printer display name, Moonraker host/IP address, port, and optional API key.
- **Automatic mDNS Hostname Resolution**: Supports standard `.local` hostnames (e.g. `voron.local`) alongside static IP addresses.
- **Direct Remote Navigation**: One-click button in the web panel to jump the physical device directly to the 3D printer screen (`POST /api/screen {"index": 9}`).

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.3.0-factory.bin` | ~4.8 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.3.0-ota.bin` | ~4.8 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.3.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
