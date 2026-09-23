# Release v2.0.0: Handcrafted Pixel Art DigiCat & Autonomous AI Companion

Welcome to **v2.0.0** of **ESP-MeteoPlaneRadar**! This major milestone introduces a complete visual and behavioral overhaul of your radar flight companion: **DigiCat**. Procedural geometry has been replaced with 32 handcrafted pixel art frames, high-speed span blitting, full multi-directional walking and jumping physics, an autonomous feline AI brain with free will, and full trilingual localization (EN / SK / CZ).

---

## What's New in v2.0.0

### 🐱 Handcrafted 32-Frame Pixel Art Companion
- **High-Performance 16-Color Retro Palette:** 32 bespoke 64×64 pixel art frames scaled 2× to 128×128 px on display, rendered via run-length span blitter (<0.5 ms per frame).
- **Rich Animation Cycles:**
  - **Walk Cycle** (4 frames): Dual-direction walking (`flipX`) with paw movement, ear sway, and tail motion.
  - **Sitting Idle** (4 frames): Natural breathing, ear twitches, and blinking.
  - **Happy Purring** (4 frames): Blissful closed eyes, blushing cheeks, floating hearts, and acoustic purr.
  - **Sky Watching** (2 frames): Real-time aircraft tracking overhead.
  - **Eating Treats** (4 frames): Fish crackers, crumbs, and satisfied licking.
  - **Sleeping Loaf** (4 frames): Curled warm loaf with rhythmic breathing and floating *Zzz*.
  - **Jumping & Pouncing** (4 frames): Pre-jump crouch, parabolic flight arc (-32 px), and springy landing.
  - **Grooming Routine** (4 frames): Paw lifting, pad licking, cheek wash, and ear grooming.
  - **Yoga Stretch** (2 frames): Downward cat stretch and high rainbow arched back.

### 🧠 Autonomous AI Brain with Free Will
- **Multi-Directional Entrance:** DigiCat randomly trots in from the left or right when opening the Pet Drawer.
- **Dynamic Deck Exploration:** Wanders autonomously across the flight radar deck, leaps after invisible moths, stretches, and grooms itself.
- **Airfield Excursions:** DigiCat can choose to step out off-screen to inspect the airfield or chase butterflies.
- **Instant Return on Call:** Tapping the screen or pet action buttons summons DigiCat back immediately at a sprint with a cheerful meow.
- **Non-blocking Flight Tracking:** Watching nearby flights (within 45 km) is now a dynamic 3.5s action rather than a freeze, allowing DigiCat to stay lively and animated at all times.

### 🌐 Dedicated Pet Web Configuration & Trilingual Support
- **Dedicated Web UI Tab:** Clean, separate configuration tab for DigiCat with API key setup guide and touch gesture reference.
- **Full Trilingual Localization:** English, Slovak, and Czech thought bubbles, rank titles, and interface text.

---

## 📦 Firmware Binaries Included

| File | Type | Description |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v2.0.0-factory.bin` | Full Flash | Complete firmware image for clean flashing via esptool / ESP Web Flasher (`0x00000`) |
| `MeteoPlaneRadar-v2.0.0-ota.bin` | OTA Update | Direct firmware update via Web UI at `http://<device-ip>/update` |

---

### Flash Commands:
- **Web UI:** Upload `MeteoPlaneRadar-v2.0.0-ota.bin` directly on the firmware update page.
- **esptool (Full Factory Flash):**
  ```bash
  esptool.py --chip esp32s3 --baud 921600 write_flash 0x00000 MeteoPlaneRadar-v2.0.0-factory.bin
  ```
