#!/usr/bin/env python3
"""
gen_digicat_media.py
Extracts key frames from PACK/cat 2 (64x64).png, scales 3x,
and produces:
  - docs/media/digicat_showcase.png  (composite strip)
  - docs/media/digicat_walk.gif
  - docs/media/digicat_jump.gif
  - docs/media/digicat_groom.gif
  - docs/media/digicat_stretch.gif
  - docs/media/digicat_happy.gif
  - docs/media/digicat_eat.gif
  - docs/media/digicat_sleep.gif
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

PACK_DIR = "PACK"
MEDIA_DIR = "docs/media"
SPRITE_W = 64
SPRITE_H = 64
COLS     = 14
SCALE    = 3   # 3x -> 192x192 per cell

def load_sheet():
    cat_file = next(
        (os.path.join(PACK_DIR, fn) for fn in os.listdir(PACK_DIR)
         if fn.startswith("cat 2") and fn.endswith(".png")), None)
    if not cat_file:
        sys.exit("ERROR: cat 2 *.png not found in PACK/")
    print(f"Loading: {cat_file}")
    return Image.open(cat_file).convert("RGBA")

def get_frame(sheet, row_1idx, col=0):
    """Extract a single frame (row is 1-indexed)."""
    row0 = row_1idx - 1
    x0 = col * SPRITE_W
    y0 = row0 * SPRITE_H
    return sheet.crop((x0, y0, x0 + SPRITE_W, y0 + SPRITE_H))

def get_row_frames(sheet, row_1idx):
    """Extract all non-empty frames from a row."""
    frames = []
    for col in range(COLS):
        cell = get_frame(sheet, row_1idx, col)
        pix = cell.load()
        if any(pix[x, y][3] > 10 for y in range(SPRITE_H) for x in range(SPRITE_W)):
            frames.append(cell)
    return frames

def scale3(img):
    """Scale frame to 3x with checkerboard background for transparency."""
    # Create white background (for showcase)
    bg = Image.new("RGBA", (SPRITE_W * SCALE, SPRITE_H * SCALE), (255, 255, 255, 255))
    scaled = img.resize((SPRITE_W * SCALE, SPRITE_H * SCALE), Image.NEAREST)
    bg.paste(scaled, (0, 0), scaled)
    return bg.convert("RGBA")

def frames_on_bg(frames, bg_color=(240, 240, 240)):
    """Scale all frames with given background."""
    result = []
    for f in frames:
        bg = Image.new("RGBA", (SPRITE_W * SCALE, SPRITE_H * SCALE), bg_color + (255,))
        scaled = f.resize((SPRITE_W * SCALE, SPRITE_H * SCALE), Image.NEAREST)
        bg.paste(scaled, (0, 0), scaled)
        result.append(bg.convert("RGB"))
    return result

sheet = load_sheet()
os.makedirs(MEDIA_DIR, exist_ok=True)

# ── Showcase: select one good frame per animation ────────────────────────
showcase_specs = [
    # (row, col, label)
    (20, 2,  "Idle"),
    (6,  2,  "Walk"),
    (65, 2,  "Jump"),
    (13, 3,  "Groom"),
    (7,  3,  "Stretch"),
    (20, 0,  "Happy"),
    (37, 3,  "Swat"),
    (46, 1,  "Sleep"),
]

BG = (30, 30, 40, 255)   # dark background for showcase
CELL_W = SPRITE_W * SCALE
CELL_H = SPRITE_H * SCALE
PAD    = 12
LABEL_H = 28
COLS_SH = len(showcase_specs)
IMG_W = COLS_SH * (CELL_W + PAD) + PAD
IMG_H = CELL_H + LABEL_H + PAD * 2

showcase = Image.new("RGBA", (IMG_W, IMG_H), BG)
try:
    font = ImageFont.truetype("arial.ttf", 16)
except:
    font = ImageFont.load_default()

draw = ImageDraw.Draw(showcase)

for i, (row, col, label) in enumerate(showcase_specs):
    frame = get_frame(sheet, row, col)
    scaled = frame.resize((CELL_W, CELL_H), Image.NEAREST)
    x = PAD + i * (CELL_W + PAD)
    y = PAD
    showcase.paste(scaled, (x, y), scaled)
    # label
    bbox = draw.textbbox((0, 0), label, font=font)
    tw = bbox[2] - bbox[0]
    draw.text((x + (CELL_W - tw) // 2, y + CELL_H + 4), label, fill=(200, 200, 220), font=font)

showcase_path = os.path.join(MEDIA_DIR, "digicat_showcase.png")
showcase.convert("RGB").save(showcase_path)
print(f"Saved: {showcase_path}  ({IMG_W}x{IMG_H})")

# ── Individual GIFs ───────────────────────────────────────────────────────
gif_specs = [
    ("digicat_walk.gif",    6,  75),   # walk right
    ("digicat_jump.gif",    65, 90),   # jump right
    ("digicat_groom.gif",   13, 100),  # lick paw sit
    ("digicat_stretch.gif", 7,  100),  # going to sleep / stretch
    ("digicat_happy.gif",   20, 90),   # tail wag sit
    ("digicat_eat.gif",     57, 100),  # eat front
    ("digicat_sleep.gif",   46, 200),  # sleep 1 right (slow breathing)
]

for fname, row, frame_ms in gif_specs:
    raw = get_row_frames(sheet, row)
    if not raw:
        print(f"  SKIP {fname} - no frames in row {row}")
        continue
    gif_frames = frames_on_bg(raw, bg_color=(30, 30, 40))
    path = os.path.join(MEDIA_DIR, fname)
    gif_frames[0].save(
        path, save_all=True, append_images=gif_frames[1:],
        duration=frame_ms, loop=0, optimize=False)
    print(f"Saved: {path}  ({len(gif_frames)} frames @ {frame_ms}ms)")

print("\nDone.")
