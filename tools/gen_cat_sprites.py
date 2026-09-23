#!/usr/bin/env python3
"""
gen_cat_sprites.py
------------------
Extracts ginger-cat animations from PACK/cat 2 (64x64).png,
optionally interpolates frames, and writes MeteoPlaneRadar/CatSpritesHiRes.h.

Sprite sheet layout: 14 columns x 72 rows, each cell = 64x64 RGBA.
Row numbers are 1-indexed (matching activities.ini).
"""

import os, sys
from PIL import Image

PACK_DIR   = "PACK"
OUT_FILE   = "MeteoPlaneRadar/CatSpritesHiRes.h"
SPRITE_W   = 64
SPRITE_H   = 64
COLS       = 14          # frames per row in the sheet

# ── Animation definitions ────────────────────────────────────────────────
# Each entry: (c_name, row_1idx, target_frames, interpolate, description)
# target_frames: desired frame count after interpolation
# interpolate: True = blend consecutive frames to reach target_frames
ANIMATIONS = [
    # name                       row  tgt  interp  description
    # ── Idle sub-animations ────────────────────────────────────────────────
    ("cat_idle",                 20, 10,  True, "Tail wag sit front – primary idle"),
    ("cat_idle_lick",            13,  8, False, "Lick paw sitting front – groom sub-anim"),
    ("cat_idle_lick_lie",        14,  8, False, "Lick paw lying front – lying groom"),
    ("cat_idle_meow",            15,  6, False, "Meow sit front – idle sub-anim"),
    ("cat_idle_meow_lie",        16,  6, False, "Meow lie front – idle sub-anim"),
    ("cat_idle_meow_stand",      17,  6, False, "Meow stand front – idle sub-anim"),
    ("cat_idle_scratch",         18,  6, False, "Scratch sit left paw – idle fidget"),
    ("cat_idle_scratch_right",   19,  6, False, "Scratch sit right paw – idle fidget"),
    ("cat_yawn",                 44,  6, False, "Yawn sit front – drowsy / pre-sleep"),
    # ── Reactive ──────────────────────────────────────────────────────────
    ("cat_happy",                20, 10,  True, "Tail wag sit front – happy / petted"),
    ("cat_happy_stand_front",    24, 10,  True, "Tail wag stand front – standing happy"),
    ("cat_happy_stand_right",    27, 10,  True, "Tail wag stand right – excited happy"),
    ("cat_watch_plane",          23,  6, False, "Tail wag sit right – watching sky"),
    ("cat_hiss",                 62,  6, False, "Hiss front right – scared / defensive"),
    # ── Locomotion ────────────────────────────────────────────────────────
    ("cat_walk",                  6, 12,  True, "Walk right – patrol / leaving"),
    ("cat_run",                  11,  8, False, "Running right – entering from left"),
    ("cat_run_left",             12,  8, False, "Running left – entering from right"),
    # ── Sleep variants (5 styles × left/right) ────────────────────────────
    ("cat_sleep_1_left",         45,  8,  True, "Sleep 1 left front – breathing loop"),
    ("cat_sleep_1_right",        46,  8,  True, "Sleep 1 right front – breathing loop"),
    ("cat_sleep_2_left",         49,  8,  True, "Sleep 2 left front – breathing loop"),
    ("cat_sleep_2_right",        50,  8,  True, "Sleep 2 right front – breathing loop"),
    ("cat_sleep_3_left",         51,  8,  True, "Sleep 3 left front – breathing loop"),
    ("cat_sleep_3_right",        52,  8,  True, "Sleep 3 right front – breathing loop"),
    ("cat_sleep_4_left",         53,  8,  True, "Sleep 4 left front – breathing loop"),
    ("cat_sleep_4_right",        54,  8,  True, "Sleep 4 right front – breathing loop"),
    ("cat_sleep_5_left",         55,  8,  True, "Sleep 5 left front – breathing loop"),
    ("cat_sleep_5_right",        56,  8,  True, "Sleep 5 right front – breathing loop"),
    # ── Action states ─────────────────────────────────────────────────────
    ("cat_eat",                  60,  8, False, "Eat food stand right – eating"),
    ("cat_eat_front",            57,  8, False, "Eat food stand front – eating face-on"),
    ("cat_swat",                 36,  8, False, "Right paw swipe stand right – swat at plane"),
    ("cat_swat_sit_right",       37,  8, False, "Right paw swipe sit front – sit swat"),
    ("cat_swat_sit_left",        38,  8, False, "Left paw swipe sit front – sit swat"),
    ("cat_jump",                 65,  8, False, "Jump right – jump state"),
    # ── Physics states ────────────────────────────────────────────────────
    ("cat_cling",                66,  6, False, "On hind legs – cling to bezel rim"),
    ("cat_slide",                29,  8, False, "Tail wag lie right – slide right"),
    ("cat_slide_left",           28,  8, False, "Tail wag lie left – slide left"),
]

# ── Helpers ──────────────────────────────────────────────────────────────

def rgba_to_rgb565(r, g, b, a):
    if a < 128:
        return 0x0000
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    val = (r5 << 11) | (g6 << 5) | b5
    if val == 0x0000:
        val = 0x0001  # avoid collision with transparent sentinel
    return val

def extract_row_frames(sheet, row_1idx):
    row0 = row_1idx - 1
    y0   = row0 * SPRITE_H
    frames = []
    for col in range(COLS):
        x0   = col * SPRITE_W
        cell = sheet.crop((x0, y0, x0 + SPRITE_W, y0 + SPRITE_H))
        pix  = cell.load()
        has_content = any(pix[x, y][3] > 10
                          for y in range(SPRITE_H) for x in range(SPRITE_W))
        if has_content:
            frames.append(cell)
    return frames

def lerp_image(a, b, t):
    import numpy as np
    fa = np.array(a, dtype=float)
    fb = np.array(b, dtype=float)
    mask = (fa[:,:,3:4] > 10) | (fb[:,:,3:4] > 10)
    blended       = fa * (1 - t) + fb * t
    blended[:,:,3:4] = np.where(mask, np.maximum(fa[:,:,3:4], fb[:,:,3:4]), 0)
    return Image.fromarray(blended.clip(0, 255).astype('uint8'), 'RGBA')

def interpolate_frames(frames, target_count):
    if len(frames) >= target_count:
        return frames[:target_count]
    src_n  = len(frames)
    result = []
    for i in range(target_count):
        pos = i * (src_n - 1) / (target_count - 1)
        lo  = int(pos)
        hi  = min(lo + 1, src_n - 1)
        t   = pos - lo
        result.append(lerp_image(frames[lo], frames[hi], t) if abs(t) > 1e-9 else frames[lo])
    return result

def frame_to_rgb565(img):
    rgba = img.convert("RGBA")
    pix  = rgba.load()
    return [rgba_to_rgb565(*pix[x, y]) for y in range(SPRITE_H) for x in range(SPRITE_W)]

def format_c_row(vals, cols=16):
    lines = []
    for i in range(0, len(vals), cols):
        chunk = vals[i:i+cols]
        lines.append("    " + ", ".join(f"0x{v:04X}" for v in chunk) + ",")
    return "\n".join(lines)

# ── Main ─────────────────────────────────────────────────────────────────

def main():
    cat_file = next(
        (os.path.join(PACK_DIR, fn) for fn in os.listdir(PACK_DIR)
         if fn.startswith("cat 2") and fn.endswith(".png")), None)
    if not cat_file:
        sys.exit("ERROR: cat 2 *.png not found in PACK/")

    print(f"Loading: {cat_file}")
    sheet = Image.open(cat_file).convert("RGBA")
    print(f"  Size: {sheet.size}  ({sheet.size[0]//SPRITE_W} cols x {sheet.size[1]//SPRITE_H} rows)")

    anim_frames = {}
    total_frames = 0

    for (c_name, row, target, do_interp, desc) in ANIMATIONS:
        raw = extract_row_frames(sheet, row)
        if not raw:
            print(f"  SKIP  row {row:2d} {c_name} – no content")
            continue
        frames = interpolate_frames(raw, target) if (do_interp and len(raw) < target) else raw[:target]
        print(f"  Row {row:2d}  {c_name:25s}  {len(raw):2d} raw -> {len(frames)} frames")
        anim_frames[c_name] = [frame_to_rgb565(f) for f in frames]
        total_frames += len(frames)

    total_bytes = total_frames * SPRITE_W * SPRITE_H * 2
    print(f"\nTotal frames : {total_frames}")
    print(f"PROGMEM size : {total_bytes:,} bytes  ({total_bytes/1024:.1f} KB)")
    print(f"Flash budget : {total_bytes/1024:.1f} KB / 6144 KB  ({total_bytes/(6144*1024)*100:.1f}%)")

    with open(OUT_FILE, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("// =============================================================================\n")
        f.write("//  MeteoPlaneRadar\n")
        f.write("//  CatSpritesHiRes.h  -  Ginger cat sprite animations (64x64 RGB565, PROGMEM)\n")
        f.write("//\n")
        f.write("//  AUTO-GENERATED by tools/gen_cat_sprites.py  -- do not edit by hand.\n")
        f.write("//  Source: PACK/cat 2 (64x64).png + activities.ini\n")
        f.write("// =============================================================================\n")
        f.write("#pragma once\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define CAT_HIRES_W  {SPRITE_W}\n")
        f.write(f"#define CAT_HIRES_H  {SPRITE_H}\n\n")

        for (c_name, row, target, do_interp, desc) in ANIMATIONS:
            if c_name not in anim_frames:
                continue
            frames = anim_frames[c_name]
            define = f"HIRES_{c_name.upper()}_FRAMES"
            f.write(f"// Row {row}: {desc}\n")
            f.write(f"#define {define} {len(frames)}\n")
            f.write(f"const uint16_t {c_name}[{len(frames)}][{SPRITE_W * SPRITE_H}] PROGMEM = {{\n")
            for fi, pix in enumerate(frames):
                f.write(f"  {{ // frame {fi}\n")
                f.write(format_c_row(pix))
                f.write("\n  },\n")
            f.write("};\n\n")

    print(f"\nWrote: {OUT_FILE}")

if __name__ == "__main__":
    main()
