# MeteoPlaneRadar (Slovenská verzia s Taktickým radarom a rozšírenými ciferníkmi)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v1.3.0-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteo-radarová stanica, živý letecký radar, zrážkový meteoradar, taktický kombinovaný radar a dizajnové hodiny na okrúhlom 2.1" IPS dotykovom displeji.**  
Vyvinuté pre hardvérovú dosku **Waveshare ESP32-S3-Touch-LCD-2.1** s moderným dotykovým ovládaním podobným smartfónu, vysúvacím Ovládacím centrom (Control Center), kompletnou konfiguráciou a diaľkovým ovládaním cez responzívny webový prehliadač.

> 🇸🇰 Tento projekt je pokročilým forkom a významným rozšírením pôvodného projektu **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)** od **[chiptron.cz](https://chiptron.cz)**.  
> 🇬🇧 English version: **[README.md](README.md)**

---

## 📸 Ukážka zariadenia v akcii (Live Demo)

<p align="center">
  <img src="docs/media/tactical_radar_live.gif" width="31%" alt="Taktický radar (Lietadlá + Zrážky)" />
  <img src="docs/media/weather_radar_chmu.gif" width="31%" alt="Animovaný meteoradar ČHMÚ" />
  <img src="docs/media/clock_stacked_bold.gif" width="31%" alt="Ciferník Stacked Bold" />
</p>

<p align="center">
  <em>Zľava doprava: <b>Taktický radar</b> (live lietadlá nad zrážkovou mapou), <b>Zrážkový meteoradar</b> (slučka búrkovej oblačnosti), <b>Moderný ciferník Stacked Bold</b>.</em>
</p>

---

## 🌟 Novinky vo verzii v1.3.0

- 📱 **Moderné dotykové gestá ako na mobile:** Horizontálny swipe na prepínanie obrazoviek, vertikálny swipe na plynulý zoom, zmena zoomu dotykom na spodnú lištu a stiahnutie zhora pre Ovládacie centrum.
- 🎛️ **Rýchle ovládacie centrum (Quick Control Center):** Tmavé vysúvacie menu s okamžitým nastavením jasu (`-` / `+` 15%), prepínačom nočného režimu a rýchlymi prepínačmi funkcií pre aktuálnu obrazovku (letiská, okruhy, trasy, prepínanie zdrojov ČHMÚ/RainViewer, auto-rotácia).
- 🎬 **Plynulé animácie prechodov (Slide Transitions):** Žiadne blikanie pri zmene obrazovky vďaka dvojitému framebufferu v PSRAM s jemnou ease-out krivkou (~100 ms).
- 🚁 **Inteligentné sledovanie zaujímavých lietadiel (Watchlist & Alert HUD):** Automatická detekcia a farebné pulzujúce kruhy pre záchranárske vrtuľníky (ATE, Kryštof, HZS, SAR, HEMS), vládne špeciály (SSG, CEF, VIP), ikonické veľké stroje (Airbus A380, Boeing 747, Antonov, Beluga) a vojenské lety so stavovým Alert štítkom na radare.
- ✈️ **Zrozumiteľné názvy modelov a farebná telemetrická karta:** Preklad ICAO kódov do reálnych názvov (napr. `Airbus A320`, `Boeing 737 MAX 8`, `ATR 72-600`) a jednotný detailný panel s farebne odlíšenou výškou, kurzom, stúpaním/klesaním a letiskami.
- 🌤️ **Hodinová minipredpoveď priamo na ciferníku hodín:** 3 štýlové kapsule (`+1h`, `+2h`, `+3h`) s WMO ikonkami a očakávanou teplotou/zrážkami.
- 🔄 **Auto-rotácia podľa gravitácie (QMI8658 IMU):** Automatické prispôsobenie orientácie radaru pri pootočení zariadenia v stojane alebo na stole.

---

## 🌟 Kľúčové funkcie a inovácie v tomto forku

### 🕒 1. Bohatá kolekcia unikátnych ciferníkov hodín
Displej ponúka až **7 úplne odlišných geometrických štýlov ciferníka** s okamžitým prepínaním cez web, Ovládacie centrum alebo poklepaním na telo prístroja:
1. **Digitálny klasický (Classic Digital)** – čistý horizontálny čas s u8g2 fontom, dátumom, vycentrovaným počasím, 3-hodinovou minipredpoveďou, vetrom a fázou mesiaca.
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

### 🛩️ 3. Pokročilé sledovanie letov & Inteligentný Alert HUD
- **Špeciálne kategórie letov:** Záchranári (zelená), vládne lety (zlatá), obrie a ikonické lietadlá (azúrová) a vojenské lety (červená) sú automaticky identifikované a zvýraznené dvojitým pulzujúcim kruhom.
- **Notifikačný Alert štítok:** Okamžité upozornenie na radare pod časom pri výskyte dôležitého letu (napr. `! Zachranny vrtulnik: ATE02 (18 km) !`).
- **Núdzové lety (Squawk 7500, 7600, 7700):** Ak sa v dosahu objaví lietadlo v núdzi, mapa automaticky skryje ostatné lety, zameria a sleduje núdzové lietadlo a zobrazí živú telemetriu (výška, rýchlosť, klesanie/stúpanie).
- **Vektor k najbližšiemu lietadlu:** Dynamická čiara ukazujúca smer, vzdialenosť a prevýšenie k najbližšiemu lietadlu nad vašou hlavou.
- **Letiská & Databáza trás:** Zobrazenie letísk v okolí a offline dekódovanie letových trás z volacích znakov (napr. `Burgas -> Warsaw [BOJ>WAW]`).

### 🛰️ 4. Taktický radar (`ScreenTactical`)
Unikátna obrazovka kombinujúca **zrážkový radar (ČHMÚ / RainViewer) v pozadí** a **letovú prevádzku v reálnom čase v popredí**. Leteckí nadšenci tak okamžite vidia, ako piloti oblietavajú búrkové jadrá.

### 🕒 5. Hardware RTC čip (PCF85063) & Časová nezávislosť
- Automatická synchronizácia palubného RTC čipu s internetovým časom.
- Po odpojení od napájania beží čas ďalej (podpora pripojenia 1.0F / 1.5F 3.3V superkondenzátora na piny `BAT` a `GND`).
- Webový indikátor straty napájania (`OSF - Oscillator Stop Flag`).
- Tlačidlá vo webovom rozhraní pre okamžitú synchronizáciu s NTP serverom alebo priamo z hodín internetového prehliadača.

### ⚡ 6. Bezpečná dvojjadrová FreeRTOS architektúra (Dual-Core)
- **Jadro 1 (Core 1):** Vyhradené výhradne pre plynulé vykresľovanie displeja ST7701 (dvojitý framebuffer bez blikania), čítanie dotyku CST820 a detekciu gest z IMU.
- **Jadro 0 (Core 0):** Beží na ňom asynchrónny worker (`AsyncNetWorker`), ktorý na pozadí spracováva TLS šifrovanie, sťahuje radarové dlaždice, komunikuje s ADS-B API a obsluhuje webový server.
- Zbernica I2C je chránená rekurzívnym mutexom s časovým limitom, čo zaručuje nulové kolízie jadier.

---

## 📱 Prehľad obrazoviek

| Obrazovka | Popis | Zdroj dát |
| :--- | :--- | :--- |
| **1. Hodiny** | 7 voliteľných ciferníkov, 3-hodinová minipredpoveď, 7 štýlov sekúnd, počasie, vietor, fáza mesiaca a 24h solárny oblúk | Open-Meteo & Astro engine |
| **2. Lietadlá** | Živá radarová mapa letov, trasy lietadiel, letiská, vzdialenostné kružnice, sledovanie špeciálnych a núdzových letov | adsb.fi / adsb.lol |
| **3. Meteoradar** | Animovaný zrážkový radar s 6-snímkovou animáciou vývoja oblačnosti | ČHMÚ (CZ/SK) alebo RainViewer (Svet) |
| **4. Taktický radar** | **Kombinovaný pohľad:** Zrážková oblačnosť + lietadlá na jednej mape v reálnom čase | RainViewer / ČHMÚ + adsb.fi |
| **5. Predpoveď** | 6-hodinový detail, 3-dňový výhľad, kvalita ovzdušia (AQI, PM2.5) a peľová situácia | Open-Meteo Weather & Air Quality |
| **6. Nastavenia** | Stav zariadenia, IP adresa, jas, orientácia mapy, voľba jazyka a správa WiFi | Systém |

---

## 🖐️ Dotykové ovládanie & Gestá

| Gesto / Akcia | Výsledná akcia |
| :--- | :--- |
| **Potiahnutie doľava / doprava (Swipe L/R)** | Plynulé prepnutie na ďalšiu / predchádzajúcu obrazovku s horizontálnou animáciou. |
| **Stiahnutie z horného okraja (Pull-down)** | Otvorenie **Rýchleho ovládacieho centra** (jas, nočný režim, rýchle prepínače). |
| **Potiahnutie hore / dole v strede** | **Na radaroch:** Zoom In (priblíženie) / Zoom Out (oddialenie).<br>**V Ovládacom centre:** Zatvorenie centra. |
| **Klepnutie na spodnú lištu rozsahu** | Ľavá polovica oddiali (Zoom Out), pravá polovica priblíži (Zoom In). |
| **Klepnutie na lietadlo** | Zobrazenie farebného detailu letu (reálny model, trasa, výška, rýchlosť, stúpanie). |
| **Dvojité poklepanie (Double-Tap na rámček/stôl)** | **Na hodinách:** Prepne na ďalší ciferník.<br>**Na radaroch:** Prepne Čistý režim (skryje popisy). |
| **Podržanie tlačidla BOOT pri štarte (~3 s)** | Továrenský reset (vymazanie uloženej WiFi a NVS nastavení). |

---

## 🔧 Hardvérové špecifikácie

Projekt je navrhnutý priamo pre modul **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Komponent | Špecifikácia |
| :--- | :--- |
| **MCU** | Espressif ESP32-S3R8 (Xtensa® Dual-Core 32-bit LX7, 240 MHz) |
| **Pamäť** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Displej** | Okrúhly 2.1" IPS, 480×480 px, 65k farieb RGB565, ST7701 RGB interface |
| **Dotyk** | CST820 / CHSC6540 Kapacitný dotykový radič (I2C) |
| **I/O Expander** | TCA9554PWR (riadenie napájania displeja, podsvietenia a resetu) |
| **IMU Senzor** | QMI8658 6-osový akcelerometer & gyroskop (detekcia klepnutia a orientácie) |
| **RTC Čip** | PCF85063 Real-Time Clock s batériovým zálohovaním (I2C `0x51`) |
| **Konektivita** | USB-C (napájanie + natívny CDC sériový port), Wi-Fi 802.11 b/g/n (2.4 GHz) |

---

## 🚀 Inštalácia a prvé spustenie

### 1. Zostavenie a nahratie cez PlatformIO
1. Otvorte priečinok projektu vo **Visual Studio Code** s rozšírením **PlatformIO IDE**.
2. Pripojte dosku cez USB-C kábel.
3. Spustite príkazy:
   ```bash
   # Kompilácia firmvéru
   pio run

   # Nahratie do zariadenia
   pio run -t upload
   ```

### 2. Pripojenie na Wi-Fi (Captive Portal)
1. Pri prvom štarte zariadenie vytvorí otvorenú Wi-Fi sieť s názvom **`MeteoPlaneRadar`** a zobrazí QR kód.
2. Naskenujte QR kód alebo sa pripojte mobilom na sieť `MeteoPlaneRadar`.
3. Otvorte v prehliadači adresu **`http://192.168.4.1/`**.
4. Vyberte vašu domácu Wi-Fi sieť, zadajte heslo a uložte.
5. Zariadenie sa reštartuje, pripojí k vašej sieti a zobrazí pridelenú IP adresu.

### 3. Webové ovládacie rozhranie
Po pripojení otvorte prehliadač na počítači alebo mobile:
- **`http://meteoplaneradar.local/`** (alebo priamo IP adresa zariadenia).

Webové rozhranie umožňuje:
- **Poloha:** Automatická detekcia cez GeoIP alebo vlastné mesto a GPS súradnice.
- **Štýl ciferníka a grafika:** Výber ciferníka, sekundového prstenca a akcentovej farby.
- **Detailné prepínače widgetov:** Samostatné zapnutie dátumu, počasia, vetra, mesiaca, leteckých stôp, letísk a okruhov.
- **Nočný režim hodín (`nightClockOnly`):** V noci zastaví rotáciu obrazoviek a ponechá stlmené hodiny.
- **Hardvérová diagnostika:** Živá kontrola I2C zbernice, stavu RTC čipu a tlačidlá okamžitej synchronizácie.
- **Diaľkové ovládanie:** Prepínanie obrazoviek a zmena priblíženia radaru priamo z gauča.
- **Bezdrôtová aktualizácia (OTA):** Nahrávanie nového `.bin` firmvéru bez nutnosti pripájania kábla.

---

## 🌐 REST API pre inteligentnú domácnosť

Jednoduchá integrácia do systémov **Home Assistant**, **Node-RED** alebo terminálových skriptov:

- `GET /api/status` – Kompletný JSON stav (počasie, počty lietadiel, pamäť, uptime).
- `GET /api/hardware` – Zoznam periférií, RTC čas, I2C bus scan a dôvod posledného reštartu.
- `POST /api/screen` – Zmena obrazovky: `{"screen": 0}` (0: Hodiny, 1: Lietadlá, 2: Meteoradar, 3: Taktický, 4: Predpoveď, 5: Nastavenia).
- `POST /api/toggle-legends` – Vyvolanie prepnutia Čistého režimu.
- `POST /api/rtc/sync_ntp` – Okamžitá synchronizácia RTC s NTP serverom.

---

## 📜 Licencia a poďakovanie

Šírené pod licenciou **MIT License**.
- Pôvodný základ projektu: **Petr / [chiptron.cz](https://chiptron.cz)** ([petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)).
- Vylepšenia, slovenská lokalizácia, taktický radar, kolekcia ciferníkov, RTC ovládač, IMU gestá, dotyková navigácia, rýchle Ovládacie centrum, sledovanie zaujímavých letiek a viacjadrová stabilizácia: **Rado & Antigravity AI**.
