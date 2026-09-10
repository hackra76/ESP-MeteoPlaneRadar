// =============================================================================
//  MeteoPlaneRadar
//  FinanceData.h - Stock, Crypto & Commodity market data model and API client.
//
//  Fetches realtime prices, percentage changes, and 24-hour hourly sparkline
//  curves from Yahoo Finance chart v8 API without requiring any API keys.
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

#define FINANCE_SPARK_MAX 24

struct FinanceItem {
  char symbol[16];
  char shortName[32];
  char currency[8];
  float price;
  float changePct;
  float prevClose;
  float dayHigh;
  float dayLow;
  float sparkline[FINANCE_SPARK_MAX];
  uint8_t sparkCount;
  bool valid;
  unsigned long lastUpdated;
};

void Finance_Init();
void Finance_SetTickers(const char* tickersCsv);
const char* Finance_GetTickers();
int Finance_Count();
bool Finance_GetItem(int idx, FinanceItem* out);
const FinanceItem* Finance_GetItemPtr(int idx);
int Finance_ActiveIndex();
void Finance_SetActiveIndex(int idx);
void Finance_NextActive();

// Non-blocking step called from background Core 0 task
bool Finance_Step();
void Finance_RequestFetch();
bool Finance_IsBusy();
unsigned long Finance_LastUpdated();
