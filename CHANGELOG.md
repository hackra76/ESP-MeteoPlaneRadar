# Changelog

Všechny podstatné změny v projektu **MeteoPlaneRadar**.
Formát vychází z [Keep a Changelog](https://keepachangelog.com/cs/1.1.0/),
verzování je [semantické](https://semver.org/lang/cs/).

Verze je v jediném místě: `MeteoPlaneRadar/Version.h` (`FW_VERSION`). Zobrazuje se na
obrazovce Nastavení, na webové stránce a v sériovém výpisu při startu.
Laditelné konstanty (krok otočení, tolerance výpadků, ladicí výpisy) jsou
pohromadě v `MeteoPlaneRadar/Config.h`.

## [2.0.4] - 2026-09-20

### Opravené / Fixed
- **Vykresľovanie dáždnika a doplnkov (Non-Destructive Weather Accessories):**
  - **Odstránené čierne orezávacie obdĺžniky:** Dáždnik (ručná verzia aj stojanový slnečník) bol prepracovaný na polkruhové riadkové scanline vykresľovanie (`drawUmbrellaCanopy`) bez použitia deštruktívneho `fillRect(..., C_BLACK)`, ktorý predtým prekrýval hlavu, uši a srsť zvieratka čiernym blokom.
  - **Stojanový slnečník pre spiace zvieratko:** Pri spánku v daždi je slnečník umiestnený nad zvieratkom s tyčou a podstavcom ukotveným vedľa neho na dráhe, takže zvieratko je chránené pred dažďom bez kolízie so spiacim telíčkom.
  - **Potlačenie dáždnika pri akrobatických akciách:** Počas skákania (`JUMP`), chňapania po lietadle (`SWAT`), jedenia maškrty (`EATING`) alebo neprítomnosti (`AWAY`) sa držanie dáždnika dočasne deaktivuje pre prirodzenejší vzhľad.
  - **Čistý vrhaný tieň na dráhe:** Odstránené vnútorné `0x0000` čierne výrezy v tieni pod labkami, ktoré predtým rezali čierny otvor cez farebný asfalt dráhy.
- **Optimalizácia sledovania preletov (Flight Tracking Calibration):**
  - **Obmedzenie preletov na bezprostrednú blízkosť ($\le 10\text{ km}$):** Vykresľovanie lietadielka a chňapanie sa aktivuje výhradne pri blízkych preletoch priamo nad hlavou ($\le 10\text{ km}$), čo zabraňuje neustálemu rozptyľovaniu zvieratka vzdialenými letmi.
  - **Prirodzený oddych po chňapaní:** Pridaný časovač pauzy (`s_planeChasePauseUntilMs`), vďaka ktorému sa DigiCat po niekoľkých cykloch chňapania vráti k pokojnému posedávaniu a ďalším autonómnym aktivitám.
  - **Rozlíšené radarové myšlienky:** Pre lety vo vzdialenosti $10-40\text{ km}$ DigiCat komentuje sledovanie cieľa na radare namiesto priameho chňapania do vzduchu.

## [2.0.3] - 2026-09-19

### Pridané / Added
- **Sledovanie preletu, lovenie a chňapanie po lietadlách (Overhead Aircraft Tracking & Paw Swatting):**
  - **Lietadlo na oblohe v reálnom čase:** Ak je v blízkosti detegované lietadlo cez ADS-B radar, v hornej časti obrazovky zvieratka ($Y = 175..205$) plynule prelieta dvojmotorové pixel art lietadlo (28×14 px škálované 2× na 56×28 px).
  - **Dymové stopy a blikajúce majáky:** Dvojité aerodynamické kondenzačné čiary (contrails) a striedavo blikajúce krídelné navigačné majáky doplnené o štítok s volacím znakom a vzdialenosťou (napr. `DLH123 · 8km`).
  - **Lovenie a nová 8-snímková animácia chňapania (`CAT_STATE_SWAT`):** DigiCat beží po dráhe pod lietadlom, postaví sa na zadné labky a labkami zúrivo chňapá a škrabká do vzduchu priamo po lietadielku s vizuálnymi iskrami pazúrikov.
  - **Úhybný manéver lietadla:** Pri zásahu labkou alebo ťuknutí na lietadlo prstami lietadlo vykoná úhybný skok nahor so zvukovým čipnutím a DigiCat po ňom môže skočiť (`CAT_STATE_JUMP`).
- **Vylepšenia používateľského rozhrania a dialógov (UI & Speech Bubble Enhancements):**
  - **Vycentrovaný text v bubline dialógu:** Prepracovaný layout textu presne počíta výšku riadkov a dokonale centruje viacriadkový text horizontálne aj vertikálne v bubline.
  - **Odstránené slnečné okuliare:** Pri slnečnom počasí už zvieratko nenosí slnečné okuliare, takže má čistú a ničím nezakrytú tváričku.
  - **Kontextové hlášky a Gemini AI:** Pridané témy lovenia lietadiel, chňapania labkami a sledovania preletov do offline generátora myšlienok aj do promptu pre Google Gemini LLM.

## [2.0.2] - 2026-09-18

### Pridané / Added
- **Rozšírený nočný režim a postupné zaspávanie (Extended Night Wake & Gradual Sleep):**
  - Predĺžený interaktívny čas bdenia v noci na 2 minúty po akejkoľvek interakcii (kŕmenie, hladkanie, privolanie alebo ťuknutie).
  - Počas bdenia má DigiCat plnú autonómiu denného správania (prechádzky, výskoky, strečing, hygiena, sledovanie lietadiel).
  - Plynulý a prirodzený prechod do spánku: 20 sekúnd pred vypršaním času DigiCat zívne, pretiahne sa a spomalí žmurkanie, než sa stočí do spiaceho bochníčka s tichým pradivým prianím dobrej noci.
  - Prebudenie spiaceho zvieratka sprevádza rozkošný ranný strečing a pradivý pozdrav.
  - Obnovená animácia príchodu zo strán obrazovky pri aktivácii panela v bdelom stave.
- **Dráhový asfalt, 3D tieň a dynamické počasie pre DigiCat (Runway Deck & Weather Effects):**
  - **Asfaltová dráha a 3D vrhaný tieň:** Vytvorená fyzická runway platforma s centrálnymi prerušovanými pruhmi a obvodovými návestidlami (jantárové a azúrové). Mäkký vrhaný tieň pod labkami dynamicky reaguje na skoky (zmenšuje sa a bledne s výškou skoku).
  - **Efekty počasia v reálnom čase:** Nočná obloha posiata trblietajúcimi sa hviezdami, šikmé dažďové kvapky s nárazovými kruhmi na dráhe, búrka s bleskami a plávajúce snehové vločky reagujúce na náklon senzora QMI8658.
  - **Doplnky prispôsobené počasiu:** Žlto-červený dáždnik chrániaci DigiCat v daždi a búrke, hrejivý červeno-biely pletený šál v chlade a snehu ($\le 2^\circ\text{C}$), a retro letecké okuliare s azúrovými sklami za jasného počasia.

## [2.0.1] - 2026-09-18

### Pridané / Added
- **Diaľkové ovládanie DigiCat cez Web UI:**
  - Pridané tlačidlo v sekcii Diaľkový ovládač (`#cardRemote`) pre zobrazenie / skrytie panela DigiCat priamo z prehliadača.
  - Plná podpora stavu v reálnom čase (`petOpen`), dynamická indikácia na štítku obrazovky a lokalizácia do všetkých 3 jazykov (SK, CZ, EN).
  - Nový REST koncový bod `POST /api/pet/toggle` s bezpečným spracovaním v hlavnej slučke pre plynulé prekreslenie.

## [2.0.0] - 2026-09-18

### Pridané / Added
- **Plne animovaný Pixel Art DigiCat (Handcrafted 32-Frame Pixel Art Companion):**
  - Kompletné prepracovanie vizuálov zvieratka z procedurálnej geometrie na prémiový 16-farebný pixel art s 32 ručne navrhnutými snímkami (64×64 px škálovanými 2× na 128×128 px) bežiacimi cez hardvérovo akcelerovaný run-length span blitter (< 0.5 ms na snímku).
  - Plynulé animované cykly:
    - **Chôdza / Walk Cycle** (4 snímky): Pohyb labiek, kývanie tela a dynamický chvostík s obojsmerným horizontálnym zrkadlením (`flipX`).
    - **Sedenie & Život / Sitting Idle** (4 snímky): Plynulé dýchanie, prirodzené žmurkanie, šklbnutie uškom.
    - **Hladkanie & Láska / Happy Purr** (4 snímky): Blažený úsmev, červenajúce sa líčka, lietajúce srdiečka a akustické pradenie bzučiaka.
    - **Letecký radar / Sledovanie oblohy** (2 snímky): Otáčanie v smere letu a pohľad vysoko nahor.
    - **Kŕmenie / Eating Snack** (4 snímky): Chrumkavá rybička, hryzenie s omrvinkami a oblizovanie fúzikov.
    - **Spánok / Sleeping Loaf** (4 snímky): Stočený bochníček s hlbokým dýchaním a stúpajúcim *Zzz*.
    - **Výskok & Pounce / Jumping** (4 snímky): Prikrčenie pred skokom, dynamický parabolický výskok (-32 px), let vo vzduchu a mäkké odpružené dosadnutie.
    - **Umývanie & Hygiena / Grooming** (4 snímky): Zdvihnutie labky, olizovanie vankúšika, umývanie líčka a čistenie uška.
    - **Veľký mačací strečing / Yoga Stretch** (2 snímky): Prehnutie predných labiek s vystrčeným chrbtom a vysoký dúhový mačací chrbát.
- **Autonómny mačací mozog s vlastnou vôľou (Autonomous AI Brain & Free Will):**
  - **Príchod z ktorejkoľvek strany:** Pri otvorení zásuvky DigiCat prichádza náhodne zľava alebo sprava energickým klusom.
  - **Voľný pohyb a prieskum:** DigiCat sa nenechá zamknúť na jednom mieste; autonómne sa prechádza po radarovej palube, vyskakuje za neviditeľnými moľami, umýva sa a naťahuje.
  - **Odchod na prieskum hangáru (Airfield Excursion):** DigiCat sa občas rozhodne odísť z obrazovky na prieskum hangáru alebo za motýľom. Na ploche zanechá radarový pulz a vtipnú myšlienku.
  - **Privolanie ťuknutím (Return on Call):** Ťuknutie kdekoľvek na displej, tlačidlá kŕmenia/hladkania alebo zatrasenie zariadením okamžite privolá DigiCat späť rýchlym behom, veselým mňauknutím a radostnou reakciou.
- **Vyhradená karta Zvieratko vo Web Konfigurácii (Dedicated Pet Web UI Tab):**
  - Samostatná karta pre DigiCat vo webovom rozhraní s podrobným návodom na získanie bezplatného Google Gemini API kľúča a sprievodcom dotykovými gestami.
- **Plná trojjazyčná lokalizácia (SK / CZ / EN):**
  - Kompletné slovenské, české a anglické dialógy, myšlienky, hodnosti a webové popisky pre DigiCat.

### Opravené / Fixed
- **Odblokovanie autonómneho správania pri leteckej prevádzke:**
  - Odstránená trvalá blokáda v rozhodovacej slučke pri zachytení lietadla do 45 km. Sledovanie lietadla je teraz dynamickou 3.5s akciou, vďaka čomu sa DigiCat nezastaví uprostred obrazovky a naďalej voľne žije, skáče a behá.

## [1.9.9] - 2026-09-17

### Pridané / Added
- **Interaktívna zásuvka Pet Drawer s replikou DigiCat (Interactive Virtual Pet & DigiCat):**
  - Nová celoobrazovková interaktívna zásuvka Pet Drawer dostupná z akejkoľvek obrazovky potiahnutím zospodu nahor (Swipe Up) alebo cez dotykové tlačidlo labky v Quick Control ponuke.
  - Plná integrácia a autentická grafická replika **DigiCat** (inšpirovaná projektom [aquascape123/digicat](https://github.com/aquascape123/digicat) pod licenciou MIT):
    - Žiarivo oranžový tabby kožuch (`0xFD20`) s tmavými pruhmi (`0x9260`), ikonické znaky „M“ na čele a lícach, biela náprsenka, ružový noštek (`0xFBEF`) a labky s vankúšikmi.
    - Smaragdovo zelené oči (`0x07E0`) s vertikálnymi zreničkami otáčajúcimi sa v reálnom čase v smere azimutu najbližšieho lietadla na radare.
    - Hladká animácia kývajúceho sa chvostíka pomocou sínusoidy a animácia žmurkania.
    - Útulná spiaca poloha (Sleeping Ball) so stočeným telíčkom a stúpajúcimi animovanými písmenkami *„Z z z“* počas nočného režimu alebo neskoro v noci.
  - **Mechaniky virtuálneho zvieratka a letecké hodnosti (Virtual Pet Mechanics):**
    - Sledovanie štatistík **Šťastie (Happiness 0–100%)** a **Hlad (Hunger 0–100%)** s priamym zobrazením na HUD displeji zásuvky.
    - Hladkanie a ťuknutie: Okamžitá interaktívna reakcia zvieratka (0 ms latencia) s poskočením, červenajúcimi sa lícami, srdiečkami a radostným zavrnením/štekotom.
    - Kŕmenie maškrtami: Dvojité ťuknutie alebo kŕmne tlačidlo hádže zlaté rybičky/maškrty s animáciou jedenia, znižuje hlad a zvyšuje šťastie.
    - Letecký rast a XP: Sledovanie preletov lietadiel zvyšuje skúsenosti zvieratka a odomyká letecké hodnosti (*Kitten Cadet / Flight Cadet* → *Radar Navigator* → *Airspace Ace*).
- **Dynamické zisťovanie modelov Gemini LLM (Dynamic Gemini Flash Discovery):**
  - Automatická dynamická detekcia a fallback pre najnovšie modely Google Gemini Flash API (`gemini-2.5-flash`, `gemini-3-flash-preview` a pod.) pri vyradení starších modelov bez nutnosti manuálneho zásahu.
  - Zvieratko reaguje na reálne lietadlá v okolí, počasie a náladu cez kontextové AI myšlienky.
- **Právna a licenčná atribúcia (MIT License Attribution):**
  - Do všetkých dokumentov (`README.md`, `README_SK.md`, `PetBrain.h`, `PetDrawer.cpp`) pridané transparentné poďakovanie autorovi `aquascape123/digicat` za vizuálne inšpirácie a herné mechaniky virtuálneho zvieratka pod licenciou MIT.

### Zmenené / Changed
- **Úplné nahradenie Nimbus Duck:**
  - Charakter číslo 3 (pôvodne Nimbus Duck) bol kompletne odstránený zo všetkých dialógov, nastavení, webového rozhrania a grafických modulov a nahradený zvieratkom **DigiCat**.

## [1.9.8] - 2026-09-16

### Opravené / Fixed
- **Hardvérový posun a plávanie obrazu ST7701 (ST7701 RGB Hardware Display Drift Fix):**
  - Trvalo odstránené periodické zápisy do SPI Flash pamäte pri automatickom striedaní obrazoviek (carousel), ktoré spôsobovali pozastavenie vyrovnávacej pamäte PSRAM (15–30 ms) a následné podtečenie GDMA FIFO vyvolávajúce zvislý posun obrazu.
  - Obnovené továrenské parametre časovania a verandy ST7701 (VBP = 8, HBP = 10, HFP = 50, VPW = 8) a register `0xC1` (PORCTRL `0x0B, 0x02`), čím sa obraz vycentroval s nulovou medzerou na okrajoch.
  - Implementovaná bezpečná hardvérová resynchronizácia v `LCD_Restart` cez SPI príkazy Display OFF (`0x28`), vyčkanie na VSYNC hranicu a Display ON (`0x29`).
  - Pridaný nový REST koncový bod `/api/display/resync` pre okamžité manuálne zarovnanie obrazu bez nutnosti reštartu zariadenia.
- **Serializácia prepínača YouTube obrazovky vo Web Konfigurácii:**
  - Opravená chýbajúca serializácia poľa `youtube` v objekte `screens` vo `WebPage.h`, vďaka čomu sa vypnutie/zapnutie obrazovky YouTube cez web spoľahlivo ukladá do pamäte NVS (`ytScrInit`).
  - Webový server a navigačné jadro okamžite odmietajú skok na vypnutú obrazovku s HTTP 409 Conflict.
- **Optimalizácia pamäte pre HTTPS / TLS handshake (`mbedTLS`):**
  - Vybalansovaná veľkosť DMA bounce bufferov na 30 riadkov ($30 \times 480\text{ px}$), čím sa uvoľnilo 97+ KB nepretržitého interného SRAM heapu potrebného pre TLS buffery `mbedTLS` pri šifrovaných HTTPS volaniach (ADSB, predpoveď počasia, NTP).

## [1.9.7] - 2026-09-15

### Pridané / Added
- **Nová obrazovka: Analytika YouTube kanála (YouTube Channel Analytics Screen):**
  - Nová samostatná obrazovka (`SCREEN_YOUTUBE_I`) zobrazujúca aktuálne metriky YouTube kanála v reálnom čase pomocou oficiálneho YouTube Data API v3.
  - Výrazná Hero karta s počtom odberateľov (Subscribers) v novom veľkom bold štýle písma `FONT_HERO` (32 px) a presným celkovým počtom odberateľov.
  - Spodné rozdelené karty s celkovým počtom pozretí kanála (Total Views) s čipom počtu videí a štatistikou najnovšieho nahraného videa (Latest Video) s označením NEW, počtom zhliadnutí a dvojriadkovým zalamovaným názvom videa.
  - Interaktívne obnovenie údajov ťuknutím na displej s akustickou odozvou a automatická periodická aktualizácia na pozadí (3 min aktívna obrazovka, 15 min na pozadí).
- **Nastavenie YouTube v ovládacom paneli Web Portal:**
  - Samostatná záložka YouTube vo webovom rozhraní s poliami pre vloženie bezplatného Google Cloud YouTube API kľúča a identifikátora kanála (handle napr. `@CuriousCat` alebo Channel ID `UC...`) s priamym ukladaním do pamäte NVS.
- **Nový typografický stupeň `FONT_HERO` (FontEngine):**
  - Pridané 32-pixelové vyhladené vektorové písmo (`u8g2_font_logisoso32_tr`) do grafického enginu pre zobrazenie veľkých a dobre čitateľných čísiel a metrík.
- **Symbol slnka s lúčmi na 24h solárnom oblúku (Solar Arc):**
  - Pôvodný jednoduchý žltý bod na ciferníkoch hodín nahradený grafickým symbolom žiariaceho slnka s centrálnym diskom a 8 radiálnymi lúčmi na kontrastnom podklade.

### Zmenené / Changed
- **Odstránený ciferník stíhačky (Fighter HUD watchface):**
  - Ciferník HUD bol kompletne odstránený z enginu hodín, výberu štýlov aj webového rozhrania. Celkový počet ciferníkov upravený na 6 (Digitálny, Analógový, Orbitálny, Regulátor, Vrstvený, Minimálny).
- **Optimalizácia sieťových slučiek na jadre 0 (AsyncCore):**
  - Opravené podmienky periodického sťahovania YouTube a ISS údajov tak, aby striktne rešpektovali nastavené časové periódy a nezaťažovali sieťovú zbernicu.
- **Odstránené rizikové reštarty RGB DMA (`LCD_Restart`):**
  - Zamedzené volaniu `esp_lcd_rgb_panel_restart` počas OTA zápisov do flash pamäte, čím sa natrvalo eliminovala možná desynchronizácia parity bounce bufferov.

## [1.9.6] - 2026-09-14

### Vylepšené / Improved
- **Paralelné počítanie štatistík letov pre všetky úrovne priblíženia (Zero-Reset Multi-Scope Flight Statistics):**
  - Všetky prijaté ADS-B správy sú v pamäti PSRAM paralelne a nezávisle vyhodnocované a započítavané do samostatných štatistických kategórií pre všetky zoom rozsahy (celý feed ALL, $\le 10\text{ km}$, $\le 25\text{ km}$, $\le 50\text{ km}$, $\le 100\text{ km}$, $\le 200\text{ km}$).
  - Prepínanie rozsahu na obrazovke Info & Štatistiky ťuknutím na hlavičku karty premávky plynulo cykluje medzi rozsahmi bez akéhokoľvek resetovania alebo straty nazbieraných denných údajov.
  - Resetovanie štatistík prebieha výhradne automaticky o polnoci alebo manuálnym stlačením tlačidla Reset v spodnej časti obrazovky.
  - Webové rozhranie aj REST API `/api/stats?scope=X` umožňujú okamžité zobrazenie ľubovoľného rozsahu bez vymazania údajov.

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
