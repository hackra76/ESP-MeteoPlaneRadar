# Release v2.0.2: Runway Deck, Dynamic Weather Effects & Extended Night Wake

This release brings physical grounding, real-time live weather effects, interactive accessories, and natural sleep cycles to the DigiCat Companion on MeteoPlaneRadar!

---

### 🌟 New Features & Enhancements

#### 1. Airfield Runway Deck & 3D Cast Shadow
- **Runway Asphalt Platform:** Placed at $Y = 342..382$ with subtle centerline dash markings and perimeter runway beacon lights (left amber, right cyan).
- **Physical Cast Shadow:** Soft feathered oval drop shadow under DigiCat's paws. During `CAT_STATE_JUMP`, the shadow remains grounded on the deck and shrinks/fades with jump apex, providing authentic 3D spatial depth.
- **Weather-Adaptive Surface:** Adapts between clean dark slate asphalt, wet reflective surface during rain, and snow accumulation specks during snowfall.

#### 2. Live Weather Effects & Night Stars
- **Real-Time Weather Integration:** Syncs with both live radar nowcasting (`PrecipTracker`) and Open-Meteo current condition codes:
  - **Clear Night Skies:** 18 glittering stars twinkling across the perimeter sky when night mode is active.
  - **Rain:** 22 dynamic slanted rain streaks that tilt with IMU accelerometer orientation, complete with ground impact splash ripples on the runway curb.
  - **Thunderstorms:** Fast heavy rain streaks with periodic atmospheric lightning flashes (70ms sky illumination and branching discharge bolt).
  - **Snowfall:** 22 drifting snowflakes ($1\times1$ dots and $3\times3$ '+' cross flakes) with sinusoidal wind drift and IMU tilt responsiveness.

#### 3. Weather-Adaptive DigiCat Accessories
- **Rain & Thunderstorms:** Bright yellow and red umbrella protecting DigiCat (pitched beside him while sleeping with bouncing droplets; held in paw while walking or sitting).
- **Snow & Cold Weather ($\le 2^\circ\text{C}$):** Cozy red & cream knit winter scarf with a fluttering tassel tail around DigiCat's neck.
- **Clear Weather:** Retro aviator goggles perched on DigiCat's forehead with leather strap and tinted cyan lenses.

#### 4. Extended Night Wake & Natural Gradual Sleep
- **2-Minute Wake Window:** Interacting with DigiCat at night (tapping, stroking, feeding, or calling back) keeps him active and playful for 2 full minutes.
- **Full Daytime Autonomy:** DigiCat patrols, jumps, grooms, stretches, watches overhead flights, and explores the hangar during the wake period.
- **Gradual Sleep Wind-Down:** During the last 20 seconds of the wake window without user interaction, DigiCat yawns, stretches, and slows down into heavy, prolonged blinks before peacefully curling into a sleeping loaf with a quiet goodnight purr.
- **Wake-Up Stretch:** Tapping a sleeping pet triggers a wake-up stretch, chirping purr, and greeting dialogue.
- **Restored Side Entrance Animation:** Opening the pet screen while awake triggers the smooth side-entrance trot from offscreen.

---

### 📦 Installation & Updating
- **OTA Update:** Upload `MeteoPlaneRadar-v2.0.2-ota.bin` via the web dashboard at `http://<device-ip>/`.
- **Factory Flash:** Flash `MeteoPlaneRadar-v2.0.2-factory.bin` starting at address `0x0000` via esptool or PlatformIO.
