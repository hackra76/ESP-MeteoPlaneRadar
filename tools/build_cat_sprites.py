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

# --- PROCEDURAL SPRITE BUILDER (64-FRAME HIGH FLUIDITY EXPANSION) ---

def build_sitting_cat(frame_idx, mode="idle"):
    s = Sprite(f"sit_{mode}_{frame_idx}")

    # 8-phase sub-pixel breathing cycle
    if mode == "watch":
        bob_y = 0
        ear_twitch = 0
    else:
        # Smooth breath cycle across 8 frames
        b_cycles = [0, 1, 1, 2, 1, 0, -1, 0]
        bob_y = b_cycles[frame_idx % 8]
        ear_twitch = 1 if (mode == "idle" and frame_idx == 6) else 0

    # Ground shadow
    s.draw_ellipse(32, 60, 20, 3, 15)

    # 1. Tail (curled to right side with continuous oscillation)
    if mode == "happy":
        tail_phase = (frame_idx % 8) * (2.0 * math.pi / 8.0) * 2.0
    elif mode == "watch":
        tail_phase = (frame_idx % 4) * (2.0 * math.pi / 4.0)
    else:
        tail_phase = (frame_idx % 8) * (2.0 * math.pi / 8.0)

    for i in range(12):
        tx = 44 + int(math.sin(i * 0.4 + tail_phase) * 5)
        ty = 54 - i
        s.draw_disc(tx, ty, 3, 4)
        if i in (3, 7):
            s.draw_disc(tx, ty, 3, 2)
        if i >= 10:
            s.draw_disc(tx, ty, 2, 6)

    # 2. Main Body / Torso
    s.draw_ellipse(32, 45 + bob_y, 16, 13, 4)
    for sx in (22, 42):
        s.draw_disc(sx, 44 + bob_y, 2, 2)
        s.draw_disc(sx, 48 + bob_y, 3, 2)

    # White chest bib & tummy
    s.draw_ellipse(32, 47 + bob_y, 8, 10, 6)
    s.draw_ellipse(32, 44 + bob_y, 6, 7, 5)

    # 3. Paws
    s.draw_ellipse(26, 57, 4, 3, 6)
    s.draw_disc(26, 56, 1, 1)
    s.draw_ellipse(38, 57, 4, 3, 6)
    s.draw_disc(38, 56, 1, 1)
    s.draw_ellipse(19, 52 + bob_y, 6, 7, 3)
    s.draw_ellipse(45, 52 + bob_y, 6, 7, 3)

    # 4. Head
    head_y = 28 + bob_y
    if mode == "watch":
        head_y -= 2
    s.draw_disc(32, head_y, 15, 4)
    s.draw_disc(20, head_y + 4, 6, 4)
    s.draw_disc(44, head_y + 4, 6, 4)

    # Ears
    for i in range(8):
        s.draw_disc(20 - i//2, head_y - 12 - i + ear_twitch, 3 - i//3, 4)
        if i < 6:
            s.draw_disc(20 - i//2, head_y - 12 - i + ear_twitch, 1, 7)
    for i in range(8):
        s.draw_disc(44 + i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6:
            s.draw_disc(44 + i//2, head_y - 12 - i, 1, 7)

    # Forehead Tabby "M" mark
    s.draw_disc(32, head_y - 9, 2, 2)
    s.draw_disc(28, head_y - 8, 1, 2)
    s.draw_disc(36, head_y - 8, 1, 2)
    s.draw_disc(25, head_y - 10, 1, 2)
    s.draw_disc(39, head_y - 10, 1, 2)

    # White Muzzle & Cheeks
    s.draw_ellipse(28, head_y + 6, 5, 4, 6)
    s.draw_ellipse(36, head_y + 6, 5, 4, 6)

    # Pink nose & mouth
    s.set(31, head_y + 4, 7)
    s.set(32, head_y + 4, 7)
    s.set(33, head_y + 4, 7)
    s.set(32, head_y + 5, 7)
    s.set(32, head_y + 6, 1)
    s.set(30, head_y + 7, 1)
    s.set(34, head_y + 7, 1)

    # Eyes & Expressions
    if mode == "idle":
        if frame_idx in (3, 4): # Natural 2-frame smooth blink
            for dx in range(5):
                s.set(23 + dx, head_y - 1 + abs(dx - 2), 1)
                s.set(37 + dx, head_y - 1 + abs(dx - 2), 1)
        else:
            s.draw_ellipse(25, head_y, 4, 5, 10)
            s.draw_ellipse(39, head_y, 4, 5, 10)
            s.draw_disc(25, head_y, 3, 9)
            s.draw_disc(39, head_y, 3, 9)
            s.set(24, head_y - 2, 6)
            s.set(25, head_y - 2, 6)
            s.set(38, head_y - 2, 6)
            s.set(39, head_y - 2, 6)
    elif mode == "happy":
        # Blissful purr arches
        for dx in range(6):
            s.set(22 + dx, head_y - 1 - abs(dx - 3), 1)
            s.set(36 + dx, head_y - 1 - abs(dx - 3), 1)
        # Rosy blush
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)
        # Floating heart smoothly moving upward and oscillating
        hx = 52 + int(math.sin(frame_idx * 0.8) * 3)
        hy = 20 - int(frame_idx * 2.2)
        s.draw_disc(hx - 2, hy, 2, 13)
        s.draw_disc(hx + 2, hy, 2, 13)
        s.set(hx, hy + 2, 13)
    elif mode == "watch":
        # Looking up at aircraft, tracking across sky (4 frames)
        look_offset_x = int((frame_idx - 1.5) * 2)
        s.draw_ellipse(25, head_y - 3, 4, 5, 10)
        s.draw_ellipse(39, head_y - 3, 4, 5, 10)
        s.draw_disc(25 + look_offset_x, head_y - 5, 2, 9)
        s.draw_disc(39 + look_offset_x, head_y - 5, 2, 9)
        s.set(24 + look_offset_x, head_y - 6, 6)
        s.set(38 + look_offset_x, head_y - 6, 6)

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
    # 8-frame walk cycle with natural sinusoidal bobbing
    b_cycles = [0, 1, 0, -1, 0, 1, 0, -1]
    bob_y = b_cycles[frame_idx % 8]

    # Ground shadow
    s.draw_ellipse(32, 59, 22, 3, 15)

    # 1. High tail posture with smooth sine sway
    tail_sway = int(math.sin(frame_idx * 2.0 * math.pi / 8.0) * 5)
    for i in range(14):
        tx = 12 - i//2 + tail_sway * (i / 14.0)
        ty = 44 - i
        s.draw_disc(int(tx), int(ty), 3, 4)
        if i in (4, 8):
            s.draw_disc(int(tx), int(ty), 3, 2)
        if i >= 12:
            s.draw_disc(int(tx), int(ty), 2, 6)

    # 2. Body
    s.draw_ellipse(30, 42 + bob_y, 18, 11, 4)
    for sx in (22, 28, 34):
        s.draw_disc(sx, 38 + bob_y, 2, 2)
        s.draw_disc(sx - 1, 42 + bob_y, 2, 2)
    s.draw_ellipse(36, 44 + bob_y, 10, 8, 6)

    # 3. 8-Phase Walking Gait
    angle = frame_idx * (2.0 * math.pi / 8.0)
    fl_x = int(math.sin(angle) * 5.5)
    fr_x = int(math.sin(angle + math.pi) * 5.5)
    bl_x = int(math.sin(angle + 0.8 * math.pi) * 5.0)
    br_x = int(math.sin(angle - 0.2 * math.pi) * 5.0)

    # Far legs
    s.draw_ellipse(18 + br_x, 50, 4, 7, 3)
    s.draw_ellipse(18 + br_x, 57, 3, 2, 6)
    s.draw_ellipse(42 + fr_x, 50, 4, 7, 3)
    s.draw_ellipse(42 + fr_x, 57, 3, 2, 6)

    # Near legs
    s.draw_ellipse(20 + bl_x, 51 + bob_y, 5, 8, 4)
    s.draw_ellipse(20 + bl_x, 58, 4, 3, 6)
    s.draw_ellipse(44 + fl_x, 51 + bob_y, 5, 8, 4)
    s.draw_ellipse(44 + fl_x, 58, 4, 3, 6)

    # 4. Head
    head_x = 48
    head_y = 28 + bob_y
    s.draw_disc(head_x, head_y, 13, 4)
    s.draw_disc(head_x + 5, head_y + 3, 5, 4)

    # Ears
    for i in range(7):
        s.draw_disc(head_x - 3 + i//3, head_y - 10 - i, 2, 3)
    for i in range(8):
        s.draw_disc(head_x + 3 + i//3, head_y - 10 - i, 3, 4)
        if i < 6:
            s.draw_disc(head_x + 3 + i//3, head_y - 10 - i, 1, 7)

    # Muzzle & nose
    s.draw_ellipse(head_x + 6, head_y + 4, 5, 4, 6)
    s.set(head_x + 10, head_y + 2, 7)
    s.set(head_x + 11, head_y + 2, 7)
    s.set(head_x + 10, head_y + 3, 7)

    # Profile Eye
    s.draw_ellipse(head_x + 4, head_y - 1, 3, 4, 10)
    s.draw_disc(head_x + 5, head_y - 1, 2, 9)
    s.set(head_x + 4, head_y - 2, 6)

    # Whiskers
    for dy in (-1, 1):
        for w in range(5):
            s.set(head_x + 11 + w, head_y + 3 + dy*w//3, 6)

    s.draw_outline()
    return s

def build_jumping_cat(frame_idx):
    s = Sprite(f"jump_{frame_idx}")
    # 8-frame jump sequence:
    # 0: Crouch anticipate
    # 1: Deep spring coil
    # 2: Launch extension
    # 3: Mid-air ascent
    # 4: Apex float
    # 5: Descent glide
    # 6: Touchdown paws
    # 7: Cushion squash & recovery

    if frame_idx == 0:
        # 0. Crouch anticipate
        s.draw_ellipse(32, 60, 24, 4, 15)
        for i in range(12):
            tx = 12 + i//2
            ty = 53 - int(math.sin(i * 0.5) * 3)
            s.draw_disc(tx, ty, 3, 4)
            if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
            if i >= 10: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(28, 48, 17, 9, 4)
        s.draw_ellipse(32, 50, 8, 6, 6)
        s.draw_ellipse(16, 49, 6, 7, 3)
        s.draw_ellipse(42, 56, 5, 3, 6)
        head_x, head_y = 44, 37
        s.draw_disc(head_x, head_y, 13, 4)
        s.draw_disc(head_x + 4, head_y + 3, 5, 4)
        for i in range(7):
            s.draw_disc(head_x - 3 + i//3, head_y - 8 - i//2, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 8 - i//2, 2, 4)
        s.draw_ellipse(head_x + 6, head_y + 4, 4, 3, 6)
        s.set(head_x + 9, head_y + 2, 7)
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 3, 9)
        s.set(head_x + 3, head_y - 2, 6)
    elif frame_idx == 1:
        # 1. Deep spring compression
        s.draw_ellipse(32, 60, 26, 4, 15)
        for i in range(12):
            tx = 10 + i//2
            ty = 55 - int(math.sin(i * 0.4) * 2)
            s.draw_disc(tx, ty, 3, 4)
            if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
            if i >= 10: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(28, 51, 18, 8, 4)
        s.draw_ellipse(32, 53, 8, 5, 6)
        s.draw_ellipse(15, 52, 6, 6, 3)
        s.draw_ellipse(43, 57, 6, 3, 6)
        head_x, head_y = 45, 39
        s.draw_disc(head_x, head_y, 13, 4)
        s.draw_disc(head_x + 4, head_y + 3, 5, 4)
        for i in range(7):
            s.draw_disc(head_x - 3 + i//3, head_y - 7 - i//2, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 7 - i//2, 2, 4)
        s.draw_ellipse(head_x + 6, head_y + 4, 4, 3, 6)
        s.set(head_x + 9, head_y + 2, 7)
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 3, 9)
    elif frame_idx == 2:
        # 2. Launch extension
        s.draw_ellipse(28, 60, 18, 3, 15)
        for i in range(13):
            tx = 11 - i//3
            ty = 48 + i//3
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 11: s.draw_disc(int(tx), int(ty), 2, 6)
        s.draw_ellipse(30, 39, 18, 9, 4)
        s.draw_ellipse(34, 40, 9, 6, 6)
        s.draw_ellipse(15, 46, 5, 7, 3)
        s.draw_ellipse(46, 33, 4, 7, 4)
        head_x, head_y = 47, 26
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 2, 9)
    elif frame_idx == 3:
        # 3. Ascent stretch
        s.draw_ellipse(28, 60, 14, 2, 15)
        for i in range(14):
            tx = 10 - i//2
            ty = 44 + i//3
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 12: s.draw_disc(int(tx), int(ty), 2, 6)
        s.draw_ellipse(32, 32, 18, 9, 4)
        s.draw_ellipse(36, 33, 9, 6, 6)
        s.draw_ellipse(16, 39, 4, 8, 3)
        s.draw_ellipse(12, 45, 3, 3, 6)
        s.draw_ellipse(46, 26, 4, 7, 4)
        s.draw_ellipse(50, 22, 4, 3, 6)
        head_x, head_y = 48, 20
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 2, 9)
        s.set(head_x + 3, head_y - 2, 6)
    elif frame_idx == 4:
        # 4. Apex float
        s.draw_ellipse(32, 60, 11, 2, 15)
        for i in range(14):
            tx = 12 + int(math.sin(i * 0.3) * 3)
            ty = 30 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (4, 8): s.draw_disc(tx, ty, 3, 2)
            if i >= 12: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(32, 23, 17, 10, 4)
        s.draw_ellipse(34, 25, 8, 7, 6)
        s.draw_ellipse(46, 28, 4, 5, 6)
        s.draw_ellipse(20, 29, 4, 5, 3)
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
    elif frame_idx == 5:
        # 5. Descent glide
        s.draw_ellipse(32, 60, 16, 3, 15)
        for i in range(13):
            tx = 14 + i//3
            ty = 36 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (4, 8): s.draw_disc(tx, ty, 3, 2)
            if i >= 11: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(32, 33, 17, 10, 4)
        s.draw_ellipse(34, 35, 8, 7, 6)
        s.draw_ellipse(46, 42, 4, 6, 6)
        s.draw_ellipse(20, 40, 4, 6, 3)
        head_x, head_y = 46, 25
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 3, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 4, head_y - 1, 2, 9)
    elif frame_idx == 6:
        # 6. Touchdown paws
        s.draw_ellipse(32, 60, 22, 4, 15)
        for i in range(12):
            tx = 15 + i//2
            ty = 44 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
            if i >= 10: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(32, 43, 18, 10, 4)
        s.draw_ellipse(34, 45, 9, 7, 6)
        s.draw_ellipse(44, 55, 5, 4, 6)
        s.draw_ellipse(20, 50, 5, 6, 3)
        head_x, head_y = 45, 30
        s.draw_disc(head_x, head_y, 13, 4)
        s.draw_disc(head_x + 4, head_y + 3, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 6, head_y + 4, 4, 3, 6)
        s.set(head_x + 9, head_y + 2, 7)
        for dx in range(5):
            s.set(head_x + 1 + dx, head_y - 1 - abs(dx - 2), 1)
    else:
        # 7. Cushion squash & recovery
        s.draw_ellipse(32, 60, 24, 4, 15)
        for i in range(12):
            tx = 16 + i//2
            ty = 48 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
            if i >= 10: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(32, 47, 18, 9, 4)
        s.draw_ellipse(34, 49, 9, 6, 6)
        s.draw_ellipse(44, 57, 5, 3, 6)
        s.draw_ellipse(20, 54, 5, 5, 3)
        s.draw_ellipse(18, 57, 4, 2, 6)
        head_x, head_y = 44, 34
        s.draw_disc(head_x, head_y, 13, 4)
        s.draw_disc(head_x + 4, head_y + 3, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 6, head_y + 4, 4, 3, 6)
        s.set(head_x + 9, head_y + 2, 7)
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
    s.draw_ellipse(24, 57, 4, 3, 6)
    s.draw_ellipse(19, 52, 6, 7, 3)
    s.draw_ellipse(45, 52, 6, 7, 3)

    # Head
    head_y = 28
    s.draw_disc(32, head_y, 15, 4)
    s.draw_disc(20, head_y + 4, 6, 4)
    s.draw_disc(44, head_y + 4, 6, 4)

    # Ears (Right ear bends down dynamically when rubbed in frames 3, 4, 5)
    ear_bend = (i//2) if (frame_idx in (3, 4, 5)) else i
    for i in range(8):
        s.draw_disc(20 - i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6: s.draw_disc(20 - i//2, head_y - 12 - i, 1, 7)
    for i in range(8):
        ry = head_y - 12 - (ear_bend if i < 6 else i)
        s.draw_disc(44 + i//2, ry, 3 - i//3, 4)
        if i < 6: s.draw_disc(44 + i//2, ry, 1, 7)

    # Forehead Tabby
    s.draw_disc(32, head_y - 9, 2, 2)
    s.draw_ellipse(28, head_y + 6, 5, 4, 6)
    s.draw_ellipse(36, head_y + 6, 5, 4, 6)
    s.set(31, head_y + 4, 7)
    s.set(32, head_y + 4, 7)

    # 8-Frame Smooth Grooming Motion Arc
    if frame_idx == 0:
        # 0. Lifting paw
        s.draw_ellipse(34, 44, 4, 5, 6)
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 2, 9)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(38, head_y + 1, 2, 9)
    elif frame_idx == 1:
        # 1. Paw at mouth, licking paw pad
        s.draw_ellipse(36, 38, 4, 5, 6)
        s.draw_disc(34, 37, 2, 7)
        s.draw_disc(32, head_y + 7, 2, 8)
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 2, 9)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(37, head_y + 1, 2, 9)
    elif frame_idx == 2:
        # 2. Paw wiping lower cheek
        s.draw_ellipse(40, 32, 5, 5, 6)
        s.draw_disc(42, head_y + 5, 3, 8)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 3:
        # 3. Paw wiping upper cheek
        s.draw_ellipse(43, 26, 5, 5, 6)
        s.draw_disc(43, head_y + 3, 3, 8)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 4:
        # 4. Paw reaching over ear apex
        s.draw_ellipse(45, 18, 5, 5, 6)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(44, head_y + 5, 3, 8)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 5:
        # 5. Paw sliding down front of forehead
        s.draw_ellipse(41, 22, 5, 5, 6)
        s.draw_disc(38, head_y + 4, 3, 8)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 6:
        # 6. Lowering paw, licking whiskers
        s.draw_ellipse(38, 44, 4, 4, 6)
        s.draw_disc(32, head_y + 6, 2, 8)
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 2, 9)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(39, head_y, 2, 9)
    else:
        # 7. Setting paw down cleanly
        s.draw_ellipse(38, 52, 4, 3, 6)
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
    # 6-frame yoga stretch arc:
    # 0: Lower chest start
    # 1: Deep downward dog stretch
    # 2: Full stretch hold & paw claw flex
    # 3: Returning from downward dog
    # 4: High rainbow arched back peak
    # 5: Relaxing arch back to neutral

    if frame_idx == 0:
        # 0. Lowering chest
        s.draw_ellipse(32, 60, 24, 3, 15)
        for i in range(14):
            tx = 14 + i//3
            ty = 43 - i
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 12: s.draw_disc(int(tx), int(ty), 2, 6)
        s.draw_ellipse(24, 41, 12, 11, 3)
        s.draw_ellipse(16, 49, 5, 7, 3)
        s.draw_ellipse(16, 57, 4, 2, 6)
        s.draw_ellipse(34, 45, 14, 9, 4)
        s.draw_ellipse(46, 53, 5, 4, 4)
        s.draw_ellipse(50, 57, 5, 3, 6)
        head_x, head_y = 42, 38
        s.draw_disc(head_x, head_y, 11, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(6):
            s.draw_disc(head_x - 3 + i//3, head_y - 8 - i, 2, 4)
            s.draw_disc(head_x + 3 + i//3, head_y - 8 - i, 2, 4)
        s.draw_ellipse(head_x + 4, head_y + 3, 4, 3, 6)
        s.set(head_x + 7, head_y + 2, 7)
        for dx in range(5):
            s.set(head_x - 2 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 1 or frame_idx == 2:
        # 1 & 2. Deep downward cat stretch (2: claws flexed)
        paw_ext = 2 if frame_idx == 2 else 0
        s.draw_ellipse(32, 60, 26 + paw_ext, 3, 15)
        for i in range(15):
            tx = 15 + (i//4 if i < 12 else (i - 12))
            ty = 40 - i
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 13: s.draw_disc(int(tx), int(ty), 2, 6)
        s.draw_ellipse(22, 38, 10, 11, 3)
        s.draw_ellipse(16, 46, 5, 8, 3)
        s.draw_ellipse(16, 57, 4, 2, 6)
        s.draw_ellipse(34, 46, 14, 9, 4)
        s.draw_ellipse(38, 48, 8, 6, 6)
        s.draw_ellipse(48 + paw_ext, 54, 5, 4, 4)
        s.draw_ellipse(54 + paw_ext, 57, 6, 3, 6)
        head_x, head_y = 40, 42
        s.draw_disc(head_x, head_y, 11, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(6):
            s.draw_disc(head_x - 3 + i//3, head_y - 8 - i, 2, 4)
            s.draw_disc(head_x + 3 + i//3, head_y - 8 - i, 2, 4)
        s.draw_ellipse(head_x + 4, head_y + 3, 4, 3, 6)
        s.set(head_x + 7, head_y + 2, 7)
        for dx in range(5):
            s.set(head_x - 2 + dx, head_y - 1 - abs(dx - 2), 1)
    elif frame_idx == 3:
        # 3. Transition to arch
        s.draw_ellipse(32, 60, 22, 3, 15)
        for i in range(14):
            tx = 14 + i//3
            ty = 42 - i
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 12: s.draw_disc(int(tx), int(ty), 2, 6)
        s.draw_ellipse(28, 38, 14, 11, 4)
        s.draw_ellipse(18, 50, 4, 7, 3)
        s.draw_ellipse(18, 57, 4, 2, 6)
        s.draw_ellipse(42, 50, 4, 7, 4)
        s.draw_ellipse(42, 57, 4, 2, 6)
        head_x, head_y = 44, 32
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 2, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 3, head_y - 1, 2, 9)
    elif frame_idx == 4:
        # 4. Peak rainbow arched back
        s.draw_ellipse(32, 60, 22, 4, 15)
        for i in range(14):
            tx = 12 + int(math.sin(i * 0.3) * 4)
            ty = 44 - i
            s.draw_disc(tx, ty, 3, 4)
            if i in (4, 8): s.draw_disc(tx, ty, 3, 2)
            if i >= 12: s.draw_disc(tx, ty, 2, 6)
        s.draw_ellipse(30, 32, 16, 14, 4)
        for sx in (22, 28, 34):
            s.draw_disc(sx, 26, 2, 2)
        s.draw_ellipse(32, 36, 8, 8, 6)
        s.draw_ellipse(18, 48, 4, 9, 3)
        s.draw_ellipse(18, 57, 4, 3, 6)
        s.draw_ellipse(42, 48, 4, 9, 4)
        s.draw_ellipse(42, 57, 4, 3, 6)
        head_x, head_y = 44, 25
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 2, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 3, head_y - 1, 2, 9)
    else:
        # 5. Relaxing arch
        s.draw_ellipse(32, 60, 21, 3, 15)
        for i in range(13):
            tx = 13 + i//3
            ty = 46 - i
            s.draw_disc(int(tx), int(ty), 3, 4)
            if i in (4, 8): s.draw_disc(int(tx), int(ty), 3, 2)
            if i >= 11: s.draw_disc(int(tx), int(ty), 2, 6)
        s.draw_ellipse(30, 36, 16, 12, 4)
        s.draw_ellipse(18, 50, 4, 8, 3)
        s.draw_ellipse(18, 57, 4, 3, 6)
        s.draw_ellipse(42, 50, 4, 8, 4)
        s.draw_ellipse(42, 57, 4, 3, 6)
        head_x, head_y = 44, 28
        s.draw_disc(head_x, head_y, 12, 4)
        s.draw_disc(head_x + 4, head_y + 2, 4, 4)
        for i in range(7):
            s.draw_disc(head_x - 2 + i//3, head_y - 9 - i, 2, 3)
            s.draw_disc(head_x + 3 + i//3, head_y - 9 - i, 2, 4)
        s.draw_ellipse(head_x + 5, head_y + 3, 4, 3, 6)
        s.set(head_x + 8, head_y + 2, 7)
        s.draw_ellipse(head_x + 2, head_y - 1, 3, 4, 10)
        s.draw_disc(head_x + 3, head_y - 1, 2, 9)

    s.draw_outline()
    return s

def build_eating_cat(frame_idx):
    s = Sprite(f"eat_{frame_idx}")
    b_cycles = [0, 1, 1, 0, 1, 1, 0, 0]
    bob_y = b_cycles[frame_idx % 8]

    s.draw_ellipse(32, 60, 20, 3, 15)

    # Tail
    tail_wave = (frame_idx % 8) * (2.0 * math.pi / 8.0)
    for i in range(12):
        tx = 44 + int(math.sin(i * 0.4 + tail_wave) * 4)
        ty = 54 - i
        s.draw_disc(tx, ty, 3, 4)
        if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
        if i >= 10: s.draw_disc(tx, ty, 2, 6)

    # Torso
    s.draw_ellipse(32, 45 + bob_y, 16, 13, 4)
    for sx in (22, 42):
        s.draw_disc(sx, 44 + bob_y, 2, 2)
        s.draw_disc(sx, 48 + bob_y, 3, 2)
    s.draw_ellipse(32, 47 + bob_y, 8, 10, 6)
    s.draw_ellipse(32, 44 + bob_y, 6, 7, 5)

    # Paws
    s.draw_ellipse(26, 57, 4, 3, 6)
    s.draw_ellipse(38, 57, 4, 3, 6)
    s.draw_ellipse(19, 52 + bob_y, 6, 7, 3)
    s.draw_ellipse(45, 52 + bob_y, 6, 7, 3)

    # Head
    head_y = 29 + bob_y
    s.draw_disc(32, head_y, 15, 4)
    s.draw_disc(20, head_y + 4, 6, 4)
    s.draw_disc(44, head_y + 4, 6, 4)

    for i in range(8):
        s.draw_disc(20 - i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6: s.draw_disc(20 - i//2, head_y - 12 - i, 1, 7)
    for i in range(8):
        s.draw_disc(44 + i//2, head_y - 12 - i, 3 - i//3, 4)
        if i < 6: s.draw_disc(44 + i//2, head_y - 12 - i, 1, 7)

    s.draw_disc(32, head_y - 9, 2, 2)
    s.draw_disc(28, head_y - 8, 1, 2)
    s.draw_disc(36, head_y - 8, 1, 2)

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

    # 8-Phase Eating Animation
    if frame_idx == 0:
        # Looking down at treat on floor
        fx, fy = 32, 54
        s.draw_ellipse(fx, fy, 7, 3, 11)
        s.draw_disc(fx + 6, fy, 2, 11)
        s.set(fx - 3, fy - 1, 12)
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(25, head_y + 1, 2, 9)
        s.draw_disc(39, head_y + 1, 2, 9)
    elif frame_idx in (1, 2):
        # Grabbing & chewing first big crunch
        s.draw_ellipse(32, head_y + 8, 4, 3, 8)
        s.set(31, head_y + 8, 11)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)
        s.set(26, head_y + 12, 11)
        s.set(38, head_y + 11, 11)
    elif frame_idx in (3, 4):
        # Chewing with closed eyes and crumbs
        s.draw_ellipse(32, head_y + 7, 3, 2, 8)
        s.set(32, head_y + 7, 11)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)
        s.set(28, head_y + 11, 11)
    elif frame_idx in (5, 6):
        # Licking whiskers clean!
        s.draw_ellipse(32, head_y + 6, 3, 2, 8)
        for dx in range(5):
            s.set(23 + dx, head_y - 1 - abs(dx - 2), 1)
            s.set(37 + dx, head_y - 1 - abs(dx - 2), 1)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)
    else:
        # Satisfied smile
        s.draw_ellipse(25, head_y, 4, 5, 10)
        s.draw_ellipse(39, head_y, 4, 5, 10)
        s.draw_disc(25, head_y, 2, 9)
        s.draw_disc(39, head_y, 2, 9)
        s.set(24, head_y - 2, 6)
        s.set(38, head_y - 2, 6)
        s.draw_disc(18, head_y + 5, 3, 8)
        s.draw_disc(46, head_y + 5, 3, 8)

    s.draw_outline()
    return s

def build_sleeping_cat(frame_idx):
    s = Sprite(f"sleep_{frame_idx}")
    # 8-phase breathing cycle
    b_cycles = [0, 1, 1, 2, 1, 0, 0, 0]
    breath = b_cycles[frame_idx % 8]

    # Ground shadow
    s.draw_ellipse(32, 57, 24, 4, 15)

    # Tail wrapped around
    for i in range(16):
        tx = 10 + i * 2
        ty = 53 + int(math.sin(i * 0.35) * 2)
        s.draw_disc(tx, ty, 3, 4)
        if i in (5, 9): s.draw_disc(tx, ty, 3, 2)
        if i >= 13: s.draw_disc(tx, ty, 2, 6)

    # Curled Body
    s.draw_ellipse(30, 44 - breath, 18, 12 + breath, 4)
    for sx in (20, 26, 32):
        s.draw_disc(sx, 36 - breath, 2, 2)
        s.draw_disc(sx, 40 - breath, 3, 2)

    s.draw_ellipse(34, 47 - breath, 8, 6, 6)

    # Head resting on paws
    head_x = 42
    head_y = 40 - breath
    s.draw_disc(head_x, head_y, 11, 4)
    s.draw_disc(head_x + 5, head_y + 3, 4, 4)

    for i in range(6):
        s.draw_disc(head_x - 4 + i//3, head_y - 9 - i, 2, 4)
        s.draw_disc(head_x + 4 + i//3, head_y - 9 - i, 2, 4)
        if i < 4:
            s.draw_disc(head_x - 4 + i//3, head_y - 9 - i, 1, 7)
            s.draw_disc(head_x + 4 + i//3, head_y - 9 - i, 1, 7)

    s.draw_ellipse(head_x + 3, head_y + 4, 4, 3, 6)
    s.set(head_x + 6, head_y + 2, 7)

    s.set(head_x - 1, head_y - 1, 1)
    s.set(head_x, head_y, 1)
    s.set(head_x + 1, head_y, 1)
    s.set(head_x + 2, head_y - 1, 1)

    s.set(head_x + 8, head_y + 3, 6)
    s.set(head_x + 9, head_y + 3, 6)
    s.set(head_x + 8, head_y + 5, 6)
    s.set(head_x + 9, head_y + 6, 6)

    s.draw_ellipse(36, 51, 4, 3, 6)

    # Floating Zzz continuously drifting upward
    prog = (frame_idx % 8) / 8.0
    zx = int(24 - prog * 10)
    zy = int(28 - prog * 14)
    s.draw_disc(zx, zy, 2, 14)
    s.draw_disc(zx + 5, zy - 4, 2, 14)
    s.draw_disc(zx + 10, zy - 8, 3, 14)

    s.draw_outline()
    return s

def build_swatting_cat(frame_idx):
    s = Sprite(f"swat_{frame_idx}")

    # Ground shadow beneath rear paws
    s.draw_ellipse(32, 60, 22, 3, 15)

    # High swishing tail
    sway = int(math.sin(frame_idx * 0.9) * 4)
    for i in range(13):
        tx = 14 + sway + (i // 3)
        ty = 53 - i
        s.draw_disc(tx, ty, 3, 4)
        if i in (3, 7): s.draw_disc(tx, ty, 3, 2)
        if i >= 10: s.draw_disc(tx, ty, 2, 6)

    # Rear planted paws & thighs (sitting back on haunches, standing upright on hind legs)
    s.draw_ellipse(22, 54, 7, 6, 3)
    s.draw_ellipse(42, 54, 7, 6, 3)
    s.draw_ellipse(20, 58, 5, 3, 6)
    s.draw_ellipse(44, 58, 5, 3, 6)

    # Tall upright torso
    torso_bob = 1 if (frame_idx in (2, 5)) else 0
    s.draw_ellipse(32, 42 - torso_bob, 12, 13, 4)
    s.draw_ellipse(32, 43 - torso_bob, 7, 10, 6) # white belly

    # Head tilted up to track overhead aircraft
    head_y = 23 - torso_bob
    head_x = 32
    s.draw_disc(head_x, head_y, 13, 4)
    s.draw_disc(head_x - 10, head_y + 3, 5, 4)
    s.draw_disc(head_x + 10, head_y + 3, 5, 4)

    # Ears perked backwards in excitement
    for i in range(7):
        s.draw_disc(head_x - 11 - i//3, head_y - 8 - i, 2, 4)
        if i < 5: s.draw_disc(head_x - 10 - i//3, head_y - 8 - i, 1, 7)
        s.draw_disc(head_x + 11 + i//3, head_y - 8 - i, 2, 4)
        if i < 5: s.draw_disc(head_x + 10 + i//3, head_y - 8 - i, 1, 7)

    # Tabby forehead markings
    s.draw_disc(head_x, head_y - 9, 2, 2)
    s.draw_disc(head_x - 4, head_y - 8, 1, 2)
    s.draw_disc(head_x + 4, head_y - 8, 1, 2)

    # White muzzle & pink nose
    s.draw_ellipse(head_x, head_y + 5, 6, 4, 6)
    s.set(head_x, head_y + 3, 7)
    s.set(head_x - 1, head_y + 3, 7)
    s.set(head_x + 1, head_y + 3, 7)
    s.set(head_x, head_y + 4, 1)

    # Excited eyes looking straight up!
    eye_y = head_y - 2
    s.draw_ellipse(head_x - 5, eye_y, 4, 4, 10)
    s.draw_ellipse(head_x + 5, eye_y, 4, 4, 10)
    s.draw_disc(head_x - 5, eye_y - 2, 2, 9)
    s.draw_disc(head_x + 5, eye_y - 2, 2, 9)
    s.set(head_x - 6, eye_y - 3, 6)
    s.set(head_x + 4, eye_y - 3, 6)

    # Whiskers
    for dy in (-1, 1):
        for w in range(5):
            s.set(head_x - 12 - w, head_y + 4 + dy*w//3, 6)
            s.set(head_x + 12 + w, head_y + 4 + dy*w//3, 6)

    # 8-Frame Dynamic Swatting / Batting / Scratching Paw Arcs
    if frame_idx == 0:
        s.draw_ellipse(20, 30, 4, 5, 4)
        s.draw_ellipse(18, 25, 4, 3, 6)
        s.draw_ellipse(44, 28, 4, 5, 4)
        s.draw_ellipse(46, 22, 4, 3, 6)
    elif frame_idx == 1:
        s.draw_ellipse(20, 28, 4, 4, 4)
        s.draw_ellipse(19, 23, 4, 3, 6)
        for i in range(8):
            s.draw_disc(40 + i//2, 24 - i*2, 3, 4)
        s.draw_ellipse(44, 11, 4, 4, 6)
        s.set(43, 8, 6); s.set(45, 7, 6); s.set(47, 8, 6)
    elif frame_idx == 2:
        for i in range(8):
            s.draw_disc(42 - i, 22 - i, 3, 4)
        s.draw_ellipse(34, 10, 5, 4, 6)
        s.set(31, 7, 11); s.set(34, 6, 11); s.set(37, 7, 11)
        s.draw_ellipse(20, 26, 4, 5, 4)
        s.draw_ellipse(18, 20, 4, 3, 6)
    elif frame_idx == 3:
        for i in range(8):
            s.draw_disc(24 - i//2, 24 - i*2, 3, 4)
        s.draw_ellipse(20, 11, 4, 4, 6)
        s.set(18, 8, 6); s.set(20, 7, 6); s.set(22, 8, 6)
        s.draw_ellipse(42, 25, 4, 5, 4)
        s.draw_ellipse(44, 20, 4, 3, 6)
    elif frame_idx == 4:
        for i in range(8):
            s.draw_disc(22 + i, 22 - i, 3, 4)
        s.draw_ellipse(30, 10, 5, 4, 6)
        s.set(28, 7, 11); s.set(31, 6, 11); s.set(34, 7, 11)
        s.draw_ellipse(44, 24, 4, 5, 4)
        s.draw_ellipse(45, 18, 4, 3, 6)
    elif frame_idx == 5:
        for i in range(7):
            s.draw_disc(25 - i//3, 23 - i*2, 3, 4)
            s.draw_disc(39 + i//3, 23 - i*2, 3, 4)
        s.draw_ellipse(22, 10, 4, 4, 6)
        s.draw_ellipse(42, 10, 4, 4, 6)
        s.set(20, 8, 6); s.set(22, 7, 6); s.set(24, 8, 6)
        s.set(40, 8, 6); s.set(42, 7, 6); s.set(44, 8, 6)
    elif frame_idx == 6:
        for i in range(8):
            s.draw_disc(24 - i//2, 22 - i*2, 3, 4)
        s.draw_ellipse(19, 9, 4, 4, 6)
        s.set(17, 7, 6); s.set(19, 6, 6)
        s.draw_ellipse(43, 20, 4, 5, 4)
        s.draw_ellipse(45, 16, 4, 3, 6)
    else:
        for i in range(8):
            s.draw_disc(40 + i//2, 22 - i*2, 3, 4)
        s.draw_ellipse(45, 9, 4, 4, 6)
        s.set(45, 6, 6); s.set(47, 7, 6)
        s.draw_ellipse(21, 20, 4, 5, 4)
        s.draw_ellipse(19, 16, 4, 3, 6)

    s.draw_outline()
    return s

def build_airplane():
    pixels = [[0 for _ in range(28)] for _ in range(14)]
    for x in range(3, 25):
        pixels[6][x] = 6
        pixels[7][x] = 6
    for x in range(4, 24):
        pixels[8][x] = 14
    pixels[6][25] = 6
    pixels[7][25] = 6
    pixels[6][26] = 6
    pixels[5][21] = 9
    pixels[5][22] = 9
    pixels[6][23] = 9
    for i in range(6):
        pixels[5 - i][11 + i] = 6
        pixels[5 - i][12 + i] = 6
    pixels[0][11] = 13 # Red port wingtip nav light
    for i in range(6):
        pixels[8 + i][11 + i] = 6
        pixels[8 + i][12 + i] = 14
    pixels[13][11] = 10 # Green starboard wingtip nav light
    for x in range(14, 18):
        pixels[4][x] = 5
        pixels[9][x] = 5
    for i in range(5):
        pixels[5 - i][3 + i] = 6
        pixels[5 - i][4 + i] = 6
    pixels[1][3] = 11 # Strobe beacon
    pixels[6][2] = 6
    pixels[7][2] = 6
    pixels[5][1] = 6
    pixels[8][1] = 6
    copy = [row[:] for row in pixels]
    for y in range(14):
        for x in range(28):
            if copy[y][x] != 0:
                for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < 14 and 0 <= nx < 28:
                        if copy[ny][nx] == 0:
                            pixels[ny][nx] = 1
    return pixels

def main():
    sprites = []

    # 1. Walk Cycle (8 frames)
    for i in range(8):
        sprites.append(build_walking_cat(i))

    # 2. Sitting Idle (8 frames)
    for i in range(8):
        sprites.append(build_sitting_cat(i, "idle"))

    # 3. Happy / Petting (8 frames)
    for i in range(8):
        sprites.append(build_sitting_cat(i, "happy"))

    # 4. Watch Sky / Airspace (4 frames)
    for i in range(4):
        sprites.append(build_sitting_cat(i, "watch"))

    # 5. Eating treat (8 frames)
    for i in range(8):
        sprites.append(build_eating_cat(i))

    # 6. Sleeping (8 frames)
    for i in range(8):
        sprites.append(build_sleeping_cat(i))

    # 7. Jumping / Pouncing (8 frames)
    for i in range(8):
        sprites.append(build_jumping_cat(i))

    # 8. Grooming / Face Wash (8 frames)
    for i in range(8):
        sprites.append(build_grooming_cat(i))

    # 9. Stretching (6 frames)
    for i in range(6):
        sprites.append(build_stretching_cat(i))

    # 10. Swatting / Scratching Overhead Aircraft (8 frames)
    for i in range(8):
        sprites.append(build_swatting_cat(i))

    # Save PNG previews
    out_dir = os.path.join(os.path.dirname(__file__), "preview_sprites")
    os.makedirs(out_dir, exist_ok=True)

    sheet_cols = 8
    sheet_rows = (len(sprites) + sheet_cols - 1) // sheet_cols
    sheet = Image.new("RGBA", (sheet_cols * WIDTH, sheet_rows * HEIGHT), (30, 35, 45, 255))

    for idx, s in enumerate(sprites):
        img = s.to_image()
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
        f.write("static const uint8_t* const CAT_WALK_FRAMES[8] = {\n")
        f.write("  CAT_WALK_0, CAT_WALK_1, CAT_WALK_2, CAT_WALK_3,\n")
        f.write("  CAT_WALK_4, CAT_WALK_5, CAT_WALK_6, CAT_WALK_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_IDLE_FRAMES[8] = {\n")
        f.write("  CAT_SIT_IDLE_0, CAT_SIT_IDLE_1, CAT_SIT_IDLE_2, CAT_SIT_IDLE_3,\n")
        f.write("  CAT_SIT_IDLE_4, CAT_SIT_IDLE_5, CAT_SIT_IDLE_6, CAT_SIT_IDLE_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_HAPPY_FRAMES[8] = {\n")
        f.write("  CAT_SIT_HAPPY_0, CAT_SIT_HAPPY_1, CAT_SIT_HAPPY_2, CAT_SIT_HAPPY_3,\n")
        f.write("  CAT_SIT_HAPPY_4, CAT_SIT_HAPPY_5, CAT_SIT_HAPPY_6, CAT_SIT_HAPPY_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_WATCH_FRAMES[4] = {\n")
        f.write("  CAT_SIT_WATCH_0, CAT_SIT_WATCH_1, CAT_SIT_WATCH_2, CAT_SIT_WATCH_3\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_EAT_FRAMES[8] = {\n")
        f.write("  CAT_EAT_0, CAT_EAT_1, CAT_EAT_2, CAT_EAT_3,\n")
        f.write("  CAT_EAT_4, CAT_EAT_5, CAT_EAT_6, CAT_EAT_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_SLEEP_FRAMES[8] = {\n")
        f.write("  CAT_SLEEP_0, CAT_SLEEP_1, CAT_SLEEP_2, CAT_SLEEP_3,\n")
        f.write("  CAT_SLEEP_4, CAT_SLEEP_5, CAT_SLEEP_6, CAT_SLEEP_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_JUMP_FRAMES[8] = {\n")
        f.write("  CAT_JUMP_0, CAT_JUMP_1, CAT_JUMP_2, CAT_JUMP_3,\n")
        f.write("  CAT_JUMP_4, CAT_JUMP_5, CAT_JUMP_6, CAT_JUMP_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_GROOM_FRAMES[8] = {\n")
        f.write("  CAT_GROOM_0, CAT_GROOM_1, CAT_GROOM_2, CAT_GROOM_3,\n")
        f.write("  CAT_GROOM_4, CAT_GROOM_5, CAT_GROOM_6, CAT_GROOM_7\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_STRETCH_FRAMES[6] = {\n")
        f.write("  CAT_STRETCH_0, CAT_STRETCH_1, CAT_STRETCH_2,\n")
        f.write("  CAT_STRETCH_3, CAT_STRETCH_4, CAT_STRETCH_5\n")
        f.write("};\n\n")

        f.write("static const uint8_t* const CAT_SWAT_FRAMES[8] = {\n")
        f.write("  CAT_SWAT_0, CAT_SWAT_1, CAT_SWAT_2, CAT_SWAT_3,\n")
        f.write("  CAT_SWAT_4, CAT_SWAT_5, CAT_SWAT_6, CAT_SWAT_7\n")
        f.write("};\n\n")

        # Airplane sprite for overhead tracking
        airplane_pixels = build_airplane()
        f.write("#define CAT_AIRPLANE_W 28\n")
        f.write("#define CAT_AIRPLANE_H 14\n\n")
        f.write("static const uint8_t CAT_AIRPLANE[28 * 14] PROGMEM = {\n")
        for y in range(14):
            f.write("  ")
            for x in range(28):
                f.write(f"{airplane_pixels[y][x]},")
            f.write("\n")
        f.write("};\n\n")

    print(f"Generated {target_h} successfully.")

if __name__ == "__main__":
    main()
