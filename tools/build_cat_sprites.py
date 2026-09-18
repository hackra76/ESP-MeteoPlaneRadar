import os
import math
from PIL import Image

# 16-color retro palette for DigiCat
PALETTE_RGBA = {
    0: (0, 0, 0, 0),             # 0: Transparent
    1: (28, 20, 16, 255),        # 1: Dark Outline / Contour
    2: (170, 75, 20, 255),       # 2: Dark Ginger Tabby Stripe / Fur Shadow
    3: (220, 115, 30, 255),      # 3: Medium Ginger Orange Fur
    4: (250, 160, 50, 255),      # 4: Bright Orange Main Coat
    5: (255, 225, 175, 255),     # 5: Warm Cream Muzzle & Underbelly
    6: (255, 255, 255, 255),     # 6: Pure White Chest Bib & Paws
    7: (255, 165, 180, 255),     # 7: Soft Pink Nose & Inner Ear
    8: (245, 90, 130, 255),      # 8: Rosy Blush & Tongue
    9: (20, 20, 24, 255),        # 9: Deep Eye Pupil
    10: (45, 185, 75, 255),      # 10: Emerald Green Cat Eye Iris
    11: (255, 205, 45, 255),     # 11: Crispy Golden Fish Cracker
    12: (140, 80, 20, 255),      # 12: Fish Crust Detail
    13: (255, 55, 85, 255),      # 13: Floating Love Heart
    14: (130, 190, 255, 255),    # 14: Sleeping Zzz Soft Blue
    15: (0, 0, 0, 50),           # 15: Ground Shadow
}

def to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

PALETTE_RGB565 = [
    0x0000, # 0: Transparent
    to_rgb565(28, 20, 16),
    to_rgb565(170, 75, 20),
    to_rgb565(220, 115, 30),
    to_rgb565(250, 160, 50),
    to_rgb565(255, 225, 175),
    to_rgb565(255, 255, 255),
    to_rgb565(255, 165, 180),
    to_rgb565(245, 90, 130),
    to_rgb565(20, 20, 24),
    to_rgb565(45, 185, 75),
    to_rgb565(255, 205, 45),
    to_rgb565(140, 80, 20),
    to_rgb565(255, 55, 85),
    to_rgb565(130, 190, 255),
    0x0821, # 15: Dark slate shadow
]

WIDTH = 64
HEIGHT = 64

class Sprite:
    def __init__(self, name):
        self.name = name
        self.pixels = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

    def set(self, x, y, c):
        if 0 <= x < WIDTH and 0 <= y < HEIGHT:
            self.pixels[y][x] = c

    def fill_rect(self, x, y, w, h, c):
        for cy in range(y, y + h):
            for cx in range(x, x + w):
                self.set(cx, cy, c)

    def draw_disc(self, cx, cy, r, c, fill=True):
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                d2 = (x - cx)**2 + (y - cy)**2
                if fill and d2 <= r*r:
                    self.set(x, y, c)
                elif not fill and abs(d2 - r*r) <= r:
                    self.set(x, y, c)

    def draw_ellipse(self, cx, cy, rx, ry, c):
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                dx = (x - cx) / float(rx if rx > 0 else 1)
                dy = (y - cy) / float(ry if ry > 0 else 1)
                if dx*dx + dy*dy <= 1.0:
                    self.set(x, y, c)

    def draw_outline(self):
        # Auto-generate a 1-pixel dark outline around all non-transparent pixels
        copy = [row[:] for row in self.pixels]
        for y in range(HEIGHT):
            for x in range(WIDTH):
                if copy[y][x] != 0:
                    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1), (-1,-1), (-1,1), (1,-1), (1,1)]:
                        ny, nx = y + dy, x + dx
                        if 0 <= ny < HEIGHT and 0 <= nx < WIDTH:
                            if copy[ny][nx] == 0:
                                self.pixels[ny][nx] = 1

    def to_image(self):
        img = Image.new("RGBA", (WIDTH, HEIGHT))
        p = img.load()
        for y in range(HEIGHT):
            for x in range(WIDTH):
                c = self.pixels[y][x]
                p[x, y] = PALETTE_RGBA[c]
        return img

# --- PROCEDURAL SPRITE BUILDER ---

def build_sitting_cat(frame_idx, mode="idle"):
    s = Sprite(f"sit_{mode}_{frame_idx}")

    # Breathing bounce
    bob_y = 1 if frame_idx in (1, 2) else 0
    ear_twitch = 1 if (mode == "idle" and frame_idx == 3) else 0

    # Ground shadow
    s.draw_ellipse(32, 60, 20, 3, 15)

    # 1. Tail (curled to right side)
    tail_wave = (frame_idx % 4) * 2
    if mode == "happy":
        tail_wave = (frame_idx % 2) * 4
    for i in range(12):
        tx = 44 + int(math.sin((i + tail_wave)*0.4) * 5)
        ty = 54 - i
        s.draw_disc(tx, ty, 3, 4)
        if i in (3, 7): # Tabby tail stripes
            s.draw_disc(tx, ty, 3, 2)
        if i >= 10: # White tail tip
            s.draw_disc(tx, ty, 2, 6)

    # 2. Main Body / Torso
    s.draw_ellipse(32, 45 + bob_y, 16, 13, 4)
    # Tabby stripes on flank
    for sx in (22, 42):
        s.draw_disc(sx, 44 + bob_y, 2, 2)
        s.draw_disc(sx, 48 + bob_y, 3, 2)

    # White chest bib & tummy
    s.draw_ellipse(32, 47 + bob_y, 8, 10, 6)
    s.draw_ellipse(32, 44 + bob_y, 6, 7, 5)

    # 3. Paws (Sitting neatly together)
    # Left front paw
    s.draw_ellipse(26, 57, 4, 3, 6)
    s.draw_disc(26, 56, 1, 1) # Paw toe crease
    # Right front paw
    s.draw_ellipse(38, 57, 4, 3, 6)
    s.draw_disc(38, 56, 1, 1) # Paw toe crease
    # Back haunches
    s.draw_ellipse(19, 52 + bob_y, 6, 7, 3)
    s.draw_ellipse(45, 52 + bob_y, 6, 7, 3)

    # 4. Head
    head_y = 28 + bob_y
    if mode == "watch":
        head_y -= 2 # looking up
    s.draw_disc(32, head_y, 15, 4)
    # Chubby cheeks
    s.draw_disc(20, head_y + 4, 6, 4)
    s.draw_disc(44, head_y + 4, 6, 4)

    # Ears
    # Left Ear
    for i in range(8):
        s.draw_disc(20 - i//2, head_y - 12 - i + ear_twitch, 3 - i//3, 4)
        if i < 6:
            s.draw_disc(20 - i//2, head_y - 12 - i + ear_twitch, 1, 7) # Pink inner ear
    # Right Ear
    for i in range(8):
        s.draw_disc(44 + i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6:
            s.draw_disc(44 + i//2, head_y - 12 - i, 1, 7) # Pink inner ear

    # Forehead Tabby "M" mark
    s.draw_disc(32, head_y - 9, 2, 2)
    s.draw_disc(28, head_y - 8, 1, 2)
    s.draw_disc(36, head_y - 8, 1, 2)
    s.draw_disc(25, head_y - 10, 1, 2)
    s.draw_disc(39, head_y - 10, 1, 2)

    # White Muzzle & Cheeks
    s.draw_ellipse(28, head_y + 6, 5, 4, 6)
    s.draw_ellipse(36, head_y + 6, 5, 4, 6)

    # Cute pink triangular nose
    s.set(31, head_y + 4, 7)
    s.set(32, head_y + 4, 7)
    s.set(33, head_y + 4, 7)
    s.set(32, head_y + 5, 7)

    # Cat mouth: :3
    s.set(32, head_y + 6, 1)
    s.set(30, head_y + 7, 1)
    s.set(34, head_y + 7, 1)

    # Eyes & Expressions
    if mode == "idle" and frame_idx == 2: # Blinking
        # Closed smiling eyes: ^  ^
        for dx in range(5):
            s.set(23 + dx, head_y - 1 + abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 + abs(dx - 2), 1)
    elif mode == "happy":
        # Blissful purr arches: ^  ^
        for dx in range(6):
            s.set(22 + dx, head_y - 1 - abs(dx - 3), 1)
            s.set(36 + dx, head_y - 1 - abs(dx - 3), 1)
        # Rosy blush cheeks
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)
        # Love heart bubble
        hx = 52 + (frame_idx % 2) * 2
        hy = 14 - frame_idx * 3
        s.draw_disc(hx - 2, hy, 2, 13)
        s.draw_disc(hx + 2, hy, 2, 13)
        s.set(hx, hy + 2, 13)
    elif mode == "watch":
        # Looking high up at aircraft!
        s.draw_ellipse(25, head_y - 3, 4, 5, 10)
        s.draw_ellipse(39, head_y - 3, 4, 5, 10)
        # Pupils looking up
        s.draw_disc(25, head_y - 5, 2, 9)
        s.draw_disc(39, head_y - 5, 2, 9)
        # Sparkle
        s.set(24, head_y - 6, 6)
        s.set(38, head_y - 6, 6)
    else: # Normal bright open eyes
        s.draw_ellipse(25, head_y, 4, 5, 10) # Emerald iris
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 3, 9)        # Black pupil
        s.draw_disc(39, head_y, 3, 9)
        # Specular white highlights
        s.set(24, head_y - 2, 6)
        s.set(25, head_y - 2, 6)
        s.set(38, head_y - 2, 6)
        s.set(39, head_y - 2, 6)

    # Whiskers
    whisker_y = head_y + 4
    for dy in (-1, 1):
        for w in range(6):
            s.set(16 - w, whisker_y + dy*w//3, 6)
            s.set(48 + w, whisker_y + dy*w//3, 6)

    s.draw_outline()
    return s

def build_walking_cat(frame_idx):
    s = Sprite(f"walk_{frame_idx}")
    bob_y = 1 if frame_idx in (1, 3) else 0

    # Ground shadow moves with body
    s.draw_ellipse(32, 59, 22, 3, 15)

    # 1. Tail (walking high tail posture)
    tail_sway = int(math.sin(frame_idx * math.pi / 2.0) * 4)
    for i in range(14):
        tx = 12 - i//2 + tail_sway * (i / 14.0)
        ty = 44 - i
        s.draw_disc(int(tx), int(ty), 3, 4)
        if i in (4, 8):
            s.draw_disc(int(tx), int(ty), 3, 2)
        if i >= 12:
            s.draw_disc(int(tx), int(ty), 2, 6) # White tip

    # 2. Body (horizontal elongated walking cat)
    s.draw_ellipse(30, 42 + bob_y, 18, 11, 4)
    # Tabby stripes on flank
    for sx in (22, 28, 34):
        s.draw_disc(sx, 38 + bob_y, 2, 2)
        s.draw_disc(sx - 1, 42 + bob_y, 2, 2)

    # White chest & belly
    s.draw_ellipse(36, 44 + bob_y, 10, 8, 6)

    # 3. Four Articulated Walking Legs
    # Phase positions: frame 0, 1, 2, 3
    leg_offsets = [
        (-4, 4, 4, -4), # Front Left, Front Right, Back Left, Back Right
        (-1, 2, 2, -1),
        (4, -4, -4, 4),
        (2, -1, -1, 2),
    ][frame_idx]

    fl_x, fr_x, bl_x, br_x = leg_offsets

    # Back Right Leg (far)
    s.draw_ellipse(18 + br_x, 50, 4, 7, 3)
    s.draw_ellipse(18 + br_x, 57, 3, 2, 6)

    # Front Right Leg (far)
    s.draw_ellipse(42 + fr_x, 50, 4, 7, 3)
    s.draw_ellipse(42 + fr_x, 57, 3, 2, 6)

    # Back Left Leg (near)
    s.draw_ellipse(20 + bl_x, 51 + bob_y, 5, 8, 4)
    s.draw_ellipse(20 + bl_x, 58, 4, 3, 6)

    # Front Left Leg (near)
    s.draw_ellipse(44 + fl_x, 51 + bob_y, 5, 8, 4)
    s.draw_ellipse(44 + fl_x, 58, 4, 3, 6)

    # 4. Head
    head_x = 48
    head_y = 28 + bob_y
    s.draw_disc(head_x, head_y, 13, 4)
    s.draw_disc(head_x + 5, head_y + 3, 5, 4) # Cheek

    # Ears
    # Back ear
    for i in range(7):
        s.draw_disc(head_x - 3 + i//3, head_y - 10 - i, 2, 3)
    # Front ear
    for i in range(8):
        s.draw_disc(head_x + 3 + i//3, head_y - 10 - i, 3, 4)
        if i < 6:
            s.draw_disc(head_x + 3 + i//3, head_y - 10 - i, 1, 7)

    # Muzzle
    s.draw_ellipse(head_x + 6, head_y + 4, 5, 4, 6)
    # Pink nose
    s.set(head_x + 10, head_y + 2, 7)
    s.set(head_x + 11, head_y + 2, 7)
    s.set(head_x + 10, head_y + 3, 7)

    # Eye (Profile view, facing forward-right)
    s.draw_ellipse(head_x + 4, head_y - 1, 3, 4, 10)
    s.draw_disc(head_x + 5, head_y - 1, 2, 9)
    s.set(head_x + 4, head_y - 2, 6)

    # Whiskers
    for dy in (-1, 1):
        for w in range(5):
            s.set(head_x + 11 + w, head_y + 3 + dy*w//3, 6)

    s.draw_outline()
    return s

def build_eating_cat(frame_idx):
    s = Sprite(f"eat_{frame_idx}")
    bob_y = 1 if frame_idx in (1, 2) else 0

    # Ground shadow
    s.draw_ellipse(32, 60, 20, 3, 15)

    # 1. Tail (curled to right side)
    tail_wave = (frame_idx % 4) * 2
    for i in range(12):
        tx = 44 + int(math.sin((i + tail_wave)*0.4) * 5)
        ty = 54 - i
        s.draw_disc(tx, ty, 3, 4)
        if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
        if i >= 10: s.draw_disc(tx, ty, 2, 6)

    # 2. Torso
    s.draw_ellipse(32, 45 + bob_y, 16, 13, 4)
    for sx in (22, 42):
        s.draw_disc(sx, 44 + bob_y, 2, 2)
        s.draw_disc(sx, 48 + bob_y, 3, 2)
    s.draw_ellipse(32, 47 + bob_y, 8, 10, 6)
    s.draw_ellipse(32, 44 + bob_y, 6, 7, 5)

    # 3. Paws
    s.draw_ellipse(26, 57, 4, 3, 6)
    s.draw_ellipse(38, 57, 4, 3, 6)
    s.draw_ellipse(19, 52 + bob_y, 6, 7, 3)
    s.draw_ellipse(45, 52 + bob_y, 6, 7, 3)

    # 4. Head
    head_y = 29 + bob_y
    s.draw_disc(32, head_y, 15, 4)
    s.draw_disc(20, head_y + 4, 6, 4)
    s.draw_disc(44, head_y + 4, 6, 4)

    # Ears (matching sitting cat)
    for i in range(8):
        s.draw_disc(20 - i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6: s.draw_disc(20 - i//2, head_y - 12 - i, 1, 7)
    for i in range(8):
        s.draw_disc(44 + i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6: s.draw_disc(44 + i//2, head_y - 12 - i, 1, 7)

    # Forehead Tabby "M" mark
    s.draw_disc(32, head_y - 9, 2, 2)
    s.draw_disc(28, head_y - 8, 1, 2)
    s.draw_disc(36, head_y - 8, 1, 2)

    # White Muzzle
    s.draw_ellipse(28, head_y + 6, 5, 4, 6)
    s.draw_ellipse(36, head_y + 6, 5, 4, 6)
    s.set(31, head_y + 4, 7)
    s.set(32, head_y + 4, 7)
    s.set(33, head_y + 4, 7)
    s.set(32, head_y + 5, 7)

    # Whiskers
    for dy in (-1, 1):
        for w in range(6):
            s.set(16 - w, head_y + 4 + dy*w//3, 6)
            s.set(48 + w, head_y + 4 + dy*w//3, 6)

    # Eating mouth & treat
    if frame_idx == 0:
        # Golden fish cracker on ground
        fx, fy = 32, 54
        s.draw_ellipse(fx, fy, 7, 3, 11)
        s.draw_disc(fx + 6, fy, 2, 11)
        s.set(fx - 3, fy - 1, 12)
        # Open eyes looking down at treat
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(25, head_y + 1, 2, 9)
        s.draw_disc(39, head_y + 1, 2, 9)
    elif frame_idx in (1, 2):
        # Munch crunch! Mouth open with treat crumbs
        s.draw_ellipse(32, head_y + 8, 4, 3, 8)
        s.set(31, head_y + 8, 11) # fish piece in mouth
        # Smiling eyes: ^  ^
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
        s.draw_disc(18, head_y + 5, 3, 8) # Blush
        s.draw_disc(46, head_y + 5, 3, 8)
        # Crumb particles
        s.set(26, head_y + 12, 11)
        s.set(38, head_y + 11, 11)
    else: # frame 3: Licking chops clean!
        s.draw_ellipse(32, head_y + 6, 3, 2, 8) # cute pink tongue out
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)

    s.draw_outline()
    return s

def build_sleeping_cat(frame_idx):
    s = Sprite(f"sleep_{frame_idx}")
    breath = 1 if frame_idx in (1, 2) else 0

    # Ground shadow
    s.draw_ellipse(32, 57, 24, 4, 15)

    # 1. Tail wrapped around
    for i in range(16):
        tx = 10 + i * 2
        ty = 53 + int(math.sin(i * 0.35) * 2)
        s.draw_disc(tx, ty, 3, 4)
        if i in (5, 9): s.draw_disc(tx, ty, 3, 2)
        if i >= 13: s.draw_disc(tx, ty, 2, 6)

    # 2. Curled Body
    s.draw_ellipse(30, 44 - breath, 18, 12 + breath, 4)
    for sx in (20, 26, 32):
        s.draw_disc(sx, 36 - breath, 2, 2)
        s.draw_disc(sx, 40 - breath, 3, 2)

    # White belly patch tucked in
    s.draw_ellipse(34, 47 - breath, 8, 6, 6)

    # 3. Head resting on paws
    head_x = 42
    head_y = 40 - breath
    s.draw_disc(head_x, head_y, 11, 4)
    s.draw_disc(head_x + 5, head_y + 3, 4, 4)

    # Ears folded peacefully
    for i in range(6):
        s.draw_disc(head_x - 4 + i//3, head_y - 9 - i, 2, 4)
        s.draw_disc(head_x + 4 + i//3, head_y - 9 - i, 2, 4)
        if i < 4:
            s.draw_disc(head_x - 4 + i//3, head_y - 9 - i, 1, 7)
            s.draw_disc(head_x + 4 + i//3, head_y - 9 - i, 1, 7)

    # White muzzle & nose
    s.draw_ellipse(head_x + 3, head_y + 4, 4, 3, 6)
    s.set(head_x + 6, head_y + 2, 7) # Pink nose

    # Sleeping eyes: quiet gentle curve
    s.set(head_x - 1, head_y - 1, 1)
    s.set(head_x, head_y, 1)
    s.set(head_x + 1, head_y, 1)
    s.set(head_x + 2, head_y - 1, 1)

    # Whiskers
    s.set(head_x + 8, head_y + 3, 6)
    s.set(head_x + 9, head_y + 3, 6)
    s.set(head_x + 8, head_y + 5, 6)
    s.set(head_x + 9, head_y + 6, 6)

    # Paws curled under head
    s.draw_ellipse(36, 51, 4, 3, 6)

    # Floating Zzz
    zx = 20 - frame_idx * 3
    zy = 24 - frame_idx * 3
    if frame_idx >= 1:
        s.draw_disc(zx, zy, 2, 14)
        s.draw_disc(zx + 5, zy - 4, 2, 14)
        s.draw_disc(zx + 10, zy - 8, 3, 14)

    s.draw_outline()
    return s

def build_jumping_cat(frame_idx):
    s = Sprite(f"jump_{frame_idx}")
    if frame_idx == 0:
        # 1. Crouch / Coil anticipation
        s.draw_ellipse(32, 60, 24, 4, 15) # Full shadow
        # Tail low twitch
        for i in range(12):
            tx = 12 + i//2
            ty = 53 - int(math.sin(i * 0.5) * 3)
            s.draw_disc(tx, ty, 3, 4)
            if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
            if i >= 10: s.draw_disc(tx, ty, 2, 6)
        # Low coiled body
        s.draw_ellipse(28, 48, 17, 9, 4)
        for sx in (20, 26, 32):
            s.draw_disc(sx, 46, 2, 2)
        s.draw_ellipse(32, 50, 8, 6, 6)
        # Haunches coiled tight
        s.draw_ellipse(16, 49, 6, 7, 3)
        # Front paws planted forward
        s.draw_ellipse(42, 56, 5, 3, 6)
        # Head lowered forward
        head_x, head_y = 44, 37
        s.draw_disc(head_x, head_y, 13, 4)
        s.draw_disc(head_x + 4, head_y + 3, 5, 4)
        # Flattened pounce ears
        for i in range(7):
            s.draw_disc(head_x - 3 + i//3, head_y - 8 - i//2, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 8 - i//2, 2, 4)
        # Muzzle & nose
        s.draw_ellipse(head_x + 6, head_y + 4, 4, 3, 6)
        s.set(head_x + 9, head_y + 2, 7)
        # Wide focused predatory pupils!
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 3, 9)
        s.set(head_x + 3, head_y - 2, 6)
    elif frame_idx == 1:
        # 2. Launch / Ascending leap diagonally up
        s.draw_ellipse(28, 60, 14, 2, 15) # Faded smaller shadow
        # Tail streaming backwards
        for i in range(14):
            tx = 10 - i//2
            ty = 44 + i//3
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 12: s.draw_disc(int(tx), int(ty), 2, 6)
        # Stretched leaping torso
        s.draw_ellipse(32, 32, 18, 9, 4)
        for sx in (24, 30, 36):
            s.draw_disc(sx, 30, 2, 2)
        s.draw_ellipse(36, 33, 9, 6, 6)
        # Back legs trailing stretched behind
        s.draw_ellipse(16, 39, 4, 8, 3)
        s.draw_ellipse(12, 45, 3, 3, 6)
        # Front legs reaching forward-up
        s.draw_ellipse(46, 26, 4, 7, 4)
        s.draw_ellipse(50, 22, 4, 3, 6)
        # Head forward-up
        head_x, head_y = 48, 20
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        # Ears perked high
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        # Determined leaping eyes
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 2, 9)
        s.set(head_x + 3, head_y - 2, 6)
    elif frame_idx == 2:
        # 3. Apex flight / mid-air float
        s.draw_ellipse(32, 60, 11, 2, 15) # Very small high shadow
        # Curving tail in flight
        for i in range(14):
            tx = 12 + int(math.sin(i * 0.3) * 3)
            ty = 30 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (4, 8): s.draw_disc(tx, ty, 3, 2)
            if i >= 12: s.draw_disc(tx, ty, 2, 6)
        # Graceful body in air
        s.draw_ellipse(32, 23, 17, 10, 4)
        s.draw_ellipse(34, 25, 8, 7, 6)
        # Tucked paws ready to land
        s.draw_ellipse(46, 28, 4, 5, 6)
        s.draw_ellipse(20, 29, 4, 5, 3)
        # Head
        head_x, head_y = 46, 17
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 2, 9)
    else:
        # 4. Landing cushion
        s.draw_ellipse(32, 60, 24, 4, 15)
        # Tail held high balancing
        for i in range(12):
            tx = 16 + i//2
            ty = 46 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
            if i >= 10: s.draw_disc(tx, ty, 2, 6)
        # Body absorbing landing shock
        s.draw_ellipse(32, 46, 18, 10, 4)
        s.draw_ellipse(34, 48, 9, 7, 6)
        # Front paws planted firmly on ground
        s.draw_ellipse(44, 57, 5, 3, 6)
        s.draw_ellipse(20, 54, 5, 5, 3)
        s.draw_ellipse(18, 57, 4, 2, 6)
        # Head
        head_x, head_y = 44, 32
        s.draw_disc(head_x, head_y, 13, 4)
        s.draw_disc(head_x + 4, head_y + 3, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 6, head_y + 4, 4, 3, 6)
        s.set(head_x + 9, head_y + 2, 7)
        # Happy squint landing eyes: ^
        for dx in range(5):
            s.set(head_x + 1 + dx, head_y - 1 - abs(dx - 2), 1)

    s.draw_outline()
    return s

def build_grooming_cat(frame_idx):
    s = Sprite(f"groom_{frame_idx}")
    s.draw_ellipse(32, 60, 20, 3, 15)

    # Curled tail
    for i in range(12):
        tx = 44 + int(math.sin(i * 0.4) * 4)
        ty = 54 - i
        s.draw_disc(tx, ty, 3, 4)
        if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
        if i >= 10: s.draw_disc(tx, ty, 2, 6)

    # Torso
    s.draw_ellipse(32, 45, 16, 13, 4)
    s.draw_ellipse(32, 47, 8, 10, 6)

    # Left front paw & haunches
    s.draw_ellipse(24, 57, 4, 3, 6)
    s.draw_ellipse(19, 52, 6, 7, 3)
    s.draw_ellipse(45, 52, 6, 7, 3)

    # Head
    head_y = 28
    s.draw_disc(32, head_y, 15, 4)
    s.draw_disc(20, head_y + 4, 6, 4)
    s.draw_disc(44, head_y + 4, 6, 4)

    # Ears
    for i in range(8):
        s.draw_disc(20 - i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6: s.draw_disc(20 - i//2, head_y - 12 - i, 1, 7)
    for i in range(8):
        # Right ear bends down when rubbed in frame 2
        ry = head_y - 12 - (i//2 if frame_idx == 2 else i)
        s.draw_disc(44 + i//2, ry, 3 - i//3, 4)
        if i < 6: s.draw_disc(44 + i//2, ry, 1, 7)

    # Forehead Tabby
    s.draw_disc(32, head_y - 9, 2, 2)
    s.draw_ellipse(28, head_y + 6, 5, 4, 6)
    s.draw_ellipse(36, head_y + 6, 5, 4, 6)
    s.set(31, head_y + 4, 7)
    s.set(32, head_y + 4, 7)

    # Active Raised Grooming Paw & Face Wash Motion
    if frame_idx == 0:
        # Lifting right paw to mouth, pink tongue out licking paw pad
        s.draw_ellipse(36, 38, 4, 5, 6)
        s.draw_disc(34, 37, 2, 7) # Paw pad
        s.draw_disc(32, head_y + 7, 2, 8) # Cute tongue licking
        # Left eye open, right looking at paw
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 2, 9)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(37, head_y + 1, 2, 9)
    elif frame_idx == 1:
        # Paw wiping cheek
        s.draw_ellipse(42, 28, 5, 5, 6)
        s.draw_disc(42, head_y + 5, 3, 8) # Cheek squished blush
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 2:
        # Paw rubbing over ear in full face wash arc!
        s.draw_ellipse(44, 18, 5, 5, 6)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(44, head_y + 5, 3, 8)
        # Contented curved eyes: ^  ^
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
    else: # frame_idx == 3
        # Paw lowered, licking whiskers clean
        s.draw_ellipse(38, 48, 4, 3, 6)
        s.draw_disc(32, head_y + 6, 2, 8) # Tongue flick
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 2, 9)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(39, head_y, 2, 9)
        s.set(24, head_y - 2, 6)
        s.set(38, head_y - 2, 6)

    s.draw_outline()
    return s

def build_stretching_cat(frame_idx):
    s = Sprite(f"stretch_{frame_idx}")
    if frame_idx == 0:
        # 1. Downward Dog/Cat Yoga Stretch (Front low, butt high)
        s.draw_ellipse(32, 60, 26, 3, 15) # Long shadow
        # Tail held high and proud straight up with hook!
        for i in range(15):
            tx = 15 + (i//4 if i < 12 else (i - 12))
            ty = 40 - i
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 13: s.draw_disc(int(tx), int(ty), 2, 6)
        # Slanted body: high back (Y=38), low chest (Y=52)
        s.draw_ellipse(22, 38, 10, 11, 3) # High rear haunches
        s.draw_ellipse(16, 46, 5, 8, 3)
        s.draw_ellipse(16, 57, 4, 2, 6) # Rear paw
        s.draw_ellipse(34, 46, 14, 9, 4)  # Slanted torso
        s.draw_ellipse(38, 48, 8, 6, 6)   # Belly
        # Front paws extended far forward on floor!
        s.draw_ellipse(48, 54, 5, 4, 4)
        s.draw_ellipse(54, 57, 6, 3, 6)
        # Low head resting between paws
        head_x, head_y = 40, 42
        s.draw_disc(head_x, head_y, 11, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(6):
            s.draw_disc(head_x - 3 + i//3, head_y - 8 - i, 2, 4)
            s.draw_disc(head_x + 3 + i//3, head_y - 8 - i, 2, 4)
        s.draw_ellipse(head_x + 4, head_y + 3, 4, 3, 6)
        s.set(head_x + 7, head_y + 2, 7)
        # Satisfied stretch eyes: closed gentle smile
        for dx in range(5):
            s.set(head_x - 2 + dx, head_y - 1 - abs(dx - 2), 1)
    else:
        # 2. Tall arched back stretch
        s.draw_ellipse(32, 60, 22, 4, 15)
        # Arched inverted-U tail
        for i in range(14):
            tx = 12 + int(math.sin(i * 0.3) * 4)
            ty = 44 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (4, 8): s.draw_disc(tx, ty, 3, 2)
            if i >= 12: s.draw_disc(tx, ty, 2, 6)
        # Very tall hump body
        s.draw_ellipse(30, 32, 16, 14, 4)
        for sx in (22, 28, 34):
            s.draw_disc(sx, 26, 2, 2)
        s.draw_ellipse(32, 36, 8, 8, 6)
        # Straight legs planted claws out
        s.draw_ellipse(18, 48, 4, 9, 3)
        s.draw_ellipse(18, 57, 4, 3, 6)
        s.draw_ellipse(42, 48, 4, 9, 4)
        s.draw_ellipse(42, 57, 4, 3, 6)
        # Head tilted up
        head_x, head_y = 44, 25
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        # Eyes
        s.draw_ellipse(head_x + 2, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 3, head_y - 1, 2, 9)

    s.draw_outline()
    return s

def main():
    sprites = []

    # 1. Walk Cycle (4 frames)
    for i in range(4):
        sprites.append(build_walking_cat(i))

    # 2. Sitting Idle (4 frames: normal, breathe, blink, ear-twitch)
    for i in range(4):
        sprites.append(build_sitting_cat(i, "idle"))

    # 3. Happy / Petting (4 frames)
    for i in range(4):
        sprites.append(build_sitting_cat(i, "happy"))

    # 4. Watch Sky / Airspace (2 frames)
    for i in range(2):
        sprites.append(build_sitting_cat(i, "watch"))

    # 5. Eating treat (4 frames)
    for i in range(4):
        sprites.append(build_eating_cat(i))

    # 6. Sleeping (4 frames)
    for i in range(4):
        sprites.append(build_sleeping_cat(i))

    # 7. Jumping / Pouncing (4 frames)
    for i in range(4):
        sprites.append(build_jumping_cat(i))

    # 8. Grooming / Face Wash (4 frames)
    for i in range(4):
        sprites.append(build_grooming_cat(i))

    # 9. Stretching (2 frames)
    for i in range(2):
        sprites.append(build_stretching_cat(i))

    # Save PNG previews in artifacts / preview folder
    out_dir = os.path.join(os.path.dirname(__file__), "preview_sprites")
    os.makedirs(out_dir, exist_ok=True)

    # Stitch a master sprite sheet image for user inspection
    sheet_cols = 4
    sheet_rows = (len(sprites) + sheet_cols - 1) // sheet_cols
    sheet = Image.new("RGBA", (sheet_cols * WIDTH, sheet_rows * HEIGHT), (30, 35, 45, 255))

    for idx, s in enumerate(sprites):
        img = s.to_image()
        # Scale up 2x for super crisp viewing
        scaled = img.resize((WIDTH * 2, HEIGHT * 2), Image.NEAREST)
        scaled.save(os.path.join(out_dir, f"{s.name}.png"))

        col = idx % sheet_cols
        row = idx // sheet_cols
        sheet.paste(img, (col * WIDTH, row * HEIGHT), img)

    sheet_scaled = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
    sheet_path = os.path.join(out_dir, "digicat_spritesheet.png")
    sheet_scaled.save(sheet_path)
    print(f"Sprite previews saved to {out_dir} (Total frames: {len(sprites)})")

    # Generate C++ header: MeteoPlaneRadar/CatSprites.h
    target_h = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "MeteoPlaneRadar", "CatSprites.h"))
    with open(target_h, "w", encoding="utf-8") as f:
        f.write("// =============================================================================\n")
        f.write("//  MeteoPlaneRadar\n")
        f.write("//  CatSprites.h - Handcrafted 16-color pixel-art sprite sheets for DigiCat.\n")
        f.write("//  Palette: 16 colors (RGB565). Resolution: 64x64 pixels (scaled 2x on display).\n")
        f.write("// =============================================================================\n")
        f.write("#pragma once\n")
        f.write("#include <Arduino.h>\n\n")
        f.write("#define CAT_SPRITE_W 64\n")
        f.write("#define CAT_SPRITE_H 64\n\n")

        # Palette
        f.write("static const uint16_t CAT_PALETTE[16] PROGMEM = {\n")
        for i, col in enumerate(PALETTE_RGB565):
            f.write(f"  0x{col:04X}, // {i}\n")
        f.write("};\n\n")

        # Sprite frames
        f.write(f"// Total animation frames: {len(sprites)}\n")
        for s in sprites:
            f.write(f"static const uint8_t CAT_{s.name.upper()}[{WIDTH * HEIGHT}] PROGMEM = {{\n")
            for y in range(HEIGHT):
                f.write("  ")
                for x in range(WIDTH):
                    f.write(f"{s.pixels[y][x]},")
                f.write("\n")
            f.write("};\n\n")

        # Frame enumerations & lookup tables
        f.write("// Animation groups & frame pointers\n")
        f.write("static const uint8_t* const CAT_WALK_FRAMES[4] = {\n")
        f.write("  CAT_WALK_0, CAT_WALK_1, CAT_WALK_2, CAT_WALK_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_IDLE_FRAMES[4] = {\n")
        f.write("  CAT_SIT_IDLE_0, CAT_SIT_IDLE_1, CAT_SIT_IDLE_2, CAT_SIT_IDLE_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_HAPPY_FRAMES[4] = {\n")
        f.write("  CAT_SIT_HAPPY_0, CAT_SIT_HAPPY_1, CAT_SIT_HAPPY_2, CAT_SIT_HAPPY_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_WATCH_FRAMES[2] = {\n")
        f.write("  CAT_SIT_WATCH_0, CAT_SIT_WATCH_1\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_EAT_FRAMES[4] = {\n")
        f.write("  CAT_EAT_0, CAT_EAT_1, CAT_EAT_2, CAT_EAT_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_SLEEP_FRAMES[4] = {\n")
        f.write("  CAT_SLEEP_0, CAT_SLEEP_1, CAT_SLEEP_2, CAT_SLEEP_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_JUMP_FRAMES[4] = {\n")
        f.write("  CAT_JUMP_0, CAT_JUMP_1, CAT_JUMP_2, CAT_JUMP_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_GROOM_FRAMES[4] = {\n")
        f.write("  CAT_GROOM_0, CAT_GROOM_1, CAT_GROOM_2, CAT_GROOM_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_STRETCH_FRAMES[2] = {\n")
        f.write("  CAT_STRETCH_0, CAT_STRETCH_1\n")
        f.write("};\n\n")

    print(f"Generated {target_h} successfully.")

if __name__ == "__main__":
    main()
