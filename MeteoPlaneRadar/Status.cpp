// =============================================================================
//  MeteoPlaneRadar
//  Per-source status notes. See Status.h.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Status.h"
#include "Lang.h"
#include <stdarg.h>
#include <stdio.h>

#define STATUS_TEXT_MAX 48

static char          s_txt[ST_COUNT][STATUS_TEXT_MAX];
static unsigned long s_at[ST_COUNT] = {0};

void Status_Set(StatusSlot slot, const char* fmt, ...) {
  if (slot >= ST_COUNT) return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(s_txt[slot], STATUS_TEXT_MAX, fmt, ap);
  va_end(ap);
  s_at[slot] = millis();
}

void Status_Text(StatusSlot slot, char* out, size_t cap) {
  if (!out || cap == 0) return;
  out[0] = '\0';
  if (slot >= ST_COUNT) return;
  if (!s_at[slot]) { snprintf(out, cap, "-"); return; }

  unsigned long age = (millis() - s_at[slot]) / 1000UL;
  const bool en = (Lang_Get() == LANG_EN);
  if (age < 90)          snprintf(out, cap, "%s (%lu %s)", s_txt[slot], age, en ? "s ago" : "s zpet");
  else if (age < 5400)   snprintf(out, cap, "%s (%lu %s)", s_txt[slot], age / 60, en ? "min ago" : "min zpet");
  else                   snprintf(out, cap, "%s (%lu %s)", s_txt[slot], age / 3600, en ? "h ago" : "h zpet");
}
