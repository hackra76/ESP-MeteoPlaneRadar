#include "FontEngine.h"
#include <lvgl.h>

static uint8_t s_fontSize = FONT_MEDIUM;
static uint16_t s_fgColor = 0xFFFF;

void Font_Init(Arduino_GFX* g) {
    // Nothing needed for LVGL mock
}

void Font_SetSize(uint8_t size) {
    s_fontSize = size;
}

void Font_SetColor(uint16_t fg, uint16_t bg, bool transparent) {
    s_fgColor = fg;
}

extern const lv_font_t lv_font_clock_76;
extern const lv_font_t lv_font_hero_54;
extern const lv_font_t lv_font_montserrat_custom_10;
extern const lv_font_t lv_font_montserrat_custom_14;
extern const lv_font_t lv_font_montserrat_custom_20;
extern const lv_font_t lv_font_montserrat_custom_28;

static const lv_font_t* get_lv_font(uint8_t size) {
    switch (size) {
        case FONT_TINY:   return &lv_font_montserrat_custom_10;
        case FONT_SMALL:  return &lv_font_montserrat_custom_14;
        case FONT_MEDIUM: return &lv_font_montserrat_custom_14;
        case FONT_LARGE:  return &lv_font_montserrat_custom_14;
        case FONT_TITLE:  return &lv_font_montserrat_custom_20;
        case FONT_HUGE:   return &lv_font_montserrat_custom_28;
        case FONT_HERO:   return &lv_font_hero_54;
        case FONT_CLOCK:  return &lv_font_clock_76;
        default: return &lv_font_montserrat_custom_14;
    }
}

static inline lv_color_t r565_to_lv(uint16_t c) {
    uint8_t r = (c >> 11) & 0x1F; r = (r << 3) | (r >> 2);
    uint8_t g = (c >> 5) & 0x3F;  g = (g << 2) | (g >> 4);
    uint8_t b = c & 0x1F;         b = (b << 3) | (b >> 2);
    uint32_t hex = (r << 16) | (g << 8) | b;
    return lv_color_hex(hex);
}

static void sanitizeUtf8(const char* src, char* dst, size_t maxLen) {
    size_t si = 0, di = 0;
    while (src[si] && di + 1 < maxLen) {
        uint8_t c = (uint8_t)src[si];
        if (c >= 0xF0 && c <= 0xF4) {
            int skip = 4;
            while (skip-- > 0 && src[si]) si++;
            continue;
        }
        dst[di++] = src[si++];
    }
    dst[di] = '\0';
}

void Font_Draw(const char* str, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    if (!gfx || !gfx->layer || !str || !str[0]) return;
    
    char clean[256];
    sanitizeUtf8(str, clean, sizeof(clean));
    if (!clean[0]) return;

    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = r565_to_lv(color);
    dsc.font = get_lv_font(size);
    dsc.text = clean;
    dsc.text_local = 1;
    
    lv_point_t size_res;
    lv_text_get_size(&size_res, clean, dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    
    lv_area_t a;
    lv_area_set(&a, x, y, x + size_res.x, y + size_res.y);
    lv_draw_label(gfx->layer, &dsc, &a);
}

void Font_Draw(const char* str, int16_t x, int16_t y) {
    Font_Draw(str, x, y, s_fgColor, s_fontSize);
}

void Font_DrawCentered(const char* str, int16_t cx, int16_t y, uint16_t color, uint8_t size) {
    if (!gfx || !gfx->layer || !str || !str[0]) return;
    
    char clean[256];
    sanitizeUtf8(str, clean, sizeof(clean));
    if (!clean[0]) return;

    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = r565_to_lv(color);
    dsc.font = get_lv_font(size);
    dsc.text = clean;
    dsc.align = LV_TEXT_ALIGN_CENTER;
    dsc.text_local = 1;
    
    lv_point_t size_res;
    lv_text_get_size(&size_res, clean, dsc.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    
    lv_area_t a;
    lv_area_set(&a, cx - size_res.x / 2, y, cx + size_res.x / 2, y + size_res.y);
    lv_draw_label(gfx->layer, &dsc, &a);
}

void Font_DrawCentered(const char* str, int16_t cx, int16_t y) {
    Font_DrawCentered(str, cx, y, s_fgColor, s_fontSize);
}

void Font_DrawCenteredIn(const char* str, int16_t x, int16_t w, int16_t y, uint16_t color, uint8_t size) {
    Font_DrawCentered(str, x + w / 2, y, color, size);
}

void Font_DrawCenteredBox(const char* str, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint8_t size) {
    if (!gfx || !gfx->layer || !str || !str[0]) return;
    char clean[256];
    sanitizeUtf8(str, clean, sizeof(clean));
    if (!clean[0]) return;

    const lv_font_t* font = get_lv_font(size);
    lv_point_t size_res;
    lv_text_get_size(&size_res, clean, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

    // Calculate vertical centering within the bounding box
    int16_t textY = y + (h - size_res.y) / 2;
    Font_DrawCentered(clean, x + w / 2, textY, color, size);
}

int16_t Font_TextWidth(const char* str, uint8_t size) {
    if (!str || !str[0]) return 0;
    char clean[256];
    sanitizeUtf8(str, clean, sizeof(clean));
    if (!clean[0]) return 0;
    lv_point_t size_res;
    lv_text_get_size(&size_res, clean, get_lv_font(size), 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size_res.x;
}

int16_t Font_TextHeight(uint8_t size) {
    return lv_font_get_line_height(get_lv_font(size));
}

// Dummy object to satisfy linker if it expects U8g2 object
class DummyU8G2 {
public:
    void nothing() {}
};
static DummyU8G2 dummy_u8g2;
// U8G2_FOR_ADAFRUIT_GFX& Font_GetU8g2() { return *(U8G2_FOR_ADAFRUIT_GFX*)&dummy_u8g2; }
