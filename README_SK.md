# MeteoPlaneRadar (Taktický a meteorologický radar pre ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v2.0.9-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteorologická stanica, živý ADS-B letecký radar, animovaný radar zrážok (SHMÚ, ČHMÚ, RainViewer), kombinovaný taktický radar, sledovanie dráhy ISS, analytika YouTube, finančné trhy a animovaný virtuálny spoločník na okrúhlom 2.1" IPS dotykovom displeji.**  
Vyvinuté pre **Waveshare ESP32-S3-Touch-LCD-2.1** s dotykovými gestami v štýle smartfónu, sťahovacím Ovládacím centrom, reálnymi fotografiami lietadiel, bilineárnym vyhladzovaním radaru a webovým ovládacím panelom.

> 🇬🇧 English documentation: **[README.md](README.md)**  
> 📌 Forknuté a výrazne rozšírené z **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.

*Ak sa vám projekt páči, zvážte podporu vývoja!* ☕  
<a href="https://buymeacoffee.com/hackra" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

---

## 📸 Ukážky zo zariadenia

<p align="center">
  <img src="docs/media/tactical_radar_live.gif" width="16%" alt="Taktický radar" />
  <img src="docs/media/screen_iss_live.png" width="16%" alt="Sledovanie ISS" />
  <img src="docs/media/finance_screen.png" width="16%" alt="Trhy a krypto" />
  <img src="docs/media/plane_detail_photo.png" width="16%" alt="Detail lietadla" />
  <img src="docs/media/weather_radar_chmu.gif" width="16%" alt="Radar zrážok" />
  <img src="docs/media/clock_stacked_bold.png" width="16%" alt="Ciferník" />
</p>

### 🐱 DigiCat — Animovaný virtuálny spoločník

<p align="center">
  <img src="docs/media/digicat_showcase.png" width="80%" alt="DigiCat hi-res animácie ginger mačky" />
</p>

<p align="center">
  <em>DigiCat v2.0.9 — Profesionálne 64×64 sprite animácie ryšavej mačky (247 snímok, 38 animačných sekvencií, RGB565 PROGMEM, škálované na 3× / 192×192 px).</em>
</p>

---

## 🌟 Funkcie

### 🕒 Ciferníky a čas
6 voliteľných ciferníkov (Stacked Bold, Aviator, Orbital, Régulateur, Nordic Minimal, Classic Digital) s minipredpoveďou, fázou mesiaca a 24-hodinovým solárnym oblúkom. **7 štýlov sekundového prstenca**: Vypnutý, Bodka, Plynulý oblúk, Pulz, Radarový lúč, Hodinárske indexy, Satelit na orbite.

### 🛩️ Sledovanie lietadiel & Alert HUD
- **360° radar** (14–200 km) cez adsb.fi / adsb.lol s kružnicami dosahu a letiskami.
- **Špeciálne kategórie**: Záchranári (zelená), vládne lety (zlatá), obrie lietadlá (azúrová), vojenské (červená) — pulzujúce kruhy a alert štítok navrchu.
- **Auto-fokus na núdzové lety** (7500/7600/7700): uzamkne lietadlo, stmaví ostatné, zobrazí živú telemetriu.
- **Vektor k najbližšiemu lietadlu** so vzdialenosťou, smerom a prevýšením.
- **Offline databáza trás**: volací znak → odlet/prístup (napr. `BOJ→WAW`).
- **Ťuknutie na lietadlo** otvorí telemetrickú kartu s reálnou fotografiou z Planespotters.net.

### 🌧️ Radar zrážok & Nowcasting
- Animované radarové slučky (SHMÚ / ČHMÚ / RainViewer) s plynulým prechodom a bilineárnym vyhladzovaním.
- **TREC 2D nowcasting**: upozorňuje iba keď zrážky skutočne smerujú k vám (ETA ≤ 35 min, odchýlka ≤ 6 km).
- **Automatická klasifikácia**: Dážď / Dážď so snehom (1–3°C) / Sneh (≤1°C) / Krupobitie (>50 dBZ).

### 🛰️ Taktický radar, ISS, YouTube & Finance
- **Kombinovaný taktický radar**: radar zrážok + živá ADS-B premávka na jednej obrazovke.
- **Sledovanie ISS**: mapa sveta so slnečným terminátorom, minulá a budúca orbita, kruh viditeľnosti, odpočet preletu a sonarové pípnutie.
- **YouTube analytika**: živý počet odberateľov, celkové zhliadnutia a štatistiky nového videa (YouTube Data API v3).
- **Finančné trhy**: 4 vlastné tickery (ETF, akcie, krypto, forex) s čiarovým a sviečkovým grafom (Yahoo Finance v8).

### 🌤️ Počasie, Kvalita ovzdušia & Nočný režim
- Hodinové krivky teploty, vetra a zrážok, 3-dňová predpoveď, AQI, PM2.5 a peľ (Open-Meteo).
- **Ultra nočný režim** (0,5% jas) a `nightClockOnly` (zamkne displej na ciferníku v nočných hodinách).

### 🐾 DigiCat — Virtuálny spoločník (v2.0.9)
Celoobrazovková interaktívna zásuvka (Swipe Up z akejkoľvek obrazovky alebo ikona labky v Ovládacom centre).

- **247 snímok · 38 animačných sekvencií** — profesionálne 64×64 RGBA sprite animácie ryšavej mačky, RGB565 PROGMEM, škálované na **3× (192×192 px)**.
- **Bohatá pokojová osobnosť (9 sub-animácií)**: vrtenie chvosta v sede, lízanie labky v sede/v ľahu, mňaukanie v sede/v ľahu/v stoji, škrabanie ľavého/pravého ucha, zívanie — náhodne každých 4–10 s.
- **Šťastná reakcia (3 varianty)**: vrtenie chvosta v sede, v stoji spredu, v stoji vpravo — cykli každých 1,5–4 s.
- **Lovenie lietadla (3 varianty)**: státie + mávnutie, sedenie + pravá labka, sedenie + ľavá labka — vylosuje sa pri každom stretnutí.
- **5 náhodných spánkových póz** (Rows 45–56): variant + smer ležania zamknutý na celú noc.
- **Smerovo-vedomá lokomócia**: autentická `cat_run_left` pri vstupe sprava; `cat_slide_left` pri náklone doľava.
- **Doplnky podľa počasia**: dáždnik (dážď/búrka), pletený šál (sneh/zima ≤2°C), letecké okuliare (jasno).
- **Dráha reagujúca na počasie**: suchý asfalt / mokrý lesklý povrch / snehový poprašok.
- **Autonómny mačací mozog**: náhodný príchod, prechádzky, výskoky, umývanie, strečing, výlety z obrazovky každých 6–12 s.
- **Interakcia s lietadlami**: lietadlo letí s kondenzačnými stopami → DigiCat beží a chňapá; lietadlo uhýba; DigiCat skáče v naháňačke.
- **Nočné správanie**: 2-minútový čas bdenia pri interakcii; zívanie → strečing → spánok; ťuknutie = prebudenie.
- **Štatistiky zvieratka**: Hlad & Šťastie (0–100%); letecké XP hodnosti (*Mačací kadet → Radarový navigátor → Letecké eso*).
- **Google Gemini Live AI**: voliteľné prepojenie s Gemini Flash pre živé komentovanie premávky (100% offline fallback, SK/CZ/EN).

### ⚡ Systém & Konektivita
- **Dvojjadrový FreeRTOS**: Jadro 1 pre ST7701 RGB (dvojitý framebuffer, nulové blikanie) + dotyk; Jadro 0 pre sieť, radar a ADS-B.
- **Stabilný ST7701 ovládač**: kalibrovaný 8 MHz RGB clock, bez zápisov do Flash pri striedaní obrazoviek, auto-resync VSYNC.
- **Hardvérové RTC** (PCF85063) s voliteľným superkondenzátorom (3,3 V, 1,0–1,5 F).
- **Webový panel**: konfigurácia, OTA, živý sériový monitor (64 KB PSRAM), screenshot, diaľkové ovládanie.
- **Smart Home REST API**: `/api/status`, `/api/screen`, `/api/display/resync`, `/api/toggle-legends`, `/api/rtc/sync_ntp`.
- **Aktívny bzučiak**: núdzové squawk alarmy, búrkové výstrahy, sonarové pípnutie ISS, hodinové odbíjanie, nočné stíšenie.

---

## 📱 Prehľad obrazoviek

| Obrazovka | Náhľad | Popis | Zdroj |
| :--- | :---: | :--- | :--- |
| **0. Hodiny** | <img src="docs/media/clock_stacked_bold.png" width="70" /> | 6 ciferníkov, predpoveď, mesiac, solárny oblúk | Open-Meteo |
| **1. Lietadlá** | <img src="docs/media/plane_radar_live.png" width="70" /> | 360° ADS-B radar, núdzové lety, trasy, letiská | adsb.fi / adsb.lol |
| **1b. Detail lietadla** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Kompletná telemetria + reálna fotografia na ťuknutie | Planespotters.net |
| **2. Radar zrážok** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animovaná slučka radaru, plynulé prelínanie | SHMÚ / ČHMÚ / RainViewer |
| **3. Taktický radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | Radar zrážok + živá ADS-B premávka na jednej obrazovke | SHMÚ / ČHMÚ + adsb.fi |
| **4. Predpoveď** | <img src="docs/media/forecast_screen.png" width="70" /> | Hodinové krivky, 3-dňová predpoveď, AQI, PM2.5, peľ | Open-Meteo |
| **5. Trhy & Krypto** | <img src="docs/media/finance_screen.png" width="70" /> | 4 vlastné tickery, čiarový a sviečkový graf | Yahoo Finance v8 |
| **6. Dráha ISS** | <img src="docs/media/screen_iss_live.png" width="70" /> | Mapa sveta, terminátor, orbity, kruh viditeľnosti | WhereTheISS API |
| **7. YouTube** | <img src="docs/media/youtube_screen.png" width="70" /> | Odberatelia, celkové zhliadnutia, posledné video | YouTube Data API v3 |
| **8. Štatistiky letov** | <img src="docs/media/flight_stats_screen.png" width="70" /> | 24h aktivita vzdušného priestoru v 6 rozsahoch | PSRAM Tracker |
| **9. Nastavenia** | <img src="docs/media/settings_screen.png" width="70" /> | Telemetria, IP, jas, jazyk, konfigurácia radaru | Systém |

---

## 🖐️ Dotykové ovládanie a gestá

| Gesto | Výsledok |
| :--- | :--- |
| **Potiahnutie doľava / doprava** | Predchádzajúca / nasledujúca obrazovka |
| **Stiahnutie z horného okraja** | Rýchle ovládacie centrum (jas, nočný režim, prepínače) |
| **Potiahnutie zospodu nahor** | Zásuvka DigiCat |
| **Ťuknutie na zvieratko** | Pohladkanie (pradenie, červenanie, dialóg) |
| **Dvojité ťuknutie na zvieratko** | Kŕmenie maškrtou (obnoví hlad & šťastie) |
| **Potiahnutie hore/dole (stred)** | Hodiny: cykluje ciferník · Radary: zoom in/out |
| **Ťuknutie na spodnú lištu** | Ľavá = oddiali · Pravá = priblíži |
| **Ťuknutie na lietadlo** | Detailná karta s fotografiou |
| **Ťuknutie na fotografiu** | Celá obrazovka (ťuknutie = späť) |
| **Dvojité poklepanie (na rám)** | Radary: čistý režim · Finance/ISS: okamžitá obnova |
| **Podržanie BOOT ~3 s** | Továrenský reset |

---

## 🔧 Hardvérové špecifikácie

Pre **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Komponent | Špecifikácia |
| :--- | :--- |
| **Procesor** | ESP32-S3R8 · Dual-Core LX7 @ 240 MHz |
| **Pamäť** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Displej** | Okrúhly 2.1" IPS · 480×480 px · ST7701 RGB |
| **Dotyk** | CST820 / CHSC6540 kapacitný (I2C) |
| **IMU** | QMI8658 6-osový akcelerometer + gyroskop |
| **RTC** | PCF85063 (I2C `0x51`) |
| **Konektivita** | USB-C · Wi-Fi 802.11 b/g/n 2.4 GHz |

---

## 🚀 Inštalácia

### Možnosť A — Predkompilovaná binárka (Najjednoduchšie)
Stiahnite z **[Vydania (Releases)](https://github.com/hackra76/ESP-MeteoPlaneRadar/releases)**:
- `MeteoPlaneRadar-v2.0.9-factory.bin` — kompletný obraz (bootloader + partície + firmvér).
- Flash cez [ESP Web Flasher](https://espressif.github.io/esptool-js/) pri 921600 baud od adresy `0x00000000`.
- Alebo: `esptool.py -p COM_PORT -b 921600 write_flash 0x0 MeteoPlaneRadar-v2.0.9-factory.bin`

### Možnosť B — PlatformIO
```bash
pio run            # kompilácia
pio run -t upload  # nahratie
```

### Prvé zapnutie & Wi-Fi
1. Zariadenie vytvorí sieť **`MeteoPlaneRadar`** a zobrazí QR kód.
2. Pripojte sa → otvorte `http://192.168.4.1/` → zadajte Wi-Fi heslo → Uložiť.
3. Ovládací panel na **`http://meteoplaneradar.local/`** (alebo IP zariadenia).

---

## 🔑 Nastavenie YouTube Data API kľúča

1. Vytvorte projekt na [Google Cloud Console](https://console.cloud.google.com/).
2. Povoľte **YouTube Data API v3** a vytvorte **API Key** (APIs & Services → Credentials).
3. V webovom paneli → záložka **YouTube** → vložte kľúč + handle kanála (`@meno`) alebo Channel ID (`UC...`).

---

## 🌐 REST API

| Endpoint | Metóda | Popis |
| :--- | :---: | :--- |
| `/api/status` | GET | Kompletný JSON stav (počasie, lietadlá, pamäť, ISS) |
| `/api/hardware` | GET | Periférie, RTC, I2C scan, dôvod resetu |
| `/api/screen` | POST | Prepnutie obrazovky `{"index": 0}` (0–9) |
| `/api/display/resync` | POST | Hardvérová resynchronizácia displeja |
| `/api/toggle-legends` | POST | Prepnutie čistého režimu mapy |
| `/api/rtc/sync_ntp` | POST | Vynútenie NTP → RTC synchronizácie |

---

## 📜 Licencia & Poďakovanie

MIT License.
- Pôvodný projekt: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**
- DigiCat herné mechaniky: **[aquascape123/digicat](https://github.com/aquascape123/digicat)** (MIT)
- Vylepšenia, SHMÚ radar, ISS tracker, ciferníky, dotykové ovládanie, DigiCat hi-res sprite, AI mačací mozog, webový panel, slovenská lokalizácia: **Rado & Antigravity AI**
