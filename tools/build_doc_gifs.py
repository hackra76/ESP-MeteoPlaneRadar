import os
from PIL import Image
import build_cat_sprites as bcs

def save_gif(frames, path, duration_ms=120, scale=3):
    scaled_frames = []
    for f in frames:
        img = f.to_image()
        # Scale nearest neighbor for crisp pixel art
        w, h = img.size
        scaled = img.resize((w * scale, h * scale), Image.NEAREST)
        scaled_frames.append(scaled)
    
    # Save animated GIF
    scaled_frames[0].save(
        path,
        save_all=True,
        append_images=scaled_frames[1:],
        optimize=False,
        duration=duration_ms,
        loop=0,
        disposal=2
    )
    print(f"Generated {path}")

def main():
    target_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "docs", "media"))
    os.makedirs(target_dir, exist_ok=True)

    # 1. Walk GIF
    walk_frames = [bcs.build_walking_cat(i) for i in range(8)]
    save_gif(walk_frames, os.path.join(target_dir, "digicat_walk.gif"), duration_ms=90)

    # 2. Jump GIF
    jump_frames = [bcs.build_jumping_cat(i) for i in range(8)]
    save_gif(jump_frames, os.path.join(target_dir, "digicat_jump.gif"), duration_ms=80)

    # 3. Groom / Face Wash GIF
    groom_frames = [bcs.build_grooming_cat(i) for i in range(8)]
    save_gif(groom_frames, os.path.join(target_dir, "digicat_groom.gif"), duration_ms=220)

    # 4. Stretch GIF
    stretch_frames = [bcs.build_stretching_cat(i) for i in range(6)]
    save_gif(stretch_frames, os.path.join(target_dir, "digicat_stretch.gif"), duration_ms=450)

    # 5. Happy / Purr GIF
    happy_frames = [bcs.build_sitting_cat(i, "happy") for i in range(8)]
    save_gif(happy_frames, os.path.join(target_dir, "digicat_happy.gif"), duration_ms=110)

    # 6. Eat Snack GIF
    eat_frames = [bcs.build_eating_cat(i) for i in range(8)]
    save_gif(eat_frames, os.path.join(target_dir, "digicat_eat.gif"), duration_ms=140)

    # 7. Sleep Loaf GIF
    sleep_frames = [bcs.build_sleeping_cat(i) for i in range(8)]
    save_gif(sleep_frames, os.path.join(target_dir, "digicat_sleep.gif"), duration_ms=400)

    # 8. Composite Showcase Strip PNG
    actions = [
        ("Walk", walk_frames[0]),
        ("Jump", jump_frames[1]),
        ("Groom", groom_frames[2]),
        ("Stretch", stretch_frames[0]),
        ("Purr", happy_frames[1]),
        ("Eat", eat_frames[1]),
        ("Sleep", sleep_frames[1]),
    ]
    strip = Image.new("RGBA", (len(actions) * 64 * 3, 64 * 3), (20, 24, 32, 255))
    for idx, (lbl, f) in enumerate(actions):
        img = f.to_image().resize((64 * 3, 64 * 3), Image.NEAREST)
        strip.paste(img, (idx * 64 * 3, 0), img)
    strip_path = os.path.join(target_dir, "digicat_showcase.png")
    strip.save(strip_path)
    print(f"Generated {strip_path}")

if __name__ == "__main__":
    main()
