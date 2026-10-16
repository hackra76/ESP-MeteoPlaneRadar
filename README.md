# MeteoPlaneRadar (Tactical & Precipitation Weather Radar for ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-EN%20%7C%20SK%20%7C%20CZ-green.svg)
![Release](https://img.shields.io/badge/Release-v3.6.0-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunctional weather station, live ADS-B flight radar, animated precipitation radar (SHMÚ, ČHMÚ, RainViewer), combined tactical radar, ISS orbit tracking, YouTube channel analytics, financial market tickers, Klipper 3D printer telemetry HUD, disaster warning system with hardware seismometer, and an animated virtual pet companion on a round 2.1" IPS touchscreen.**  
Designed for the **Waveshare ESP32-S3-Touch-LCD-2.1** with smartphone-like touch gestures, a pull-down Control Center, live aircraft photos, bilinear radar smoothing, and a responsive web dashboard.

> 🇸🇰 Slovenská dokumentácia: **[README_SK.md](README_SK.md)**  
> 🔄 Forked and significantly enhanced from **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.

*If you enjoy this project, please consider supporting its development!* ☕  
<a href="https://buymeacoffee.com/hackra" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

---

## 📸 Live Device Demo

<p align="center">
  <img src="docs/media/clock_luxury.png" width="13%" alt="Luxury Astronomical Chronograph" />
  <img src="docs/media/tactical_radar_live.gif" width="13%" alt="Tactical Radar" />
  <img src="docs/media/screen_sonar.png" width="13%" alt="Phosphor Sonar Radar" />
  <img src="docs/media/screen_klipper.png" width="13%" alt="Klipper 3D Printer Telemetry" />
  <img src="docs/media/screen_iss_live.png" width="13%" alt="3D ISS Orbit Tracker" />
  <img src="docs/media/finance_screen.png" width="13%" alt="Markets & Crypto" />
  <img src="docs/media/plane_detail_photo.png" width="13%" alt="Aircraft Detail" />
</p>

### 🐱 DigiCat — Animated Virtual Pet Companion

<p align="center">
  <img src="docs/media/digicat_showcase_v2.png" width="80%" alt="DigiCat hi-res ginger cat animations" />
</p>

<p align="center">
  <em>DigiCat v3.6.0 — Professional 64×64 ginger cat sprites (247 frames, 38 animation sequences, RGB565 PROGMEM, rendered at 3× scale / 192×192 px).</em>
</p>

---

## 🌟 Features

### ⏱️ v3.6.0 Luxury Dial Complications, Moon Lighting & Network Isolation Release
- **Configurable Dual Circular Dial Complications (Screen 0):**
  - Independent configuration of Top ($r=48$, $y=96$) and Bottom ($r=56$, $y=356$) circular subdial apertures on the Luxury Astronomical Chronograph.
  - 7 interchangeable complication types: 3D Photorealistic Moon Phase, Temperature Trend & Sparkline (24h), Current Weather & Condition Icon, ADS-B Aircraft count within 10 km, YouTube Subscribers count, Primary Market Ticker, and Klipper 3D Printer Progress.
  - High-resolution dynamic luxury dial background rendering in PSRAM with engraved subdial floors, radial ambient occlusion, and 12 calibrated gold tick marks.
  - Interactive touch routing: tapping either subdial aperture directly jumps to its full telemetry screen.
  - Quick Control Center complication selector buttons with tactile buzzer feedback.
  - Web Portal configuration card with top/bottom slot dropdowns and full i18n (SK, CS, EN).
- **Astrophysical Moon Phase Lighting Correction:**
  - Corrected light vector normal calculation in `ScreenClock.cpp` (`float lx = sinf(phaseRad);`). Waxing crescent and quarter moon phases now accurately cast illumination on the right hemisphere, matching physical lunar observations in the northern hemisphere.
- **Automatic Complication Telemetry Supervision:**
  - Active complication telemetry (YouTube, Market, ADS-B, Weather) automatically updates in the background without requiring manual rotation through the carousel screens.
- **Klipper 3D Printer Network Isolation & Fast LAN Timeouts:**
  - Confined Klipper / Moonraker queries strictly to when the Klipper screen is actively displayed on device (`curScr == SCREEN_KLIPPER_I`). Completely eliminates background connection attempts to offline printers from other screens, preventing socket exhaustion, network freezes, and serial error spam.
  - Added configurable connect/request timeouts to `Net_GetString`: Klipper uses 1500 ms connect and 2500 ms request timeouts on LAN instead of blocking for 8000 ms.
  - Added 15-second offline backoff when actively viewing the Klipper screen while the printer is off.

### 📐 v3.5.0 Custom Screen Sequence & 3D Printer Overhaul Release
- **Custom Screen Ordering & Rotation Sequence Manager:**
  - Intuitive Tactile Field Instrument drag-and-drop & mechanical `▲`/`▼` step button matrix (`#scrOrderList`) in Common Settings (`tCommon`).
  - Seamless desktop drag-and-drop reordering with real-time visual guide lines paired with single-tap precision mechanical step buttons for touchscreens (smartphones/tablets).
  - Configurable sequence array `s_scrOrder[SCREEN_N]` persisted in ESP32 non-volatile storage (NVS key `scrOrd`).
  - Touchscreen swiping (`UI_SwitchScreenStep`) and remote web step navigation (`/api/screen`) traverse the exact user-configured order.
  - Auto-rotation cycling (`UI_AutoRotate_Tick`) follows the user-defined order while respecting disabled screen toggles and skipping the Settings screen.
  - Sidebar Quick Control buttons (`#scrBtns`) dynamically mirror the user's custom sequence in real time.
  - Instant debounced auto-save (`POST /api/config`) and one-click factory reset button (`↺ Default Order`).
- **Klipper / Moonraker 3D Printer Screen Overhaul:**
  - Accurate print ETA calculations using Moonraker `print_duration` vs `total_duration` vs `estimated_time` and layer-time estimation fallbacks (eliminates stale `0s` bug).
  - Screen enabled state persistence fix ensuring Screen 9 stays active across reboots.
  - Live Current Layer / Total Layers indicator parsed from Moonraker status (`L X/Y`, e.g. `L 142/350`).
  - Enlarged XYZ coordinate and homing status indicators for crisp legibility on the round 480×480 screen.
- **Embedded Web Flash Optimization:**
  - Re-compressed Web UI (`WebPageGz.h`) using Gzip Level 9 (packed to 65.8 KB, saving 187.8 KB flash PROGMEM).

### 🎛️ v3.4.0 Tactile Field Instrument & Industrial Web UI Release
- **Tactile Field Instrument Design Overhaul:** Embedded web portal completely re-engineered under the aesthetic principles of Braun, Teenage Engineering, and Swiss Architectural Grids.
- **Strict Anti-Slop Ban List Enforcement:** Completely banished generic AI tropes: slate/zinc blue dark backgrounds (`#0f172a`, `#1e293b`), neon purple/cyan glowing blurs, glassmorphism (`backdrop-filter: blur`), 8-20px rounded bento cards, generic pill badges, and generic sans data fonts.
- **Matte Industrial Materials Palette:** Grounded in deep matte carbon (`#121211`), milled chassis plate (`#171716`), structural hairlines (`#2A2A28`), and off-white primary ink (`#F4F3EF`), with strictly rationed functional accents: International Signal Orange (`#FF4F00`), Lab Ochre (`#E5A93C`), and Phosphor Green (`#00D062`).
- **Exposed Architectural Swiss Grid:** Zero-gap cell topology (`gap: 0; 1px solid #2A2A28`), technical corner crosshair markings (`+`) at junctions, and sharp 0px–2px chamfered corners.
- **Extreme Scale Typographic Contrast:** Grotesque technical headers (`Space Grotesk`) paired with strict tabular monospace (`JetBrains Mono`, `tabular-nums`) for all numbers, coordinates, altitudes, voltages, and logs (micro `10px` uppercase tracked labels vs raw `42px` hero readouts).
- **Tactile Laboratory Controls:** Segmented mechanical rocker switches with Signal Orange slides, rotary SVG arc gauges with calibrated tick marks for core die temperature and PSRAM, and phosphor LED matrix status diodes.
- **Standalone Instrument Simulator (`portal.html`):** Integrated real-time 480×480 CRT radar scope with sweeping beam, persistent phosphor decay, and aircraft telemetry blips.
- **PROGMEM Payload Optimization:** Pre-compressed Gzip Level 9 payload (`WebPageGz.h`) reduced to 62.7 KB (25.4% ratio, saving 184.8 KB of flash).

### 🖨️ v3.3.0 Klipper & Moonraker 3D Printer Telemetry Release
- **Cyberpunk / KNOMI-Inspired Circular HUD:** Dedicated live 3D printer telemetry screen (Screen 9) querying Moonraker over local Wi-Fi.
- **Hardware-Rendered Vector Micro-Icons:** Custom isometric 3D shaded calibration cube with wireframe edges, hotend nozzle with heating block, heated bed plate with rising heat waves, aviation homing crosshairs, bed mesh grid, and G-code document.
- **Dynamic State Badge & Status LED:** Real-time state detection with color-coded illuminated LED indicators for 16 operational states: `STANDBY`, `HOMING`, `PROBING / MESH`, `HEATING`, `HEAT SOAKING`, `PRINTING`, `PAUSED`, `FILAMENT RUNOUT`, `PURGING / CLEAN`, `COOLING DOWN`, `COMPLETED`, `CANCELLED`, `ERROR`, `MCU SHUTDOWN`, `STARTING UP`, and `DISCONNECTED`.
- **Intelligent Macro Lifecycle Gating:** Gated transient states on Moonraker's `idle_timeout` (`state: "Printing"` vs `"Ready"`/`"Idle"`) and `motion_report` (`live_velocity`), preventing displays from getting stuck on lingering LCD strings like `"Z tilt adjusting...."`.
- **Dual Glassmorphism Thermal Cards:** Hotend and heatbed cards with neon accent strips, high-precision temperatures, and target pill badges (`OFF` / `TGT XX°C`).
- **Real-Time Toolhead Elevation & Telemetry:** Dynamic X/Y/Z homed status badges, Z-height elevation pill, and volumetric flow / filament usage chip.
- **Web Interface Configuration:** Dedicated 3D printer configuration card in the web management portal with custom printer name, host/IP address, port, and API key with automatic mDNS hostname resolution.

### 🕒 Luxury Astronomical Chronograph
- **24-Hour Solar Twilight Arc:** Outer perimeter ring displaying real-time solar progression (Noon at 12 o'clock, Midnight at 6 o'clock), astronomical sunrise and sunset markers, 24K gold morning/evening golden hour bands, and a dynamic Sun glyph orbiting in real-time according to local solar time.
- **Photorealistic 3D Moon Phase Aperture:** Top complication featuring an embedded 92×92 photographic NASA LROC lunar albedo texture rendered with analytical 3D Lambertian diffuse lighting, limb darkening, smooth phase terminator shadow, and real-time illumination percentage.
- **Temperature Trend Subdial:** Bottom complication with 12 rose-gold radial hour ticks, outdoor temperature, and an electric-blue area sparkline trend curve with glowing end bead. Tap jumps directly to the 3-day weather forecast.
- **Dynamic Seconds Orbit & Bloom:** Electric-blue concentric ring with smooth orbiting luminous seconds bead (~30 FPS) leaving an active Gaussian bloom arc trailing from 12 o'clock to the current second.
- **Swiss Anti-Aliased Typography:** Clean white 76px digital clock digits (HH:MM) with supersampled anti-aliased bezels and a localized date complication.
- **Interactive Alert Banners:** Floating overhead flight banner and approaching precipitation banner with tap-to-inspect navigation.

### 🛩️ Aircraft Tracking & Glass Cockpit Radar
- **360° radar** (14–200 km) via adsb.fi / adsb.lol with range rings, airport beacons, and 360° aviation compass rose bezel.
- **Target blip styling**: Choose between Glass Cockpit aerodynamic vector chevrons (with combat jet double-chevrons) and aircraft silhouettes.
- **Forward velocity vectors**: 1-minute forward ground track leader lines with speed indicator pip.
- **Stacked avionics callouts**: 3-line floating data tags (Callsign or ORIG>DEST, Altitude & vertical rate trend, Ground speed & route) with decluttered spacing and metric/aviation units toggle.
- **Special flight detection**: Rescue (green), VIP/Government (gold), Heavy jets (cyan), Military (red) — glowing target rings and top alert banner.
- **Emergency squawk auto-focus** (7500/7600/7700): locks onto aircraft, dims all others, shows live telemetry banner.
- **Proximity vector** to nearest aircraft with distance, bearing, and altitude delta.
- **Offline route database**: callsign → origin/destination decoding (e.g., `BOJ→WAW`).
- **Aero Glass Flight Deck HUD**: Tap any aircraft for a full telemetry card with live photo from Planespotters.net; tap photo for an aspect-fit, edge-to-edge Aero Glass HUD with zero truncation and live avionics telemetry overlay.

### 🌧️ Precipitation Radar & Nowcasting
- Animated radar loops (SHMÚ / ČHMÚ / RainViewer) with temporal cross-dissolve, isotropic circular masking, and zero-fragmentation row downsampling.
- **TREC 2D nowcasting**: alerts only when rain is genuinely heading toward your location (ETA ≤ 35 min, miss ≤ 6 km).
- **Auto precipitation typing**: Rain / Sleet (1–3°C) / Snow (≤1°C) / Hail (>50 dBZ).

### 🛰️ Tactical Radar, Sonar, ISS, YouTube & Finance
- **Combined Tactical Radar**: precipitation radar + ADS-B flights on one screen simultaneously with glass cockpit avionics.
- **Tactical Acoustic Sonar**: 25 FPS CRT phosphor sweep, BTR waterfall spectrogram history, Doppler velocity encoding, and acoustic contact ping.
- **3D ISS Orbit Tracker**: 3D orthographic Earth globe with atmospheric Rayleigh limb halo, solar terminator Day/Night shading, orbital tracks, and 3 view modes (3D ISS, 3D Home, 2D Map).
- **YouTube Analytics**: live subscriber count, total views, and latest / top video stats (YouTube Data API v3).
- **Financial Markets**: up to 8 configurable tickers (ETFs, stocks, crypto, forex) with sparkline and candlestick charts (Yahoo Finance v8).

### 🌤️ Weather, Air Quality & Night Mode
- Hourly temperature, wind, and rain curves, 3-day forecast, AQI, PM2.5, and pollen (Open-Meteo).
- **Deep-Red Night Mode** (0.5% backlight) and `nightClockOnly` mode (locks to clock face during sleep hours).

### 🐾 DigiCat Virtual Pet (v3.0.0)
Full-screen interactive companion (Swipe Up from bottom edge or Quick Control Center paw icon).

- **247 frames · 38 animation sequences** — professional 64×64 RGBA ginger cat sprites, RGB565 PROGMEM, rendered at **3× scale (192×192 px)**.
- **Aero-Cat Co-Pilot**: Top Gun military intercept mode with gold aviator sunglasses, animated mini radar scope, and live 6-axis IMU attitude telemetry (PITCH, ROLL, GYRO HEADING).
- **Rich idle personality (9 sub-animations)**: tail-wag sit, lick paw sitting, lick paw lying, meow sitting/lying/standing, scratch left/right ear, yawn — randomly cycled every 4–10 s.
- **Happy reaction (3 variants)**: sitting, standing-front, standing-right tail-wag — cycling every 1.5–4 s.
- **Aircraft swat (3 variants)**: standing swipe, sitting right paw, sitting left paw — rolled each encounter.
- **5 randomised sleep poses**: variant + left/right orientation locked for the entire night.
- **Direction-aware locomotion**: authentic `cat_run_left` when entering from the right; `cat_slide_left` when tilting left.
- **Live weather accessories**: umbrella (rain/storm), knit scarf (snow/cold ≤2°C), aviator goggles (clear).
- **Weather-reactive runway**: dry asphalt / wet reflection / snow dusting.
- **Autonomous AI brain**: random entrances, patrol strolls, pounces, grooming, stretches, and off-screen airfield excursions every 6–12 s.
- **Live aircraft interaction**: jet glides overhead with contrails & strobes → DigiCat sprints and swats; jet hops evasively; DigiCat jumps in pursuit.
- **Night behavior**: 2-min wake window on interaction; yawn → stretch → sleep wind-down; tap to wake.
- **Virtual pet stats**: Happiness & Hunger (0–100%); aviation XP rank (*Kitten Cadet → Radar Navigator → Airspace Ace*).
- **Google Gemini Live AI**: optional Gemini Flash integration for live air traffic commentary (100% offline fallback, SK/CZ/EN).

### ⚡ System & Connectivity
- **Dual-Core FreeRTOS**: Core 1 for LVGL 9 RGB rendering (double-framebuffer, zero flicker) + touch; Core 0 for network, radar caching, ADS-B parsing.
- **Zero-Drift ST7701 driver**: calibrated 8 MHz RGB clock, no SPI Flash writes during carousel, VSYNC auto-recovery.
- **Hardware RTC** (PCF85063) with optional supercapacitor backup (3.3 V, 1.0–1.5 F).
- **Web dashboard**: configuration, OTA, live serial monitor (64 KB PSRAM ring buffer), interactive virtual device mirror with touch simulation, remote screen control.
- **Smart Home REST API**: `/api/status`, `/api/screen`, `/api/display/resync`, `/api/toggle-legends`, `/api/rtc/sync_ntp`.
- **Active buzzer**: emergency squawk alerts, storm warnings, ISS sonar ping, acoustic sweep ping, hourly chimes, night muting.

---

## 📱 Screen Overview

| Screen | Preview | Description | Source |
| :--- | :---: | :--- | :--- |
| **0. Clock** | <img src="docs/media/clock_luxury.png" width="70" /> | Luxury Astronomical Chronograph (24h solar arc, 3D moon, temp sparkline) | Open-Meteo |
| **1. Planes** | <img src="docs/media/plane_radar_live.png" width="70" /> | Glass Cockpit ADS-B radar, 360° compass rose, velocity vectors, map tiles | adsb.fi / adsb.lol |
| **1b. Aircraft Detail** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Aero Glass Flight Deck HUD with full telemetry + live photo on tap | Planespotters.net |
| **2. Weather Radar** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animated radar loop, downsampled memory-safe decoding, rain nowcasting | SHMÚ / ČHMÚ / RainViewer |
| **3. Tactical Radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | Precipitation radar + live ADS-B overlay with glass cockpit styling | SHMÚ / ČHMÚ + adsb.fi |
| **4. Sonar** | <img src="docs/media/screen_sonar.png" width="70" /> | CRT phosphor radar sweep, BTR waterfall spectrogram, Doppler velocity, acoustic ping | adsb.fi |
| **5. Forecast** | <img src="docs/media/forecast_screen.png" width="70" /> | Hourly curves, 3-day forecast, AQI, PM2.5, pollen | Open-Meteo |
| **6. Markets & Crypto** | <img src="docs/media/finance_screen.png" width="70" /> | Up to 8 custom tickers, sparkline & candlestick charts | Yahoo Finance v8 |
| **7. ISS Tracker** | <img src="docs/media/screen_iss_live.png" width="70" /> | 3D orthographic Earth globe, solar terminator, Rayleigh limb glow, orbits | WhereTheISS API |
| **8. YouTube** | <img src="docs/media/youtube_screen.png" width="70" /> | Subscriber count, total views, latest / top video stats | YouTube Data API v3 |
| **9. 3D Printer (Klipper)** | <img src="docs/media/screen_klipper.png" width="70" /> | Live Klipper 3D printer telemetry HUD, vector icons, nozzle/bed temps, homing, progress | Moonraker API |
| **10. Flight Stats & Info** | <img src="docs/media/flight_stats_screen.png" width="70" /> | 24h airspace activity across 6 zoom scopes, internal SRAM & PSRAM telemetry | PSRAM Tracker |
| **11. Settings** | <img src="docs/media/settings_screen.png" width="70" /> | Device telemetry, IP, brightness, language, map provider, audio, OTA | System |

---

## 🖐️ Gestures & Touch Controls

| Gesture | Action |
| :--- | :--- |
| **Swipe Left / Right** | Previous / next screen |
| **Pull Down from top edge** | Quick Control Center (brightness, night mode, trails, blip style, map provider) |
| **Swipe Up from bottom edge** | DigiCat companion drawer |
| **Tap right flank zone (Radars)** | Reveal floating translucent `+` / `-` zoom buttons (auto-hide after 3.5s) |
| **Tap `+` / `-` zoom buttons** | Zoom in / zoom out radar range |
| **Tap bottom subdial (Clock)** | Jump directly to Weather Forecast screen |
| **Tap alert banner (Clock)** | Jump to Planes (overhead flight) or Weather Radar (rain alert) |
| **Tap aircraft blip** | Open telemetry detail card with live photo |
| **Tap aircraft photo** | Open full-screen Aero Glass Flight Deck HUD (tap to return) |
| **Double-tap screen (Radars)** | Toggle clean map mode (hide / show legends & labels) |
| **Tap top-right badge (Sonar)** | Cycle sonar view mode (Classic PPI → Split → Waterfall) |
| **Tap bottom range pill (Sonar)** | Cycle sonar range (25, 50, 100, 150 km) |
| **Tap Earth globe (ISS)** | Cycle ISS view mode (3D ISS → 3D Home → 2D Map) |
| **Tap screen (Klipper)** | Request instant telemetry refresh from Moonraker |
| **Tap pet (DigiCat)** | Pet / affection (purr, blush, dialogue); tap forehead to toggle Top Gun mode |
| **Double-tap pet** | Feed treat (restores hunger & happiness) |
| **Hold BOOT button ~3 s** | Factory reset (clears Wi-Fi & NVS) |

---

## 🔧 Hardware Specifications

Built for the **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Component | Specification |
| :--- | :--- |
| **MCU** | ESP32-S3R8 · Dual-Core LX7 @ 240 MHz |
| **Memory** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Display** | Round 2.1" IPS · 480×480 px · ST7701 RGB |
| **Touch** | CST820 / CHSC6540 Capacitive (I2C) |
| **IMU** | QMI8658 6-axis Accel + Gyro |
| **RTC** | PCF85063 (I2C `0x51`) |
| **Connectivity** | USB-C · Wi-Fi 802.11 b/g/n 2.4 GHz |

---

## 🚀 Installation

### Method A — Pre-built Binary (Easiest)
Download from **[Releases](https://github.com/hackra76/ESP-MeteoPlaneRadar/releases)**:
- `MeteoPlaneRadar-v3.5.0-factory.bin` — full image (bootloader + partitions + firmware).
- Flash via [ESP Web Flasher](https://espressif.github.io/esptool-js/) at 921600 baud, address `0x00000000`.
- Or: `esptool.py -p COM_PORT -b 921600 write_flash 0x0 MeteoPlaneRadar-v3.5.0-factory.bin`

### Method B — PlatformIO
```bash
pio run            # build
pio run -t upload  # flash
```

### First Boot & Wi-Fi Setup
1. Device creates open AP **`MeteoPlaneRadar`** and shows QR code on screen.
2. Connect → open `http://192.168.4.1/` → enter home Wi-Fi credentials → Save.
3. Web dashboard at **`http://meteoplaneradar.local/`** (or device IP).

---

## 🔑 YouTube Data API Key Setup

1. Create a project at [Google Cloud Console](https://console.cloud.google.com/).
2. Enable **YouTube Data API v3** and create an **API Key** (APIs & Services → Credentials).
3. In the device web dashboard → **YouTube** tab → paste key + channel handle (`@name`) or Channel ID (`UC...`).

---

## 🌐 REST API

| Endpoint | Method | Description |
| :--- | :---: | :--- |
| `/api/status` | GET | Full JSON status (weather, aircraft, heap, ISS) |
| `/api/hardware` | GET | Peripherals, RTC, I2C scan, reset reason |
| `/api/screen` | POST | Switch screen `{"index": 0}` (0–10) |
| `/api/display/resync` | POST | Force display hardware resync |
| `/api/toggle-legends` | POST | Toggle clean map mode |
| `/api/rtc/sync_ntp` | POST | Force NTP → RTC sync |

---

## 📜 License & Credits

MIT License.
- Base project: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**
- DigiCat virtual pet mechanics: **[aquascape123/digicat](https://github.com/aquascape123/digicat)** (MIT)
- Enhancements, SHMÚ radar, ISS tracker, watchfaces, touch navigation, DigiCat hi-res sprites, AI pet brain, web dashboard, Slovak localization: **Rado & Antigravity AI**
