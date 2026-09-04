# MeteoPlaneRadar (Taktický a meteorologický radar pre ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v1.5.6-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteo-radarová stanica, živý letecký radar, zrážkový meteoradar (SHMÚ, ČHMÚ, RainViewer), taktický kombinovaný radar a dizajnové hodiny na okrúhlom 2.1" IPS dotykovom displeji.**  
Vyvinuté pre hardvérovú dosku **Waveshare ESP32-S3-Touch-LCD-2.1** s moderným dotykovým ovládaním podobným smartfónu, vysúvacím Ovládacím centrom (Control Center), fotkami lietadiel, bilineárnym vyhladzovaním a diaľkovým ovládaním cez responzívny webový prehliadač.

> 🇸🇰 Tento projekt je pokročilým forkom a významným rozšírením pôvodného projektu **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)** od **[chiptron.cz](https://chiptron.cz)**.  
> 🇬🇧 English version: **[README.md](README.md)**

---

## 📸 Ukážka zariadenia v akcii (Live Demo)

<p align="center">
  <img src="docs/media/tactical_radar_live.gif" width="31%" alt="Taktický radar (Lietadlá + Zrážky)" />
  <img src="docs/media/weather_radar_chmu.gif" width="31%" alt="Animovaný meteoradar ČHMÚ / SHMÚ" />
  <img src="docs/media/clock_stacked_bold.gif" width="31%" alt="Ciferník Stacked Bold" />
</p>

<p align="center">
  <em>Zľava doprava: <b>Taktický radar</b> (live lietadlá nad zrážkovou mapou), <b>Zrážkový meteoradar</b> (slučka búrkovej oblačnosti), <b>Moderný ciferník Stacked Bold</b>.</em>
</p>

---

## 🌟 Kľúčové novinky vo verzii v1.5.5

- 🇸🇰 **Oficiálny radar SHMÚ (Slovensko):** Priama natívna podpora sťahovania vysoko detailných radarových kompozitov CMAX priamo zo serverov Slovenského hydrometeorologického ústavu (SHMÚ) s oficiálnou farebnou škálou zrážok (dBZ / mm/h).
- 🇨🇿 **Radar ČHMÚ & 🌍 Globálny RainViewer:** Okamžité prepínanie medzi poskytovateľmi zrážkových dát (SHMÚ, ČHMÚ, RainViewer) priamo cez dotykové menu, web alebo nastavenia.
- 🎨 **Bilineárne vyhladzovanie zrážkového radaru (Anti-Aliasing):** Pri vysokom priblížení (zoom 25 km a 50 km) algoritmus bilineárnej interpolácie odstraňuje kockatý "pixel-art" efekt a vykresľuje zrážkové polia a búrkové bunky v hodvábne plynulých farebných prechodoch. Prepínač je dostupný v QuickControl, na obrazovke Nastavení aj na webe.
- 📷 **Reálne fotografie lietadiel (Planespotters.net):** Po kliknutí na lietadlo sa v detailnej karte popri trase a telemetrii načíta aj reálna farebná fotografia konkrétneho lietadla s uvedením mena fotografa.
- 📐 **Vyladený dizajn obrazoviek:**
  - **Predpoveď počasia (Forecast):** Dokonale vertikálne aj horizontálne vycentrované rozloženie hodín a denných predpovedí na kruhovom displeji.
  - **Nastavenia (Settings):** Tlačidlá preusporiadané do prehľadnej mriežky 2×2 (Jednotky, Vyhladenie, Jazyk, Reset WiFi) s čistým odstupom od podpisu autora.
- 🚀 **Optimalizované webové rozhranie:** Odstránený zbytočný canvas náhľad displeja, čo radikálne znížilo zaťaženie CPU ESP32 a ušetrilo pamäť RAM a sieťovú prevádzku. Diaľkové ovládanie prepínania obrazoviek a zoomu zostáva plne funkčné.
- 🛡️ **Extrémne stabilné jadro FreeRTOS:** Sieťový worker disponuje zväčšeným stackom 24 KB, vláknovo bezpečnými mutexmi a bezpečnou správou LwIP TCP socketov zabraňujúcou reštartom pamäte.

---

## 🌟 Kľúčové funkcie a inovácie

### 🕒 1. Bohatá kolekcia 7 unikátnych ciferníkov hodín
Okrúhly 480×480 displej ponúka **7 odlišných geometrických štýlov ciferníka** s okamžitým prepínaním cez web, Ovládacie centrum alebo dvojitým poklepaním na telo prístroja:
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
- **Špeciálne kategórie letov:** Záchranári (zelená), vládne lety (zlatá), obrie a ikonické lietadlá (azúrová) a vojenské lety (červená) sú automaticky identifikované a zvýraznené pulzujúcim kruhom.
- **Notifikačný Alert štítok:** Okamžité upozornenie na radare pod časom pri výskyte dôležitého letu (napr. `! Zachranny vrtulnik: ATE02 (18 km) !`).
- **Núdzové lety (Squawk 7500, 7600, 7700):** Automatické uzamknutie a sledovanie lietadla v núdzi so živou telemetriou.
- **Vektor k najbližšiemu lietadlu:** Dynamická čiara ukazujúca smer, vzdialenosť a prevýšenie k najbližšiemu lietadlu.
- **Letiská & Databáza trás:** Zobrazenie letísk v okolí a offline dekódovanie letových trás z volacích znakov (napr. `Burgas -> Warsaw [BOJ>WAW]`).

### 🛰️ 4. Taktický radar (`ScreenTactical`)
Unikátna obrazovka kombinujúca **zrážkový radar (SHMÚ / ČHMÚ / RainViewer) v pozadí** a **letovú prevádzku v reálnom čase v popredí**.

### 🕒 5. Hardware RTC čip (PCF85063) & Časová nezávislosť
- Automatická synchronizácia palubného RTC čipu s internetovým časom.
- Po odpojení od napájania beží čas ďalej (podpora pripojenia 1.0F / 1.5F 3.3V superkondenzátora na piny `BAT` a `GND`).
- Tlačidlá vo webovom rozhraní pre okamžitú synchronizáciu s NTP serverom alebo priamo z hodín prehliadača.

### ⚡ 6. Bezpečná dvojjadrová FreeRTOS architektúra (Dual-Core)
- **Jadro 1 (Core 1):** Vyhradené výhradne pre plynulé vykresľovanie displeja ST7701 (dvojitý framebuffer bez blikania), čítanie dotyku CST820 a detekciu gest z IMU.
- **Jadro 0 (Core 0):** Asynchrónny worker (`AsyncNetWorker`) na pozadí spracováva TLS šifrovanie, sťahuje radarové snímky, komunikuje s ADS-B API a obsluhuje webový server.

---

## 📱 Prehľad obrazoviek

| Obrazovka | Popis | Zdroj dát |
| :--- | :--- | :--- |
| **1. Hodiny** | 7 voliteľných ciferníkov, 3-hodinová minipredpoveď, 7 štýlov sekúnd, počasie, vietor, fáza mesiaca a solárny oblúk | Open-Meteo & Astro engine |
| **2. Lietadlá** | Živá radarová mapa letov, fotky lietadiel, trasy, letiská, vzdialenostné kružnice, sledovanie špeciálnych letov | adsb.fi / adsb.lol / Planespotters |
| **3. Meteoradar** | Animovaný zrážkový radar s vyhladzovaním a možnosťou zmeny poskytovateľa | SHMÚ (SK), ČHMÚ (CZ), RainViewer (Svet) |
| **4. Taktický radar** | **Kombinovaný pohľad:** Zrážková oblačnosť + lietadlá na jednej mape v reálnom čase | SHMÚ / ČHMÚ / RainViewer + adsb.fi |
| **5. Predpoveď** | 5-hodinový detail, 3-dňový výhľad a kvalita ovzdušia (AQI) | Open-Meteo Weather & Air Quality |
| **6. Nastavenia** | Stav zariadenia, IP adresa, jas, orientácia mapy, voľba jazyka, vyhladenie a správa WiFi | Systém |

---

## 🖐️ Dotykové gestá a ovládanie

| Gesto / Akcia | Funkcia |
| :--- | :--- |
| **Potiahnutie doľava / doprava** | Plynulé prepnutie na nasledujúcu / predchádzajúcu obrazovku s animáciou. |
| **Stiahnutie z horného okraja** | Otvorenie **Rýchleho ovládacieho centra (Control Center)** (jas, nočný režim, prepínače). |
| **Potiahnutie hore / dolu v strede** | **Na radaroch:** Zoom In (hore) / Zoom Out (dolu).<br>**V Ovládacom centre:** Zatvorenie menu. |
| **Dotyk na spodnú lištu rozsahu** | Ľavá polovica oddiali (Zoom Out), pravá polovica priblíži (Zoom In). |
| **Dotyk na lietadlo** | Zobrazenie detailnej karty lietadla s fotkou, trasou a telemetriou. |
| **Dotyk na fotografiu lietadla** | Zväčšenie fotografie na celú obrazovku (opätovné ťuknutie vráti detail). |
| **Dvojité poklepanie (telo / stôl)** | **Na hodinách:** Prepnutie na ďalší ciferník.<br>**Na radaroch:** Čistý režim mapy (skrytie legiend). |
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
- **Hardvérová diagnostika:** Kontrola I2C zbernice, RTC hodín a synchronizačné tlačidlá.
- **Diaľkové ovládanie:** Prepínanie obrazoviek a zmena zoomu priamo z prehliadača.
- **OTA Aktualizácia:** Pohodlné nahrávanie nového `.bin` firmvéru vzduchom cez Wi-Fi bez nutnosti pripájať USB kábel.

---

## 📜 Licencia a autori

Vydané pod licenciou **MIT License**.
- Pôvodný základ projektu: **Petr / [chiptron.cz](https://chiptron.cz)** ([petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)).
- Vylepšenia, slovenská lokalizácia, integrácia SHMÚ radaru, bilineárne vyhladzovanie, Planespotters fotografie lietadiel, taktický kombinovaný radar, RTC ovládač, IMU gestá, dotyková navigácia, rýchle Ovládacie centrum, watchlist lietadiel a multi-core optimalizácia: **Rado & Antigravity AI**.
