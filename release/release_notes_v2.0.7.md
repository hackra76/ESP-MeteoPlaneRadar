# Release Notes v2.0.7

## 🐱 DigiCat Hi-Res Sprite Overhaul

### Added
- **Professional 64x64 Ginger Cat Sprites (113 frames, 17 animations):**
  All 16 DigiCat animation states replaced with authentic hi-res RGB565 PROGMEM sprites sourced from a professional ginger tabby sprite sheet, correctly mapped using activities.ini:

  | Animation | Row | Description |
  | :--- | :---: | :--- |
  | cat_idle | 20 | Tail wag sit front - primary idle |
  | cat_idle_lick | 13 | Lick paw sitting front |
  | cat_idle_meow | 15 | Meow sit front |
  | cat_idle_scratch | 18 | Scratch sit left paw |
  | cat_yawn | 44 | Yawn sit front - drowsy / pre-sleep |
  | cat_happy | 20 | Tail wag sit front - happy / petted |
  | cat_watch_plane | 23 | Tail wag sit right - watching sky |
  | cat_hiss | 62 | Hiss front right - scared / defensive |
  | cat_walk | 6 | Walk right - patrol / leaving |
  | cat_run | 11 | Running right - entering fast |
  | cat_going_to_sleep | 7 | Going to sleep - transition / stretch |
  | cat_sleep | 46 | Sleep 1 right front - sleep loop |
  | cat_eat | 60 | Eat food stand right |
  | cat_swat | 36 | Right paw swipe stand right - swat at plane |
  | cat_jump | 65 | Jump right |
  | cat_cling | 66 | On hind legs - cling to bezel rim |
  | cat_slide | 29 | Tail wag lie right - slide / surf |

- **Frame interpolation:** Walk (6->12), Happy tail wag (5->10), Sleep breathing (2->10) frames interpolated for fluid motion.
- **Idle personality cycling:** Every 4-10s DigiCat randomly switches between tail-wag sitting (63%), licking paw (20%), meowing (10%), and scratching ear (7%).
- **Sleep transition:** cat_going_to_sleep plays the full lying-down sequence before looping cat_sleep breathing.
- **3x scale rendering (192x192 px):** DigiCat is now 50% larger on screen - precise Y alignment ensures paws land on the runway.
- **Sprite generator tool:** tools/gen_cat_sprites.py automates extraction, interpolation, and PROGMEM header generation.
- **Old palette sprites removed:** CatSprites.h (32 old 16-colour frames) fully retired; airplane sprite preserved in new AirplaneSprite.h.

### Fixed
- **OTA same-version reflash:** isNewerVersion logic changed to >=, allowing re-flash of the same version via OTA.

---

*Flash over OTA:* MeteoPlaneRadar-v2.0.7-ota.bin
*Flash via USB cable:* MeteoPlaneRadar-v2.0.7-factory.bin
