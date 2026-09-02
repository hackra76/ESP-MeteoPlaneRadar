# MeteoPlaneRadar (Slovenská verzia s Taktickým radarom a rozšírenými ciferníkmi)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteo-radarová stanica, živý letecký radar, zrážkový meteoradar, taktický kombinovaný radar a dizajnové hodiny na okrúhlom 2.1" IPS dotykovom displeji.**  
Vyvinuté pre hardvérovú dosku **Waveshare ESP32-S3-Touch-LCD-2.1** s kompletnou konfiguráciou a diaľkovým ovládaním cez responzívny webový prehliadač.

> 🇸🇰 Tento projekt je pokročilým forkom a významným rozšírením pôvodného projektu **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)** od **[chiptron.cz](https://chiptron.cz)**.  
> 🇬🇧 English version: **[README_EN.md](README_EN.md)**

---

## 🌟 Kľúčové funkcie a inovácie v tomto forku

### 🕒 1. Bohatá kolekcia unikátnych ciferníkov hodín
Displej ponúka až **7 úplne odlišných geometrických štýlov ciferníka** s okamžitým prepínaním cez web alebo jednoduchým poklepaním na telo prístroja:
1. **Digitálny klasický (Classic Digital)** – čistý horizontálny čas s u8g2 fontom, dátumom, vycentrovaným počasím, vetrom a fázou mesiaca.
2. **Letecký kokpitový analóg (Aviator Cockpit)** – autentické pilotné hodinky s luminiscenčnými ručičkami, hodinovými indexmi a dvoma sub-ciferníkmi (počasie vľavo na 9:00, fáza mesiaca vpravo na 3:00).
3. **🚀 Planetárne prstence (Orbital Gauges)** – sci-fi dizajn tvorený tromi sústrednými kruhovými oblúkmi (minúty, hodiny, sekundy) s dorastajúcimi svetelnými perlami a centrálnym informačným jadrom.
4. **🛩️ Stíhací priehľadový displej (Fighter HUD)** – priehľadový Head-Up Display z bojového lietadla s umelým horizontom (pitch ladder -10°/0°/+10°), zameriavacím boresight krížom, časom uzamknutým v taktickom rámčeku (`SYS·TGT·LOCK`), hornou kompasovou páskou `HDG 360` a bočnými indikátormi teploty a vetra.
5. **⏱️ Astronomický regulátor (Régulateur Chrono)** – mechanický observatórny chronometer s oddelenými osami: veľká centrálna minútová ručička cez celý obvod displeja, samostatný horný hodinový sub-ciferník (o 12:00) a dolný sekundový sub-ciferník (o 6:00) s bočnými komplikáciami.
6. **🔲 Vertikálna typografia (Stacked Bold)** – moderný smartwatch dizajn (Pixel / Nothing) s obrovským dvojčíslom hodín hore, obrovskými minútami dolu a zaoblenými kapsulami s počasím a mesiacom po stranách.
7. **Nordic minimalistický (Minimal)** – masívny, vysoko kontrastný čas čitateľný cez celú izbu.

### ⏱️ 2. Zobrazenie sekúnd na okrúhlom obvode
Až **7 štýlov sekundového prstenca**:
- `Vypnuté` (čistý obvod)
- `Bodka` (klasický obiehajúci bod)
- `Plynulý oblúk` (dorastajúci neónový kruh)
- `Pulz` (dýchajúca žiara)
- `Radarový lúč (Sweep)` (rotujúci primárny radarový lúč s plynulým doznievaním)
- `Hodinárske indexy (Ticks)` (60 precíznych švajčiarskych indexov)
- `Satelit na orbite (Orbit)` (družica obiehajúca ciferník so solárnymi panelmi)

### ✋ 3. Hardvérové gestá poklepania (Double-Tap cez 6-osové IMU)
Vstavaný čip **QMI8658** sníma otrasy a zrýchlenie:
- **Na obrazovke Hodín:** Dvojité poklepanie (Double-Tap) na rámček alebo položený stôl **okamžite prepína medzi jednotlivými ciferníkmi**.
- **Na radaroch (Lietadlá, Meteoradar, Taktický):** Dvojité poklepanie prepína **Čistý režim** (skryje legendy, textové popisy letov a názvy miest, takže vidíte iba čistú mapu / zrážky).

### 🛩️ 4. Pokročilé sledovanie letov & Núdzové stavy (ADS-B)
- **Vojenské lety:** Automatická detekcia vojenských letov so špeciálnou ikonou stíhačky s delta krídlami v žiarivo červenej farbe.
- **Núdzové lety (Squawk 7500, 7600, 7700):** Ak sa v dosahu objaví lietadlo v núdzi, mapa automaticky skryje ostatné lety, zameria a sleduje núdzové lietadlo a zobrazí živú telemetriu (výška, rýchlosť, klesanie/stúpanie).
- **Vektor k najbližšiemu lietadlu:** Dynamická čiara ukazujúca smer, vzdialenosť a prevýšenie k najbližšiemu lietadlu nad vašou hlavou.
- **Letiská & Databáza trás:** Zobrazenie letísk v okolí a offline dekódovanie letových trás z volacích znakov (napr. `Burgas -> Warsaw [BOJ>WAW]`).

### 🛰️ 5. Taktický radar (`ScreenTactical`)
Unikátna obrazovka kombinujúca **zrážkový radar (ČHMÚ / RainViewer) v pozadí** a **letovú prevádzku v reálnom čase v popredí**. Leteckí nadšenci tak okamžite vidia, ako piloti oblietavajú búrkové jadrá.

### 🕒 6. Hardware RTC čip (PCF85063) & Časová nezávislosť
- Automatická synchronizácia palubného RTC čipu s internetovým časom.
- Po odpojení od napájania beží čas ďalej (podpora pripojenia 1.0F / 1.5F 3.3V superkondenzátora na piny `BAT` a `GND`).
- Webový indikátor straty napájania (`OSF - Oscillator Stop Flag`).
- Tlačidlá vo webovom rozhraní pre okamžitú synchronizáciu s NTP serverom alebo priamo z hodín internetového prehliadača.

### ⚡ 7. Bezpečná dvojjadrová FreeRTOS architektúra (Dual-Core)
- **Jadro 1 (Core 1):** Vyhradené výhradne pre plynulé vykresľovanie displeja ST7701 (dvojitý framebuffer bez blikania), čítanie dotyku CST820 a detekciu gest z IMU.
- **Jadro 0 (Core 0):** Beží na ňom asynchrónny worker (`AsyncNetWorker`), ktorý na pozadí spracováva TLS šifrovanie, sťahuje radarové dlaždice, komunikuje s ADS-B API a obsluhuje webový server.
- Zbernica I2C je chránená rekurzívnym mutexom s časovým limitom, čo zaručuje nulové kolízie jadier.

---

## 📱 Prehľad obrazoviek

| Obrazovka | Popis | Zdroj dát |
| :--- | :--- | :--- |
| **1. Hodiny** | 7 voliteľných ciferníkov, 7 štýlov sekúnd, počasie, vietor, fáza mesiaca a 24h solárny oblúk | Open-Meteo & Astro engine |
| **2. Lietadlá** | Živá radarová mapa letov, trasy lietadiel, letiská, vzdialenostné kružnice, núdzové lety | adsb.fi / adsb.lol |
| **3. Meteoradar** | Animovaný zrážkový radar s 6-snímkovou animáciou vývoja oblačnosti | ČHMÚ (CZ/SK) alebo RainViewer (Svet) |
| **4. Taktický radar** | **Kombinovaný pohľad:** Zrážková oblačnosť + lietadlá na jednej mape v reálnom čase | RainViewer / ČHMÚ + adsb.fi |
| **5. Predpoveď** | 6-hodinový detail, 3-dňový výhľad, kvalita ovzdušia (AQI, PM2.5) a peľová situácia | Open-Meteo Weather & Air Quality |
| **6. Nastavenia** | Stav zariadenia, IP adresa, jas, orientácia mapy, voľba jazyka a správa WiFi | Systém |

---

## 🔧 Hardvérové špecifikácie

Projekt je navrhnutý priamo pre modul **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Komponent | Špecifikácia |
| :--- | :--- |
| **MCU** | Espressif ESP32-S3R8 (Xtensa® Dual-Core 32-bit LX7, 240 MHz) |
| **Pamäť** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Displej** | 2.1" IPS kruhový displej, 480×480 px, 65k RGB565 farieb, ST7701 RGB interface |
| **Dotyk** | CST820 / CHSC6540 kapacitný dotykový panel (I2C) |
| **I/O Expandér** | TCA9554PWR (ovládanie napájania a resetu displeja) |
| **IMU senzor** | QMI8658 6-osový akcelerometer & gyroskop (detekcia poklepania a orientácie) |
| **RTC čip** | PCF85063 reálny čas s nízkou spotrebou (I2C adresa `0x51`) |
| **Konektivita** | USB-C (napájanie + sériový port), Wi-Fi 802.11 b/g/n (2.4 GHz) |

### Zálohovanie RTC hodín (Batéria / Superkondenzátor)
Na zadnej strane dosky sú spájkovacie plôšky **BAT** a **GND**. Odporúča sa pripojiť:
- **3.3V Superkondenzátor (1.0 F až 1.5 F):** Udrží čas RTC čipu počas niekoľkých dní až týždňov bez batérie.
- **Li-Ion akumulátor:** Cez nabíjací obvod.

---

## 🚀 Inštalácia a prvé spustenie

### 1. Nahratie firmvéru cez PlatformIO
1. Otvorte projekt vo **Visual Studio Code** s rozšírením **PlatformIO**.
2. Pripojte dosku k PC cez USB-C kábel.
3. V termináli spustite:
   ```bash
   # Zostavenie firmvéru
   pio run

   # Nahratie do dosky
   pio run -t upload
   ```

### 2. Pripojenie na Wi-Fi (Captive Portal)
1. Pri prvom zapnutí vytvorí zariadenie prístupový bod **`MeteoPlaneRadar`** a na displeji zobrazí QR kód.
2. Naskenujte QR kód telefónom alebo sa pripojte na Wi-Fi sieť `MeteoPlaneRadar`.
3. Otvorte v prehliadači adresu `http://192.168.4.1/`.
4. Zvoľte vašu domácu sieť, zadajte heslo a uložte.
5. Zariadenie sa pripojí na vašu sieť a zobrazí svoju novú IP adresu.

### 3. Webový ovládací dashboard
Po pripojení otvorte v prehliadači:
- **`http://meteoplaneradar.local/`** (alebo pridelenú IP adresu, napr. `http://192.168.0.7/`).

Webový panel umožňuje:
- **Geolokácia:** Automatická detekcia polohy cez GeoIP alebo manuálne vyhľadanie mesta.
- **Vzhľad & Ciferníky:** Voľba štýlu ciferníka (Digitálny, Analógový, Orbital, HUD, Regulátor, Stacked, Minimalistický) a štýlu sekúnd.
- **Granulárne prepínače widgetov:** Samostatné zapínanie dátumu, počasia, vetra, mesiaca, letových chvostov, letísk a kružníc.
- **Režim hodín v noci (`nightClockOnly`):** Automatické zastavenie striedania obrazoviek v noci – displej zostane trvalo stlmený iba na hodinách.
- **Hardware diagnostika:** Živý I2C Bus Scanner, stav RTC hodín s manuálnou synchronizáciou z NTP alebo prehliadača.
- **Diaľkový ovládač:** Prepínanie obrazoviek a zmena zoomu bez dotyku displeja.
- **OTA aktualizácia:** Pohodlné nahrávanie nových verzií firmvéru cez Wi-Fi bez kábla.

---

## 🖐️ Ovládanie gestami na displeji

| Gesto / Akcia | Funkcia |
| :--- | :--- |
| **Dvojklik (Double-Tap na telo prístroja)** | **Na hodinách:** Prepne na ďalší ciferník.<br>**Na radaroch:** Prepne čistý režim (skryje legendy, texty a popisy). |
| **Potiahnutie doľava / doprava (Swipe)** | Zmena polomeru radaru (10/25 km, 50 km, 100 km, 200 km, Celá krajina). |
| **Klepnutie na lietadlo (Tap)** | Otvorenie detailu letu (trasa letu, výška, rýchlosť, stúpanie, squawk). |
| **Klepnutie na spodok displeja** | Cyklovanie úrovní priblíženia. |
| **Dlhý stisk vľavo / vpravo** | Manuálne prepnutie na predchádzajúcu / nasledujúcu obrazovku. |
| **Podržanie tlačidla BOOT pri štarte (~3 s)** | Továrenský reset (vymazanie nastavení a WiFi poverení). |

---

## 🌐 API Rozhranie pre domácu automatizáciu

Zariadenie je možné integrovať do systémov ako **Home Assistant**, **Node-RED** alebo ovládať cez `curl`:

- `GET /api/status` – Kompletný JSON so stavom zariadenia, počasím, letmi a pamäťou.
- `GET /api/hardware` – Diagnostika dosky, stav RTC, I2C zariadenia a dôvod reštartu.
- `POST /api/screen` – Zmena obrazovky: `{"screen": 0}` (0: Hodiny, 1: Lietadlá, 2: Meteoradar, 3: Taktický, 4: Predpoveď, 5: Nastavenia).
- `POST /api/toggle-legends` – Simulácia gesta dvojkliku.
- `POST /api/rtc/sync_ntp` – Okamžitá synchronizácia RTC s NTP.

---

## 📜 Licencia & Poďakovanie

Tento projekt je zverejnený pod licenciou **MIT**.
- Pôvodný autor základnej verzie: **Petr / [chiptron.cz](https://chiptron.cz)** ([petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)).
- Rozšírenia, slovenská lokalizácia, taktický radar, nová kolekcia ciferníkov, RTC manažment, IMU gestá a dual-core stabilizácia: **Rado & Antigravity AI**.
