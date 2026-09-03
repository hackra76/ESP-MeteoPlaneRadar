# MeteoPlaneRadar (Tactical Radar & Enhanced Watchfaces Edition)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-EN%20%7C%20SK%20%7C%20CZ-green.svg)
![Release](https://img.shields.io/badge/Release-v1.3.0-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunctional weather station, live ADS-B flight radar, animated precipitation radar, combined tactical radar, and designer clock faces on a round 2.1" IPS touchscreen.**  
Designed specifically for the **Waveshare ESP32-S3-Touch-LCD-2.1** development board with modern smartphone-like touch gestures, a pull-down Control Center, and a responsive web dashboard for real-time remote control and full configuration.

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

## 🌟 What's New in v1.3.0

- 📱 **Fluid Smartphone-Style Gestures:** Horizontal swipes between screens, vertical swipes for zooming, tap-to-zoom on the range bar, and pull-down from the top edge.
- 🎛️ **Quick Control Center Overlay:** Instant brightness slider (`-` / `+` 15%), night mode switch, and screen-specific toggles (airports, range rings, trails, weather sources, and auto-rotation).
- 🎬 **Smooth Hardware Slide Transitions:** Zero-flicker carousel-style screen transitions rendered in ~100 ms using PSRAM double framebuffers.
- 🚁 **Intelligent Flight Watchlist & Alert HUD:** Automatic recognition and color-coded halo markers for rescue helicopters (HEMS / ATE / Kryštof / SAR), government specials (SSG / CEF / VIP), heavy giants (Airbus A380, Boeing 747, Antonov, Beluga), and military sorties. Real-time alert banner at the top of the radar.
- ✈️ **Human-Readable Aircraft Models & Color Telemetry Card:** ICAO types translated into clear names (e.g. `Airbus A320`, `Boeing 737 MAX 8`, `ATR 72-600`) with unified color-coded altitude, speed, climb/descent rates, and routes.
- 🌤️ **3-Hour Mini-Forecast Pills on Clock Face:** Hourly forecast capsules (`+1h`, `+2h`, `+3h`) with WMO weather condition icons and temperatures.
- 🔄 **IMU Gravity Auto-Orientation:** Uses the onboard 6-axis accelerometer to automatically rotate the radar heading to match device orientation in a stand or on a desk.

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
- `Off` (clean bezel)
- `Dot` (orbiting pip)
- `Smooth Arc` (filling neon ring)
- `Pulse` (breathing halo)
- `Radar Sweep` (rotating primary radar beam with smooth phosphor decay trail)
- `Swiss Ticks` (60 perimeter chronometer indices)
- `Orbital Satellite` (solar-winged satellite tracking the bezel)

### 🛩️ 3. Advanced Aircraft Tracking & Intelligent Alert HUD
- **Special Flight Recognition:** Rescue helicopters (green), VIP/Government flights (gold), Iconic aircraft (cyan), and Military sorties (red) are automatically detected and highlighted with glowing target rings.
- **Top Alert Banner:** Displays live alerts for special flights within range (e.g. `! Rescue Helicopter: ATE02 (18 km) !`).
- **Emergency Squawk Auto-Focus (7500, 7600, 7700):** If an aircraft transmits an emergency transponder code, all other flights are dimmed, the map smoothly locks onto and follows the aircraft, and a live telemetry banner displays altitude, ground speed, and vertical rate (climb/descent).
- **Proximity Vector:** Real-time vector line pointing directly to the nearest aircraft with distance, bearing, and altitude delta.
- **Airports & Route Database:** Nearby airports plotted on the radar with offline callsign route decoding (e.g., `Burgas -> Warsaw [BOJ>WAW]`).

### 🛰️ 4. Combined Tactical Radar (`ScreenTactical`)
A unique real-time screen overlaying **animated precipitation radar tiles in the background** with **live ADS-B aircraft traffic in the foreground**. Aviation enthusiasts can observe pilots navigating around storm cells and convective turbulence live.

### 🕒 5. Hardware RTC (PCF85063) & Power Independence
- Automatic boot and network NTP synchronization with the onboard PCF85063 real-time clock.
- Keeps precise time during power outages or network disconnects.
- Supports soldering a **3.3V 1.0F–1.5F supercapacitor** to the `BAT` and `GND` pads for battery-free time preservation for weeks.
- Web diagnostics displaying RTC status, live hardware time, and the Oscillator Stop Flag (`OSF`).
- Web buttons for manual one-click sync from NTP or the browser's local clock.

### ⚡ 6. Robust Dual-Core FreeRTOS Architecture
- **Core 1:** Dedicated to high-speed ST7701 RGB rendering (double-framebuffer, zero flicker), CST820 capacitive touch pumping, and IMU gesture processing.
- **Core 0 (`AsyncNetWorker`):** Non-blocking background worker handling mbedTLS handshakes, RainViewer tile caching, ADS-B JSON parsing, and HTTP web serving.
- **Non-blocking Mutexes:** All shared models and the I2C bus are protected with timeout-bounded recursive FreeRTOS mutexes (`Async_LockI2C`), preventing multi-core contention and task watchdog timeouts.

---

## 📱 Screen Overview

| Screen | Description | Data Source |
| :--- | :--- | :--- |
| **1. Clock** | 7 selectable watchfaces, 3-hour forecast pills, 7 seconds styles, weather, wind, moon phase, and 24h solar arc | Open-Meteo & Astro Engine |
| **2. Planes** | Live aircraft traffic radar, routes, airports, distance rings, emergency & watchlist tracking | adsb.fi / adsb.lol |
| **3. Weather Radar** | 6-frame animated precipitation radar map with playback control | ČHMÚ (CZ/SK) or RainViewer (Global) |
| **4. Tactical Radar** | **Combined view:** Live precipitation radar + ADS-B aircraft overlay on a single screen | RainViewer / ČHMÚ + adsb.fi |
| **5. Forecast** | 6-hour meteogram, 3-day daily outlook, Air Quality (AQI, PM2.5), and pollen forecast | Open-Meteo Weather & Air Quality |
| **6. Settings** | Device telemetry, IP address, brightness control, map orientation, language selector | System |

---

## 🖐️ Gesture & Touch Controls

| Gesture / Action | Action |
| :--- | :--- |
| **Swipe Left / Right** | Transitions smoothly to the next / previous screen with slide animation. |
| **Pull Down from Top Edge** | Opens the **Quick Control Center** (brightness, night mode, screen toggles). |
| **Swipe Up / Down in Center** | **On Radars:** Zoom In (swipe up) / Zoom Out (swipe down).<br>**On Control Center:** Closes the overlay. |
| **Tap Bottom Range Bar** | Left half zooms Out, right half zooms In. |
| **Tap on Aircraft** | Opens full color-coded aircraft telemetry detail card. |
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
- **Granular Widget Toggles:** Independently enable/disable date, weather, wind, moon, flight trails, nearest plane vector, airports, and range rings.
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
- Enhancements, Slovak localization, tactical combined radar, expanded watchface collection, RTC driver, IMU gestures, touch navigation, Quick Control Center, flight watchlist, and multi-core stabilization: **Rado & Antigravity AI**.
