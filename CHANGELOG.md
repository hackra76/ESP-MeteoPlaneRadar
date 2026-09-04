# Changelog

Všechny podstatné změny v projektu **MeteoPlaneRadar**.
Formát vychází z [Keep a Changelog](https://keepachangelog.com/cs/1.1.0/),
verzování je [semantické](https://semver.org/lang/cs/).

Verze je v jediném místě: `MeteoPlaneRadar/Version.h` (`FW_VERSION`). Zobrazuje se na
obrazovce Nastavení, na webové stránce a v sériovém výpisu při startu.
Laditelné konstanty (krok otočení, tolerance výpadků, ladicí výpisy) jsou
pohromadě v `MeteoPlaneRadar/Config.h`.

## [1.5.6] - 2026-09-04

### Pridané / Added
- **Celoobrazovkový náhľad fotografie lietadla (Fullscreen Aircraft Photo Mode):**
  - V detailnej karte lietadla (na obrazovke *Lietadlá* aj *Taktický radar*) stačí ťuknúť na rámček s fotografiou lietadla.
  - Fotografia sa proporcionálne zväčší cez celú plochu okrúhleho 480×480 displeja bez orezania rohov a bez skreslenia pomeru strán.
  - V hornej časti sa zobrazí hlavička s volacím znakom, typom a registráciou lietadla, v dolnej časti kredit fotografa (Planespotters.net) a navigačný tip.
  - Opätovným ťuknutím kamkoľvek na obrazovku sa displej okamžite vráti späť na štandardnú kartu detailu s telemetriou a trasou letu.
- **Korektné škálovanie náhľadu fotky (Aspect Ratio Fitting):**
  - Náhľad v rámčeku detailnej karty lietadla sa teraz proporcionálne prispôsobuje pomeru strán snímky, takže nikdy nedochádza k orezaniu bokov ani pretečeniu rámčeka.
  - Pridaný jemný indikátor `+` v rohu fotky signalizujúci možnosť zväčšenia.

---

## [1.5.5] - 2026-09-04

### Zmenené / Changed
- **Zarovnanie obrazovky Predpoveď počasia (ScreenForecast):**
  - Posunutie obsahu o 20 px nižšie (`HOUR_Y0` posunuté z 58 na 78 px) pre dokonalé vertikálne vyváženie v kruhovom výreze displeja.
  - Horizontálne zarovnanie stĺpcov symetricky na stred (`CX = 240`) s rovnomernými 85 px okrajmi z oboch strán.
- **Preusporiadanie tlačidiel na obrazovke Nastavenia (ScreenSettings):**
  - 4 tlačidlá (Jednotky, Vyhladenie, Jazyk, Reset WiFi) boli usporiadané do prehľadnej mriežky 2×2 namiesto jedného dlhého vertikálneho stĺpca.
  - Vznikla tak veľkorysá 78-pixelová rezerva medzi tlačidlami a menom autora ("H4CKR4"), čím sa úplne odstránilo prekrývanie prvkov v dolnej časti displeja.
- **Optimalizácia webového rozhrania (WebConfig):**
  - Kompletne odstránený živý náhľad displeja (HTML5 canvas) a dopytovací endpoint `/api/live`.
  - Výrazné zníženie zaťaženia CPU ESP32, sieťovej prevádzky a RAM (ušetrené periodické serializácie JSON a volania na pozadí).
- **Prečistenie kódu (Codebase cleanup):**
  - Odstránené pozostatky po vývoji a testovaní, dočistené nepoužívané premenné a dočasné bloky kódu pri zachovaní 100% funkcionality.

---

## [1.5.4] - 2026-09-04

### Pridané / Added
- **Bilineárne vyhladzovanie zrážkového radaru (Anti-aliasing / Bilinear Filtering) pre SHMÚ a ČHMÚ:**
  - Pri väčšom priblížení (zoom 25 km a 50 km) bol orez národných radarových kompozitov (cca 50×50 px) na okrúhly 480×480 displej silne pixelizovaný (schodovité bloky pixelov).
  - Implementovaná vysoko optimalizovaná bilineárna interpolácia s celočíselnou (fixed-point 8-bit) matematikou a predpočítanou mapovacou tabuľkou v `ScreenWeather.cpp` (`blitCrop`) aj `ScreenTactical.cpp` (`buildTacticalComposite`).
  - Radarové odrazy a zrážkové polia majú teraz krásne hladké, plynulé prechody a prirodzený vzhľad.
- **Prepínač vyhladzovania radaru (Radar smoothing toggle):**
  - Trvalé uloženie voľby do NVS pamäte (`Settings_SmoothRadar()` / `Settings_SetSmoothRadar()`, predvolene zapnuté).
  - Prepínač je prístupný priamo na 4 miestach:
    1. **Rýchle menu (QuickControl):** na obrazovke *Meteoradar* aj *Taktický radar* tlačidlo `Vyhladenie: ZAP / VYP`.
    2. **Obrazovka Nastavení (Settings):** nové tlačidlo `Vyhladenie radaru: ZAP / VYP`.
    3. **Webové rozhranie (WebConfig):** zaškrtávacie pole `Vyhladenie radarových dát (bilineárna interpolácia pre SHMÚ a ČHMÚ)`.
    4. Dynamická okamžitá zmena bez nutnosti reštartu zariadenia.

---

## [1.5.3] - 2026-09-04

### Opravené / Fixed
- **Odstránenie pádu LwIP (`pbuf_free: p->ref > 0`) pri dopytoch na trasy lietadiel:**
  - **Príčina:** Sieťový worker task na jadre 0 mal stack iba 8 KB. Pri vykonávaní mbedTLS TLS handshakeu, HTTPClient a následnom parsovaní cez ArduinoJson dochádzalo k vyčerpaniu a pretečeniu stacku, čo korumpovalo hlavičky paketových bufferov LwIP (`pbuf`) na heape. Navyše dopyty na trasy lietadiel bežali v rýchlom 1,5s intervale, čím sa hromadili neuzatvorené TCP sockety v stave FIN_WAIT.
  - **Riešenie:**
    - Veľkosť stacku úlohy `AsyncNetWorker` bola zvýšená z 8 KB na **24 KB** (vďaka 136 KB ušetrenej internej RAM v verzii 1.5.2 je pre task k dispozícii obrovská rezerva).
    - V `Route.cpp` a `PlanePhoto.cpp` sa socket po prečítaní tela explicitne uzatvára cez `client.stop()` s ochranným oneskorením 100 ms, aby LwIP TCP stack stihol korektne dokončiť `FIN/ACK` handshake.
    - Interval dopytovania trás lietadiel na pozadí bol upravený na stabilných **3,5 s**, čím sa zamedzilo preťažovaniu TCP stacku a verejného API `api.adsb.lol`.
    - Opravená manipulácia s expirovanými záznamami v cache pri presune z fronty (`Route_Tick`).

---

## [1.5.2] - 2026-09-04

### Zásadná optimalizácia pamäte / Core Memory Architecture Fix
- **Trvalé vyriešenie chýb alokácie pamäte pre SSL (`SSL - Memory allocation failed -32512`):**
  - **Príčina:** Inštancie dekodéra `PNGdec` (`PNG`) zaberali v internej SRAM pamäti (`.bss`) 45,6 KB každá. Pretože `RainViewer.cpp`, `ScreenWeather.cpp` a `ScreenTactical.cpp` mali každá svoju vlastnú statickú inštanciu, plytvalo sa **136,8 KB internej SRAM** pamäte (z celkových 320 KB čipu). Pri otvorení viacerých obrazoviek alebo sťahovaní zostávalo pre mbedTLS len cca 30 KB, čo spôsobovalo trvalé zlyhávanie `start_ssl_client` na chybu `-32512`.
  - **Riešenie:** Všetky objekty `PNG` dekodérov a riadkové buffery boli presunuté do externej **8 MB PSRAM** pamäte (`MALLOC_CAP_SPIRAM`).
  - **Výsledok:** Využitie internej RAM kleslo z **60.9 % na iba 19.2 %**! Uvoľnilo sa vyše **136 KB internej pamäte** pre LwIP sieťový zásobník a mbedTLS handshaky. Voľná interná pamäť pri štarte vzrástla z cca 156 KB na takmer 293 KB.
- **Oprava pádu LwIP (`pbuf_free: p->ref > 0 assert`):**
  - V `Route.cpp`, `SHMU.cpp`, `CHMU.cpp` a `PlanePhoto.cpp` sa pri chybovom HTTP kóde (napr. HTTP 500) teraz pred zatvorením socketu vyprázdnia prichádzajúce dáta (`while (client.available()) client.read();`). Tým sa zabráni násilnému TCP `RST` a kolízii v paketových bufferoch LwIP.
  - Zvýšená doba usadenia medzi TLS dopytmi (`delay(150)` namiesto `delay(30)`) zabraňuje prekrývaniu FIN/ACK handshakeov.
  - Zvýšené bezpečnostné prahy pre sieťové dopyty v `Config.h` (`NET_MIN_HEAP 45000`, `NET_MIN_BLOCK 24000`).

---

## [1.5.1] - 2026-09-04

### Opravené / Fixed
- **Podpora meteoradaru SHMÚ a ČHMÚ na obrazovke Tactical (Kombinovaný radar):**
  - Obrazovka Tactical pôvodne podporovala iba podklad z RainVieweru (`rvMode()`); pri voľbe SHMÚ alebo ČHMÚ bol podklad čierny a zobrazovali sa len lietadlá.
  - Implementované plné orezávanie a renderovanie kompozitných radarových máp (SHMÚ aj ČHMÚ) priamo do 480×480 framebufferu na pozadí lietadiel.
  - Okamžitý prepočet orezu pri zmene dosahu (25, 50, 100, 200 km, Celá krajina) priamo z pamäte PSRAM bez zbytočného opakovaného sťahovania zo siete.
  - Asynchrónne periodické sťahovanie (Core 0) koordinované tak, aby nekolidovalo s TLS spojeniami pre ADS-B a fotografie lietadiel.
  - Okamžitá reakcia pri prepnutí zdroja radaru cez vysúvacie menu QuickControl.
- **Oprava zlyhania TLS pamäte pri sťahovaní zo SHMÚ (`SSL - Memory allocation failed -32512`):**
  - Dôsledné uvoľňovanie a scoping `WiFiClientSecure client` pred začiatkom sťahovania snímok.

---

## [1.5.0] - 2026-09-04

### Pridané / Added
- **Zobrazenie fotografie lietadla (Planespotters.net):**
  - Po kliknutí na lietadlo sa v hornej časti okrúhleho displeja zobrazí miniatúra skutočného lietadla podľa registrácie alebo ICAO kódu.
  - Využitie rýchleho hardvérovo akcelerovaného JPEG dekodéra `bitbank2/JPEGDEC` s optimalizáciami pre ESP32-S3 SIMD.
  - Zobrazenie mena autora fotografie („Photo: <fotograf>“).
  - Asynchrónne sťahovanie na jadre 0 (Core 0) bez blokovania vykresľovania radarovej scény na jadre 1.
  - Viacúrovňová vyrovnávacia pamäť v PSRAM pre okamžité zobrazenie pri prepínaní lietadiel.
- **Nový 2-stĺpcový HUD panel detailu lietadla:**
  - Moderný dvojstĺpcový layout s prehľadnými parametrami letu:
    - Callsign a typ lietadla (napr. `ETD87A B789`)
    - Vľavo: Výška (`ALT`), Rýchlosť (`SPD`), Vzdialenosť od pozorovateľa (`DIST`)
    - Vpravo: Vertikálna rýchlosť stúpania/klesania (`V/S`), Kurz (`HDG`), Squawk kód (`SQK`)
    - Dole: Zelený štítok trasy letu (odkiaľ -> kam)
  - Radar a spodné indikátory dosahu zostávajú na pozadí viditeľné a interaktívne.

---

## [1.4.0] - 2026-09-04

### Pridané / Added
- **Integrácia slovenského meteoradaru SHMÚ (Slovensko):**
  - Pridaný plnohodnotný tretí poskytovateľ radarových dát popri ČHMÚ a RainViewer.
  - Sťahovanie priamych ostrých 8-bitových PNG kompozitov (800 × 550 px) z `shmu.sk` s 5-minútovou frekvenciou.
  - Vynikajúce priestorové rozlíšenie cca 0,94 km/px (ostré jadrá zrážok namiesto hrubých blokov z RainVieweru).
  - Minimálna latencia (nové snímky dostupné do 2–4 minút od merania).
  - Filtrovanie masky mimo dosahu radaru pre čisté zobrazenie na okrúhlom displeji.
  - Špecifická stupnica intenzity zrážok (dBZ / mm/h) pre SHMÚ kompozit.
  - Podpora v rýchlom menu (Quick Control Center), webovom rozhraní a automatické prispôsobenie zobrazenia celého Slovenska.

---

## [1.3.0] - 2026-09-03

### Pridané / Added
- **Moderné dotykové gestá a Plynulé prechody obrazoviek (Slide Transitions):**
  - **Horizontálny swipe (doľava / doprava):** Okamžité a plynulé prepínanie medzi obrazovkami.
  - **Plynulá animácia prechodu (Slide Transition):** Dvojitý hardvérový framebuffer v PSRAM zabezpečuje bezblikajúce odsunutie a nasunutie novej obrazovky s ease-out krivkou (~100 ms).
  - **Stiahnutie z horného okraja (Pull-down):** Vyvolanie nového rýchleho ovládacieho centra (**Quick Control Center**).
  - **Vertikálny swipe v strede:** Rýchly zoom na radaroch (hore = Zoom In, dole = Zoom Out) alebo zatvorenie ovládacieho centra.
  - **Klepnutie na spodnú lištu rozsahu:** Dotyková zmena zoomu (ľavá polovica = zmenšenie, pravá polovica = zväčšenie).
- **Rýchle ovládacie centrum (Quick Control Center):**
  - Translucentné tmavé vysúvacie menu z horného okraja.
  - Okamžitá regulácia jasu podsvietenia (tlačidlá `[-]`, `[+]` s krokom 15%).
  - Rýchly prepínač nočného režimu (`Noc: AUTO / ZAP / VYP`) a skratka do Nastavení.
  - Kontextové prepínače pre aktívnu obrazovku: Letiská, Okruhy, Popisy lietadiel, Trasy, Prepínanie radarového zdroja ČHMÚ/RainViewer, cyklovanie ciferníkov, solárny oblúk a auto-rotácia.
- **Inteligentné sledovanie zaujímavých lietadiel (Watchlist & Alert HUD):**
  - Automatická detekcia a špeciálne vizuálne zvýraznenie:
    - **Záchranárske vrtuľníky (HEMS):** ATE, Kryštof, HZS, SAR, LZZ, typy H135, H145, EC35, EC45, A109, UH60 $\rightarrow$ smaragdovo zelený pulzujúci kruh.
    - **Vládne špeciály (VIP):** SSG (Letecký útvar MV SR), CEF (Vzdušné sily AČR), IAM, GAF, COTAM $\rightarrow$ kráľovský zlatý kruh.
    - **Ikonické stroje a obry:** Airbus A380, Boeing 747, Antonov An-124/225, Airbus Beluga $\rightarrow$ azúrový kruh.
    - **Vojenské lety:** detekované cez databázu, typy a volacie znaky NATO, JAS, ALCA, TIGER, F16, F35... $\rightarrow$ červený kruh.
    - **Používateľský watchlist:** zhodné s nastaveným filtrom $\rightarrow$ fialový purpurový kruh.
  - **Alert HUD štítok:** Automatické upozornenie v hornej časti radaru pod časom pri výskyte špeciálneho letu v dosahu (napr. `! Zachranny vrtulnik: ATE02 (18 km) !`).
- **Ľudsky čitateľné názvy lietadiel & Farebný telemetrický detail:**
  - ICAO kódy sa automaticky prekladajú do zrozumiteľných názvov (napr. `A320` $\rightarrow$ `Airbus A320`, `B38M` $\rightarrow$ `Boeing 737 MAX 8`, `AT76` $\rightarrow$ `ATR 72-600`).
  - Zjednotený detailný panel lietadla s farebne odlíšenými údajmi: žltý/červený volací znak, azúrová výška, biela rýchlosť a kurz, dynamická zelená/červená pre stúpanie/klesanie, jantárový typ a registrácia, zelené odletové a jantárové cieľové letisko.
- **Bohatší ciferník hodín (Minipredpoveď počasia):**
  - 3 moderné kapsuly (pills) priamo na obrazovke hodín s hodinovou predpoveďou na najbližšie 3 hodiny (`+1h`, `+2h`, `+3h`) vrátane WMO ikoniek a teplôt.
- **Auto-rotácia podľa polohy (QMI8658 IMU):**
  - Automatické prispôsobenie orientácie radaru gravitácii pri otočení zariadenia o 90°, 180° alebo 270°.
  - Rýchly prepínač `[Auto-rotácia: ZAP / VYP]` priamo v Quick Control Center.

---

## [1.2.0] - 2026-09-02

### Pridané / Added
- **7 unikátnych dizajnových štýlov ciferníka (Clock Styles):**
  - Classic Digital (digitálny klasický s počasím a fázou mesiaca)
  - Aviator Cockpit Analog (letecký analóg s 2 sub-ciferníkmi)
  - Orbital Gauges (3 sústredné sci-fi oblúky minút, hodín a sekúnd)
  - Fighter HUD (stíhací priehľadový displej s umelým horizontom, boresight krížom a uzamknutým časom)
  - Régulateur Chrono (vysokohodinársky trojosový regulátor s veľkou centrálnou minútovou ručičkou)
  - Stacked Bold Typography (moderná smartwatch typografia Pixel/Nothing)
  - Nordic Minimal (čistý masívny minimalistický čas)
- **7 štýlov sekundového prstenca (Seconds Styles):** Vypnuté, Bodka, Plynulý oblúk, Pulz, Radarový lúč (Sweep s fosforovým doznievaním), Švajčiarske indexy (Ticks), Satelit na orbite (Orbit).
- **Gesto dvojkliku (Double-Tap na 6-osové IMU QMI8658):** Prepínanie ciferníkov na obrazovke hodín a prepínanie čistého režimu (skrytie legiend) na radaroch.
- **Vojenské lety a automatické zameranie núdzových squawkov (7500, 7600, 7700):** Červená ikona stíhačky s delta krídlami a automatické zamknutie na lietadlo v núdzi so stavovou telemetriou.
- **Hardvér RTC čip (PCF85063):** Automatická synchronizácia s NTP, uchovanie času pri výpadku napájania/WiFi, podpora 3.3V superkondenzátora, diagnostika a web tlačidlá pre manuálnu synchronizáciu.
- **Granulárne prepínače widgetov vo webe:** Samostatné prepínanie dátumu, počasia, vetra, mesiaca, letových chvostov, vektora k najbližšiemu lietadlu a kružníc.
- **Nočný režim hodín (`nightClockOnly`):** Zastavenie rotácie obrazoviek v noci s trvalým zobrazením stlmených hodín.
- **I2C Bus Scanner vo webovom rozhraní:** Živá detekcia pripojených čipov na zbernici.

### Zmenené a Opravené / Changed & Fixed
- **FreeRTOS Dual-Core stabilizácia:**
  - Izolácia dotykového panelu CST820 výhradne na Jadro 1.
  - Ochrana I2C zbernice rekurzívnym mutexom s časovým limitom 150 ms.
  - Uvoľnenie mutexu radaru počas sieťových HTTPS prenosov (`fetchIndex` a `fetchOneTile`) na Jadre 0.
  - Časový limit 200 ms na všetkých dátových mutexoch zamedzujúci akémukoľvek watchdog reštartu.
  - Presná kalibrácia mbedTLS buffer guardu na 18 000 B podľa RFC špecifikácie.
- **Zarovnanie grafiky:** Odstránenie prekrytia teploty a hodín, vycentrované zobrazenie komplikácií vo všetkých ciferníkoch.
- **Kompletná trojjazyčná lokalizácia (SK, CS, EN):** Plná diakritika na displeji, v captive portáli, OTA rozhraní aj vo webovom dashboarde.

---

## [0.6.4]

### Opraveno

- **Radar letadel zůstával prázdný.** adsb.fi začalo odpovídat chunked a
  hexadecimální hlavičky bloků zůstávaly v těle, takže se JSON rozpadl, aniž by
  to kdokoli ohlásil. Týkalo se i tras z adsb.lol.
- **Meteoradar se přestal aktualizovat.** Výpis adresáře ČHMÚ přerostl
  vyhrazených 256 kB. Nově se prohledává průběžně a na jeho velikosti nezáleží.
- **Po neúspěchu se výpis ČHMÚ stahoval každou vteřinu**, u RainVieweru se
  naopak nezkusil znovu vůbec. Oba zdroje se teď zkusí po minutě.
- **Filtr odpovědi adsb.fi mohl vyjít prázdný** a zahodit tím všechna pole.
- **Letadlo na zemi se počítalo jako v nulové výšce** (`alt_baro` nese `ground`).
- **Zařízení se mohlo restartovat během TLS handshake.** Ten má vlastní limit
  120 s, tedy šestinásobek watchdogu; nově 8 s.
- **Stažení trasy mohlo restartovat zařízení**, protože se během něj
  neresetoval watchdog.
- **Poškozený snímek meteoradaru se mohl vykreslit**, nově se kontroluje PNG
  signatura.

### Změněno

- Všechna textová stahování vedou přes `Net_GetString()`, včetně polohy podle IP
  a venkovní teploty, které měly vlastní `HTTPClient`. Nově zvládne i `http://`.
- Textové odpovědi mají strop `NET_MAX_TEXT` a čtou se do PSRAM.
- Kontrola volné paměti před TLS je na jednom místě (`Net_HeapOk()`); do teď ji
  mělo pět souborů okopírovanou.
- Velké buffery se alokují výhradně v PSRAM.
- Kromě pole `ac` se přijímá i `aircraft` (servery odvozené z ADSBexchange).
- Sériový výpis říká velikost indexu ČHMÚ a důvod nečekané odpovědi z adsb.fi.

### Odebráno

- Jednosnímkové API meteoradaru (`CHMU_FetchLatest()` a spol.) — obrazovka jede
  na animaci a tuhle cestu nevolal nikdo.
- Třídy sinku z `NetSink.h` do `.cpp`; hlavička ze 139 řádků na 54.
- Prázdné soubory `OTA.cpp` a `OTA.h`, zbytek po ElegantOTA.
- `Watchdog_Suspend()` a `Watchdog_Resume()`, konstanty `OTA_IDLE_MS`,
  `PORTAL_TIMEOUT_S`, `RV_MAX_TILES` a několik nevolaných funkcí v `Layout`,
  `UI`, `WxIcon` a `Forecast`.

---

## [0.6.3]

### Opraveno

- **V detailu letadla se ukazovala trasa, která neodpovídala skutečnosti** —
  třeba Atény → Istanbul u letadla nad Prahou. Sešly se tři příčiny:
  letadlo bez callsignu poslalo do dotazu svou ICAO adresu (`a31234` se
  normalizuje na `A31234`, což je platné číslo letu Aegean Airlines 1234),
  zdroj tras neověřoval polohu, a u letů s mezipřistáním se ukazoval první
  odlet a poslední přílet, i když letadlo letělo prostřední úsek.
- **Hlavička `User-Agent` se neposílala vůbec.** `HTTPClient::addHeader()` ji
  mlčky zahazuje, takže odcházelo výchozí `ESP32HTTPClient`. Týkalo se to
  i dotazů na adsb.fi ve starších verzích.
- **Trasa se dokreslila až s dalším stažením letadel**, tedy podle dosahu
  o 5 až 15 sekund později, než dorazila. Nově se objeví hned.
- **Keš tras si odpověď pamatovala až do restartu.** Callsigny se recyklují
  a výsledek platí k poloze, se kterou se ptalo, takže po mezipřistání
  ukazoval pořád první úsek. Záznam teď platí 20 minut (1 minutu, když
  trasa nebyla nalezena).

### Změněno

- **Trasy nově z adsb.lol** místo adsbdb.com. Spolu s callsignem se posílá
  i poloha letadla a server vrátí, jestli k ní trasa vůbec sedí — co neprojde,
  se nezobrazí. U letů s mezipřistáním se vybere ten úsek, ke kterému je
  letadlo nejblíž. Letadlo, které callsign nevysílá, se na trasu neptá vůbec.
- **Registrace a typ letadla se berou z adsb.fi** (pole `r` a `t`) ze stejné
  odpovědi, která se stahuje kvůli polohám. Odpadl tím dotaz na druhé API.

---

## [0.6.2]

### Opraveno

- Automatické střídání se po prvním doteku zastavilo a už se nerozjelo. Pauza po
  gestu byla pevných deset minut — což dávalo smysl, dokud se interval zadával
  v minutách. Nově je to trojnásobek intervalu, nejméně 30 s a nejvýš 10 minut.

### Změněno

- Aktualizace firmwaru už nepoužívá knihovnu **ElegantOTA**. Byla pod AGPL-3.0,
  což by dělalo z každé přeložené binárky AGPL dílo, i když je zdroják pod MIT.
  Stránku `/update` teď obsluhuje projekt sám nad třídou `Update` z ESP32 core.
  Ovládá se stejně, jen umí i anglicky. **Projekt je nově celý MIT.**
- Změna hesla se projeví hned, bez restartu.

---

## [0.6.1]

### Opraveno

- Obrazovka s QR kódem se překreslila datovou obrazovkou — při prvním spuštění
  i po továrním resetu. Dokud běží přístupový bod, patří displej jemu.
- V detailu letadla se místo typu ukazovalo `adsb_icao` (typ zprávy místo typu
  letounu).
- WiFi šlo změnit jen od displeje. Karta WiFi je teď na webu i po připojení.
- Změna jazyka se neukládala na záložce Ovládání.
- Nabídka nalezených měst zůstávala viset, když další hledání nic nenašlo.

### Změněno

- Automatické střídání se nastavuje v sekundách, ne v minutách (0 až 3600).
  Hodnota z dřívějších verzí se jednorázově převede.
- Většina nastavení se ukládá hned po změně. Tlačítko Uložit zůstává jen pro
  polohu, výběr obrazovek a zdroj radaru — ty vyžadují restart.
- Klepnutí už nepozastaví střídání obrazovek. Pozastaví ho jen swipe, dlouhý
  stisk nebo zásah z prohlížeče; otevřený detail letadla ho drží.

---

## [0.6.0]

Konfigurace se přesunula do prohlížeče, přibyly dvě obrazovky a meteoradar
funguje i mimo ČR.

### Přidáno

- **Webové nastavení** na `http://meteoplaneradar.local/`, dostupné trvale.
  Poloha (i vyhledáním města), obrazovky, zdroj radaru, jazyk, jas, filtry
  letadel, stavová stránka, export/import nastavení, heslo.
- **Přístupový bod vydrží, dokud nezadáte WiFi.** Vlastní captive portál,
  česky i anglicky.
- **Obrazovka hodin** — čas, datum, počasí, vteřinový prstenec. Bez Home
  Assistantu.
- **Obrazovka předpovědi** — 6 hodin a 3 dny z Open-Meteo, kvalita ovzduší a pyl.
- **RainViewer** jako alternativa k ČHMÚ — meteoradar funguje i v zahraničí.
- **Volitelné obrazovky** a jejich automatické střídání.
- **Noční režim** podle východu a západu slunce, denní a noční jas zvlášť.
- **Angličtina** na displeji, webu i v portálu.
- **Nouzové squawky** 7500/7600/7700, filtry letadel a sledovaný callsign.
- **Dálkové ovládání z webu** — přepínání obrazovek a změna rozsahu bez
  dotyku displeje.
- **Sdílený layout** — obrazovky si rezervují místo dřív, než se kreslí mapa,
  takže se popisky měst ani callsigny nepřekrývají.

### Změněno

- **OTA je na `/update`** běžné adresy zařízení. Zvláštní přístupový bod zmizel.
- **WiFiManager odstraněn** — nešel přeložit a blokoval hlavní smyčku.
- **Obrazovka Nastavení je kratší**, zbytek je v prohlížeči.
- **Dotyk reaguje i během stahování.**

### Poznámky k aktualizaci

- Nastavení z 0.5.5 zůstává, **uloženou WiFi je nutné zadat znovu** (dřív ji
  držel WiFiManager).
- Rozdělení paměti se nemění, OTA z 0.5.x funguje.
- Číslování se rozchází s forkem ok1cdj — stejné číslo neznamená stejný firmware.

---

## [0.5.5]

### Přidáno
- **Česká města mají vlastní seznam.** Evropská data obsahují jen sídla nad
  50 000 obyvatel a zkratky si generují strojově prvními čtyřmi písmeny, takže
  z Prahy byla `PRAH` a okresní města jako Znojmo úplně vypadla. Nový
  `CzCitiesData.h` proto pro Českou republiku evropská data **nahrazuje** —
  všech 59 měst i s původními zkratkami (`PHA`, `OVA`, `PLZ`) a při malých
  rozsazích plnými názvy. Dlouhá jména jsou zkrácená: `Jablonec n. N.`,
  `Zdar n. S.`, `Usti n. L.`
  Města z evropské sady, která leží v Česku, se přeskakují — párování je podle
  **souřadnic**, ne podle názvu, protože ten se v obou sadách píše jinak.
  Ověřeno proti datům: všech 21 překryvů se spáruje jedna ku jedné, nic se
  nekreslí dvakrát a Drážďany ani Žilina, které padnou do stejného obdélníku,
  o evropský záznam nepřijdou.
- **Popisky měst se už nepřekrývají.** Při rozsahu 200 km a u pohledu na celou
  republiku se jména psala přes sebe. Každý vykreslený popisek si nově zabírá
  místo a ten, který by do něj zasáhl, se zahodí celý — půlka jména pod jiným
  jménem je horší než žádné jméno. Města se kreslí po skupinách od největších,
  takže o místo přijde vždy ta menší obec vedle. Ověřeno: u pohledu na celou ČR
  53 popisků a **nula překryvů**.
- **Meteoradar kreslí stejnou evropskou mapu jako radar letadel.** Dosud měl
  vlastní obrys ČR s padesáti body; nově používá `EuMapData` s 30 894 body
  hranic a 1 100 městy, tedy stejná data i stejnou podrobnost jako letadla.
  Na rozsahu „cela CR“ jsou díky tomu vidět i obrysy sousedních států, takže
  republika nevisí v prázdnu. Soubory `CzBorder.*` a `CzBorderData.h` odpadly.
- **Čas a venkovní teplota** na jednom řádku pod tečkami výběru obrazovky, na
  radaru letadel i na meteoradaru. Dokud se čas nenačte, zůstává řádek prázdný.
  Počet letadel je nově menším písmem a na meteoradaru se indikátor snímku
  posunul níž, aby se řádek vešel.
- **Hodiny bez NTP.** Čas se bere z hlavičky `Date` v HTTP odpovědích, které
  stahujeme tak jako tak. Neprochází portem 123, takže ho poskytovatel
  internetu nemá jak zablokovat, nečeká se při startu a nepřibylo žádné
  spojení navíc.
- **Rozsah „cela CR“ na meteoradaru.** Pátý rozsah za 200 km ukazuje celou
  republiku nezávisle na tom, kde uživatel je.
- **Odkud a kam letadlo letí.** V detailu letadla přibyly dva řádky `Z:`
  a `Do:` s výchozím a cílovým městem z adsbdb.com, k tomu registrace a typ
  letounu. Rozdělení na dva řádky bylo kvůli velikosti písma — jedna řádka se
  šipkou mezi městy se musela zmenšit tak, že byla špatně čitelná. Takhle má
  každý řádek celou šířku panelu a u většiny tras vyjde největší velikost.
  Ptáme se jen na vybrané letadlo, nikdy na celý seznam, a odpověď se pamatuje.
  Řada letů trasu v databázi nemá a pak se nezobrazí nic.

### Opraveno
- **Meteoradar při načítání nezčerná.** Dosud se každých pět minut na několik
  sekund objevila černá obrazovka s nápisem „Nacitam animaci...“. Nově zůstane
  poslední platná animace na displeji a pod indikátorem snímku se objeví jen
  malá žlutá poznámka. Když stahování selže — výpadek spojení, nedostupný
  server — snímky se **nezahodí**, zůstane starší sada a v poznámce se objeví
  `bez spojeni, zobrazena starsi data`.
- **V detailu letadla chyběl popisek `Typ:`.** Přidán zpátky. Zobrazuje se
  krátký kód typu (`A320`, `B738`); adsb.fi ho hlásí jen někdy, takže když
  chybí, doplní se z adsbdb (pole `icao_type`).
- **Názvy měst s diakritikou se vykreslovaly jako změť znaků.** adsbdb vrací
  jména v Unicode — turecký İzmir se posílá jako `İzmir`, což je v UTF-8 dva
  bajty, a sedmibitový font z nich udělal dva náhodné znaky. Na displeji pak
  bylo něco jako `-?zmir`. Nově se text před vykreslením převede na ASCII:
  diakritika se zahodí, písmeno pod ní zůstane. Ověřeno na `İzmir`, `Malmö`,
  `Kraków`, `Košice`, `Gdańsk`, `Şanlıurfa` i `Tromsø`.
- **Teplota se nezobrazovala.** První pokus o stažení se počítal i tehdy, když
  ještě neběžela WiFi, a další byl pak na řadě až za deset minut. Nově se čeká
  na připojení a dokud teplota není známá, zkouší se každou minutu. Odpověď se
  navíc čte celá najednou, ne po proudu, aby ji nemohl utnout chunked přenos.
- **U velkých rozsahů skákala poloha do nesmyslů.** Výřez z obrázku ČHMÚ se
  ořezával na okraje snímku, takže přestal být vystředěný na uživatele — ale
  křížek se pořád kreslil doprostřed displeje. U Chebu na 200 km to dělalo přes
  80 km stranou, u Ostravy zhruba 10 km; z Prahy se chyba neprojevila, protože
  tam výřez na okraj nedosáhne. Nově se výřez neořezává, chybějící část se
  vykreslí černě a křížek se kreslí na skutečnou polohu.
- **Dotykový řadič se čte, jen když má co říct.** CST820 hlásí připravená data
  signálem INT, který jsme dosud vůbec nesledovali — četlo se každých pár
  milisekund bez ohledu na stav čipu. Odtud pocházela ta „vadná čtení“ a samé
  `0xFF`, které jsme od 0.5.1 zahazovali. Nově se čte na přerušení, během
  doteku průběžně, plus záchranné čtení jednou za `TOUCH_IDLE_POLL_MS`.
  Neúspěšné čtení se už nepočítá jako chyba — spící čip prostě neodpovídá.
  Vypíná se přepínačem `TOUCH_USE_INT`.

### Odebráno
- **Automatická obnova dotykového řadiče** včetně počítání vadných vzorků.
  Po opravě výše nemá co řešit, a jejím smazáním zmizelo jediné místo, které
  za běhu zapisovalo na obvod držící napájení displeje.
- Nepoužívané snímkové API ČHMÚ, `LCD_DrawBitmap` a `TCA9554_ReadOutput`.

### Změněno
- **Nové zdroje dat** — adsbdb.com (trasa letu, registrace, typ) a Open-Meteo
  (venkovní teplota), obojí zdarma a bez registrace. Do přehledu zdrojů je
  doplněna i mapa: hranice z Natural Earth (public domain) a města z GeoNames
  (CC BY 4.0), což je licence vyžadující uvedení autora — dosud chybělo.
- **Přepínání jednotek se přesunulo do Nastavení.** Je to předvolba jako každá
  jiná, ne něco, co se mění u konkrétního letadla — a v detailu letadla se tím
  uvolnilo místo pro trasu.
- **Trasa v detailu letadla je stejně velká a stejnou barvou jako letecké
  údaje nad ní.** Je to informace stejného druhu a odlišná barva naznačovala
  rozdíl, který tam není. Zmenší se jen tehdy, když se delší z obou řádků
  nevejde do panelu; případný přesah se ořízne a zakončí tečkou. Typ letounu
  a registrace nově sdílejí jeden řádek, aby na trasu zbyly dva.
- Výřezy snímků se alokují rovnou na největší rozsah, takže přepnutí rozsahu
  už nikdy nealokuje a nemá jak narazit na fragmentovanou PSRAM.

## [0.5.4]

### Opraveno
- **Displeji po chvíli zčernal obraz a pomohl jen restart.** Podsvícení přitom
  svítilo dál a deska běžela normálně. Na vině byla automatická obnova
  dotykového řadiče přidaná v 0.5.3: za zaseknutý řadič považovala i to, když
  čip jen usne a přestane odpovídat, což je jeho běžné chování. Resetovala ho
  proto pořád dokola — a protože reset vede přes obvod, který drží i napájení
  displeje, dřív nebo později jeden zápis skončil špatně a panel zhasl.
  Obnova je nově vypnutá (`TOUCH_RECOVERY 0` v `Config.h`).

### Přidáno
- **Hlídač displeje.** Sleduje, jestli panel opravdu vykresluje snímky. Když se
  zastaví, zkusí ho oživit, a pokud se to nepovede, deska se sama restartuje —
  místo aby zůstala černá až do odpojení napájení.
- **Hlídač I/O expandéru.** Průběžně kontroluje, že obvod držící napájení
  a reset displeje má nastavené to, co má, a případný rozdíl opraví.

## [0.5.3]

### Opraveno
- **Gesto už nekončí na prvním prázdném vzorku.** CST820 běžně vynechá vzorek
  uprostřed tahu a od verze 0.5.1 se navíc zahazují vadná čtení — obojí se
  tvářilo jako zvednutí prstu. Jeden swipe se tak rozpadl na několik krátkých
  klepnutí, která pak zavírala detail letadla nebo klepala do mapy. Nově se
  vyžaduje `TOUCH_RELEASE_MS` (60 ms) souvislého ticha. Ověřeno simulací:
  swipe s výpadkem 20 ms dřív dal *dvě klepnutí a žádný swipe*, teď jeden swipe.
- **Čas u snímků meteoradaru byl v létě o hodinu vedle.** Posun UTC → místní čas
  se počítal z *aktuálních* hodin. Dokud nedorazila odpověď z NTP, ležely
  v roce 1970, tedy v lednu, takže se použil CET místo CEST. Nově se posun
  odvozuje z data *toho snímku* (název souboru nese `YYYYMMDDHHMM` v UTC), takže
  vychází správně nezávisle na hodinách — včetně nocí, kdy se přechází mezi
  letním a zimním časem. Popisek „nyni / −X min“ se počítá z pořadí snímku
  a nebyl ovlivněn nikdy.
- **Jas se zapisoval do flash při každém pohybu slideru.** Přetažení přes celou
  šířku znamenalo desítky zápisů do NVS. Nově prochází stejným odloženým
  zápisem (~2 s klidu) jako rozsah a orientace.
- **Bílý záblesk při startu.** Podsvícení se rozsvěcelo dřív, než byl vykreslen
  první snímek, takže bylo vidět náhodný obsah paměti panelu. Rozsvítí se až za
  prvním `flush()`.

### Přidáno
- **Zotavení dotykového řadiče.** Po `TOUCH_REINIT_BAD` (40) vadných čteních za
  sebou se CST820 resetuje. Dřív po zámrzu I2C přestal dotyk fungovat až do
  odpojení napájení.
- **Kontrola volné paměti před TLS.** Handshake potřebuje ~45 kB *interní* RAM;
  když chybí, mbedTLS to hlásí jen jako `HTTP -1`. Nově se poll přeskočí
  a důvod se vypíše (`NET_MIN_HEAP`).
- **Ošetření selhání displeje.** `ST7701_Init()` vrací `bool` a kontroluje
  návratové kódy SPI i `esp_lcd_new_rgb_panel` — nejčastější příčina (PSRAM
  není v IDE nastavená na OPI) se teď vypíše místo černé obrazovky bez stopy.
  `LCD_Flush()` a `LCD_DrawBitmap()` navíc nesáhnou na neinicializovaný panel.
- **Důvod restartu a volná paměť v sériovém výpisu** při startu — panic,
  watchdog a brownout jsou v hlášení chyb rozlišitelné na první pohled.
- Timeout konfiguračního portálu je nově v `Config.h` (`PORTAL_TIMEOUT_S`).

### Odebráno
- **NTP klient a systémový čas.** Po opravě časů výše je nepotřebuje nic:
  popisek HH:MM se odvozuje z názvu snímku, „nyni / −X min“ z jeho pořadí
  v animaci a všechna HTTPS spojení jedou přes `setInsecure()`, takže se
  neověřuje ani platnost certifikátů. Zmizelo čekání při startu i varování
  „NTP neodpovedel“. Kdyby na displeji někdy přibyly hodiny, vrátí se
  `configTzTime()` zpátky nad `setup()`.
- Konstanty `NTP_SERVER`, `NTP_WAIT_MS` a `NTP_RETRY_MS` z `Config.h`.

### Změněno
- **Časová zóna se nastavuje explicitně** (`setenv("TZ", ...)` + `tzset()`
  v `setup()`). Dřív to byl vedlejší efekt `configTzTime()`; bez něj by
  `localtime_r()` v `CHMU.cpp` tiše vracelo UTC a popisky by byly o hodinu
  nebo dvě vedle. `TZ_INFO` v `Config.h` proto zůstává.

## [0.5.2]

### Změněno
- **Otočení mapy se nastavuje srozumitelněji.** Řádek v Nastavení se jmenuje
  `Nahore` a udává, **který světový směr je nahoře na displeji** — tedy směr,
  kterým se díváte z okna. Dřív se zadávalo „o kolik mapu otočit“, což je něco
  jiného: pro výhled na východ bylo potřeba nastavit 270°, a při špatné hodnotě
  mapa působila zrcadlově. Nově se zadává rovnou `V`.
  Hodnota se zobrazuje jako zkratka světové strany (`S`, `SV`, `V`, `JV`, `J`,
  `JZ`, `Z`, `SZ`), tlačítko `+` jde po směru hodinových ručiček.
  Uloženo je pod novým klíčem v NVS, takže **stará hodnota se po aktualizaci
  nepřenese** a orientace začíná na severu nahoře — nastavte si ji prosím
  znovu (jedno klepnutí).

## [0.5.1]

### Opraveno
- **Detail letadla se zavíral sám od sebe.** Skutečnou příčinou nebyla data
  z adsb.fi, ale **vadná čtení z dotykového řadiče**. Když I2C přenos selže na
  úrovni dat, CST820 vrátí samé `0xFF` — a to se dekódovalo jako „15 bodů na
  souřadnicích (4095, 4095)“, tedy jako platné klepnutí mimo panel, které
  detail zavřelo. Ve výpisu uživatele tomu odpovídá **každé** samovolné
  zavření. Nově se surová data ověřují: zahodí se samé `0xFF`, nesmyslný počet
  bodů (CST820 je jednodotykový) i souřadnice mimo displej. Počet zahozených
  čtení se při zapnutém `TOUCH_DEBUG` vypisuje jednou za sekundu.

## [0.5]

### Přidáno
- **Otočení mapy** — v Nastavení přibyl řádek `Otoceni` s tlačítky `−` / `+`,
  krok **45°** (osm poloh). Slouží k tomu, aby letadlo viděné z okna bylo na
  displeji ve stejném směru. Otáčí se **projekce**, ne displej, takže se spolu
  s letadly správně otočí i obrys států, města a ikony letadel.
  Vedle ovládání je **kompasový náhled** (kroužek s ryskou a písmenem S), takže
  je nastavení vidět hned bez přepínání na radar.
  **Meteoradar se záměrně neotáčí** — srážková mapa se čte severem nahoru a
  orientaci v ní drží obrys ČR.
  Tlačítko `+` otáčí mapou **po směru hodinových ručiček** (sever putuje nahoru
  → vpravo nahoru → vpravo), `−` opačně.
  Krok se dá změnit přes `MAP_ROT_STEP_DEG` v `Config.h` — musí ale dělit 90,
  jinak přestanou být dosažitelné přesný východ a západ.
- **Značky světových stran** (S, V, J, Z) po obvodu radaru letadel; otáčejí se
  spolu s mapou.

### Změněno
- **Uživatelské rozhraní je celé česky** (bez diakritiky — vestavěný font umí
  jen ASCII). Například „Letadel: 12“, „Nastaveni“, „Jas“, „Poloha“,
  „Aktualizace FW“, v detailu letadla „Vyska“, „Rychlost“, „Kurz“, „Stoupani“.
- Rozvržení obrazovky Nastavení zhuštěno, aby se vešel řádek s otočením.

### Opraveno
- **Trhající se pás uprostřed displeje.** Měření ukázalo, že kopie celého
  snímku do framebufferu panelu trvala **28 ms**, zatímco jeden snímek trvá
  34 ms — zápis a vykreslování se tedy pohybovaly skoro stejnou rychlostí a
  někde uprostřed obrazovky se předjely. Nově má panel **dva framebuffery** a
  kreslí se rovnou do toho, který zrovna není vidět (`num_fbs = 2`, bounce
  buffery pryč). Driver pozná svůj vlastní buffer a místo kopírování jen
  přepne DMA, takže se ta 28ms kopie neprovádí vůbec a zátěž sběrnice PSRAM
  výrazně klesne.
- **Detail letadla se už nezavírá sám.** adsb.fi občas letadlo v jednom stažení
  vynechá a v dalším ho zase pošle; dřív stačil jeden takový výpadek a panel se
  zavřel. Nově se tolerují **dvě po sobě jdoucí chybějící stažení**
  (`DETAIL_GRACE_POLLS` v `Config.h`), během nichž panel zůstane otevřený
  s posledními známými hodnotami a poznámkou „signal ztracen“.

### Diagnostika
- Volitelné **měření doby překreslení** (`FLUSH_DEBUG` v `Config.h`) — jednou
  za sekundu vypíše min/poslední/max dobu jednoho flushe. Slouží k ověření,
  jestli se stíhá překreslit do jednoho snímku.
- Volitelné **ladicí výpisy dotyku** (`TOUCH_DEBUG` v `Config.h`, výchozí
  zapnuto). Do sériové linky se vypisuje každé dokončené gesto i každá změna
  výběru letadla **včetně důvodu**, proč se detail zavřel (`tap mimo panel`,
  `letadlo zmizelo z dat`, `dlouhy stisk`). Podle toho jde odlišit falešný
  dotyk od výpadku dat.

## [0.4]

> ### ⚠️ Upozornění k aktualizaci na 0.4
> Verze 0.4 mění **rozdělení paměti** (dvě aplikační oblasti, aby bylo kam
> nahrát bezdrátovou aktualizaci). Proto se na ni **z verze 0.3 a nižší nedá
> přejít přes OTA** — je nutné jednou nahrát soubor `*.merged.bin` přes
> [esp32flasher.chiptron.cz](https://esp32flasher.chiptron.cz) a USB kabel.
> Od 0.4 dál už aktualizace probíhá bezdrátově.

### Přidáno
- **OTA aktualizace firmware přes WiFi** (ElegantOTA). V Nastavení přibylo
  tlačítko „Firmware update“: deska vytvoří AP `MeteoPlaneRadar`, na displeji
  ukáže QR kód a firmware se nahraje z prohlížeče na `192.168.4.1/update`.
  Vyžaduje OTA rozdělení flash (`src/partitions.csv`, dva app sloty).
- **Zapamatování stavu UI** — poslední rozsah (zvlášť pro letadla a meteoradar)
  a naposledy zobrazená obrazovka se ukládají do NVS a obnoví se po restartu.
  Zápis je odložený (~2 s po poslední změně), aby swipování nezatěžovalo flash.
- **Zobrazení verze firmwaru** na obrazovce Nastavení (pod titulkem), na OTA
  obrazovce a v sériovém výpisu. Nová sdílená hlavička `MeteoPlaneRadar/Version.h`.
- **Sjednocení nastavení** do `MeteoPlaneRadar/Config.h` — časová zóna, výchozí poloha,
  rozsahy, intervaly stahování, název AP a limity na jednom místě.
- **CI build na GitHubu** — každý push se automaticky zkusí přeložit.
- Tento `CHANGELOG.md`, `.gitignore`, `sketch.yaml` a `LICENSE` v kořeni.

### Změněno
- Během OTA se na displeji ukáže jen „Probiha aktualizace…“ a **podsvícení se
  vypne** po dobu zápisu. Průběh v procentech se nevykresluje: RGB panel čte
  obraz z PSRAM průběžně a zápis do flash mu data odřezává, takže by obraz
  poskakoval. Procenta jsou vidět v prohlížeči.
- Historie verzí se přesunula z hlavičky `.ino` sem.

### Opraveno
- Meteoradar se při každém vstupu na obrazovku zbytečně znovu dekódoval
  (všech 6 PNG). Nově se přepočítá jen při skutečné změně rozsahu.

## [0.3]

### Změněno
- **Robustní stahování ADS-B.** Celé HTTP tělo se načte do znovupoužitelného
  PSRAM bufferu a parsuje se až kompletní (kontrola utnutí proti
  `Content-Length` + jeden retry), místo parsování přímo z TLS streamu. Tím
  zmizely občasné chyby stahování „IncompleteInput“.
- Parsování používá **ArduinoJson filtr** (nechá jen pole, která se používají),
  takže dokument zůstává malý bez ohledu na objem dat.
- Pozemní letadla se zahazují už při parsování.
- **Perioda stahování podle rozsahu** (5 / 10 / 15 s) a po neúspěšném stažení
  dvojnásobek, aby se šetřilo bezplatné API adsb.fi.
- Limit letadel `ADSB_MAX` zvýšen ze 40 na **100**.
- **Ovládání:** dlouhý stisk přepíná obrazovky směrově (levá půlka =
  předchozí, pravá = následující, s přetočením dokola) místo slepého cyklení.

### Opraveno
- Při chybě stahování zůstane poslední platný snímek — radar už nebliká na
  prázdno.

## [0.2]

### Přidáno
- První veřejná verze: radar letadel (adsb.fi) + animovaný srážkový meteoradar
  ČHMÚ na kulatém displeji 480×480.
- Oprava problikávání pixelů uprostřed displeje: jedno plátno v PSRAM a jediný
  přenos snímku synchronizovaný s VSYNC (`num_fbs=1` + bounce buffery,
  pixel clock 8 MHz).
