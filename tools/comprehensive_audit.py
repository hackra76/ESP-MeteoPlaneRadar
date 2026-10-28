import urllib.request
import urllib.parse
import json
import time
import os
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else "192.168.0.2"
BASE_URL = f"http://{DEVICE_IP}"
ARTIFACT_DIR = os.getenv("AUDIT_OUT_DIR", os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "audit_output"))
os.makedirs(ARTIFACT_DIR, exist_ok=True)

def http_get(path, timeout=10):
    url = f"{BASE_URL}{path}"
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read(), resp.status

def http_post(path, data=None, timeout=10):
    url = f"{BASE_URL}{path}"
    body = json.dumps(data).encode('utf-8') if data is not None else b"{}"
    req = urllib.request.Request(url, data=body, headers={'Content-Type': 'application/json'}, method='POST')
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.read(), resp.status

def save_screenshot(filename):
    url = f"{BASE_URL}/api/screenshot.bmp"
    req = urllib.request.Request(url)
    dest_path = os.path.join(ARTIFACT_DIR, filename)
    with urllib.request.urlopen(req, timeout=15) as resp:
        content = resp.read()
        with open(dest_path, "wb") as f:
            f.write(content)
    print(f"  [Screenshot] Saved {filename} ({len(content)} bytes)")
    return dest_path

def get_status():
    raw, status = http_get("/api/status")
    return json.loads(raw.decode('utf-8'))

def set_screen(idx):
    http_post("/api/screen", {"index": idx})
    time.sleep(1.2)  # Wait for LVGL & rendering to settle

def set_range(step):
    http_post("/api/range", {"step": step})
    time.sleep(1.2)

def send_input(cmd, extra=None):
    payload = {"cmd": cmd}
    if extra:
        payload.update(extra)
    http_post("/api/input", payload)
    time.sleep(0.8)

def test_all_screens():
    screens = [
        (0, "Clock", "audit_scr0_clock_v2.png"),
        (1, "Planes Radar", "audit_scr1_planes_v2.png"),
        (2, "Meteo Radar 100km", "audit_scr2_meteo_100km_v2.png"),
        (3, "Tactical Radar", "audit_scr3_tactical_v2.png"),
        (4, "Sonar", "audit_scr4_sonar_v2.png"),
        (5, "Forecast", "audit_scr5_forecast_v2.png"),
        (6, "Finance", "audit_scr6_finance_v2.png"),
        (7, "ISS Tracker", "audit_scr7_iss_v2.png"),
        (8, "YouTube", "audit_scr8_youtube_v2.png"),
        (9, "Klipper 3D", "audit_scr9_klipper_v2.png"),
        (10, "System Info", "audit_scr10_info_v2.png"),
        (11, "Settings", "audit_scr11_settings_v2.png"),
    ]
    print("\n--- PHASE 1: SCREEN-BY-SCREEN AUDIT ---")
    for idx, name, filename in screens:
        print(f"Testing Screen {idx}: {name}...")
        set_screen(idx)
        st = get_status()
        print(f"  Active screen reported: {st.get('screen')}, Heap: {st.get('heap')}, PSRAM: {st.get('psram')}")
        bmp_file = filename.replace(".png", ".bmp")
        save_screenshot(bmp_file)

    # Test Meteo Zoom levels
    print("\nTesting Meteo Zoom Levels (Screen 2)...")
    set_screen(2)
    # Range minus twice to reach 25km
    set_range(-1)
    set_range(-1)
    save_screenshot("audit_meteo_25km_v2.bmp")
    set_range(1)
    save_screenshot("audit_meteo_50km_v2.bmp")
    set_range(1)
    save_screenshot("audit_meteo_100km_v2.bmp")
    set_range(1)
    save_screenshot("audit_meteo_200km_v2.bmp")

def test_interactive_features():
    print("\n--- PHASE 2: INTERACTIVE FEATURES & OVERLAYS ---")
    set_screen(2) # Meteo
    print("Testing Quick Control toggle...")
    send_input("qc_toggle")
    time.sleep(0.5)
    save_screenshot("audit_quick_control_v2.bmp")
    send_input("qc_toggle") # close
    time.sleep(0.5)

    print("Testing Pet drawer toggle...")
    http_post("/api/pet/toggle")
    time.sleep(0.5)
    save_screenshot("audit_pet_open_v2.bmp")
    print("Testing Pet tap interaction...")
    send_input("pet_tap", {"x": 240, "y": 200})
    time.sleep(0.5)
    http_post("/api/pet/toggle") # close
    time.sleep(0.5)

    print("Testing Legends toggle...")
    send_input("toggle_legends")
    time.sleep(0.5)
    send_input("toggle_legends")
    time.sleep(0.5)

    print("Testing Buzzer test...")
    try:
        http_post("/api/buzzer/test")
        print("  Buzzer test triggered successfully.")
    except Exception as e:
        print(f"  Buzzer test error: {e}")

def test_high_load():
    print("\n--- PHASE 3: HIGH-LOAD & CONCURRENCY STRESS TEST ---")
    st_start = get_status()
    print(f"Initial Health: Uptime={st_start.get('uptime')}, Heap={st_start.get('heap')}, PSRAM={st_start.get('psram')}")

    # 1. Rapid API concurrency hammering (80 requests across 8 threads)
    print("Running 80 concurrent API calls (status, screenshot, config, hardware)...")
    endpoints = ["/api/status", "/api/hardware", "/api/config", "/api/stats"] * 20
    def fetch_ep(ep):
        try:
            r, s = http_get(ep, timeout=10)
            return s == 200
        except Exception:
            return False

    with ThreadPoolExecutor(max_workers=8) as ex:
        futures = [ex.submit(fetch_ep, ep) for ep in endpoints]
        successes = sum(1 for f in as_completed(futures) if f.result())
    print(f"  Concurrent API Requests: {successes}/80 succeeded ({successes/80*100:.1f}%)")

    # 2. Screen switching stress test (rapidly switch 20 times)
    print("Rapid Screen Switching Stress (20 iterations)...")
    for i in range(20):
        target = i % 12
        try:
            http_post("/api/screen", {"index": target}, timeout=5)
            time.sleep(0.3)
        except Exception as e:
            print(f"  Error at iteration {i}: {e}")
    print("  Completed 20 rapid screen switches.")

    # 3. Settings rapid writes stress test (debounced flush verification)
    print("Rapid Settings Writes Stress (10 iterations)...")
    for i in range(10):
        try:
            # Alternate brightness between 180 and 220
            http_post("/api/config", {"bri": 180 + (i % 2) * 40}, timeout=5)
            time.sleep(0.1)
        except Exception as e:
            print(f"  Error setting config: {e}")
    print("  Completed 10 rapid settings updates.")

    # Settle down
    print("Waiting 5s for background tasks and debounced Settings_Commit to settle...")
    time.sleep(5)

    st_end = get_status()
    print(f"Post-Stress Health: Uptime={st_end.get('uptime')}, Heap={st_end.get('heap')}, PSRAM={st_end.get('psram')}")
    heap_diff = st_end.get('heap', 0) - st_start.get('heap', 0)
    psram_diff = st_end.get('psram', 0) - st_start.get('psram', 0)
    print(f"Memory Delta: Heap diff = {heap_diff:+d} B, PSRAM diff = {psram_diff:+d} B")

def check_serial_logs():
    print("\n--- PHASE 4: SERIAL ERROR AUDIT ---")
    try:
        raw, _ = http_get("/api/serial/read?since=0")
        doc = json.loads(raw.decode('utf-8'))
        logs = doc.get("data", "")
        print(f"Retrieved {len(logs)} bytes of in-memory serial logs.")
        errors = [line for line in logs.splitlines() if any(k in line.lower() for k in ["error", "panic", "abort", "corrupt", "wdt", "assert", "fail"])]
        print(f"Found {len(errors)} error/warning occurrences in serial ring buffer:")
        for err in errors[-15:]:
            print(f"  {err}")
    except Exception as e:
        print(f"Failed to read serial logs: {e}")

if __name__ == "__main__":
    test_all_screens()
    test_interactive_features()
    test_high_load()
    check_serial_logs()
    print("\nAUDIT COMPLETE!")
