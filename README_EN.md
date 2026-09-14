# MeteoPlaneRadar (Tactical & Precipitation Weather Radar for ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-EN%20%7C%20SK%20%7C%20CZ-green.svg)
![Release](https://img.shields.io/badge/Release-v1.9.2-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunctional weather station, live ADS-B flight radar, animated precipitation radar (SHMÚ, ČHMÚ, RainViewer), combined tactical radar, ISS orbit tracking, and designer clock faces on a round 2.1" IPS touchscreen.**  
Designed specifically for the **Waveshare ESP32-S3-Touch-LCD-2.1** development board with modern smartphone-like touch gestures, a pull-down Control Center, live aircraft photos, bilinear radar smoothing, and a responsive web dashboard for remote control and complete configuration.

> 🇸🇰 Slovenská dokumentácia: **[README_SK.md](README_SK.md)**  
> 📌 Forked and significantly enhanced from the original project **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.

---

## 📸 Live Device Demo

<p align="center">
  <img src="docs/media/tactical_radar_live.gif" width="16%" alt="Tactical Radar (Planes + Rain)" />
  <img src="docs/media/screen_iss_live.png" width="16%" alt="ISS Orbit Tracker with Day/Night Map" />
  <img src="docs/media/finance_screen.png" width="16%" alt="Markets & Crypto Tracker" />
  <img src="docs/media/plane_detail_photo.png" width="16%" alt="Aircraft Detail with Live Photo" />
  <img src="docs/media/weather_radar_chmu.gif" width="16%" alt="Animated Rain Radar loop" />
  <img src="docs/media/clock_stacked_bold.png" width="16%" alt="Stacked Bold Watch Face" />
</p>

<p align="center">
  <em>From left to right: <b>Tactical Radar</b>, <b>ISS Orbit Tracker</b> (day/night terminator, past/future orbits, footprint ring), <b>Markets & Crypto</b> (custom tickers & sparkline), <b>Aircraft Detail</b>, <b>Weather Radar</b>, <b>Stacked Bold Watch Face</b>.</em>
</p>


---

## 🌟 Implemented Features

### 🕒 1. Rich Collection of 7 Unique Clock Faces
The round 480×480 display features **7 completely distinct geometry styles** with smooth vertical swipe switching:
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
- **360° Airspace Surveillance:** Tracks live flight traffic within a 25–300 km radius via adsb.fi / adsb.lol.
- **Special Flight Recognition:** Rescue helicopters (green), VIP/Government flights (gold), Iconic heavy aircraft (cyan), and Military sorties (red) are automatically detected and highlighted with glowing target rings.
- **Top Alert Banner:** Displays live alerts for special flights within range (e.g. `! Rescue Helicopter: ATE02 (18 km) !`).
- **Emergency Squawk Auto-Focus (7500, 7600, 7700):** If an aircraft transmits an emergency code, all other flights are dimmed, the map smoothly locks onto and follows the aircraft, and a live telemetry banner displays altitude, ground speed, and vertical rate.
- **Proximity Vector:** Real-time vector line pointing directly to the nearest aircraft with distance, bearing, and altitude delta.
- **Airports & Route Database:** Nearby airports plotted on the radar with offline callsign route decoding (e.g., `Burgas -> Warsaw [BOJ>WAW]`).

### 🔍 4. Aircraft Detail Card with Live High-Res Photos
- Tap any aircraft icon on the radar to open a detailed color-coded telemetry card.
- Live aircraft photography fetched on-demand from the **Planespotters.net API**.
- Tap the photo to view it full-screen on the round 480×480 display.

### 🌧️ 5. Animated Precipitation Radar (SHMÚ, ČHMÚ, RainViewer)
- High-resolution precipitation composite loops with temporal cross-dissolve between radar frames.
- 256 kB external PSRAM buffers per composite ensuring uninterrupted display even during severe widespread storm fronts.
- Optional bilinear anti-aliasing / smoothing filter for smooth, organic radar contours.

### ⚡ 6. Approaching Precipitation Nowcasting (TREC) & Precipitation Typing
- **2D Vector Nowcasting:** Real-time 2D spatial cross-correlation tracking precipitation velocity and trajectory. Alerts strictly when rain or storms are heading towards your location ($v_{radial} > 0$, miss distance $\le 15\text{ km}$, $\text{ETA} \le 60\text{ min}$) to eliminate false alarms.
- **Dynamic Precipitation Typing:** Automatically classifies incoming precipitation into **Rain**, **Sleet** ($1^\circ\text{C}\dots3^\circ\text{C}$), **Snow** ($\le 1^\circ\text{C}$), or severe **Hail** ($>50\text{ dBZ}$) using radar reflectivity and local temperature telemetry.
- **Clock Face Alert Widget:** Displays an alert pill on the clock screen with tap-to-radar navigation.

### 🛰️ 7. Combined Tactical Radar (`ScreenTactical`)
- Simultaneous real-time overlay of **animated precipitation radar tiles in the background** with **live ADS-B aircraft traffic in the foreground** on a single unified tactical screen.

### 🌤️ 8. 3-Day Weather Forecast & Air Quality
- Hourly temperature, precipitation probability, and wind speed curves powered by Open-Meteo.
- 3-day weather overview, Air Quality Index (AQI), PM2.5, and European pollen count.

### 📈 9. Financial Markets & Crypto Tracker (`ScreenFinance`)
- Real-time financial telemetry powered by Yahoo Finance v8 API.
- **4 Custom Asset Slots**: Configurable for European ETFs (Amundi MSCI World, Stoxx Europe 600), US indices, commodities (Gold, Crude Oil), equities, cryptocurrencies, and Forex currency pairs.
- **On-Device Asset Picker**: Select any tracked slot directly from the pull-down Quick Control drawer with 34 popular presets across 6 categories.
- **Line & Candlestick Charts**: Toggle between continuous sparkline trendlines and authentic colored trading candlesticks (emerald bullish, coral bearish, wicks for High/Low, bodies for Open/Close).
- Tap rows to select focused asset, double-tap to trigger immediate refresh.

### 🛰️ 10. ISS Orbit Tracker (`ScreenIss`)
- **Astronomical Day/Night World Map**: High-contrast 320×160 equirectangular world map with real-time solar terminator day/night shading.
- **Orbit Ground Track**: Past trajectory (45-min dashed path) and forward predicted ground track (92-min solid amber curve).
- **Line-of-Sight Footprint**: Radio/visual horizon ring (~2,200 km range) around the ISS with observer reticle.
- **Telemetry HUD**: Live altitude, velocity, slant distance, next pass countdown, and peak elevation prediction.
- **Acoustic Proximity Ping**: Sonar ping (`BEEP_SONAR_PING`) when the ISS enters visible range.

### 📊 11. Daily Air Traffic Statistics & Info Screen (`ScreenInfo`)
- Tracks 24-hour airspace activity: unique aircraft count, speed record with callsign, maximum detection distance with callsign, altitude flight level span (FL min/max), and total ADS-B reports received.
- **Configurable Filter**: Count all received ADS-B aircraft or only planes within the active zoom radius. Auto-resets at midnight.

### 🔊 12. Active Buzzer Acoustic Alert System
- Onboard active buzzer integration for emergency squawks (7700/7600/7500), watched aircraft entry, overhead passes, approaching storm alerts, sonar pings, hourly chimes, touch clicks, and automatic night-time muting.

### 🌙 13. Ultra Night Mode & Ambient Display Control
- **Deep-Red Night Mode**: Monochromatic deep-red sleep mode running at ultra-low backlight brightness (0.5%) to preserve dark-adapted vision.
- **Night Clock Only Mode (`nightClockOnly`)**: Automatically stops screen cycling during sleep hours and locks the display dimmed onto the Clock face.

### 📱 14. Quick Control Center & Smart Gestures
- Pull-down drawer from top edge for instant control of brightness, night mode, buzzer mute, legend toggle, and screen cycling.
- Smartphone-like touch gestures: horizontal swipes for screen transitions, vertical swipe for watchface style and radar zoom.

### 🕒 15. Hardware RTC (PCF85063) & Power Independence
- Automatic boot and network NTP synchronization with the onboard PCF85063 real-time clock.
- Supports soldering a **3.3V 1.0F–1.5F supercapacitor** to the `BAT` and `GND` pads for battery-free timekeeping during power outages.

### 🖥️ 16. Rock-Solid ST7701 Display Driver
- Calibrated 8 MHz RGB pixel clock with widened timing porches (`HBP 50`, `VPW 8`, `VBP 20`), 4 MHz SPI init, and deferred display enablement permanently eliminating hardware vertical image wrap and row counter desynchronization.
- Configurable manual screen orientation (0°, 90°, 180°, 270°).

### ⚡ 17. Robust Dual-Core FreeRTOS Architecture
- **Core 1:** Dedicated to ST7701 RGB rendering (double-framebuffer, zero flicker), CST820 capacitive touch, and UI animations.
- **Core 0 (`AsyncNetWorker`):** Non-blocking background worker handling mbedTLS handshakes, radar tile caching, ADS-B JSON parsing, and HTTP web serving with cooperative dual-core network arbitration.

### 🌐 18. Comprehensive Web Dashboard & Zero-Fail OTA
- Complete device configuration, screen switching, and remote controls.
- Live telemetry tables for ADS-B flights and ISS status.
- Real-time web serial monitor (64 KB PSRAM ring buffer) over Wi-Fi without USB cables.
- Remote uncompressed 24-bit BMP screenshot capture tool.
- **Zero-Fail OTA**: Proactive PSRAM cache reclamation (>6 MB freed) and automatic direct-to-flash streaming fallback.

### 🔌 19. Smart Home REST API
- Direct JSON REST endpoints for Home Assistant, Node-RED, or scripts (`/api/status`, `/api/hardware`, `/api/screen`, `/api/toggle-legends`, `/api/rtc/sync_ntp`).

---

## 📱 Screen Overview

| Screen | Preview | Description | Data Source |
| :--- | :---: | :--- | :--- |
| **1. Clock** | <img src="docs/media/clock_stacked_bold.png" width="70" /> | 7 selectable watchfaces (Stacked Bold, Aviator, HUD, etc.), forecast pills, weather, moon phase, solar arc | Open-Meteo & Astro Engine |
| **2. Planes** | <img src="docs/media/plane_radar_live.png" width="70" /> | 360° airspace tracking, emergency squawks (7700/7600), airline routes, airport beacons, range rings | adsb.fi / adsb.lol |
| **2b. Aircraft Detail** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Tap any aircraft to reveal full telemetry, route origin/destination, and high-res aircraft photography | Planespotters.net API |
| **3. Weather Radar** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animated precipitation radar loop with smooth cross-dissolve, reflectivity scale, and city markers | SHMÚ (SK), ČHMÚ (CZ), RainViewer |
| **4. Tactical Radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | **Combined tactical view:** Live precipitation radar + ADS-B flights overlay on a single screen | SHMÚ / ČHMÚ / RainViewer + adsb.fi |
| **5. Forecast** | <img src="docs/media/forecast_screen.png" width="70" /> | Hourly temperature, wind, and rain curves, 3-day forecast, Air Quality (AQI), PM2.5, and pollen count | Open-Meteo Weather & Air Quality |
| **6. Markets & Crypto** | <img src="docs/media/finance_screen.png" width="70" /> | Real-time tracking of 4 customizable market tickers (ETFs, Stocks, Commodities, Crypto, Forex) with interactive sparkline charts | Yahoo Finance v8 |
| **7. ISS Orbit Tracker** | <img src="docs/media/screen_iss_live.png" width="70" /> | Global equirectangular world map with real-time day/night solar terminator, past/future orbits, footprint ring, next pass countdown | WhereTheISS API |
| **8. Air Traffic Stats** | <img src="docs/media/flight_stats_screen.png" width="70" /> | Daily 24h airspace activity: unique aircraft count, speed record, altitude span, max range, ADS-B reports | FreeRTOS PSRAM Tracker |
| **9. Settings** | <img src="docs/media/settings_screen.png" width="70" /> | Device telemetry, IP address, brightness slider, map orientation, radar smoothing, language selector | System |

---

## 🖐️ Gesture & Touch Controls

| Gesture / Action | Action |
| :--- | :--- |
| **Swipe Left / Right** | Transitions smoothly to the next / previous screen with slide animation. |
| **Pull Down from Top Edge** | Opens the **Quick Control Center** (brightness, night mode, screen toggles). |
| **Swipe Up / Down in Center** | **On Clock:** Cycles to previous / next watchface style.<br>**On Radars:** Zoom In (swipe up) / Zoom Out (swipe down).<br>**On Control Center:** Closes the overlay. |
| **Tap Bottom Range Bar** | Left half zooms Out, right half zooms In. |
| **Tap on Aircraft** | Opens full color-coded aircraft telemetry detail card with live photo. |
| **Tap on Aircraft Photo** | Enlarge aircraft photo to full screen (tap anywhere to return to card). |
| **Double-Tap (Knock on chassis / desk)** | **On Radars:** Toggles Clean Map Mode (hides legends).<br>**On Markets / ISS:** Forces immediate live data refresh. |
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
- **Hardware Diagnostics & Web Serial Monitor:** Real-time web serial console (64 KB PSRAM ring buffer) over Wi-Fi without USB cables, RTC status, and manual time sync buttons.
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
- Original base project: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.
- Enhancements, Slovak localization, SHMÚ radar integration, bilinear anti-aliasing, Planespotters aircraft photos, tactical combined radar, expanded watchfaces, RTC driver, IMU gestures, touch navigation, Quick Control Center, flight watchlist, and multi-core stabilization: **Rado & Antigravity AI**.
