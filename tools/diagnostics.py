import urllib.request
import json
import time
import os
import threading
import serial
from PIL import Image

DEVICE_IP = "192.168.0.2"
BASE_URL = f"http://{DEVICE_IP}"

serial_lines = []
serial_running = True

def serial_monitor():
    global serial_running
    try:
        with serial.Serial('COM9', 115200, timeout=1) as ser:
            while serial_running:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    ts = time.strftime('%H:%M:%S')
                    serial_lines.append(f"[{ts}] {line}")
    except Exception as e:
        serial_lines.append(f"[SERIAL ERROR] {e}")

monitor_thread = threading.Thread(target=serial_monitor, daemon=True)
monitor_thread.start()

def api_post(endpoint, data=None):
    url = f"{BASE_URL}{endpoint}"
    body = json.dumps(data).encode('utf-8') if data is not None else b''
    req = urllib.request.Request(url, data=body, headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(req, timeout=8) as resp:
        return json.loads(resp.read().decode('utf-8'))

def api_get(endpoint):
    url = f"{BASE_URL}{endpoint}"
    with urllib.request.urlopen(url, timeout=8) as resp:
        return json.loads(resp.read().decode('utf-8'))

def take_screenshot(filename):
    url = f"{BASE_URL}/api/screenshot.bmp"
    bmp_path = filename.replace('.png', '.bmp')
    with urllib.request.urlopen(url, timeout=15) as resp:
        open(bmp_path, 'wb').write(resp.read())
    Image.open(bmp_path).save(filename)
    if os.path.exists(bmp_path):
        os.remove(bmp_path)
    print(f"  [Screenshot] Saved {filename}")

print("=== Starting Full ESP-MeteoPlaneRadar Device Diagnostics ===")

# 1. Status Check
status = api_get("/api/status")
print(f"Firmware: {status.get('version')}")
print(f"Uptime: {status.get('uptime')}")
print(f"Free Heap: {status.get('heap')} B | Free PSRAM: {status.get('psram')} B")
print(f"Current Screen: {status.get('screen')}")

# 2. Cycle each screen 0 to 9
screen_names = [
    "Clock", "Planes", "Meteo", "Tactical", "Forecast",
    "Finance", "ISS", "YouTube", "Info", "Settings"
]

results = []

for idx, name in enumerate(screen_names):
    print(f"\n--- Testing Screen {idx}: {name} ---")
    try:
        resp = api_post("/api/screen", {"index": idx})
        time.sleep(1.2)
        st = api_get("/api/status")
        cur_scr = st.get("screen")
        match = (cur_scr == idx)
        print(f"  Switch command ok: {resp.get('ok')}, Reported active screen: {cur_scr} (Match: {match})")
        scr_file = f"tools/screen_{idx}_{name.lower()}.png"
        take_screenshot(scr_file)
        results.append({
            "screen_idx": idx,
            "name": name,
            "match": match,
            "file": scr_file,
            "heap": st.get("heap"),
            "psram": st.get("psram")
        })
    except Exception as e:
        print(f"  [ERROR] Screen {idx} failed: {e}")
        results.append({"screen_idx": idx, "name": name, "error": str(e)})

# 3. Test DigiPet overlay
print("\n--- Testing DigiPet Drawer Overlay ---")
try:
    pet_resp = api_post("/api/pet/toggle")
    print(f"  Pet toggle response: {pet_resp}")
    time.sleep(1.0)
    st = api_get("/api/status")
    print(f"  Pet Open: {st.get('petOpen')}")
    take_screenshot("tools/screen_pet_open.png")
    
    # Close pet
    api_post("/api/pet/toggle")
    time.sleep(0.5)
    st = api_get("/api/status")
    print(f"  Pet Closed: {not st.get('petOpen')}")
except Exception as e:
    print(f"  [ERROR] DigiPet test failed: {e}")

# 4. Test Range stepping on active screen (switch to Planes screen first)
print("\n--- Testing Range Zoom Control ---")
try:
    api_post("/api/screen", {"index": 1})
    time.sleep(0.5)
    st1 = api_get("/api/status")
    r1 = st1.get("range")
    api_post("/api/range", {"step": 1})
    time.sleep(0.5)
    st2 = api_get("/api/status")
    r2 = st2.get("range")
    api_post("/api/range", {"step": -1})
    time.sleep(0.5)
    st3 = api_get("/api/status")
    r3 = st3.get("range")
    print(f"  Range progression: {r1} -> {r2} -> {r3}")
except Exception as e:
    print(f"  [ERROR] Range test failed: {e}")

# Stop serial monitoring and analyze logs
time.sleep(1)
serial_running = False

print("\n=== Serial Log Summary (Recent lines) ===")
err_count = 0
for l in serial_lines[-35:]:
    print(" ", l)
    if "error" in l.lower() or "panic" in l.lower() or "abort" in l.lower() or "wdt" in l.lower():
        err_count += 1

print(f"\nDiagnostics completed. Potential error lines detected in serial: {err_count}")
with open("tools/diagnostics_result.json", "w") as f:
    json.dump({"results": results, "errors": err_count, "logs": serial_lines}, f, indent=2)
print("Results saved to tools/diagnostics_result.json")
