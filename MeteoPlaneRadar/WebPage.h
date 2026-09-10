// =============================================================================
//  MeteoPlaneRadar
//  The configuration page, as one PROGMEM string.
//
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
header{padding:12px 20px;display:flex;align-items:center;justify-content:space-between;gap:14px;flex-wrap:wrap;background:#0d131f;border-bottom:1px solid var(--line)}
.brand{display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.brand .logo{font-size:22px;color:var(--acc);line-height:1}
.brand .title{font-size:18px;font-weight:700;color:#fff;letter-spacing:-0.02em;line-height:1}
.badge{background:rgba(56,189,248,0.12);color:var(--acc);padding:4px 9px;border-radius:6px;font-size:11px;font-weight:700;letter-spacing:0.05em;border:1px solid rgba(56,189,248,0.35);text-decoration:none;transition:all 0.15s ease;display:inline-flex;align-items:center;line-height:1}
.badge:hover{background:rgba(56,189,248,0.25);color:#fff;border-color:var(--acc);box-shadow:0 0 10px var(--acc-glow)}
.ver-pill{color:var(--mut);font-size:11.5px;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;background:rgba(255,255,255,0.05);padding:3px 8px;border-radius:6px;border:1px solid rgba(255,255,255,0.09);line-height:1}
.hdr-right{display:flex;align-items:center;gap:12px;margin-left:auto}
.live-pill{display:inline-flex;align-items:center;gap:7px;background:rgba(34,197,94,0.1);color:var(--ok);border:1px solid rgba(34,197,94,0.25);padding:4px 10px;border-radius:20px;font-size:12px;font-weight:600}
.pulse-dot{width:7px;height:7px;border-radius:50%;background:var(--ok);box-shadow:0 0 8px var(--ok);animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1;transform:scale(1)}50%{opacity:0.4;transform:scale(0.85)}}

nav.tabs{display:flex;gap:6px;overflow-x:auto;padding:8px 16px;border-bottom:1px solid var(--line);
 position:sticky;top:0;background:rgba(9,13,20,0.94);backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px);z-index:20;scrollbar-width:none}
nav.tabs::-webkit-scrollbar{display:none}
nav.tabs button{background:transparent;color:var(--mut);border:1px solid transparent;border-radius:8px;
 padding:7px 13px;white-space:nowrap;font-weight:500;font-size:13px;cursor:pointer;transition:all 0.15s ease}
nav.tabs button:hover{color:var(--fg);background:#161f30}
nav.tabs button.on{background:var(--card-hdr);color:var(--acc);border-color:var(--line);font-weight:600;box-shadow:0 0 12px var(--acc-glow)}
nav.tabs button.tab-serial{background:rgba(56,189,248,0.06);border-color:rgba(56,189,248,0.2);color:var(--acc)}
nav.tabs button.tab-serial:hover{background:rgba(56,189,248,0.15)}
nav.tabs button.tab-serial.on{background:var(--card-hdr);color:var(--acc);border-color:var(--acc);box-shadow:0 0 12px var(--acc-glow)}
nav.tabs button.tab-common{margin-left:auto;background:rgba(255,255,255,0.03);border-color:rgba(255,255,255,0.08)}
nav.tabs button.tab-common.on{background:var(--card-hdr);color:var(--acc);border-color:var(--line)}

.wrap{max-width:1240px;margin:0 auto;padding:16px 16px 96px}
.main-grid{display:flex;flex-direction:column;gap:18px}
@media(min-width:980px){
  .main-grid{display:grid;grid-template-columns:1fr 390px;gap:20px;align-items:start}
  .hw-col{position:sticky;top:54px;max-height:calc(100vh - 72px);overflow-y:auto;scrollbar-width:thin;scrollbar-color:var(--line) transparent;padding-right:4px}
}

.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 4px 16px rgba(0,0,0,0.2)}
.card h2{font-size:15px;margin:0 0 12px;color:var(--acc);display:flex;align-items:center;gap:8px;font-weight:600;border-bottom:1px solid rgba(34,45,66,0.6);padding-bottom:8px}

.screen-hero{background:linear-gradient(135deg,#151e2e 0%,#0f1725 100%);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:14px;display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:12px}
.hero-title{display:flex;align-items:center;gap:10px}
.hero-title h2{font-size:16px;font-weight:700;color:#fff;margin:0}
.hero-actions{display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.btn-live{background:var(--ok);color:#000;font-weight:700;border:0;border-radius:8px;padding:8px 14px;cursor:pointer;display:inline-flex;align-items:center;gap:6px;font-size:13px;transition:all 0.15s ease}
.btn-live:hover{filter:brightness(1.1);box-shadow:0 0 12px rgba(34,197,94,0.3)}

.row{display:flex;align-items:center;gap:10px;margin:10px 0;flex-wrap:wrap}
.row label{flex:1 1 200px;min-width:150px;color:var(--fg);font-size:13.5px}
input[type=text],input[type=password],input[type=number],select{background:#0a0e17;color:var(--fg);
 border:1px solid var(--line);border-radius:7px;padding:8px 10px;min-width:110px;font-size:13.5px;outline:none;transition:border-color 0.15s}
input[type=text]:focus,input[type=password]:focus,input[type=number]:focus,select:focus{border-color:var(--acc)}
input[type=color]{background:#0a0e17;border:1px solid var(--line);border-radius:7px;height:36px;width:56px;padding:2px;cursor:pointer}
input[type=range]{flex:1 1 160px;accent-color:var(--acc)}
input[type=checkbox]{
  -webkit-appearance:none;appearance:none;
  width:38px;height:22px;min-width:38px;
  background:#1a2333;border:1px solid var(--line);border-radius:999px;
  position:relative;cursor:pointer;outline:none;
  transition:all 0.2s cubic-bezier(0.4,0,0.2,1);
  flex-shrink:0;margin:0;vertical-align:middle;
}
input[type=checkbox]::after{
  content:"";position:absolute;top:2px;left:2px;
  width:16px;height:16px;background:#64748b;border-radius:50%;
  transition:all 0.2s cubic-bezier(0.4,0,0.2,1);
  box-shadow:0 1px 3px rgba(0,0,0,0.5);
}
input[type=checkbox]:hover{border-color:rgba(56,189,248,0.4)}
input[type=checkbox]:hover::after{background:#94a3b8}
input[type=checkbox]:checked{background:var(--acc);border-color:var(--acc);box-shadow:0 0 10px var(--acc-glow)}
input[type=checkbox]:checked::after{transform:translateX(16px);background:#090d14}
input[type=checkbox]:focus-visible{box-shadow:0 0 0 2px var(--acc-glow)}
button{background:var(--acc);color:#08202a;border:0;border-radius:8px;padding:8px 14px;font-weight:600;
 cursor:pointer;font-size:13.5px;transition:all 0.15s ease}
button:hover{filter:brightness(1.1)}
button:active{transform:scale(0.98)}
button.sec{background:#1e2738;color:var(--fg);border:1px solid var(--line)}
button.sec:hover{background:#28344a;border-color:var(--acc)}
button.danger{background:var(--err);color:#fff}
button:disabled{cursor:default;opacity:0.4}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px}
.chk{display:flex;align-items:center;gap:10px;cursor:pointer;font-size:13.5px;user-select:none;padding:2px 0}
.hint{color:var(--mut);font-size:12.5px;margin:6px 0 0;line-height:1.4}
.bar{position:fixed;left:0;right:0;bottom:0;background:rgba(13,19,31,0.95);backdrop-filter:blur(12px);border-top:1px solid var(--line);
 padding:12px 16px calc(12px + env(safe-area-inset-bottom));display:flex;gap:12px;align-items:center;z-index:30}
#msg{font-size:13.5px;font-weight:500}
.ok{color:var(--ok)}.err{color:var(--err)}.warn{color:var(--warn)}
table{width:100%;border-collapse:collapse;font-size:13px}
td{padding:6px 4px;border-bottom:1px solid var(--line)}
td:first-child{color:var(--mut);width:45%}
.hide{display:none !important}
.prog-bar{background:#0a0e17;border:1px solid var(--line);border-radius:999px;height:10px;overflow:hidden;width:100%;margin-top:4px}
.prog-fill{height:100%;background:linear-gradient(90deg,var(--acc),var(--purple));border-radius:999px;transition:width 0.3s ease}
.pill{display:inline-block;padding:2px 8px;border-radius:6px;font-size:11px;font-weight:600}
.pill-ok{background:rgba(34,197,94,0.15);color:var(--ok);border:1px solid rgba(34,197,94,0.3)}
.pill-warn{background:rgba(245,158,11,0.15);color:var(--warn);border:1px solid rgba(245,158,11,0.3)}
.pill-err{background:rgba(239,68,68,0.15);color:var(--err);border:1px solid rgba(239,68,68,0.3)}
.stat-val{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:12.5px;color:#fff}
.range-ctrl-row{display:flex;align-items:center;justify-content:space-between;margin-top:10px;background:#0a0e17;border:1px solid var(--line);border-radius:8px;padding:6px 12px}
.range-lbl{font-weight:600;color:var(--mut);font-size:12.5px}
.range-stepper{display:inline-flex;align-items:center;background:#111827;border:1px solid var(--line);border-radius:6px;overflow:hidden}
.btn-step{background:#1e293b;color:#fff;border:none;width:32px;height:28px;font-size:16px;font-weight:700;cursor:pointer;display:inline-flex;align-items:center;justify-content:center;transition:background 0.15s,color 0.15s;padding:0;line-height:1}
.btn-step:hover:not(:disabled){background:var(--acc);color:#0b0f19}
.btn-step:disabled{opacity:0.35;cursor:not-allowed}
.range-val{min-width:68px;text-align:center;font-size:13px;font-weight:700;color:var(--acc);padding:0 6px;user-select:none}

@media (max-width:560px){
  .row label{flex:1 1 100%}
  .row input[type=text],.row input[type=password],.row input[type=number],.row select{flex:1 1 100%}
  .screen-hero{flex-direction:column;align-items:flex-start}
}
</style></head><body>

<header>
  <div class="brand">
    <span class="logo">✈</span>
    <span class="title">MeteoPlaneRadar</span>
    <a href="https://github.com/hackra76" target="_blank" rel="noopener noreferrer" class="badge">H4CKR4</a>
    <span class="ver-pill" id="ver"></span>
  </div>
  <div class="hdr-right">
    <div id="liveDispPill" class="live-pill" title="Aktuálne zobrazená obrazovka na displeji">
      <span class="pulse-dot"></span>
      <span id="liveDispName">Hodiny</span>
    </div>
    <select id="uiLang" onchange="setLang(this.value)">
      <option value="2">🇸🇰 Slovenčina</option>
      <option value="0">🇨🇿 Čeština</option>
      <option value="1">🇬🇧 English</option>
    </select>
  </div>
</header>

<nav class="tabs" id="tabs">
  <button data-tab="tScrClock"    class="on" data-i18n="tabScrClock">🕒 Hodiny</button>
  <button data-tab="tScrPlanes"   data-i18n="tabScrPlanes">✈️ Lietadlá</button>
  <button data-tab="tScrMeteo"    data-i18n="tabScrMeteo">🌧️ Meteoradar</button>
  <button data-tab="tScrTactical" data-i18n="tabScrTactical">🎯 Taktický radar</button>
  <button data-tab="tScrForecast" data-i18n="tabScrForecast">⛅ Predpoveď</button>
  <button data-tab="tScrFinance"  data-i18n="tabScrFinance">📈 Trhy & Krypto</button>
  <button data-tab="tScrIss"      data-i18n="tabScrIss">🛰️ ISS Tracker</button>
  <button data-tab="tScrInfo"     data-i18n="tabScrInfo">ℹ️ Info & Štatistiky</button>
  <button data-tab="tSerial"      data-i18n="tabSerial" class="tab-serial">📟 Sériový monitor</button>
  <button data-tab="tCommon"      data-i18n="tabCommon" class="tab-common">⚙️ Spoločné nastavenia</button>
</nav>

<div class="wrap">
  <div class="main-grid">

    <!-- ================= ĽAVÝ STĹPEC: OBRAZOVKY & NASTAVENIA ================= -->
    <div class="screens-col">

      <!-- 1. OBRAZOVKA: HODINY & ASTRO -->
      <section id="tScrClock" class="tab">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>🕒 <span data-i18n="scrClockHdr">Hodiny & Astro</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(0)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sClock">
              <span data-i18n="scrClockActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
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
        </div>

        <div class="card">
          <h2 data-i18n="clockWidgets">Prvky na obrazovke hodín</h2>
          <div class="grid">
            <label class="chk"><input type="checkbox" id="cDate"><span data-i18n="cDate">Dátum</span></label>
            <label class="chk"><input type="checkbox" id="cWx"><span data-i18n="cWx">Počasie & teplota</span></label>
            <label class="chk"><input type="checkbox" id="cWind"><span data-i18n="cWind">Rýchlosť vetra</span></label>
            <label class="chk"><input type="checkbox" id="cMoon"><span data-i18n="cMoon">Fáza mesiaca</span></label>
            <label class="chk"><input type="checkbox" id="cAstro"><span data-i18n="cAstro">24h solárny prstenec</span></label>
            <label class="chk"><input type="checkbox" id="nightClockOnly"><span data-i18n="nightClockOnly">V noci iba Hodiny (zastaviť radary)</span></label>
            <label class="chk"><input type="checkbox" id="bzHourlyClock"><span data-i18n="bzHourly">🕒 Pípnutie na celú hodinu (chime)</span></label>
            <label class="chk"><input type="checkbox" id="cOver"><span data-i18n="cOver">✈️ Nadhlavný let (Overhead widget)</span></label>
            <label class="chk"><input type="checkbox" id="bzOverheadClock"><span data-i18n="bzOverhead">🔊 Pípnutie pri prelete nad hlavou</span></label>
            <label class="chk"><input type="checkbox" id="cPrecip"><span data-i18n="cPrecip">🌧️ Výstraha blížiacich sa zrážok</span></label>
            <label class="chk"><input type="checkbox" id="bzPrecipClock"><span data-i18n="bzPrecip">🔊 Pípnutie pri blížiacich sa zrážkach</span></label>
          </div>
          <div class="row" style="margin-top:12px;"><label data-i18n="ovRad">Polomer preletu nad hlavou (km)</label><input type="number" id="ovRad" min="1" max="50" step="1" style="max-width:110px;"></div>
        </div>
      </section>

      <!-- 2. OBRAZOVKA: LIETADLÁ RADAR -->
      <section id="tScrPlanes" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>✈️ <span data-i18n="scrPlanesHdr">Lietadlá radar (ADS-B)</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(1)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sPlanes">
              <span data-i18n="scrPlanesActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
        </div>

        <div class="card">
          <h2 data-i18n="planes">✈️ ADS-B Filtre & Sledovanie</h2>
          <div class="row"><label data-i18n="altMin">Minimálna letová výška (ft)</label><input type="number" id="altMin" step="500"></div>
          <div class="row"><label data-i18n="altMax">Maximálna letová výška (ft)</label><input type="number" id="altMax" step="500"></div>
          <div class="row"><label class="chk"><input type="checkbox" id="onlyCallsign"><span data-i18n="onlyCs">Iba lietadlá so známym volacím znakom (callsign)</span></label></div>
          <div class="row"><label class="chk"><input type="checkbox" id="squawkAlert"><span data-i18n="sqAlert">Zvýrazniť a upozorniť na núdzové squawky (7500 / 7600 / 7700)</span></label></div>
          <div class="row"><label class="chk"><input type="checkbox" id="bzEmergencyPlanes"><span data-i18n="bzEmergency">🚨 Zvuková výstraha pri núdzovom squawku</span></label></div>
          <div class="row"><label data-i18n="watch">Sledovaný let (Callsign alebo ICAO hex)</label><input type="text" id="watch" placeholder="RYR, WZZ, LZ..."></div>
          <div class="row"><label class="chk"><input type="checkbox" id="bzWatchPlanes"><span data-i18n="bzWatch">⭐ Zvukové upozornenie na sledovaný let</span></label></div>
          <p class="hint" data-i18n="planesHint">Filtre ovplyvňujú len vykresľovanie. Núdzový squawk ani sledované lietadlo filter nikdy neskryje.</p>
        </div>

        <div class="card">
          <h2 data-i18n="typeFiltersTitle">🎯 Filter typov strojov na radare</h2>
          <p class="hint" data-i18n="typeFiltersHint">Vyberte kategórie strojov, ktoré sa majú zobrazovať na radarovej mape. Núdzové a sledované lety sa zobrazia vždy.</p>
          <div class="grid">
            <label class="chk"><input type="checkbox" id="tfAirliner"><span data-i18n="tfAirliner">✈️ Dopravné lietadlá</span></label>
            <label class="chk"><input type="checkbox" id="tfLight"><span data-i18n="tfLight">🛩️ Malé a športové lietadlá</span></label>
            <label class="chk"><input type="checkbox" id="tfHeli"><span data-i18n="tfHeli">🚁 Vrtuľníky a záchranári</span></label>
            <label class="chk"><input type="checkbox" id="tfMil"><span data-i18n="tfMil">⚔️ Vojenské letectvo a stíhačky</span></label>
            <label class="chk"><input type="checkbox" id="tfHeavy"><span data-i18n="tfHeavy">🛫 Veľké nákladné obry</span></label>
            <label class="chk"><input type="checkbox" id="tfGlider"><span data-i18n="tfGlider">🪂 Vetrone a klzáky</span></label>
          </div>
        </div>

        <div class="card">
          <h2 data-i18n="radarWidgets">Prvky radarovej mapy lietadiel</h2>
          <div class="grid">
            <label class="chk"><input type="checkbox" id="rTrails"><span data-i18n="rTrails">Trajektórie lietadiel (Trails)</span></label>
            <label class="chk"><input type="checkbox" id="rNearest"><span data-i18n="rNearest">Vektor k najbližšiemu lietadlu</span></label>
            <label class="chk"><input type="checkbox" id="rAirports"><span data-i18n="rAirports">Letiská (Runway ikony)</span></label>
            <label class="chk"><input type="checkbox" id="rRings"><span data-i18n="rRings">Kilometrové kružnice dosahu</span></label>
            <label class="chk"><input type="checkbox" id="rCompass"><span data-i18n="rCompass">Elektronický kompas (miniatúra na mape)</span></label>
            <label class="chk"><input type="checkbox" id="showLegends"><span data-i18n="showAltBar">Výšková lišta letových hladín</span></label>
          </div>
        </div>
      </section>

      <!-- 3. OBRAZOVKA: METEORADAR -->
      <section id="tScrMeteo" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>🌧️ <span data-i18n="scrMeteoHdr">Zrážkový meteoradar</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(2)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sMeteo">
              <span data-i18n="scrMeteoActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
        </div>

        <div class="card">
          <h2 data-i18n="radar">🌧️ Meteoradar & Zobrazenie</h2>
          <div class="row"><label data-i18n="radarSrc">Zdroj radarových dát</label>
            <select id="radarSrc">
              <option value="0" data-i18n="srcChmu">ČHMÚ (veľmi ostré dáta, len ČR)</option>
              <option value="2" data-i18n="srcShmu">SHMÚ (veľmi ostré dáta, Slovensko)</option>
              <option value="1" data-i18n="srcRv">RainViewer (Európa a svet)</option>
            </select>
          </div>
          <div class="row"><label class="chk"><input type="checkbox" id="smoothRadar"><span data-i18n="smoothRadar">Vyhladenie zrážkového radaru (bilineárna interpolácia pre SHMÚ a ČHMÚ)</span></label></div>
          <div class="row"><label class="chk"><input type="checkbox" id="showLegendsMeteo"><span data-i18n="showPrecipBar">Farebná škála zrážok (alebo poklepanie)</span></label></div>
          <p class="hint" data-i18n="radarHint">V SR použite SHMÚ, v ČR ČHMÚ, inde v Európe a vo svete RainViewer.</p>
        </div>

        <div class="card">
          <h2 data-i18n="radarWidgets">Prvky meteoradaru</h2>
          <div class="grid">
            <label class="chk"><input type="checkbox" id="rRingsMeteo" onchange="$('rRings').checked=this.checked;autoSave('rRings',this.checked)"><span data-i18n="rRings">Kilometrové kružnice dosahu</span></label>
            <label class="chk"><input type="checkbox" id="rAirportsMeteo" onchange="$('rAirports').checked=this.checked;autoSave('rAirports',this.checked)"><span data-i18n="rAirports">Letiská (Runway ikony)</span></label>
          </div>
        </div>

        <div class="card">
          <h2 data-i18n="precipTrackerHdr">🌧️ Detekcia blížiacich sa zrážok (Nowcasting)</h2>
          <p class="hint" data-i18n="precipHint">Vektorová analýza pohybu frontu (TREC). Upozorní na dážď, krúpy alebo sneh, iba ak zrážky smerujú priamo k vašej polohe.</p>
          <div class="grid">
            <label class="chk"><input type="checkbox" id="cPrecipMeteo"><span data-i18n="cPrecip">Výstraha a odpočet na obrazovke</span></label>
            <label class="chk"><input type="checkbox" id="bzPrecipMeteo"><span data-i18n="bzPrecip">Zvukové pípnutie bzučiaka pri príchode zrážok</span></label>
          </div>
          <div style="margin-top:12px;padding:10px;background:rgba(255,255,255,0.03);border:1px solid var(--line);border-radius:8px;">
            <div style="font-size:12px;color:var(--mut);margin-bottom:4px;" data-i18n="precipLiveHdr">Aktuálny stav nowcastingu:</div>
            <div id="livePrecipStatus" style="font-weight:600;font-size:14px;color:var(--acc);">—</div>
            <div id="livePrecipDetail" style="font-size:12px;color:var(--mut);margin-top:2px;"></div>
          </div>
        </div>
      </section>

      <!-- 4. OBRAZOVKA: TAKTICKÝ RADAR -->
      <section id="tScrTactical" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>🎯 <span data-i18n="scrTacticalHdr">Taktický radar (Lietadlá + Zrážky)</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(3)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sTactical">
              <span data-i18n="scrTacticalActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
        </div>

        <div class="card">
          <h2 data-i18n="tacticalHdr">🎯 Taktické zobrazenie</h2>
          <p class="hint" data-i18n="tacticalDesc" style="font-size:13.5px;color:var(--fg);margin-bottom:12px;">
            Kombinovaný taktický radar spája ADS-B lety a búrkové radarové odrazy do jednej spoločnej obrazovky v reálnom čase.
          </p>
          <div class="grid" style="margin-top:10px;">
            <label class="chk"><input type="checkbox" id="rTrailsTac" onchange="$('rTrails').checked=this.checked;autoSave('rTrails',this.checked)"><span data-i18n="rTrails">Trajektórie lietadiel</span></label>
            <label class="chk"><input type="checkbox" id="rNearestTac" onchange="$('rNearest').checked=this.checked;autoSave('rNearest',this.checked)"><span data-i18n="rNearest">Vektor k najbližšiemu lietadlu</span></label>
            <label class="chk"><input type="checkbox" id="rAirportsTac" onchange="$('rAirports').checked=this.checked;autoSave('rAirports',this.checked)"><span data-i18n="rAirports">Letiská (Runway ikony)</span></label>
            <label class="chk"><input type="checkbox" id="rRingsTac" onchange="$('rRings').checked=this.checked;autoSave('rRings',this.checked)"><span data-i18n="rRings">Kilometrové kružnice</span></label>
            <label class="chk"><input type="checkbox" id="rCompassTac" onchange="$('rCompass').checked=this.checked;autoSave('rCompass',this.checked)"><span data-i18n="rCompass">Elektronický kompas</span></label>
          </div>
          <p class="hint" data-i18n="tacticalHint">Filtre výšky a volacích znakov sa preberajú z nastavení Lietadiel, zdroj zrážok z Meteoradaru.</p>
        </div>
      </section>

      <!-- 5. OBRAZOVKA: PREDPOVEĎ POČASIA -->
      <section id="tScrForecast" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>⛅ <span data-i18n="scrForecastHdr">Predpoveď počasia</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(4)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sForecast">
              <span data-i18n="scrForecastActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
        </div>

        <div class="card">
          <h2 data-i18n="forecastHdr">⛅ Predpoveď počasia</h2>
          <p class="hint" data-i18n="forecastDesc" style="font-size:13.5px;color:var(--fg);margin-bottom:12px;">
            Predpoveď počasia sa automaticky sťahuje zo služby Open-Meteo pre vašu domovskú polohu.
          </p>
          <div class="row" style="margin-top:14px;">
            <button type="button" class="sec" onclick="showTab('tCommon')" data-i18n="btnGoLocSettings">📍 Nastaviť domovskú polohu v Spoločných nastaveniach</button>
          </div>
        </div>
      </section>

      <!-- 6. OBRAZOVKA: FINANČNÉ TRHY & KRYPTO -->
      <section id="tScrFinance" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>📈 <span data-i18n="scrFinanceHdr">Finančné trhy, Akcie & Krypto</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(5)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sFinance">
              <span data-i18n="scrFinanceActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
        </div>

        <div class="card">
          <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;border-bottom:1px solid rgba(34,45,66,0.6);padding-bottom:8px;">
            <h2 style="margin:0;border:0;padding:0;" data-i18n="finSlotsHdr">📈 Sledované trhy & aktíva (4 pozície)</h2>
            <span class="pill pill-ok" style="font-size:11px;">Yahoo Finance</span>
          </div>

          <input type="hidden" id="financeTickers">

          <div style="display:flex;flex-direction:column;gap:10px;">
            <!-- Line 1: Hero -->
            <div style="display:flex;align-items:center;gap:10px;background:#0d1523;border:1px solid rgba(56,189,248,0.3);border-radius:10px;padding:8px 12px;flex-wrap:wrap;">
              <div style="display:flex;align-items:center;gap:8px;min-width:180px;">
                <span style="display:inline-flex;align-items:center;justify-content:center;width:26px;height:26px;border-radius:6px;background:rgba(56,189,248,0.2);color:var(--acc);font-weight:700;font-size:13px;">1</span>
                <div>
                  <div style="font-weight:700;font-size:13.5px;color:#fff;" data-i18n="finSlot1">1. Hlavný trh (Hero graf)</div>
                  <div style="font-size:11px;color:var(--acc);" data-i18n="finSlot1Sub">Veľký reálny graf + kurz</div>
                </div>
              </div>
              <select id="finSel1" onchange="onFinSelChange(1)" style="flex:1 1 180px;min-width:160px;"></select>
              <div style="display:flex;align-items:center;gap:6px;">
                <input type="text" id="finTk1" placeholder="BTC-USD" style="width:110px;text-transform:uppercase;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-weight:700;letter-spacing:0.5px;" oninput="onFinInputChange(1)">
                <button type="button" class="sec" onclick="clearFinSlot(1)" style="padding:6px 10px;font-size:12px;" title="Vymazať" data-i18n="finClear">✕</button>
              </div>
            </div>

            <!-- Line 2: Slot 2 -->
            <div style="display:flex;align-items:center;gap:10px;background:#0a0e17;border:1px solid var(--line);border-radius:10px;padding:8px 12px;flex-wrap:wrap;">
              <div style="display:flex;align-items:center;gap:8px;min-width:180px;">
                <span style="display:inline-flex;align-items:center;justify-content:center;width:26px;height:26px;border-radius:6px;background:rgba(100,116,139,0.15);color:var(--mut);font-weight:700;font-size:13px;">2</span>
                <div>
                  <div style="font-weight:600;font-size:13px;color:var(--fg);" data-i18n="finSlot2">2. Sledovaný trh</div>
                  <div style="font-size:11px;color:var(--mut);" data-i18n="finSlotWatchlist">Karta v spodnom zozname</div>
                </div>
              </div>
              <select id="finSel2" onchange="onFinSelChange(2)" style="flex:1 1 180px;min-width:160px;"></select>
              <div style="display:flex;align-items:center;gap:6px;">
                <input type="text" id="finTk2" placeholder="^GSPC" style="width:110px;text-transform:uppercase;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-weight:700;letter-spacing:0.5px;" oninput="onFinInputChange(2)">
                <button type="button" class="sec" onclick="clearFinSlot(2)" style="padding:6px 10px;font-size:12px;" title="Vymazať" data-i18n="finClear">✕</button>
              </div>
            </div>

            <!-- Line 3: Slot 3 -->
            <div style="display:flex;align-items:center;gap:10px;background:#0a0e17;border:1px solid var(--line);border-radius:10px;padding:8px 12px;flex-wrap:wrap;">
              <div style="display:flex;align-items:center;gap:8px;min-width:180px;">
                <span style="display:inline-flex;align-items:center;justify-content:center;width:26px;height:26px;border-radius:6px;background:rgba(100,116,139,0.15);color:var(--mut);font-weight:700;font-size:13px;">3</span>
                <div>
                  <div style="font-weight:600;font-size:13px;color:var(--fg);" data-i18n="finSlot3">3. Sledovaný trh</div>
                  <div style="font-size:11px;color:var(--mut);" data-i18n="finSlotWatchlist">Karta v spodnom zozname</div>
                </div>
              </div>
              <select id="finSel3" onchange="onFinSelChange(3)" style="flex:1 1 180px;min-width:160px;"></select>
              <div style="display:flex;align-items:center;gap:6px;">
                <input type="text" id="finTk3" placeholder="AAPL" style="width:110px;text-transform:uppercase;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-weight:700;letter-spacing:0.5px;" oninput="onFinInputChange(3)">
                <button type="button" class="sec" onclick="clearFinSlot(3)" style="padding:6px 10px;font-size:12px;" title="Vymazať" data-i18n="finClear">✕</button>
              </div>
            </div>

            <!-- Line 4: Slot 4 -->
            <div style="display:flex;align-items:center;gap:10px;background:#0a0e17;border:1px solid var(--line);border-radius:10px;padding:8px 12px;flex-wrap:wrap;">
              <div style="display:flex;align-items:center;gap:8px;min-width:180px;">
                <span style="display:inline-flex;align-items:center;justify-content:center;width:26px;height:26px;border-radius:6px;background:rgba(100,116,139,0.15);color:var(--mut);font-weight:700;font-size:13px;">4</span>
                <div>
                  <div style="font-weight:600;font-size:13px;color:var(--fg);" data-i18n="finSlot4">4. Sledovaný trh</div>
                  <div style="font-size:11px;color:var(--mut);" data-i18n="finSlotWatchlist">Karta v spodnom zozname</div>
                </div>
              </div>
              <select id="finSel4" onchange="onFinSelChange(4)" style="flex:1 1 180px;min-width:160px;"></select>
              <div style="display:flex;align-items:center;gap:6px;">
                <input type="text" id="finTk4" placeholder="GC=F" style="width:110px;text-transform:uppercase;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-weight:700;letter-spacing:0.5px;" oninput="onFinInputChange(4)">
                <button type="button" class="sec" onclick="clearFinSlot(4)" style="padding:6px 10px;font-size:12px;" title="Vymazať" data-i18n="finClear">✕</button>
              </div>
            </div>
          </div>

          <!-- Quick Chips -->
          <div style="margin-top:14px;padding-top:12px;border-top:1px solid rgba(34,45,66,0.6);">
            <div style="display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:6px;margin-bottom:8px;">
              <span style="font-size:12px;font-weight:600;color:var(--mut);" data-i18n="finPresets">⚡ Rýchle pridanie populárnych symbolov:</span>
            </div>
            <div style="display:flex;flex-wrap:wrap;gap:6px;">
              <button type="button" class="sec" onclick="addFinPreset('CW8.PA')" style="padding:4px 8px;font-size:11.5px;">🇪🇺 Amundi World (CW8.PA)</button>
              <button type="button" class="sec" onclick="addFinPreset('500.PA')" style="padding:4px 8px;font-size:11.5px;">🇪🇺 Amundi 500 (500.PA)</button>
              <button type="button" class="sec" onclick="addFinPreset('VWCE.DE')" style="padding:4px 8px;font-size:11.5px;">🌍 Vanguard VWCE (VWCE.DE)</button>
              <button type="button" class="sec" onclick="addFinPreset('SXR8.DE')" style="padding:4px 8px;font-size:11.5px;">🇺🇸 iShares SXR8 (SXR8.DE)</button>
              <button type="button" class="sec" onclick="addFinPreset('BTC-USD')" style="padding:4px 8px;font-size:11.5px;">₿ Bitcoin (BTC-USD)</button>
              <button type="button" class="sec" onclick="addFinPreset('ETH-USD')" style="padding:4px 8px;font-size:11.5px;">Ξ Ethereum (ETH-USD)</button>
              <button type="button" class="sec" onclick="addFinPreset('SOL-USD')" style="padding:4px 8px;font-size:11.5px;">◎ Solana (SOL-USD)</button>
              <button type="button" class="sec" onclick="addFinPreset('^GSPC')" style="padding:4px 8px;font-size:11.5px;">📈 S&P 500 (^GSPC)</button>
              <button type="button" class="sec" onclick="addFinPreset('^IXIC')" style="padding:4px 8px;font-size:11.5px;">📊 Nasdaq (^IXIC)</button>
              <button type="button" class="sec" onclick="addFinPreset('GC=F')" style="padding:4px 8px;font-size:11.5px;">🥇 Zlato (GC=F)</button>
              <button type="button" class="sec" onclick="addFinPreset('CL=F')" style="padding:4px 8px;font-size:11.5px;">🛢️ Ropa WTI (CL=F)</button>
              <button type="button" class="sec" onclick="addFinPreset('NVDA')" style="padding:4px 8px;font-size:11.5px;">💻 Nvidia (NVDA)</button>
              <button type="button" class="sec" onclick="addFinPreset('AAPL')" style="padding:4px 8px;font-size:11.5px;">🍏 Apple (AAPL)</button>
            </div>
          </div>

          <p class="hint" style="margin-top:12px;" data-i18n="financeHint">1. pozícia má veľký graf (Hero), pozície 2–4 sa zobrazujú v dolnom prehľade. Môžete vybrať z predvolieb alebo zadať ľubovoľný symbol z Yahoo Finance (napr. BTC-USD, ^GSPC, AAPL, GC=F).</p>
        </div>
      </section>

      <!-- 6. OBRAZOVKA: ISS TRACKER -->
      <section id="tScrIss" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>🛰️ <span data-i18n="scrIssHdr">ISS Tracker (Medzinárodná vesmírna stanica)</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(6)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sIss">
              <span data-i18n="scrIssActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
          <div class="row">
            <label class="chk" style="font-size:13.5px;">
              <input type="checkbox" id="issAlert">
              <span data-i18n="issAlertLbl">🔊 Zvuková výstraha pri prelete stanice nad obzorom (v dosahu / nad hlavou)</span>
            </label>
          </div>
        </div>

        <div class="card">
          <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;">
            <h2 style="margin:0;" data-i18n="issTelemetryHdr">🛰️ Živá telemetria stanice ISS</h2>
            <button type="button" class="sec" onclick="refreshIssLive()" style="padding:4px 10px;font-size:12px;" data-i18n="btnIssRefresh">🔄 Aktualizovať</button>
          </div>
          <table style="margin-top:4px;">
            <tr><td data-i18n="issStateLbl">Aktuálny stav:</td><td><span class="pill" id="issStatePill">-</span></td></tr>
            <tr><td data-i18n="issLatLonLbl">Poloha (Lat / Lon):</td><td><span class="stat-val" id="issPos">-</span></td></tr>
            <tr><td data-i18n="issAltLbl">Výška letu:</td><td><span class="stat-val" id="issAltVal">-</span></td></tr>
            <tr><td data-i18n="issVelLbl">Rýchlosť:</td><td><span class="stat-val" id="issVelVal">-</span></td></tr>
            <tr><td data-i18n="issDistLbl">Vzdialenosť:</td><td><span class="stat-val" id="issDistVal">-</span></td></tr>
            <tr><td data-i18n="issElLbl">Elevácia / Azimut:</td><td><span class="stat-val" id="issElVal">-</span></td></tr>
            <tr><td data-i18n="issSunLbl">Osvetlenie:</td><td><span class="stat-val" id="issSunVal">-</span></td></tr>
          </table>
          <p class="hint" data-i18n="issHint">Sledovanie preletu Medzinárodnej vesmírnej stanice ISS v reálnom čase (API WhereTheISS). Zobrazuje mapu sveta s dňom/nocou, orbitálnu dráhu, horizont priamej viditeľnosti (~2200 km) a časovač ďalšieho preletu.</p>
        </div>
      </section>

      <!-- 7. OBRAZOVKA: INFO & ŠTATISTIKY -->
      <section id="tScrInfo" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>ℹ️ <span data-i18n="scrInfoHdr">Info & Denná štatistika letov</span></h2>
          </div>
          <div class="hero-actions">
            <button type="button" class="btn-live" onclick="goScreen(7)">
              ▶ <span data-i18n="btnShowOnDisp">Zobraziť na displeji</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div class="row" style="margin-top:0;">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="sInfo">
              <span data-i18n="scrInfoActive">Zahrnúť obrazovku do automatického striedania</span>
            </label>
          </div>
        </div>

        <div class="card">
          <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;">
            <h2 style="margin:0;" data-i18n="statsTrafficHdr">✈️ Dnešná letecká štatistika</h2>
            <button type="button" class="sec" onclick="resetStats()" style="padding:4px 10px;font-size:12px;" data-i18n="btnResetStats">🔄 Resetovať</button>
          </div>
          <table style="margin-top:4px;">
            <tr><td data-i18n="stUnique">Unikátne lietadlá dnes:</td><td><span class="stat-val" id="stCount" style="color:var(--acc);font-weight:700;font-size:16px;">-</span></td></tr>
            <tr><td data-i18n="stTopSpeed">Najvyššia rýchlosť:</td><td><span class="stat-val" id="stSpeed" style="color:var(--warn);">-</span></td></tr>
            <tr><td data-i18n="stMaxDist">Maximálna vzdialenosť:</td><td><span class="stat-val" id="stDist">-</span></td></tr>
            <tr><td data-i18n="stAltSpan">Rozpätie letových výšok:</td><td><span class="stat-val" id="stAlt">-</span></td></tr>
            <tr><td data-i18n="stReports">Prijaté ADS-B správy:</td><td><span class="stat-val" id="stReports">-</span></td></tr>
          </table>
          <p class="hint" data-i18n="statsHint">Štatistika sa automaticky nuluje o polnoci a uchováva sa v pamäti PSRAM.</p>
        </div>
      </section>


      <!-- 8. SÉRIOVÝ MONITOR (WEB CONSOLE) -->
      <section id="tSerial" class="tab hide">
        <div class="screen-hero">
          <div class="hero-title">
            <h2>📟 <span data-i18n="serialHdr">Sériový monitor</span></h2>
          </div>
          <div class="hero-actions">
            <div id="serialStatusBadge" class="live-pill" style="background:rgba(100,116,139,0.15);color:var(--mut);border-color:rgba(100,116,139,0.3);">
              <span class="pulse-dot" style="background:var(--mut);box-shadow:none;" id="serialDot"></span>
              <span id="serialStateTxt">Pozastavené</span>
            </div>
            <button type="button" class="btn-live" id="btnToggleSerial" onclick="toggleSerialMonitor()">
              ▶ <span id="btnToggleSerialTxt" data-i18n="btnSerialStart">Spustiť monitor</span>
            </button>
          </div>
        </div>

        <div class="card">
          <div style="display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;margin-bottom:12px;">
            <div style="display:flex;align-items:center;gap:12px;flex-wrap:wrap;">
              <label class="chk" style="font-size:13px;">
                <input type="checkbox" id="serialAutoScroll" checked>
                <span data-i18n="serialAutoScroll">Automatický posun (Auto-scroll)</span>
              </label>
              <span class="ver-pill" id="serialBytesCounter">0 KB</span>
            </div>
            <div style="display:flex;align-items:center;gap:8px;flex-wrap:wrap;">
              <input type="text" id="serialFilter" placeholder="🔍 Filtrovať výpis..." oninput="filterSerialLines()" style="min-width:160px;padding:5px 10px;font-size:12.5px;">
              <button type="button" class="sec" onclick="copySerialLog()" data-i18n="btnSerialCopy" style="padding:6px 12px;font-size:12.5px;">📋 Kopírovať</button>
              <button type="button" class="sec" onclick="downloadSerialLog()" data-i18n="btnSerialDl" style="padding:6px 12px;font-size:12.5px;">💾 Stiahnuť</button>
              <button type="button" class="sec" onclick="clearSerialLog()" data-i18n="btnSerialClear" style="padding:6px 12px;font-size:12.5px;color:var(--err);border-color:rgba(239,68,68,0.3);">🧹 Vymazať</button>
            </div>
          </div>

          <div id="serialTermWrap" style="background:#060a12;border:1px solid var(--line);border-radius:10px;padding:12px 14px;position:relative;box-shadow:inset 0 2px 8px rgba(0,0,0,0.6);">
            <div id="serialTerminal" style="min-height:360px;max-height:560px;overflow-y:auto;overflow-x:auto;font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,'Courier New',monospace;font-size:12.5px;line-height:1.48;color:#cbd5e1;white-space:pre-wrap;word-break:break-all;scrollbar-width:thin;scrollbar-color:var(--line) transparent;"></div>
          </div>

          <div style="display:flex;gap:8px;margin-top:12px;align-items:center;">
            <input type="text" id="serialCmdInput" placeholder="Zadajte príkaz (napr. heap, status, ping, reboot)..." onkeydown="if(event.key==='Enter')sendSerialCmd()" style="flex:1;">
            <button type="button" onclick="sendSerialCmd()" data-i18n="btnSerialSend">Odoslať ↵</button>
          </div>
          <p class="hint" data-i18n="serialHint" style="margin-top:8px;">Streamovanie výstupov sériového portu cez WiFi bez nutnosti USB kábla. Pri odchode zo záložky sa prenos automaticky pozastaví.</p>
        </div>
      </section>

      <!-- 8. SPOLOČNÉ NASTAVENIA -->
      <section id="tCommon" class="tab hide">

        <!-- 1. WiFi & Sieťové profily (najčastejšie používané pri prenášaní) -->
        <div class="card" id="cardWifi">
          <h2 data-i18n="wifi">📶 WiFi Pripojenie</h2>
          <p class="hint" id="wifiNow"></p>
          <div class="row"><label data-i18n="network">Názov siete (SSID)</label>
            <select id="ssid" style="flex:2 1 200px"></select>
            <button class="sec" onclick="scan()" data-i18n="scan">Vyhľadať</button>
          </div>
          <div class="row"><label data-i18n="password">Heslo siete</label><input type="password" id="wpass" style="flex:2 1 200px"></div>
          <div class="row"><label data-i18n="netHostLbl">Názov v sieti (Hostname)</label><input type="text" id="hostname" maxlength="32" placeholder="MeteoPlaneRadar" style="flex:2 1 200px"></div>
          <p class="hint" id="wifiHintTxt"></p>
          <div style="margin-top:12px;"><button onclick="saveWifi()" data-i18n="connect">Pripojiť k sieti</button></div>
          <table style="margin-top:14px;">
            <tr><td data-i18n="netHostLbl">Názov v sieti (Hostname):</td><td><span class="stat-val" id="netHost">MeteoPlaneRadar.local</span></td></tr>
            <tr><td data-i18n="netIpLbl">IP adresa:</td><td><span class="stat-val" id="netIp">-</span></td></tr>
            <tr><td data-i18n="netRssiLbl">Sila signálu (RSSI):</td><td><span class="stat-val" id="netRssi">-</span></td></tr>
            <tr><td data-i18n="netMacLbl">MAC adresa:</td><td><span class="stat-val" id="netMac">-</span></td></tr>
          </table>
          <div id="savedWifiWrap" style="margin-top:16px;border-top:1px solid var(--line);padding-top:14px;">
            <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:6px;">
              <h3 style="margin:0;font-size:14px;color:var(--acc);" data-i18n="savedWifiHdr">💾 Uložené WiFi siete (max 5)</h3>
              <span id="savedWifiCount" class="ver-pill">0 / 5</span>
            </div>
            <p class="hint" data-i18n="savedWifiHint" style="margin-bottom:10px;">Zariadenie si pamätá až 5 sietí (napr. doma a v práci) a pri štarte sa automaticky pripojí k najsilnejšej známej sieti.</p>
            <div id="savedWifiList" style="display:flex;flex-direction:column;gap:6px;"></div>
          </div>
        </div>

        <!-- 2. Jas displeja & Nočný režim -->
        <div class="card">
          <h2 data-i18n="brightness">☀️ Jas displeja & Nočný režim</h2>
          <div class="row"><label data-i18n="briDay">Denný jas</label><input type="range" id="briDay" min="10" max="100"><span id="briDayV" class="stat-val"></span></div>
          <div class="row"><label data-i18n="briNight">Nočný jas</label><input type="range" id="briNight" min="5" max="100"><span id="briNightV" class="stat-val"></span></div>
          <div class="row"><label class="chk"><input type="checkbox" id="nightAuto"><span data-i18n="nightAuto">Prepínať nočný režim automaticky podľa západu/východu slnka</span></label></div>
          <div class="row"><label class="chk"><input type="checkbox" id="ultraNight"><span data-i18n="ultraNight">🌙 Ultra Night režim (hlboká červená / spánkový monochróm, min. jas)</span></label></div>
          <div class="row"><label data-i18n="nightOffset">Posun voči východu/západu (minúty)</label><input type="number" id="nightOffset" min="-120" max="120"></div>
          <p class="hint" data-i18n="liveHint">Zmeny jasu sa ukladajú okamžite v reálnom čase.</p>
        </div>

        <!-- 3. Zvukové výstrahy & Bzučiak -->
        <div class="card">
          <h2 data-i18n="buzzerHdr">🔊 Zvukové výstrahy & Bzučiak</h2>
          <p class="hint" data-i18n="buzzerHint">Nastavenie vstavaného bzučiaka na doske pre radarové výstrahy a odozvu.</p>
          <div class="row">
            <label class="chk" style="font-weight:600;font-size:14px;">
              <input type="checkbox" id="buzzerOn">
              <span data-i18n="bzMaster">Povoliť bzučiak (hlavný vypínač)</span>
            </label>
          </div>
          <div class="grid" style="margin-top:10px;">
            <label class="chk"><input type="checkbox" id="bzEmergency"><span data-i18n="bzEmergency">🚨 Núdzový squawk (7700 / 7600 / 7500)</span></label>
            <label class="chk"><input type="checkbox" id="bzWatch"><span data-i18n="bzWatch">⭐ Sledovaný let (vstup do dosahu)</span></label>
            <label class="chk"><input type="checkbox" id="bzTouch"><span data-i18n="bzTouch">👆 Akustická odozva na dotyk displeja</span></label>
            <label class="chk"><input type="checkbox" id="bzHourly"><span data-i18n="bzHourly">🕒 Pípnutie na celú hodinu (chime)</span></label>
            <label class="chk"><input type="checkbox" id="bzOverhead"><span data-i18n="bzOverhead">🔊 Prelet nad hlavou (Overhead alert)</span></label>
            <label class="chk"><input type="checkbox" id="bzPrecip"><span data-i18n="bzPrecip">🌧️ Blížiace sa zrážky (Nowcasting alert)</span></label>
            <label class="chk"><input type="checkbox" id="bzNightMute"><span data-i18n="bzNightMute">🌙 Nočný kľud (stíšiť bzučiak v noci)</span></label>
          </div>
          <div style="margin-top:12px;">
            <button type="button" class="sec" onclick="testBuzzer()" data-i18n="btnTestBuzzer">🔊 Otestovať bzučiak</button>
          </div>
        </div>

        <!-- 4. Automatické striedanie obrazoviek -->
        <div class="card">
          <h2 data-i18n="rotateHdr">🔄 Automatické striedanie obrazoviek</h2>
          <div class="row"><label data-i18n="autoRotate">Čas zobrazenia obrazovky (sekundy, 0 = vypnuté)</label>
            <input type="number" id="autoRotate" min="0" max="3600" step="5">
          </div>
          <p class="hint" data-i18n="rotHint">Striedanie pozastaví potiahnutie prstom alebo prepnutie z prehliadača. Otvorený detail lietadla striedanie pozastaví.</p>
        </div>

        <!-- 5. Orientácia, Kompas & Jednotky -->
        <div class="card">
          <h2 data-i18n="planesView">🧭 Orientácia, Kompas & Jednotky</h2>
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
          <div class="grid" style="margin-top:10px;">
            <label class="chk"><input type="checkbox" id="rCompassCommon" onchange="$('rCompass').checked=this.checked;autoSave('rCompass',this.checked)"><span data-i18n="rCompass">Elektronický kompas (miniatúra na mape)</span></label>
            <label class="chk"><input type="checkbox" id="autoRotateBearing"><span data-i18n="autoRotateBearing">Auto-rotácia radaru podľa kompasu (Live Heading)</span></label>
            <label class="chk"><input type="checkbox" id="metric"><span data-i18n="metric">Metrické jednotky (km, km/h, m namiesto NM, kt, ft)</span></label>
          </div>
          <p class="hint" data-i18n="planesViewHint">Nastavte smer podľa toho, kam smeruje váš výhľad. Meteoradar sa zámerne orientuje na sever.</p>
          <p class="hint" data-i18n="compassHint" style="margin-top:6px;">Elektronický kompas (QMI8658) umožňuje zobraziť miniatúru kompasu v rohu mapy, alebo dynamicky otáčať celú radarovú mapu podľa natočenia zariadenia. Klepnutím na kompas na displeji nakalibrujete sever.</p>
        </div>

        <!-- 6. Časové pásmo & Posun GMT -->
        <div class="card">
          <h2 data-i18n="tzHdr">🕒 Časové pásmo & Posun GMT</h2>
          <div class="row"><label data-i18n="tzSelectLbl">Časové pásmo / Posun</label>
            <select id="timezone" style="flex:2 1 240px">
              <optgroup label="Európske časové pásma (automatický letný/zimný čas)">
                <option value="CET-1CEST,M3.5.0,M10.5.0/3">Stredoeurópsky čas (CET/CEST: SK, CZ, AT, DE... UTC+1 / leto UTC+2)</option>
                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Východoeurópsky čas (EET/EEST: UA, FI, GR, RO... UTC+2 / leto UTC+3)</option>
                <option value="GMT0BST,M3.5.0/1,M10.5.0">Západoeurópsky čas / UK (WET/WEST: UK, IE, PT... UTC+0 / leto UTC+1)</option>
              </optgroup>
              <optgroup label="Svetový referenčný čas">
                <option value="UTC0">UTC / Zulu / GMT (UTC+0 bez zmeny času)</option>
              </optgroup>
              <optgroup label="Fixné posuny voči UTC (bez zmeny času)">
                <option value="<-12>12">UTC-12:00</option>
                <option value="<-11>11">UTC-11:00 (Samoa)</option>
                <option value="<-10>10">UTC-10:00 (Hawaii)</option>
                <option value="<-09>9">UTC-09:00 (Alaska)</option>
                <option value="<-08>8">UTC-08:00 (PST / US Pacific)</option>
                <option value="<-07>7">UTC-07:00 (MST / US Mountain)</option>
                <option value="<-06>6">UTC-06:00 (CST / US Central)</option>
                <option value="<-05>5">UTC-05:00 (EST / US Eastern)</option>
                <option value="<-04>4">UTC-04:00 (AST / Atlantic)</option>
                <option value="<-03>3">UTC-03:00 (Brazília, Argentína)</option>
                <option value="<-02>2">UTC-02:00</option>
                <option value="<-01>1">UTC-01:00 (Azory)</option>
                <option value="<+01>-1">UTC+01:00 (CET fixný)</option>
                <option value="<+02>-2">UTC+02:00 (EET fixný)</option>
                <option value="<+03>-3">UTC+03:00 (Moskva, Saudská Arábia, Turecko)</option>
                <option value="<+0330>-3:30">UTC+03:30 (Teherán)</option>
                <option value="<+04>-4">UTC+04:00 (Dubaj, Baku)</option>
                <option value="<+0430>-4:30">UTC+04:30 (Kábul)</option>
                <option value="<+05>-5">UTC+05:00 (Pakistan, Uzbekistan)</option>
                <option value="<+0530>-5:30">UTC+05:30 (India, Srí Lanka)</option>
                <option value="<+06>-6">UTC+06:00 (Bangladéš, Astana)</option>
                <option value="<+07>-7">UTC+07:00 (Bangkok, Jakarta)</option>
                <option value="<+08>-8">UTC+08:00 (Singapur, Peking, Perth)</option>
                <option value="<+09>-9">UTC+09:00 (Tokio, Soul)</option>
                <option value="<+0930>-9:30">UTC+09:30 (Adelaide)</option>
                <option value="<+10>-10">UTC+10:00 (Sydney, Melbourne)</option>
                <option value="<+11>-11">UTC+11:00 (Šalamúnove ostrovy)</option>
                <option value="<+12>-12">UTC+12:00 (Auckland, Fidži)</option>
                <option value="<+13>-13">UTC+13:00 (Tonga, Samoa)</option>
                <option value="<+14>-14">UTC+14:00 (Kiritimati)</option>
              </optgroup>
            </select>
            <button type="button" class="sec" onclick="detectBrowserTz()" data-i18n="btnDetectTz">🌐 Zistiť z prehliadača</button>
          </div>
          <p class="hint" data-i18n="tzHint">Určuje posun času voči UTC pre hodiny, radarové snímky a predpoveď počasia. Pre Slovensko a Česko zvoľte CET/CEST (automatický letný a zimný čas).</p>
        </div>

        <!-- 7. Domovská poloha -->
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

        <div class="card" id="cardGithubOta">
          <h2 data-i18n="otaHdr">🚀 Aktualizácia firmvéru (GitHub OTA)</h2>
          <table>
            <tr><td data-i18n="otaCurLbl">Nainštalovaná verzia:</td><td><span class="stat-val" id="otaCurVer">-</span></td></tr>
            <tr><td data-i18n="otaGitLbl">Najnovšia verzia na GitHube:</td><td><span class="stat-val" id="otaLatVer">-</span> <span id="otaTagBadge" class="pill hide"></span></td></tr>
          </table>
          <div id="otaReleaseNotes" class="hide" style="margin-top:10px;padding:10px;background:rgba(255,255,255,0.04);border-radius:6px;max-height:160px;overflow-y:auto;font-size:12px;white-space:pre-wrap;color:#cbd5e1;border:1px solid rgba(255,255,255,0.08);font-family:monospace;"></div>
          <div style="margin-top:14px;display:flex;gap:8px;flex-wrap:wrap;align-items:center;">
            <button type="button" class="sec" id="btnOtaCheck" onclick="checkGithubUpdates(true)" data-i18n="btnOtaCheck">🔍 Skontrolovať aktualizácie</button>
            <button type="button" class="hide" id="btnOtaInstall" onclick="startGithubOta()" style="background:#22c55e;color:#000;font-weight:700;" data-i18n="btnOtaInstall">⚡ Aktualizovať z GitHubu</button>
            <span id="otaSpinner" class="hide" style="font-size:13px;color:#94a3b8;">⏳ <span id="otaStatusTxt">...</span></span>
          </div>
          <div id="otaProgWrap" class="hide" style="margin-top:14px;">
            <div style="display:flex;justify-content:space-between;font-size:12.5px;margin-bottom:4px;">
              <span data-i18n="otaProgLbl">Priebeh inštalácie:</span>
              <span class="stat-val" id="otaPctTxt">0%</span>
            </div>
            <div class="prog-bar"><div class="prog-fill" id="otaProgBar" style="width:0%;background:linear-gradient(90deg,#06b6d4,#10b981);"></div></div>
            <p class="hint" data-i18n="otaWarn" style="margin-top:6px;color:#f59e0b;">⚠️ Zariadenie nevypínajte. Po dokončení zápisu sa zariadenie automaticky reštartuje.</p>
          </div>
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

    <!-- ================= PRAVÝ STĹPEC: HARDVÉR & DIAĽKOVÝ OVLÁDAČ (STÁLE VIDITEĽNÝ) ================= -->
    <aside class="hw-col">

      <!-- Rýchle ovládanie displeja -->
      <div class="card" id="cardRemote">
        <h2 data-i18n="remote">🎮 Diaľkový ovládač</h2>
        <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:10px;">
          <span style="font-size:12.5px;color:var(--mut);" data-i18n="liveDispLbl">Na displeji:</span>
          <span class="pill pill-ok" id="hwCurScreenName">Hodiny</span>
        </div>
        <div class="row" style="justify-content:center;gap:6px;margin:8px 0;">
          <button class="sec" onclick="stepScreen(-1)" data-i18n="btnPrev" style="flex:1;">&#8592; Predch.</button>
          <button class="sec" onclick="toggleLegendsRemote()" style="background:#1d293d;color:var(--acc);border-color:var(--acc);flex:1;" data-i18n="btnDblTap" title="Dvojklik / Legenda">🔄 Legenda</button>
          <button class="sec" onclick="stepScreen(1)" data-i18n="btnNext" style="flex:1;">Nasled. &#8594;</button>
        </div>
        <div class="range-ctrl-row" id="rangeControlRow">
          <span data-i18n="rangeLbl" class="range-lbl">Mierka:</span>
          <div class="range-stepper">
            <button type="button" class="btn-step" id="rMinus" onclick="stepRange(-1)" data-i18n-title="btnDec" title="Priblížiť (− km)">−</button>
            <span id="rangeNow" class="range-val">–</span>
            <button type="button" class="btn-step" id="rPlus" onclick="stepRange(1)" data-i18n-title="btnInc" title="Oddialiť (+ km)">+</button>
          </div>
        </div>
        <div class="row" id="scrBtns" style="justify-content:center;gap:5px;margin-top:10px;"></div>
      </div>

      <!-- Snímka displeja -->
      <div class="card" id="cardScreenshot">
        <h2 data-i18n="scrShotHdr">📸 Snímka displeja</h2>
        <div style="text-align:center;margin:10px 0;">
          <div id="scrShotWrap" style="position:relative;display:inline-block;margin:0 auto;">
            <img id="scrShotImg" class="hide" style="width:200px;height:200px;border-radius:50%;border:2px solid var(--acc);background:#000;margin:0 auto;box-shadow:0 4px 16px rgba(0,0,0,0.6);object-fit:cover;" alt="Display Screenshot">
            <div id="scrShotPlaceholder" style="width:200px;height:200px;border-radius:50%;border:2px dashed var(--line);display:flex;align-items:center;justify-content:center;margin:0 auto;color:var(--mut);font-size:13px;" data-i18n="scrShotNone">Kliknite pre zachytenie</div>
          </div>
        </div>
        <div style="display:flex;gap:8px;justify-content:center;margin-top:10px;flex-wrap:wrap;">
          <button type="button" class="sec" id="btnScrShot" onclick="takeScreenshot()" data-i18n="btnTakeScrShot" style="flex:1;">📸 Zachytiť obrazovku</button>
          <a id="scrShotDl" href="/api/screenshot.bmp" download="screenshot.bmp" class="hide" style="text-decoration:none;"><button type="button" class="sec" data-i18n="btnDlScrShot">💾 Stiahnuť</button></a>
        </div>
        <div id="scrShotSpin" class="hide" style="text-align:center;font-size:12px;color:var(--mut);margin-top:6px;">⏳ Generujem snímku...</div>
      </div>

      <!-- ESP32-S3 CPU & Teplota -->
      <div class="card">
        <h2 data-i18n="hwCpu">⚡ ESP32-S3 & Teplota</h2>
        <table>
          <tr><td data-i18n="hwChipModel">Model čipu:</td><td><span class="stat-val" id="hwCpuModel">-</span> (<span data-i18n="hwRev">Rev</span> <span id="hwCpuRev">-</span>, <span id="hwCpuCores">-</span> <span data-i18n="hwCores">jadrá</span>)</td></tr>
          <tr><td data-i18n="hwCpuFreqLbl">Frekvencia CPU:</td><td><span class="stat-val" id="hwCpuFreq">-</span> MHz</td></tr>
          <tr><td data-i18n="hwCpuTempLbl">Teplota CPU:</td><td><span class="stat-val" id="hwCpuTemp">-</span> °C</td></tr>
          <tr><td data-i18n="hwResetLbl">Dôvod reštartu:</td><td><span class="pill pill-ok" id="hwReset">-</span></td></tr>
          <tr><td data-i18n="hwUptimeLbl">Doba behu (Uptime):</td><td><span class="stat-val" id="hwUptime">-</span></td></tr>
        </table>
      </div>

      <!-- Pamäť & PSRAM -->
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

      <!-- 6-Axis IMU senzor -->
      <div class="card">
        <h2 data-i18n="hwSensors">🧭 6-Axis IMU Senzor (QMI8658)</h2>
        <table>
          <tr><td data-i18n="hwImuLbl">Stav senzora:</td><td><span class="pill pill-ok" id="hwImuState" data-i18n="hwImuActive">Aktívny (I2C 0x6B)</span></td></tr>
          <tr><td data-i18n="hwAxLbl">Akcelerometer (g):</td><td><span class="stat-val">X: <span id="hwAx">0</span> | Y: <span id="hwAy">0</span> | Z: <span id="hwAz">0</span></span></td></tr>
          <tr><td data-i18n="hwGxLbl">Gyroskop (°/s):</td><td><span class="stat-val">X: <span id="hwGx">0</span> | Y: <span id="hwGy">0</span> | Z: <span id="hwGz">0</span></span></td></tr>
          <tr><td data-i18n="hwTiltLbl">Náklon (Pitch / Roll):</td><td><span class="stat-val">Pitch: <span id="hwPitch">0</span>° | Roll: <span id="hwRoll">0</span>°</span></td></tr>
          <tr><td data-i18n="hwDblTapLbl">Gesto poklepania:</td><td><span class="pill pill-ok" data-i18n="hwDblTapOn">Double-Tap zapnuté</span></td></tr>
        </table>
      </div>

      <!-- Hardware RTC -->
      <div class="card">
        <h2 data-i18n="hwRtc">⏱️ Systémový čas & RTC (PCF85063)</h2>
        <table>
          <tr><td data-i18n="hwLocalTimeLbl">Miestny čas:</td><td><span class="stat-val" id="hwLocalTime" style="color:var(--acc);font-weight:700;">-</span></td></tr>
          <tr><td data-i18n="hwTzLbl">Pásmo / Posun:</td><td><span class="stat-val" id="hwTzOffset">-</span></td></tr>
          <tr><td data-i18n="hwRtcTimeLbl">Čas v RTC čipe:</td><td><span class="stat-val" id="hwRtcTime">-</span></td></tr>
          <tr><td data-i18n="hwRtcLbl">Stav RTC čipu:</td><td><span class="pill pill-ok" id="hwRtcState">Aktívny (I2C 0x51)</span></td></tr>
        </table>
        <div style="margin-top:12px;display:flex;gap:8px;flex-wrap:wrap;">
          <button class="sec" onclick="syncRtcNtp()" data-i18n="btnSyncNtp" style="flex:1;">🌐 NTP sync</button>
          <button class="sec" onclick="syncRtcBrowser()" data-i18n="btnSyncBrowser" style="flex:1;">💻 Z prehliadača</button>
        </div>
      </div>

      <!-- Periférie & I2C Zbernica -->
      <div class="card">
        <h2 data-i18n="hwPeripherals">🔌 Periférie & Displej</h2>
        <table>
          <tr><td data-i18n="hwDispLbl">Displej:</td><td><span class="stat-val" id="hwDisp">ST7701 (480x480 RGB 16-bit)</span></td></tr>
          <tr><td data-i18n="hwTouchLbl">Dotykový panel:</td><td><span class="stat-val" id="hwTouch">CST820 (I2C 0x15)</span></td></tr>
          <tr><td data-i18n="hwExpLbl">I/O Expandér:</td><td><span class="stat-val" id="hwExp">TCA9554 (I2C 0x20)</span></td></tr>
        </table>
      </div>

    </aside>

  </div>
</div>

<div class="bar" id="saveBar">
  <button onclick="save()" data-i18n="save">💾 Uložiť nastavenia</button>
  <span id="msg"></span>
</div>

<script>
const D={
 cs:{
  tabScrClock:"🕒 Hodiny",tabScrPlanes:"✈️ Letadla",tabScrMeteo:"🌧️ Meteoradar",tabScrTactical:"🎯 Taktický radar",tabScrForecast:"⛅ Předpověď",tabScrInfo:"ℹ️ Info & Statistiky",tabScrFinance:"📈 Trhy & Krypto",tabCommon:"⚙️ Společná nastavení",
  scrClockHdr:"Hodiny & Astro",scrPlanesHdr:"Letadla radar (ADS-B)",scrMeteoHdr:"Srážkový meteoradar",scrTacticalHdr:"Taktický radar (Letadla + Srážky)",scrForecastHdr:"Předpověď počasí",scrFinanceHdr:"Finanční trhy, Akcie & Krypto",
  btnShowOnDisp:"Zobrazit na displeji",liveDispLbl:"Na displeji:",
  scrClockActive:"Zahrnout obrazovku do automatického střídání",scrPlanesActive:"Zahrnout obrazovku do automatického střídání",scrMeteoActive:"Zahrnout obrazovku do automatického střídání",scrTacticalActive:"Zahrnout obrazovku do automatického střídání",scrForecastActive:"Zahrnout obrazovku do automatického střídání",scrFinanceActive:"Zahrnout obrazovku do automatického střídání",
  rotateHdr:"🔄 Automatické střídání obrazovek",
  remote:"🎮 Dálkové ovládání",rangeLbl:"Měřítko:",
  btnPrev:"← Předchozí",btnDblTap:"🔄 Legenda",btnNext:"Následující →",btnDec:"Přiblížit (− km)",btnInc:"Oddálit (+ km)",
  remoteHint:"Rozsah se mění na obrazovkách Letadla, Meteoradar a Taktický radar. Zásah pozastaví automatické střídání.",
  location:"📍 Domovská poloha",findCity:"Vyhledat město",search:"Hledat",found:"Nalezené výsledky",lat:"Zeměpisná šířka (°N)",lon:"Zeměpisná délka (°E)",
  locHint:"Změna polohy vyžaduje restart pro přepočet map a předpovědi.",
  tzHdr:"🕒 Časové pásmo & Posun GMT",tzSelectLbl:"Časové pásmo / Posun GMT",btnDetectTz:"🌐 Zjistit z prohlížeče",
  tzHint:"Určuje posun času vůči UTC pro hodiny, radarové snímky a předpověď počasí. Pro ČR a SR zvolte CET/CEST (automatický letní a zimní čas).",
  planesView:"🧭 Orientace, Kompas & Jednotky",topBearing:"Směr nahoře na radaru",metric:"Metrické jednotky (km, km/h, m místo NM, kt, ft)",
  tb0:"Sever (Sever nahoře / North-Up)",tb45:"Severovýchod (45°)",tb90:"Východ (90°)",tb135:"Jihovýchod (135°)",tb180:"Jih (180°)",tb225:"Jihozápad (225°)",tb270:"Západ (270°)",tb315:"Severozápad (315°)",
  planesViewHint:"Nastavte směr podle toho, kam se díváte z okna. Meteoradar je orientován na sever.",
  compassHint:"Elektronický kompas (QMI8658) umožňuje buď zobrazit miniaturu kompasu v rohu mapy, nebo dynamicky otáčet celou mapu podle fyzického natočení displeje. Klepnutím na kompas na displeji zkalibrujete sever.",
  autoRotateBearing:"Auto-rotace mapy podle kompasu (Live Heading)",
  autoRotate:"Automatické střídání (sekundy, 0 = vypnuto)",
  rotHint:"Střídání pozastaví potažení prstem nebo přepnutí z prohlížeče. Otevřený detail letadla střídání drží.",
  radar:"🌧️ Meteoradar & Zobrazení",radarSrc:"Zdroj radarových dat",srcRv:"RainViewer (Evropa a svět)",srcChmu:"ČHMÚ (velmi ostrá data, jen ČR)",srcShmu:"SHMÚ (velmi ostrá data, Slovensko)",
  showAltBar:"Výšková lišta letových hladin",showPrecipBar:"Barevná stupnice srážek (nebo poklepání)",
  smoothRadar:"Vyhlazení radarových dat (bilineární interpolace pro ČHMÚ a SHMÚ)",
  radarHint:"V ČR použijte ČHMÚ, na Slovensku SHMÚ, jinde ve světě RainViewer.",
  planes:"✈️ ADS-B Filtry & Sledování",altMin:"Minimální letová výška (ft)",altMax:"Maximální letová výška (ft)",onlyCs:"Jen letadla s volacím znakem (callsign)",
  sqAlert:"Zvýraznit a upozornit na nouzové squawky (7500/7600/7700)",watch:"Sledovaný let (Callsign nebo ICAO hex)",
  planesHint:"Filtry se týkají jen kreslení. Nouzový squawk ani sledované letadlo neschovají.",
  typeFiltersTitle:"🎯 Filtr typů strojů na radaru",typeFiltersHint:"Vyberte kategorie strojů, které se mají zobrazovat na radarové mapě. Nouzové a sledované lety se zobrazí vždy.",
  tfAirliner:"✈️ Dopravní letadla (A320, B737...)",tfLight:"🛩️ Malá a sportovní letadla (Cessna, Piper...)",
  tfHeli:"🚁 Vrtulníky a záchranné složky (HEMS, ATE...)",tfMil:"⚔️ Vojenské letectvo a stíhačky",
  tfHeavy:"🛫 Těžké obří letouny (A380, B747, Beluga...)",tfGlider:"🪂 Větroně a kluzáky (Gliders)",
  buzzerHdr:"🔊 Zvukové výstrahy & Bzučák",buzzerHint:"Nastavení vestavěného bzučáku na desce pro radarové výstrahy a odezvu.",
  bzMaster:"Povolit bzučák (hlavní vypínač)",bzEmergency:"🚨 Nouzový squawk (7700 / 7600 / 7500)",bzWatch:"⭐ Sledovaný let (vstup do dosahu)",
  bzTouch:"👆 Akustická odezva na dotyk displeje",bzHourly:"🕒 Pípnutí v celou hodinu (chime)",bzNightMute:"🌙 Noční klid (ztlumit bzučák v noci)",btnTestBuzzer:"🔊 Otestovat bzučák",
  brightness:"☀️ Jas displeje & Noční režim",briDay:"Denní jas",briNight:"Noční jas",nightAuto:"Přepínat noční režim automaticky podle slunce",
  nightOffset:"Posun proti východu/západu (minuty)",clockHdr:"🕒 Ciferník hodin",secStyle:"Styl vteřinového prstence",
  secOff:"Vypnuto",secDots:"Tečky (Dots)",secLine:"Plná čára (Line)",secComet:"Kometa (Comet)",
  secRadar:"Radarový paprsek (Sweep)",secTicks:"Hodinářské indexy (Ticks)",secOrbit:"Satelit na orbitě (Orbit)",
  clockColor:"Barva číslic hodin",secColor:"Barva vteřinového prstence",
  clockStyle:"Styl ciferníku",clkDigital:"Digitální klasický",clkAnalog:"Letecký kokpitový analog (Aviator)",clkOrbital:"Planetární prstence (Orbital Gauges)",clkHud:"Stíhací průhledový displej (Fighter HUD)",clkRegulator:"Astronomický regulátor (Régulateur)",clkStacked:"Vertikální typografie (Stacked Bold)",clkMinimal:"Minimalistický moderní (Nordic)",
  clockWidgets:"Prvky na obrazovce hodin",cDate:"Datum",cWx:"Počasí & teplota",cWind:"Rychlost větru",cMoon:"Fáze měsíce",cAstro:"24h solární prstenec",nightClockOnly:"V noci pouze Hodiny (zastavit radary)",
  radarWidgets:"Prvky radarových map",rTrails:"Trajektorie letadel (Trails)",rNearest:"Vektor k nejbližšímu letadlu",rAirports:"Letiště (Runway ikony)",rRings:"Kilometrové kružnice dosahu",rCompass:"Elektronický kompas (miniatura na mapě)",
  tacticalHdr:"🎯 Taktické zobrazení",tacticalDesc:"Kombinovaný taktický radar spojuje ADS-B lety a bouřkové radarové odrazy do jedné společné obrazovky v reálném čase.",tacticalHint:"Filtry výšky a volacích znaků se přebírají z nastavení Letadel, zdroj srážek z Meteoradaru.",
  forecastHdr:"⛅ Předpověď počasí",forecastDesc:"Předpověď počasí se automaticky stahuje ze služby Open-Meteo pro vaši domovskou polohu.",btnGoLocSettings:"📍 Nastavit domovskou polohu ve Společných nastaveních",
  hwRtc:"⏱️ Systémový čas & RTC (PCF85063)",hwLocalTimeLbl:"Místní čas:",hwTzLbl:"Pásmo / Posun:",hwRtcLbl:"Stav RTC čipu:",hwRtcTimeLbl:"Čas v RTC čipu:",btnSyncNtp:"🌐 NTP sync",btnSyncBrowser:"💻 Z prohlížeče",hwI2c:"🔍 I2C Sběrnice (Bus Inspector)",
  liveHint:"Změny jasu a prvků se ukládají okamžitě v reálném čase.",
  hwCpu:"⚡ ESP32-S3 & Teplota",hwChipModel:"Model čipu:",hwRev:"Rev",hwCores:"jádra",hwCpuFreqLbl:"Frekvence CPU:",hwCpuTempLbl:"Teplota CPU:",hwResetLbl:"Důvod restartu:",hwUptimeLbl:"Doba běhu (Uptime):",
  hwMem:"💾 Paměť & Úložiště",hwHeapLbl:"Interní RAM (Heap):",hwPsramLbl:"Octal PSRAM (8 MB):",hwFlashLbl:"Flash paměť:",
  hwSensors:"🧭 6-Axis IMU Senzor (QMI8658)",hwImuLbl:"Stav senzoru:",hwImuActive:"Aktivní (I2C 0x6B)",hwAxLbl:"Akcelerometr (g):",hwGxLbl:"Gyroskop (°/s):",hwTiltLbl:"Náklon (Pitch / Roll):",hwDblTapLbl:"Gesto poklepání:",hwDblTapOn:"Double-Tap zapnuto",
  hwPeripherals:"🔌 Periferie & Displej",hwDispLbl:"Displej:",hwTouchLbl:"Dotykový panel:",hwExpLbl:"I/O Expandér:",
  wifi:"📶 WiFi Připojení",network:"Název sítě (SSID)",password:"Heslo sítě",scan:"Vyhledat",connect:"Připojit k síti",
  savedWifiHdr:"💾 Uložené WiFi sítě (max 5)",savedWifiHint:"Zařízení si pamatuje až 5 sítí (např. doma a v práci) a při startu se automaticky připojí k nejsilnější dostupné síti.",
  delWifiConfirm:"Opravdu chcete zapomenout WiFi síť",activeNet:"(aktivní)",btnForgetNet:"Smazat",noSavedWifi:"Žádné uložené sítě",
  netHostLbl:"Název v síti (Hostname):",netIpLbl:"IP adresa:",netRssiLbl:"Síla signálu (RSSI):",netMacLbl:"MAC adresa:",
  wifiHint:"Po uložení se zařízení připojí a přístupový bod zmizí.",
  wifiHintSta:"Změna sítě přeruší spojení. Při neúspěchu zařízení vytvoří vlastní síť MeteoPlaneRadar.",
  wifiNow:"Připojeno k síti",system:"🔒 Zabezpečení správce",adminPass:"Současné heslo",newPass:"Nové heslo",
  passHint:"Současné heslo je potřeba pro aktualizaci, import, reset i pro změnu hesla. Prázdné nové heslo nic nemění; jedna mezera ochranu zruší.",
  statusHdr:"🛠️ Údržba & Záloha",fwUpdate:"Aktualizace firmwaru",export:"Export nastavení",import:"Import nastavení",reboot:"Restartovat",factory:"Tovární reset",save:"💾 Uložit nastavení",
  pwNone:"Zatím není nastavené žádné heslo — aktualizace, import a reset jsou otevřené.",pwSet:"Heslo je nastavené.",
  wrongPass:"Chybné heslo",doneReboot:"Hotovo. Restartuji...",importOk:"Import úspěšný. Restartuji...",
  autoSaved:"Uloženo",saved:"Uloženo",failed:"Nepovedlo se",searching:"Hledám…",nothing:"Nic nenalezeno",
  disabled:"Obrazovka je vypnutá",confirmReset:"Opravdu smazat všechna nastavení včetně WiFi?",
  otaHdr:"🚀 Aktualizace firmwaru (GitHub OTA)",otaCurLbl:"Nainstalovaná verze:",otaGitLbl:"Nejnovější verze na GitHubu:",
  btnOtaCheck:"🔍 Zkontrolovat aktualizace",btnOtaInstall:"⚡ Aktualizovat z GitHubu",otaProgLbl:"Průběh instalace:",
  otaWarn:"⚠️ Zařízení nevypínejte. Po dokončení zápisu se zařízení automaticky restartuje.",
  otaUpToDate:"Máte nejnovější verzi",otaNewAvail:"K dispozici je nová verze!",otaChecking:"Kontroluji GitHub...",
  otaDownloading:"Stahování a zápis firmwaru...",otaSuccess:"Aktualizace úspěšná! Restartuji...",otaErr:"Chyba aktualizace",
  otaNoAsset:"Vydání neobsahuje soubor OTA (-ota.bin)",otaConfirm:"Opravdu spustit aktualizaci firmwaru na verzi",
  scrClock:"Hodiny & Astro",scrPlanes:"Letadla radar",scrMeteo:"Meteoradar",scrTactical:"Taktický radar",scrForecast:"Předpověď počasí",scrInfo:"Info & Statistiky",scrFinance:"Trhy & Krypto",scrSettings:"Nastavení",
  finSlotsHdr:"📈 Sledované trhy & aktiva (4 pozice)",finSlot1:"1. Hlavní trh (Hero graf)",finSlot1Sub:"Velký reálný graf + kurz",finSlot2:"2. Sledovaný trh",finSlot3:"3. Sledovaný trh",finSlot4:"4. Sledovaný trh",finSlotWatchlist:"Karta v dolním přehledu",finPresets:"⚡ Rychlé přidání populárních aktiv:",finClear:"Vymazat",financeHint:"1. pozice má velký graf (Hero), pozice 2–4 se zobrazují v dolním přehledu. Podporuje evropské ETF fondy (Amundi CW8.PA, 500.PA, Vanguard VWCE.DE), krypto, indexy, komodity i světové akcie z Yahoo Finance.",
  ultraNight:"🌙 Ultra Night režim (hluboká červená / spánkový monochróm, min. jas)",
  tabScrInfo:"ℹ️ Info & Statistiky",scrInfoHdr:"Info & Denní statistika letů",scrInfoActive:"Zahrnout obrazovku do automatického střídání",
  statsTrafficHdr:"✈️ Dnešní letecká statistika",btnResetStats:"🔄 Resetovat",stUnique:"Unikátní letadla dnes:",stTopSpeed:"Nejvyšší rychlost:",stMaxDist:"Maximální vzdálenost:",stAltSpan:"Rozpětí výšek:",stReports:"Přijaté ADS-B zprávy:",statsHint:"Statistika se automaticky nuluje o půlnoci a uchovává se v paměti PSRAM.",confirmResetStats:"Opravdu resetovat dnešní statistiku letů?",statsResetOk:"Statistiky byly resetovány",
  cOver:"✈️ Let nad hlavou (Overhead widget)",ovRad:"Poloměr přeletu nad hlavou (km)",bzOverhead:"🔊 Přelet nad hlavou (Overhead výstraha)",
   cPrecip:"🌧️ Výstraha blížících se srážek",bzPrecip:"🔊 Pípnutí při blížících se srážkách",precipTrackerHdr:"🌧️ Detekce blížících se srážek (Nowcasting)",precipHint:"Vektorová analýza pohybu frontu (TREC). Upozorní na déšť, kroupy nebo sníh, pouze pokud srážky směřují přímo k vaší poloze.",precipLiveHdr:"Aktuální stav nowcastingu:",
  scrShotHdr:"📸 Snímek displeje",scrShotNone:"Klikněte pro zachycení",btnTakeScrShot:"📸 Zachytit obrazovku",btnDlScrShot:"💾 Stáhnout BMP",scrShotOk:"Snímek úspěšně načten",scrShotErr:"Chyba načtení snímku",
  tabScrIss:"🛰️ ISS Tracker",scrIssHdr:"ISS Tracker (Mezinárodní vesmírná stanice)",scrIssActive:"Zahrnout obrazovku do automatického střídání",scrIss:"ISS Tracker",issAlertLbl:"🔊 Zvuková výstraha při přeletu stanice nad obzorem (v dosahu / nad hlavou)",issTelemetryHdr:"🛰️ Živá telemetrie stanice ISS",issStateLbl:"Aktuální stav:",issLatLonLbl:"Poloha (Lat / Lon):",issAltLbl:"Výška letu:",issVelLbl:"Rychlost:",issDistLbl:"Vzdálenost:",issElLbl:"Elevace / Azimut:",issSunLbl:"Osvětlení:",issSunlit:"☀️ Na denním světle",issEclipsed:"🌑 Ve stínu Země",issInRange:"🛰️ V dosahu přímé viditelnosti",issOverhead:"⭐ PŘÍMO NAD HLAVOU (> 45°)",issOutRange:"Mimo dosah horizontu",btnIssRefresh:"🔄 Aktualizovat telemetrii",issHint:"Sledování přeletu Mezinárodní vesmírné stanice ISS v reálném čase (API WhereTheISS). Zobrazuje mapu světa se dnem/nocí, orbitální dráhu, horizont přímé viditelnosti (~2200 km) a odpočet dalšího přeletu.",
   tabSerial:"📟 Sériový monitor",serialHdr:"Sériový monitor (Live Web Console)",btnSerialStart:"Spustit monitor",btnSerialPause:"Pozastavit monitor",serialActive:"Aktivní (Live)",serialPaused:"Pozastaveno",serialAutoScroll:"Automatický posun",btnSerialCopy:"📋 Kopírovat",btnSerialDl:"💾 Stáhnout",btnSerialClear:"🧹 Vymazat",btnSerialSend:"Odeslat ↵",serialCopied:"Výpis zkopírován do schránky",serialCleared:"Konzole vymazána",serialHint:"Streamování výstupů sériového portu přes WiFi bez nutnosti USB kabelu. Při odchodu ze záložky se přenos automaticky pozastaví."
 },
 sk:{
  tabScrClock:"🕒 Hodiny",tabScrPlanes:"✈️ Lietadlá",tabScrMeteo:"🌧️ Meteoradar",tabScrTactical:"🎯 Taktický radar",tabScrForecast:"⛅ Predpoveď",tabScrInfo:"ℹ️ Info & Štatistiky",tabScrFinance:"📈 Trhy & Krypto",tabCommon:"⚙️ Spoločné nastavenia",
  scrClockHdr:"Hodiny & Astro",scrPlanesHdr:"Lietadlá radar (ADS-B)",scrMeteoHdr:"Zrážkový meteoradar",scrTacticalHdr:"Taktický radar (Lietadlá + Zrážky)",scrForecastHdr:"Predpoveď počasia",scrFinanceHdr:"Finančné trhy, Akcie & Krypto",
  btnShowOnDisp:"Zobraziť na displeji",liveDispLbl:"Na displeji:",
  scrClockActive:"Zahrnúť obrazovku do automatického striedania",scrPlanesActive:"Zahrnúť obrazovku do automatického striedania",scrMeteoActive:"Zahrnúť obrazovku do automatického striedania",scrTacticalActive:"Zahrnúť obrazovku do automatického striedania",scrForecastActive:"Zahrnúť obrazovku do automatického striedania",scrFinanceActive:"Zahrnúť obrazovku do automatického striedania",
  rotateHdr:"🔄 Automatické striedanie obrazoviek",
  remote:"🎮 Diaľkový ovládač",rangeLbl:"Mierka:",
  btnPrev:"← Predchádzajúca",btnDblTap:"🔄 Legenda",btnNext:"Nasledujúca →",btnDec:"Priblížiť (− km)",btnInc:"Oddialiť (+ km)",
  remoteHint:"Rozsah sa mení na obrazovkách Lietadlá, Meteoradar a Taktický radar. Zásah pozastaví automatické striedanie.",
  location:"📍 Domovská poloha",findCity:"Vyhľadať mesto",search:"Hľadať",found:"Nájdené výsledky",lat:"Zemepisná šírka (°N)",lon:"Zemepisná dĺžka (°E)",
  locHint:"Zmena polohy vyžaduje reštart pre prepočet máp a predpovede.",
  tzHdr:"🕒 Časové pásmo & Posun GMT",tzSelectLbl:"Časové pásmo / Posun GMT",btnDetectTz:"🌐 Zistiť z prehliadača",
  tzHint:"Určuje posun času voči UTC pre hodiny, radarové snímky a predpoveď počasia. Pre Slovensko a Česko zvoľte CET/CEST (automatický letný a zimný čas).",
  planesView:"🧭 Orientácia, Kompas & Jednotky",topBearing:"Smer hore na radare",metric:"Metrické jednotky (km, km/h, m namiesto NM, kt, ft)",
  tb0:"Sever (Sever hore / North-Up)",tb45:"Severovýchod (45°)",tb90:"Východ (90°)",tb135:"Juhovýchod (135°)",tb180:"Juh (180°)",tb225:"Juhozápad (225°)",tb270:"Západ (270°)",tb315:"Severozápad (315°)",
  planesViewHint:"Nastavte smer podľa toho, kam smeruje váš výhľad. Meteoradar sa zámerne orientuje na sever.",
  compassHint:"Elektronický kompas (QMI8658) umožňuje buď zobraziť miniatúru kompasu v rohu mapy, alebo dynamicky otáčať celú radarovú mapu podľa natočenia zariadenia. Klepnutím na kompas na displeji nakalibrujete sever.",
  autoRotateBearing:"Auto-rotácia mapy podľa kompasu (Live Heading)",
  autoRotate:"Automatické striedanie (sekundy, 0 = vypnuté)",
  rotHint:"Striedanie pozastaví potiahnutie prstom alebo prepnutie z prehliadača. Otvorený detail lietadla striedanie pozastaví.",
  radar:"🌧️ Meteoradar & Zobrazenie",radarSrc:"Zdroj radarových dát",srcRv:"RainViewer (Európa a svet)",srcChmu:"ČHMÚ (veľmi ostré dáta, len ČR)",srcShmu:"SHMÚ (veľmi ostré dáta, Slovensko)",
  showAltBar:"Výšková lišta letových hladín",showPrecipBar:"Farebná škála zrážok (alebo poklepanie)",
  smoothRadar:"Vyhladenie radarových dát (bilineárna interpolácia pre SHMÚ a ČHMÚ)",
  radarHint:"V SR použite SHMÚ, v ČR ČHMÚ, inde v Európe a vo svete RainViewer.",
  planes:"✈️ ADS-B Filtre & Sledovanie",altMin:"Minimálna letová výška (ft)",altMax:"Maximálna letová výška (ft)",onlyCs:"Iba lietadlá so známym volacím znakom (callsign)",
  sqAlert:"Zvýrazniť a upozorniť na núdzové squawky (7500 / 7600 / 7700)",watch:"Sledovaný let (Callsign alebo ICAO hex)",
  planesHint:"Filtre ovplyvňujú len vykresľovanie. Núdzový squawk ani sledované lietadlo filter nikdy neskryje.",
  typeFiltersTitle:"🎯 Filter typov strojov na radare",typeFiltersHint:"Vyberte kategórie strojov, ktoré sa majú zobrazovať na radarovej mape. Núdzové a sledované lety sa zobrazia vždy.",
  tfAirliner:"✈️ Dopravné lietadlá (A320, B737...)",tfLight:"🛩️ Malé a športové lietadlá (Cessna, Piper...)",
  tfHeli:"🚁 Vrtuľníky a záchranári (ATE, HEMS...)",tfMil:"⚔️ Vojenské letectvo a stíhačky",
  tfHeavy:"🛫 Ťažké nákladné obry (A380, B747, Beluga...)",tfGlider:"🪂 Vetrone a klzáky (Gliders)",
  buzzerHdr:"🔊 Zvukové výstrahy & Bzučiak",buzzerHint:"Nastavenie vstavaného bzučiaka na doske pre radarové výstrahy a odozvu.",
  bzMaster:"Povoliť bzučiak (hlavný vypínač)",bzEmergency:"🚨 Núdzový squawk (7700 / 7600 / 7500)",bzWatch:"⭐ Sledovaný let (vstup do dosahu)",
  bzTouch:"👆 Akustická odozva na dotyk displeja",bzHourly:"🕒 Pípnutie na celú hodinu (chime)",bzNightMute:"🌙 Nočný kľud (stíšiť bzučiak v noci)",btnTestBuzzer:"🔊 Otestovať bzučiak",
  brightness:"☀️ Jas displeja & Nočný režim",briDay:"Denný jas",briNight:"Nočný jas",nightAuto:"Prepínať nočný režim automaticky podľa západu/východu slnka",
  nightOffset:"Posun voči východu/západu (minúty)",clockHdr:"🕒 Ciferník hodín",secStyle:"Štýl sekundového prstenca",
  secOff:"Vypnuté",secDots:"Bodky (Dots)",secLine:"Plná čiara (Line)",secComet:"Kométa (Comet)",
  secRadar:"Radarový lúč (Sweep)",secTicks:"Hodinárske indexy (Ticks)",secOrbit:"Satelit na orbite (Orbit)",
  clockColor:"Farba číslic hodín",secColor:"Farba sekundového prstenca",
  clockStyle:"Štýl ciferníka",clkDigital:"Digitálny klasický",clkAnalog:"Letecký kokpitový analóg (Aviator)",clkOrbital:"Planetárne prstence (Orbital Gauges)",clkHud:"Stíhací priehľadový displej (Fighter HUD)",clkRegulator:"Astronomický regulátor (Régulateur)",clkStacked:"Vertikálna typografia (Stacked Bold)",clkMinimal:"Minimalistický moderný (Nordic)",
  clockWidgets:"Prvky na obrazovke hodín",cDate:"Dátum",cWx:"Počasie & teplota",cWind:"Rychlosť vetra",cMoon:"Fáza mesiaca",cAstro:"24h solárny prstenec",nightClockOnly:"V noci iba Hodiny (zastavit radary)",
  radarWidgets:"Prvky radarových máp",rTrails:"Trajektórie lietadiel (Trails)",rNearest:"Vektor k najbližšiemu lietadlu",rAirports:"Letiská (Runway ikony)",rRings:"Kilometrové kružnice dosahu",rCompass:"Elektronický kompas (miniatúra na mape)",
  tacticalHdr:"🎯 Taktické zobrazenie",tacticalDesc:"Kombinovaný taktický radar spája ADS-B lety a búrkové radarové odrazy do jednej spoločnej obrazovky v reálnom čase.",tacticalHint:"Filtre výšky a volacích znakov sa preberajú z nastavení Lietadiel, zdroj zrážok z Meteoradaru.",
  forecastHdr:"⛅ Predpoveď počasia",forecastDesc:"Predpoveď počasia sa automaticky sťahuje zo služby Open-Meteo pre vašu domovskú polohu.",btnGoLocSettings:"📍 Nastaviť domovskú polohu v Spoločných nastaveniach",
  hwRtc:"⏱️ Systémový čas & RTC (PCF85063)",hwLocalTimeLbl:"Miestny čas:",hwTzLbl:"Pásmo / Posun:",hwRtcLbl:"Stav RTC čipu:",hwRtcTimeLbl:"Čas v RTC čipe:",btnSyncNtp:"🌐 NTP sync",btnSyncBrowser:"💻 Z prehliadača",hwI2c:"🔍 I2C Zbernica (Bus Inspector)",
  liveHint:"Zmeny jasu a prvkov sa ukladajú okamžite v reálnom čase.",
  hwCpu:"⚡ ESP32-S3 & Teplota",hwChipModel:"Model čipu:",hwRev:"Rev",hwCores:"jadrá",hwCpuFreqLbl:"Frekvencia CPU:",hwCpuTempLbl:"Teplota CPU:",hwResetLbl:"Dôvod reštartu:",hwUptimeLbl:"Doba behu (Uptime):",
  hwMem:"💾 Pamäť & Úložisko",hwHeapLbl:"Interná RAM (Heap):",hwPsramLbl:"Octal PSRAM (8 MB):",hwFlashLbl:"Flash pamäť:",
  hwSensors:"🧭 6-Axis IMU Senzor (QMI8658)",hwImuLbl:"Stav senzora:",hwImuActive:"Aktívny (I2C 0x6B)",hwAxLbl:"Akcelerometer (g):",hwGxLbl:"Gyroskop (°/s):",hwTiltLbl:"Náklon (Pitch / Roll):",hwDblTapLbl:"Gesto poklepania:",hwDblTapOn:"Double-Tap zapnuté",
  hwPeripherals:"🔌 Periférie & Displej",hwDispLbl:"Displej:",hwTouchLbl:"Dotykový panel:",hwExpLbl:"I/O Expandér:",
  wifi:"📶 WiFi Pripojenie",network:"Názov siete (SSID)",password:"Heslo siete",scan:"Vyhľadať",connect:"Pripojiť k sieti",
  savedWifiHdr:"💾 Uložené WiFi siete (max 5)",savedWifiHint:"Zariadenie si pamätá až 5 sietí (napr. doma a v práci) a pri štarte sa automaticky pripojí k najsilnejšej dostupnej sieti.",
  delWifiConfirm:"Naozaj chcete zabudnúť WiFi sieť",activeNet:"(aktívna)",btnForgetNet:"Zmazať",noSavedWifi:"Žiadne uložené siete",
  netHostLbl:"Názov v sieti (Hostname):",netIpLbl:"IP adresa:",netRssiLbl:"Sila signálu (RSSI):",netMacLbl:"MAC adresa:",
  wifiHint:"Po uložení sa zariadenie pripojí a prístupový bod zmizne.",
  wifiHintSta:"Zmena siete preruší spojenie. Pri neúspěchu zariadenie vytvorí vlastnú sieť MeteoPlaneRadar.",
  wifiNow:"Pripojené k sieti",system:"🔒 Zabezpečenie správcu",adminPass:"Súčasné heslo",newPass:"Nové heslo",
  passHint:"Súčasné heslo je potrebné pre aktualizáciu, import, reset aj pre zmenu hesla. Prázdne nové heslo nič nemení; jedna medzera ochranu zruší.",
  statusHdr:"🛠️ Údržba & Záloha",fwUpdate:"Aktualizácia firmvéru",export:"Export nastavení",import:"Import nastavení",reboot:"Reštartovať",factory:"Továrenský reset",save:"💾 Uložiť nastavenia",
  pwNone:"Zatiaľ nie je nastavené žiadne heslo — aktualizácia, import a reset sú otvorené.",pwSet:"Heslo je nastavené.",
  wrongPass:"Chybné heslo",doneReboot:"Hotovo. Reštartujem...",importOk:"Import úspešný. Reštartujem...",
  autoSaved:"Uložené",saved:"Uložené",failed:"Nepodarilo sa",searching:"Hľadám…",nothing:"Nič sa nenašlo",
  disabled:"Obrazovka je vypnutá",confirmReset:"Naozaj vymazať všetky nastavenia vrátane WiFi?",
  otaHdr:"🚀 Aktualizácia firmvéru (GitHub OTA)",otaCurLbl:"Nainštalovaná verzia:",otaGitLbl:"Najnovšia verzia na GitHube:",
  btnOtaCheck:"🔍 Skontrolovať aktualizácie",btnOtaInstall:"⚡ Aktualizovať z GitHubu",otaProgLbl:"Priebeh inštalácie:",
  otaWarn:"⚠️ Zariadenie nevypínajte. Po dokončení zápisu sa zariadenie automaticky reštartuje.",
  otaUpToDate:"Máte najnovšiu verziu",otaNewAvail:"K dispozícii je nová verzia!",otaChecking:"Kontrolujem GitHub...",
  otaDownloading:"Sťahovanie a zápis firmvéru...",otaSuccess:"Aktualizácia úspešná! Reštartujem...",otaErr:"Chyba aktualizácie",
  otaNoAsset:"Vydanie neobsahuje súbor OTA (-ota.bin)",otaConfirm:"Naozaj spustiť aktualizáciu firmvéru na verziu",
  scrClock:"Hodiny & Astro",scrPlanes:"Lietadlá radar",scrMeteo:"Meteoradar",scrTactical:"Taktický radar",scrForecast:"Predpoveď počasia",scrInfo:"Info & Štatistiky",scrFinance:"Trhy & Krypto",scrSettings:"Nastavenia",
  finSlotsHdr:"📈 Sledované trhy & aktíva (4 pozície)",finSlot1:"1. Hlavný trh (Hero graf)",finSlot1Sub:"Veľký reálny graf + kurz",finSlot2:"2. Sledovaný trh",finSlot3:"3. Sledovaný trh",finSlot4:"4. Sledovaný trh",finSlotWatchlist:"Karta v spodnom zozname",finPresets:"⚡ Rýchle pridanie populárnych aktív:",finClear:"Vymazať",financeHint:"1. pozícia má veľký graf (Hero), pozície 2–4 sa zobrazujú v dolnom prehľade. Podporuje európske ETF fondy (Amundi CW8.PA, 500.PA, Vanguard VWCE.DE), krypto, indexy, komodity aj svetové akcie z Yahoo Finance.",
  ultraNight:"🌙 Ultra Night režim (hlboká červená / spánkový monochróm, min. jas)",
  tabScrInfo:"ℹ️ Info & Štatistiky",scrInfoHdr:"Info & Denná štatistika letov",scrInfoActive:"Zahrnúť obrazovku do automatického striedania",
  statsTrafficHdr:"✈️ Dnešná letecká štatistika",btnResetStats:"🔄 Resetovať",stUnique:"Unikátne lietadlá dnes:",stTopSpeed:"Najvyššia rýchlosť:",stMaxDist:"Maximálna vzdialenosť:",stAltSpan:"Rozpätie výšok:",stReports:"Prijaté ADS-B správy:",statsHint:"Štatistika sa automaticky nuluje o polnoci a uchováva sa v pamäti PSRAM.",confirmResetStats:"Naozaj resetovať dnešnú štatistiku letov?",statsResetOk:"Štatistiky boli resetované",
  cOver:"✈️ Prelet nad hlavou (Overhead widget)",ovRad:"Polomer preletu nad hlavou (km)",bzOverhead:"🔊 Prelet nad hlavou (Overhead výstraha)",
   cPrecip:"🌧️ Výstraha blížiacich sa zrážok",bzPrecip:"🔊 Pípnutie pri blížiacich sa zrážkach",precipTrackerHdr:"🌧️ Detekcia blížiacich sa zrážok (Nowcasting)",precipHint:"Vektorová analýza pohybu frontu (TREC). Upozorní na dážď, krúpy alebo sneh, iba ak zrážky smerujú priamo k vašej polohe.",precipLiveHdr:"Aktuálny stav nowcastingu:",
  scrShotHdr:"📸 Snímka displeja",scrShotNone:"Kliknite pre zachytenie",btnTakeScrShot:"📸 Zachytiť obrazovku",btnDlScrShot:"💾 Stiahnuť BMP",scrShotOk:"Snímka úspešne načítaná",scrShotErr:"Chyba načítania snímky",
  tabScrIss:"🛰️ ISS Tracker",scrIssHdr:"ISS Tracker (Medzinárodná vesmírna stanica)",scrIssActive:"Zahrnúť obrazovku do automatického striedania",scrIss:"ISS Tracker",issAlertLbl:"🔊 Zvuková výstraha pri prelete stanice nad obzorom (v dosahu / nad hlavou)",issTelemetryHdr:"🛰️ Živá telemetria stanice ISS",issStateLbl:"Aktuálny stav:",issLatLonLbl:"Poloha (Lat / Lon):",issAltLbl:"Výška letu:",issVelLbl:"Rýchlosť:",issDistLbl:"Vzdialenosť:",issElLbl:"Elevácia / Azimut:",issSunLbl:"Osvetlenie:",issSunlit:"☀️ Na dennom svetle",issEclipsed:"🌑 V tieni Zeme",issInRange:"🛰️ V dosahu priamej viditeľnosti",issOverhead:"⭐ PRIAMO NAD HLAVOU (> 45°)",issOutRange:"Mimo dosahu horizontu",btnIssRefresh:"🔄 Aktualizovať telemetriu",issHint:"Sledovanie preletu Medzinárodnej vesmírnej stanice ISS v reálnom čase (API WhereTheISS). Zobrazuje mapu sveta s dňom/nocou, orbitálnu dráhu, horizont priamej viditeľnosti (~2200 km) a časovač ďalšieho preletu.",
   tabSerial:"📟 Sériový monitor",serialHdr:"Sériový monitor (Live Web Console)",btnSerialStart:"Spustiť monitor",btnSerialPause:"Pozastaviť monitor",serialActive:"Aktívny (Live)",serialPaused:"Pozastavené",serialAutoScroll:"Automatický posun",btnSerialCopy:"📋 Kopírovať",btnSerialDl:"💾 Stiahnuť",btnSerialClear:"🧹 Vymazať",btnSerialSend:"Odoslať ↵",serialCopied:"Výpis skopírovaný do schránky",serialCleared:"Konzola vymazaná",serialHint:"Streamovanie výstupov sériového portu cez WiFi bez nutnosti USB kábla. Pri odchode zo záložky sa prenos automaticky pozastaví."
 },
 en:{
  tabScrClock:"🕒 Clock",tabScrPlanes:"✈️ Aircraft",tabScrMeteo:"🌧️ Weather Radar",tabScrTactical:"🎯 Tactical Radar",tabScrForecast:"⛅ Forecast",tabScrInfo:"ℹ️ Info & Stats",tabScrFinance:"📈 Markets & Crypto",tabCommon:"⚙️ Shared Settings",
  scrClockHdr:"Clock & Astro",scrPlanesHdr:"Aircraft radar (ADS-B)",scrMeteoHdr:"Precipitation radar",scrTacticalHdr:"Tactical radar (Aircraft + Rain)",scrForecastHdr:"Weather forecast",scrFinanceHdr:"Financial Markets, Stocks & Crypto",
  btnShowOnDisp:"Show on display",liveDispLbl:"On display:",
  scrClockActive:"Include screen in automatic cycling",scrPlanesActive:"Include screen in automatic cycling",scrMeteoActive:"Include screen in automatic cycling",scrTacticalActive:"Include screen in automatic cycling",scrForecastActive:"Include screen in automatic cycling",scrFinanceActive:"Include screen in automatic cycling",
  rotateHdr:"🔄 Auto Screen Cycling",
  remote:"🎮 Remote Control",rangeLbl:"Radar Scale:",
  btnPrev:"← Previous",btnDblTap:"🔄 Legend",btnNext:"Next →",btnDec:"Zoom In (− km)",btnInc:"Zoom Out (+ km)",
  remoteHint:"Range applies to Aircraft, Weather and Tactical screens. Manual action pauses auto cycling.",
  location:"📍 Home Location",findCity:"Search town",search:"Search",found:"Found results",lat:"Latitude (°N)",lon:"Longitude (°E)",
  locHint:"Changing location requires a reboot to recalculate maps and forecast.",
  tzHdr:"🕒 Timezone & GMT Offset",tzSelectLbl:"Timezone / GMT Offset",btnDetectTz:"🌐 Detect from browser",
  tzHint:"Sets the UTC time offset for clocks, radar frames, and weather forecasts. For Central Europe choose CET/CEST (automatic daylight saving time).",
  planesView:"🧭 Orientation, Compass & Units",topBearing:"Radar top orientation",metric:"Metric units (km, km/h, m instead of NM, kt, ft)",
  tb0:"North (North-Up)",tb45:"Northeast (45°)",tb90:"East (90°)",tb135:"Southeast (135°)",tb180:"South (180°)",tb225:"Southwest (225°)",tb270:"West (270°)",tb315:"Northwest (315°)",
  planesViewHint:"Set the bearing you are looking out of your window. Weather radar is North-Up.",
  compassHint:"Electronic compass (QMI8658) enables either a corner compass widget or dynamic real-time map auto-rotation based on physical orientation. Tap the compass on display to recalibrate North.",
  autoRotateBearing:"Auto-rotate map by compass (Live Heading)",
  autoRotate:"Auto cycle (seconds, 0 = off)",
  rotHint:"Cycling is paused by swiping or browser actions. An open aircraft detail keeps cycling paused.",
  radar:"🌧️ Weather Radar & Feeds",radarSrc:"Radar Data Source",srcRv:"RainViewer (Europe & Global)",srcChmu:"CHMU (high-res, Czechia only)",srcShmu:"SHMU (high-res, Slovakia)",
  showAltBar:"Flight levels altitude bar",showPrecipBar:"Rain dBZ color scale (or double-tap)",
  smoothRadar:"Radar smoothing (bilinear interpolation for SHMU and CHMU)",
  radarHint:"Use CHMU for Czechia, SHMU for Slovakia, or RainViewer globally.",
  planes:"✈️ ADS-B Filters & Watchlist",altMin:"Min altitude (ft)",altMax:"Max altitude (ft)",onlyCs:"Only aircraft with callsign",
  sqAlert:"Highlight emergency squawks (7500/7600/7700)",watch:"Watched flight (Callsign or ICAO hex)",
  planesHint:"Filters only affect drawing. Emergencies and watched flights are never hidden.",
  typeFiltersTitle:"🎯 Aircraft Type Filter",typeFiltersHint:"Choose which aircraft categories to display on the radar map. Emergency squawks and watched flights are always shown.",
  tfAirliner:"✈️ Commercial Airliners (A320, B737...)",tfLight:"🛩️ Light & Sport Aircraft (Cessna, Piper...)",
  tfHeli:"🚁 Helicopters & Rescue (HEMS, SAR...)",tfMil:"⚔️ Military & Fighter Jets",
  tfHeavy:"🛫 Heavy Giants (A380, B747, Beluga...)",tfGlider:"🪂 Gliders & Sailplanes",
  buzzerHdr:"🔊 Acoustic Alerts & Buzzer",buzzerHint:"Configure onboard active buzzer for radar alerts and touch feedback.",
  bzMaster:"Enable buzzer (Master Switch)",bzEmergency:"🚨 Emergency squawk (7700 / 7600 / 7500)",bzWatch:"⭐ Watched flight (arrival in range)",
  bzTouch:"👆 Touch feedback click",bzHourly:"🕒 Hourly chime",bzNightMute:"🌙 Night quiet mode (mute buzzer at night)",btnTestBuzzer:"🔊 Test buzzer",
  brightness:"☀️ Display Brightness & Night Mode",briDay:"Day brightness",briNight:"Night brightness",nightAuto:"Automatic night mode with sun position",
  nightOffset:"Offset from sunset/sunrise (minutes)",clockHdr:"🕒 Clock Face",secStyle:"Seconds ring style",
  secOff:"Off",secDots:"Dots",secLine:"Line",secComet:"Comet",
  secRadar:"Radar sweep",secTicks:"Swiss ticks",secOrbit:"Orbiting satellite",
  clockColor:"Clock digits colour",secColor:"Seconds ring colour",
  clockStyle:"Clock face style",clkDigital:"Classic Digital",clkAnalog:"Aviator Cockpit Analog",clkOrbital:"Orbital Gauges",clkHud:"Fighter HUD",clkRegulator:"Observatory Régulateur",clkStacked:"Stacked Bold Typography",clkMinimal:"Nordic Minimal",
  clockWidgets:"Clock screen widgets",cDate:"Date",cWx:"Weather & temp",cWind:"Wind speed",cMoon:"Moon phase",cAstro:"24h solar arc",nightClockOnly:"Night: Clock only (pause radars)",
  radarWidgets:"Radar map widgets",rTrails:"Flight trails (breadcrumbs)",rNearest:"Vector to nearest aircraft",rAirports:"Airports (runway icons)",rRings:"Range rings",rCompass:"Electronic compass (map widget)",
  tacticalHdr:"🎯 Tactical Display",tacticalDesc:"Tactical radar brings ADS-B aircraft traffic and real-time weather radar together onto one unified display.",tacticalHint:"Aircraft altitude/callsign filters are inherited from Aircraft radar, weather source from Weather radar.",
  forecastHdr:"⛅ Weather Forecast",forecastDesc:"Weather forecast is automatically fetched via Open-Meteo for your configured home location.",btnGoLocSettings:"📍 Set home location in Shared Settings",
  hwRtc:"⏱️ System Clock & RTC (PCF85063)",hwLocalTimeLbl:"Local time:",hwTzLbl:"Timezone / Offset:",hwRtcLbl:"RTC chip status:",hwRtcTimeLbl:"RTC hardware time:",btnSyncNtp:"🌐 NTP sync",btnSyncBrowser:"💻 Browser sync",hwI2c:"🔍 I2C Bus Inspector",
  liveHint:"Brightness and widget changes are saved instantly.",
  hwCpu:"⚡ ESP32-S3 & Temp",hwChipModel:"Chip model:",hwRev:"Rev",hwCores:"cores",hwCpuFreqLbl:"CPU frequency:",hwCpuTempLbl:"CPU temperature:",hwResetLbl:"Reset reason:",hwUptimeLbl:"Uptime:",
  hwMem:"💾 Memory & Storage",hwHeapLbl:"Internal RAM (Heap):",hwPsramLbl:"Octal PSRAM (8 MB):",hwFlashLbl:"Flash memory:",
  hwSensors:"🧭 6-Axis IMU Sensor (QMI8658)",hwImuLbl:"Sensor state:",hwImuActive:"Active (I2C 0x6B)",hwAxLbl:"Accelerometer (g):",hwGxLbl:"Gyroscope (°/s):",hwTiltLbl:"Tilt (Pitch / Roll):",hwDblTapLbl:"Double-Tap gesture:",hwDblTapOn:"Double-Tap active",
  hwPeripherals:"🔌 Peripherals & Display",hwDispLbl:"Display:",hwTouchLbl:"Touch panel:",hwExpLbl:"I/O Expander:",
  wifi:"📶 WiFi Connection",network:"Network Name (SSID)",password:"Network Password",scan:"Scan",connect:"Connect",
  savedWifiHdr:"💾 Saved WiFi Networks (max 5)",savedWifiHint:"The device remembers up to 5 networks (e.g. home and work) and automatically connects to the strongest available on boot.",
  delWifiConfirm:"Do you really want to forget WiFi network",activeNet:"(active)",btnForgetNet:"Forget",noSavedWifi:"No saved networks",
  netHostLbl:"Network name (Hostname):",netIpLbl:"IP address:",netRssiLbl:"Signal strength (RSSI):",netMacLbl:"MAC address:",
  wifiHint:"After saving the device connects and the AP closes.",
  wifiHintSta:"Changing WiFi disconnects this page. If connection fails, MeteoPlaneRadar AP will be restored.",
  wifiNow:"Connected to",system:"🔒 Admin Security",adminPass:"Current password",newPass:"New password",
  passHint:"Current password is required for updates, import, reset and password changes.",
  statusHdr:"🛠️ Maintenance & Backup",fwUpdate:"Firmware update",export:"Export settings",import:"Import settings",reboot:"Reboot",factory:"Factory reset",save:"💾 Save Settings",
  pwNone:"No password set — update, import and reset are open.",pwSet:"Password is set.",
  wrongPass:"Wrong password",doneReboot:"Done. Rebooting...",importOk:"Import successful. Rebooting...",
  autoSaved:"Saved",saved:"Saved",failed:"Failed",searching:"Searching…",nothing:"Nothing found",
  disabled:"Screen is disabled",confirmReset:"Really erase all settings and reset WiFi?",
  otaHdr:"🚀 Firmware Update (GitHub OTA)",otaCurLbl:"Installed version:",otaGitLbl:"Latest version on GitHub:",
  btnOtaCheck:"🔍 Check for updates",btnOtaInstall:"⚡ Update from GitHub",otaProgLbl:"Installation progress:",
  otaWarn:"⚠️ Do not power off device. It will automatically restart once finished.",
  otaUpToDate:"Up to date",otaNewAvail:"New version available!",otaChecking:"Checking GitHub...",
  otaDownloading:"Downloading & flashing firmware...",otaSuccess:"Update successful! Restarting...",otaErr:"Update failed",
  otaNoAsset:"Release missing OTA binary (-ota.bin)",otaConfirm:"Really install firmware update to version",
  scrClock:"Clock & Astro",scrPlanes:"Aircraft radar",scrMeteo:"Weather radar",scrTactical:"Tactical radar",scrForecast:"Weather forecast",scrInfo:"Info & Stats",scrFinance:"Markets & Crypto",scrSettings:"Settings",
  finSlotsHdr:"📈 Watched Markets & Assets (4 slots)",finSlot1:"1. Primary Market (Hero Chart)",finSlot1Sub:"Full sparkline chart + quote",finSlot2:"2. Watchlist Market",finSlot3:"3. Watchlist Market",finSlot4:"4. Watchlist Market",finSlotWatchlist:"Bottom watchlist card",finPresets:"⚡ Quick-add popular assets:",finClear:"Clear",financeHint:"Slot 1 features the large Hero sparkline chart; slots 2–4 appear in the bottom watchlist cards. Supports European ETFs (Amundi CW8.PA, 500.PA, Vanguard VWCE.DE), crypto, indices, commodities and global equities via Yahoo Finance.",
  ultraNight:"🌙 Ultra Night mode (deep red sleep monochrome, min. brightness)",
  tabScrInfo:"ℹ️ Info & Stats",scrInfoHdr:"Info & Daily Flight Statistics",scrInfoActive:"Include screen in automatic cycling",
  statsTrafficHdr:"✈️ Flight Traffic Today",btnResetStats:"🔄 Reset",stUnique:"Unique aircraft today:",stTopSpeed:"Top speed:",stMaxDist:"Max distance:",stAltSpan:"Altitude span:",stReports:"ADS-B reports received:",statsHint:"Statistics auto-reset at midnight and are kept in PSRAM.",confirmResetStats:"Really reset today's flight statistics?",statsResetOk:"Statistics reset successfully",
  cOver:"✈️ Overhead aircraft widget",ovRad:"Overhead radius (km)",bzOverhead:"🔊 Overhead aircraft alert",
   cPrecip:"🌧️ Approaching precipitation alert",bzPrecip:"🔊 Approaching precipitation alert chime",precipTrackerHdr:"🌧️ Approaching Precipitation Detection (Nowcasting)",precipHint:"Vector motion analysis (TREC). Alerts on incoming rain, hail, or snow only when heading towards your location.",precipLiveHdr:"Current nowcasting status:",
  scrShotHdr:"📸 Screen Capture",scrShotNone:"Click to capture",btnTakeScrShot:"📸 Capture Screen",btnDlScrShot:"💾 Download BMP",scrShotOk:"Screenshot captured successfully",scrShotErr:"Failed to capture screenshot",
  tabScrIss:"🛰️ ISS Tracker",scrIssHdr:"ISS Tracker (International Space Station)",scrIssActive:"Include screen in automatic cycling",scrIss:"ISS Tracker",issAlertLbl:"🔊 Acoustic alert when ISS is in range / overhead",issTelemetryHdr:"🛰️ Live ISS Telemetry",issStateLbl:"Current status:",issLatLonLbl:"Coordinates (Lat / Lon):",issAltLbl:"Altitude:",issVelLbl:"Velocity:",issDistLbl:"Slant range:",issElLbl:"Elevation / Azimuth:",issSunLbl:"Illumination:",issSunlit:"☀️ Daylight (Sunlit)",issEclipsed:"🌑 Earth Shadow (Eclipsed)",issInRange:"🛰️ In line of sight",issOverhead:"⭐ OVERHEAD PASS (> 45°)",issOutRange:"Out of line of sight",btnIssRefresh:"🔄 Refresh Telemetry",issHint:"Real-time orbital tracking of the International Space Station (WhereTheISS API). Displays world day/night terminator, ground track orbit, line-of-sight visibility footprint (~2,200 km) and countdown to the next overhead pass.",
   tabSerial:"📟 Serial Monitor",serialHdr:"Serial Monitor (Live Web Console)",btnSerialStart:"Start Monitor",btnSerialPause:"Pause Monitor",serialActive:"Active (Live)",serialPaused:"Paused",serialAutoScroll:"Auto-scroll",btnSerialCopy:"📋 Copy",btnSerialDl:"💾 Download",btnSerialClear:"🧹 Clear",btnSerialSend:"Send ↵",serialCopied:"Log copied to clipboard",serialCleared:"Console cleared",serialHint:"Real-time serial output streaming over WiFi without needing a USB cable. Polling automatically pauses when switching tabs."
 }
};

let L="sk", CFG={}, TAB="tScrClock";
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
 document.querySelectorAll("[data-i18n-title]").forEach(e=>{
   const k=e.dataset.i18nTitle;
   const t=D[L][k];
   if(t!==undefined) e.title=t;
 });
 pwState();status();updateHardware();
}

function showTab(id){
 TAB=id;
 document.querySelectorAll("section.tab").forEach(s=>s.classList.toggle("hide",s.id!=id));
 document.querySelectorAll("#tabs button").forEach(b=>b.classList.toggle("on",b.dataset.tab==id));
 if(id=="tCommon" && !g_otaLatest) checkGithubUpdates(false);
 if(id=="tScrInfo") fetchStats();
 if(id=="tScrIss") refreshIssLive();
 if(id=="tSerial") startSerialPolling(); else stopSerialPolling();
 window.scrollTo(0,0);
}
document.querySelectorAll("#tabs button").forEach(b=>b.onclick=()=>showTab(b.dataset.tab));

function msg(t,c){$("msg").textContent=t;$("msg").className=c||"";setTimeout(()=>{$("msg").textContent=""},4000);}

const SCR=[["scrClock",0],["scrPlanes",1],["scrMeteo",2],["scrTactical",3],["scrForecast",4],["scrFinance",5],["scrIss",6],["scrInfo",7],["scrSettings",8]];
function drawScrBtns(cur,enabled){
 if(!$("scrBtns")) return;
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
async function stepRange(d){await post("/api/range",{step:d});setTimeout(status,150);}

async function toggleLegendsRemote(){
 try{
  const r = await fetch("/api/toggle-legends",{method:"POST"});
  if(r.ok){
   const res = await r.json();
   if($("showLegends")) $("showLegends").checked = !!res.legends;
   if($("showLegendsMeteo")) $("showLegendsMeteo").checked = !!res.legends;
   if(res.clockStyle !== undefined && $("clockStyle")) $("clockStyle").value = res.clockStyle;
   msg(D[L].autoSaved||"OK","ok");
   await status();
  }
 }catch(e){}
}

// --- Realtime Hardware Diagnostics ---
async function updateHardware(){
 try{
  const r=await fetch("/api/hardware");if(!r.ok)return;
  const h=await r.json();
  if($("hwCpuModel")) $("hwCpuModel").textContent=h.cpuModel||"ESP32-S3";
  if($("hwCpuRev")) $("hwCpuRev").textContent=h.cpuRev||"1";
  if($("hwCpuCores")) $("hwCpuCores").textContent=h.cpuCores||"2";
  if($("hwCpuFreq")) $("hwCpuFreq").textContent=h.cpuFreq||"240";
  if($("hwCpuTemp")) $("hwCpuTemp").textContent=h.cpuTemp||"-";
  if($("hwReset")) $("hwReset").textContent=h.resetReason||"-";
  if($("hwUptime")) $("hwUptime").textContent=h.uptime||"-";

  // RAM Heap
  const hUsed = (h.heapTotal - h.heapFree);
  const hPct = Math.round((hUsed / h.heapTotal)*100);
  if($("hwHeapTxt")) $("hwHeapTxt").textContent=Math.round(hUsed/1024)+" KB / "+Math.round(h.heapTotal/1024)+" KB ("+hPct+"%)";
  if($("hwHeapBar")) $("hwHeapBar").style.width=hPct+"%";

  // PSRAM
  const pUsed = (h.psramTotal - h.psramFree);
  const pPct = Math.round((pUsed / h.psramTotal)*100);
  if($("hwPsramTxt")) $("hwPsramTxt").textContent=(pUsed/1048576).toFixed(2)+" MB / "+(h.psramTotal/1048576).toFixed(0)+" MB ("+pPct+"%)";
  if($("hwPsramBar")) $("hwPsramBar").style.width=pPct+"%";

  // Flash
  if($("hwFlashSize")) $("hwFlashSize").textContent=Math.round(h.flashSize/1048576);
  if($("hwFlashSpd")) $("hwFlashSpd").textContent=Math.round(h.flashSpeed/1000000);

  // Network
  if($("netIp")) $("netIp").textContent=h.ip||"-";
  if($("netRssi")) $("netRssi").textContent=h.rssi ? (h.rssi+" dBm") : "-";
  if($("netMac")) $("netMac").textContent=h.mac||"-";

  // IMU
  if(h.imuOk){
   if($("hwAx")) $("hwAx").textContent=h.ax;
   if($("hwAy")) $("hwAy").textContent=h.ay;
   if($("hwAz")) $("hwAz").textContent=h.az;
   if($("hwGx")) $("hwGx").textContent=h.gx;
   if($("hwGy")) $("hwGy").textContent=h.gy;
   if($("hwGz")) $("hwGz").textContent=h.gz;
   if($("hwPitch")) $("hwPitch").textContent=h.pitch;
   if($("hwRoll")) $("hwRoll").textContent=h.roll;
  }

  // RTC details
  if($("hwLocalTime")) $("hwLocalTime").textContent = h.localTime || "-";
  if($("hwTzOffset")) $("hwTzOffset").textContent = h.tzOffset || "-";
  if($("hwRtcState")){
    const ok = !!h.rtcDetected;
    $("hwRtcState").textContent = ok ? (h.rtcOscStopped ? "Výpadok napájania (OSF)" : "Aktívny (I2C 0x51)") : "Nenájdený";
    $("hwRtcState").className = "pill " + (ok ? (h.rtcOscStopped ? "pill-warn" : "pill-ok") : "pill-err");
    if($("hwRtcTime")) $("hwRtcTime").textContent = h.rtcTime || "-";
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
setInterval(updateHardware, 4000);

// --- Okamzite ukladanie pre Vzhlad a Ovladanie ---
let saveTimer = {};
function autoSave(key, val){
 clearTimeout(saveTimer[key]);
 saveTimer[key] = setTimeout(async () => {
  const o = {}; o[key] = val;
  try {
    await fetch("/api/config", {
      method: "POST",
      headers: {"Content-Type": "application/json"},
      body: JSON.stringify(o)
    });
    msg(D[L].autoSaved || "Uložené", "ok");
  } catch(e){}
 }, 350);
}

function applyTzNow(val){
  autoSave("timezone", val);
  setTimeout(updateHardware, 400);
}

function detectBrowserTz(){
  try{
    const offMin = -new Date().getTimezoneOffset();
    const offH = offMin / 60;
    const tzName = Intl.DateTimeFormat().resolvedOptions().timeZone || "";
    const tzSel = $("timezone");
    if(!tzSel) return;
    if(tzName.includes("Bratislava")||tzName.includes("Prague")||tzName.includes("Vienna")||tzName.includes("Berlin")||tzName.includes("Warsaw")||tzName.includes("Budapest")||tzName.includes("Paris")||tzName.includes("Rome")||tzName.includes("Madrid")||tzName.includes("Amsterdam")||tzName.includes("Brussels")){
      tzSel.value = "CET-1CEST,M3.5.0,M10.5.0/3";
    } else if(tzName.includes("Athens")||tzName.includes("Kyiv")||tzName.includes("Helsinki")||tzName.includes("Bucharest")||tzName.includes("Sofia")||tzName.includes("Tallinn")||tzName.includes("Riga")||tzName.includes("Vilnius")){
      tzSel.value = "EET-2EEST,M3.5.0/3,M10.5.0/4";
    } else if(tzName.includes("London")||tzName.includes("Dublin")||tzName.includes("Lisbon")){
      tzSel.value = "GMT0BST,M3.5.0/1,M10.5.0";
    } else if(offH === 0){
      tzSel.value = "UTC0";
    } else {
      const matchPrefix = offH > 0 ? ("<+" + String(Math.abs(Math.floor(offH))).padStart(2,"0")) : ("<-" + String(Math.abs(Math.floor(offH))).padStart(2,"0"));
      for(let opt of tzSel.options){
        if(opt.value.startsWith(matchPrefix)){
          tzSel.value = opt.value;
          break;
        }
      }
    }
    applyTzNow(tzSel.value);
    msg(D[L].saved||"Uložené","ok");
  }catch(e){ msg(D[L].failed||"Chyba","err"); }
}

const AUTO = [
 ["uiLang","change","lang",e=>+e.value],
 ["metric","change","metric",e=>e.checked],
 ["topBearing","change","topBearing",e=>+e.value],
 ["briDay","input","briDay",e=>+e.value],
 ["briNight","input","briNight",e=>+e.value],
 ["nightAuto","change","nightAuto",e=>e.checked],
 ["ultraNight","change","ultraNight",e=>e.checked],
 ["nightOffset","change","nightOffset",e=>+e.value],
 ["secStyle","change","secStyle",e=>+e.value],
 ["clockColor","change","clockColor",e=>hexToRgb565(e.value)],
 ["secColor","change","secColor",e=>hexToRgb565(e.value)],
 ["showLegends","change","showLegends",e=>{ if($("showLegendsMeteo")) $("showLegendsMeteo").checked=e.checked; return e.checked; }],
 ["showLegendsMeteo","change","showLegends",e=>{ if($("showLegends")) $("showLegends").checked=e.checked; return e.checked; }],
 ["smoothRadar","change","smoothRadar",e=>e.checked],
 ["clockStyle","change","clockStyle",e=>+e.value],
 ["cDate","change","cDate",e=>e.checked],
 ["cWx","change","cWx",e=>e.checked],
 ["cWind","change","cWind",e=>e.checked],
 ["cMoon","change","cMoon",e=>e.checked],
 ["cAstro","change","cAstro",e=>e.checked],
 ["nightClockOnly","change","nightClockOnly",e=>e.checked],
 ["rTrails","change","rTrails",e=>{ if($("rTrailsTac")) $("rTrailsTac").checked=e.checked; return e.checked; }],
 ["rNearest","change","rNearest",e=>{ if($("rNearestTac")) $("rNearestTac").checked=e.checked; return e.checked; }],
 ["rAirports","change","rAirports",e=>{ if($("rAirportsMeteo")) $("rAirportsMeteo").checked=e.checked; if($("rAirportsTac")) $("rAirportsTac").checked=e.checked; return e.checked; }],
 ["rRings","change","rRings",e=>{ if($("rRingsMeteo")) $("rRingsMeteo").checked=e.checked; if($("rRingsTac")) $("rRingsTac").checked=e.checked; return e.checked; }],
 ["rCompass","change","rCompass",e=>{ if($("rCompassTac")) $("rCompassTac").checked=e.checked; if($("rCompassCommon")) $("rCompassCommon").checked=e.checked; return e.checked; }],
 ["rCompassTac","change","rCompass",e=>{ if($("rCompass")) $("rCompass").checked=e.checked; if($("rCompassCommon")) $("rCompassCommon").checked=e.checked; return e.checked; }],
 ["rCompassCommon","change","rCompass",e=>{ if($("rCompass")) $("rCompass").checked=e.checked; if($("rCompassTac")) $("rCompassTac").checked=e.checked; return e.checked; }],
 ["autoRotateBearing","change","autoRotateBearing",e=>e.checked],
 ["timezone","change","timezone",e=>{ applyTzNow(e.value); return e.value; }],
 ["tfAirliner","change","typeAirliner",e=>e.checked],
 ["tfLight","change","typeLight",e=>e.checked],
 ["tfHeli","change","typeHeli",e=>e.checked],
 ["tfMil","change","typeMilitary",e=>e.checked],
 ["tfHeavy","change","typeHeavy",e=>e.checked],
 ["tfGlider","change","typeGlider",e=>e.checked],
 ["buzzerOn","change","buzzerOn",e=>e.checked],
 ["bzEmergency","change","buzzerEmergency",e=>{ if($("bzEmergencyPlanes")) $("bzEmergencyPlanes").checked=e.checked; return e.checked; }],
 ["bzEmergencyPlanes","change","buzzerEmergency",e=>{ if($("bzEmergency")) $("bzEmergency").checked=e.checked; return e.checked; }],
 ["bzWatch","change","buzzerWatch",e=>{ if($("bzWatchPlanes")) $("bzWatchPlanes").checked=e.checked; return e.checked; }],
 ["bzWatchPlanes","change","buzzerWatch",e=>{ if($("bzWatch")) $("bzWatch").checked=e.checked; return e.checked; }],
 ["bzTouch","change","buzzerTouch",e=>e.checked],
 ["bzHourly","change","buzzerHourly",e=>{ if($("bzHourlyClock")) $("bzHourlyClock").checked=e.checked; return e.checked; }],
 ["bzHourlyClock","change","buzzerHourly",e=>{ if($("bzHourly")) $("bzHourly").checked=e.checked; return e.checked; }],
 ["bzOverhead","change","buzzerOverhead",e=>{ if($("bzOverheadClock")) $("bzOverheadClock").checked=e.checked; return e.checked; }],
 ["bzOverheadClock","change","buzzerOverhead",e=>{ if($("bzOverhead")) $("bzOverhead").checked=e.checked; return e.checked; }],
 ["bzPrecip","change","buzzerPrecip",e=>{ if($("bzPrecipClock")) $("bzPrecipClock").checked=e.checked; if($("bzPrecipMeteo")) $("bzPrecipMeteo").checked=e.checked; return e.checked; }],
 ["bzPrecipClock","change","buzzerPrecip",e=>{ if($("bzPrecip")) $("bzPrecip").checked=e.checked; if($("bzPrecipMeteo")) $("bzPrecipMeteo").checked=e.checked; return e.checked; }],
 ["bzPrecipMeteo","change","buzzerPrecip",e=>{ if($("bzPrecip")) $("bzPrecip").checked=e.checked; if($("bzPrecipClock")) $("bzPrecipClock").checked=e.checked; return e.checked; }],
 ["cPrecip","change","cPrecip",e=>{ if($("cPrecipMeteo")) $("cPrecipMeteo").checked=e.checked; return e.checked; }],
 ["cPrecipMeteo","change","cPrecip",e=>{ if($("cPrecip")) $("cPrecip").checked=e.checked; return e.checked; }],
 ["cOver","change","cOver",e=>e.checked],
 ["ovRad","change","ovRad",e=>parseFloat(e.value)||10],
 ["sClock","change","screens",()=>getScreensObj()],
 ["sPlanes","change","screens",()=>getScreensObj()],
 ["sMeteo","change","screens",()=>getScreensObj()],
 ["sTactical","change","screens",()=>getScreensObj()],
 ["sForecast","change","screens",()=>getScreensObj()],
 ["sInfo","change","screens",()=>getScreensObj()],
 ["sFinance","change","screens",()=>getScreensObj()],
 ["financeTickers","change","financeTickers",e=>e.value.trim()],
 ["sIss","change","screens",()=>getScreensObj()],
 ["issAlert","change","issAlert",e=>e.checked],
 ["bzNightMute","change","buzzerNightMute",e=>e.checked],
 ["hostname","change","hostname",e=>{ if($("netHost")) $("netHost").textContent=e.value+".local"; return e.value.trim(); }],
];

const FIN_PRESETS = [
  { group: "Európske ETF fondy (Amundi, Vanguard, iShares)", items: [
    { v: "CW8.PA", l: "🇪🇺 Amundi MSCI World (CW8.PA)" },
    { v: "500.PA", l: "🇪🇺 Amundi S&P 500 (500.PA)" },
    { v: "C50.PA", l: "🇪🇺 Amundi Euro Stoxx 50 (C50.PA)" },
    { v: "C6E.PA", l: "🇪🇺 Amundi Stoxx Europe 600 (C6E.PA)" },
    { v: "VWCE.DE", l: "🌍 Vanguard FTSE All-World (VWCE.DE)" },
    { v: "SXR8.DE", l: "🇺🇸 iShares Core S&P 500 (SXR8.DE)" },
    { v: "EUNL.DE", l: "🌍 iShares Core MSCI World (EUNL.DE)" },
    { v: "EQQQ.DE", l: "💻 Invesco EQQQ Nasdaq 100 (EQQQ.DE)" }
  ]},
  { group: "Kryptomeny / Crypto", items: [
    { v: "BTC-USD", l: "₿ Bitcoin (BTC-USD)" },
    { v: "ETH-USD", l: "Ξ Ethereum (ETH-USD)" },
    { v: "SOL-USD", l: "◎ Solana (SOL-USD)" },
    { v: "XRP-USD", l: "✕ Ripple (XRP-USD)" },
    { v: "DOGE-USD", l: "Ð Dogecoin (DOGE-USD)" }
  ]},
  { group: "Akciové indexy / Indices", items: [
    { v: "^GSPC", l: "📈 S&P 500 (^GSPC)" },
    { v: "^IXIC", l: "📊 Nasdaq Composite (^IXIC)" },
    { v: "^DJI", l: "🏛️ Dow Jones (^DJI)" },
    { v: "^GDAXI", l: "🇩🇪 DAX 40 Germany (^GDAXI)" },
    { v: "^FTSE", l: "🇬🇧 FTSE 100 London (^FTSE)" }
  ]},
  { group: "Komodity / Commodities", items: [
    { v: "GC=F", l: "🥇 Zlato / Gold (GC=F)" },
    { v: "SI=F", l: "🥈 Striebro / Silver (SI=F)" },
    { v: "CL=F", l: "🛢️ Ropa WTI Crude (CL=F)" },
    { v: "BZ=F", l: "🛢️ Ropa Brent (BZ=F)" },
    { v: "NG=F", l: "🔥 Zemný plyn (NG=F)" },
    { v: "HG=F", l: "🧱 Meď / Copper (HG=F)" }
  ]},
  { group: "Svetové akcie / Equities", items: [
    { v: "NVDA", l: "💻 Nvidia (NVDA)" },
    { v: "AAPL", l: "🍏 Apple (AAPL)" },
    { v: "MSFT", l: "🪟 Microsoft (MSFT)" },
    { v: "TSLA", l: "🚗 Tesla (TSLA)" },
    { v: "AMZN", l: "📦 Amazon (AMZN)" },
    { v: "GOOGL", l: "🔍 Alphabet Google (GOOGL)" },
    { v: "META", l: "♾️ Meta Platforms (META)" }
  ]},
  { group: "Meny & Forex", items: [
    { v: "EURUSD=X", l: "💶 EUR / USD (EURUSD=X)" },
    { v: "CZK=X", l: "💵 USD / CZK (CZK=X)" },
    { v: "EURCZK=X", l: "💶 EUR / CZK (EURCZK=X)" }
  ]}
];

function initFinPresets() {
  for (let i = 1; i <= 4; i++) {
    const sel = $("finSel" + i);
    if (!sel || sel.options.length > 1) continue;
    sel.innerHTML = '<option value="">-- Vlastný / Presets --</option>';
    FIN_PRESETS.forEach(g => {
      const grp = document.createElement("optgroup");
      grp.label = g.group;
      g.items.forEach(it => {
        const opt = document.createElement("option");
        opt.value = it.v;
        opt.textContent = it.l;
        grp.appendChild(opt);
      });
      sel.appendChild(grp);
    });
  }
}

function onFinSelChange(slot) {
  const sel = $("finSel" + slot);
  const tk = $("finTk" + slot);
  if (!sel || !tk) return;
  if (sel.value) tk.value = sel.value;
  syncAndSaveFinance();
}

function onFinInputChange(slot) {
  const sel = $("finSel" + slot);
  const tk = $("finTk" + slot);
  if (!tk) return;
  tk.value = tk.value.toUpperCase().replace(/\s+/g, "");
  if (sel) {
    sel.value = tk.value;
    if (sel.selectedIndex < 0) sel.value = "";
  }
  syncAndSaveFinance();
}

function clearFinSlot(slot) {
  const sel = $("finSel" + slot);
  const tk = $("finTk" + slot);
  if (tk) tk.value = "";
  if (sel) sel.value = "";
  syncAndSaveFinance();
}

function addFinPreset(symbol) {
  let target = 0;
  for (let i = 1; i <= 4; i++) {
    const el = $("finTk" + i);
    if (el && !el.value.trim()) { target = i; break; }
  }
  if (!target) target = 4;
  const tk = $("finTk" + target);
  const sel = $("finSel" + target);
  if (tk) tk.value = symbol;
  if (sel) {
    sel.value = symbol;
    if (sel.selectedIndex < 0) sel.value = "";
  }
  syncAndSaveFinance();
}

function syncAndSaveFinance() {
  const list = [];
  for (let i = 1; i <= 4; i++) {
    const el = $("finTk" + i);
    if (el) {
      const v = el.value.trim().toUpperCase();
      if (v) list.push(v);
    }
  }
  const csv = list.join(",");
  if ($("financeTickers")) $("financeTickers").value = csv;
  autoSave("financeTickers", csv);
}

function loadFinanceTickers(csv) {
  initFinPresets();
  const parts = (csv || "").split(",").map(s => s.trim().toUpperCase());
  for (let i = 1; i <= 4; i++) {
    const val = parts[i - 1] || "";
    const tk = $("finTk" + i);
    const sel = $("finSel" + i);
    if (tk) tk.value = val;
    if (sel) {
      sel.value = val;
      if (sel.selectedIndex < 0) sel.value = "";
    }
  }
  if ($("financeTickers")) $("financeTickers").value = csv || "";
}

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
 if($("otaCurVer")) $("otaCurVer").textContent="v"+CFG.version;
 $("lat").value=CFG.lat;$("lon").value=CFG.lon;
 $("sClock").checked=CFG.screens.clock;$("sPlanes").checked=CFG.screens.planes;
 $("sMeteo").checked=CFG.screens.meteo;$("sTactical").checked=CFG.screens.tactical;$("sForecast").checked=CFG.screens.forecast;
 if($("sInfo")) $("sInfo").checked=CFG.screens.info!==false;
 if($("sFinance")) $("sFinance").checked=CFG.screens.finance!==false;
 if($("sIss")) $("sIss").checked=CFG.screens.iss!==false;
 if($("issAlert")) $("issAlert").checked=!!CFG.issAlert;
 if(CFG.financeTickers !== undefined) loadFinanceTickers(CFG.financeTickers);
 $("autoRotate").value=CFG.autoRotate;$("radarSrc").value=CFG.radarSrc;
 if($("showLegends")) $("showLegends").checked=!!CFG.showLegends;
 if($("showLegendsMeteo")) $("showLegendsMeteo").checked=!!CFG.showLegends;
 if($("smoothRadar")) $("smoothRadar").checked=CFG.smoothRadar!==false;
 $("briDay").value=CFG.briDay;$("briNight").value=CFG.briNight;
 $("nightAuto").checked=CFG.nightAuto;$("nightOffset").value=CFG.nightOffset;
 $("secStyle").value=CFG.secStyle;$("metric").checked=CFG.metric;$("topBearing").value=CFG.topBearing;
 if($("clockStyle")) $("clockStyle").value=CFG.clockStyle||0;
 if($("cDate")) $("cDate").checked=CFG.cDate!==false;
 if($("cWx")) $("cWx").checked=CFG.cWx!==false;
 if($("cWind")) $("cWind").checked=CFG.cWind!==false;
 if($("cMoon")) $("cMoon").checked=CFG.cMoon!==false;
 if($("cAstro")) $("cAstro").checked=CFG.cAstro!==false;
 if($("cOver")) $("cOver").checked=CFG.cOver!==false;
 if($("ovRad")) $("ovRad").value=CFG.ovRad||10;
 if($("nightClockOnly")) $("nightClockOnly").checked=!!CFG.nightClockOnly;
 if($("ultraNight")) $("ultraNight").checked=!!CFG.ultraNight;
 if($("rTrails")) $("rTrails").checked=CFG.rTrails!==false;
 if($("rTrailsTac")) $("rTrailsTac").checked=CFG.rTrails!==false;
 if($("rNearest")) $("rNearest").checked=CFG.rNearest!==false;
 if($("rNearestTac")) $("rNearestTac").checked=CFG.rNearest!==false;
 if($("rAirports")) $("rAirports").checked=CFG.rAirports!==false;
 if($("rAirportsMeteo")) $("rAirportsMeteo").checked=CFG.rAirports!==false;
 if($("rAirportsTac")) $("rAirportsTac").checked=CFG.rAirports!==false;
 if($("rRings")) $("rRings").checked=CFG.rRings!==false;
 if($("rRingsMeteo")) $("rRingsMeteo").checked=CFG.rRings!==false;
 if($("rRingsTac")) $("rRingsTac").checked=CFG.rRings!==false;
 if($("rCompass")) $("rCompass").checked=CFG.rCompass!==false;
 if($("rCompassTac")) $("rCompassTac").checked=CFG.rCompass!==false;
 if($("rCompassCommon")) $("rCompassCommon").checked=CFG.rCompass!==false;
 if($("autoRotateBearing")) $("autoRotateBearing").checked=!!CFG.autoRotateBearing;
 if($("timezone") && CFG.timezone) $("timezone").value=CFG.timezone;
 if($("tfAirliner")) $("tfAirliner").checked=CFG.typeAirliner!==false;
 if($("tfLight")) $("tfLight").checked=CFG.typeLight!==false;
 if($("tfHeli")) $("tfHeli").checked=CFG.typeHeli!==false;
 if($("tfMil")) $("tfMil").checked=CFG.typeMilitary!==false;
 if($("tfHeavy")) $("tfHeavy").checked=CFG.typeHeavy!==false;
 if($("tfGlider")) $("tfGlider").checked=CFG.typeGlider!==false;
 if($("buzzerOn")) $("buzzerOn").checked=CFG.buzzerOn!==false;
 if($("bzEmergency")) $("bzEmergency").checked=CFG.buzzerEmergency!==false;
 if($("bzEmergencyPlanes")) $("bzEmergencyPlanes").checked=CFG.buzzerEmergency!==false;
 if($("bzWatch")) $("bzWatch").checked=CFG.buzzerWatch!==false;
 if($("bzWatchPlanes")) $("bzWatchPlanes").checked=CFG.buzzerWatch!==false;
 if($("bzTouch")) $("bzTouch").checked=!!CFG.buzzerTouch;
 if($("bzHourly")) $("bzHourly").checked=!!CFG.buzzerHourly;
 if($("bzHourlyClock")) $("bzHourlyClock").checked=!!CFG.buzzerHourly;
 if($("bzOverhead")) $("bzOverhead").checked=!!CFG.buzzerOverhead;
 if($("bzOverheadClock")) $("bzOverheadClock").checked=!!CFG.buzzerOverhead;
  if($("bzPrecip")) $("bzPrecip").checked=!!CFG.buzzerPrecip;
  if($("bzPrecipClock")) $("bzPrecipClock").checked=!!CFG.buzzerPrecip;
  if($("bzPrecipMeteo")) $("bzPrecipMeteo").checked=!!CFG.buzzerPrecip;
  if($("cPrecip")) $("cPrecip").checked=CFG.cPrecip!==false;
  if($("cPrecipMeteo")) $("cPrecipMeteo").checked=CFG.cPrecip!==false;
 if($("bzNightMute")) $("bzNightMute").checked=CFG.buzzerNightMute!==false;
   $("clockColor").value=rgb565ToHex(CFG.clockColor);$("secColor").value=rgb565ToHex(CFG.secColor);
   $("altMin").value=CFG.altMin;$("altMax").value=CFG.altMax;
   pwState();
   $("onlyCallsign").checked=CFG.onlyCallsign;$("squawkAlert").checked=CFG.squawkAlert;$("watch").value=CFG.watch||"";
   $("wifiHintTxt").textContent=CFG.apMode?D[L].wifiHint:D[L].wifiHintSta;
   if($("hostname")) $("hostname").value=CFG.hostname||"MeteoPlaneRadar";
   if($("netHost")) $("netHost").textContent=(CFG.hostname||"MeteoPlaneRadar")+".local";
   renderSavedWifi(CFG.wifiNetworks);
   bri();wireAutoSave();status();updateHardware();
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
 try{
  const s=await(await fetch("/api/status")).json();
  drawScrBtns(s.screen,s.enabled);
  if(!CFG.apMode&&$("wifiNow")) $("wifiNow").textContent=D[L].wifiNow+" "+s.ssid+" ("+s.rssi+" dBm).";
  const hasR=(s.range&&s.range.length>0);
  if($("rangeNow")) $("rangeNow").textContent=hasR?s.range:"–";
  if($("rMinus")){ $("rMinus").disabled=!hasR; $("rMinus").style.opacity=hasR?"1":".4"; }
  if($("rPlus")){ $("rPlus").disabled=!hasR; $("rPlus").style.opacity=hasR?"1":".4"; }
  
  const scrKeys=["scrClock","scrPlanes","scrMeteo","scrTactical","scrForecast","scrFinance","scrIss","scrInfo","scrSettings"];
  const curKey = scrKeys[s.screen] || "scrClock";
  const curName = (D[L] && D[L][curKey]) ? D[L][curKey] : ("Screen " + s.screen);
  if($("liveDispName")) $("liveDispName").textContent = curName;
  if($("hwCurScreenName")) $("hwCurScreenName").textContent = curName;
    if(s.iss !== undefined){
    if($("issStatePill")){
      if(s.issOverhead){
        $("issStatePill").textContent = (D[L]&&D[L].issOverhead) ? D[L].issOverhead : "⭐ OVERHEAD PASS";
        $("issStatePill").className = "pill pill-warn";
      } else if(s.issInRange){
        $("issStatePill").textContent = (D[L]&&D[L].issInRange) ? D[L].issInRange : "🛰️ IN RANGE";
        $("issStatePill").className = "pill pill-ok";
      } else {
        $("issStatePill").textContent = (D[L]&&D[L].issOutRange) ? D[L].issOutRange : "Out of range";
        $("issStatePill").className = "pill";
      }
    }
    if($("issPos") && s.issLat !== undefined) $("issPos").textContent = (s.issLat >= 0 ? s.issLat.toFixed(2)+"°N" : Math.abs(s.issLat).toFixed(2)+"°S") + ", " + (s.issLon >= 0 ? s.issLon.toFixed(2)+"°E" : Math.abs(s.issLon).toFixed(2)+"°W");
    if($("issAltVal") && s.issAlt !== undefined) $("issAltVal").textContent = s.issAlt + " km (" + Math.round(s.issAlt * 0.621371) + " mi)";
    if($("issVelVal") && s.issVel !== undefined) $("issVelVal").textContent = s.issVel + " km/h (" + Math.round(s.issVel * 0.621371) + " mph / " + (s.issVel / 3600).toFixed(2) + " km/s)";
    if($("issDistVal") && s.issDist !== undefined) $("issDistVal").textContent = s.issDist + " km";
    if($("issElVal") && s.issEl !== undefined) $("issElVal").textContent = s.issEl + "° / " + s.issAz + "°";
    if($("issSunVal") && s.issSun !== undefined) $("issSunVal").textContent = s.issSun ? ((D[L]&&D[L].issSunlit) ? D[L].issSunlit : "☀️ Sunlit") : ((D[L]&&D[L].issEclipsed) ? D[L].issEclipsed : "🌑 Eclipsed");
  }
  if(s.precip && $("livePrecipStatus")){
    $("livePrecipStatus").textContent = s.precip;
    if(s.precipStatus === 2){
      $("livePrecipStatus").style.color = "var(--warn)";
      if($("livePrecipDetail")) $("livePrecipDetail").textContent = "ETA: ~" + s.precipEta + " min | " + s.precipDist + " km @ " + s.precipSpeed + " km/h (" + s.precipBearing + ")";
    } else if(s.precipStatus === 3){
      $("livePrecipStatus").style.color = "var(--err)";
      if($("livePrecipDetail")) $("livePrecipDetail").textContent = s.precipDist + " km | " + s.precipSpeed + " km/h";
    } else {
      $("livePrecipStatus").style.color = "var(--mut)";
      if($("livePrecipDetail")) $("livePrecipDetail").textContent = "";
    }
  }
 }catch(e){}
}
setInterval(status,4000);

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
 smoothRadar:$("smoothRadar")?$("smoothRadar").checked:true,
 clockStyle:$("clockStyle")?+$("clockStyle").value:0,
 cDate:$("cDate")?$("cDate").checked:true,
 cWx:$("cWx")?$("cWx").checked:true,
 cWind:$("cWind")?$("cWind").checked:true,
 cMoon:$("cMoon")?$("cMoon").checked:true,
 cAstro:$("cAstro")?$("cAstro").checked:true,
 nightClockOnly:$("nightClockOnly")?$("nightClockOnly").checked:false,
 ultraNight:$("ultraNight")?$("ultraNight").checked:false,
 rTrails:$("rTrails")?$("rTrails").checked:true,
 rNearest:$("rNearest")?$("rNearest").checked:true,
 rAirports:$("rAirports")?$("rAirports").checked:true,
 rRings:$("rRings")?$("rRings").checked:true,
 rCompass:$("rCompass")?$("rCompass").checked:true,
 autoRotateBearing:$("autoRotateBearing")?$("autoRotateBearing").checked:false,
 timezone:$("timezone")?$("timezone").value:undefined,
 typeAirliner:$("tfAirliner")?$("tfAirliner").checked:true,
 typeLight:$("tfLight")?$("tfLight").checked:true,
 typeHeli:$("tfHeli")?$("tfHeli").checked:true,
 typeMilitary:$("tfMil")?$("tfMil").checked:true,
 typeHeavy:$("tfHeavy")?$("tfHeavy").checked:true,
 typeGlider:$("tfGlider")?$("tfGlider").checked:true,
 buzzerOn:$("buzzerOn")?$("buzzerOn").checked:true,
 buzzerEmergency:$("bzEmergency")?$("bzEmergency").checked:true,
 buzzerWatch:$("bzWatch")?$("bzWatch").checked:true,
 buzzerTouch:$("bzTouch")?$("bzTouch").checked:false,
 buzzerHourly:$("bzHourly")?$("bzHourly").checked:false,
 buzzerOverhead:$("bzOverhead")?$("bzOverhead").checked:false,
  buzzerPrecip:$("bzPrecip")?$("bzPrecip").checked:false,
  cPrecip:$("cPrecip")?$("cPrecip").checked:true,
 buzzerNightMute:$("bzNightMute")?$("bzNightMute").checked:true,
 hostname:$("hostname")?$("hostname").value.trim():undefined,
 cOver:$("cOver")?$("cOver").checked:true,
 ovRad:$("ovRad")?parseFloat($("ovRad").value)||10:10,
 screens:{clock:$("sClock").checked,planes:$("sPlanes").checked,meteo:$("sMeteo").checked,tactical:$("sTactical").checked,forecast:$("sForecast").checked,finance:$("sFinance")?$("sFinance").checked:true,iss:$("sIss")?$("sIss").checked:true,info:$("sInfo")?$("sInfo").checked:true},
 issAlert:$("issAlert")?$("issAlert").checked:false,
 financeTickers:$("financeTickers")?$("financeTickers").value.trim():undefined
};}

function getScreensObj(){
 return {
  clock: $("sClock") ? $("sClock").checked : true,
  planes: $("sPlanes") ? $("sPlanes").checked : true,
  meteo: $("sMeteo") ? $("sMeteo").checked : true,
  tactical: $("sTactical") ? $("sTactical").checked : true,
  forecast: $("sForecast") ? $("sForecast").checked : true,
  finance: $("sFinance") ? $("sFinance").checked : true,
  iss: $("sIss") ? $("sIss").checked : true,
  info: $("sInfo") ? $("sInfo").checked : true
 };
}

async function refreshIssLive(){
  try{
    await fetch("/api/iss/refresh",{method:"POST"});
    await status();
    msg((D[L]&&D[L].autoSaved)?D[L].autoSaved:"OK","ok");
  }catch(e){}
}

function saveScreens(){
 autoSave("screens", getScreensObj());
}

async function fetchStats(){
 try{
  const r=await fetch("/api/stats");
  const s=await r.json();
  if($("stCount")) $("stCount").textContent=s.todayCount;
  if($("stSpeed")){
    let spd=s.maxSpeedKt>0?(CFG&&CFG.metric?Math.round(s.maxSpeedKt*1.852)+" km/h":Math.round(s.maxSpeedKt)+" kt"):"-";
    if(s.maxSpeedCallsign) spd += " ("+s.maxSpeedCallsign+")";
    $("stSpeed").textContent=spd;
  }
  if($("stDist")) $("stDist").textContent=s.maxDistKm>0?(s.maxDistKm.toFixed(1)+" km"):"-";
  if($("stAlt")){
    let alt=s.maxAltFt>0?(CFG&&CFG.metric?Math.round(s.minAltFt*0.3048)+" - "+Math.round(s.maxAltFt*0.3048)+" m":"FL"+Math.round(s.minAltFt/100)+" - FL"+Math.round(s.maxAltFt/100)):"-";
    $("stAlt").textContent=alt;
  }
  if($("stReports")) $("stReports").textContent=s.totalSightings;
 }catch(e){}
}
async function resetStats(){
 if(!confirm((D[L]&&D[L].confirmResetStats)?D[L].confirmResetStats:"Naozaj resetovať dnešnú štatistiku letov?")) return;
 await fetch("/api/stats/reset",{method:"POST"});
 fetchStats();
 msg((D[L]&&D[L].statsResetOk)?D[L].statsResetOk:"Štatistiky boli resetované","ok");
}

function takeScreenshot(){
 const img=$("scrShotImg"),ph=$("scrShotPlaceholder"),dl=$("scrShotDl"),spin=$("scrShotSpin"),btn=$("btnScrShot");
 if(spin)spin.classList.remove("hide");
 if(btn)btn.disabled=true;
 const url="/api/screenshot.bmp?t="+Date.now();
 const pre=new Image();
 pre.onload=()=>{
  img.src=url;
  img.classList.remove("hide");
  img.style.display="block";
  if(ph){ph.classList.add("hide");ph.style.display="none";}
  if(dl){dl.href=url;dl.classList.remove("hide");dl.style.display="inline-block";}
  if(spin)spin.classList.add("hide");
  if(btn)btn.disabled=false;
  msg((D[L]&&D[L].scrShotOk)?D[L].scrShotOk:"Snímka načítaná","ok");
 };
 pre.onerror=()=>{
  if(spin)spin.classList.add("hide");
  if(btn)btn.disabled=false;
  msg((D[L]&&D[L].scrShotErr)?D[L].scrShotErr:"Chyba načítania","err");
 };
 pre.src=url;
}

async function testBuzzer(){
 try{
  await fetch("/api/buzzer/test",{method:"POST"});
  msg("Píp!","ok");
 }catch(e){msg(D[L].failed||"Chyba","err");}
}

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
  if(r.ok){
   msg(D[L].saved,"ok");
   const cr=await fetch("/api/config");
   CFG=await cr.json();
   renderSavedWifi(CFG.wifiNetworks);
  }else msg(D[L].failed,"err");
 }catch(e){msg(D[L].failed,"err");}
}

function renderSavedWifi(nets){
 const list=$("savedWifiList");
 if(!list)return;
 list.innerHTML="";
 if(!nets||!nets.length){
  list.innerHTML="<div class='hint' style='padding:4px 0;'>"+(D[L].noSavedWifi||"Žiadne uložené siete")+"</div>";
  if($("savedWifiCount"))$("savedWifiCount").textContent="0 / 5";
  return;
 }
 if($("savedWifiCount"))$("savedWifiCount").textContent=nets.length+" / 5";
 nets.forEach(n=>{
  const row=document.createElement("div");
  row.style.cssText="display:flex;align-items:center;justify-content:space-between;padding:7px 10px;background:rgba(255,255,255,0.03);border:1px solid var(--line);border-radius:6px;gap:8px;";
  const left=document.createElement("div");
  left.style.cssText="display:flex;align-items:center;gap:8px;";
  const icon=document.createElement("span");
  icon.textContent="📶";
  const name=document.createElement("span");
  name.style.fontWeight="600";
  name.textContent=n.ssid;
  left.appendChild(icon);
  left.appendChild(name);
  if(n.active){
   const badge=document.createElement("span");
   badge.style.cssText="background:rgba(34,197,94,0.15);color:var(--ok);border:1px solid rgba(34,197,94,0.3);font-size:11px;padding:1px 6px;border-radius:4px;font-weight:600;";
   badge.textContent=D[L].activeNet||"(aktívna)";
   left.appendChild(badge);
  }
  const delBtn=document.createElement("button");
  delBtn.type="button";
  delBtn.className="sec";
  delBtn.style.cssText="padding:3px 8px;font-size:12px;color:var(--err);border-color:rgba(239,68,68,0.3);";
  delBtn.textContent="✕ "+(D[L].btnForgetNet||"Zmazať");
  delBtn.onclick=()=>deleteWifiNet(n.ssid);
  row.appendChild(left);
  row.appendChild(delBtn);
  list.appendChild(row);
 });
}

async function deleteWifiNet(ssid){
 if(!confirm((D[L].delWifiConfirm||"Naozaj chcete zabudnúť")+" '"+ssid+"'?"))return;
 try{
  const r=await fetch("/api/wifi/delete",{
   method:"POST",
   headers:{"Content-Type":"application/json"},
   body:JSON.stringify({ssid:ssid,pass:$("oldPass")?$("oldPass").value:""})
  });
  if(r.status==401){msg(D[L].wrongPass||"Chybné heslo","err");return;}
  const res=await r.json();
  if(res.ok){
   msg(D[L].saved,"ok");
   const cr=await fetch("/api/config");
   CFG=await cr.json();
   renderSavedWifi(CFG.wifiNetworks);
  }else msg(D[L].failed,"err");
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

// --- GitHub OTA Client ---
let g_otaLatest = null;
let g_otaPollTimer = null;

async function checkGithubUpdates(userTriggered){
 const sp = $("otaSpinner"), st = $("otaStatusTxt");
 if(sp) sp.classList.remove("hide");
 if(st) st.textContent = (D[L] && D[L].otaChecking) ? D[L].otaChecking : "Kontrolujem...";
 if($("otaCurVer")) $("otaCurVer").textContent = "v" + (CFG.version || "1.5.6");

 try {
  let data = null;
  try {
   const r = await fetch("/api/ota/check");
   if(r.ok) data = await r.json();
  } catch(e) {}

  if(!data || !data.latest) {
   try {
    const gh = await fetch("https://api.github.com/repos/hackra76/ESP-MeteoPlaneRadar/releases/latest");
    if(gh.ok) {
     const gj = await gh.json();
     let otaUrl = "";
     if(gj.assets) {
      for(let a of gj.assets) {
       if(a.name && (a.name.indexOf("-ota.bin") >= 0 || a.name.indexOf("ota.bin") >= 0)) {
        otaUrl = a.browser_download_url;
        break;
       }
      }
     }
     const cur = CFG.version || "1.5.6";
     const lat = (gj.tag_name || "").replace(/^v/i, "");
     const [c1,c2,c3] = cur.split(".").map(Number);
     const [l1,l2,l3] = lat.split(".").map(Number);
     const isNewer = (l1>c1) || (l1===c1 && l2>c2) || (l1===c1 && l2===c2 && l3>c3);
     data = {
      current: cur,
      latest: gj.tag_name,
      updateAvailable: isNewer,
      name: gj.name,
      body: gj.body,
      url: otaUrl
     };
    }
   } catch(e) {}
  }

  if(!data) {
   if(st) st.textContent = (D[L] && D[L].failed) ? D[L].failed : "Chyba spojenia";
   return;
  }

  g_otaLatest = data;
  if($("otaLatVer")) $("otaLatVer").textContent = data.latest || "-";
  const badge = $("otaTagBadge");
  const btnInst = $("btnOtaInstall");
  const notes = $("otaReleaseNotes");

  if(data.body && notes) {
   notes.textContent = (data.name ? (data.name + "\n\n") : "") + data.body;
   notes.classList.remove("hide");
  } else if(notes) {
   notes.classList.add("hide");
  }

  if(data.updateAvailable) {
   if(badge) {
    badge.textContent = (D[L] && D[L].otaNewAvail) ? D[L].otaNewAvail : "Nová verzia";
    badge.className = "pill pill-warn";
    badge.classList.remove("hide");
   }
   if(data.url) {
    if(btnInst) btnInst.classList.remove("hide");
   } else {
    if(btnInst) btnInst.classList.add("hide");
    if(st) st.textContent = (D[L] && D[L].otaNoAsset) ? D[L].otaNoAsset : "Chýba OTA súbor";
   }
  } else {
   if(badge) {
    badge.textContent = (D[L] && D[L].otaUpToDate) ? D[L].otaUpToDate : "Aktuálna";
    badge.className = "pill pill-ok";
    badge.classList.remove("hide");
   }
   if(btnInst) btnInst.classList.add("hide");
  }
  if(st) st.textContent = "";
 } catch(e) {
  if(st) st.textContent = (D[L] && D[L].failed) ? D[L].failed : "Chyba";
 } finally {
  if(sp) sp.classList.add("hide");
 }
}

async function startGithubOta(){
 if(!g_otaLatest || !g_otaLatest.url) return;
 const cMsg = ((D[L] && D[L].otaConfirm) ? D[L].otaConfirm : "Naozaj spustiť aktualizáciu?") + " " + g_otaLatest.latest + "?";
 if(!confirm(cMsg)) return;

 let pass = "";
 if(CFG.hasPassword) {
  pass = prompt(((D[L] && D[L].adminPass) ? D[L].adminPass : "Heslo") + ":") || "";
  if(!pass) return;
 }

 const pWrap = $("otaProgWrap");
 const pBar = $("otaProgBar");
 const pTxt = $("otaPctTxt");
 const btnInst = $("btnOtaInstall");
 const btnCheck = $("btnOtaCheck");
 const sp = $("otaSpinner");
 const st = $("otaStatusTxt");

 if(pWrap) pWrap.classList.remove("hide");
 if(pBar) pBar.style.width = "0%";
 if(pTxt) pTxt.textContent = "0%";
 if(btnInst) btnInst.disabled = true;
 if(btnCheck) btnCheck.disabled = true;
 if(sp) sp.classList.remove("hide");
 if(st) st.textContent = (D[L] && D[L].otaDownloading) ? D[L].otaDownloading : "Sťahovanie...";

 try {
  const r = await fetch("/api/ota/start", {
   method: "POST",
   headers: {"Content-Type":"application/json"},
   body: JSON.stringify({
    url: g_otaLatest.url,
    tag: g_otaLatest.latest,
    password: pass
   })
  });
  if(r.status === 403 || r.status === 401) {
   alert((D[L] && D[L].wrongPass) ? D[L].wrongPass : "Chybné heslo");
   if(btnInst) btnInst.disabled = false;
   if(btnCheck) btnCheck.disabled = false;
   if(pWrap) pWrap.classList.add("hide");
   if(sp) sp.classList.add("hide");
   return;
  }
  if(!r.ok) {
   const err = await r.json().catch(()=>({}));
   alert(((D[L] && D[L].otaErr) ? D[L].otaErr : "Chyba:") + " " + (err.error || r.status));
   if(btnInst) btnInst.disabled = false;
   if(btnCheck) btnCheck.disabled = false;
   if(pWrap) pWrap.classList.add("hide");
   if(sp) sp.classList.add("hide");
   return;
  }

  clearInterval(g_otaPollTimer);
  g_otaPollTimer = setInterval(async () => {
   try {
    const sRes = await fetch("/api/ota/status");
    if(!sRes.ok) return;
    const s = await sRes.json();
    if(pBar) pBar.style.width = s.progress + "%";
    if(pTxt) pTxt.textContent = s.progress + "%";

    if(s.state === "downloading" || s.state === "flashing") {
     if(st) st.textContent = ((D[L] && D[L].otaDownloading) ? D[L].otaDownloading : "Sťahovanie...") + " (" + s.progress + "%)";
    } else if(s.state === "success") {
     clearInterval(g_otaPollTimer);
     if(pBar) pBar.style.width = "100%";
     if(pTxt) pTxt.textContent = "100%";
     if(st) st.textContent = (D[L] && D[L].otaSuccess) ? D[L].otaSuccess : "Hotovo! Reštartujem...";
     setTimeout(() => {
      alert((D[L] && D[L].otaSuccess) ? D[L].otaSuccess : "Hotovo! Reštartujem...");
      window.location.reload();
     }, 3000);
    } else if(s.state === "error") {
     clearInterval(g_otaPollTimer);
     if(st) st.textContent = ((D[L] && D[L].otaErr) ? D[L].otaErr : "Chyba:") + " " + (s.error || "failed");
     alert(((D[L] && D[L].otaErr) ? D[L].otaErr : "Chyba aktualizácie:") + " " + s.error);
     if(btnInst) btnInst.disabled = false;
     if(btnCheck) btnCheck.disabled = false;
    }
   } catch(e) {}
  }, 1000);

 } catch(e) {
  alert(((D[L] && D[L].otaErr) ? D[L].otaErr : "Chyba spojenia:") + " " + e.message);
  if(btnInst) btnInst.disabled = false;
  if(btnCheck) btnCheck.disabled = false;
 }
}

// --- Web Serial Monitor ---
let g_serialTimer = null;
let g_serialSince = 0;
let g_serialRunning = false;
let g_serialLogText = "";

function updateSerialBadge(active){
  const b = $("serialStatusBadge");
  const d = $("serialDot");
  const t = $("serialStateTxt");
  const btn = $("btnToggleSerialTxt");
  if(!b || !t) return;
  if(active){
    b.style.background = "rgba(34,197,94,0.15)";
    b.style.color = "var(--ok)";
    b.style.borderColor = "rgba(34,197,94,0.3)";
    if(d){ d.style.background = "var(--ok)"; d.style.boxShadow = "0 0 8px var(--ok)"; }
    t.textContent = (D[L] && D[L].serialActive) ? D[L].serialActive : "Aktívny (Live)";
    if(btn) btn.textContent = (D[L] && D[L].btnSerialPause) ? D[L].btnSerialPause : "Pozastaviť monitor";
  } else {
    b.style.background = "rgba(100,116,139,0.15)";
    b.style.color = "var(--mut)";
    b.style.borderColor = "rgba(100,116,139,0.3)";
    if(d){ d.style.background = "var(--mut)"; d.style.boxShadow = "none"; }
    t.textContent = (D[L] && D[L].serialPaused) ? D[L].serialPaused : "Pozastavené";
    if(btn) btn.textContent = (D[L] && D[L].btnSerialStart) ? D[L].btnSerialStart : "Spustiť monitor";
  }
}

function startSerialPolling(){
  if(g_serialRunning) return;
  g_serialRunning = true;
  updateSerialBadge(true);
  pollSerial();
  if(!g_serialTimer) {
    g_serialTimer = setInterval(pollSerial, 600);
  }
}

function stopSerialPolling(){
  g_serialRunning = false;
  if(g_serialTimer){
    clearInterval(g_serialTimer);
    g_serialTimer = null;
  }
  updateSerialBadge(false);
}

function toggleSerialMonitor(){
  if(g_serialRunning) {
    stopSerialPolling();
  } else {
    startSerialPolling();
  }
}

async function pollSerial(){
  if(!g_serialRunning) return;
  try {
    const res = await fetch("/api/serial/read?since=" + g_serialSince);
    if(!res.ok) return;
    const json = await res.json();
    g_serialSince = json.head;
    if(json.data && json.data.length > 0){
      appendSerialText(json.data);
    }
  } catch(e) {}
}

function appendSerialText(text){
  g_serialLogText += text;
  if(g_serialLogText.length > 400000){
    g_serialLogText = g_serialLogText.slice(-300000);
  }
  renderSerialDisplay();
}

function renderSerialDisplay(){
  const term = $("serialTerminal");
  if(!term) return;
  const filter = $("serialFilter") ? $("serialFilter").value.trim().toLowerCase() : "";
  let textToDisplay = g_serialLogText;
  if(filter){
    textToDisplay = g_serialLogText.split("\n").filter(l => l.toLowerCase().includes(filter)).join("\n");
  }
  term.textContent = textToDisplay;
  if($("serialBytesCounter")){
    $("serialBytesCounter").textContent = Math.round(g_serialLogText.length / 1024) + " KB";
  }
  if($("serialAutoScroll") && $("serialAutoScroll").checked){
    term.scrollTop = term.scrollHeight;
  }
}

function filterSerialLines(){
  renderSerialDisplay();
}

async function clearSerialLog(){
  g_serialLogText = "";
  renderSerialDisplay();
  try {
    await fetch("/api/serial/clear", { method: "POST" });
    msg((D[L] && D[L].serialCleared) ? D[L].serialCleared : "Konzola vymazaná", "ok");
  } catch(e) {}
}

function copySerialLog(){
  if(!g_serialLogText) return;
  if(navigator.clipboard && navigator.clipboard.writeText){
    navigator.clipboard.writeText(g_serialLogText).then(() => {
      msg((D[L] && D[L].serialCopied) ? D[L].serialCopied : "Skopírované", "ok");
    }).catch(() => fallbackCopy());
  } else {
    fallbackCopy();
  }
  function fallbackCopy(){
    const ta = document.createElement("textarea");
    ta.value = g_serialLogText;
    document.body.appendChild(ta);
    ta.select();
    document.execCommand("copy");
    document.body.removeChild(ta);
    msg((D[L] && D[L].serialCopied) ? D[L].serialCopied : "Skopírované", "ok");
  }
}

function downloadSerialLog(){
  if(!g_serialLogText) return;
  const blob = new Blob([g_serialLogText], { type: "text/plain;charset=utf-8" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = "serial_log_" + new Date().toISOString().replace(/[:.]/g,"-") + ".txt";
  a.click();
  URL.revokeObjectURL(a.href);
}

async function sendSerialCmd(){
  const input = $("serialCmdInput");
  if(!input) return;
  const val = input.value.trim();
  if(!val) return;
  input.value = "";
  try {
    await fetch("/api/serial/send", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ text: val })
    });
  } catch(e) {}
}

window.onload=load;
</script>
</body></html>
)rawliteral";
