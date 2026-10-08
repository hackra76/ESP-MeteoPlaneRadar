# MeteoPlaneRadar v3.0.0

## Changed
- **LVGL 9 Rendering Engine:** Completely rewritten rendering pipeline. The original Arduino_GFX was replaced with LVGL 9 featuring double buffering (2x 480x480 PSRAM) for hardware-accelerated smooth animations and anti-aliasing via DMA.
- **PSRAM Memory Management:** Complete overhaul of PNG memory allocation to prevent 'SHMU Error' crashes when zooming out on the radar by recalculating and freeing all frames simultaneously, effectively preventing PSRAM fragmentation.
- **Touch Controls Overhaul:** Rewritten touch logic for flawless swipe gestures and accurate double-tap detection.

## Added
- **Sonar Screen Mode:** Added a brand new tactical radar screen in the style of a green phosphor sonar, displaying live ADS-B data with dynamically fading trails.
- **Quick Control Menu:** A new pull-down menu accessible from any screen allowing quick access to settings.
