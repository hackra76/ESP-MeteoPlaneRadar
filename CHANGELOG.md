# Changelog

Všechny podstatné změny v projektu **MeteoPlaneRadar**.
Formát vychází z [Keep a Changelog](https://keepachangelog.com/cs/1.1.0/),
verzování je [semantické](https://semver.org/lang/cs/).

Verze je v jediném místě: `MeteoPlaneRadar/Version.h` (`FW_VERSION`). Zobrazuje se na
obrazovce Nastavení, na webové stránce a v sériovém výpisu při startu.
Laditelné konstanty (krok otočení, tolerance výpadků, ladicí výpisy) jsou
pohromadě v `MeteoPlaneRadar/Config.h`.

## [1.9.5] - 2026-09-14

### Vylepšené / Improved
- **Presná lokálna detekcia zrážok a filtrácia falošných poplachov (Localized Nowcasting & Ground Clutter Filtering):**
  - Stav "Prebieha dážď / Prší" (`Raining now`) bol naviazaný na striktný fyzický rádius 3,0 km okolo reálnej GPS polohy používateľa namiesto predošlého kontrolovania fixných buniek v mriežke radaru, ktoré pri širokom zoome (150–300 km) nesprávne spúšťali hlásenie dažďa aj pri zrážkach vzdialených 20–40 km.
  - Zvýšený prah odrazivosti na mierny dážď ($\ge 50$ / $\approx 25\text{ dBZ}$) spoľahlivo odfiltroval radarový šum, vlhkosť v hraničnej vrstve a virgu (zrážky vyparujúce sa pred dopadom na zem).
  - Zúžený koridor zásahu blížiaceho sa zrážkového frontu z 15 km na 6 km a skrátené časové okno varovania zo 75 minút na $\le 35\text{ minút}$, vďaka čomu systém varuje výhradne pred bezprostredne hroziacim priamym zásahom.

## [1.9.4] - 2026-09-14

### Pridané / Added
- **Výber sledovaných aktív priamo na zariadení (On-Device Markets Asset Picker):**
  - Do sťahovacieho Ovládacieho centra na obrazovke Trhov (`SCREEN_FINANCE_I`) boli pridané 4 interaktívne výberové boxy pre všetky sledované pozície (Hero graf aj watchlist).
  - Vstavaný celoobrazovkový výberový dialóg s 6 kategóriami (ETF fondy, Krypto, Indexy, Komodity, Akcie, Forex) a 34 prednastavenými populárnymi aktívami s okamžitým uložením a vymazaním slotu jedným dotykom.
- **Voľba štýlu grafu: Čiarový vs Sviečkový graf (Line vs Candlestick Chart):**
  - Možnosť prepínania hlavného grafu trhov medzi klasickou čiarovou krivkou (Line) a sviečkovým grafom (Candlestick) používaným v profesionálnych obchodných platformách.
  - Sviečkový graf presne vizualizuje knôty najvyššej a najnižšej ceny (High / Low) a telá otváracej a zatváracej ceny (Open / Close) vo výrazných kontrastných farbách (zelená pre rastové sviečky, červená pre klesajúce).
  - Prepínanie štýlu grafu je dostupné priamo v Ovládacom centre na displeji aj vo webovom rozhraní s trvalým uložením v pamäti NVS (`finGr`).

## [1.9.3] - 2026-09-14

### Opravené / Fixed
- **Oprava online aktualizácie firmvéru priamo z displeja (On-Device GitHub OTA HTTP -1 Fix):**
  - Vyriešená chyba zlyhania TLS spojenia (`Update failed HTTP -1`) pri presmerovaní (HTTP 302 Redirect z `github.com` na úložisko binárnych súborov `release-assets.githubusercontent.com`). Predchádzajúca implementácia opakovane používala inštanciu `WiFiClientSecure`, ktorej interný kontext mbedTLS po ukončení spojenia neumožňoval čistý re-handshake so zmenou SNI (Server Name Indication) na inú doménu.
  - Každé presmerovanie teraz dôsledne uvoľňuje a vytvára čistú inštanciu `WiFiClientSecure` aj `HTTPClient`, čím je garantovaný správny handshake a autentifikácia s CDN úložiskom.
  - Proaktívne uvoľnenie všetkých vyrovnávacích pamätí (radarové snímky RainViewer, ČHMÚ, SHMÚ, počasie, fotografie lietadiel a trasy) sa teraz vykonáva okamžite pred inicializáciou TLS spojenia, čím je zabezpečený dostatok voľnej internej pamäte SRAM pre mbedTLS vyrovnávacie pamäte.
  - Optimalizovaná veľkosť zásobníka asynchrónnej úlohy OTA sťahovania z 20 kB na 14 kB pre úsporu internej SRAM.

## [1.9.2] - 2026-09-14

### Opravené / Fixed
- **Hardvérový posun a zalamovanie obrazu ST7701 (ST7701 Hardware Shift/Wrap Permanent Fix):**
  - Znížená inicializačná SPI frekvencia zo 40 MHz na 4 MHz (špecifikácia radiča ST7701 definuje SPI maximum na 10 MHz), predĺžený hardvérový resetovací interval na 120 ms.
  - Stabilný pixel clock RGB periférie ESP32-S3 nastavený na 8 MHz s precízne kalibrovaným časovaním horizontálnych a vertikálnych synchronizačných intervalov (HBP 50, HFP 10, VPW 8, VBP 20, VFP 10).
  - Explicitné riadenie CS zbernice a aktivácia displeja (príkaz `0x29`) až po stabilizácii živých RGB taktovacích signálov zabraňuje desynchronizácii interného riadkového čítača panela pri štarte a prepínaní obrazoviek.
- **Uvoľnenie pamäte PSRAM pred aktualizáciou firmvéru (OTA PSRAM Cache Reclamation & Zero-Fail Flash Fallback):**
  - Pri otvorení webovej stránky `/update` a na začiatku nahrávania súboru sa automaticky uvoľnia všetky veľké vyrovnávacie pamäte (snímky radarov RainViewer, ČHMÚ, SHMÚ, vyrovnávacia pamäť počasia, framebuffer taktického radaru, dekódované fotografie lietadiel a fronta letových trás), čím sa okamžite sprístupní viac ako 6 MB súvislej pamäte PSRAM.
  - V obidvoch aktualizačných motoroch (Web Dashboard OTA aj online GitHub OTA) implementovaný dynamický záložný mechanizmus priameho streamovania zápisu do flash pamäte (`direct-to-flash streaming`) pri nedostupnosti súvislého bloku PSRAM, vďaka čomu aktualizácia nikdy nezlyhá na chybe "Out of PSRAM memory" bez ohľadu na veľkosť nahrávaného binárneho súboru.

### Zmenené / Changed
- **Zmena gesta prepínania štýlu ciferníka na obrazovke Hodín (Clock Watchface Swipe Gesture):**
  - Dvojité ťuknutie na obrazovke hodín bolo zrušené z dôvodu nechceného a náhodného prepínania vzhľadu ciferníka.
  - Nový štýl ciferníka sa teraz prepína intuitívnym zvislým potiahnutím prsta (Swipe Hore / Dole).
- **Filtrovanie štatistiky lietadiel na obrazovke Informácie (Aircraft Count Statistics Filter):**
  - Pridaná voľba rozsahu počítania lietadiel: používateľ si môže zvoliť zobrazenie všetkých prijatých lietadiel v pamäti ADS-B alebo iba tých, ktoré sa nachádzajú v rámci aktuálne nastaveného polomeru (zoomu) na obrazovke Lietadiel. Prepínateľné ťuknutím priamo na štatistiku aj cez webové rozhranie.
- **Odstránenie automatického otáčania obrazovky podľa interných senzorov:**
  - Odstránená automatická rotácia displeja cez akcelerometer QMI8658 kvôli nechcenému pretáčaniu. Orientácia obrazovky je teraz plne pod kontrolou používateľa cez menu Nastavení a webový dashboard (0°, 90°, 180°, 270°).

## [1.9.1] - 2026-09-11

### Opravené / Fixed
- **Sťahovanie radarových snímok SHMÚ počas plošných zrážok (SHMÚ Radar Buffer Overflow Fix):**
  - Zvýšená alokácia vyrovnávacej pamäte `SHMU_MAX_PNG` zo 128 kB (`131072`) na 256 kB (`262144`) v externej PSRAM pamäti.
  - Pri intenzívnych plošných zrážkach a búrkach na území SR klesá efektivita kompresie PNG a veľkosť kompozitu `cmax.kruh` dosahuje až ~175 kB, čo spôsobovalo okamžité odmietnutie sťahovania v `Net_GetBinary()` s chybou `response does not fit into buffer` a výpadok celého radaru SHMÚ.
- **Kooperatívna sieťová koordinácia medzi jadrami (Dual-Core Network Arbitration):**
  - Zabránené súbežným TLS spojeniam medzi jadrom 0 (AsyncNetWorker: predpoveď, ADS-B, ISS) a jadrom 1 (sťahovanie radarových snímok) pomocou neblokujúcich stavových príznakov a kooperatívneho yieldovania, čím sa predišlo vyčerpaniu pamäte mbedTLS (chyba -10368 / -32512) bez rizika zablokovania watchdogu.
- **Východzia obrazovka po štarte (Always Boot into Clock Screen):**
  - Zariadenie po každom reštarte, resete alebo zotavení z pádu vždy nabieha priamo na obrazovku 1 (Hodiny / `SCREEN_CLOCK_I`), bez ohľadu na to, na akej obrazovke sa nachádzalo pred reštartom.
- **Odstránenie falošných poplachov zo starej vyrovnávacej pamäte (Stale Aircraft Cache Alert Suppression):**
  - Pri prepnutí na radar lietadiel alebo taktický radar sa poplašné zvuky bzučiaka (`Buzzer`) a výstražné lišty pre vojenské, núdzové a sledované lety aktivujú výhradne nad čerstvými živými dátami (`ADSB_IsFresh()`).
  - Ak sú dáta v pamäti staršie ako prah platnosti, audio poplach a hlásenie sa potlačia a na obrazovke sa zobrazuje stav načítavania (`Načítavam...`), kým nedorazia aktuálne živé dáta. Tým sa zabránilo falošnému spusteniu poplachu a okamžitému vypnutiu, ak už lietadlo dávno opustilo vzdušný priestor.
- **Rýchlejšia odozva dotykových gest (Instant Touch Swipes):**
  - Prepnutie obrazovky vľavo/vpravo a stiahnutie horného Ovládacieho centra sa po prekročení prahovej vzdialenosti aktivujú okamžite počas pohybu prsta bez nutnosti čakať na jeho zdvihnutie z dotykovej plochy.

## [1.9.0] - 2026-09-10

### Pridané / Added
- **Nová obrazovka Sledovanie ISS & Obežnej dráhy (ISS Orbit Tracker - `SCREEN_ISS_I`):**
  - Plnohodnotná nová obrazovka zaradená medzi Trhy & Krypto a Informácie (`Markets` -> `ISS` -> `Info`).
  - Globálna mapa sveta (360×180 equirectangular) s dynamickým vykresľovaním dňa a noci (solárny terminátor vypočítaný podľa reálneho slnečného času a deklinácie).
  - Vykreslenie trajektórie ISS: minulá dráha (45 min, prerušovaná čiara) a budúca predpovedaná dráha (92 min, plná jantárová krivka).
  - Kruh zemského horizontu / viditeľnosti z paluby stanice (~2 200 km) s pulzujúcim stredovým piktogramom ISS a zameriavačom domácej polohy.
  - Telemetrický HUD panel: Výška (Altitude v km), Rýchlosť (Velocity v tisíckach km/h), Šikmá vzdialenosť od pozorovateľa (Distance v km) a Elevácia / výškový uhol nad obzorom.
  - Spodná karta stavu preletu: Indikátor viditeľnosti (IN RANGE / OUT OF RANGE), Azimut stanice so svetovou stranou, osvetlenie (Sunlit / In Eclipse), odpočet nasledujúceho preletu a maximálna elevácia.
  - Akustický sonarový ping (`BEEP_SONAR_PING`) pri vstupe ISS do zóny viditeľnosti zo zeme.
  - Interaktívne dotykové tlačidlo zapnutia/vypnutia zvukového upozornenia priamo na displeji a možnosť vynútenia obnovy dvojitým ťuknutím.
- **Webové rozhranie pre správu a živú telemetriu ISS:**
  - Samostatná karta "🛰️ ISS" v prehliadači s okamžitým prepínačom na displej a zaradením do kolobehu obrazoviek.
  - Živá tabuľka telemetrických dát (zemepisná šírka, dĺžka, rýchlosť, stav viditeľnosti, azimut a elevácia) aktualizovaná cez REST API.
  - Prepínač akustického upozornenia pri prelete (`issAlert`).
