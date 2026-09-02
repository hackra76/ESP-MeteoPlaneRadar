// MeteoPlaneRadar - vyvoj / development: chiptron.cz & Antigravity
// =============================================================================
//  Slovenske mesta - kompletny a optimalizovany zoznam.
//
//  Tier 1: 8 krajskych miest a metropol
//  Tier 2: Hlavne regionalne centra a vacsie okresne mesta (vratane Bardejova)
//  Tier 3: Ostatne okresne mesta pre detailnejsie priblizenie
// =============================================================================
#pragma once
#include "EuMapData.h"   // struct EuCity

#define SK_BOX_LAT0 47.70f
#define SK_BOX_LAT1 49.65f
#define SK_BOX_LON0 16.80f
#define SK_BOX_LON1 22.60f

static const EuCity SK_CITIES[] = {
  // --- Krajske mesta a metropoly (Tier 1) ---
  {"Bratislava",        "BA",  17.1077f, 48.1486f, 1},
  {"Kosice",            "KE",  21.2581f, 48.7164f, 1},
  {"Presov",            "PO",  21.2407f, 48.9984f, 1},
  {"Zilina",            "ZA",  18.7408f, 49.2232f, 1},
  {"Banska Bystrica",   "BB",  19.1462f, 48.7363f, 1},
  {"Nitra",             "NR",  18.0845f, 48.3061f, 1},
  {"Trnava",            "TT",  17.5858f, 48.3775f, 1},
  {"Trencin",           "TN",  18.0444f, 48.8945f, 1},

  // --- Klucove regionalne centra a mesta (Tier 2) ---
  {"Poprad",            "PP",  20.3000f, 49.0500f, 2},
  {"Martin",            "MT",  18.9220f, 49.0665f, 2},
  {"Zvolen",            "ZV",  19.1245f, 48.5744f, 2},
  {"Prievidza",         "PD",  18.6275f, 48.7745f, 2},
  {"Bardejov",          "BJ",  21.2758f, 49.2918f, 2},
  {"Michalovce",        "MI",  21.9195f, 48.7543f, 2},
  {"Spisska Nova Ves",  "SN",  20.5670f, 48.9442f, 2},
  {"Nove Zamky",        "NZ",  18.1619f, 47.9854f, 2},
  {"Komarno",           "KN",  18.1294f, 47.7636f, 2},
  {"Levice",            "LV",  18.6075f, 48.2156f, 2},
  {"Liptovsky Mikulas", "LM",  19.6133f, 49.0806f, 2},
  {"Ruzomberok",        "RK",  19.3039f, 49.0747f, 2},
  {"Lucenec",           "LC",  19.6671f, 48.3325f, 2},
  {"Humenne",           "HE",  21.9079f, 48.9371f, 2},
  {"Povazska Bystrica", "PB",  18.4447f, 49.1161f, 2},
  {"Piestany",          "PN",  17.8283f, 48.5914f, 2},
  {"Dunajska Streda",   "DS",  17.6125f, 47.9928f, 2},
  {"Cadca",             "CA",  18.7897f, 49.4386f, 2},

  // --- Okresne mesta a vyznamne uzly (Tier 3) ---
  {"Svidnik",           "SK",  21.5714f, 49.3082f, 3},
  {"Stara Lubovna",     "SL",  20.6897f, 49.3014f, 3},
  {"Stropkov",          "SP",  21.6522f, 49.2025f, 3},
  {"Vranov n. T.",      "VT",  21.6847f, 48.8883f, 3},
  {"Trebisov",          "TV",  21.7196f, 48.6289f, 3},
  {"Snina",             "SV",  22.1524f, 48.9877f, 3},
  {"Kezmarok",          "KK",  20.4283f, 49.1357f, 3},
  {"Levoca",            "LE",  20.5894f, 49.0219f, 3},
  {"Sabinov",           "SB",  21.0983f, 49.1031f, 3},
  {"Roznava",           "RV",  20.5372f, 48.6614f, 3},
  {"Rimavska Sobota",   "RS",  20.0224f, 48.3828f, 3},
  {"Brezno",            "BR",  19.6417f, 48.8042f, 3},
  {"Dolny Kubin",       "DK",  19.2986f, 49.2081f, 3},
  {"Namestovo",         "NO",  19.4961f, 49.4078f, 3},
  {"Tvrdosin",          "TS",  19.5564f, 49.3333f, 3},
  {"Puchov",            "PU",  18.3247f, 49.1247f, 3},
  {"Dubnica n. V.",     "DCA", 18.1750f, 48.9597f, 3},
  {"Nove Mesto n. V.",  "NM",  17.8308f, 48.7567f, 3},
  {"Myjava",            "MY",  17.5686f, 48.7586f, 3},
  {"Banovce n. B.",     "BN",  18.2586f, 48.7197f, 3},
  {"Partizanske",       "PE",  18.3758f, 48.6272f, 3},
  {"Topolcany",         "TO",  18.1706f, 48.5600f, 3},
  {"Senica",            "SE",  17.3667f, 48.6792f, 3},
  {"Skalica",           "SI",  17.2250f, 48.8447f, 3},
  {"Malacky",           "MA",  17.0219f, 48.4361f, 3},
  {"Pezinok",           "PK",  17.2667f, 48.2861f, 3},
  {"Senec",             "SC",  17.4003f, 48.2197f, 3},
  {"Galanta",           "GA",  17.7281f, 48.1900f, 3},
  {"Sala",              "SA",  17.8714f, 48.1511f, 3},
  {"Hlohovec",          "HC",  17.8031f, 48.4319f, 3},
  {"Zlate Moravce",     "ZM",  18.3972f, 48.3847f, 3},
  {"Ziar n. H.",        "ZH",  18.8600f, 48.5917f, 3},
  {"Zarnovica",         "ZC",  18.7178f, 48.4828f, 3},
  {"Banska Stiavnica",  "BS",  18.8925f, 48.4586f, 3},
  {"Krupina",           "KA",  19.0647f, 48.3547f, 3},
  {"Velky Krtis",       "VK",  19.3503f, 48.2106f, 3},
  {"Detva",             "DT",  19.4186f, 48.5606f, 3},
  {"Revuca",            "RA",  20.1169f, 48.6833f, 3},
  {"Bytca",             "BY",  18.5586f, 49.2231f, 3},
  {"Kysucke N. M.",     "KM",  18.7861f, 49.3003f, 3},
  {"Sobrance",          "SO",  22.1814f, 48.7447f, 3},
  {"Gelnica",           "GL",  20.9369f, 48.8553f, 3},
  {"Medzilaborce",      "ML",  21.9042f, 49.2711f, 3},
  {"Turcianske Tepl.",  "TR",  18.8600f, 48.8617f, 3}
};
static const int SK_CITY_COUNT = sizeof(SK_CITIES) / sizeof(SK_CITIES[0]);
