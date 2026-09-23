# Release v2.0.1: Web Remote Control for DigiCat Companion

This update adds remote control support for **DigiCat** directly from your browser's Web Dashboard.

---

### What's New in v2.0.1
- **Web Remote Toggle Button:** Added a dedicated show/hide button in the **Remote Control (Diaľkový ovládač)** card on the web dashboard to open or close the DigiCat pet drawer without touching the screen.
- **Realtime Status Sync:** The dashboard dynamically indicates whether DigiCat is open (`+ 🐾 DigiCat`) and updates the button style and label in real time.
- **Full Localization:** Complete translations across English, Slovak, and Czech for button actions, tooltips, and status notifications.
- **REST Endpoint:** Added `/api/pet/toggle` for clean, frame-synchronized pet drawer toggling.

---

## 📦 Firmware Binaries Included

| File | Type | Description |
| :--- | :--- | :--- |
| `MeteoPlaneRadar-v2.0.1-factory.bin` | Full Flash | Complete firmware image for clean flashing via esptool / ESP Web Flasher (`0x00000`) |
| `MeteoPlaneRadar-v2.0.1-ota.bin` | OTA Update | Direct firmware update via Web UI at `http://<device-ip>/update` or GitHub OTA |

---

### How to Update
1. **GitHub OTA (One-Click):** Open your device dashboard (`http://meteoplaneradar.local/`), navigate to **Shared Settings (Spoločné nastavenia)** -> **Firmware Update**, click **Check for updates**, and then **Update from GitHub**.
2. **Manual Web OTA:** Navigate to `http://<device-ip>/update` and upload `MeteoPlaneRadar-v2.0.1-ota.bin`.
