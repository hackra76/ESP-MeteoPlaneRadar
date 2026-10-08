# MeteoPlaneRadar v3.0.0 — Next-Gen Circular UI & LVGL 9 Overhaul

**MeteoPlaneRadar v3.0.0** is the biggest update in the project's history. It replaces the legacy graphics pipeline with a double-buffered **LVGL 9.1** rendering engine, modernizes the radar displays into a true **Glass Cockpit** avionics experience, introduces a handcrafted **Luxury Astronomical Chronograph**, adds a brand-new **Cold War Tactical Sonar**, and enhances the virtual companion into **DigiCat 2.0 Aero-Cat Co-Pilot**.

---

## 🌟 Highlights & New Features

### ⏱️ Luxury Astronomical Chronograph (Screen 0)
- **Unified Luxury Watchface:** Replaced legacy multi-watchfaces with a handcrafted Swiss luxury horology chronograph designed specifically for 480×480 round displays.
- **24-Hour Solar Twilight Arc:** Outer perimeter bezel displaying real-time solar progression (Noon at 12 o'clock, Midnight at 6 o'clock), astronomical sunrise and sunset markers, 24K gold morning/evening golden hour bands, and a dynamic Sun glyph orbiting in real-time.
- **Photorealistic 3D Moon Phase Aperture:** Top complication featuring an embedded 92×92 photographic NASA LROC lunar albedo texture rendered with analytical 3D Lambertian diffuse lighting, limb darkening, smooth phase terminator shadow, and illumination percentage.
- **Temperature Trend Subdial:** Bottom complication featuring 12 rose-gold radial ticks, outdoor temperature, and an electric-blue area sparkline trend curve with glowing end bead. Tap jumps directly to the 3-day weather forecast.
- **Dynamic Seconds Orbit & Bloom Arc:** Electric-blue concentric track with an orbiting 30 FPS luminous bead leaving an active Gaussian bloom arc from 12 o'clock to the current second.
- **Swiss Anti-Aliased Typography:** Clean white 76px digital clock digits (HH:MM) with supersampled anti-aliased bezels and a localized date complication.
- **Interactive Overlays:** Floating alert banners for overhead aircraft and approaching precipitation with tap-to-inspect navigation.

### 🛩️ Glass Cockpit Avionics Radar (Screens 1 & 3)
- **360° Aviation Compass Rose:** Outer perimeter heading bezel featuring 10° minor ticks, 30° major ticks, yellow North caret, and standard aviation heading numerals (30, 33, 03, 06, E, 12, 15, S, 21, 24, W).
- **Forward Velocity Vectors:** Dynamic 1-minute forward ground track leader lines with speed indicator pips.
- **Selectable Target Blip Styles:** Toggle between Glass Cockpit aerodynamic vector chevrons (with red double-chevrons for combat jets) and aircraft silhouettes.
- **Decluttered Floating Stacked Callouts:** High-contrast 3-line avionics data tags with smart ORIG>DEST route replacement, altitude + vertical rate trend, and speed.
- **Metric vs. Aviation Units:** Seamless switching between meters & km/h and Flight Levels (FL/ft) & knots.
- **Aero Glass Flight Deck HUD (Screen 1b):** Aspect-fit edge-to-edge aircraft photo viewer with zero truncation and live avionics telemetry overlay (altitude, speed, heading, route, registration).

### 🗺️ Multi-Provider Map Underlays
- **Online Raster Map Tiles:** Seamlessly switch between **Esri World Dark Gray Canvas** and **OpenStreetMap Standard** tiles cached to flash memory.
- **Offline Vector Maps:** High-speed offline European borders and city dataset (`EuBorder`) as an instant fallback without network dependence.

### 📡 Cold War Tactical Sonar (Screen 4)
- **Phosphor Green CRT Radar Sweep:** Dedicated 25 FPS radar screen featuring an analog CRT phosphor persistence decay shadow.
- **3 View Modes:**
  - **Classic PPI:** 360° circular phosphor sweep with fading target contacts.
  - **Split View:** Upper CRT sweep arc paired with lower Bearing-Time Record (BTR) waterfall spectrogram.
  - **Full Waterfall:** Full-height 340px BTR waterfall spectrogram with time marks (`NOW`, `-1m`, `-2m`, `-3m`, `-4m`).
- **Doppler Velocity Color Encoding:** Closing targets in cyan, opening targets in amber, and cross-track targets in phosphor green.
- **Acoustic Ping:** Tactile 8ms micro-click ping played when the sweep beam contacts aircraft targets.

### 🛰️ 3D Orbital Space Command ISS Tracker (Screen 7)
- **3D Orthographic Spherical Earth:** Scanline raytraced 3D globe with real-time Day/Night solar terminator shading.
- **Rayleigh Atmospheric Limb Scattering:** Radiant blue atmospheric glow encircling the planetary rim.
- **Orbit Propagation & Horizon Footprint:** Orbital track lines and radio reception horizon circle projected on the 3D sphere.
- **3 View Modes:** Tap to switch between 3D ISS-centered camera, 3D Home-centered camera, and classic 2D equirectangular world map.

### 🐱 DigiCat 2.0 Aero-Cat Flight Deck Co-Pilot
- **Top Gun Military Intercept Mode:** Gold aviator sunglasses, animated mini CRT radar scope, and automatic intercept reaction when combat aircraft are within 65 km.
- **Live 6-Axis IMU Attitude Telemetry:** Real-time PITCH, ROLL, and GYRO HEADING readout directly on the pet drawer HUD.
- **Google Gemini Live AI:** Powered by Gemini 3.5 Flash Lite / 3.8 Flash for intelligent air traffic thoughts with full offline fallback.
- **Smooth Animation Pipeline:** Frame caching and planar sprite buffers preventing display DMA bus saturation.

### 🖐️ Gesture Overhaul & Translucent Zoom Controls
- **Auto-Hiding Floating Zoom Controls:** Translucent circular glass `+` and `-` buttons appear on right-edge touch and auto-hide after 3.5s, eliminating accidental zoom triggers during horizontal swipes.
- **Quick Control Center:** Pull down from the top edge for instant access to brightness, sound, flight trails, target styles, map providers, and sonar acoustic pings.
- **Pet Drawer:** Swipe up from the bottom edge to reveal DigiCat.

---

## ⚡ Under the Hood & Performance

- **LVGL 9.1 Double-Buffering:** Two full 480×480 RGB565 framebuffers allocated in PSRAM with DMA transfers to eliminate tearing and achieve buttery-smooth 60 FPS transitions.
- **Zero PSRAM Fragmentation:** Dynamic row decimation during PNG decoding caps memory usage to 307KB per frame, guaranteeing rock-solid stability during 250km SHMÚ/ČHMÚ radar animation.
- **Freed Internal SRAM:** Moving image decoders to PSRAM recovered 63.5KB of internal SRAM, reducing heap usage from 71.3% to 51.9% and permanently resolving mbedTLS SSL allocation errors (`-32512`).
- **Interactive Web Mirror:** Web dashboard at `http://meteoplaneradar.local/` now includes an interactive virtual porthole device mirror with silent auto-refresh and touch simulation.

---

## 📦 Firmware Files

| File | Description | Target |
| :--- | :--- | :--- |
| **`MeteoPlaneRadar-v3.0.0-factory.bin`** | Complete factory image (bootloader, partitions, firmware) | USB Flash @ `0x00000000` |
| **`MeteoPlaneRadar-v3.0.0-ota.bin`** | Application firmware update | On-device Web OTA update |

### Flashing via Web Flasher / USB
Flash `MeteoPlaneRadar-v3.0.0-factory.bin` using [ESP Web Flasher](https://espressif.github.io/esptool-js/) or `esptool.py`:
```bash
esptool.py -p COM_PORT -b 921600 write_flash 0x0 MeteoPlaneRadar-v3.0.0-factory.bin
```
