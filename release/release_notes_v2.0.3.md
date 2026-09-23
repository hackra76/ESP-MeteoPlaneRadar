# Release v2.0.3: Overhead Aircraft Tracking, Swatting & Flight Chasing Animations

This release introduces live overhead aircraft tracking, interactive swatting & jumping animations, and UI typography enhancements to the DigiCat Companion!

---

### 🌟 New Features & Enhancements

#### 1. Live Overhead Aircraft Tracking on Pet Screen
- **Gliding Pixel Art Aircraft:** When an aircraft is detected in the vicinity via ADS-B/radar, a custom $28\times14$ pixel art twin-engine jet (rendered at $2\times$ scale, $56\times28\text{ px}$) glides gracefully across the sky band ($Y = 175..205$).
- **Dual Contrail Vapor Trails:** Twin engine trails drift and expand horizontally behind the aircraft with authentic aerodynamic dissipation.
- **Navigation Strobes & Flight Badge:** Alternating red/green navigation strobe beacons flash on the wingtips, accompanied by a callsign and distance pill badge (e.g., `DLH123 · 8km`).
- **Interactive Jet Tap:** Tapping the aircraft directly on the touchscreen causes the jet to perform an evasive vertical dash with an avionics chirp, provoking DigiCat to jump and swipe at it.

#### 2. Aircraft Chasing, Swatting & Scratching Animations
- **Runway Chasing:** DigiCat tracks the plane's flight path, turning and sprinting along the runway deck to stay directly beneath the aircraft.
- **8-Frame Swatting Sprite Animation (`CAT_STATE_SWAT`):** DigiCat rises onto hind legs, stretching paws upward and performing playful alternating paw swipes and scratching motions at the plane.
- **Claw Swipe Sparks & Evasive Hops:** At the swipe apex, bright claw scratch sparks appear beneath the jet, prompting the aircraft to perform an evasive hop with an audible chirp.
- **High-Altitude Pounces (`CAT_STATE_JUMP`):** DigiCat leaps straight up towards the overhead jet with dynamic shadow scaling.

#### 3. Speech Bubble & Accessory Enhancements
- **Perfect Speech Bubble Text Centering:** Improved text layout engine computes vertical text block bounds and perfectly centers text horizontally and vertically, making dialogue crisp and readable.
- **Accessory Refinement:** Removed sunglasses during sunny weather for a clean, unobstructed view of DigiCat's facial expressions.
- **Context-Aware Dialogue & Gemini AI Prompt:** Updated offline dialogue and Gemini LLM system prompt to include aircraft chasing, swatting, and runway antics.

---

### 📦 Installation & Updating
- **OTA Update:** Upload `MeteoPlaneRadar-v2.0.3-ota.bin` via the web dashboard at `http://<device-ip>/`.
- **Factory Flash:** Flash `MeteoPlaneRadar-v2.0.3-factory.bin` starting at address `0x0000` via esptool or PlatformIO.
