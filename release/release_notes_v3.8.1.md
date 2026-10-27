# MeteoPlaneRadar v3.8.1 — YouTube API Polling Throttle & Anti-Quota Exhaustion

**MeteoPlaneRadar v3.8.1** delivers critical improvements to the **YouTube Channel Analytics API client**, extending query intervals to 30 minutes and eliminating rapid retry loops to prevent daily Google Cloud quota exhaustion (10,000 units/day).

---

## 🌟 Highlights & Bug Fixes

### 📺 YouTube Polling Throttle & Anti-Quota Protection
- **30-Minute Universal Polling Intervals**:
  - `YOUTUBE_PERIOD_ACTIVE_MS`: Increased from 3 minutes to **30 minutes** (`30UL * 60UL * 1000UL`).
  - `YOUTUBE_PERIOD_BG_MS`: Increased from 15 minutes to **30 minutes** (`30UL * 60UL * 1000UL`).
  - Protects against reaching the default free quota of 10,000 daily Google YouTube Data API v3 units.
- **Eliminated 15-Second Retry Storm**:
  - Fixed logic in `AsyncCore.cpp` where uninitialized or failed YouTube fetches (`YouTube_LastUpdated() == 0`) repeatedly retried every 15 seconds (5,760 polls/day).
  - Failed fetches now enforce a full 30-minute background backoff period (`YOUTUBE_PERIOD_BG_MS`).
- **Fixed 4-Second Complication Poll Loop**:
  - Fixed a condition in `ScreenClock.cpp` where missing YouTube data (`ytMissing`) caused complications on the Clock screen to request updates every 4 seconds.
  - Now strictly throttled to the 30-minute interval (`YOUTUBE_PERIOD_ACTIVE_MS`).
- **Carousel Swipe Navigation Guard**:
  - Swiping past the YouTube screen or Clock screen complication now checks cached data age before requesting a network update.
  - Tapping the YouTube screen directly remains available for on-demand immediate refresh.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.8.1-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.8.1-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## 🚀 How to Flash / Update

### On-Device OTA Update
Navigate to **Settings** (`Screen 11`) on the device or open the **Web Portal** at `http://<device-ip>/` and click **Check for Updates** under the System card.

### Web Flash Tool
1. Connect the ESP32-S3 board via USB.
2. Open Chrome/Edge and visit [ESP Web Flasher](https://espressif.github.io/esptool-js/).
3. Load `MeteoPlaneRadar-v3.8.1-factory.bin` at offset `0x0000`.
4. Click **Program**.

### Command Line (`esptool.py`)
```bash
esptool.py --chip esp32s3 -p COMx -b 921600 write_flash 0x0000 MeteoPlaneRadar-v3.8.1-factory.bin
```
