// MeteoPlaneRadar - vyvoj / development: chiptron.cz
// =============================================================================
//  Slovenske mesta - optimalizovany zoznam.
//
//  Zoznam obsahuje 8 krajskych miest a 9 vybranych regionalnych centier
//  rovnomerne rozmiestnenych po celom Slovensku pre cisty a prehladny radar.
// =============================================================================
#pragma once
#include "EuMapData.h"   // struct EuCity

#define SK_BOX_LAT0 47.70f
#define SK_BOX_LAT1 49.65f
#define SK_BOX_LON0 16.80f
#define SK_BOX_LON1 22.60f

static const EuCity SK_CITIES[] = {
  // Krajske mesta a metropoly (Tier 1)
  {"Bratislava", "BA", 17.1077f, 48.1486f, 1},
  {"Kosice", "KE", 21.2581f, 48.7164f, 1},
  {"Presov", "PO", 21.2407f, 48.9984f, 1},
  {"Zilina", "ZA", 18.7408f, 49.2232f, 1},
  {"Banska Bystrica", "BB", 19.1462f, 48.7363f, 1},
  {"Nitra", "NR", 18.0845f, 48.3061f, 1},
  {"Trnava", "TT", 17.5858f, 48.3775f, 1},
  {"Trencin", "TN", 18.0444f, 48.8945f, 1},

  // Vybrane klucove regionalne centra (Tier 2)
  {"Poprad", "PP", 20.3000f, 49.0500f, 2},
  {"Martin", "MT", 18.9220f, 49.0665f, 2},
  {"Zvolen", "ZV", 19.1245f, 48.5744f, 2},
  {"Prievidza", "PD", 18.6275f, 48.7745f, 2},
  {"Michalovce", "MI", 21.9195f, 48.7543f, 2},
  {"Nove Zamky", "NZ", 18.1619f, 47.9854f, 2},
  {"Liptovsky Mikulas", "LM", 19.6133f, 49.0806f, 2},
  {"Lucenec", "LC", 19.6671f, 48.3325f, 2},
  {"Humenne", "HE", 21.9079f, 48.9371f, 2}
};
static const int SK_CITY_COUNT = sizeof(SK_CITIES) / sizeof(SK_CITIES[0]);
