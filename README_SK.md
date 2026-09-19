# MeteoPlaneRadar (Taktický a meteorologický radar pre ESP32-S3)

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz%20Dual--Core-red.svg)
![Display](https://img.shields.io/badge/Display-Round%202.1%22%20480x480%20IPS-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange.svg)
![Languages](https://img.shields.io/badge/Languages-SK%20%7C%20CZ%20%7C%20EN-green.svg)
![Release](https://img.shields.io/badge/Release-v2.0.2-brightgreen.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

**Multifunkčná meteorologická stanica, živý letecký ADS-B radar, animovaný radar zrážok (SHMÚ, ČHMÚ, RainViewer), kombinovaný taktický radar, sledovanie stanice ISS, analytika YouTube kanála, finančné trhy, dizajnové ciferníky a animovaný pixel art virtuálny spoločník na okrúhlom 2.1" IPS dotykovom displeji.**  
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

### 🐱 Animovaný Pixel Art spoločník DigiCat

<p align="center">
  <img src="docs/media/digicat_walk.gif" width="12%" alt="DigiCat Chôdza" />
  <img src="docs/media/digicat_jump.gif" width="12%" alt="DigiCat Skok" />
  <img src="docs/media/digicat_groom.gif" width="12%" alt="DigiCat Hygiena" />
  <img src="docs/media/digicat_stretch.gif" width="12%" alt="DigiCat Strečing" />
  <img src="docs/media/digicat_happy.gif" width="12%" alt="DigiCat Pradenie" />
  <img src="docs/media/digicat_eat.gif" width="12%" alt="DigiCat Kŕmenie" />
  <img src="docs/media/digicat_sleep.gif" width="12%" alt="DigiCat Spánok" />
</p>

<p align="center">
  <em>Ručne tvorený 32-snímkový pixel art DigiCat: <b>Chôdza</b>, <b>Výskok s oblúkom</b>, <b>Umývanie uška a líčka</b>, <b>Joga strečing</b>, <b>Pradenie a červenanie</b>, <b>Chrumkanie rybičky</b>, <b>Spiaci bochníček</b>.</em>
</p>

---

## 🌟 Implementované funkcie

### 🕒 1. Kolekcia 6 unikátnych ciferníkov hodín
Okrúhly 480×480 displej ponúka **6 odlišných geometrických štýlov ciferníka** s plynulým prepínaním zvislým potiahnutím prsta (Swipe Hore / Dole):
1. **Vertikálna typografia (Stacked Bold)** – moderný smartwatch dizajn (Pixel / Nothing štýl) s obrovským dvojčíslom hodín `HH` a minút `MM` doplneným zaoblenými kapsulami pre počasie, mesiac, dátum a vietor.
2. **Letecký kokpitový analóg (Aviator Cockpit)** – autentické pilotné hodinky s luminiscenčnými ručičkami, hodinovými indexmi a dvoma sub-ciferníkmi (počasie na 9. hodine, fáza mesiaca na 3. hodine).
3. **🚀 Planetárne prstence (Orbital Gauges)** – sci-fi dizajn tvorený tromi sústrednými kruhovými oblúkmi (minúty, hodiny, sekundy) so svetelnými perlami a centrálnym telemetrickým jadrom.
4. **⏱️ Astronomický regulátor (Régulateur Chrono)** – mechanický chronometer s oddelenými osami: hlavná minútová ručička, horný sub-ciferník hodín (na 12:00) a spodný sub-ciferník sekúnd (na 6:00).
5. **Nordic minimalistický (Minimal)** – masívny, vysoko kontrastný čas výborne čitateľný aj z veľkej vzdialenosti.
6. **Digitálny klasický (Classic Digital)** – čistý horizontálny čas `HH:MM`, dátum, počasie, 3-hodinová minipredpoveď, vietor a fáza mesiaca.

- **Solárny oblúk so slnečnými lúčmi (Solar Arc):** Realistický 24-hodinový astronomický oblúk dráhy slnka so zlatým lúčovým symbolom žiariaceho slnka, nočnou modrou fázou a presnými míľnikmi východu a západu slnka.

### ⏱️ 2. Zobrazenie sekúnd na okrúhlom obvode
Až **7 štýlov sekundového prstenca**:
- `Vypnuté`, `Bodka`, `Plynulý oblúk`, `Pulz`, `Radarový lúč (Sweep)`, `Hodinárske indexy (Ticks)`, `Satelit na orbite (Orbit)`.

### 🛩️ 3. Pokročilé sledovanie letov & Inteligentný Alert HUD
- **360° radar vzdušného priestoru:** Živá letecká premávka v okruhu 14–200 km cez adsb.fi / adsb.lol.
- **Špeciálne kategórie letov:** Záchranári (zelená), vládne lety (zlatá), obrie lietadlá (azúrová) a vojenské lety (červená) sú automaticky identifikované a zvýraznené pulzujúcim kruhom.
- **Notifikačný Alert štítok:** Okamžité textové upozornenie pri výskyte špeciálneho letu v dosahu (napr. `! Zachranny vrtulnik: ATE02 (18 km) !`).
- **Núdzové lety (Squawk 7500, 7600, 7700):** Automatické uzamknutie a sledovanie lietadla v núdzi so stmavením okolitých letov a živou telemetriou (výška, rýchlosť, klesanie/stúpanie).
- **Vektor k najbližšiemu lietadlu:** Dynamická čiara ukazujúca smer, vzdialenosť a prevýšenie k najbližšiemu lietadlu.
- **Letiská & Databáza trás:** Zobrazenie letísk v okolí a offline dekódovanie letových trás z volacích znakov (napr. `Burgas -> Warsaw [BOJ>WAW]`).

### 🔍 4. Detailná karta lietadla so živými fotografiami
- Po kliknutí na lietadlo na radare sa zobrazí kompletná farebná telemetrická karta.
- Živé fotografie konkrétneho lietadla sťahované na požiadanie z **Planespotters.net API**.
- Ťuknutím na fotku sa obrázok roztiahne na celú okrúhlu obrazovku 480×480 px.

### 🌧️ 5. Animovaný radar zrážok (SHMÚ, ČHMÚ, RainViewer)
- Slučka radarových kompozitov s vysokým rozlíšením a plynulým prechodom (cross-dissolve) medzi snímkami.
- 256 kB vyrovnávacia pamäť v PSRAM pre každý kompozit zaručujúca plynulosť aj počas silných búrkových frontov.
- Voliteľné bilineárne vyhladzovanie (anti-aliasing) pre organický a prirodzený vzhľad zrážkových jadier.

### ⚡ 6. Nowcasting blížiaceho sa dažďa (TREC) & Detekcia typu zrážok
- **2D vektorový nowcasting:** Priestorová krížová korelácia sledujúca smer a rýchlosť postupu zrážkových polí. Upozorňuje výhradne vtedy, keď zrážky reálne smerujú na vašu polohu ($v_{radial} > 0$, odchýlka $\le 6\text{ km}$, $\text{ETA} \le 35\text{ min}$) s filtráciou zemských odrazov a virgy.
- **Dynamická klasifikácia zrážok:** Automaticky rozlišuje **Dážď**, **Dážď so snehom** ($1^\circ\text{C}\dots3^\circ\text{C}$), **Sneh** ($\le 1^\circ\text{C}$) alebo nebezpečné **Krupobitie** ($>50\text{ dBZ}$) kombináciou radarovej odrazivosti a lokálnej teploty.
- **Výstražný widget na hodinách:** Indikačná kapsula na ciferníku s možnosťou prekliku priamo do radaru.

### 🛰️ 7. Kombinovaný taktický radar (`ScreenTactical`)
- Unikátna fúzia dát: **animovaný radar zrážok na pozadí** a **živá ADS-B letecká premávka v popredí** na jednej spoločnej obrazovke.

### 🌤️ 8. Predpoveď počasia na 3 dni & Kvalita ovzdušia
- Hodinové krivky teploty, pravdepodobnosti zrážok a vetra cez Open-Meteo.
- 3-dňový prehľad počasia, Index kvality ovzdušia (AQI), prachové častice PM2.5 a peľové spravodajstvo.

### 📈 9. Finančné trhy a kryptomeny (`ScreenFinance`)
- Živé trhové dáta cez Yahoo Finance v8 API.
- **4 voliteľné pozície**: ETF fondy (MSCI World, S&P 500), indexy, komodity (Zlato, Ropa), akcie, kryptomeny a menové páry Forex.
- **On-Device výber aktív**: Výber sledovaných inštrumentov priamo na displeji z ponuky 34 populárnych predvolieb v 6 kategóriách.
- **Prepínanie štýlu grafu**: Možnosť voľby medzi čiarovou krivkou (Line) a sviečkovým grafom (Candlestick).
- Ťuknutím na riadok prepnete detail, dvojitým poklepaním okamžite obnovíte dáta.

### 🛰️ 10. Sledovanie dráhy stanice ISS (`ScreenIss`)
- **Astronomická mapa sveta Deň/Noc**: Ekvidistantná mapa sveta 320×160 so slnečným terminátorom zobrazujúcim reálny tieň Zeme.
- **Trajektória letu**: Minulá orbita (45 min, čiarkovaná) a budúci vypočítaný prelet (92 min, plná jantárová krivka).
- **Rádius viditeľnosti**: Zobrazenie kruhu priamej viditeľnosti stanice (~2 200 km) voči pozorovateľovi.
- **Telemetrický panel**: Živá výška, rýchlosť, vzdialenosť, odpočet do najbližšieho preletu a maximálna elevačná výška.
- **Akustické echo**: Sonarový ping bzučiaka (`BEEP_SONAR_PING`) pri vstupe stanice do zóny viditeľnosti.

### ▶️ 11. Analytika YouTube kanála (`ScreenYouTube`)
- **Živé štatistiky kanála**: Aktuálny počet odberateľov (Subscribers), celkový počet zhliadnutí (Total Views) a metriky najnovšieho videa cez oficiálne YouTube Data API v3.
- **Výrazná typografia**: Počet odberateľov zobrazený masívnym vyhladeným fontom (`FONT_HERO`, 32 px).
- **Dve informačné karty**: Spodné karty zobrazujúce celkové pozretia (azúrová) s počtom videí a posledné video (zlatá) so štítkom NEW a zalamovaným názvom.
- **Okamžité a periodické obnovovanie**: Obnovenie dát ťuknutím na displej so zvukovou odozvou; periodická aktualizácia šetrí kvótu API.

### 📊 12. Paralelné štatistiky letov pre všetky rozsahy (`ScreenInfo`)
- Sleduje 24-hodinovú letovú aktivitu nezávisle pre **6 rozsahov priblíženia** (Všetko, $\le 10\text{ km}$, $\le 25\text{ km}$, $\le 50\text{ km}$, $\le 100\text{ km}$, $\le 200\text{ km}$) počítaných paralelne v pamäti PSRAM.
- Ťuknutím na hlavičku karty prepínate rozsah **bez akejkoľvek straty alebo resetu denných dát**.
- Počet unikátnych lietadiel, rýchlostný rekord, najvzdialenejšie lietadlo, letové hladiny (FL min/max) a počet prijatých správ.
- Automatický reset o polnoci alebo manuálne cez tlačidlo na displeji / webovom rozhraní.

### 🔊 13. Akustický výstražný systém s bzučiakom
- Integrovaný aktívny bzučiak pre núdzové hlásenia (Squawk 7700/7600/7500), prelety sledovaných lietadiel, búrkové výstrahy, sonarové echo stanice ISS, hodinové odbíjanie, odozvu na dotyk a nočné stíšenie.

### 🌙 14. Hlboký nočný režim & Šetrenie zraku
- **Ultra nočný režim (Deep-Red):** Monochromatický tmavočervený nočný režim s minimálnym jasom podsvietenia (0,5%).
- **Režim nočných hodín (`nightClockOnly`):** Počas nočných hodín automaticky zastaví striedanie obrazoviek a ponechá displej stmavený výhradne na ciferníku hodín.

### 📱 15. Rýchle ovládacie centrum & Dotykové gestá
- Sťahovacie menu potiahnutím z horného okraja pre bleskové nastavenie jasu, nočného režimu, bzučiaka, popiskov a striedania obrazoviek.
- Intuitívne dotykové gestá smartfónu: vodorovné potiahnutie mení obrazovky, zvislé potiahnutie mení štýl hodín alebo zoom radaru.

### 🕒 16. Hardvérové RTC hodiny (PCF85063) & Nezávislosť
- Automatická synchronizácia času cez sieťové NTP pri štarte so záložným čipom PCF85063 na zbernici I2C (`0x51`).
- Možnosť napájkovania **3.3V 1.0F–1.5F superkondenzátora** na plôšky `BAT` a `GND` pre udržanie presného času aj pri výpadku napájania bez batérie.

### 🖥️ 17. Stabilný ST7701 ovládač displeja (Zero-Drift)
- Kalibrovaný 8 MHz RGB pixel clock s továrenským časovaním (`HBP 10`, `HFP 50`, `VPW 8`, `VBP 8`, `VFP 8`), 4 MHz SPI inicializácia a zapnutie panela až po stabilizácii RGB signálu.
- **Trvalá eliminácia posunu obrazu**: Odstránené zápisy do SPI Flash pamäte pri automatickom striedaní obrazoviek, vďaka čomu nedochádza k pozastaveniu zbernice PSRAM a podtečeniu GDMA FIFO.
- Automatická resynchronizácia vo VSYNC (`CONFIG_LCD_RGB_RESTART_IN_VSYNC`) a REST koncový bod `/api/display/resync`.

### ⚡ 18. Dvojjadrová architektúra FreeRTOS
- **Jadro 1:** Vyhradené pre vykresľovanie ST7701 (dvojitý framebuffer v PSRAM, nulové blikanie), dotykový čip CST820 a plynulé animácie.
- **Jadro 0 (`AsyncNetWorker`):** Asynchrónny sieťový worker obsluhujúci mbedTLS šifrovanie, sťahovanie radarov, parsovanie ADS-B JSON správ a webový server.

### 🌐 19. Webový portál & Bezpečné OTA aktualizácie
- Kompletná konfigurácia zariadenia, prepínanie obrazoviek a virtuálne diaľkové ovládanie.
- Živá tabuľka letov a telemetria ISS.
- Vstavaný webový sériový monitor (64 KB ring buffer v PSRAM) cez Wi-Fi bez nutnosti pripájania USB kábla.
- Nástroj na vytvorenie a stiahnutie nekomprimovaného 24-bitového BMP screenshotu.
- **Bezpečné OTA aktualizácie**: Proaktívne uvoľnenie vyrovnávacej pamäte (>6 MB v PSRAM) pred spustením aktualizácie a záložný priamy zápis do flash pamäte.

### 🔌 20. Smart Home REST API
- Integrované REST rozhranie pre Home Assistant, Node-RED alebo skripty (`/api/status`, `/api/hardware`, `/api/screen`, `/api/display/resync`, `/api/toggle-legends`, `/api/rtc/sync_ntp`).

### 🐾 21. Interaktívny Pixel Art spoločník & Autonómny DigiCat
- Celoobrazovková interaktívna zásuvka s virtuálnym spoločníkom dostupná z akejkoľvek obrazovky potiahnutím zospodu nahor (**Swipe Up**) alebo cez ikonu labky v Ovládacom centre.
- **Ručne tvorený 40-snímkový Pixel Art animačný engine**:
  - Žiarivá 16-farebná retro paleta (RGB565) škálovaná 2× (128×128 px) cez hardvérovo optimalizovaný span blitter (< 0.5 ms na snímku).
  - Autentický ryšavý tabby kocúrik s ikonickým „M“ na čele, bielou náprsenkou, ružovými vankúšikmi, smaragdovými očkami a animovaným chvostíkom.
  - **10 kompletných animovaných cyklov**: Chôdza (4 snímky s obojsmerným zrkadlením), Sedenie so žmurkaním a dýchaním (4 snímky), Blažené pradenie a červenanie (4 snímky), Sledovanie lietadiel na oblohe (2 snímky), Chrumkanie rybičky (4 snímky), Spiaci bochníček s písmenkami Zzz (4 snímky), Výskok s parabolickým letom (4 snímky), Umývanie tváre a uška labkou (4 snímky), Veľký mačací joga strečing (2 snímky) a **Chňapanie a škrabkanie labkami do výšky** (8 snímok).
- **Sledovanie preletu, lovenie a chňapanie po lietadlách**:
  - **Lietadlo na oblohe v reálnom čase:** Ak je v blízkosti detegované lietadlo cez ADS-B radar, v hornej časti obrazovky ($Y = 175..205$) plynule prelieta dvojmotorové pixel art lietadlo (28×14 px škálované 2× na 56×28 px).
  - **Kondenzačné stopy & zábleskové majáky:** Dvojité aerodynamické stopy motora, striedavo blikajúce krídelné navigačné majáky a štítok s volacím znakom a vzdialenosťou (`DLH123 · 8km`).
  - **Chňapanie a iskry pazúrikov (`CAT_STATE_SWAT`):** DigiCat beží po dráhe priamo pod lietadlom, postaví sa na zadné labky a labkami zúrivo chňapá do vzduchu s viditeľnými iskrami pazúrikov.
  - **Úhybné manévre & výskoky:** Zásah labkou alebo dotyk prsta na displeji vyvolá úhybný skok lietadla nahor s pípnutím, načo DigiCat vyskočí do výšky v pokuse ho chytiť (`CAT_STATE_JUMP`).
  - **Vycentrovaný text bubliny:** Výpočet textových blokov dokonale vycentruje dialóg v bubline horizontálne aj vertikálne.
- **Asfaltová dráha & 3D vrhaný tieň**:
  - **Runway platforma:** Fyzický dráhový povrch s centrálnymi žltými prerušovanými pruhmi a obvodovými návestidlami (jantárové vľavo, azúrové vpravo).
  - **Dynamický vrhaný tieň:** Mäkký oválny tieň pod labkami, ktorý pri skokoch zostáva na dráhe a plynulo sa zmenšuje a bledne s výškou skoku pre realistickú 3D priestorovú hĺbku.
  - **Povrch reagujúci na počasie:** Tmavý bridlicový asfalt, mokrý lesklý povrch počas dažďa a snehový poprašok počas sneženia.
- **Živé efekty počasia a adaptívne doplnky**:
  - **Integrácia živého počasia:** Prepojenie v reálnom čase s radarovým nowcastingom (`PrecipTracker`) a aktuálnymi kódmi počasia Open-Meteo.
  - **Animované vrstvy počasia:** Nočná obloha posiata 18 trblietajúcimi sa hviezdami, šikmé dažďové kvapky s nárazovými kruhmi na obrubníku dráhy, búrka s bleskami (70ms osvetlenie oblohy a rozvetvený výboj) a jemné snehové vločky reagujúce na náklon IMU senzora.
  - **Doplnky podľa počasia pre DigiCat:** Žlto-červený dáždnik v daždi a búrke (postavený vedľa spiaceho bochníčka s odskakujúcimi kvapkami; držaný v labke pri bdení), hrejivý pletený červeno-biely šál s vejúcim strapcom v zime a snehu ($\le 2^\circ\text{C}$) a retro letecké okuliare s azúrovými sklami na čele za jasného dňa.
- **Predĺžený nočný režim a postupné zaspávanie**:
  - **2-minútový nočný čas bdenia:** Interakcia s DigiCat v noci ho udrží bdelého a hravého celé 2 minúty s plnou autonómiou denného správania.
  - **Plynulé upokojenie pred spánkom:** 20 sekúnd pred zaspávaním DigiCat zívne, pretiahne sa a spomalí žmurkanie, než sa stočí do spiaceho bochníčka s tichým pradivým prianím dobrej noci.
  - **Ranné prebudenie so strečingom:** Ťuknutie na spiaceho kocúrika ho prebudí s rozkošným mačacím strečingom, pradivým pípnutím a pozdravom.
- **Autonómny mačací mozog s vlastnou vôľou**:
  - **Príchod z ktorejkoľvek strany:** Pri otvorení zásuvky v bdelom stave DigiCat pribehne náhodne zľava alebo sprava svižným klusom.
  - **Aktívny život na obrazovke:** Každých 6–12 sekúnd DigiCat sám podniká aktivity – prechádza sa po radarovej ploche, vyskakuje do výšky, umýva sa alebo sa naťahuje.
  - **Výlety po letisku (Airfield Excursion):** DigiCat sa môže autonómne rozhodnúť odísť z obrazovky na prieskum hangáru alebo loviť motýle. Na ploche zanechá radarový signál a vtipnú myšlienku.
  - **Privolanie na zavolanie:** Ťuknutie kdekoľvek na displej, kŕmenie, hladkanie alebo poklepanie po tele zariadenia okamžite privolá kocúrika späť rýchlym behom, veselým mňauknutím a radostnou reakciou.
- **Diaľkové ovládanie cez Web Rozhranie**:
  - Vyhradené tlačidlo v sekcii Diaľkový ovládač vo webovom prehliadači pre okamžité zobrazenie / zatvorenie zásuvky DigiCat cez Wi-Fi.
- **Mechaniky virtuálneho zvieratka a letecký rast**:
  - Živé sledovanie hladu (0–100%) a šťastia (0–100%) na HUD displeji.
  - Sledovaním reálnych lietadiel na radare získava DigiCat skúsenosti (XP) a postupuje v leteckých hodnostiach: *Mačací kadet* → *Radarový navigátor* → *Letecké eso*.
- **Umelá inteligencia Google Gemini Live**:
  - Voliteľné priame prepojenie s Google Gemini Flash API pre živé kontextové komentovanie počasia a preletov, s plnohodnotným 100% offline režimom v slovenčine, češtine a angličtine.
- **Atribúcia a licencia**: Projekt transparentne vychádza z inšpirácie a herných mechaník [aquascape123/digicat](https://github.com/aquascape123/digicat) pod licenciou MIT.

---

## 📱 Prehľad obrazoviek

| Obrazovka | Náhľad | Popis | Zdroj údajov |
| :--- | :---: | :--- | :--- |
| **0. Hodiny** | <img src="docs/media/clock_stacked_bold.png" width="70" /> | 6 voliteľných ciferníkov (Stacked Bold, Aviator, Orbital, Régulateur, Minimal, Classic Digital), počasie, mesiac, solárny oblúk so slnkom | Open-Meteo & Astro Engine |
| **1. Lietadlá** | <img src="docs/media/plane_radar_live.png" width="70" /> | 360° sledovanie vzdušného priestoru, núdzové lety (7700/7600), trasy liniek, letiská, kružnice dosahu | adsb.fi / adsb.lol |
| **1b. Detail lietadla** | <img src="docs/media/plane_detail_photo.png" width="70" /> | Po ťuknutí na lietadlo zobrazí kompletnú telemetriu, trasu a reálnu fotografiu lietadla | Planespotters.net API |
| **2. Radar zrážok** | <img src="docs/media/weather_radar_chmu.gif" width="70" /> | Animovaná slučka zrážkových kompozitov, plynulé prelínanie, stupnica odrazivosti, mestá | SHMÚ (SK), ČHMÚ (CZ), RainViewer |
| **3. Taktický radar** | <img src="docs/media/tactical_radar_live.gif" width="70" /> | **Kombinovaný taktický pohľad:** Živý radar zrážok + ADS-B letecká premávka na jednej obrazovke | SHMÚ / ČHMÚ / RainViewer + adsb.fi |
| **4. Predpoveď** | <img src="docs/media/forecast_screen.png" width="70" /> | Hodinové krivky teploty, vetra a zrážok, 3-dňová predpoveď, kvalita ovzdušia (AQI), PM2.5 a peľ | Open-Meteo Weather & Air Quality |
| **5. Trhy a Krypto** | <img src="docs/media/finance_screen.png" width="70" /> | Živé sledovanie 4 vlastných trhových tickerov (ETF, akcie, komodity, krypto, forex) so sviečkovým grafom | Yahoo Finance v8 |
| **6. Dráha ISS** | <img src="docs/media/screen_iss_live.png" width="70" /> | Mapa sveta so solárnym terminátorom deň/noc, minulá a budúca trajektória, kruh viditeľnosti, odpočet preletu | WhereTheISS API |
| **7. YouTube Analytika** | <img src="docs/media/youtube_screen.png" width="70" /> | Živá analytika kanála: počet odberateľov (veľká tučná typografia), celkové pozretia a štatistiky nového videa | YouTube Data API v3 |
| **8. Štatistiky letov** | <img src="docs/media/flight_stats_screen.png" width="70" /> | Denná 24h aktivita vzdušného priestoru: unikátne lietadlá, rekordná rýchlosť, dosah pre 6 rozsahov zoomu | FreeRTOS PSRAM Tracker |
| **9. Nastavenia** | <img src="docs/media/settings_screen.png" width="70" /> | Telemetria zariadenia, IP adresa, posuvník jasu, orientácia mapy, vyhladzovanie radaru, jazyk | Systém |

---

## 🔑 Návod: Ako získať bezplatný kľúč YouTube Data API

Obrazovka YouTube Analytiky sa pripája k oficiálnemu rozhraniu **Google YouTube Data API v3**, aby zobrazovala presný počet odberateľov, celkové zhliadnutia a štatistiku najnovšieho videa. Postupujte podľa tohto jednoduchého návodu:

### Krok 1: Vytvorenie projektu v Google Cloud
1. Otvorte konzolu **[Google Cloud Console](https://console.cloud.google.com/)** a prihláste sa svojím Google účtom.
2. V hornej lište kliknite na rozbaľovací zoznam projektov a zvoľte **New Project** (Nový projekt).
3. Zadajte názov projektu (napr. `MeteoPlaneRadar`) a kliknite na **Create** (Vytvoriť).

### Krok 2: Povolenie YouTube Data API v3
1. Do vyhľadávacieho poľa v hornej časti zadajte `YouTube Data API v3` a vyberte ho zo zoznamu výsledkov.
2. Kliknite na modré tlačidlo **Enable** (Povoliť).

### Krok 3: Vytvorenie prihlasovacích údajov (API Kľúč)
1. V ľavom navigačnom menu prejdite na **APIs & Services** > **Credentials** (Poverenia).
2. V hornej lište kliknite na **+ CREATE CREDENTIALS** a vyberte možnosť **API key**.
3. Váš nový API kľúč sa okamžite vygeneruje (začína sa reťazcom `AIzaSy...`). Skopírujte si ho.  
   *(Voliteľné, ale odporúčané: Kliknite na "Edit API key" a v sekcii "API restrictions" obmedzte použitie kľúča výhradne na "YouTube Data API v3").*

### Krok 4: Zistenie identifikátora kanála
Môžete použiť buď:
- **Handle kanála**: Používateľské meno kanála so zavináčom (napr. `@CuriousCatFPV` alebo `@MKBHD`).
- **Channel ID**: 24-znakový reťazec začínajúci na `UC...` dostupný v YouTube Štúdiu > Prispôsobenie > Základné informácie.

### Krok 5: Nastavenie v zariadení MeteoPlaneRadar
1. Otvorte webové rozhranie zariadenia na adrese `http://meteoplaneradar.local/`.
2. Prejdite na záložku **YouTube**.
3. Vložte váš **API Kľúč** a zadajte **Handle alebo ID kanála**.
4. Kliknite na **Uložiť nastavenia**. Zariadenie okamžite overí kľúč a na obrazovke YouTube zobrazí živé dáta!

---

## 🖐️ Dotykové ovládanie a gestá

| Gesto / Akcia | Výsledok |
| :--- | :--- |
| **Potiahnutie doľava / doprava** | Plynulý posun na nasledujúcu / predchádzajúcu obrazovku. |
| **Stiahnutie z horného okraja** | Otvorí **Rýchle ovládacie centrum** (jas, nočný režim, prepínače obrazoviek, výber trhov). |
| **Potiahnutie zospodu nahor** | Otvorí **Zásuvku virtuálneho zvieratka DigiCat**. |
| **V zásuvke Pet Drawer** | **Ťuknutie na zvieratko:** Pohladkanie / prejav lásky (okamžité dialógy, červenanie líc, poskočenie).<br>**Dvojité ťuknutie:** Hodenie maškrty (zlatá rybička, nasýti zvieratko a zvýši šťastie).<br>**Potiahnutie nadol / ťuknutie na lištu:** Zavrie zásuvku. |
| **Potiahnutie hore / dole v strede** | **Na hodinách:** Prepína predchádzajúci / nasledujúci ciferník.<br>**Na radaroch:** Priblíženie (Zoom In - hore) / Oddialenie (Zoom Out - dole).<br>**V ovládacom centre:** Zavrie menu. |
| **Ťuknutie na spodnú lištu rozsahu** | Ľavá polovica oddiali (Zoom Out), pravá polovica priblíži (Zoom In). |
| **Ťuknutie na lietadlo** | Otvorí detailnú telemetrickú kartu lietadla s fotografiou. |
| **Ťuknutie na fotografiu lietadla** | Zväčší fotografiu lietadla na celú obrazovku. |
| **Ťuknutie na obrazovku YouTube** | Okamžite vyžiada čerstvé dáta odberateľov a videí so zvukovým potvrdením. |
| **Dvojité poklepanie (na rám / stôl)** | **Na radaroch:** Režim čistého zobrazenia (skryje legendy a texty).<br>**Na trhoch / ISS:** Vynúti okamžité obnovenie údajov. |
| **Podržanie tlačidla BOOT pri štarte (~3 s)** | Továrenský reset (vymaže uložené Wi-Fi siete a NVS nastavenia). |

---

## 🔧 Hardvérové špecifikácie

Vyvinuté presne pre **[Waveshare ESP32-S3-Touch-LCD-2.1](https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm)**:

| Komponent | Špecifikácia |
| :--- | :--- |
| **Procesor (MCU)** | Espressif ESP32-S3R8 (Xtensa® Dual-Core 32-bit LX7 @ 240 MHz) |
| **Pamäť** | 8 MB Octal PSRAM + 16 MB Quad SPI Flash |
| **Displej** | Okrúhly 2.1" IPS, 480×480 px, 65k RGB565 farieb, ST7701 RGB zbernica |
| **Dotyk** | CST820 / CHSC6540 kapacitný dotykový ovládač (I2C) |
| **I/O expandér** | TCA9554PWR (ovláda napájanie displeja, podsvietenie a reset) |
| **Senzor IMU** | QMI8658 6-osový akcelerometer a gyroskop (detekcia poklepania a orientácie) |
| **RTC čip** | PCF85063 Real-Time Clock so zálohovaním času (I2C `0x51`) |
| **Konektivita** | USB-C (napájanie + CDC sériová linka), Wi-Fi 802.11 b/g/n (2.4 GHz) |

---

## 🚀 Inštalácia a nahrávanie firmvéru

### Možnosť A: Web Flasher / Predkompilované binárky (Najjednoduchšie)
Stiahnite si najnovšiu hotovú binárku zo stránky [Vydania (Releases)](https://github.com/hackra76/ESP-MeteoPlaneRadar/releases):
- `MeteoPlaneRadar-v1.9.9-factory.bin` (Kompletný samostatný obraz vrátane bootloadera, partícií a aplikácie).
- Nahrajte firmvér priamo v prehliadači Chrome cez [ESP Web Flasher](https://espressif.github.io/esptool-js/) pri rýchlosti 921600 baud od adresy `0x00000000`.

Alebo cez príkazový riadok pomocou `esptool.py`:
```bash
esptool.py -p COM_PORT -b 921600 --before default_reset --after hard_reset write_flash 0x0 MeteoPlaneRadar-v1.9.9-factory.bin
```

### Možnosť B: Kompilácia cez PlatformIO
1. Otvorte priečinok projektu vo **Visual Studio Code** s nainštalovaným rozšírením **PlatformIO IDE**.
2. Pripojte dosku k PC cez USB-C.
3. Spustite príkazy:
   ```bash
   # Kompilácia firmvéru
   pio run

   # Nahratie do zariadenia
   pio run -t upload
   ```

---

## 📶 Pripojenie k Wi-Fi sieti

1. Pri prvom zapnutí zariadenie vytvorí otvorenú sieť s názvom **`MeteoPlaneRadar`** a na displeji zobrazí QR kód.
2. Pripojte sa k sieti mobilom alebo notebookom.
3. Otvorte prehliadač na adrese **`http://192.168.4.1/`**.
4. Vyberte vašu domácu Wi-Fi sieť, zadajte heslo a uložte nastavenia.
5. Zariadenie sa pripojí a na displeji zobrazí svoju pridelenú IP adresu.

### Webový ovládací panel
Po pripojení otvorte ovládací panel v prehliadači:
- **`http://meteoplaneradar.local/`** (alebo cez IP adresu, napr. `http://192.168.0.2/`).

---

## 🌐 Smart Home REST API

Jednoduchá integrácia s **Home Assistant**, **Node-RED** alebo vlastnými skriptami:

- `GET /api/status` – Kompletný JSON stav (počasie, počty lietadiel, voľná pamäť, stanica ISS).
- `GET /api/hardware` – Zoznam periférií, stav RTC hodín, sken I2C zbernice a dôvod posledného resetu.
- `POST /api/screen` – Prepnutie obrazovky: `{"index": 0}` (0: Hodiny, 1: Lietadlá, 2: Zrážky, 3: Taktický radar, 4: Predpoveď, 5: Trhy, 6: ISS, 7: YouTube, 8: Štatistiky, 9: Nastavenia).
- `POST /api/display/resync` – Okamžitá hardvérová resynchronizácia displeja bez reštartu.
- `POST /api/toggle-legends` – Prepnutie zobrazenia legiend a čistého režimu mapy.
- `POST /api/rtc/sync_ntp` – Vynútenie okamžitej synchronizácie hardvérového RTC času z NTP.

---

## 📜 Licencia a poďakovanie

Šírené pod licenciou **MIT License**.
- Pôvodný základný projekt: **[petus/MeteoPlaneRadar](https://github.com/petus/MeteoPlaneRadar)**.
- Vektorová grafika virtuálneho zvieratka, spiaca mačka a herné štatistiky: **[aquascape123/digicat](https://github.com/aquascape123/digicat)** od aquascape123 (MIT License).
- Vylepšenia, slovenská lokalizácia, SHMÚ radar, bilineárny anti-aliasing, Planespotters fotografie lietadiel, kombinovaný taktický radar, rozšírené ciferníky hodín, ovládač RTC, IMU gestá, dotykové ovládanie, Ovládacie centrum, analytika YouTube kanála, sledovanie dráhy ISS, AI zvieratko a stabilizácia: **Rado & Antigravity AI**.
