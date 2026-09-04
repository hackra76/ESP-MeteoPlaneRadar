# MeteoPlaneRadar (Tactical & Precipitation Weather Radar for ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-EN%20%7C%20SK%20%7C%20CZ-green.svg)
![Release](https://img.shields.io/badge/Release-v1.5.7-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunctional weather station, live ADS-B flight radar, animated precipitation radar (SHMÚ, ČHMÚ, RainViewer), combined tactical radar, and designer clock faces on a round 2.1" IPS touchscreen.**  
Designed specifically for the **Waveshare ESP32-S3-Touch-LCD-2.1** development board with modern smartphone-like touch gestures, a pull-down Control Center, live aircraft photos, bilinear radar smoothing, and a responsive web dashboard for remote control and complete configuration.

> 🇸🇰 Slovenská dokumentácia: **[README_SK.md](README_SK.md)**  
> 📌 Forked and significantly enhanced from the original project **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)** by **[chiptron.cz](https://chiptron.cz)**.

---

## 📸 Live Device Demo

<p align="center">
  <img src="docs/media/tactical_radar_live.gif" width="31%" alt="Tactical Radar (Planes + Rain)" />
  <img src="docs/media/weather_radar_chmu.gif" width="31%" alt="Animated Rain Radar loop" />
  <img src="docs/media/clock_stacked_bold.gif" width="31%" alt="Stacked Bold Watch Face" />
</p>

<p align="center">
  <em>From left to right: <b>Tactical Radar</b> (live aircraft tracking over precipitation map), <b>Weather Radar</b> (smooth radar loop), <b>Stacked Bold Watch Face</b>.</em>
</p>

---

## 🌟 Key Highlights in v1.5.7

- 🚀 **Direct Online GitHub OTA Updates:** The web dashboard (*System* tab) now features a dedicated card to automatically check for new releases on GitHub (`hackra76/ESP-MeteoPlaneRadar`). When available, the user can review release notes and trigger a one-click OTA upgrade. The ESP32 streams the `-ota.bin` image securely over HTTPS, flashes the memory with live progress %, and reboots.
- 🖼️ **Fullscreen Aircraft Photo Mode:** Tapping an aircraft photo in the detail view smoothly expands it to fill the entire round 480×480 screen while preserving aspect ratio.
- 🇸🇰 **Native SHMÚ Radar Support (Slovakia):** Direct integration of high-resolution CMAX radar composites from the Slovak Hydrometeorological Institute (SHMÚ) with authentic national reflectivity scales.
- 🇨🇿 **ČHMÚ Radar (Czechia) & 🌍 Global RainViewer:** Instant live switching between national radar providers (SHMÚ, ČHMÚ) and worldwide RainViewer directly via Quick Control, Settings, or Web dashboard.
- 🎨 **Bilinear Radar Anti-Aliasing (Smoothing):** Eliminates blocky pixelation on high zooms (25 km and 50 km). Renders silky smooth precipitation gradients and storm cells. Easily toggled on/off in Settings, QuickControl, or Web UI.
- 📐 **Polished Round Display Layout:** Centered forecast display, 2×2 button layout on the Settings screen, and streamlined web interface for minimal CPU and RAM overhead.

---

## 🌟 Key Features & Innovations

### 🕒 1. Rich Collection of 7 Unique Clock Faces
The round 480×480 display features **7 completely distinct geometry styles** with instant switching via the web dashboard, Control Center, or double-tap on the chassis:
1. **Classic Digital** – Clean horizontal `HH:MM` layout with u8g2 font, date, centered weather condition icon, 3-hour forecast pills, wind, and moon phase.
2. **Aviator Cockpit Analog** – Authentic pilot dial with luminous tapered hands, hour chapter ring, and two sub-dials (weather at 9 o'clock, moon phase at 3 o'clock).
3. **🚀 Orbital Gauges** – Futuristic sci-fi face with three concentric circular arcs (minutes, hours, seconds) with glowing tip markers and a central telemetry hub.
4. **🛩️ Fighter HUD (Head-Up Display)** – Cockpit head-up display featuring an artificial horizon (pitch ladder -10°/0°/+10°), boresight reticle, target lock-on box (`SYS·TGT·LOCK`), top heading tape `HDG 360`, and vertical side tapes for temperature and wind.
5. **⏱️ Observatory Régulateur Chrono** – High-horology regulator chronometer with decoupled axes: full-diameter master minute hand, separate upper hour sub-dial (at 12:00), separate lower second sub-dial (at 6:00), and side complications.
6. **🔲 Stacked Bold Typography** – Contemporary smartwatch aesthetic (Pixel / Nothing style) with giant stacked hours `HH` and minutes `MM` flanked by rounded capsule badges for weather, moon, date, and wind.
7. **Nordic Minimal** – High-contrast, clean minimalist time easily readable across the room.

### ⏱️ 2. Outer Seconds Ring Customization
**7 distinct perimeter seconds styles**:
- `Off` (clean bezel), `Dot` (orbiting pip), `Smooth Arc` (filling neon ring), `Pulse` (breathing halo), `Radar Sweep` (rotating radar beam), `Swiss Ticks` (60 indices), `Orbital Satellite` (satellite tracking the bezel).

### 🛩️ 3. Advanced Aircraft Tracking & Intelligent Alert HUD
- **Special Flight Recognition:** Rescue helicopters (green), VIP/Government flights (gold), Iconic heavy aircraft (cyan), and Military sorties (red) are automatically detected and highlighted with glowing target rings.
- **Top Alert Banner:** Displays live alerts for special flights within range (e.g. `! Rescue Helicopter: ATE02 (18 km) !`).
- **Emergency Squawk Auto-Focus (7500, 7600, 7700):** If an aircraft transmits an emergency code, all other flights are dimmed, the map smoothly locks onto and follows the aircraft, and a live telemetry banner displays altitude, ground speed, and vertical rate.
- **Proximity Vector:** Real-time vector line pointing directly to the nearest aircraft with distance, bearing, and altitude delta.
- **Airports & Route Database:** Nearby airports plotted on the radar with offline callsign route decoding (e.g., `Burgas -> Warsaw [BOJ>WAW]`).

### 🛰️ 4. Combined Tactical Radar (`ScreenTactical`)
A unique real-time screen overlaying **animated precipitation radar tiles (SHMÚ / ČHMÚ / RainViewer) in the background** with **live ADS-B aircraft traffic in the foreground**.

### 🕒 5. Hardware RTC (PCF85063) & Power Independence
- Automatic boot and network NTP synchronization with the onboard PCF85063 real-time clock.
- Keeps precise time during power outages or network disconnects.
- Supports soldering a **3.3V 1.0F–1.5F supercapacitor** to the `BAT` and `GND` pads for battery-free time preservation for weeks.
- Web buttons for manual one-click sync from NTP or the browser's local clock.

### ⚡ 6. Robust Dual-Core FreeRTOS Architecture
- **Core 1:** Dedicated to high-speed ST7701 RGB rendering (double-framebuffer, zero flicker), CST820 capacitive touch pumping, and IMU gesture processing.
- **Core 0 (`AsyncNetWorker`):** Non-blocking background worker handling mbedTLS handshakes, radar tile caching, ADS-B JSON parsing, and HTTP web serving.

---

## 📱 Screen Overview

| Screen | Description | Data Source |
| :--- | :--- | :--- |
| **1. Clock** | 7 selectable watchfaces, 3-hour forecast pills, 7 seconds styles, weather, wind, moon phase, and solar arc | Open-Meteo & Astro Engine |
| **2. Planes** | Live aircraft traffic radar, aircraft photos, routes, airports, distance rings, emergency & watchlist tracking | adsb.fi / adsb.lol / Planespotters |
| **3. Weather Radar** | Animated precipitation radar map with smoothing and provider selection | SHMÚ (SK), ČHMÚ (CZ), RainViewer (Global) |
| **4. Tactical Radar** | **Combined view:** Live precipitation radar + ADS-B aircraft overlay on a single screen | SHMÚ / ČHMÚ / RainViewer + adsb.fi |
| **5. Forecast** | 5-hour detail, 3-day daily outlook, Air Quality (AQI), and weather icons | Open-Meteo Weather & Air Quality |
| **6. Settings** | Device telemetry, IP address, brightness control, map orientation, radar smoothing, language selector | System |

---

## 🖐️ Gesture & Touch Controls

| Gesture / Action | Action |
| :--- | :--- |
| **Swipe Left / Right** | Transitions smoothly to the next / previous screen with slide animation. |
| **Pull Down from Top Edge** | Opens the **Quick Control Center** (brightness, night mode, screen toggles). |
| **Swipe Up / Down in Center** | **On Radars:** Zoom In (swipe up) / Zoom Out (swipe down).<br>**On Control Center:** Closes the overlay. |
| **Tap Bottom Range Bar** | Left half zooms Out, right half zooms In. |
| **Tap on Aircraft** | Opens full color-coded aircraft telemetry detail card with live photo. |
| **Tap on Aircraft Photo** | Enlarge aircraft photo to full screen (tap anywhere to return to card). |
| **Double-Tap (Knock on chassis / desk)** | **On Clock:** Cycles to next watchface.<br>**On Radars:** Toggles Clean Map Mode (hides legends). |
| **Hold BOOT button at startup (~3 s)** | Factory Reset (clears stored Wi-Fi and NVS settings). |

---

## 🔧 Hardware Specifications

Built specifically for the **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Component | Specification |
| :--- | :--- |
| **MCU** | Espressif ESP32-S3R8 (Xtensa® Dual-Core 32-bit LX7 @ 240 MHz) |
| **Memory** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Display** | Round 2.1" IPS, 480×480 px, 65k RGB565 colors, ST7701 RGB interface |
| **Touch** | CST820 / CHSC6540 Capacitive Touch Controller (I2C) |
| **I/O Expander** | TCA9554PWR (Controls display power, backlight & reset) |
| **IMU Sensor** | QMI8658 6-axis Accelerometer & Gyroscope (tap & orientation detection) |
| **RTC Chip** | PCF85063 Real-Time Clock with low-power battery backup (I2C `0x51`) |
| **Connectivity** | USB-C (power + native CDC serial), Wi-Fi 802.11 b/g/n (2.4 GHz) |

---

## 🚀 Installation & First Boot

### 1. Build and Flash via PlatformIO
1. Open the project folder in **Visual Studio Code** with the **PlatformIO IDE** extension.
2. Connect the board via USB-C.
3. Run the following commands:
   ```bash
   # Build firmware
   pio run

   # Upload to device
   pio run -t upload
   ```

### 2. Wi-Fi Configuration (Captive Portal)
1. On first boot, the device creates an open Wi-Fi network named **`MeteoPlaneRadar`** and displays a QR code.
2. Scan the QR code or connect your phone/laptop to the `MeteoPlaneRadar` Wi-Fi.
3. Navigate to **`http://192.168.4.1/`**.
4. Select your home network, enter the Wi-Fi password, and click Save.
5. The device connects to your Wi-Fi and displays its assigned local IP address.

### 3. Web Dashboard
Once connected, open the dashboard in your web browser:
- **`http://meteoplaneradar.local/`** (or via its IP address, e.g., `http://192.168.0.7/`).

The web interface features:
- **Location:** Automatic GeoIP detection or custom city/GPS coordinates.
- **Clock Styles & Aesthetics:** Choose watchface, seconds ring, and primary colors.
- **Radar Settings:** Select radar source (SHMÚ / ČHMÚ / RainViewer), toggle bilinear smoothing, airports, and flight trails.
- **Night Clock Only Mode (`nightClockOnly`):** Stops screen auto-rotation during the night and locks the display dimmed onto the clock.
- **Hardware Diagnostics:** Live I2C bus device inspection, RTC status, and manual sync buttons.
- **Remote Control:** Switch screens and change radar range remotely from your browser.
- **OTA Updates:** Upload new `.bin` firmware builds wirelessly without plugging into USB.

---

## 🌐 REST API for Smart Home Automation

Integrate easily with **Home Assistant**, **Node-RED**, or terminal scripts via JSON REST endpoints:

- `GET /api/status` – Full JSON status (weather, aircraft counts, free heap, uptime).
- `GET /api/hardware` – Hardware peripherals, RTC clock, I2C bus scan, and last reset reason.
- `POST /api/screen` – Change screen: `{"screen": 0}` (0: Clock, 1: Planes, 2: Weather, 3: Tactical, 4: Forecast, 5: Settings).
- `POST /api/toggle-legends` – Trigger a clean mode toggle.
- `POST /api/rtc/sync_ntp` – Trigger immediate RTC synchronization with NTP.

---

## 📜 License & Credits

Distributed under the **MIT License**.
- Original base project: **Petr / [chiptron.cz](https://chiptron.cz)** ([petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)).
- Enhancements, Slovak localization, SHMÚ radar integration, bilinear anti-aliasing, Planespotters aircraft photos, tactical combined radar, expanded watchfaces, RTC driver, IMU gestures, touch navigation, Quick Control Center, flight watchlist, and multi-core stabilization: **Rado & Antigravity AI**.
