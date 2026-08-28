# MeteoPlaneRadar (Slovenská verzia s Taktickým radarom)

**Hodiny, radar lietadiel, zrážkový meteoradar, kombinovaný taktický radar a predpoveď počasia na okrúhlom dotykovom displeji.**  
Funguje na vývojovej doske **Waveshare ESP32-S3-Touch-LCD-2.1** a konfiguruje sa jednoducho cez webový prehliadač.

> 🇸🇰 Tento projekt je rozšíreným forkom pôvodného projektu **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)** od **[chiptron.cz](https://chiptron.cz)**.  
> 🇬🇧 English version: **[README_EN.md](README_EN.md)**

---

## 🌟 Hlavné vylepšenia v tomto forku

* 🇸🇰 **Plná podpora slovenského jazyka (`LANG_SK`)** – kompletná diakritika na displeji, v captive portáli, OTA rozhraní aj vo webovej administrácii (prepínač `Slovenčina` / `Čeština` / `English`).
* 🛰️ **Nová obrazovka Taktického radaru (`ScreenTactical`)** – kombinovaný pohľad v reálnom čase, ktorý spája zrážkový meteoradar (podklad) a živú letovú prevádzku ADS-B (popredie) na jednom displeji vrátane kurzu, výšky, volacích znakov a interaktívneho detailu lietadla po klepnutí.
* 🔍 **Zosúladenie 5 úrovní priblíženia naprieč obrazovkami** – `10/25 km`, `50 km`, `100 km`, `200 km` a `Celé Slovensko / Celá ČR` (`0 km`).
* 🇸🇰 **Zobrazenie celej krajiny podľa polohy** – pri maximálnom oddialení meteoradaru a taktického radaru sa na základe polohy automaticky vycentruje a zobrazí celé územie Slovenska (polomer 250 km, Zoom 6) alebo ČR.
* 🏙️ **Optimalizovaná databáza slovenských miest (`SkCitiesData.h`)** – 17 vybraných a prehľadne rozmiestnených centier s 2-písmenovými skratkami a hierarchiou zobrazenia, ktorá zabraňuje prekrývaniu textu.
* ⚡ **Optimalizácia sieťových požiadaviek v pozadí** – lietadlá, zrážkový radar a peľové správy sa sťahujú výhradne pre aktívne/povolené obrazovky, čo šetrí pamäť aj procesor.
* 🛠️ **Kompletná integrácia s PlatformIO (`platformio.ini`)** – jednoduchá kompilácia a nahrávanie do ESP32-S3 jedným klikom/príkazom.

---

## 📱 Obrazovky

| Obrazovka | Popis | Zdroj dát |
| --- | --- | --- |
| **Hodiny** | Aktuálny čas, dátum, vonkajšia teplota, ikona počasia a sekundový prstenec | Open-Meteo |
| **Lietadlá** | Živá letová prevádzka v okolí, kurzy, výšky, núdzové squawky, detail letu s trasou | adsb.fi, adsb.lol |
| **Meteoradar** | Animovaný zrážkový radar | ČHMÚ alebo RainViewer |
| **Taktický radar** | **Kombinovaný pohľad:** Zrážky + lietadlá v reálnom čase na jednej obrazovke | RainViewer / ČHMÚ + adsb.fi |
| **Předpoveď** | Hodinová predpoveď (6 h), denný výhľad (3 dni), kvalita ovzdušia (AQI, PM2.5) a peľová situácia | Open-Meteo |
| **Nastavenia** | Prehľad stavu, IP adresa, jas, orientácia mapy, jazyk a WiFi | — |

Všetky dátové obrazovky sa dajú vo webovom rozhraní zapnúť/vypnúť podľa preferencií; obrazovka Nastavenia je dostupná vždy.

---

## 🔧 Hardware

Projekt je pripravený pre dosku **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:
- **MCU:** ESP32-S3R8 (Dual-Core 240 MHz, 8 MB Octal PSRAM, 16 MB Flash)
- **Displej:** Okrúhly 2.1" IPS dotykový displej (480×480 px, ST7701 RGB interface)
- **Dotyk:** CST820 (kapacitný, I2C)
- **I/O expandér:** TCA9554 (napájanie a reset LCD)
- **Pripojenie:** USB-C (nie je potrebné nič spájkovať).

---

## 🚀 Prvé spustenie a konfigurácia

1. Nahrajte firmvér do dosky (cez USB alebo PlatformIO).
2. Po zapnutí doska vytvorí otvorenú WiFi sieť **`MeteoPlaneRadar`** (na displeji sa zobrazí QR kód).
3. Pripojte sa k nej smartfónom alebo počítačom a otvorte adresu **`http://192.168.4.1/`**.
4. Vyberte vašu domácu WiFi sieť, zadajte heslo a uložte.
5. Po pripojení nájdete webové rozhranie na adrese:
   - **`http://meteoplaneradar.local/`** (alebo na IP adrese zobrazenej na obrazovke Nastavenia).

### Webové rozhranie
Web beží trvalo na pozadí a umožňuje:
- Nastavenie polohy (vrátane vyhľadania mesta).
- Výber aktívnych obrazoviek a automatického striedania (interval v sekundách).
- Výber zdroja zrážok (ČHMÚ pre ČR / RainViewer pre Európu a svet).
- Denný a nočný jas (manuálne alebo automaticky podľa východu/západu slnka).
- Filtre letovej hladiny, sledovanie konkrétneho volacieho znaku a núdzových squawkov.
- Diaľkové ovládanie prepínania obrazoviek a zmeny priblíženia bez dotyku displeja.
- Export a import nastavení, OTA aktualizáciu firmvéru.

---

## 🖐️ Dotykové ovládanie

| Gesto / Akcia | Funkcia |
| --- | --- |
| **Potiahnutie doľava / doprava (Swipe)** | Zmena priblíženia (rozsahu) radaru (10/25, 50, 100, 200 km, Celá krajina) |
| **Klepnutie na lietadlo (Tap)** | Otvorenie / zatvorenie detailu vybraného lietadla s trasou |
| **Klepnutie na spodok displeja** | Cyklovanie úrovní priblíženia radaru |
| **Dlhý stisk vľavo / vpravo** | Prepnutie na predchádzajúcu / nasledujúcu obrazovku |
| **Podržanie tlačidla BOOT pri štarte (~3 s)** | Továrenský reset (vymazanie nastavení a WiFi) |

---

## 💻 Kompilácia a nahrávanie (PlatformIO)

Projekt obsahuje kompletnú konfiguráciu `platformio.ini`:

1. Otvorte projekt vo **Visual Studio Code** s rozšírením **PlatformIO IDE**.
2. Pripojte dosku cez USB-C (port `USB`).
3. Spustite zostavenie a nahrávanie:
   ```powershell
   # Kompilácia
   pio run

   # Nahrávanie do dosky cez USB
   pio run -t upload --upload-port COM9
   ```

### OTA aktualizácia cez WiFi
V prehliadači otvorte **`http://meteoplaneradar.local/update`** a nahrajte súbor `.pio/build/esp32-s3-touch-lcd-21/firmware.bin`.

---

## 🌐 Zdroje dát

Projekt využíva bezplatné verejné rozhrania (určené len na osobné nekomerčné použitie):
- **Lietadlá, registrácie a typy:** [adsb.fi](https://adsb.fi)
- **Trasy letov:** [adsb.lol](https://adsb.lol)
- **Srážkový radar SR a svet:** [RainViewer](https://www.rainviewer.com)
- **Srážkový radar ČR:** [ČHMÚ](https://opendata.chmi.cz)
- **Počasie, predpoveď a kvalita ovzdušia:** [Open-Meteo](https://open-meteo.com)
- **Geokódovanie a poloha:** [ip-api.com](http://ip-api.com) a Open-Meteo Geocoding
- **Vektorová mapa:** Hranice Natural Earth (public domain), mestá GeoNames (CC BY 4.0).

---

## 📄 Pôvodný projekt a poďakovanie

Tento projekt je forkom a rozšírením práce komunity:
- **Pôvodný repozitár:** [petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar) (Petr / [chiptron.cz](https://chiptron.cz))
- **Článok a popis hardware:** [chiptron.cz: Meteoradar a radar letadel na kulatém displeji](https://chiptron.cz/meteoradar-a-radar-letadel-na-jednom-kulatem-displeji/)
- [ok1cdj/MeteoPlaneRadar](https://github.com/ok1cdj/MeteoPlaneRadar) – Ondra OK1CDJ (RainViewer a predpoveď)
- [CooLajz/waveshare-hodiny](https://github.com/CooLajz/waveshare-hodiny) (inšpirácia pre hodiny, nočný režim a UI)
- [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar) a [Selbyl](https://github.com/Selbyl/ESP32-S30Touch-LCD-2.1_Plane-Radar)

---

## 📜 Licencia

Projekt je zverejnený pod licenciou **MIT** – viď [LICENSE.txt](LICENSE.txt).
