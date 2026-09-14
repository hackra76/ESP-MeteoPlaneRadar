# MeteoPlaneRadar (Taktický a meteorologický radar pre ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v1.9.2-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteorologická stanica, živý letecký ADS-B radar, animovaný radar zrážok (SHMÚ, ČHMÚ, RainViewer), kombinovaný taktický radar, sledovanie stanice ISS a dizajnové ciferníky hodín na okrúhlom 2.1" IPS dotykovom displeji.**  
Vyvinuté špeciálne pre vývojovú dosku **Waveshare ESP32-S3-Touch-LCD-2.1** s modernými dotykovými gestami v štýle smartfónu, sťahovacím Ovládacím centrom (Control Center), reálnymi fotografiami lietadiel, bilineárnym vyhladzovaním radaru a responzívnym webovým rozhraním pre diaľkové ovládanie a kompletnú konfiguráciu.

> 🇬🇧 English documentation: **[README.md](README.md)**  
> 📌 Forknuté a výrazne vylepšené z pôvodného projektu **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.

---

## 📸 Živé ukážky zo zariadenia

<p align="center">
  <img src="docs/media/tactical_radar_live.gif" width="16%" alt="Taktický radar (Lietadlá + Zrážky)" />
  <img src="docs/media/screen_iss_live.png" width="16%" alt="Sledovanie dráhy ISS a deň/noc mapa" />
  <img src="docs/media/finance_screen.png" width="16%" alt="Finančné trhy a krypto" />
  <img src="docs/media/plane_detail_photo.png" width="16%" alt="Detail lietadla s fotkou" />
  <img src="docs/media/weather_radar_chmu.gif" width="16%" alt="Animovaný radar zrážok" />
  <img src="docs/media/clock_stacked_bold.png" width="16%" alt="Ciferník Stacked Bold" />
</p>

<p align="center">
  <em>Zľava doprava: <b>Taktický radar</b>, <b>Sledovanie ISS</b> (solárny terminátor deň/noc, minulá a budúca trajektória, kruh viditeľnosti), <b>Trhy & Krypto</b> (živé grafy a tickery), <b>Detail lietadla</b>, <b>Meteorologický radar</b>, <b>Ciferník Stacked Bold</b>.</em>
</p>

---

## 🌟 Implementované funkcie

### 🕒 1. Bohatá kolekcia 7 unikátnych ciferníkov hodín
Okrúhly 480×480 displej ponúka **7 odlišných geometrických štýlov ciferníka** s plynulým prepínaním zvislým potiahnutím prsta (Swipe Hore / Dole):
1. **Digitálny klasický (Classic Digital)** – čistý horizontálny čas, dátum, počasie, 3-hodinová minipredpoveď, vietor a fáza mesiaca.
2. **Letecký kokpitový analóg (Aviator Cockpit)** – pilotné hodinky s luminiscenčnými ručičkami, hodinovými indexmi a dvoma sub-ciferníkmi.
3. **🚀 Planetárne prstence (Orbital Gauges)** – sci-fi dizajn tvorený tromi sústrednými kruhovými oblúkmi (minúty, hodiny, sekundy) s dorastajúcimi svetelnými perlami.
4. **🛩️ Stíhací priehľadový displej (Fighter HUD)** – Head-Up Display z bojového lietadla s umelým horizontom, zameriavacím krížom (`SYS·TGT·LOCK`), hornou kompasovou páskou a indikátormi vetra.
5. **⏱️ Astronomický regulátor (Régulateur Chrono)** – mechanický observatórny chronometer s oddelenými osami ručičiek.
6. **🔲 Vertikálna typografia (Stacked Bold)** – moderný smartwatch dizajn (Pixel / Nothing) s obrovským dvojčíslom hodín a minút.
7. **Nordic minimalistický (Minimal)** – masívny, vysoko kontrastný čas čitateľný z veľkej diaľky.

### ⏱️ 2. Zobrazenie sekúnd na okrúhlom obvode
Až **7 štýlov sekundového prstenca**:
- `Vypnuté`, `Bodka`, `Plynulý oblúk`, `Pulz`, `Radarový lúč (Sweep)`, `Hodinárske indexy (Ticks)`, `Satelit na orbite (Orbit)`.

### 🛩️ 3. Pokročilé sledovanie letov & Inteligentný Alert HUD
- **360° radar vzdušného priestoru:** Živá letecká premávka v okruhu 25–300 km cez adsb.fi / adsb.lol.
- **Špeciálne kategórie letov:** Záchranári (zelená), vládne lety (zlatá), obrie a ikonické lietadlá (azúrová) a vojenské lety (červená) sú automaticky identifikované a zvýraznené pulzujúcim kruhom.
- **Notifikačný Alert štítok:** Okamžité upozornenie na radare pod časom pri výskyte dôležitého letu (napr. `! Zachranny vrtulnik: ATE02 (18 km) !`).
- **Núdzové lety (Squawk 7500, 7600, 7700):** Automatické uzamknutie a sledovanie lietadla v núdzi so živou telemetriou.
- **Vektor k najbližšiemu lietadlu:** Dynamická čiara ukazujúca smer, vzdialenosť a prevýšenie k najbližšiemu lietadlu.
- **Letiská & Databáza trás:** Zobrazenie letísk v okolí a offline dekódovanie letových trás z volacích znakov (napr. `Burgas -> Warsaw [BOJ>WAW]`).

### 🔍 4. Detailná karta lietadla so živými fotografiami
- Po kliknutí na lietadlo na radare sa zobrazí kompletná farebná telemetrická karta.
- Živá fotografia konkrétneho stroja sťahovaná z **Planespotters.net API**.
- Možnosť zväčšenia fotografie na celú plochu okrúhleho displeja.

### 🌧️ 5. Animovaný radar zrážok (SHMÚ, ČHMÚ, RainViewer)
- Slučka radarových kompozitov s vysokým rozlíšením a plynulým prelínaním snímok v čase.
- 256 kB vyrovnávacia pamäť v PSRAM pre stabilitu aj pri intenzívnych plošných búrkach.
- Voliteľný bilineárny anti-aliasing pre organické vyhladenie zrážkových polí.

### ⚡ 6. Predpoveď príchodu zrážok (TREC Nowcasting) & Typizácia
- **2D vektorový nowcasting:** Priestorová krížová korelácia sleduje rýchlosť a smer zrážkových buniek. Upozornenie sa aktivuje iba a výhradne vtedy, ak zrážky smerujú k vašej polohe ($v_{radial} > 0$, minutie $\le 15\text{ km}$, $\text{ETA} \le 60\text{ min}$).
- **Automatická klasifikácia zrážok:** Rozlíšenie na **Dážď**, **Dážď so snehom** ($1^\circ\text{C}\dots3^\circ\text{C}$), **Sneh** ($\le 1^\circ\text{C}$) alebo **Krúpy** ($>50\text{ dBZ}$).
- **Výstražný widget na hodinách:** Štítok s odpočtom príchodu zrážok priamo na ciferníku s preklikom na radar.

### 🛰️ 7. Kombinovaný taktický radar (`ScreenTactical`)
- Unikátna obrazovka spájajúca **animovanú zrážkovú oblačnosť v pozadí** a **letovú prevádzku v reálnom čase v popredí** na jednom zjednotenom taktickom displeji.

### 🌤️ 8. 3-dňová predpoveď počasia & Kvalita ovzdušia
- Hodinové krivky teploty, pravdepodobnosti zrážok a vetra cez Open-Meteo.
- 3-dňový prehľad, index kvality ovzdušia (AQI), PM2.5 a peľový monitoring.

### 📈 9. Finančné trhy a krypto (`ScreenFinance`)
- Živé trhové dáta cez Yahoo Finance v8 API.
- **4 voliteľné sloty:** Podpora pre európske ETF fondy (Amundi MSCI World, Stoxx Europe 600), americké indexy, akcie, komodity (zlato, ropa), kryptomeny a menové páry (forex).
- **Interaktívny sparkline graf:** Zobrazenie krivky vývoja ceny vybraného aktíva; ťuknutím prepínanie aktívneho grafu, dvojitým ťuknutím okamžitá obnova.

### 🛰️ 10. Sledovanie dráhy stanice ISS (`ScreenIss`)
- **Astronomická mapa sveta Deň/Noc:** Mapa 320×160 s reálnym solárnym terminátorom počítaným v reálnom čase.
- **Trajektória obehu:** Minulá dráha (45 min) a predpoveď budúceho obletu (92 min).
- **Kruh viditeľnosti:** Zóna dosahu (~2 200 km) okolo stanice so zameriavačom domácej polohy.
- **Telemetrický HUD:** Okamžitá výška, rýchlosť, vzdialenosť, odpočet do ďalšieho preletu a vrcholová elevácia.
- **Akustické upozornenie:** Sonarový ping (`BEEP_SONAR_PING`) pri vstupe stanice do zóny viditeľnosti.

### 📊 11. Denné štatistiky letov & Obrazovka Informácie (`ScreenInfo`)
- 24-hodinový prehľad letovej premávky: počet unikátnych lietadiel za deň, rýchlostný rekord, maximálny dosah, rozpätie letových hladín a celkový počet správ.
- **Konfigurovateľný filter:** Počítanie všetkých prijatých lietadiel alebo iba strojov v aktuálnom polomere (zoome) na obrazovke Lietadiel. Automatický reset o polnoci.

### 🔊 12. Akustický výstražný systém (Aktívny bzučiak)
- Vstavaný aktívny bzučiak s tónmi pre núdzový squawk (7700/7600/7500), sledovaný let, prelet nad hlavou, blížiace sa búrky, sonarový ping, celú hodinu a nočný kľud.

### 🌙 13. Ultra Night režim & Nočné hodiny
- **Hlboký červený nočný režim:** Monochromatický tmavočervený režim s minimálnym jasom (0.5%), ktorý chráni nočné videnie.
- **Režim `nightClockOnly`:** Zastaví rotáciu obrazoviek počas spánku a uzamkne displej stlmený na hodinách.

### 📱 14. Rýchle ovládacie centrum & Dotykové gestá
- Sťahovacia lišta z horného okraja pre okamžité ovládanie jasu, nočného režimu, stlmenia bzučiaka, legiend a cyklovania.
- Prirodzené dotykové gestá: potiahnutie do strán pre prepínanie obrazoviek, zvislé potiahnutie pre zmenu ciferníka a zoom radaru.

### 🕒 15. Hardvérový RTC čip (PCF85063) & Prevádzka bez batérií
- Automatická synchronizácia palubného RTC čipu s NTP serverom alebo hodinami prehliadača.
- Podpora osadenia 3.3V superkondenzátora na piny `BAT` a `GND` pre zachovanie času bez batérií počas výpadkov napájania.

### 🖥️ 16. Spoľahlivý radič displeja ST7701
- Kalibrovaný 8 MHz RGB pixel clock s rozšírenými synchronizačnými intervalmi (`HBP 50`, `VPW 8`, `VBP 20`), 4 MHz SPI inicializácia a zapnutie displeja až po nábehu RGB taktovania eliminujúce posun a zalamovanie obrazu.
- Manuálne nastavenie orientácie displeja (0°, 90°, 180°, 270°).

### ⚡ 17. Bezpečná dvojjadrová FreeRTOS architektúra
- **Jadro 1 (Core 1):** Plynulé vykresľovanie ST7701 (dvojitý framebuffer bez blikania), čítanie dotyku CST820 a animácie.
- **Jadro 0 (Core 0):** Asynchrónny worker na pozadí spracováva TLS spojenia, radarové kompozity, ADS-B JSON dáta a webový server s kooperatívnou sieťovou arbitrážou.

### 🌐 18. Responzívne webové rozhranie & Bezchybné OTA
- Kompletná konfigurácia zariadenia, diaľkové prepínanie obrazoviek a ovládanie.
- Živé telemetrické tabuľky pre ADS-B lety a ISS.
- Bezdrôtový webový sériový monitor (64 kB PSRAM ring buffer) cez Wi-Fi bez káblov.
- Nástroj na zachytenie obrazovky do 24-bitového BMP.
- **Bezchybné OTA aktualizácie:** Automatické uvoľnenie PSRAM (>6 MB) pred nahrávaním a dynamický záložný mechanizmus priameho zápisu do flash pamäte.

### 🔌 19. REST API pre inteligentnú domácnosť
- JSON koncové body pre integráciu s Home Assistant, Node-RED alebo skriptami (`/api/status`, `/api/hardware`, `/api/screen`, `/api/toggle-legends`, `/api/rtc/sync_ntp`).

---

## 📱 Prehľad obrazoviek

| Obrazovka | Náhľad | Popis | Zdroj dát |
| :--- | :---: | :--- | :--- |
| **1. Hodiny** | <img src="docs/media/clock_stacked_bold.png" width="70" /> | 7 voliteľných ciferníkov (Stacked Bold, Aviator, HUD atď.), minipredpoveď, počasie, fáza mesiaca, solárny oblúk | Open-Meteo & Astro engine |
| **2. Lietadlá** | <img src="docs/media/plane_radar_live.png" width="70" /> | 360° radar vzdušného priestoru, núdzové kódy (7700/7600), aerolínie a trasy, letiská, vzdialenostné kružnice | adsb.fi / adsb.lol |
| **2b. Detail lietadla** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Po kliknutí na lietadlo sa zobrazí kompletná telemetria, trasa odkiaľ-kam a fotografia daného stroja | Planespotters.net API |
| **3. Meteoradar** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animovaná radarová slučka zrážok s plynulým prelínaním, legendou odrazivosti dBZ a mestami | SHMÚ (SK), ČHMÚ (CZ), RainViewer |
| **4. Taktický radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | **Kombinovaný taktický pohľad:** Živá zrážková oblačnosť + prelety lietadiel na jedinej obrazovke | SHMÚ / ČHMÚ / RainViewer + adsb.fi |
| **5. Predpoveď** | <img src="docs/media/forecast_screen.png" width="70" /> | Hodinové krivky teploty, vetra a zrážok, 3-dňový výhľad, index kvality ovzdušia (AQI), PM2.5 a peľ | Open-Meteo Weather & Air Quality |
| **6. Trhy a Krypto** | <img src="docs/media/finance_screen.png" width="70" /> | Živé sledovanie 4 konfigurovateľných trhových tickerov (ETF, akcie, komodity, kryptomeny, forex) so sparkline grafmi | Yahoo Finance v8 |
| **7. Sledovanie dráhy ISS** | <img src="docs/media/screen_iss_live.png" width="70" /> | Globálna mapa sveta s reálnym solárnym terminátorom deň/noc, trajektória obehu, kruh viditeľnosti a odpočet preletu | WhereTheISS API |
| **8. Štatistiky letov** | <img src="docs/media/flight_stats_screen.png" width="70" /> | Denná 24h štatistika: počet unikátnych lietadiel, rýchlostný rekord, letové hladiny, max dosah, ADS-B správy | FreeRTOS PSRAM Tracker |
| **9. Nastavenia** | <img src="docs/media/settings_screen.png" width="70" /> | Stav zariadenia, IP adresa, regulácia jasu, orientácia mapy, voľba jazyka a vyhladenie | Systém |

---

## 🖐️ Dotykové gestá a ovládanie

| Gesto / Akcia | Funkcia |
| :--- | :--- |
| **Potiahnutie doľava / doprava** | Plynulé prepnutie na nasledujúcu / predchádzajúcu obrazovku s animáciou. |
| **Stiahnutie z horného okraja** | Otvorenie **Rýchleho ovládacieho centra (Control Center)** (jas, nočný režim, prepínače). |
| **Potiahnutie hore / dolu v strede** | **Na hodinách:** Prepnutie na predchádzajúci / ďalší štýl ciferníka.<br>**Na radaroch:** Zoom In (hore) / Zoom Out (dolu).<br>**V Ovládacom centre:** Zatvorenie menu. |
| **Dotyk na spodnú lištu rozsahu** | Ľavá polovica oddiali (Zoom Out), pravá polovica priblíži (Zoom In). |
| **Dotyk na lietadlo** | Zobrazenie detailnej karty lietadla s fotkou, trasou a telemetriou. |
| **Dotyk na fotografiu lietadla** | Zväčšenie fotografie na celú obrazovku (opätovné ťuknutie vráti detail). |
| **Dvojité poklepanie (telo / stôl)** | **Na radaroch:** Čistý režim mapy (skrytie legiend).<br>**Na Trhoch / ISS:** Vynútenie okamžitej obnovy dát z internetu. |
| **Podržanie tlačidla BOOT pri štarte (~3 s)** | Továrenský reset (vymazanie uloženej Wi-Fi a nastavení NVS). |

---

## 🔧 Hardvérové špecifikácie

Vyvinuté špeciálne pre **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Komponent | Špecifikácia |
| :--- | :--- |
| **MCU** | Espressif ESP32-S3R8 (Xtensa® Dual-Core 32-bit LX7 @ 240 MHz) |
| **Pamäť** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Displej** | Okrúhly 2.1" IPS, 480×480 px, 65k RGB565 farieb, ST7701 RGB zbernica |
| **Dotyk** | CST820 / CHSC6540 kapacitný dotykový ovládač (I2C) |
| **I/O Expandér** | TCA9554PWR (riadenie napájania displeja, podsvietenia a resetu) |
| **IMU Senzor** | QMI8658 6-osý akcelerometer a gyroskop (detekcia poklepania a orientácie) |
| **RTC Čip** | PCF85063 hodiny reálneho času so záložným napájaním (I2C `0x51`) |
| **Konektivita** | USB-C, Wi-Fi 802.11 b/g/n (2.4 GHz) |

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

### 2. Nastavenie Wi-Fi (Captive Portal)
1. Pri prvom štarte zariadenie vytvorí otvorenú Wi-Fi sieť s názvom **`MeteoPlaneRadar`** a zobrazí QR kód.
2. Naskenujte QR kód alebo sa pripojte na sieť `MeteoPlaneRadar` z mobilu/PC.
3. Otvorte adresu **`http://192.168.4.1/`**.
4. Vyberte vašu domácu Wi-Fi sieť, zadajte heslo a uložte.
5. Zariadenie sa pripojí a zobrazí svoju pridelenú IP adresu.

### 3. Webové rozhranie
Po pripojení otvorte prehliadač na adrese:
- **`http://meteoplaneradar.local/`** (alebo cez IP adresu, napr. `http://192.168.0.7/`).

Webové rozhranie ponúka:
- **Lokalita:** Automatická GeoIP detekcia alebo manuálny výber mesta/GPS súradníc.
- **Štýl hodín:** Výber ciferníka, sekundového prstenca a farebných akcentov.
- **Nastavenia radaru:** Voľba poskytovateľa zrážok (SHMÚ / ČHMÚ / RainViewer), vyhladzovanie, zobrazenie letísk a trás.
- **Nočný režim hodín (`nightClockOnly`):** Zastaví cyklické prepínanie obrazoviek v noci a uzamkne stlmený ciferník.
- **Hardvérová diagnostika & Webový sériový monitor:** Vstavaná live konzola sériového monitora cez web (64 KB PSRAM buffer) bez nutnosti USB pripojenia, RTC hodiny a synchronizačné tlačidlá.
- **Diaľkové ovládanie:** Prepínanie obrazoviek a zmena zoomu priamo z prehliadača.
- **OTA Aktualizácia:** Pohodlné nahrávanie nového `.bin` firmvéru vzduchom cez Wi-Fi bez nutnosti pripájať USB kábel.

---

## 📜 Licencia a autori

Vydané pod licenciou **MIT License**.
- Pôvodný základ projektu: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.
- Vylepšenia, slovenská lokalizácia, integrácia SHMÚ radaru, bilineárne vyhladzovanie, Planespotters fotografie lietadiel, taktický kombinovaný radar, RTC ovládač, IMU gestá, dotyková navigácia, rýchle Ovládacie centrum, watchlist lietadiel a multi-core optimalizácia: **Rado & Antigravity AI**.
