# Release v2.0.4: Non-Destructive Weather Accessories & Flight Tracking Calibration

This maintenance release refines the DigiCat companion graphics engine, eliminates overlapping clipping masks, and calibrates overhead flight reactions!

---

### 🌟 Key Fixes & Improvements

#### 1. Non-Destructive Weather Accessory Rendering
- **Scanline Dome Rendering (`drawUmbrellaCanopy`):** Replaced destructive `fillCircle` + `fillRect(..., C_BLACK)` trimming with a pure upper-semicircle scanline loop ($dy \le 0$). Zero pixels are drawn below the canopy rim, completely eliminating the black rectangle that previously cut into DigiCat's head, ears, and fur during rain.
- **Sheltered Sleeping Loaf Parasol:** In wet weather, a pitched garden/beach parasol canopy ($cx = \text{catX} + 24, cy = 265, r = 26$) vaults gracefully above the sleeping loaf with its pole grounded beside the cat on the runway deck.
- **Acrobatic Accessory Suppression:** Holding the umbrella is cleanly suppressed during acrobatics (`JUMP`, `SWAT`, `EATING`, `AWAY`) to prevent unnatural sprite overlap.
- **Clean Runway Cast Shadow:** Removed inner `0x0000` pitch-black cutouts from `drawCatCastShadow`, ensuring the shadow is a soft darkening footprint on the runway tarmac without punching holes in the pavement.

#### 2. Flight Tracking & Swatting Calibration
- **Proximity Limit ($\le 10\text{ km}$):** Real-time aircraft simulation and paw swatting on the pet screen are strictly reserved for low-altitude/close overhead passes ($\le 10\text{ km}$), preventing the pet from being constantly distracted by distant traffic.
- **Natural Cooldown Pause (`s_planeChasePauseUntilMs`):** Added an autonomous cooldown interval between swat and chase routines, allowing DigiCat to alternate between playful jumps, idling, grooming, and stretching rather than looping indefinitely.
- **Distinct Radar Thoughts ($10–40\text{ km}$):** DigiCat now displays distinct radar tracking remarks for distant flights instead of attempting to swat at them through the air.

---

### 📦 Installation & Updating
- **OTA Update:** Upload `MeteoPlaneRadar-v2.0.4-ota.bin` via the web dashboard at `http://<device-ip>/`.
- **Factory Flash:** Flash `MeteoPlaneRadar-v2.0.4-factory.bin` starting at address `0x0000` via esptool or PlatformIO.
