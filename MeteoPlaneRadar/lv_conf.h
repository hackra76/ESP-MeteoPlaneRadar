#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE "Arduino.h"
#define LV_MEM_CUSTOM_ALLOC   ps_malloc
#define LV_MEM_CUSTOM_FREE    free
#define LV_MEM_CUSTOM_REALLOC ps_realloc

/* LVGL 9 ignores the v8 LV_MEM_CUSTOM_* macros above: its builtin TLSF pool
 * was a static 64 kB array in internal DRAM .bss. That internal RAM is exactly
 * what mbedTLS needs (2x ~17 kB record buffers), so place the pool in PSRAM. */
#define LV_MEM_SIZE (256 * 1024U)
#define LV_MEM_POOL_INCLUDE "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_48 1

#endif /*LV_CONF_H*/
