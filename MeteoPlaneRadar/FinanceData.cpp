// =============================================================================
//  MeteoPlaneRadar
//  FinanceData.cpp - Yahoo Finance v8 chart client & state manager.
//
//  Board: Waveshare ESP32-S3-Touch-LCD-2.1
// =============================================================================
#include "FinanceData.h"
#include "Net.h"
#include "AsyncCore.h"
#include "Settings.h"
#include <WiFi.h>
#include <ArduinoJson.h>

static FinanceItem s_items[FINANCE_MAX_ITEMS];
static int s_itemCount = 0;
static char s_tickersCsv[128] = DEFAULT_FINANCE_TICKERS;
static int s_activeIdx = 0;
static unsigned long s_lastCycleTime = 0;
static unsigned long s_lastStepTime = 0;
static int s_stepIdx = 0;
static bool s_isStepping = false;
static bool s_forceFetch = true;
static unsigned long s_lastUpdated = 0;

static void parseTickerList(const char* csv) {
  if (!csv || !*csv) csv = DEFAULT_FINANCE_TICKERS;
  strncpy(s_tickersCsv, csv, sizeof(s_tickersCsv) - 1);
  s_tickersCsv[sizeof(s_tickersCsv) - 1] = '\0';

  Async_LockFinance();
  s_itemCount = 0;
  char buf[128];
  strncpy(buf, s_tickersCsv, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char* tok = strtok(buf, ",; ");
  while (tok && s_itemCount < FINANCE_MAX_ITEMS) {
    while (*tok == ' ') tok++;
    if (*tok) {
      bool found = false;
      for (int i = 0; i < s_itemCount; i++) {
        if (strcasecmp(s_items[i].symbol, tok) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        strncpy(s_items[s_itemCount].symbol, tok, sizeof(s_items[s_itemCount].symbol) - 1);
        s_items[s_itemCount].symbol[sizeof(s_items[s_itemCount].symbol) - 1] = '\0';
        s_items[s_itemCount].shortName[0] = '\0';
        strncpy(s_items[s_itemCount].currency, "USD", sizeof(s_items[s_itemCount].currency));
        s_items[s_itemCount].price = 0.0f;
        s_items[s_itemCount].changePct = 0.0f;
        s_items[s_itemCount].prevClose = 0.0f;
        s_items[s_itemCount].dayHigh = 0.0f;
        s_items[s_itemCount].dayLow = 0.0f;
        s_items[s_itemCount].sparkCount = 0;
        s_items[s_itemCount].valid = false;
        s_items[s_itemCount].lastUpdated = 0;
        s_itemCount++;
      }
    }
    tok = strtok(nullptr, ",; ");
  }
  if (s_activeIdx >= s_itemCount) s_activeIdx = 0;
  Async_UnlockFinance();
}

void Finance_Init() {
  const char* saved = Settings_FinanceTickers();
  parseTickerList(saved);
  s_forceFetch = true;
}

void Finance_SetTickers(const char* tickersCsv) {
  parseTickerList(tickersCsv);
  s_forceFetch = true;
  s_stepIdx = 0;
  s_isStepping = true;
}

const char* Finance_GetTickers() {
  return s_tickersCsv;
}

int Finance_Count() {
  return s_itemCount;
}

bool Finance_GetItem(int idx, FinanceItem* out) {
  if (!out || idx < 0 || idx >= s_itemCount) return false;
  Async_LockFinance();
  *out = s_items[idx];
  Async_UnlockFinance();
  return true;
}

const FinanceItem* Finance_GetItemPtr(int idx) {
  if (idx < 0 || idx >= s_itemCount) return nullptr;
  return &s_items[idx];
}

int Finance_ActiveIndex() {
  return s_activeIdx;
}

void Finance_SetActiveIndex(int idx) {
  if (s_itemCount <= 0) return;
  if (idx >= 0 && idx < s_itemCount) {
    s_activeIdx = idx;
  }
}

void Finance_NextActive() {
  if (s_itemCount <= 0) return;
  s_activeIdx = (s_activeIdx + 1) % s_itemCount;
}

void Finance_RequestFetch() {
  s_forceFetch = true;
  s_stepIdx = 0;
  s_isStepping = true;
}

bool Finance_IsBusy() {
  return s_isStepping;
}

unsigned long Finance_LastUpdated() {
  return s_lastUpdated;
}

static void encodeSymbol(const char* in, char* out, size_t outCap) {
  size_t j = 0;
  for (size_t i = 0; in[i] && j + 3 < outCap; i++) {
    if (in[i] == '^') {
      out[j++] = '%'; out[j++] = '5'; out[j++] = 'E';
    } else {
      out[j++] = in[i];
    }
  }
  out[j] = '\0';
}

static bool fetchOneItem(int idx) {
  if (idx < 0 || idx >= s_itemCount) return false;

  char symEnc[32];
  encodeSymbol(s_items[idx].symbol, symEnc, sizeof(symEnc));

  char url[160];
  snprintf(url, sizeof(url), "https://query1.finance.yahoo.com/v8/finance/chart/%s?range=1d&interval=1h", symEnc);

  String body;
  if (!Net_GetString(url, body, "FINANCE")) {
    Serial.printf("FINANCE: Failed fetching %s\n", s_items[idx].symbol);
    return false;
  }

  JsonDocument filter;
  filter["chart"]["result"][0]["meta"]["symbol"] = true;
  filter["chart"]["result"][0]["meta"]["shortName"] = true;
  filter["chart"]["result"][0]["meta"]["currency"] = true;
  filter["chart"]["result"][0]["meta"]["regularMarketPrice"] = true;
  filter["chart"]["result"][0]["meta"]["chartPreviousClose"] = true;
  filter["chart"]["result"][0]["meta"]["regularMarketChangePercent"] = true;
  filter["chart"]["result"][0]["meta"]["regularMarketDayHigh"] = true;
  filter["chart"]["result"][0]["meta"]["regularMarketDayLow"] = true;
  filter["chart"]["result"][0]["indicators"]["quote"][0]["close"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
  if (err) {
    Serial.printf("FINANCE: JSON parse error for %s: %s\n", s_items[idx].symbol, err.c_str());
    return false;
  }

  JsonObject meta = doc["chart"]["result"][0]["meta"];
  if (meta.isNull()) {
    Serial.printf("FINANCE: Missing meta object for %s\n", s_items[idx].symbol);
    return false;
  }

  Async_LockFinance();
  FinanceItem& item = s_items[idx];

  const char* sn = meta["shortName"] | "";
  if (sn && *sn) {
    strncpy(item.shortName, sn, sizeof(item.shortName) - 1);
    item.shortName[sizeof(item.shortName) - 1] = '\0';
  } else if (!item.shortName[0]) {
    strncpy(item.shortName, item.symbol, sizeof(item.shortName) - 1);
  }

  const char* cur = meta["currency"] | "USD";
  strncpy(item.currency, cur, sizeof(item.currency) - 1);
  item.currency[sizeof(item.currency) - 1] = '\0';

  item.price = meta["regularMarketPrice"] | 0.0f;
  item.prevClose = meta["chartPreviousClose"] | item.price;
  item.dayHigh = meta["regularMarketDayHigh"] | item.price;
  item.dayLow = meta["regularMarketDayLow"] | item.price;

  if (meta.containsKey("regularMarketChangePercent") && !meta["regularMarketChangePercent"].isNull()) {
    item.changePct = meta["regularMarketChangePercent"].as<float>();
  } else if (item.prevClose > 0.0001f) {
    item.changePct = ((item.price - item.prevClose) / item.prevClose) * 100.0f;
  } else {
    item.changePct = 0.0f;
  }

  JsonArray closeArr = doc["chart"]["result"][0]["indicators"]["quote"][0]["close"];
  item.sparkCount = 0;
  if (!closeArr.isNull() && closeArr.size() > 0) {
    int total = closeArr.size();
    int start = (total > FINANCE_SPARK_MAX) ? (total - FINANCE_SPARK_MAX) : 0;
    float lastValid = item.prevClose > 0 ? item.prevClose : item.price;

    for (int i = start; i < total && item.sparkCount < FINANCE_SPARK_MAX; i++) {
      if (!closeArr[i].isNull()) {
        float val = closeArr[i].as<float>();
        if (val > 0.0f) {
          lastValid = val;
        }
      }
      item.sparkline[item.sparkCount++] = lastValid;
    }
  }

  // If no sparkline points in response, add prevClose and current price
  if (item.sparkCount < 2) {
    item.sparkline[0] = (item.prevClose > 0) ? item.prevClose : item.price;
    item.sparkline[1] = item.price;
    item.sparkCount = 2;
  }

  item.valid = (item.price > 0.0f);
  item.lastUpdated = millis();
  Async_UnlockFinance();

  Serial.printf("FINANCE: %s = %.2f %s (%.2f%%)\n", item.symbol, item.price, item.currency, item.changePct);
  return true;
}

bool Finance_Step() {
  if (WiFi.status() != WL_CONNECTED || s_itemCount <= 0) return false;

  unsigned long now = millis();

  if (!s_isStepping) {
    if (s_forceFetch || (now - s_lastCycleTime >= FINANCE_PERIOD_MS)) {
      s_isStepping = true;
      s_stepIdx = 0;
      s_forceFetch = false;
      s_lastCycleTime = now;
      s_lastStepTime = 0;
    } else {
      return false;
    }
  }

  if (s_isStepping) {
    if (now - s_lastStepTime < FINANCE_STEP_GAP_MS) {
      return false;
    }

    s_lastStepTime = now;
    bool updated = fetchOneItem(s_stepIdx);
    s_stepIdx++;

    if (s_stepIdx >= s_itemCount) {
      s_isStepping = false;
      s_stepIdx = 0;
      s_lastUpdated = now;
    }

    return updated;
  }

  return false;
}
