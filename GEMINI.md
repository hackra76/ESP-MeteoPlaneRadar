# Workspace Guidelines

## 1. Execution & Build Protocol
- **Language**: Always communicate in English with the user.
- **Auto-Accept Changes**: Automatically implement, edit, and apply code changes directly without pausing to ask for approval. Only pause with a single-line query if an explicit architectural tradeoff strictly blocks execution.
- **Verification**: Proactively run `pio run` to verify and ensure zero compile regressions.
- **Flashing Firmware**: Always use `$env:PYTHONIOENCODING="utf8"; pio run -t upload` when flashing the firmware to prevent Python UnicodeEncodeError crashes in the Windows console.
- **Release Documentation Protocol**: Whenever asked to make a new release, always update the README files (`README.md`, `README_EN.md`, `README_SK.md`) and wiki pages first to keep them completely accurate before publishing the release.
- **Local Memory Persistence**: Mandatory protocol: after every change, fix, or implementation, immediately store the exact architectural and behavioral changes in local memory (local-memory MCP), and continuously consult the knowledge graph as the foundation for all future activities.

## 2. Token Efficiency & Reasoning
- **Prompt Comprehension Acknowledgment**: Whenever the user gives a command or asks a question, always begin the response by stating concisely in 1–2 sentences what is understood from their request before proceeding with execution or explanation.
- **Extreme Conciseness**: Skip polite greetings, conversational filler, and unnecessary commentary. Output actions and conclusions directly.
- **No Self-Evident Commentary**: Do not explain code that speaks for itself. Only provide commentary if a non-obvious design trade-off, edge case, or bug workaround is involved.
- **Internal Analysis**: Prioritize deep internal chain-of-thought, static analysis, and edge-case verification before executing changes. Output conclusions and action plans directly without narrating step-by-step thinking.
- **Native Inspection**: Rely on native LSP/workspace tools to inspect definitions, signatures, and types rather than asking the user to paste context.

## 3. Output Format
- **Code Changes**: Apply directly to files on disk. In chat, output only clickable file links with concise bullet points of modified functions/symbols (do not reprint diffs or file boilerplate).
- **Execution Steps**: Bulleted commands and verification status only.
- **Questions**: Single-line queries only when strictly blocking.

## 4. Anti-Slop Industrial UI Design Protocol (Strictly Mandatory)
- **Zero AI-Slop Tropes**:
  - NO slate/zinc blue-gray dark mode backgrounds (`#0f172a`, `#1e293b`, `#111827`).
  - NO purple, indigo, violet, or cyan neon gradients, glowing orbs, or radial background blurs.
  - NO uniform Bento grids of floating cards with soft rounded corners (`border-radius` between 8px and 20px is strictly banned) and subtle drop shadows.
  - NO glassmorphism (`backdrop-filter: blur`) or translucent white borders (`rgba(255,255,255,0.1)`).
  - NO generic pill badges (`border-radius: 9999px`) or generic floating lift hovers (`transform: translateY(-2px)`).
  - NO Inter, Roboto, Open Sans, or generic system sans-serif for data readouts.
- **Aesthetic Standard ("Tactile Field Instrument" / Braun / Teenage Engineering / Swiss Grid)**:
  - **Surfaces & Palette**: Matte carbon (`#121211`), chassis plate (`#171716`), structural hairlines (`#2A2A28`), primary off-white ink (`#F4F3EF`), strictly rationed functional accents: International Signal Orange (`#FF4F00`), Lab Ochre (`#E5A93C`), Phosphor Green (`#00D062`).
  - **Structural Geometry**: Exposed Swiss architectural grid (`gap: 0`, `1px solid #2A2A28` dividers, sharp 0px–2px chamfered corners, corner crosshairs `+` at junctions). Dense technical sidebar paired with asymmetric telemetry canvas.
  - **Typography**: Grotesque technical header (`Space Grotesk`) paired with strict tabular monospace (`JetBrains Mono`, `tabular-nums`) for all numbers, coordinates, voltages, and logs. Extreme scale contrast (10px uppercase tracked-out labels vs 36px–48px hero numbers).
  - **Controls & Graphics**: Segmented mechanical rocker switches, rotary SVG arc gauges with calibrated tick marks, phosphor LED matrix dots, and crisp geometric 1.5px inline SVGs.

