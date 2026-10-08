import serial
import time
import re

ip = None
try:
    with serial.Serial('COM9', 115200, timeout=1) as ser:
        ser.setDTR(False)
        ser.setRTS(True)
        time.sleep(0.1)
        ser.setDTR(True)
        ser.setRTS(False)
        
        start = time.time()
        while time.time() - start < 15:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(line)
                if "IP:" in line or "http://" in line:
                    match = re.search(r'http://([\d\.]+)', line)
                    if match:
                        ip = match.group(1)
                        print(f"FOUND IP: {ip}")
                        break
                    match2 = re.search(r'IP:\s*([\d\.]+)', line)
                    if match2:
                        ip = match2.group(1)
                        print(f"FOUND IP: {ip}")
                        break
                        
        if ip:
            import urllib.request
            req = urllib.request.Request(f"http://{ip}/api/screen", data=b'index=4') # screen 4 = pet screen probably, let's just trigger it. Or we can just read the serial.
            print("Triggering screen via IP...")
except Exception as e:
    print(f"Error: {e}")
