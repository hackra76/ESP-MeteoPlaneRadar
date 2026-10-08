# MeteoPlaneRadar (Taktický a meteorologický radar pre ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v3.0.0-brightgreen.svg)
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
  <img src="docs/media/clock_luxury.png" width="16%" alt="Luxusný astronomický chronograf" />
  <img src="docs/media/tactical_radar_live.gif" width="16%" alt="Taktický radar" />
  <img src="docs/media/screen_sonar.png" width="16%" alt="Fosforový sonarový radar" />
  <img src="docs/media/screen_iss_live.png" width="16%" alt="3D sledovanie ISS" />
  <img src="docs/media/finance_screen.png" width="16%" alt="Trhy a krypto" />
  <img src="docs/media/plane_detail_photo.png" width="16%" alt="Detail lietadla" />
</p>

### 🐱 DigiCat — Animovaný virtuálny spoločník

<p align="center">
  <img src="docs/media/digicat_showcase_v2.png" width="80%" alt="DigiCat hi-res animácie ginger mačky" />
</p>

<p align="center">
  <em>DigiCat v3.0.0 — Profesionálne 64×64 sprite animácie ryšavej mačky (247 snímok, 38 animačných sekvencií, RGB565 PROGMEM, škálované na 3× / 192×192 px).</em>
</p>

---

## 🌟 Funkcie

### 🚀 v3.0.0 Hlavná architektonická aktualizácia
- **Vykresľovanie cez LVGL 9:** Nahradenie starého Arduino_GFX systému za LVGL 9.1 s dvojitým bufferingom (2× 480×480 v PSRAM) a DMA prenosmi, prinášajúce plynulé animácie a hardvérové vyhladzovanie hrán (anti-aliasing).
- **Luxusný astronomický chronograf:** Prémiový ciferník inšpirovaný švajčiarskou hodinarinou s 24-hodinovým solárnym oblúkom a obiehajúcim slnkom s pásmami zlatej hodinky, fotorealistickou 3D raymarchovanou lunárnou sférou s fotografickou textúrou NASA LROC, subciferníkom 24-hodinového trendu teploty s krivkou sparkline a svietiacim 30 FPS sekundovým prstencom s Gaussian bloom efektom.
- **Modernizácia Glass Cockpit & radarové funkcie:** 360-stupňová letecká kompasová ružica, prediktívne 1-minútové vektory rýchlosti letu, voliteľný štýl cieľov (aerodynamické vektory chevron vs. siluety lietadiel), optimalizované viacriadkové telemetrické štítky (so smart skratkami letov ODKIAĽ>KAM) a prepínanie metrických/leteckých jednotiek.
- **Viaczdrojové mapové podklady:** Možnosť prepínania medzi offline vektorovými hranicami štátov a mestami (`EuBorder`) a online rastrovými mapovými dlaždicami (Esri World Dark Gray Canvas, OpenStreetMap Standard) kešovanými do flash pamäte.
- **Taktický sonarový radar:** Obrazovka v štýle vojenského sonaru so zeleným fosforovým CRT skenovaním (25 FPS), analógovým doznievaním stopy, dopplerovským farebným kódovaním rýchlosti (približovanie azúrová, vzďaľovanie jantárová, kolmý let zelená), akustickým sonarovým pípnutím pri zameraní kontaktu a 3 režimami zobrazenia (klasický kruhový PPI sweep, rozdelený Split režim s BTR spektrogramom, celoobrazovkový vodopád).
- **3D sledovanie dráhy ISS (Space Command):** Reálna 3D ortografická guľová projekcia Zeme so scanline raytracingom, atmosférickým Rayleighovým halo efektom, reálnym solárnym terminátorom deň/noc, dráhou letu, rádiovým dosahom a 3 režimami pohľadu (3D ISS, 3D Domov, 2D mapa).
- **DigiCat 2.0 Aero-Cat letový kopilot:** Taktický vojenský režim Top Gun so slnečnými leteckými okuliarmi, animovaným mini radarovým displejom, živou telemetriou 6-osého IMU senzora (náklon, rotácia, gyroskopický kurz) a generovanými myšlienkami cez Google Gemini AI.
- **Automaticky miznúce priesvitné tlačidlá zoomu:** Sklenené plávajúce tlačidlá `+` a `-` na pravej strane radarov, ktoré sa zobrazia dotykom a po 3,5 sekundách nečinnosti automaticky zmiznú.
- **Rýchle ovládacie centrum (Quick Control):** Sťahovací panel z horného okraja displeja s priamymi prepínačmi jasu, zvuku, stôp letu, štýlu cieľov, mapových podkladov, sonarového pípnutia a spúšťača DigiCat.
- **Nulová fragmentácia PSRAM:** Dynamické riadkové vzorkovanie PNG obrázkov obmedzujúce alokáciu na 307 KB na snímku, čo zaručuje bezchybnú stabilitu pri prehrávaní radarov SHMÚ a ČHMÚ bez pádov pamäte.

### 🕒 Luxusný astronomický chronograf
- **24-hodinový solárny oblúk súmraku:** Vonkajší prstenec zobrazujúci pohyb Slnka (obed na 12. hodine, polnoc na 6. hodine), astronomický východ a západ slnka, 24K zlaté pásma rannej a večernej zlatej hodinky a dynamický symbol Slnka obiehajúci podľa aktuálneho miestneho solárneho času.
- **Fotorealistická 3D lunárna komplikácia:** Horný subciferník s fotografickou NASA LROC albedo textúrou (92×92 px) vykresľovaný s 3D Lambertovým difúznym osvetlením, zatienením okraja, plynulou fázovou líniou tieňa a percentom osvetlenia.
- **Subciferník vývoja teploty:** Dolný subciferník s 12 lúčmi, vonkajšou teplotou a elektricky modrou krivkou vývoja teploty sparkline so svietiacou koncovou perlou. Ťuknutie prepne priamo na 3-dňovú predpoveď počasia.
- **Dynamická sekundová stopa a žiara:** Elektricky modrý sústredný kruh s plynulou obiehajúcou sekundovou perlou (~30 FPS) a aktívnym Gaussian bloom oblúkom od 12. hodiny po aktuálnu sekundu.
- **Švajčiarska vyhladzovaná typografia:** Čisté biele 76px číslice digitálneho času (HH:MM) s vyhladzovanými okrajmi a lokalizovaným dátumom.
- **Interaktívne výstražné lišty:** Plávajúci štítok preletu lietadla priamo nad vami a štítok blížiaceho sa dažďa s možnosťou prekliknutia na príslušný radar.

### 🛩️ Sledovanie lietadiel & Glass Cockpit radar
- **360° radar** (14–200 km) cez adsb.fi / adsb.lol s kružnicami dosahu, letiskami a 360° leteckou kompasovou ružicou.
- **Voľba štýlu cieľov**: Možnosť prepínania medzi aerodynamickými vektormi chevron (s dvojitým chevronom pre stíhačky) a siluetami lietadiel.
- **Prediktívne vektory rýchlosti**: 1-minútové vodiace čiary kurzu letu s koncovým indikátorom rýchlosti.
- **Avionické dátové štítky**: 3-riadkové plávajúce bloky (volací znak alebo ODKIAĽ>KAM, letová hladina a trend vertikálnej rýchlosti, rýchlosť a trasa) s ochranou pred prekrývaním a prepínaním metrických/leteckých jednotiek.
- **Špeciálne kategórie**: Záchranári (zelená), vládne lety (zlatá), obrie lietadlá (azúrová), vojenské (červená) — pulzujúce kruhy a alert štítok navrchu.
- **Auto-fokus na núdzové lety** (7500/7600/7700): uzamkne lietadlo, stmaví ostatné, zobrazí živú telemetriu.
- **Vektor k najbližšiemu lietadlu** so vzdialenosťou, smerom a prevýšením.
- **Offline databáza trás**: volací znak → odlet/prístup (napr. `BOJ→WAW`).
- **Aero Glass Flight Deck HUD**: Ťuknutie na lietadlo otvorí kartu s reálnou fotografiou z Planespotters.net; ťuknutie na fotografiu otvorí celoobrazovkový HUD bez orezania s kompletnou leteckou telemetriou.

### 🌧️ Radar zrážok & Nowcasting
- Animované radarové slučky (SHMÚ / ČHMÚ / RainViewer) s plynulým prechodom, kruhovou maskou a riadkovým vzorkovaním bez pamäťovej fragmentácie.
- **TREC 2D nowcasting**: upozorňuje iba keď zrážky skutočne smerujú k vám (ETA ≤ 35 min, odchýlka ≤ 6 km).
- **Automatická klasifikácia**: Dážď / Dážď so snehom (1–3°C) / Sneh (≤1°C) / Krupobitie (>50 dBZ).

### 🛰️ Taktický radar, Sonar, ISS, YouTube & Finance
- **Kombinovaný taktický radar**: radar zrážok + živá ADS-B premávka na jednej obrazovke s avionikou Glass Cockpit.
- **Taktický akustický sonar**: 25 FPS CRT fosforové skenovanie, BTR spektrogram, dopplerovské rýchlosti a akustické pípnutie.
- **3D sledovanie ISS**: 3D ortografická guľa Zeme s Rayleighovým atmosférickým halo, slnečným terminátorom deň/noc, orbitami a 3 režimami pohľadu (3D ISS, 3D Domov, 2D mapa).
- **YouTube analytika**: živý počet odberateľov, celkové zhliadnutia a štatistiky nového / najsledovanejšieho videa (YouTube Data API v3).
- **Finančné trhy**: až 8 vlastných tickerov (ETF, akcie, krypto, forex) s čiarovým a sviečkovým grafom (Yahoo Finance v8).

### 🌤️ Počasie, Kvalita ovzdušia & Nočný režim
- Hodinové krivky teploty, vetra a zrážok, 3-dňová predpoveď, AQI, PM2.5 a peľ (Open-Meteo).
- **Ultra nočný režim** (0,5% jas) a `nightClockOnly` (zamkne displej na ciferníku v nočných hodinách).

### 🐾 DigiCat — Virtuálny spoločník (v3.0.0)
Celoobrazovková interaktívna zásuvka (Swipe Up zospodu alebo ikona labky v Ovládacom centre).

- **247 snímok · 38 animačných sekvencií** — profesionálne 64×64 RGBA sprite animácie ryšavej mačky, RGB565 PROGMEM, škálované na **3× (192×192 px)**.
- **Aero-Cat letový kopilot**: Taktický vojenský režim Top Gun so zlatými leteckými okuliarmi, animovaným mini radarom a živou telemetriou 6-osého IMU senzora (PITCH, ROLL, GYRO KURZ).
- **Bohatá pokojová osobnosť (9 sub-animácií)**: vrtenie chvosta v sede, lízanie labky v sede/v ľahu, mňaukanie v sede/v ľahu/v stoji, škrabanie ľavého/pravého ucha, zívanie — náhodne každých 4–10 s.
- **Šťastná reakcia (3 varianty)**: vrtenie chvosta v sede, v stoji spredu, v stoji vpravo — cykli každých 1,5–4 s.
- **Lovenie lietadla (3 varianty)**: státie + mávnutie, sedenie + pravá labka, sedenie + ľavá labka — vylosuje sa pri každom stretnutí.
- **5 náhodných spánkových póz**: variant + smer ležania zamknutý na celú noc.
- **Smerovo-vedomá lokomócia**: autentická `cat_run_left` pri vstupe sprava; `cat_slide_left` pri náklone doľava.
- **Doplnky podľa počasia**: dáždnik (dážď/búrka), pletený šál (sneh/zima ≤2°C), letecké okuliare (jasno).
- **Dráha reagujúca na počasie**: suchý asfalt / mokrý lesklý povrch / snehový poprašok.
- **Autonómny mačací mozog**: náhodný príchod, prechádzky, výskoky, umývanie, strečing, výlety z obrazovky každých 6–12 s.
- **Interakcia s lietadlami**: lietadlo letí s kondenzačnými stopami → DigiCat beží a chňapá; lietadlo uhýba; DigiCat skáče v naháňačke.
- **Nočné správanie**: 2-minútový čas bdenia pri interakcii; zívanie → strečing → spánok; ťuknutie = prebudenie.
- **Štatistiky zvieratka**: Hlad & Šťastie (0–100%); letecké XP hodnosti (*Mačací kadet → Radarový navigátor → Letecké eso*).
- **Google Gemini Live AI**: voliteľné prepojenie s Gemini Flash pre živé komentovanie premávky (100% offline fallback, SK/CZ/EN).

### ⚡ Systém & Konektivita
- **Dvojjadrový FreeRTOS**: Jadro 1 pre LVGL 9 RGB (dvojitý framebuffer, nulové blikanie) + dotyk; Jadro 0 pre sieť, radar a ADS-B.
- **Stabilný ST7701 ovládač**: kalibrovaný 8 MHz RGB clock, bez zápisov do Flash pri striedaní obrazoviek, auto-resync VSYNC.
- **Hardvérové RTC** (PCF85063) s voliteľným superkondenzátorom (3,3 V, 1,0–1,5 F).
- **Webový panel**: konfigurácia, OTA, živý sériový monitor (64 KB PSRAM), virtuálne zrkadlo displeja s dotykovou simuláciou, diaľkové ovládanie.
- **Smart Home REST API**: `/api/status`, `/api/screen`, `/api/display/resync`, `/api/toggle-legends`, `/api/rtc/sync_ntp`.
- **Aktívny bzučiak**: núdzové squawk alarmy, búrkové výstrahy, sonarové pípnutie ISS, akustické skenovacie pípnutie, hodinové odbíjanie, nočné stíšenie.

---

## 📱 Prehľad obrazoviek

| Obrazovka | Náhľad | Popis | Zdroj |
| :--- | :---: | :--- | :--- |
| **0. Hodiny** | <img src="docs/media/clock_luxury.png" width="70" /> | Luxusný astronomický chronograf (24h solárny oblúk, 3D mesiac, krivka teploty) | Open-Meteo |
| **1. Lietadlá** | <img src="docs/media/plane_radar_live.png" width="70" /> | Glass Cockpit ADS-B radar, 360° kompas, vektory rýchlosti, mapové podklady | adsb.fi / adsb.lol |
| **1b. Detail lietadla** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Aero Glass Flight Deck HUD s kompletnou telemetriou a reálnou fotografiou | Planespotters.net |
| **2. Radar zrážok** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animovaná slučka radaru, pamäťovo bezpečné dekódovanie, nowcasting zrážok | SHMÚ / ČHMÚ / RainViewer |
| **3. Taktický radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | Radar zrážok + živá ADS-B premávka v štýle Glass Cockpit | SHMÚ / ČHMÚ + adsb.fi |
| **4. Sonar** | <img src="docs/media/screen_sonar.png" width="70" /> | CRT fosforový radar, BTR spektrogram, dopplerovské rýchlosti, akustické pípnutie | adsb.fi |
| **5. Predpoveď** | <img src="docs/media/forecast_screen.png" width="70" /> | Hodinové krivky teploty, 3-dňová predpoveď, AQI, PM2.5, peľ | Open-Meteo |
| **6. Trhy & Krypto** | <img src="docs/media/finance_screen.png" width="70" /> | Až 8 vlastných tickerov, čiarový a sviečkový graf | Yahoo Finance v8 |
| **7. Dráha ISS** | <img src="docs/media/screen_iss_live.png" width="70" /> | 3D ortografická guľa Zeme, slnečný terminátor, Rayleighovo halo, orbity | WhereTheISS API |
| **8. YouTube** | <img src="docs/media/youtube_screen.png" width="70" /> | Živý počet odberateľov, celkové zhliadnutia, štatistiky nového / top videa | YouTube Data API v3 |
| **9. Štatistiky letov & Info** | <img src="docs/media/flight_stats_screen.png" width="70" /> | 24h aktivita vzdušného priestoru v 6 rozsahoch, telemetria pamäte SRAM a PSRAM | PSRAM Tracker |
| **10. Nastavenia** | <img src="docs/media/settings_screen.png" width="70" /> | Telemetria zariadenia, IP, jas, jazyk, poskytovateľ máp, zvuk, OTA | Systém |

---

## 🖐️ Dotykové ovládanie a gestá

| Gesto | Výsledok |
| :--- | :--- |
| **Potiahnutie doľava / doprava** | Predchádzajúca / nasledujúca obrazovka |
| **Stiahnutie z horného okraja** | Rýchle ovládacie centrum (jas, nočný režim, stopy letu, štýl cieľov, mapy) |
| **Potiahnutie zospodu nahor** | Zásuvka DigiCat |
| **Ťuknutie na pravý okraj (Radary)** | Zobrazenie priesvitných tlačidiel zoomu `+` / `-` (zmiznú po 3,5 s) |
| **Ťuknutie na tlačidlá `+` / `-`** | Priblíženie / oddialenie dosahu radaru |
| **Ťuknutie na dolný subciferník (Hodiny)** | Prechod na obrazovku Predpovede počasia |
| **Ťuknutie na výstražný štítok (Hodiny)** | Prechod na Lietadlá (prelet) alebo Radar zrážok (dážď) |
| **Ťuknutie na lietadlo** | Otvorenie detailnej karty s reálnou fotografiou |
| **Ťuknutie na fotografiu** | Otvorenie celoobrazovkového Aero Glass Flight Deck HUD (ťuknutie = späť) |
| **Dvojité ťuknutie na obrazovku (Radary)** | Prepnutie čistého režimu mapy (skrytie / zobrazenie legiend a textov) |
| **Ťuknutie na štítok vpravo hore (Sonar)** | Prepnutie režimu zobrazenia (Klasický PPI → Rozdelený Split → Vodopád) |
| **Ťuknutie na spodný dosah (Sonar)** | Prepnutie rozsahu sonaru (25, 50, 100, 150 km) |
| **Ťuknutie na guľu Zeme (ISS)** | Prepnutie režimu zobrazenia (3D ISS → 3D Domov → 2D mapa) |
| **Ťuknutie na mačku (DigiCat)** | Pohladkanie (pradenie, dialóg); ťuknutie na čelo prepína Top Gun režim |
| **Dvojité ťuknutie na mačku** | Kŕmenie maškrtou (obnoví hlad a šťastie) |
| **Podržanie tlačidla BOOT ~3 s** | Továrenský reset (vymazanie Wi-Fi a nastavení) |

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
- `MeteoPlaneRadar-v3.0.0-factory.bin` — kompletný obraz (bootloader + partície + firmvér).
- Flash cez [ESP Web Flasher](https://espressif.github.io/esptool-js/) pri 921600 baud od adresy `0x00000000`.
- Alebo: `esptool.py -p COM_PORT -b 921600 write_flash 0x0 MeteoPlaneRadar-v3.0.0-factory.bin`

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
| `/api/screen` | POST | Prepnutie obrazovky `{"index": 0}` (0–10) |
| `/api/display/resync` | POST | Hardvérová resynchronizácia displeja |
| `/api/toggle-legends` | POST | Prepnutie čistého režimu mapy |
| `/api/rtc/sync_ntp` | POST | Vynútenie NTP → RTC synchronizácie |

---

## 📜 Licencia & Poďakovanie

MIT License.
- Pôvodný projekt: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**
- DigiCat herné mechaniky: **[aquascape123/digicat](https://github.com/aquascape123/digicat)** (MIT)
- Vylepšenia, SHMÚ radar, ISS tracker, ciferníky, dotykové ovládanie, DigiCat hi-res sprite, AI mačací mozog, webový panel, slovenská lokalizácia: **Rado & Antigravity AI**

