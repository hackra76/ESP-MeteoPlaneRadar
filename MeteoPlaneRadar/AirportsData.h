// =============================================================================
//  MeteoPlaneRadar
//  Airport data for the radar map underlay.
//
//  Curated list of major international and regional airports in Central Europe
//  and surrounding countries. Displayed as map waypoints with IATA codes.
//
//  Coordinates: WGS84 decimal degrees (lat, lon).
//  Tier 1 = major international hub (always shown)
//  Tier 2 = regional/national (shown at <= 200 km)
//  Tier 3 = domestic/small (shown at <= 100 km)
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#pragma once
#include <Arduino.h>

struct AirportEntry {
  const char* iata;      // 3-letter IATA code (e.g. "PRG")
  float lat;
  float lon;
  uint8_t tier;          // 1=major, 2=regional, 3=domestic
};

// --- Slovakia ---
// --- Czechia ---
// --- Austria ---
// --- Poland ---
// --- Hungary ---
// --- Germany (nearby) ---
// --- Others (nearby) ---

static const AirportEntry AIRPORTS[] = {
  // Slovakia
  {"BTS",  48.1702f, 17.2127f, 1},   // Bratislava M.R.Stefanik
  {"KSC",  48.6631f, 21.2411f, 2},   // Kosice
  {"TAT",  49.0736f, 20.2411f, 3},   // Poprad-Tatry
  {"SLD",  48.6378f, 19.1341f, 3},   // Sliac
  {"PZY",  48.6253f, 17.8284f, 3},   // Piestany
  {"ILZ",  49.2315f, 18.6135f, 3},   // Zilina

  // Czechia
  {"PRG",  50.1008f, 14.2600f, 1},   // Prague Vaclav Havel
  {"BRQ",  49.1513f, 16.6944f, 2},   // Brno Turany
  {"OSR",  49.6963f, 18.1111f, 2},   // Ostrava L.Janacek
  {"PED",  50.0133f, 15.7386f, 3},   // Pardubice
  {"KLV",  50.2028f, 12.9150f, 3},   // Karlovy Vary

  // Austria
  {"VIE",  48.1103f, 16.5697f, 1},   // Vienna Schwechat
  {"SZG",  47.7933f, 13.0043f, 2},   // Salzburg W.A.Mozart
  {"GRZ",  46.9911f, 15.4396f, 2},   // Graz
  {"INN",  47.2602f, 11.3440f, 2},   // Innsbruck
  {"LNZ",  48.2332f, 14.1876f, 2},   // Linz

  // Poland
  {"WAW",  52.1657f, 20.9671f, 1},   // Warsaw Chopin
  {"KRK",  50.0777f, 19.7848f, 1},   // Krakow Balice
  {"KTW",  50.4743f, 19.0800f, 1},   // Katowice Pyrzowice
  {"WRO",  51.1027f, 16.8858f, 2},   // Wroclaw
  {"GDN",  54.3776f, 18.4662f, 2},   // Gdansk Walesa
  {"POZ",  52.4211f, 16.8263f, 2},   // Poznan Lawica
  {"RZE",  50.1100f, 22.0190f, 2},   // Rzeszow Jasionka

  // Hungary
  {"BUD",  47.4369f, 19.2556f, 1},   // Budapest Liszt Ferenc
  {"DEB",  47.4889f, 21.6153f, 3},   // Debrecen

  // Germany (nearby)
  {"MUC",  48.3538f, 11.7861f, 1},   // Munich
  {"FRA",  50.0333f, 8.5706f,  1},   // Frankfurt
  {"BER",  52.3667f, 13.5033f, 1},   // Berlin Brandenburg
  {"DRS",  51.1328f, 13.7672f, 2},   // Dresden
  {"NUE",  49.4987f, 11.0669f, 2},   // Nuremberg
  {"LEJ",  51.4324f, 12.2416f, 2},   // Leipzig/Halle
  {"STR",  48.6899f, 9.2220f,  2},   // Stuttgart

  // Romania (nearby)
  {"OTP",  44.5711f, 26.0850f, 1},   // Bucharest Otopeni
  {"CLJ",  46.7852f, 23.6862f, 2},   // Cluj-Napoca

  // Croatia / Slovenia / Serbia
  {"ZAG",  45.7429f, 16.0688f, 1},   // Zagreb
  {"LJU",  46.2237f, 14.4576f, 2},   // Ljubljana
  {"BEG",  44.8184f, 20.3091f, 1},   // Belgrade

  // Ukraine (western)
  {"LWO",  49.8125f, 23.9561f, 2},   // Lviv

  // Italy (northern, nearby)
  {"VCE",  45.5053f, 12.3519f, 1},   // Venice Marco Polo
  {"MXP",  45.6306f, 8.7281f,  1},   // Milan Malpensa

  // Switzerland
  {"ZRH",  47.4647f, 8.5492f,  1},   // Zurich

  // Denmark / Baltic
  {"CPH",  55.6180f, 12.6560f, 1},   // Copenhagen
};

static const int AIRPORT_COUNT = sizeof(AIRPORTS) / sizeof(AIRPORTS[0]);
