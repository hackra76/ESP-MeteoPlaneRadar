// =============================================================================
//  MeteoPlaneRadar
//  The configuration page, as one PROGMEM string.
//
//  Author:  Petr / chiptron.cz & Antigravity
// =============================================================================
#pragma once
#include <Arduino.h>

static const char PAGE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="sk"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="dark">
<title>MeteoPlaneRadar · H4CKR4</title>
<style>
:root{
  --bg:#090d14;--card:#121824;--card-hdr:#182030;--line:#222d42;--fg:#e2e8f0;--mut:#8896ab;
  --acc:#38bdf8;--acc-glow:rgba(56,189,248,0.25);--ok:#22c55e;--warn:#f59e0b;--err:#ef4444;--purple:#a855f7;
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;-webkit-text-size-adjust:100%}
header{padding:12px 18px;display:flex;align-items:center;gap:12px;flex-wrap:wrap;background:#0d131f;border-bottom:1px solid var(--line)}
.badge{background:rgba(56,189,248,0.15);color:var(--acc);padding:2px 7px;border-radius:6px;font-size:11px;font-weight:600;border:1px solid rgba(56,189,248,0.3);text-decoration:none;transition:all 0.15s ease}
.badge:hover{background:rgba(56,189,248,0.3);color:#fff;border-color:var(--acc);box-shadow:0 0 10px var(--acc-glow)}
.ver{color:var(--mut);font-size:12px}
nav.tabs{display:flex;gap:6px;overflow-x:auto;padding:8px 14px;border-bottom:1px solid var(--line);
 position:sticky;top:0;background:rgba(9,13,20,0.92);backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px);z-index:20;scrollbar-width:none}
nav.tabs::-webkit-scrollbar{display:none}
nav.tabs button{background:transparent;color:var(--mut);border:1px solid transparent;border-radius:8px;
 padding:7px 13px;white-space:nowrap;font-weight:500;font-size:13px;cursor:pointer;transition:all 0.15s ease}
nav.tabs button:hover{color:var(--fg);background:#161f30}
nav.tabs button.on{background:var(--card-hdr);color:var(--acc);border-color:var(--line);font-weight:600;box-shadow:0 0 12px var(--acc-glow)}
.wrap{max-width:840px;margin:0 auto;padding:16px 14px 96px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 4px 16px rgba(0,0,0,0.2)}
.card h2{font-size:15px;margin:0 0 12px;color:var(--acc);display:flex;align-items:center;gap:8px;font-weight:600;border-bottom:1px solid rgba(34,45,66,0.6);padding-bottom:8px}
.row{display:flex;align-items:center;gap:10px;margin:10px 0;flex-wrap:wrap}
.row label{flex:1 1 200px;min-width:150px;color:var(--fg);font-size:13.5px}
input[type=text],input[type=password],input[type=number],select{background:#0a0e17;color:var(--fg);
 border:1px solid var(--line);border-radius:7px;padding:8px 10px;min-width:110px;font-size:13.5px;outline:none;transition:border-color 0.15s}
input[type=text]:focus,input[type=password]:focus,input[type=number]:focus,select:focus{border-color:var(--acc)}
input[type=color]{background:#0a0e17;border:1px solid var(--line);border-radius:7px;height:36px;width:56px;padding:2px;cursor:pointer}
input[type=range]{flex:1 1 160px;accent-color:var(--acc)}
input[type=checkbox]{width:18px;height:18px;accent-color:var(--acc);cursor:pointer}
button{background:var(--acc);color:#08202a;border:0;border-radius:8px;padding:8px 14px;font-weight:600;
 cursor:pointer;font-size:13.5px;transition:all 0.15s ease}
button:hover{filter:brightness(1.1)}
button:active{transform:scale(0.98)}
button.sec{background:#1e2738;color:var(--fg);border:1px solid var(--line)}
button.sec:hover{background:#28344a;border-color:var(--acc)}
button.danger{background:var(--err);color:#fff}
button:disabled{cursor:default;opacity:0.4}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px}
.chk{display:flex;align-items:center;gap:8px;cursor:pointer;font-size:13.5px}
.hint{color:var(--mut);font-size:12.5px;margin:6px 0 0;line-height:1.4}
.bar{position:fixed;left:0;right:0;bottom:0;background:rgba(13,19,31,0.95);backdrop-filter:blur(12px);border-top:1px solid var(--line);
 padding:12px 16px calc(12px + env(safe-area-inset-bottom));display:flex;gap:12px;align-items:center;z-index:30}
#msg{font-size:13.5px;font-weight:500}
.ok{color:var(--ok)}.err{color:var(--err)}.warn{color:var(--warn)}
table{width:100%;border-collapse:collapse;font-size:13px}
td{padding:6px 4px;border-bottom:1px solid var(--line)}
td:first-child{color:var(--mut);width:45%}
.hide{display:none}
.prog-bar{background:#0a0e17;border:1px solid var(--line);border-radius:999px;height:10px;overflow:hidden;width:100%;margin-top:4px}
.prog-fill{height:100%;background:linear-gradient(90deg,var(--acc),var(--purple));border-radius:999px;transition:width 0.3s ease}
.pill{display:inline-block;padding:2px 8px;border-radius:6px;font-size:11px;font-weight:600}
.pill-ok{background:rgba(34,197,94,0.15);color:var(--ok);border:1px solid rgba(34,197,94,0.3)}
.pill-warn{background:rgba(245,158,11,0.15);color:var(--warn);border:1px solid rgba(245,158,11,0.3)}
.stat-val{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:12.5px;color:#fff}
@media (max-width:560px){
  .row label{flex:1 1 100%}
  .row input[type=text],.row input[type=password],.row input[type=number],.row select{flex:1 1 100%}
}
</style></head><body>

<header>
  <h1><span>✈ MeteoPlaneRadar</span><a href="https://github.com/hackra76" target="_blank" rel="noopener noreferrer" class="badge">H4CKR4</a></h1>
  <span class="ver" id="ver"></span>
  <span style="flex:1"></span>
  <select id="uiLang" onchange="setLang(this.value)">
    <option value="2">Slovenčina</option>
    <option value="0">Čeština</option>
    <option value="1">English</option>
  </select>
</header>

<nav class="tabs" id="tabs">
  <button data-tab="tCtl"    class="on" data-i18n="tabCtl">🎛️ Ovládanie</button>
  <button data-tab="tLoc"    data-i18n="tabLoc">📍 Poloha</button>
  <button data-tab="tScr"    data-i18n="tabScr">🖥️ Obrazovky</button>
  <button data-tab="tPlanes" data-i18n="tabPlanes">✈️ Lietadlá</button>
  <button data-tab="tLook"   data-i18n="tabLook">🎨 Vzhľad</button>
  <button data-tab="tHw"     data-i18n="tabHw">⚡ Hardvér</button>
  <button data-tab="tSys"    data-i18n="tabSys">⚙️ Systém</button>
</nav>

<div class="wrap">

<!-- ================= 1. OVLÁDANIE & ŽIVÝ RADAR ================= -->
<section id="tCtl" class="tab">
  <div class="card" id="cardLive">
    <h2 data-i18n="liveRadar">📡 Živý náhľad displeja (Live Screen Mirror)</h2>
    <div style="display:flex;flex-direction:column;align-items:center;gap:12px;margin:8px 0;">
      <div style="position:relative;width:280px;height:280px;border-radius:50%;overflow:hidden;border:2px solid var(--acc);box-shadow:0 0 20px rgba(56,189,248,0.25);background:#05080e;">
        <canvas id="cvDisplay" width="280" height="280" style="display:block;width:280px;height:280px;"></canvas>
      </div>
      <div id="radarTargetBadge" style="background:#182232;border:1px solid var(--line);border-radius:8px;padding:6px 14px;font-size:12.5px;color:var(--acc);font-weight:600" data-i18n="radarActive">
        ✈ Radar aktívny
      </div>
    </div>
  </div>

  <div class="card" id="cardRemote">
    <h2 data-i18n="remote">🎮 Diaľkový ovládač</h2>
    <div class="row" id="scrBtns" style="justify-content:center;gap:6px;"></div>
    <div class="row" style="justify-content:center;gap:8px;margin-top:12px;">
      <button class="sec" onclick="stepScreen(-1)" data-i18n="btnPrev">&#8592; Predchádzajúca</button>
      <button class="sec" onclick="toggleLegendsRemote()" style="background:#223348;color:var(--acc);border-color:var(--acc)" data-i18n="btnDblTap">🔄 Dvojklik / Legenda</button>
      <button class="sec" onclick="stepScreen(1)" data-i18n="btnNext">Nasledujúca &#8594;</button>
    </div>
    <div class="row" style="justify-content:center;gap:10px;margin-top:12px;">
      <span data-i18n="rangeLbl" style="font-weight:600;color:var(--mut)">Rozsah radaru:</span>
      <button class="sec" id="rMinus" onclick="stepRange(-1)" data-i18n="btnDec">&minus; Zmenšiť</button>
      <b id="rangeNow" class="stat-val" style="min-width:70px;text-align:center;font-size:14px;color:var(--acc)">&ndash;</b>
      <button class="sec" id="rPlus" onclick="stepRange(1)" data-i18n="btnInc">+ Zväčšiť</button>
    </div>
    <p class="hint" data-i18n="remoteHint">Rozsah sa mení na obrazovkách Lietadlá, Meteoradar a Taktický radar. Zásah pozastaví automatické striedanie.</p>
  </div>
</section>

<!-- ================= 2. POLOHA & ORIENTÁCIA ================= -->
<section id="tLoc" class="tab hide">
  <div class="card">
    <h2 data-i18n="location">📍 Domovská poloha</h2>
    <div class="row"><label data-i18n="findCity">Vyhľadať mesto</label>
      <input type="text" id="q" style="flex:2 1 200px" placeholder="Hertník, Bardejov, Praha...">
      <button class="sec" onclick="geo()" data-i18n="search">Hľadať</button>
    </div>
    <div class="row hide" id="geoRow"><label data-i18n="found">Nájdené výsledky</label>
      <select id="geoSel" style="flex:2 1 240px" onchange="pickCity()"></select>
    </div>
    <div class="row"><label data-i18n="lat">Zemepisná šírka (°N)</label><input type="number" step="0.0001" id="lat"></div>
    <div class="row"><label data-i18n="lon">Zemepisná dĺžka (°E)</label><input type="number" step="0.0001" id="lon"></div>
    <p class="hint" data-i18n="locHint">Zmena polohy vyžaduje reštart pre prepočet máp a predpovede.</p>
  </div>

  <div class="card">
    <h2 data-i18n="planesView">🧭 Orientácia & Jednotky</h2>
    <div class="row"><label data-i18n="topBearing">Smer hore na radare</label>
      <select id="topBearing">
        <option value="0" data-i18n="tb0">Sever (Sever hore / North-Up)</option>
        <option value="45" data-i18n="tb45">Severovýchod (45°)</option>
        <option value="90" data-i18n="tb90">Východ (90°)</option>
        <option value="135" data-i18n="tb135">Juhovýchod (135°)</option>
        <option value="180" data-i18n="tb180">Juh (180°)</option>
        <option value="225" data-i18n="tb225">Juhozápad (225°)</option>
        <option value="270" data-i18n="tb270">Západ (270°)</option>
        <option value="315" data-i18n="tb315">Severozápad (315°)</option>
      </select>
    </div>
    <div class="row"><label class="chk"><input type="checkbox" id="metric"><span data-i18n="metric">Metrické jednotky (km, km/h, m namiesto NM, kt, ft)</span></label></div>
    <p class="hint" data-i18n="planesViewHint">Nastavte smer podľa toho, kam smeruje váš výhľad. Meteoradar sa zámerne orientuje na sever.</p>
  </div>
</section>

<!-- ================= 3. OBRAZOVKY & RADAR ================= -->
<section id="tScr" class="tab hide">
  <div class="card">
    <h2 data-i18n="screens">🖥️ Zoznam aktívnych obrazoviek</h2>
    <div class="grid">
      <label class="chk"><input type="checkbox" id="sClock"><span data-i18n="scrClock">Hodiny & Astro</span></label>
      <label class="chk"><input type="checkbox" id="sPlanes"><span data-i18n="scrPlanes">Lietadlá radar</span></label>
      <label class="chk"><input type="checkbox" id="sMeteo"><span data-i18n="scrMeteo">Meteoradar</span></label>
      <label class="chk"><input type="checkbox" id="sTactical"><span data-i18n="scrTactical">Taktický radar</span></label>
      <label class="chk"><input type="checkbox" id="sForecast"><span data-i18n="scrForecast">Predpoveď počasia</span></label>
    </div>
    <div class="row" style="margin-top:14px;"><label data-i18n="autoRotate">Automatické striedanie (sekundy, 0 = vypnuté)</label>
      <input type="number" id="autoRotate" min="0" max="3600" step="5">
    </div>
    <p class="hint" data-i18n="rotHint">Striedanie pozastaví potiahnutie prstom alebo prepnutie z prehliadača. Otvorený detail lietadla striedanie pozastaví.</p>
  </div>

  <div class="card">
    <h2 data-i18n="radar">🌧️ Meteoradar & Zobrazenie</h2>
    <div class="row"><label data-i18n="radarSrc">Zdroj radarových dát</label>
      <select id="radarSrc">
        <option value="1" data-i18n="srcRv">RainViewer (Slovensko, Európa a svet)</option>
        <option value="0" data-i18n="srcChmu">ČHMÚ (veľmi ostré dáta, len ČR)</option>
      </select>
    </div>
    <div class="row"><label class="chk"><input type="checkbox" id="showLegends"><span data-i18n="showLegends">Zobrazovať výškovú lištu a farebnú škálu zrážok (alebo poklepanie)</span></label></div>
    <h3 style="margin:16px 0 8px;font-size:14px;color:#94a3b8;" data-i18n="radarWidgets">Prvky radarových máp</h3>
    <div class="grid">
      <label class="chk"><input type="checkbox" id="rTrails"><span data-i18n="rTrails">Trajektórie lietadiel (Trails)</span></label>
      <label class="chk"><input type="checkbox" id="rNearest"><span data-i18n="rNearest">Vektor k najbližšiemu lietadlu</span></label>
      <label class="chk"><input type="checkbox" id="rAirports"><span data-i18n="rAirports">Letiská (Runway ikony)</span></label>
      <label class="chk"><input type="checkbox" id="rRings"><span data-i18n="rRings">Kilometrové kružnice dosahu</span></label>
    </div>
    <p class="hint" data-i18n="radarHint">Mimo územia ČR použite RainViewer, inak meteoradar zostane bez odrazov.</p>
  </div>
</section>

<!-- ================= 4. LIETADLÁ & FILTER ================= -->
<section id="tPlanes" class="tab hide">
  <div class="card">
    <h2 data-i18n="planes">✈️ ADS-B Filtre & Sledovanie</h2>
    <div class="row"><label data-i18n="altMin">Minimálna letová výška (ft)</label><input type="number" id="altMin" step="500"></div>
    <div class="row"><label data-i18n="altMax">Maximálna letová výška (ft)</label><input type="number" id="altMax" step="500"></div>
    <div class="row"><label class="chk"><input type="checkbox" id="onlyCallsign"><span data-i18n="onlyCs">Iba lietadlá so známym volacím znakom (callsign)</span></label></div>
    <div class="row"><label class="chk"><input type="checkbox" id="squawkAlert"><span data-i18n="sqAlert">Zvýrazniť a upozorniť na núdzové squawky (7500 / 7600 / 7700)</span></label></div>
    <div class="row"><label data-i18n="watch">Sledovaný let (Callsign alebo ICAO hex)</label><input type="text" id="watch" placeholder="RYR, WZZ, LZ..."></div>
    <p class="hint" data-i18n="planesHint">Filtre ovplyvňujú len vykresľovanie. Núdzový squawk ani sledované lietadlo filter nikdy neskryje.</p>
  </div>
</section>

<!-- ================= 5. VZHĽAD & JAS ================= -->
<section id="tLook" class="tab hide">
  <div class="card">
    <h2 data-i18n="brightness">☀️ Jas displeja</h2>
    <div class="row"><label data-i18n="briDay">Denný jas</label><input type="range" id="briDay" min="10" max="100"><span id="briDayV" class="stat-val"></span></div>
    <div class="row"><label data-i18n="briNight">Nočný jas</label><input type="range" id="briNight" min="5" max="100"><span id="briNightV" class="stat-val"></span></div>
    <div class="row"><label class="chk"><input type="checkbox" id="nightAuto"><span data-i18n="nightAuto">Prepínať nočný režim automaticky podľa západu/východu slnka</span></label></div>
    <div class="row"><label data-i18n="nightOffset">Posun voči východu/západu (minúty)</label><input type="number" id="nightOffset" min="-120" max="120"></div>
    <p class="hint" data-i18n="liveHint">Zmeny na tejto záložke sa ukladajú okamžite v reálnom čase.</p>
  </div>

  <div class="card">
    <h2 data-i18n="clockHdr">🕒 Ciferník hodín</h2>
    <div class="row"><label data-i18n="clockStyle">Štýl ciferníka</label>
      <select id="clockStyle">
        <option value="0" data-i18n="clkDigital">Digitálny klasický</option>
        <option value="1" data-i18n="clkAnalog">Letecký kokpitový analóg (Aviator)</option>
        <option value="2" data-i18n="clkOrbital">Planetárne prstence (Orbital Gauges)</option>
        <option value="3" data-i18n="clkHud">Stíhací priehľadový displej (Fighter HUD)</option>
        <option value="4" data-i18n="clkRegulator">Astronomický regulátor (Régulateur)</option>
        <option value="5" data-i18n="clkStacked">Vertikálna typografia (Stacked Bold)</option>
        <option value="6" data-i18n="clkMinimal">Minimalistický moderný (Nordic)</option>
      </select>
    </div>
    <div class="row"><label data-i18n="secStyle">Štýl sekundového prstenca</label>
      <select id="secStyle">
        <option value="0" data-i18n="secOff">Vypnuté</option>
        <option value="1" data-i18n="secDots">Bodky (Dots)</option>
        <option value="2" data-i18n="secLine">Plná čiara (Line)</option>
        <option value="3" data-i18n="secComet">Kométa (Comet)</option>
        <option value="4" data-i18n="secRadar">Radarový lúč (Sweep)</option>
        <option value="5" data-i18n="secTicks">Hodinárske indexy (Ticks)</option>
        <option value="6" data-i18n="secOrbit">Satelit na orbite (Orbit)</option>
      </select>
    </div>
    <div class="row"><label data-i18n="clockColor">Farba číslic hodín</label><input type="color" id="clockColor"></div>
    <div class="row"><label data-i18n="secColor">Farba sekundového prstenca</label><input type="color" id="secColor"></div>
    <h3 style="margin:16px 0 8px;font-size:14px;color:#94a3b8;" data-i18n="clockWidgets">Prvky na obrazovke hodín</h3>
    <div class="grid">
      <label class="chk"><input type="checkbox" id="cDate"><span data-i18n="cDate">Dátum</span></label>
      <label class="chk"><input type="checkbox" id="cWx"><span data-i18n="cWx">Počasie & teplota</span></label>
      <label class="chk"><input type="checkbox" id="cWind"><span data-i18n="cWind">Rýchlosť vetra</span></label>
      <label class="chk"><input type="checkbox" id="cMoon"><span data-i18n="cMoon">Fáza mesiaca</span></label>
      <label class="chk"><input type="checkbox" id="cAstro"><span data-i18n="cAstro">24h solárny prstenec</span></label>
      <label class="chk"><input type="checkbox" id="nightClockOnly"><span data-i18n="nightClockOnly">V noci iba Hodiny (zastaviť radary)</span></label>
    </div>
  </div>
</section>

<!-- ================= 6. HARDVÉR & SENZORY ================= -->
<section id="tHw" class="tab hide">
  <div class="card">
    <h2 data-i18n="hwCpu">⚡ ESP32-S3 Procesor & Teplota</h2>
    <table>
      <tr><td data-i18n="hwChipModel">Model čipu:</td><td><span class="stat-val" id="hwCpuModel">-</span> (<span data-i18n="hwRev">Rev</span> <span id="hwCpuRev">-</span>, <span id="hwCpuCores">-</span> <span data-i18n="hwCores">jadrá</span>)</td></tr>
      <tr><td data-i18n="hwCpuFreqLbl">Frekvencia CPU:</td><td><span class="stat-val" id="hwCpuFreq">-</span> MHz</td></tr>
      <tr><td data-i18n="hwCpuTempLbl">Teplota procesora:</td><td><span class="stat-val" id="hwCpuTemp">-</span> °C</td></tr>
      <tr><td data-i18n="hwResetLbl">Dôvod reštartu:</td><td><span class="pill pill-ok" id="hwReset">-</span></td></tr>
      <tr><td data-i18n="hwUptimeLbl">Doba behu (Uptime):</td><td><span class="stat-val" id="hwUptime">-</span></td></tr>
    </table>
  </div>

  <div class="card">
    <h2 data-i18n="hwMem">💾 Pamäť & Úložisko</h2>
    <div style="margin:8px 0;">
      <div style="display:flex;justify-content:space-between;font-size:12.5px;">
        <span data-i18n="hwHeapLbl">Interná RAM (Heap):</span>
        <span class="stat-val" id="hwHeapTxt">-</span>
      </div>
      <div class="prog-bar"><div class="prog-fill" id="hwHeapBar" style="width:0%"></div></div>
    </div>
    <div style="margin:14px 0 8px;">
      <div style="display:flex;justify-content:space-between;font-size:12.5px;">
        <span data-i18n="hwPsramLbl">Octal PSRAM (8 MB):</span>
        <span class="stat-val" id="hwPsramTxt">-</span>
      </div>
      <div class="prog-bar"><div class="prog-fill" id="hwPsramBar" style="width:0%"></div></div>
    </div>
    <table>
      <tr><td data-i18n="hwFlashLbl">Flash pamäť:</td><td><span class="stat-val" id="hwFlashSize">-</span> MB @ <span id="hwFlashSpd">-</span> MHz</td></tr>
    </table>
  </div>

  <div class="card">
    <h2 data-i18n="hwSensors">🧭 6-Axis IMU Senzor (QMI8658)</h2>
    <table>
      <tr><td data-i18n="hwImuLbl">Stav senzora:</td><td><span class="pill pill-ok" id="hwImuState" data-i18n="hwImuActive">Aktívny (I2C 0x6B)</span></td></tr>
      <tr><td data-i18n="hwAxLbl">Akcelerometer (g):</td><td><span class="stat-val">X: <span id="hwAx">0</span> | Y: <span id="hwAy">0</span> | Z: <span id="hwAz">0</span></span></td></tr>
      <tr><td data-i18n="hwGxLbl">Gyroskop (°/s):</td><td><span class="stat-val">X: <span id="hwGx">0</span> | Y: <span id="hwGy">0</span> | Z: <span id="hwGz">0</span></span></td></tr>
      <tr><td data-i18n="hwTiltLbl">Náklon (Pitch / Roll):</td><td><span class="stat-val">Pitch: <span id="hwPitch">0</span>° | Roll: <span id="hwRoll">0</span>°</span></td></tr>
      <tr><td data-i18n="hwDblTapLbl">Gesto poklepania:</td><td><span class="pill pill-ok" data-i18n="hwDblTapOn">Double-Tap detekcia zapnutá</span></td></tr>
    </table>
  </div>

  <div class="card">
    <h2 data-i18n="hwRtc">⏱️ Hardware RTC Hodiny (PCF85063)</h2>
    <table>
      <tr><td data-i18n="hwRtcLbl">Stav RTC čipu:</td><td><span class="pill pill-ok" id="hwRtcState">Aktívny (I2C 0x51)</span></td></tr>
      <tr><td data-i18n="hwRtcTimeLbl">Čas v RTC čipe:</td><td><span class="stat-val" id="hwRtcTime">-</span></td></tr>
    </table>
    <div style="margin-top:12px;display:flex;gap:8px;flex-wrap:wrap;">
      <button class="sec" onclick="syncRtcNtp()" data-i18n="btnSyncNtp">🌐 Synchronizovať s NTP</button>
      <button class="sec" onclick="syncRtcBrowser()" data-i18n="btnSyncBrowser">💻 Odoslať čas z prehliadača</button>
    </div>
  </div>

  <div class="card">
    <h2 data-i18n="hwI2c">🔍 I2C Zbernica (Bus Inspector)</h2>
    <table id="hwI2cTable">
      <tr><th>Adresa</th><th>Názov komponentu</th></tr>
      <tr><td><code>0x51</code></td><td>Hardware RTC (PCF85063)</td></tr>
      <tr><td><code>0x6B</code></td><td>6-osové IMU (QMI8658)</td></tr>
    </table>
  </div>

  <div class="card">
    <h2 data-i18n="hwPeripherals">🔌 Periférie & Displej</h2>
    <table>
      <tr><td data-i18n="hwDispLbl">Displej:</td><td><span class="stat-val" id="hwDisp">ST7701 (480x480 RGB 16-bit)</span></td></tr>
      <tr><td data-i18n="hwTouchLbl">Dotykový panel:</td><td><span class="stat-val" id="hwTouch">CST820 (I2C 0x15)</span></td></tr>
      <tr><td data-i18n="hwExpLbl">I/O Expandér:</td><td><span class="stat-val" id="hwExp">TCA9554 (I2C 0x20)</span></td></tr>
    </table>
  </div>
</section>

<!-- ================= 7. SYSTÉM & SIEŤ ================= -->
<section id="tSys" class="tab hide">
  <div class="card" id="cardWifi">
    <h2 data-i18n="wifi">📶 WiFi Pripojenie</h2>
    <p class="hint" id="wifiNow"></p>
    <div class="row"><label data-i18n="network">Názov siete (SSID)</label>
      <select id="ssid" style="flex:2 1 200px"></select>
      <button class="sec" onclick="scan()" data-i18n="scan">Vyhľadať</button>
    </div>
    <div class="row"><label data-i18n="password">Heslo siete</label><input type="password" id="wpass" style="flex:2 1 200px"></div>
    <p class="hint" id="wifiHintTxt"></p>
    <div style="margin-top:12px;"><button onclick="saveWifi()" data-i18n="connect">Pripojiť k sieti</button></div>
    <table style="margin-top:14px;">
      <tr><td data-i18n="netIpLbl">IP adresa:</td><td><span class="stat-val" id="netIp">-</span></td></tr>
      <tr><td data-i18n="netRssiLbl">Sila signálu (RSSI):</td><td><span class="stat-val" id="netRssi">-</span></td></tr>
      <tr><td data-i18n="netMacLbl">MAC adresa:</td><td><span class="stat-val" id="netMac">-</span></td></tr>
    </table>
  </div>

  <div class="card">
    <h2 data-i18n="system">🔒 Zabezpečenie správcu</h2>
    <div class="row"><label data-i18n="adminPass">Súčasné heslo</label><input type="password" id="oldPass"></div>
    <div class="row"><label data-i18n="newPass">Nové heslo</label><input type="password" id="newPass"></div>
    <p class="hint" id="pwState"></p>
    <p class="hint" data-i18n="passHint">Súčasné heslo je potrebné pre aktualizáciu, import, reset aj pre zmenu hesla. Prázdne nové heslo nič nemení; jedna medzera ochranu zruší.</p>
  </div>

  <div class="card">
    <h2 data-i18n="statusHdr">🛠️ Údržba & Záloha</h2>
    <div class="row" style="gap:8px;flex-wrap:wrap;">
      <a href="/update" style="text-decoration:none"><button type="button" data-i18n="fwUpdate">Aktualizácia firmvéru</button></a>
      <a href="/api/export" download="meteoplaneradar.json" style="text-decoration:none"><button type="button" class="sec" data-i18n="export">Export nastavení</button></a>
      <button type="button" class="sec" onclick="$('imp').click()" data-i18n="import">Import nastavení</button>
      <input type="file" id="imp" class="hide" accept="application/json" onchange="importCfg(this)">
      <button type="button" class="sec" onclick="doReboot()" data-i18n="reboot">Reštartovať</button>
      <button type="button" class="danger" onclick="doReset()" data-i18n="factory">Továrenský reset</button>
    </div>
  </div>
</section>

</div>

<div class="bar hide" id="saveBar">
  <button onclick="save()" data-i18n="save">💾 Uložiť nastavenia</button>
  <span id="msg"></span>
</div>

<script>
const D={
 cs:{
  tabCtl:"🎛️ Ovládání",tabLoc:"📍 Poloha",tabScr:"🖥️ Obrazovky",tabPlanes:"✈️ Letadla",tabLook:"🎨 Vzhled",tabHw:"⚡ Hardware",tabSys:"⚙️ Systém",
  liveRadar:"📡 Živý radarový stream",radarActive:"✈ Radar aktivní",radarNoPlanes:"✈ 0 letadel v dosahu",
  remote:"🎮 Dálkové ovládání",rangeLbl:"Rozsah radaru:",
  btnPrev:"← Předchozí",btnDblTap:"🔄 Dvojklik / Legenda",btnNext:"Následující →",btnDec:"− Přiblížit (− km)",btnInc:"+ Oddálit (+ km)",
  remoteHint:"Rozsah se mění na obrazovkách Letadla, Meteoradar a Taktický radar. Zásah pozastaví automatické střídání.",
  location:"📍 Domovská poloha",findCity:"Vyhledat město",search:"Hledat",found:"Nalezené výsledky",lat:"Zeměpisná šířka (°N)",lon:"Zeměpisná délka (°E)",
  locHint:"Změna polohy vyžaduje restart pro přepočet map a předpovědi.",
  planesView:"🧭 Orientace & Jednotky",topBearing:"Směr nahoře na radaru",metric:"Metrické jednotky (km, km/h, m místo NM, kt, ft)",
  tb0:"Sever (Sever nahoře / North-Up)",tb45:"Severovýchod (45°)",tb90:"Východ (90°)",tb135:"Jihovýchod (135°)",tb180:"Jih (180°)",tb225:"Jihozápad (225°)",tb270:"Západ (270°)",tb315:"Severozápad (315°)",
  planesViewHint:"Nastavte směr podle toho, kam se díváte z okna. Meteoradar je orientován na sever.",
  screens:"🖥️ Seznam aktivních obrazovek",scrClock:"Hodiny & Astro",scrPlanes:"Letadla radar",scrMeteo:"Meteoradar",scrTactical:"Taktický radar",scrForecast:"Předpověď počasí",scrSettings:"Nastavení",
  autoRotate:"Automatické střídání (sekundy, 0 = vypnuto)",
  rotHint:"Střídání pozastaví potažení prstem nebo přepnutí z prohlížeče. Otevřený detail letadla střídání drží.",
  radar:"🌧️ Meteoradar & Zobrazení",radarSrc:"Zdroj radarových dat",srcRv:"RainViewer (Evropa a svět)",srcChmu:"ČHMÚ (velmi ostrá data, jen ČR)",
  showLegends:"Zobrazovat výškovou lištu a stupnici srážek (nebo poklepání)",
  radarHint:"Mimo ČR nemá ČHMÚ data — použijte RainViewer.",
  planes:"✈️ ADS-B Filtry & Sledování",altMin:"Minimální letová výška (ft)",altMax:"Maximální letová výška (ft)",onlyCs:"Jen letadla s volacím znakem (callsign)",
  sqAlert:"Zvýraznit a upozornit na nouzové squawky (7500/7600/7700)",watch:"Sledovaný let (Callsign nebo ICAO hex)",
  planesHint:"Filtry se týkají jen kreslení. Nouzový squawk ani sledované letadlo neschovají.",
  brightness:"☀️ Jas displeje",briDay:"Denní jas",briNight:"Noční jas",nightAuto:"Přepínat noční režim automaticky podle slunce",
  nightOffset:"Posun proti východu/západu (minuty)",clockHdr:"🕒 Ciferník hodin",secStyle:"Styl vteřinového prstence",
  secOff:"Vypnuto",secDots:"Tečky (Dots)",secLine:"Plná čára (Line)",secComet:"Kometa (Comet)",
   secRadar:"Radarový paprsek (Sweep)",secTicks:"Hodinářské indexy (Ticks)",secOrbit:"Satelit na orbitě (Orbit)",
   clockColor:"Barva číslic hodin",secColor:"Barva vteřinového prstence",
   clockStyle:"Styl ciferníku",clkDigital:"Digitální klasický",clkAnalog:"Letecký kokpitový analog (Aviator)",clkOrbital:"Planetární prstence (Orbital Gauges)",clkHud:"Stíhací průhledový displej (Fighter HUD)",clkRegulator:"Astronomický regulátor (Régulateur)",clkStacked:"Vertikální typografie (Stacked Bold)",clkMinimal:"Minimalistický moderní (Nordic)",
   clockWidgets:"Prvky na obrazovce hodin",cDate:"Datum",cWx:"Počasí & teplota",cWind:"Rychlost větru",cMoon:"Fáze měsíce",cAstro:"24h solární prstenec",nightClockOnly:"V noci pouze Hodiny (zastavit radary)",
   radarWidgets:"Prvky radarových map",rTrails:"Trajektorie letadel (Trails)",rNearest:"Vektor k nejbližšímu letadlu",rAirports:"Letiště (Runway ikony)",rRings:"Kilometrové kružnice dosahu",
   hwRtc:"⏱️ Hardware RTC Hodiny (PCF85063)",hwRtcLbl:"Stav RTC čipu:",hwRtcTimeLbl:"Čas v RTC čipu:",btnSyncNtp:"🌐 Synchronizovat s NTP",btnSyncBrowser:"💻 Odeslat čas z prohlížeče",hwI2c:"🔍 I2C Sběrnice (Bus Inspector)",
  liveHint:"Změny na této záložce se ukládají okamžitě v reálném čase.",
  hwCpu:"⚡ ESP32-S3 Procesor & Teplota",hwChipModel:"Model čipu:",hwRev:"Rev",hwCores:"jádra",hwCpuFreqLbl:"Frekvence CPU:",hwCpuTempLbl:"Teplota procesoru:",hwResetLbl:"Důvod restartu:",hwUptimeLbl:"Doba běhu (Uptime):",
  hwMem:"💾 Paměť & Úložiště",hwHeapLbl:"Interní RAM (Heap):",hwPsramLbl:"Octal PSRAM (8 MB):",hwFlashLbl:"Flash paměť:",
  hwSensors:"🧭 6-Axis IMU Senzor (QMI8658)",hwImuLbl:"Stav senzoru:",hwImuActive:"Aktivní (I2C 0x6B)",hwAxLbl:"Akcelerometr (g):",hwGxLbl:"Gyroskop (°/s):",hwTiltLbl:"Náklon (Pitch / Roll):",hwDblTapLbl:"Gesto poklepání:",hwDblTapOn:"Double-Tap detekce zapnuta",
  hwPeripherals:"🔌 Periferie & Displej",hwDispLbl:"Displej:",hwTouchLbl:"Dotykový panel:",hwExpLbl:"I/O Expandér:",
  wifi:"📶 WiFi Připojení",network:"Název sítě (SSID)",password:"Heslo sítě",scan:"Vyhledat",connect:"Připojit k síti",
  netIpLbl:"IP adresa:",netRssiLbl:"Síla signálu (RSSI):",netMacLbl:"MAC adresa:",
  wifiHint:"Po uložení se zařízení připojí a přístupový bod zmizí.",
  wifiHintSta:"Změna sítě přeruší spojení. Při neúspěchu zařízení vytvoří vlastní síť MeteoPlaneRadar.",
  wifiNow:"Připojeno k síti",system:"🔒 Zabezpečení správce",adminPass:"Současné heslo",newPass:"Nové heslo",
  passHint:"Současné heslo je potřeba pro aktualizaci, import, reset i pro změnu hesla. Prázdné nové heslo nic nemění; jedna mezera ochranu zruší.",
  statusHdr:"🛠️ Údržba & Záloha",fwUpdate:"Aktualizace firmwaru",export:"Export nastavení",import:"Import nastavení",reboot:"Restartovat",factory:"Tovární reset",save:"💾 Uložit nastavení",
  pwNone:"Zatím není nastavené žádné heslo — aktualizace, import a reset jsou otevřené.",pwSet:"Heslo je nastavené.",
  wrongPass:"Chybné heslo",doneReboot:"Hotovo. Restartuji...",importOk:"Import úspěšný. Restartuji...",
  autoSaved:"Uloženo",saved:"Uloženo",failed:"Nepovedlo se",searching:"Hledám…",nothing:"Nic nenalezeno",
  disabled:"Obrazovka je vypnutá",confirmReset:"Opravdu smazat všechna nastavení včetně WiFi?"
 },
 sk:{
  tabCtl:"🎛️ Ovládanie",tabLoc:"📍 Poloha",tabScr:"🖥️ Obrazovky",tabPlanes:"✈️ Lietadlá",tabLook:"🎨 Vzhľad",tabHw:"⚡ Hardvér",tabSys:"⚙️ Systém",
  liveRadar:"📡 Živý radarový stream",radarActive:"✈ Radar aktívny",radarNoPlanes:"✈ 0 lietadiel v dosahu",
  remote:"🎮 Diaľkový ovládač",rangeLbl:"Rozsah radaru:",
  btnPrev:"← Predchádzajúca",btnDblTap:"🔄 Dvojklik / Legenda",btnNext:"Nasledujúca →",btnDec:"− Priblížiť (− km)",btnInc:"+ Oddialiť (+ km)",
  remoteHint:"Rozsah sa mení na obrazovkách Lietadlá, Meteoradar a Taktický radar. Zásah pozastaví automatické striedanie.",
  location:"📍 Domovská poloha",findCity:"Vyhľadať mesto",search:"Hľadať",found:"Nájdené výsledky",lat:"Zemepisná šírka (°N)",lon:"Zemepisná dĺžka (°E)",
  locHint:"Zmena polohy vyžaduje reštart pre prepočet máp a predpovede.",
  planesView:"🧭 Orientácia & Jednotky",topBearing:"Smer hore na radare",metric:"Metrické jednotky (km, km/h, m namiesto NM, kt, ft)",
  tb0:"Sever (Sever hore / North-Up)",tb45:"Severovýchod (45°)",tb90:"Východ (90°)",tb135:"Juhovýchod (135°)",tb180:"Juh (180°)",tb225:"Juhozápad (225°)",tb270:"Západ (270°)",tb315:"Severozápad (315°)",
  planesViewHint:"Nastavte smer podľa toho, kam smeruje váš výhľad. Meteoradar sa zámerne orientuje na sever.",
  screens:"🖥️ Zoznam aktívnych obrazoviek",scrClock:"Hodiny & Astro",scrPlanes:"Lietadlá radar",scrMeteo:"Meteoradar",scrTactical:"Taktický radar",scrForecast:"Predpoveď počasia",scrSettings:"Nastavenia",
  autoRotate:"Automatické striedanie (sekundy, 0 = vypnuté)",
  rotHint:"Striedanie pozastaví potiahnutie prstom alebo prepnutie z prehliadača. Otvorený detail lietadla striedanie pozastaví.",
  radar:"🌧️ Meteoradar & Zobrazenie",radarSrc:"Zdroj radarových dát",srcRv:"RainViewer (Slovensko, Európa a svet)",srcChmu:"ČHMÚ (veľmi ostré dáta, len ČR)",
  showLegends:"Zobrazovať výškovú lištu a farebnú škálu zrážok (alebo poklepanie)",
  radarHint:"Mimo územia ČR použite RainViewer, inak meteoradar zostane bez odrazov.",
  planes:"✈️ ADS-B Filtre & Sledovanie",altMin:"Minimálna letová výška (ft)",altMax:"Maximálna letová výška (ft)",onlyCs:"Iba lietadlá so známym volacím znakom (callsign)",
  sqAlert:"Zvýrazniť a upozorniť na núdzové squawky (7500 / 7600 / 7700)",watch:"Sledovaný let (Callsign alebo ICAO hex)",
  planesHint:"Filtre ovplyvňujú len vykresľovanie. Núdzový squawk ani sledované lietadlo filter nikdy neskryje.",
  brightness:"☀️ Jas displeja",briDay:"Denný jas",briNight:"Nočný jas",nightAuto:"Prepínať nočný režim automaticky podľa západu/východu slnka",
  nightOffset:"Posun voči východu/západu (minúty)",clockHdr:"🕒 Ciferník hodín",secStyle:"Štýl sekundového prstenca",
  secOff:"Vypnuté",secDots:"Bodky (Dots)",secLine:"Plná čiara (Line)",secComet:"Kométa (Comet)",
   secRadar:"Radarový lúč (Sweep)",secTicks:"Hodinárske indexy (Ticks)",secOrbit:"Satelit na orbite (Orbit)",
   clockColor:"Farba číslic hodín",secColor:"Farba sekundového prstenca",
   clockStyle:"Štýl ciferníka",clkDigital:"Digitálny klasický",clkAnalog:"Letecký kokpitový analóg (Aviator)",clkOrbital:"Planetárne prstence (Orbital Gauges)",clkHud:"Stíhací priehľadový displej (Fighter HUD)",clkRegulator:"Astronomický regulátor (Régulateur)",clkStacked:"Vertikálna typografia (Stacked Bold)",clkMinimal:"Minimalistický moderný (Nordic)",
   clockWidgets:"Prvky na obrazovke hodín",cDate:"Dátum",cWx:"Počasie & teplota",cWind:"Rýchlosť vetra",cMoon:"Fáza mesiaca",cAstro:"24h solárny prstenec",nightClockOnly:"V noci iba Hodiny (zastavit radary)",
   radarWidgets:"Prvky radarových máp",rTrails:"Trajektórie lietadiel (Trails)",rNearest:"Vektor k najbližšiemu lietadlu",rAirports:"Letiská (Runway ikony)",rRings:"Kilometrové kružnice dosahu",
   hwRtc:"⏱️ Hardware RTC Hodiny (PCF85063)",hwRtcLbl:"Stav RTC čipu:",hwRtcTimeLbl:"Čas v RTC čipe:",btnSyncNtp:"🌐 Synchronizovať s NTP",btnSyncBrowser:"💻 Odoslať čas z prehliadača",hwI2c:"🔍 I2C Zbernica (Bus Inspector)",
  liveHint:"Zmeny na tejto záložke sa ukladajú okamžite v reálnom čase.",
  hwCpu:"⚡ ESP32-S3 Procesor & Teplota",hwChipModel:"Model čipu:",hwRev:"Rev",hwCores:"jadrá",hwCpuFreqLbl:"Frekvencia CPU:",hwCpuTempLbl:"Teplota procesora:",hwResetLbl:"Dôvod reštartu:",hwUptimeLbl:"Doba behu (Uptime):",
  hwMem:"💾 Pamäť & Úložisko",hwHeapLbl:"Interná RAM (Heap):",hwPsramLbl:"Octal PSRAM (8 MB):",hwFlashLbl:"Flash pamäť:",
  hwSensors:"🧭 6-Axis IMU Senzor (QMI8658)",hwImuLbl:"Stav senzora:",hwImuActive:"Aktívny (I2C 0x6B)",hwAxLbl:"Akcelerometer (g):",hwGxLbl:"Gyroskop (°/s):",hwTiltLbl:"Náklon (Pitch / Roll):",hwDblTapLbl:"Gesto poklepania:",hwDblTapOn:"Double-Tap detekcia zapnutá",
  hwPeripherals:"🔌 Periférie & Displej",hwDispLbl:"Displej:",hwTouchLbl:"Dotykový panel:",hwExpLbl:"I/O Expandér:",
  wifi:"📶 WiFi Pripojenie",network:"Názov siete (SSID)",password:"Heslo siete",scan:"Vyhľadať",connect:"Pripojiť k sieti",
  netIpLbl:"IP adresa:",netRssiLbl:"Sila signálu (RSSI):",netMacLbl:"MAC adresa:",
  wifiHint:"Po uložení sa zariadenie pripojí a prístupový bod zmizne.",
  wifiHintSta:"Zmena siete preruší spojenie. Pri neúspěchu zariadenie vytvorí vlastnú sieť MeteoPlaneRadar.",
  wifiNow:"Pripojené k sieti",system:"🔒 Zabezpečenie správcu",adminPass:"Súčasné heslo",newPass:"Nové heslo",
  passHint:"Súčasné heslo je potrebné pre aktualizáciu, import, reset aj pre zmenu hesla. Prázdne nové heslo nič nemení; jedna medzera ochranu zruší.",
  statusHdr:"🛠️ Údržba & Záloha",fwUpdate:"Aktualizácia firmvéru",export:"Export nastavení",import:"Import nastavení",reboot:"Reštartovať",factory:"Továrenský reset",save:"💾 Uložiť nastavenia",
  pwNone:"Zatiaľ nie je nastavené žiadne heslo — aktualizácia, import a reset sú otvorené.",pwSet:"Heslo je nastavené.",
  wrongPass:"Chybné heslo",doneReboot:"Hotovo. Reštartujem...",importOk:"Import úspešný. Reštartujem...",
  autoSaved:"Uložené",saved:"Uložené",failed:"Nepodarilo sa",searching:"Hľadám…",nothing:"Nič sa nenašlo",
  disabled:"Obrazovka je vypnutá",confirmReset:"Naozaj vymazať všetky nastavenia vrátane WiFi?"
 },
 en:{
  tabCtl:"🎛️ Control",tabLoc:"📍 Location",tabScr:"🖥️ Screens",tabPlanes:"✈️ Aircraft",tabLook:"🎨 Appearance",tabHw:"⚡ Hardware",tabSys:"⚙️ System",
  liveRadar:"📡 Live Radar Stream",radarActive:"✈ Radar active",radarNoPlanes:"✈ 0 aircraft in range",
  remote:"🎮 Remote Control",rangeLbl:"Radar Range:",
  btnPrev:"← Previous",btnDblTap:"🔄 Double-Tap / Legend",btnNext:"Next →",btnDec:"− Zoom In (− km)",btnInc:"+ Zoom Out (+ km)",
  remoteHint:"Range applies to Aircraft, Weather and Tactical screens. Manual action pauses auto cycling.",
  location:"📍 Home Location",findCity:"Search town",search:"Search",found:"Found results",lat:"Latitude (°N)",lon:"Longitude (°E)",
  locHint:"Changing location requires a reboot to recalculate maps and forecast.",
  planesView:"🧭 Orientation & Units",topBearing:"Radar top orientation",metric:"Metric units (km, km/h, m instead of NM, kt, ft)",
  tb0:"North (North-Up)",tb45:"Northeast (45°)",tb90:"East (90°)",tb135:"Southeast (135°)",tb180:"South (180°)",tb225:"Southwest (225°)",tb270:"West (270°)",tb315:"Northwest (315°)",
  planesViewHint:"Set the bearing you are looking out of your window. Weather radar is North-Up.",
  screens:"🖥️ Active Screens",scrClock:"Clock & Astro",scrPlanes:"Aircraft radar",scrMeteo:"Weather radar",scrTactical:"Tactical radar",scrForecast:"Weather forecast",scrSettings:"Settings",
  autoRotate:"Auto cycle (seconds, 0 = off)",
  rotHint:"Cycling is paused by swiping or browser actions. An open aircraft detail keeps cycling paused.",
  radar:"🌧️ Weather Radar & Feeds",radarSrc:"Radar Data Source",srcRv:"RainViewer (Europe & Global)",srcChmu:"CHMU (high-res, Czechia only)",
  showLegends:"Show altitude bar and precipitation color scales (or double-tap)",
  radarHint:"Outside Czechia use RainViewer feed.",
  planes:"✈️ ADS-B Filters & Watchlist",altMin:"Min altitude (ft)",altMax:"Max altitude (ft)",onlyCs:"Only aircraft with callsign",
  sqAlert:"Highlight emergency squawks (7500/7600/7700)",watch:"Watched flight (Callsign or ICAO hex)",
  planesHint:"Filters only affect drawing. Emergencies and watched flights are never hidden.",
  brightness:"☀️ Display Brightness",briDay:"Day brightness",briNight:"Night brightness",nightAuto:"Automatic night mode with sun position",
  nightOffset:"Offset from sunset/sunrise (minutes)",clockHdr:"🕒 Clock Face",secStyle:"Seconds ring style",
  secOff:"Off",secDots:"Dots",secLine:"Line",secComet:"Comet",
   secRadar:"Radar sweep",secTicks:"Swiss ticks",secOrbit:"Orbiting satellite",
   clockColor:"Clock digits colour",secColor:"Seconds ring colour",
   clockStyle:"Clock face style",clkDigital:"Classic Digital",clkAnalog:"Aviator Cockpit Analog",clkOrbital:"Orbital Gauges",clkHud:"Fighter HUD",clkRegulator:"Observatory Régulateur",clkStacked:"Stacked Bold Typography",clkMinimal:"Nordic Minimal",
   clockWidgets:"Clock screen widgets",cDate:"Date",cWx:"Weather & temp",cWind:"Wind speed",cMoon:"Moon phase",cAstro:"24h solar arc",nightClockOnly:"Night: Clock only (pause radars)",
   radarWidgets:"Radar map widgets",rTrails:"Flight trails (breadcrumbs)",rNearest:"Vector to nearest aircraft",rAirports:"Airports (runway icons)",rRings:"Range rings",
   hwRtc:"⏱️ Hardware RTC Clock (PCF85063)",hwRtcLbl:"RTC chip status:",hwRtcTimeLbl:"RTC hardware time:",btnSyncNtp:"🌐 Sync with NTP",btnSyncBrowser:"💻 Sync from browser",hwI2c:"🔍 I2C Bus Inspector",
  liveHint:"Changes on this tab are saved instantly.",
  hwCpu:"⚡ ESP32-S3 CPU & Temp",hwChipModel:"Chip model:",hwRev:"Rev",hwCores:"cores",hwCpuFreqLbl:"CPU frequency:",hwCpuTempLbl:"CPU temperature:",hwResetLbl:"Reset reason:",hwUptimeLbl:"Uptime:",
  hwMem:"💾 Memory & Storage",hwHeapLbl:"Internal RAM (Heap):",hwPsramLbl:"Octal PSRAM (8 MB):",hwFlashLbl:"Flash memory:",
  hwSensors:"🧭 6-Axis IMU Sensor (QMI8658)",hwImuLbl:"Sensor state:",hwImuActive:"Active (I2C 0x6B)",hwAxLbl:"Accelerometer (g):",hwGxLbl:"Gyroscope (°/s):",hwTiltLbl:"Tilt (Pitch / Roll):",hwDblTapLbl:"Double-Tap gesture:",hwDblTapOn:"Double-Tap detection active",
  hwPeripherals:"🔌 Peripherals & Display",hwDispLbl:"Display:",hwTouchLbl:"Touch panel:",hwExpLbl:"I/O Expander:",
  wifi:"📶 WiFi Connection",network:"Network Name (SSID)",password:"Network Password",scan:"Scan",connect:"Connect",
  netIpLbl:"IP address:",netRssiLbl:"Signal strength (RSSI):",netMacLbl:"MAC address:",
  wifiHint:"After saving the device connects and the AP closes.",
  wifiHintSta:"Changing WiFi disconnects this page. If connection fails, MeteoPlaneRadar AP will be restored.",
  wifiNow:"Connected to",system:"🔒 Admin Security",adminPass:"Current password",newPass:"New password",
  passHint:"Current password is required for updates, import, reset and password changes.",
  statusHdr:"🛠️ Maintenance & Backup",fwUpdate:"Firmware update",export:"Export settings",import:"Import settings",reboot:"Reboot",factory:"Factory reset",save:"💾 Save Settings",
  pwNone:"No password set — update, import and reset are open.",pwSet:"Password is set.",
  wrongPass:"Wrong password",doneReboot:"Done. Rebooting...",importOk:"Import successful. Rebooting...",
  autoSaved:"Saved",saved:"Saved",failed:"Failed",searching:"Searching…",nothing:"Nothing found",
  disabled:"Screen is disabled",confirmReset:"Really erase all settings and reset WiFi?"
 }
};

let L="sk", CFG={}, TAB="tCtl";
const $=id=>document.getElementById(id);

function setLang(v){
 L=(v==1||v=="en")?"en":((v==0||v=="cs")?"cs":"sk");
 $("uiLang").value=(L=="en")?"1":((L=="cs")?"0":"2");
 document.documentElement.lang=L;
 document.querySelectorAll("[data-i18n]").forEach(e=>{
  const k=e.dataset.i18n;
  const t=D[L][k];
  if(t!==undefined){
   if(e.tagName==="INPUT" && (e.type==="button"||e.type==="submit")){
    e.value=t;
   } else {
    e.textContent=t;
   }
  }
 });
 pwState();status();updateHardware();updateLiveTelemetry();
}

function showTab(id){
 TAB=id;
 document.querySelectorAll("section.tab").forEach(s=>s.classList.toggle("hide",s.id!=id));
 document.querySelectorAll("#tabs button").forEach(b=>b.classList.toggle("on",b.dataset.tab==id));
 const isControlOrHw = (id=="tCtl" || id=="tHw");
 $("saveBar").classList.toggle("hide", isControlOrHw);
 if(id=="tHw") updateHardware();
 if(id=="tCtl") updateLiveTelemetry();
 window.scrollTo(0,0);
}
document.querySelectorAll("#tabs button").forEach(b=>b.onclick=()=>showTab(b.dataset.tab));

function msg(t,c){$("msg").textContent=t;$("msg").className=c||"";setTimeout(()=>{$("msg").textContent=""},4000);}

const SCR=[["scrClock",0],["scrPlanes",1],["scrMeteo",2],["scrTactical",3],["scrForecast",4],["scrSettings",5]];
function drawScrBtns(cur,enabled){
 $("scrBtns").innerHTML=SCR.map(([k,i])=>{
  const on=enabled?enabled[i]:true;
  const cls=(i==cur)?"":"sec";
  const dis=on?"":" disabled style='opacity:.4'";
  const label=D[L][k]||(k==="scrSettings"?"Settings":k);
  return "<button class='"+cls+"'"+dis+" onclick='goScreen("+i+")'>"+label+"</button>";
 }).join(" ");
}

async function post(u,b){try{const r=await fetch(u,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(b)});
 if(r.status==409)msg(D[L].disabled,"warn");else if(!r.ok)msg(D[L].failed,"err");
 await status();}catch(e){msg(D[L].failed,"err")}}

function goScreen(i){post("/api/screen",{index:i});}
function stepScreen(d){post("/api/screen",{step:d});}
function stepRange(d){post("/api/range",{step:d});}
function toggleLegendsRemote(){post("/api/toggle-legends",{});}

// --- Live Display Vector Renderer & Telemetry ---
function drawVectorPreview(d) {
 const cv = $("cvDisplay"); if(!cv) return;
 const ctx = cv.getContext("2d");
 const W = cv.width, H = cv.height, CX = W/2, CY = H/2, R = W/2 - 6;
 ctx.clearRect(0,0,W,H);

 // Dark background
 ctx.fillStyle = "#05080e";
 ctx.beginPath(); ctx.arc(CX,CY,CX,0,Math.PI*2); ctx.fill();

 const scr = d.screen !== undefined ? d.screen : 1;

 if (scr === 0) {
  // === SCREEN 0: HODINY & ASTRO ===
  const sec = d.sec || 0;
  const secR = R - 2;
  for (let s = 0; s < 60; s++) {
   const ang = (s * 6 - 90) * Math.PI / 180;
   const x = CX + secR * Math.cos(ang), y = CY + secR * Math.sin(ang);
   ctx.fillStyle = (s <= sec) ? "#38bdf8" : "#1e293b";
   ctx.beginPath(); ctx.arc(x, y, (s === sec) ? 2.5 : 1.2, 0, Math.PI*2); ctx.fill();
  }

  // Solar Twilight Arc
  const arcR = R - 14;
  ctx.lineWidth = 3;
  ctx.strokeStyle = "#0284c7"; ctx.beginPath(); ctx.arc(CX, CY, arcR, -0.6 * Math.PI, 0.4 * Math.PI); ctx.stroke();
  ctx.strokeStyle = "#f59e0b"; ctx.beginPath(); ctx.arc(CX, CY, arcR, 0.4 * Math.PI, 0.55 * Math.PI); ctx.stroke();
  ctx.strokeStyle = "#4338ca"; ctx.beginPath(); ctx.arc(CX, CY, arcR, 0.55 * Math.PI, 0.7 * Math.PI); ctx.stroke();
  ctx.strokeStyle = "#0f172a"; ctx.beginPath(); ctx.arc(CX, CY, arcR, 0.7 * Math.PI, 1.4 * Math.PI); ctx.stroke();

  // Screen dots inside
  ctx.fillStyle = "#ffffff"; ctx.beginPath(); ctx.arc(CX, 35, 3, 0, Math.PI*2); ctx.fill();
  ctx.strokeStyle = "#475569";
  for (let di = -2; di <= 2; di++) {
   if (di !== 0) { ctx.beginPath(); ctx.arc(CX + di * 12, 35, 2.5, 0, Math.PI*2); ctx.stroke(); }
  }

  // Date text
  ctx.fillStyle = "#94a3b8"; ctx.font = "bold 11px system-ui, sans-serif"; ctx.textAlign = "center";
  const wd = ["NEDEĽA","PONDELOK","UTOROK","STREDA","ŠTVRTOK","PIATOK","SOBOTA"][d.wday||0];
  ctx.fillText((wd + " " + (d.mday||1) + "." + (d.mon||1) + ".").toUpperCase(), CX, 68);

  // Big Digital Clock
  ctx.fillStyle = "#ffffff"; ctx.font = "bold 44px monospace, system-ui";
  const tStr = d.time ? d.time.substring(0, 5) : "--:--";
  ctx.fillText(tStr, CX, 130);

  // Seconds sub-digits
  ctx.fillStyle = "#38bdf8"; ctx.font = "bold 15px monospace";
  ctx.fillText(":" + String(sec).padStart(2, '0'), CX, 154);

  // Temperature & Weather
  if (d.temp !== undefined) {
   ctx.fillStyle = "#f8fafc"; ctx.font = "bold 16px system-ui";
   ctx.fillText(d.temp + " °C", CX, 192);
  }

  // Moon icon & Illumination
  const moonEmojis = ["🌑","🌒","🌓","🌔","🌕","🌖","🌗","🌘"];
  const mEmoji = moonEmojis[d.moonPhase !== undefined ? d.moonPhase : 4] || "🌕";
  const mName = d.moonName || "Mesiac";
  const mIllum = (d.moonIllum !== undefined) ? ("  " + d.moonIllum + "%") : "";
  ctx.fillStyle = "#e2e8f0"; ctx.font = "bold 13.5px system-ui";
  ctx.fillText(mEmoji + " " + mName + mIllum, CX, 226);

 } else if (scr === 1 || scr === 3) {
  // === SCREEN 1 (PLANES) / SCREEN 3 (TACTICAL) ===
  const isTactical = (scr === 3);
  const strokeCol = isTactical ? "#0284c7" : "#1e293b";
  const radR = R - 10;

  // Concentric range rings
  ctx.strokeStyle = strokeCol; ctx.lineWidth = 1.2;
  ctx.beginPath(); ctx.arc(CX, CY, radR, 0, Math.PI*2); ctx.stroke();
  ctx.beginPath(); ctx.arc(CX, CY, radR * 0.66, 0, Math.PI*2); ctx.stroke();
  ctx.beginPath(); ctx.arc(CX, CY, radR * 0.33, 0, Math.PI*2); ctx.stroke();

  // Crosshair
  ctx.beginPath(); ctx.moveTo(CX - radR, CY); ctx.lineTo(CX + radR, CY); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(CX, CY - radR); ctx.lineTo(CX, CY + radR); ctx.stroke();

  // Tactical bearing ticks
  if (isTactical) {
   for (let deg = 0; deg < 360; deg += 30) {
    const a = deg * Math.PI / 180;
    ctx.beginPath();
    ctx.moveTo(CX + (radR - 6) * Math.cos(a), CY + (radR - 6) * Math.sin(a));
    ctx.lineTo(CX + radR * Math.cos(a), CY + radR * Math.sin(a));
    ctx.stroke();
   }
  }

  // Home Beacon
  ctx.strokeStyle = "#38bdf8"; ctx.lineWidth = 2;
  ctx.beginPath(); ctx.arc(CX, CY, 6, 0, Math.PI*2); ctx.stroke();
  ctx.fillStyle = "#f59e0b"; ctx.beginPath(); ctx.arc(CX, CY, 3, 0, Math.PI*2); ctx.fill();

  // Aircraft targets
  if (d.aircraft && d.aircraft.length > 0) {
   const clat = d.lat || 49.2011, clon = d.lon || 21.2442;
   let rngKm = parseFloat(d.range) || 50.0; if (rngKm <= 0) rngKm = 50.0;
   const tbRad = (d.topBearing || 0) * Math.PI / 180;

   d.aircraft.forEach(ac => {
    let dLat = (ac.lat - clat) * 111.0;
    let dLon = (ac.lon - clon) * (111.0 * Math.cos(clat * 0.017453));
    let rotX = dLon * Math.cos(-tbRad) - dLat * Math.sin(-tbRad);
    let rotY = dLon * Math.sin(-tbRad) + dLat * Math.cos(-tbRad);

    const px = CX + (rotX / rngKm) * radR;
    const py = CY - (rotY / rngKm) * radR;
    const distFromCenter = Math.hypot(px - CX, py - CY);

    if (distFromCenter <= radR) {
     const altM = ac.alt * 0.3048;
     const col = ac.em ? "#ef4444" : (ac.alt > 30000) ? "#f43f5e" : (ac.alt > 15000) ? "#38bdf8" : "#22c55e";

     ctx.fillStyle = col; ctx.strokeStyle = col; ctx.lineWidth = 1.5;
     const trkRad = ((ac.trk || 0) - (d.topBearing || 0) - 90) * Math.PI / 180;
     ctx.beginPath(); ctx.arc(px, py, 3.5, 0, Math.PI*2); ctx.fill();

     // Velocity vector line
     ctx.beginPath(); ctx.moveTo(px, py); ctx.lineTo(px + 14 * Math.cos(trkRad), py + 14 * Math.sin(trkRad)); ctx.stroke();

     // Label
     ctx.fillStyle = "#f1f5f9"; ctx.font = "bold 9px system-ui"; ctx.textAlign = "left";
     let climbIco = (ac.climb > 300) ? " ▲" : (ac.climb < -300) ? " ▼" : "";
     ctx.fillText((ac.call || ac.hex) + climbIco, px + 6, py + 3);

     if (isTactical) {
      ctx.fillStyle = "#94a3b8"; ctx.font = "8px monospace";
      ctx.fillText(Math.round(altM) + "m", px + 6, py + 12);
     }
    }
   });
  }

  // Header & Range
  ctx.fillStyle = "#38bdf8"; ctx.font = "bold 11px system-ui"; ctx.textAlign = "center";
  ctx.fillText(isTactical ? "TAKTIKA" : "LIETADLÁ", CX, 24);
  ctx.fillStyle = "#94a3b8"; ctx.font = "10px monospace";
  ctx.fillText(d.range || "50 km", CX, H - 16);

 } else if (scr === 2) {
  // === SCREEN 2: METEORADAR ===
  const radR = R - 10;
  ctx.strokeStyle = "#047857"; ctx.lineWidth = 1.2;
  ctx.beginPath(); ctx.arc(CX, CY, radR, 0, Math.PI*2); ctx.stroke();
  ctx.beginPath(); ctx.arc(CX, CY, radR * 0.66, 0, Math.PI*2); ctx.stroke();
  ctx.beginPath(); ctx.arc(CX, CY, radR * 0.33, 0, Math.PI*2); ctx.stroke();

  // Radar beam sweep simulation
  const nowMs = Date.now();
  const sweepAngle = (nowMs % 4000) / 4000 * Math.PI * 2;
  ctx.strokeStyle = "rgba(16, 185, 129, 0.4)"; ctx.lineWidth = 2;
  ctx.beginPath(); ctx.moveTo(CX, CY); ctx.lineTo(CX + radR * Math.cos(sweepAngle), CY + radR * Math.sin(sweepAngle)); ctx.stroke();

  // Precipitation echoes simulation glow
  ctx.fillStyle = "rgba(16, 185, 129, 0.15)";
  ctx.beginPath(); ctx.arc(CX + 25, CY - 20, 35, 0, Math.PI*2); ctx.fill();
  ctx.fillStyle = "rgba(59, 130, 246, 0.2)";
  ctx.beginPath(); ctx.arc(CX - 30, CY + 25, 45, 0, Math.PI*2); ctx.fill();

  // Home Beacon
  ctx.fillStyle = "#f59e0b"; ctx.beginPath(); ctx.arc(CX, CY, 4, 0, Math.PI*2); ctx.fill();

  // Title & Scale
  ctx.fillStyle = "#10b981"; ctx.font = "bold 12px system-ui"; ctx.textAlign = "center";
  ctx.fillText("METEORADAR", CX, 24);
  ctx.fillStyle = "#94a3b8"; ctx.font = "10px monospace";
  ctx.fillText(d.range || "192 km", CX, H - 18);

  // Precipitation color bar
  const barW = 100, barH = 5, barX = CX - barW/2, barY = H - 32;
  const grad = ctx.createLinearGradient(barX, barY, barX + barW, barY);
  grad.addColorStop(0, "#0284c7"); grad.addColorStop(0.3, "#22c55e"); grad.addColorStop(0.7, "#eab308"); grad.addColorStop(1, "#ef4444");
  ctx.fillStyle = grad; ctx.fillRect(barX, barY, barW, barH);

 } else if (scr === 4) {
  // === SCREEN 4: PREDPOCEĎ POČASIA ===
  ctx.fillStyle = "#38bdf8"; ctx.font = "bold 13px system-ui"; ctx.textAlign = "center";
  ctx.fillText("PREDPOVEĎ POČASIA", CX, 24);

  // Current Temp Hero
  ctx.fillStyle = "#ffffff"; ctx.font = "bold 26px system-ui";
  ctx.fillText((d.temp !== undefined ? d.temp : "--") + " °C", CX, 62);
  ctx.fillStyle = "#94a3b8"; ctx.font = "11px system-ui";
  ctx.fillText("Zrážky: " + (d.precip !== undefined ? d.precip : 0) + " mm  ·  AQI: " + (d.aqi !== undefined ? d.aqi : "--"), CX, 82);

  // Hourly rows/cards
  if (d.hours && d.hours.length > 0) {
   const cardW = 38, startX = CX - (d.hours.length * (cardW + 4))/2;
   d.hours.forEach((h, idx) => {
    const x = startX + idx * (cardW + 4);
    ctx.fillStyle = "#1e293b"; ctx.roundRect(x, 100, cardW, 65, 6); ctx.fill();
    ctx.fillStyle = "#94a3b8"; ctx.font = "bold 10px system-ui"; ctx.fillText(h.h + "h", x + cardW/2, 114);
    ctx.fillStyle = "#38bdf8"; ctx.font = "14px system-ui"; ctx.fillText("⛅", x + cardW/2, 134);
    ctx.fillStyle = "#ffffff"; ctx.font = "bold 11px system-ui"; ctx.fillText(h.temp + "°", x + cardW/2, 155);
   });
  }

  // 3-Day Forecast Footer
  ctx.fillStyle = "#64748b"; ctx.font = "10.5px system-ui";
  ctx.fillText("3-dňový výhľad: ☀️ Streda  ⛅ Štvrtok  🌧️ Piatok", CX, 200);

 } else if (scr === 5) {
  // === SCREEN 5: NASTAVENIA ===
  ctx.fillStyle = "#22c55e"; ctx.font = "bold 13px system-ui"; ctx.textAlign = "center";
  ctx.fillText("NASTAVENIA", CX, 26);

  // Compass Dial
  ctx.strokeStyle = "#334155"; ctx.lineWidth = 2;
  ctx.beginPath(); ctx.arc(CX, 105, 45, 0, Math.PI*2); ctx.stroke();
  const tbRad = (d.topBearing || 0) * Math.PI / 180 - Math.PI/2;
  ctx.strokeStyle = "#38bdf8"; ctx.lineWidth = 3;
  ctx.beginPath(); ctx.moveTo(CX, 105); ctx.lineTo(CX + 35 * Math.cos(tbRad), 105 + 35 * Math.sin(tbRad)); ctx.stroke();
  ctx.fillStyle = "#ffffff"; ctx.font = "bold 11px monospace";
  ctx.fillText((d.topBearing || 0) + "° HORE", CX, 165);

  // Alias H4CKR4
  ctx.fillStyle = "#22c55e"; ctx.font = "bold 14px monospace";
  ctx.fillText("H4CKR4", CX, 215);
  ctx.fillStyle = "#64748b"; ctx.font = "10px system-ui";
  ctx.fillText("MeteoPlaneRadar v1.0.1", CX, 235);
 }
}

async function updateLiveTelemetry(){
 if(TAB !== "tCtl") return;
 try{
  const r = await fetch("/api/live"); if(!r.ok) return;
  const d = await r.json();
  drawVectorPreview(d);
  if(d.range && $("rangeNow")) $("rangeNow").textContent = d.range;
  if($("radarTargetBadge")){
   if(d.screen === 0) {
    $("radarTargetBadge").textContent = D[L].scrClock || "Hodiny & Astro";
   } else if(d.screen === 2) {
    $("radarTargetBadge").textContent = D[L].scrMeteo || "Meteoradar";
   } else if(d.screen === 4) {
    $("radarTargetBadge").textContent = D[L].scrForecast || "Predpoveď počasia";
   } else if(d.screen === 5) {
    $("radarTargetBadge").textContent = D[L].scrSettings || "Nastavenia";
   } else if(d.aircraft && d.aircraft.length > 0) {
    const clat = d.lat || 49.2011, clon = d.lon || 21.2442;
    let closest = null, minD = 999999;
    d.aircraft.forEach(ac => {
     const dLat = (ac.lat - clat) * 111.0;
     const dLon = (ac.lon - clon) * (111.0 * Math.cos(clat * 0.017453));
     const gndD = Math.sqrt(dLat * dLat + dLon * dLon);
     if(gndD < minD){ minD = gndD; closest = ac; }
    });
    if(closest){
     $("radarTargetBadge").textContent = "✈ " + (closest.call || closest.hex) + " · " + minD.toFixed(1) + " km · " + Math.round(closest.alt * 0.3048) + " m";
    }
   } else {
    $("radarTargetBadge").textContent = D[L].radarNoPlanes || "✈ 0 lietadiel v dosahu";
   }
  }
 }catch(e){}
}
setInterval(updateLiveTelemetry, 2000);

// --- Realtime Hardware Diagnostics ---
async function updateHardware(){
 try{
  const r=await fetch("/api/hardware");if(!r.ok)return;
  const h=await r.json();
  $("hwCpuModel").textContent=h.cpuModel||"ESP32-S3";
  $("hwCpuRev").textContent=h.cpuRev||"1";
  $("hwCpuCores").textContent=h.cpuCores||"2";
  $("hwCpuFreq").textContent=h.cpuFreq||"240";
  $("hwCpuTemp").textContent=h.cpuTemp||"-";
  $("hwReset").textContent=h.resetReason||"-";
  $("hwUptime").textContent=h.uptime||"-";

  // RAM Heap
  const hUsed = (h.heapTotal - h.heapFree);
  const hPct = Math.round((hUsed / h.heapTotal)*100);
  $("hwHeapTxt").textContent=Math.round(hUsed/1024)+" KB / "+Math.round(h.heapTotal/1024)+" KB ("+hPct+"%)";
  $("hwHeapBar").style.width=hPct+"%";

  // PSRAM
  const pUsed = (h.psramTotal - h.psramFree);
  const pPct = Math.round((pUsed / h.psramTotal)*100);
  $("hwPsramTxt").textContent=(pUsed/1048576).toFixed(2)+" MB / "+(h.psramTotal/1048576).toFixed(0)+" MB ("+pPct+"%)";
  $("hwPsramBar").style.width=pPct+"%";

  // Flash
  $("hwFlashSize").textContent=Math.round(h.flashSize/1048576);
  $("hwFlashSpd").textContent=Math.round(h.flashSpeed/1000000);

  // Network
  $("netIp").textContent=h.ip||"-";
  $("netRssi").textContent=h.rssi ? (h.rssi+" dBm") : "-";
  $("netMac").textContent=h.mac||"-";

  // IMU
  if(h.imuOk){
   $("hwAx").textContent=h.ax;$("hwAy").textContent=h.ay;$("hwAz").textContent=h.az;
   $("hwGx").textContent=h.gx;$("hwGy").textContent=h.gy;$("hwGz").textContent=h.gz;
   $("hwPitch").textContent=h.pitch;$("hwRoll").textContent=h.roll;
  }

  // RTC details
  if($("hwRtcState")){
    const ok = !!h.rtcDetected;
    $("hwRtcState").textContent = ok ? (h.rtcOscStopped ? "Výpadok napájania (OSF)" : "Aktívny (I2C 0x51)") : "Nenájdený";
    $("hwRtcState").className = "pill " + (ok ? (h.rtcOscStopped ? "pill-warn" : "pill-ok") : "pill-err");
    if($("hwRtcTime")) $("hwRtcTime").textContent = h.rtcTime || "-";
  }
  // I2C table
  if($("hwI2cTable") && h.i2cBus){
    let rows = "<tr><th>Adresa</th><th>Názov komponentu</th></tr>";
    h.i2cBus.forEach(dev => {
      rows += "<tr><td><code>" + dev.addr + "</code></td><td>" + dev.name + "</td></tr>";
    });
    $("hwI2cTable").innerHTML = rows;
  }
 }catch(e){}
}
async function syncRtcNtp(){
  try{
    const r = await fetch("/api/rtc/sync_ntp", {method:"POST"});
    if(r.ok) { msg("RTC synchronizované s NTP", "ok"); updateHardware(); }
    else { const err = await r.json(); msg(err.error||"Chyba synchronizácie", "err"); }
  }catch(e){ msg("Chyba spojenia", "err"); }
}
async function syncRtcBrowser(){
  try{
    const epoch = Math.round(Date.now() / 1000);
    const r = await fetch("/api/rtc/sync_browser", {
      method:"POST",
      headers:{"Content-Type":"application/json"},
      body:JSON.stringify({epoch:epoch})
    });
    if(r.ok) { msg("Čas z prehliadača uložený do RTC", "ok"); updateHardware(); }
    else { msg("Chyba zápisu", "err"); }
  }catch(e){ msg("Chyba spojenia", "err"); }
}
setInterval(()=>{if(TAB=="tHw") updateHardware();},3000);

// --- Okamzite ukladanie pre Vzhlad a Ovladanie ---
let saveTimer = {};
function autoSave(key, val){
 clearTimeout(saveTimer[key]);
 saveTimer[key] = setTimeout(async () => {
  const o = {}; o[key] = val;
  try{
   const r = await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(o)});
   msg(r.ok ? D[L].autoSaved : D[L].failed, r.ok ? "ok" : "err");
  }catch(e){ msg(D[L].failed,"err"); }
 }, 350);
}

const AUTO = [
 ["uiLang","change","lang",e=>+e.value],
 ["metric","change","metric",e=>e.checked],
 ["topBearing","change","topBearing",e=>+e.value],
 ["briDay","input","briDay",e=>+e.value],
 ["briNight","input","briNight",e=>+e.value],
 ["nightAuto","change","nightAuto",e=>e.checked],
 ["nightOffset","change","nightOffset",e=>+e.value],
 ["secStyle","change","secStyle",e=>+e.value],
 ["clockColor","change","clockColor",e=>hexToRgb565(e.value)],
 ["secColor","change","secColor",e=>hexToRgb565(e.value)],
 ["showLegends","change","showLegends",e=>e.checked],
  ["clockStyle","change","clockStyle",e=>+e.value],
  ["cDate","change","cDate",e=>e.checked],
  ["cWx","change","cWx",e=>e.checked],
  ["cWind","change","cWind",e=>e.checked],
  ["cMoon","change","cMoon",e=>e.checked],
  ["cAstro","change","cAstro",e=>e.checked],
  ["nightClockOnly","change","nightClockOnly",e=>e.checked],
  ["rTrails","change","rTrails",e=>e.checked],
  ["rNearest","change","rNearest",e=>e.checked],
  ["rAirports","change","rAirports",e=>e.checked],
  ["rRings","change","rRings",e=>e.checked],
];
function wireAutoSave(){
 AUTO.forEach(([id,ev,key,get])=>{
  const el=$(id); if(!el) return;
  el.addEventListener(ev,()=>autoSave(key,get(el)));
 });
}

async function load(){
 const r=await fetch("/api/config");CFG=await r.json();
 setLang(CFG.lang);
 $("ver").textContent="v"+CFG.version;
 $("lat").value=CFG.lat;$("lon").value=CFG.lon;
 $("sClock").checked=CFG.screens.clock;$("sPlanes").checked=CFG.screens.planes;
 $("sMeteo").checked=CFG.screens.meteo;$("sTactical").checked=CFG.screens.tactical;$("sForecast").checked=CFG.screens.forecast;
 $("autoRotate").value=CFG.autoRotate;$("radarSrc").value=CFG.radarSrc;
 if($("showLegends")) $("showLegends").checked=!!CFG.showLegends;
 $("briDay").value=CFG.briDay;$("briNight").value=CFG.briNight;
 $("nightAuto").checked=CFG.nightAuto;$("nightOffset").value=CFG.nightOffset;
 $("secStyle").value=CFG.secStyle;$("metric").checked=CFG.metric;$("topBearing").value=CFG.topBearing;
  if($("clockStyle")) $("clockStyle").value=CFG.clockStyle||0;
  if($("cDate")) $("cDate").checked=CFG.cDate!==false;
  if($("cWx")) $("cWx").checked=CFG.cWx!==false;
  if($("cWind")) $("cWind").checked=CFG.cWind!==false;
  if($("cMoon")) $("cMoon").checked=CFG.cMoon!==false;
  if($("cAstro")) $("cAstro").checked=CFG.cAstro!==false;
  if($("nightClockOnly")) $("nightClockOnly").checked=!!CFG.nightClockOnly;
  if($("rTrails")) $("rTrails").checked=CFG.rTrails!==false;
  if($("rNearest")) $("rNearest").checked=CFG.rNearest!==false;
  if($("rAirports")) $("rAirports").checked=CFG.rAirports!==false;
  if($("rRings")) $("rRings").checked=CFG.rRings!==false;
 $("clockColor").value=rgb565ToHex(CFG.clockColor);$("secColor").value=rgb565ToHex(CFG.secColor);
 $("altMin").value=CFG.altMin;$("altMax").value=CFG.altMax;
 pwState();
 $("onlyCallsign").checked=CFG.onlyCallsign;$("squawkAlert").checked=CFG.squawkAlert;$("watch").value=CFG.watch||"";
 $("wifiHintTxt").textContent=CFG.apMode?D[L].wifiHint:D[L].wifiHintSta;
 bri();wireAutoSave();status();updateLiveTelemetry();updateHardware();
}

function pwState(){
 if(!$("pwState"))return;
 const on=!!CFG.hasPassword;
 $("pwState").textContent=on?D[L].pwSet:D[L].pwNone;
 $("pwState").className="hint "+(on?"ok":"warn");
}
function bri(){$("briDayV").textContent=$("briDay").value+"%";$("briNightV").textContent=$("briNight").value+"%";}
$("briDay").addEventListener("input",bri);$("briNight").addEventListener("input",bri);

async function status(){
 try{const s=await(await fetch("/api/status")).json();
 drawScrBtns(s.screen,s.enabled);
 if(!CFG.apMode&&$("wifiNow")) $("wifiNow").textContent=D[L].wifiNow+" "+s.ssid+" ("+s.rssi+" dBm).";
 const hasR=(s.range&&s.range.length>0);
 $("rangeNow").textContent=hasR?s.range:"–";
 $("rMinus").disabled=!hasR;$("rPlus").disabled=!hasR;
 $("rMinus").style.opacity=$("rPlus").style.opacity=hasR?"1":".4";
 }catch(e){}
}
setInterval(status,5000);

function body(){return{
 lat:parseFloat($("lat").value),lon:parseFloat($("lon").value),
 lang:parseInt($("uiLang").value),metric:$("metric").checked,
 briDay:+$("briDay").value,briNight:+$("briNight").value,nightAuto:$("nightAuto").checked,
 nightOffset:+$("nightOffset").value,secStyle:+$("secStyle").value,
 clockColor:hexToRgb565($("clockColor").value),secColor:hexToRgb565($("secColor").value),
 topBearing:+$("topBearing").value,
 altMin:+$("altMin").value,altMax:+$("altMax").value,onlyCallsign:$("onlyCallsign").checked,
 squawkAlert:$("squawkAlert").checked,watch:$("watch").value.trim(),
 autoRotate:+$("autoRotate").value,radarSrc:+$("radarSrc").value,
 showLegends:$("showLegends")?$("showLegends").checked:true,
  clockStyle:$("clockStyle")?+$("clockStyle").value:0,
  cDate:$("cDate")?$("cDate").checked:true,
  cWx:$("cWx")?$("cWx").checked:true,
  cWind:$("cWind")?$("cWind").checked:true,
  cMoon:$("cMoon")?$("cMoon").checked:true,
  cAstro:$("cAstro")?$("cAstro").checked:true,
  nightClockOnly:$("nightClockOnly")?$("nightClockOnly").checked:false,
  rTrails:$("rTrails")?$("rTrails").checked:true,
  rNearest:$("rNearest")?$("rNearest").checked:true,
  rAirports:$("rAirports")?$("rAirports").checked:true,
  rRings:$("rRings")?$("rRings").checked:true,
 screens:{clock:$("sClock").checked,planes:$("sPlanes").checked,meteo:$("sMeteo").checked,tactical:$("sTactical").checked,forecast:$("sForecast").checked}
};}

async function save(){
 const b=body();
 if($("newPass").value){b.oldPass=$("oldPass").value;b.newPass=$("newPass").value;}
 try{
  const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(b)});
  if(r.status==401){msg(D[L].wrongPass||"Chybné heslo","err");return;}
  if(!r.ok){msg(D[L].failed,"err");return;}
  msg(D[L].saved,"ok");
  $("oldPass").value="";$("newPass").value="";
 }catch(e){msg(D[L].failed,"err");}
}

async function scan(){
 $("ssid").innerHTML="<option>"+D[L].searching+"</option>";
 try{
  const r=await fetch("/api/scan");const list=await r.json();
  if(!list.length){$("ssid").innerHTML="<option>"+D[L].nothing+"</option>";return;}
  $("ssid").innerHTML=list.map(n=>"<option value='"+n.ssid+"'>"+n.ssid+" ("+n.rssi+" dBm)</option>").join("");
 }catch(e){$("ssid").innerHTML="<option>"+D[L].failed+"</option>";}
}

async function saveWifi(){
 const s=$("ssid").value,p=$("wpass").value;
 if(!s){msg(D[L].failed,"err");return;}
 try{
  const r=await fetch("/api/wifi",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({ssid:s,pass:p})});
  if(r.ok)msg(D[L].saved,"ok");else msg(D[L].failed,"err");
 }catch(e){msg(D[L].failed,"err");}
}

async function geo(){
 const q=$("q").value.trim();if(!q)return;
 $("geoRow").classList.remove("hide");
 $("geoSel").innerHTML="<option>"+D[L].searching+"</option>";
 try{
  const r=await fetch("/api/geocode?q="+encodeURIComponent(q));const list=await r.json();
  if(!list.length){$("geoSel").innerHTML="<option>"+D[L].nothing+"</option>";return;}
  $("geoSel").innerHTML=list.map(c=>"<option value='"+c.lat+","+c.lon+"'>"+c.name+" ("+c.country+")</option>").join("");
  pickCity();
 }catch(e){$("geoSel").innerHTML="<option>"+D[L].failed+"</option>";}
}
function pickCity(){
 const v=$("geoSel").value;if(!v||v.indexOf(",")<0)return;
 const p=v.split(",");$("lat").value=(+p[0]).toFixed(4);$("lon").value=(+p[1]).toFixed(4);
}

function doReboot(){if(confirm(D[L].reboot+"?"))fetch("/api/reboot",{method:"POST"});}
function doReset(){
 const p=prompt(D[L].confirmReset+" ("+D[L].adminPass+"):");
 if(p===null)return;
 fetch("/api/reset",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({pass:p})}).then(r=>{
  if(r.status==401)alert(D[L].wrongPass||"Chybné heslo");else alert(D[L].doneReboot||"Hotovo. Reštartujem...");
 });
}

function importCfg(input){
 const f=input.files[0];if(!f)return;
 const r=new FileReader();
 r.onload=async e=>{
  try{
   const cfg=JSON.parse(e.target.result);
   const pass=prompt(D[L].adminPass+":")||"";
   const res=await fetch("/api/import",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({config:cfg,pass:pass})});
   if(res.status==401)alert(D[L].wrongPass||"Chybné heslo");else if(res.ok)alert(D[L].importOk||"Import úspešný. Reštartujem...");else alert(D[L].failed);
  }catch(err){alert(D[L].failed);}
 };
 r.readAsText(f);
}

function rgb565ToHex(c){
 const r=Math.round(((c>>11)&0x1F)*255/31);
 const g=Math.round(((c>>5)&0x3F)*255/63);
 const b=Math.round((c&0x1F)*255/31);
 return "#"+[r,g,b].map(x=>x.toString(16).padStart(2,"0")).join("");
}
function hexToRgb565(h){
 const r=parseInt(h.substr(1,2),16);
 const g=parseInt(h.substr(3,2),16);
 const b=parseInt(h.substr(5,2),16);
 return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
}

window.onload=load;
</script>
</body></html>
)rawliteral";
