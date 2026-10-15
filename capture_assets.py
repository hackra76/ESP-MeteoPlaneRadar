import urllib.request
import json
import time
import os
import io
from PIL import Image, ImageDraw

BASE_URL = 'http://192.168.0.6'
MEDIA_DIR = 'docs/media'
os.makedirs(MEDIA_DIR, exist_ok=True)

def post_json(endpoint, payload):
    url = f'{BASE_URL}{endpoint}'
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(req, timeout=8) as resp:
        return resp.read().decode('utf-8')

def download_screenshot(retries=3, delay=1.0):
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(f'{BASE_URL}/api/screenshot.bmp', timeout=10) as resp:
                data = resp.read()
                if len(data) >= 600000:
                    return Image.open(io.BytesIO(data))
        except Exception as e:
            print(f'Attempt {attempt+1} screenshot error: {e}')
            time.sleep(delay)
    raise RuntimeError('Failed to download valid screenshot')

def make_round_display(im):
    im = im.convert('RGBA')
    w, h = im.size
    scale = 4
    mask = Image.new('L', (w * scale, h * scale), 0)
    draw = ImageDraw.Draw(mask)
    # 1 pixel margin inside border for clean antialiasing
    draw.ellipse([(scale, scale), (w * scale - scale, h * scale - scale)], fill=255)
    mask = mask.resize((w, h), Image.Resampling.LANCZOS)
    im.putalpha(mask)
    return im

def capture_screen(screen_idx, out_filename, wait_sec=6, pre_action=None):
    print(f'--> Switching to screen {screen_idx} (waiting {wait_sec}s)...')
    post_json('/api/screen', {'index': screen_idx})
    time.sleep(wait_sec)
    if pre_action:
        pre_action()
    im = download_screenshot()
    round_im = make_round_display(im)
    dest_path = os.path.join(MEDIA_DIR, out_filename)
    round_im.save(dest_path, optimize=True)
    print(f'Saved {dest_path} ({os.path.getsize(dest_path):,} bytes)')
    return im

def main():
    print('Starting automated screen capture from device...')
    
    # 0. Screen 0: Luxury Astronomical Chronograph
    print('\n[0/11] Clock: Luxury Astronomical Chronograph (Screen 0)')
    capture_screen(0, 'clock_luxury.png', wait_sec=4)

    # 1. Screen 1: Aircraft Radar
    print('\n[1/11] Aircraft Radar (Screen 1)')
    capture_screen(1, 'plane_radar_live.png', wait_sec=6)

    # 1b. Screen 1: Aircraft Detail with Photo
    print('\n[1b/11] Aircraft Detail with Live Photo')
    def trigger_photo():
        print('Triggering plane selection for detail & photo...')
        post_json('/api/input', {'cmd': 'select_plane'})
        print('Waiting 8 seconds for photo rendering...')
        time.sleep(8)
    capture_screen(1, 'plane_detail_photo.png', wait_sec=4, pre_action=trigger_photo)

    # 2. Screen 2: Weather Radar
    print('\n[2/11] Weather Radar (Screen 2)')
    capture_screen(2, 'weather_radar_live.png', wait_sec=6)

    # 3. Screen 3: Tactical Radar (Planes + Rain)
    print('\n[3/11] Tactical Radar (Screen 3)')
    capture_screen(3, 'tactical_radar_live.png', wait_sec=6)

    # 4. Screen 4: Tactical Sonar
    print('\n[4/11] Tactical Sonar (Screen 4)')
    capture_screen(4, 'screen_sonar.png', wait_sec=5)

    # 5. Screen 5: Forecast Screen
    print('\n[5/11] Forecast Screen (Screen 5)')
    capture_screen(5, 'forecast_screen.png', wait_sec=5)

    # 6. Screen 6: Markets & Crypto
    print('\n[6/11] Financial Markets & Crypto (Screen 6)')
    capture_screen(6, 'finance_screen.png', wait_sec=5)

    # 7. Screen 7: ISS Tracker
    print('\n[7/11] ISS Tracker (Screen 7)')
    capture_screen(7, 'screen_iss_live.png', wait_sec=5)

    # 8. Screen 8: YouTube Analytics
    print('\n[8/11] YouTube Analytics (Screen 8)')
    capture_screen(8, 'youtube_screen.png', wait_sec=5)

    # 9. Screen 9: Klipper 3D Printer
    print('\n[9/11] Klipper 3D Printer HUD (Screen 9)')
    capture_screen(9, 'screen_klipper.png', wait_sec=5)

    # 10. Screen 10: Flight Stats
    print('\n[10/11] Flight Stats & Info (Screen 10)')
    capture_screen(10, 'flight_stats_screen.png', wait_sec=5)

    # 11. Screen 11: Settings Screen
    print('\n[11/11] Settings Screen (Screen 11)')
    capture_screen(11, 'settings_screen.png', wait_sec=5)

    # Switch back to Clock or Aircraft Radar
    post_json('/api/screen', {'index': 0})
    print('\nAll screens captured successfully!')


if __name__ == '__main__':
    main()
