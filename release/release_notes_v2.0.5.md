# Release Notes v2.0.5

## Pridané / Added (since v2.0.3)
- **Fyzikálna interakcia s IMU náklonom pre DigiCat (IMU Real-Time Tilt Physics & Reactions):**
  - **Mierny náklon (10° - 25°) — Zábavné kĺzanie / surfovanie:** Zvieratko sa ladne šmýka po dráhe v smere gravitácie s veternými stopami. Spúšťa pradenie a zvyšuje úroveň šťastia.
  - **Strmý náklon (25° - 45°) — Vystrašené kotúľanie:** DigiCat stráca rovnováhu, rotuje po dráhe, škrabe pazúrikmi, potí sa a prepína sa do vystrašenej nálady.
  - **Extrémny náklon (> 45°):** Náhodný výber:
    - **Zachytenie sa o okraj displeja:** Zvieratko sa zachytí prednými labkami a prosí o vyrovnanie.
    - **Zošmyknutie mimo obrazovky:** DigiCat sa zosunie úplne mimo displeja. Po vyrovnaní vbehne späť.

## Opravené / Fixed
- **Ochrana proti opotrebeniu NVS Flash pamäte (Flash Wear Prevention):**
  - Predĺžený interval debouncingu pre ukladanie štatistík letov (`FlightStats`) a stavu zvieratka (`PetBrain`) z niekoľkých sekúnd na 15 minút. Toto kritické vylepšenie zabraňuje nadmernému opotrebeniu a zničeniu internej flash pamäte ESP32.
- **Bezpečnosť Webového Rozhrania (Web UI Security Enhancements):**
  - **Ochrana proti Brute-Force útokom:** Pridaný mechanizmus uzamknutia (lockout) na 5 minút po 5 neúspešných pokusoch o zadanie administrátorského hesla. Počas uzamknutia server vracia chybu HTTP 429 Too Many Requests.
  - **Maskovanie API kľúčov:** Konfiguračný JSON odosielaný do prehliadača teraz maskuje citlivé kľúče (`youtubeKey`, `geminiKey`) hodnotou `***`. To zabraňuje ich odpočúvaniu v lokálnej sieti.
- **Vykresľovanie dáždnika a doplnkov (Non-Destructive Weather Accessories):**
  - Odstránené čierne orezávacie obdĺžniky z dáždnika, tieňov a spánku, čo zlepšilo grafický dojem.
- **Oprava IMU Tilt Senzoru (Stand Mode Fix):** Fyzikálny engine teraz správne ignoruje náklon zariadenia dopredu/dozadu (pitch), v ktorom zariadenie prirodzene odpočíva v stojane. DigiCat reaguje výlučne na naklonenie doľava/doprava (roll).
- **Optimalizácia sledovania preletov:** Chňapanie a animácie sa spúšťajú len pri bezprostredných letoch (≤ 10 km).
- **Perzistencia štatistík letov:** Automatický reset o polnoci a ukladanie videných ICAO kódov pre deduplikáciu letov (šetrenie flash pamäte).
- **Získavanie XP bodov a NVS ukladanie XP:** Optimalizované ukladanie nálady, levelov a XP pre zvieratko.

*Flashovanie OTA:* `MeteoPlaneRadar-v2.0.5-ota.bin`  
*Flashovanie cez kábel:* `MeteoPlaneRadar-v2.0.5-factory.bin`
