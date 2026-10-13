# MeteoPlaneRadar v3.2.0 — Disaster Warning System, Hardware Seismometer & Chronograph Refinement

**MeteoPlaneRadar v3.2.0** introduces a comprehensive Early Warning & Disaster Alert System featuring real-time USGS seismic feeds, NOAA tsunami alerts, an onboard hardware seismometer using the 6-axis IMU, configurable geofenced alert perimeters, full trilingual Web UI localization, and visual fixes for the Luxury Astronomical Chronograph.

---

## 🌟 Highlights & New Features

### 🌋 Disaster Warning & Earthquake Early Warning System
- **Real-Time Global Seismic Feeds**: Integrates USGS earthquake monitoring (`all_hour.geojson` and `significant_week.geojson`) with automatic epicenter distance and bearing calculation relative to your home station.
- **Hardware Seismometer (QMI8658 6-Axis IMU)**: High-precision real-time physical ground tremor and vibration detection with dynamic baseline filtering directly on the device.
- **Geofenced Alert Perimeter Filter**:
  - Configure alert radius: **Worldwide, 100 km, 250 km, 500 km, 1000 km, 2500 km, or 5000 km**.
  - Distant earthquakes beyond the perimeter are quietly suppressed.
  - Dynamically dismisses any active alert if the perimeter is updated to exclude the event's epicenter.
- **Circular HUD Emergency Banner**: High-visibility emergency overlay rendered across all screens with event magnitude, depth, location, distance from station, and elapsed time with tap-to-dismiss.
- **High-Decibel Emergency Siren**: Dual-tone emergency siren pattern (`BEEP_EMERGENCY`) via the onboard PWM buzzer.
- **Adaptive Test Alert Trigger**: One-click test button in the Web UI with intelligent distance scaling matching the selected perimeter.

### 🌐 Web UI & Multilingual Localization Overhaul
- **Dedicated Disaster Management Card**: Full control over minimum magnitude thresholds, range perimeter limit, hardware seismometer toggle, and instant alert testing.
- **Complete Trilingual Coverage**: 100% localization across English, Slovak, and Czech for all disaster settings, status pills, and buttons.
- **Optimized Mobile & Desktop UI**: Responsive layout with zero visual overflow and efficient gzip compression in flash memory.

### 🕒 Luxury Astronomical Chronograph Fixes
- **Continuous Seconds Dial**: Resolved ring discontinuities, gaps, and jumpiness around the outer chronograph bezel for a silky smooth 30 FPS second orbit.
- **Solar Arc Twilight Progression**: Eliminated duplicate sun rendering artifacts along the solar arc during daytime and twilight transitions.

### 🛡️ Network & Heap Hardening
- **Enhanced Heap Protection**: Raised network heap allocation thresholds (`NET_MIN_HEAP` 46,000 B, `NET_MIN_BLOCK` 17,500 B, `NET_TLS_BUF_BYTES` 17,200 B, `NET_TLS_STATE_BYTES` 12,000 B).
- **Dynamic mbedTLS Probe**: Pre-allocates buffer tests in `Net_HeapOk()` to guarantee zero memory crashes under heavy concurrent traffic (ADS-B + Weather Radar + USGS feeds).

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.2.0-factory.bin` | ~4.8 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.2.0-ota.bin` | ~4.8 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.2.0-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```
