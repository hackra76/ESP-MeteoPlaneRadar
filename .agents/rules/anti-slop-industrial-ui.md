# Anti-Slop Industrial UI Design Protocol

## 1. The Anti-Slop Ban List (Strictly Forbidden)
- **NO slate/zinc blue-gray dark mode backgrounds**: Never use `#0f172a`, `#1e293b`, `#111827`, or blue-tinted dark themes.
- **NO purple, indigo, violet, or cyan neon gradients**: No glowing orbs, radial background blurs, or ambient neon backdrops.
- **NO uniform Bento grids of floating cards**: Cards with soft rounded corners (`border-radius` between `8px` and `20px`) and subtle drop shadows are strictly banned.
- **NO glassmorphism**: Never use `backdrop-filter: blur(...)` or translucent white hairline borders (`rgba(255,255,255,0.1)`).
- **NO generic pill badges**: Never use `border-radius: 9999px` badges or generic floating lift hovers (`transform: translateY(-2px)`).
- **NO generic system sans-serif for numbers**: Never use Inter, Roboto, Open Sans, or generic sans for data readouts, coordinates, IPs, voltages, or telemetry.

## 2. Aesthetic Direction: "Tactile Field Instrument" (Braun / Teenage Engineering / Swiss Grid)
- **Surface & Palette**:
  - Base surfaces: Matte deep carbon (`#121211`), milled chassis plate (`#171716`), recessed bay (`#0B0B0A`).
  - Structural dividers: Hairline dividers in `#2A2A28` (1px solid).
  - Primary ink: Bone / off-white (`#F4F3EF`).
  - Rationed functional accents: International Signal Orange (`#FF4F00`) for active/warning states, Lab Ochre (`#E5A93C`) for standby/secondary channels, Phosphor Green (`#00D062`) for live telemetry/carrier lock.
- **Structural Geometry**:
  - Exposed Swiss architectural grid with zero gap (`gap: 0`, `1px solid #2A2A28` borders between cells).
  - Sharp 0px–2px chamfered corners.
  - Corner crosshairs (`+`) at grid intersections and cell perimeters.
  - Asymmetric layout pairing a dense fixed-width technical sidebar with a multi-pane telemetry canvas.
- **Typography & Scale Contrast**:
  - Technical grotesque headers: `Space Grotesk`, `Chakra Petch`, or bold geometric fallback.
  - Strict tabular monospace: `JetBrains Mono`, `IBM Plex Mono`, or `monospace` with `font-variant-numeric: tabular-nums` for all numbers, voltages, and logs.
  - Extreme scale contrast: Micro uppercase tracked labels (`10px`, `letter-spacing: 0.12em`) paired with raw oversized hero metrics (`36px–48px`).
- **Tactile Instrument Controls & SVGs**:
  - Segmented mechanical rocker switches with recessed channels and Signal Orange indicator slides.
  - Rotary-style SVG circular arc gauges with precision tick marks (1.5px stroke).
  - LED matrix status dots (square phosphor diodes).
  - Crisp geometric inline SVGs (stroke-width: 1.5px).
