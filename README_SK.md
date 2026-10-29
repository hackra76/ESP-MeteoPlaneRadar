# MeteoPlaneRadar (Taktický a meteorologický radar pre ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v3.9.1-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteorologická stanica, živý ADS-B letecký radar, animovaný radar zrážok (SHMÚ, ČHMÚ, RainViewer), kombinovaný taktický radar, sledovanie dráhy ISS, analytika YouTube, finančné trhy, telemetrický HUD pre Klipper 3D tlačiarne, systém varovania pred katastrofami s hardvérovým seizmometrom a animovaný virtuálny spoločník na okrúhlom 2.1" IPS dotykovom displeji.**  
Vyvinuté pre **Waveshare ESP32-S3-Touch-LCD-2.1** s dotykovými gestami v štýle smartfónu, sťahovacím Ovládacím centrom, reálnymi fotografiami lietadiel, bilineárnym vyhladzovaním radaru a priemyselným webovým ovládacím panelom.

> 🇬🇧 English documentation: **[README.md](README.md)** · 📋 História vydaní: **[CHANGELOG.md](CHANGELOG.md)**  
> 📌 Forknuté a výrazne rozšírené z **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.

*Ak sa vám projekt páči, zvážte podporu vývoja!* ☕  
<a href="https://buymeacoffee.com/hackra" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>

---

## 📸 Ukážky zo zariadenia

<p align="center">
  <img src="docs/media/clock_luxury.png" width="13%" alt="Luxusný astronomický chronograf" />
  <img src="docs/media/tactical_radar_live.gif" width="13%" alt="Taktický radar" />
  <img src="docs/media/screen_sonar.png" width="13%" alt="Fosforový sonarový radar" />
  <img src="docs/media/screen_klipper.png" width="13%" alt="Klipper 3D tlačiareň telemetria" />
  <img src="docs/media/screen_iss_live.png" width="13%" alt="3D sledovanie ISS" />
  <img src="docs/media/finance_screen.png" width="13%" alt="Trhy a krypto" />
  <img src="docs/media/plane_detail_photo.png" width="13%" alt="Detail lietadla" />
</p>

### 🐱 DigiCat — Animovaný virtuálny spoločník

<p align="center">
  <img src="docs/media/digicat_showcase_v2.png" width="80%" alt="DigiCat hi-res animácie ginger mačky" />
</p>

<p align="center">
  <em>Profesionálne 64×64 sprite animácie ryšavej mačky (247 snímok, 38 animačných sekvencií, RGB565 PROGMEM, škálované na 3× / 192×192 px).</em>
</p>

---

## 🌟 Funkcie

### 🕒 Luxusný astronomický chronograf a komplikácie
- **24-hodinový solárny oblúk súmraku**: Vonkajší prstenec zobrazujúci pohyb Slnka (obed na 12. hodine, polnoc na 6. hodine), astronomický východ/západ slnka, zlatú hodinku a dynamický symbol Slnka podľa miestneho solárneho času.
- **Konfigurovateľné duálne komplikácie ciferníka**: Nezávislá horná a dolná kruhová apertúra so 7 vymeniteľnými modulmi:
  - **3D fotorealistický Mesiac**: Fotografická NASA LROC albedo textúra (92×92 px) s 3D Lambertovým difúznym osvetlením (presné osvetlenie pravej/ľavej pologule pre severnú pologuľu) a percentom osvetlenia.
  - **Trend teploty so Sparkline (24h)**: Vonkajšia teplota s 24-hodinovou krivkou vývoja.
  - **Aktuálne počasie a vietor**: Ikona počasia, teplota a rýchlosť vetra.
  - **Živé telemetrické moduly**: Počet lietadiel v okruhu 10 km, počet odberateľov YouTube, primárny trhový ticker alebo priebeh tlače 3D tlačiarne Klipper (živá aktualizácia priamo na ciferníku hodín na pozadí).
- **Interaktívna navigácia**: Ťuknutie na ľubovoľný subciferník alebo výstražný štítok (prelet lietadla / blížiaci sa dážď) okamžite prepne na príslušnú obrazovku.
- **Dynamická sekundová dráha**: Plynulá obiehajúca svetelná perla (~30 FPS) s Gaussian bloom stopou a vyhladzovanou 76 px švajčiarskou typografiou.
- **DigiCat Cameo**: Nenápadné zobrazenie oddychujúcej mačky na spodnom okraji ciferníka so synchronizovaným dýchaním.

### 🛩️ ADS-B letecký radar a Aero Glass HUD
- **360° letecký radar (14–200 km)**: Živé sledovanie lietadiel cez `adsb.fi` / `adsb.lol` s kružnicami dosahu, letiskami, online mapovými podkladmi a 360° kompasovou ružicou.
- **Symbolika Glass Cockpit**: Voliteľné aerodynamické vektory chevron (s dvojitým chevronom pre stíhačky) alebo siluety lietadiel, 1-minútové vektory kurzu a 3-riadkové avionické štítky bez prekrývania.
- **Špeciálne lety a núdzové squawky**: Farebné zvýraznenie pre záchranárov (zelená), vládne/VIP lety (zlatá), obrie lietadlá (azúrová) a vojenské lety (červená), plus automatické uzamknutie a akustický alarm pri núdzových kódoch (`7500` / `7600` / `7700`).
- **Offline databáza trás**: Okamžité dekódovanie trasy z volacieho znaku (`ODKIAĽ→KAM`) a vektor k najbližšiemu lietadlu.
- **Reálne fotografie lietadiel a Flight Deck HUD**: Ťuknutie na lietadlo zobrazí detailnú kartu so živou fotografiou z Planespotters.net (vrátane podpory progresívnych JPEG); ťuknutie na fotografiu otvorí celoobrazovkový Aero Glass HUD.

### 🌧️ Radar zrážok, Taktický radar a Akustický sonar
- **Animovaný meteorologický radar**: Plynulé slučky zrážok zo zdrojov **SHMÚ**, **ČHMÚ** alebo **RainViewer** s časovým prelínaním (cross-dissolve) a kruhovou maskou.
- **TREC 2D nowcasting zrážok**: Vektorové sledovanie búrok, ktoré upozorní iba vtedy, keď zrážky skutočne smerujú k vašej polohe (ETA ≤ 35 min, odchýlka ≤ 6 km), s automatickou klasifikáciou (Dážď / Dážď so snehom / Sneh / Krupobitie).
- **Kombinovaný taktický radar**: Súčasné zobrazenie radaru zrážok a živej ADS-B leteckej premávky na jednej obrazovke.
- **Taktický akustický sonar**: 25 FPS CRT fosforový lúč, vodopádový spektrogram (BTR), dopplerovské kódovanie rýchlosti a voliteľné akustické pípnutie.

### 🌤️ Predpoveď počasia, Kvalita ovzdušia a Nočný režim
- **Hodinová a 3-dňová predpoveď**: Hodinové krivky teploty, vetra a zrážok spolu s 3-dňovým výhľadom cez Open-Meteo.
- **Kvalita ovzdušia a peľ**: Európsky index AQI, koncentrácia prachových častíc PM2.5 a peľové alergény (jelša, breza, trávy, palina, ambrózia).
- **Hlboký červený nočný režim**: Automatický solárny alebo časovaný nočný režim (0,5 % jas v červenom spektre) s voliteľným uzamknutím na obrazovku hodín (`nightClockOnly`).

### 🛰️ 3D dráha ISS, Finančné trhy a YouTube analytika
- **3D sledovanie ISS**: Ortografická 3D guľa Zeme s atmosférickým Rayleighovým halom, slnečným terminátorom deň/noc, predikciou dráhy a 3 režimami pohľadu (`3D ISS`, `3D Domov`, `2D mapa`).
- **Finančné trhy a krypto**: Až 8 vlastných tickerov z Yahoo Finance (akcie, ETF, kryptomeny, komodity, forex) s čiarovým a sviečkovým grafom.
- **YouTube analytika**: Živý počet odberateľov, celkové zhliadnutia a štatistiky najnovšieho/najsledovanejšieho videa cez neblokujúce streamovanie.

### 🖨️ Telemetrický HUD pre 3D tlačiarne Klipper / Moonraker
- **Kruhový HUD pre 3D tlačiareň**: Živá LAN telemetria z rozhrania Moonraker s hardvérovo vykresľovanými vektorovými ikonami (izometrická 3D kocka, tryska, vyhrievaná podložka, zameriavací kríž homingu, bed mesh).
- **16 prevádzkových stavov**: Farebne podsvietená stavová LED (`STANDBY`, `HOMING`, `PROBING / MESH`, `HEATING`, `PRINTING`, `PAUSED`, `FILAMENT RUNOUT`, `COMPLETED`, `ERROR` atď.) s inteligentným filtrovaním prechodných makier.
- **Teploty, vrstvy a ETA**: Karty teplôt trysky a podložky s cieľovými hodnotami, stav homingu X/Y/Z, výška Z, počítadlo vrstiev (`L X/Y`) a presný výpočet zostávajúceho času tlače.
- **Striktná sieťová izolácia**: Rýchle LAN timeouty (1,5 s connect) a podpora mDNS `.local` prekladu s neblokujúcim plánovaním.

### 🐾 DigiCat — Interaktívny virtuálny spoločník
- **Hi-Res Sprite engine**: 247 snímok v 38 animačných sekvenciách vykresľovaných v mierke 3× (`192×192 px`) vo výsuvnej zásuvke.
- **Laserové ukazovadlo**: Interaktívne navádzanie laserovej bodky dotykom po obrazovke s realistickou mačacou inverznou kinematikou plazenia a skokov.
- **Hádzanie rybičiek**: Hra s realistickou balistickou parabolickou fyzikou odrazov pochúťok a sekvenciami maškrtenia.
- **Viaczónové hladkanie**: Kontextové zóny dotyku (škrabkanie pod bradou, ušká, bruško, chvostík) s dynamickými náladami a zvukovým pradením.
- **Živý okolitý svet**: Procedurálne prvky prostredia v pozadí vrátane motýľov, vtákov, padajúceho lístia a prachových zrniek.
- **Typografický písací stroj**: Dialógové bubliny v retro štýle písacieho stroja s animovaným písaním a AI myšlienkami cez Gemini.
- **Režim Aero-Cat kopilota**: Ťuknutie na čelo prepne do režimu Top Gun so zlatými okuliarmi, mini radarom a živou telemetriou 6-osého IMU senzora (`PITCH`, `ROLL`, `KURZ`).
- **Reakcie na počasie a lietadlá**: Dáždnik v daždi, šál v mraze, letecké okuliare za jasného počasia; naháňanie a chňapanie po skutočných lietadlách prelietajúcich nad hlavou.
- **Autonómny AI mozog a Gemini komentáre**: Bohaté pokojové animácie, spánkový režim, štatistiky Hladu/Šťastia, letecké XP hodnosti a voliteľné živé komentovanie premávky cez Google Gemini Flash (so 100 % offline zálohou v SK/CZ/EN).

### 🎛️ Priemyselný webový portál, Vlastné poradie obrazoviek a Architektúra
- **Webový panel "Tactile Field Instrument"**: Responzívny Gzip-komprimovaný portál so živým 480×480 zrkadlením displeja a dotykovou simuláciou, CRT radarovým simulátorom, 64 KB PSRAM sériovým monitorom a OTA aktualizáciami.
- **Vlastné poradie obrazoviek**: Drag-and-drop a tlačidlový správca poradia obrazoviek uložený v NVS — riadi dotykové listovanie, automatickú rotáciu aj diaľkové ovládanie.
- **Dvojjadrový FreeRTOS a samoobnovujúci Anti-Tearing displej**: Jadro 1 vyhradené pre 60 FPS LVGL v9 vykresľovanie (s 20-riadkovým ST7701 RGB SRAM bounce bufferom a IRAM-safe VSYNC strážcom fázy/parity DMA proti trhaniu obrazu a vertikálnemu posunu) a dotykové gestá; Jadro 0 pre asynchrónnu sieťovú komunikáciu a dekódovanie obrázkov.
- **Hardvérový seizmometer a výstrahy**: Kontinuálne monitorovanie otrasov cez IMU QMI8658 spojené s globálnymi výstrahami pred zemetraseniami a cunami (USGS).

---

## 📱 Prehľad obrazoviek

| Obrazovka | Náhľad | Popis | Zdroj |
| :--- | :---: | :--- | :--- |
| **0. Hodiny** | <img src="docs/media/clock_luxury.png" width="70" /> | Luxusný astronomický chronograf (24h solárny oblúk, duálne komplikácie) | Open-Meteo / RTC |
| **1. Lietadlá** | <img src="docs/media/plane_radar_live.png" width="70" /> | Glass Cockpit ADS-B radar, 360° kompas, vektory rýchlosti, mapové podklady | adsb.fi / adsb.lol |
| **1b. Detail lietadla** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Aero Glass Flight Deck HUD s kompletnou telemetriou a reálnou fotografiou | Planespotters.net |
| **2. Radar zrážok** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animovaná slučka radaru, plynulé prelínanie, TREC nowcasting zrážok | SHMÚ / ČHMÚ / RainViewer |
| **3. Taktický radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | Radar zrážok + živá ADS-B premávka v štýle Glass Cockpit | SHMÚ / ČHMÚ + adsb.fi |
| **4. Sonar** | <img src="docs/media/screen_sonar.png" width="70" /> | CRT fosforový radar, BTR spektrogram, dopplerovské rýchlosti | adsb.fi |
| **5. Predpoveď** | <img src="docs/media/forecast_screen.png" width="70" /> | Hodinové krivky teploty, 3-dňová predpoveď, AQI, PM2.5, peľ | Open-Meteo |
| **6. Trhy & Krypto** | <img src="docs/media/finance_screen.png" width="70" /> | Až 8 vlastných tickerov, čiarový a sviečkový graf | Yahoo Finance v8 |
| **7. Dráha ISS** | <img src="docs/media/screen_iss_live.png" width="70" /> | 3D ortografická guľa Zeme, slnečný terminátor, Rayleighovo halo, orbity | WhereTheISS API |
| **8. YouTube** | <img src="docs/media/youtube_screen.png" width="70" /> | Živý počet odberateľov, celkové zhliadnutia, štatistiky nového / top videa | YouTube Data API v3 |
| **9. 3D tlačiareň (Klipper)** | <img src="docs/media/screen_klipper.png" width="70" /> | Živý telemetrický HUD pre Klipper, teploty trysky/podložky, vrstvy, ETA | Moonraker API |
| **10. Štatistiky letov & Info** | <img src="docs/media/flight_stats_screen.png" width="70" /> | 24h aktivita vzdušného priestoru v 6 rozsahoch, telemetria SRAM a PSRAM | PSRAM Tracker |
| **11. Nastavenia** | <img src="docs/media/settings_screen.png" width="70" /> | Telemetria zariadenia, IP, jas, jazyk, poskytovateľ máp, zvuk, OTA | Systém |

---

## 🖐️ Dotykové ovládanie a gestá

| Gesto | Výsledok |
| :--- | :--- |
| **Potiahnutie doľava / doprava** | Predchádzajúca / nasledujúca obrazovka (podľa vlastného poradia) |
| **Stiahnutie z horného okraja** | Rýchle ovládacie centrum (jas, komplikácie, nočný režim, stopy letu, mapy) |
| **Potiahnutie zospodu nahor** | Otvorenie zásuvky DigiCat |
| **Ťuknutie na pravý okraj (Radary)** | Zobrazenie tlačidiel zoomu `+` / `-` (automaticky zmiznú po 3,5 s) |
| **Ťuknutie na horný / dolný subciferník (Hodiny)** | Okamžitý prechod na obrazovku danej komplikácie |
| **Ťuknutie na výstražný štítok (Hodiny)** | Prechod na Lietadlá (prelet) alebo Radar zrážok (dážď) |
| **Ťuknutie na lietadlo** | Otvorenie detailnej karty s reálnou fotografiou |
| **Ťuknutie na fotografiu** | Otvorenie celoobrazovkového Aero Glass Flight Deck HUD (ťuknutie = späť) |
| **Dvojité ťuknutie na obrazovku (Radary)** | Prepnutie čistého režimu mapy (skrytie / zobrazenie legiend a textov) |
| **Ťuknutie na štítok / dosah (Sonar)** | Prepnutie režimu (`Klasický PPI` → `Split` → `Vodopád`) / rozsahu (`25–150 km`) |
| **Ťuknutie na guľu Zeme (ISS)** | Prepnutie režimu zobrazenia (`3D ISS` → `3D Domov` → `2D mapa`) |
| **Ťuknutie na obrazovku (Klipper)** | Okamžité vyžiadanie obnovy telemetrie z Moonraker |
| **Ťuknutie / Dvojité ťuknutie na mačku (DigiCat)** | Pohladkanie (ťuknutie na čelo prepína Top Gun režim) / nakŕmenie maškrtou |
| **Podržanie tlačidla BOOT ~3 s** | Továrenský reset (vymazanie Wi-Fi a NVS nastavení) |

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
| **RTC** | PCF85063 (I2C `0x51`) s voliteľným superkondenzátorom |
| **Konektivita** | USB-C · Wi-Fi 802.11 b/g/n 2.4 GHz |

---

## 🚀 Inštalácia

### Možnosť A — Predkompilovaná binárka (Najjednoduchšie)
Stiahnite z **[Vydania (Releases)](https://github.com/hackra76/ESP-MeteoPlaneRadar/releases)**:
- `MeteoPlaneRadar-v3.9.1-factory.bin` — kompletný obraz (bootloader + partície + firmvér).
- Flash cez [ESP Web Flasher](https://espressif.github.io/esptool-js/) pri `921600` baud od adresy `0x00000000`.
- Alebo cez príkazový riadok:
  ```bash
  esptool.py -p COM_PORT -b 921600 write_flash 0x0 MeteoPlaneRadar-v3.9.1-factory.bin
  ```

### Možnosť B — PlatformIO
```bash
pio run            # kompilácia
pio run -t upload  # nahratie
```

### Prvé zapnutie & Wi-Fi
1. Zariadenie vytvorí sieť **`MeteoPlaneRadar`** a zobrazí QR kód na displeji.
2. Pripojte sa k sieti → otvorte `http://192.168.4.1/` → zadajte prihlasovacie údaje k domácej Wi-Fi → **Uložiť**.
3. Ovládací panel je dostupný na **`http://meteoplaneradar.local/`** (alebo IP adrese zariadenia).

---

## 🔑 Nastavenie YouTube Data API kľúča

1. Vytvorte projekt v [Google Cloud Console](https://console.cloud.google.com/).
2. Povoľte **YouTube Data API v3** a vytvorte **API Key** (*APIs & Services → Credentials*).
3. Vo webovom paneli zariadenia → záložka **YouTube** → vložte API kľúč a handle kanála (`@meno`) alebo Channel ID (`UC...`).

---

## 🌐 REST API

| Endpoint | Metóda | Popis |
| :--- | :---: | :--- |
| `/api/status` | `GET` | Kompletná JSON telemetria (počasie, lietadlá, pamäť, ISS) |
| `/api/hardware` | `GET` | Periférie, RTC, I2C scan, dôvod resetu |
| `/api/screen` | `POST` | Prepnutie obrazovky (`{"index": 0}` alebo `{"step": 1}`) |
| `/api/screenshot.bmp` | `GET` | Živý 480×480 24-bitový BMP snímok obrazovky |
| `/api/display/resync` | `POST` | Hardvérová resynchronizácia ST7701 RGB displeja |
| `/api/toggle-legends` | `POST` | Prepnutie čistého režimu mapy na radaroch |
| `/api/rtc/sync_ntp` | `POST` | Vynútenie NTP → RTC synchronizácie |

---

## 📜 Licencia & Poďakovanie

MIT License.
- Pôvodný projekt: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**
- DigiCat herné mechaniky: **[aquascape123/digicat](https://github.com/aquascape123/digicat)** (MIT)
- Vylepšenia, SHMÚ radar, ISS tracker, luxusný chronograf, dotykové ovládanie, DigiCat hi-res sprite, AI mačací mozog, priemyselný webový panel, slovenská lokalizácia: **Rado & Antigravity AI**
