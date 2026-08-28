# MeteoPlaneRadar (Slovak & Tactical Radar Edition)

**Clock, Live Aircraft Radar, Weather Precipitation Radar, Combined Tactical Radar, and Weather Forecast on a round 480×480 touchscreen.**  
Runs on the **Waveshare ESP32-S3-Touch-LCD-2.1** development board with full web browser configuration.

> 🇸🇰 Slovenská verzia: **[README.md](README.md)**  
> 📌 Forked and enhanced from **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)** by **[chiptron.cz](https://chiptron.cz)**.

---

## 🌟 Key Features & Improvements in this Fork

* 🇸🇰 **Full Slovak Language Support (`LANG_SK`)** – Complete diacritics and character rendering across all display screens, the captive WiFi portal, the web configuration interface, and the OTA updater (`Slovenčina` / `Čeština` / `English`).
* 🛰️ **New Tactical / Combined Radar Screen (`ScreenTactical`)** – Live combined overlay showing precipitation radar tiles in the background and real-time ADS-B aircraft traffic in the foreground, complete with flight track orientation, altitude color coding, callsigns, squawk emergency badges, and interactive flight details on tap.
* 🔍 **Unified 5 Zoom Levels** across all radar screens – `10/25 km`, `50 km`, `100 km`, `200 km`, and `Whole Country` (`0 km`).
* 🗺️ **Location-Aware Whole Country View** – When zooming out completely, the display automatically centers and scales to cover the entire territory of Slovakia (250 km radius, Zoom 6) or Czechia (240 km radius).
* 🏙️ **Curated Slovak Cities Database (`SkCitiesData.h`)** – 17 balanced regional hubs with clean 2-letter abbreviations and collision avoidance to prevent map clutter.
* ⚡ **Optimized Background Network Activity** – Aircraft ADS-B and weather radar tiles are downloaded strictly for the currently active/enabled screens; air quality & pollen reports only refresh when the Forecast screen is enabled.
* 🛠️ **Full PlatformIO Support (`platformio.ini`)** – Fast and reliable single-command compilation and flashing with automated dependency management.

---

## 📱 Display Screens

| Screen | Description | Data Source |
| --- | --- | --- |
| **Clock** | Current time, date, outdoor temperature, weather condition icon, and seconds ring | Open-Meteo |
| **Planes** | Live surrounding aircraft traffic, altitude, speed, track, squawk alerts, and route details | adsb.fi, adsb.lol |
| **Weather** | Animated live precipitation radar map | ČHMÚ or RainViewer |
| **Tactical Radar** | **Combined view:** Live precipitation radar + live ADS-B aircraft on a single map | RainViewer / ČHMÚ + adsb.fi |
| **Forecast** | 6-hour and 3-day forecast, air quality (AQI, PM2.5), and pollen forecast | Open-Meteo |
| **Settings** | Status info, IP address, brightness, map top bearing, language, and WiFi status | — |

All data screens can be independently enabled/disabled in the web settings; the Settings screen is always accessible.

---

## 🔧 Hardware

Designed for the **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:
- **MCU:** ESP32-S3R8 (Dual-Core 240 MHz, 8 MB Octal PSRAM, 16 MB Flash)
- **Display:** Round 2.1" IPS LCD (480×480 px, ST7701 RGB interface)
- **Touch:** CST820 capacitive touch (I2C)
- **I/O Expander:** TCA9554 (controls LCD power & reset)
- **Connection:** USB-C (no soldering required).

---

## 🚀 Setup & First Boot

1. Flash the firmware onto the board (via PlatformIO or USB web flasher).
2. On initial startup, the device launches an open WiFi access point named **`MeteoPlaneRadar`** (and shows a QR code on the display).
3. Connect with a phone or laptop and open **`http://192.168.4.1/`**.
4. Select your home WiFi network, enter your password, and save.
5. Once connected, access the web interface at:
   - **`http://meteoplaneradar.local/`** (or the IP address shown on the Settings screen).

### Web Configuration
The web interface runs continuously and lets you configure:
- Device geographic location (with city search).
- Enabled screens and auto-rotation cycle timer.
- Weather radar provider (ČHMÚ for Czechia / RainViewer for Europe & worldwide).
- Daytime and nighttime brightness (manual or automatic based on sun times).
- Aircraft altitude filters, emergency squawk notifications, and watchlists.
- Remote control (switch screens and change zoom level without touching the screen).
- Settings export/import and wireless OTA firmware updates.

---

## 🖐️ Touch Gestures

| Gesture / Action | Result |
| --- | --- |
| **Swipe Left / Right** | Cycle radar range / zoom level (10/25, 50, 100, 200 km, Whole Country) |
| **Tap on Aircraft** | Open / close flight details modal (callsign, registration, type, speed, climb, origin $\to$ destination) |
| **Tap at Bottom of Display** | Cycle radar zoom level |
| **Long Press Left / Right** | Switch to previous / next screen |
| **Hold BOOT Button during power-up (~3 s)** | Factory reset (erases NVS settings & WiFi credentials) |

---

## 💻 Building and Flashing with PlatformIO

A ready-to-use `platformio.ini` configuration is included:

```powershell
# Compile the firmware
pio run

# Flash to the ESP32-S3 board over USB
pio run -t upload --upload-port COM9
```

### Wireless OTA Update
Open **`http://meteoplaneradar.local/update`** in your browser and upload the generated `.pio/build/esp32-s3-touch-lcd-21/firmware.bin` file.

---

## 🌐 Data Sources & Attribution

This project uses free, public APIs (intended strictly for personal, non-commercial use):
- **Aircraft Positions & Details:** [adsb.fi](https://adsb.fi)
- **Flight Routes:** [adsb.lol](https://adsb.lol)
- **Precipitation Radar (Worldwide):** [RainViewer](https://www.rainviewer.com)
- **Precipitation Radar (Czechia):** [ČHMÚ](https://opendata.chmi.cz)
- **Weather Forecast & Air Quality:** [Open-Meteo](https://open-meteo.com)
- **Geocoding & Location:** [ip-api.com](http://ip-api.com) and Open-Meteo Geocoding
- **Vector Outlines & Cities:** Natural Earth borders (public domain), GeoNames cities (CC BY 4.0).

---

## 📄 Original Project & Credits

This project builds upon the work of:
- **Original Project:** [petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar) (Petr / [chiptron.cz](https://chiptron.cz))
- **Article & Hardware Details:** [chiptron.cz: Meteoradar a radar letadel na kulatém displeji](https://chiptron.cz/meteoradar-a-radar-letadel-na-jednom-kulatem-displeji/)
- [ok1cdj/MeteoPlaneRadar](https://github.com/ok1cdj/MeteoPlaneRadar) by Ondra OK1CDJ
- [CooLajz/waveshare-hodiny](https://github.com/CooLajz/waveshare-hodiny)
- [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar) and [Selbyl](https://github.com/Selbyl/ESP32-S30Touch-LCD-2.1_Plane-Radar)

---

## 📜 License

Released under the **MIT License** – see [LICENSE.txt](LICENSE.txt).
