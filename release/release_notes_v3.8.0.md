# MeteoPlaneRadar v3.8.0 — DigiCat Interactive Companion & Network Engine Hardening

**MeteoPlaneRadar v3.8.0** delivers a major interactive upgrade to the **DigiCat Virtual Pet Companion** alongside comprehensive **networking stack hardening**, **live Clock complication telemetry**, and **cross-core synchronization**.

---

## 🌟 Highlights & New Features

### 🐱 DigiCat Interactive Companion Overhaul
- **🔴 Laser Pointer Chase Mode**:
  - Drag your finger across the display floor to beam an active laser dot.
  - DigiCat stalks the dot using real-time inverse kinematics, crouches into hunting posture, and leaps into a playful pounce when in strike range.
- **🐟 Fish Treat Toss Mini-Game**:
  - Tap the fish bowl or treat button to toss crunchy fish treats across the airfield runway deck.
  - Treats follow realistic parabolic ballistic trajectories, bounce against the asphalt surface, and trigger delighted munching sequences.
- **🐾 Multi-Zone Tactile Petting**:
  - Touch-sensitive interaction zones mapped across DigiCat's body:
    - **Chin Scratching**: Triggers closed-eye bliss and affectionate audio purrs.
    - **Ear Rubs**: Prompts ear wiggles and leaning body movements.
    - **Belly Tickling**: Playful rolling paws and cheerful chirps.
    - **Tail Swish**: Dynamic swishing and playful responses.
- **🍃 Living Ambient Ecosystem**:
  - Procedural background environment with fluttering butterflies, horizon birds, swirling autumn leaves, and floating atmospheric dust motes.
- **📜 Typewriter Dialogue Engine**:
  - Retro animated character-by-character typewriter text engine with word wrapping for Gemini AI live air traffic thoughts and built-in dialogues.
- **🕒 Clock Screen Peek Cameo**:
  - Ambient DigiCat cameo softly resting on the lower bezel of the Luxury Astronomical Chronograph with synchronized rhythmic breathing and responsive eye tracking.

### 🛡️ Network Stack Hardening & TLS Concurrency
- **Non-Blocking Multi-Step YouTube Client**:
  - Restructured YouTube Channel Analytics into a 3-stage non-blocking state machine with PSRAM JSON document allocation, eliminating Core 0 TLS lockups and watchdog timeouts.
- **Cross-Core Mutex Synchronization**:
  - Added dedicated recursive mutexes (`s_mtxYouTube`, `s_txtMtx`) and RAII text buffer locks protecting shared network scratch buffers between Core 0 async workers and Core 1 UI tasks.
- **Clock Complication Live Telemetry Updates**:
  - Resolved complication freezes by enabling Core 0 background polling for Klipper 3D printer, YouTube, Market, Aircraft, and Weather complications directly on the Clock screen—no screen cycling needed.
- **Dispatch Ladder Prioritization**:
  - Elevated active screen complications ahead of idle background polling cycles, guaranteeing immediate telemetry polling for displayed subdial apertures.
- **MbedTLS Socket Teardown Cleanliness**:
  - Eliminated persistent `MBEDTLS_ERR_NET_CONN_RESET (-76)` console error spam by ensuring sockets are not probed via `available()` once remote HTTP disconnection is established.
- **Serial Heartbeat Throttling**:
  - Extended heartbeat interval to 15 s and raised watchdog stall thresholds to eliminate false alarms during heavy graphical pre-rendering.

---

## 📦 Attached Firmware Binaries

| Binary File | Size | Purpose |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v3.8.0-factory.bin` | ~4.7 MB | Complete flash binary containing bootloader, partition table, boot app, and firmware at offset `0x0000`. Use for initial flashing via esptool or Web Flasher. |
| `MeteoPlaneRadar-v3.8.0-ota.bin` | ~4.7 MB | App-only firmware image for on-device GitHub OTA or manual web upload at `/api/ota`. |
| `bootloader.bin` | ~20 KB | Secondary bootloader binary at offset `0x0000`. |
| `partitions.bin` | ~3 KB | Custom flash partition table at offset `0x8000`. |

---

## 🚀 How to Flash / Update

### On-Device OTA Update
Navigate to **Settings** (`Screen 11`) on the device or open the **Web Portal** at `http://<device-ip>/` and click **Check for Updates** under the System card.

### Web Browser Flashing (Chrome / Edge)
1. Connect the ESP32-S3 board to your PC via USB-C.
2. Open the [ESP Web Flasher](https://espressif.github.io/esptool-js/).
3. Load `MeteoPlaneRadar-v3.8.0-factory.bin` at offset `0x0000`.
4. Click **Program** and reboot.

### Command Line Flashing
```bash
esptool.py --chip esp32s3 -p COMx -b 921600 write_flash 0x0000 MeteoPlaneRadar-v3.8.0-factory.bin
```
