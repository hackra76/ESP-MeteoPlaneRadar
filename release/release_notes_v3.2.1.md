# MeteoPlaneRadar v3.2.1 — Memory Stability Fix

**MeteoPlaneRadar v3.2.1** is a stability patch that fixes repeated `low memory ... fetch deferred` warnings which blocked TLS fetches (ISS, Disaster Warning, ADS-B, SHMÚ) on the serial console.

---

## 🛠️ Fixed

### 🧠 LVGL memory pool moved from internal RAM to PSRAM
- **Root cause**: LVGL 9 ignores the v8-style `LV_MEM_CUSTOM_*` macros in `lv_conf.h`, so its builtin 64 kB memory pool was a static array in internal DRAM `.bss`. This left only ~73 kB of free internal RAM with a 31 kB largest block. A single mbedTLS session needs two contiguous 17.2 kB record buffers plus ~12 kB of handshake state, so `Net_HeapOk()` kept deferring network fetches.
- **Fix**: `lv_conf.h` now sets `LV_MEM_SIZE` to 256 kB and allocates the pool with `LV_MEM_POOL_ALLOC` = `heap_caps_malloc(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`. About 64 kB of internal RAM is freed for TLS, and LVGL gains 4x more headroom.
- **Verified on hardware**: clean boot; ADS-B, forecast and GitHub OTA check run without any `low memory` warnings.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.2.1-factory.bin` | ~4.8 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.2.1-ota.bin` | ~4.8 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## ⚡ Flashing Instructions

### Web Browser / Web Flasher
1. Open the [ESP Web Tools / Web Flasher](https://espressif.github.io/esptool-js/).
2. Connect your **Waveshare ESP32-S3-Touch-LCD-2.1** via USB-C.
3. Select `MeteoPlaneRadar-v3.2.1-factory.bin` at offset `0x0000` and click **Program**.

### PlatformIO CLI
```powershell
$env:PYTHONIOENCODING="utf8"; pio run -t upload
```

### OTA (existing v3.x devices)
Use the built-in GitHub OTA updater, or upload `MeteoPlaneRadar-v3.2.1-ota.bin` manually at `/api/ota`.
