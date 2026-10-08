import serial
import time
import urllib.request

try:
    urllib.request.urlopen("http://192.168.0.2/api/pet/toggle", data=b'').read()
except Exception as e:
    pass

try:
    ser = serial.Serial()
    ser.port = 'COM9'
    ser.baudrate = 115200
    ser.timeout = 1
    ser.dtr = False
    ser.rts = False
    ser.open()
    start = time.time()
    while time.time() - start < 10:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line:
            print(line)
    ser.close()
except Exception as e:
    print(f"Error: {e}")
