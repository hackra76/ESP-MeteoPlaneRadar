#pragma once
#include <stdint.h>
#include <lvgl.h>

// Convert RGB565 to LVGL native color
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

static inline lv_color_t rgb565_to_lv(uint16_t c) {
    // Extract RGB565 channels
    uint8_t r = (c >> 11) & 0x1F;
    uint8_t g = (c >> 5) & 0x3F;
    uint8_t b = c & 0x1F;
    // Scale to 8-bit
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    uint32_t hex = (r << 16) | (g << 8) | b;
    return lv_color_hex(hex);
}

class Arduino_GFX {
public:
    lv_layer_t* layer = nullptr;

    void setLayer(lv_layer_t* l) {
        layer = l;
    }

    void fillScreen(uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_COVER; dsc.bg_color = rgb565_to_lv(c);
        lv_area_t a;
        lv_area_set(&a, 0, 0, 479, 479);
        lv_draw_rect(layer, &dsc, &a);
    }
    
    void flush() {}

    void drawFastHLine(int x, int y, int w, uint16_t c) {
        drawLine(x, y, x + w - 1, y, c);
    }

    void drawFastVLine(int x, int y, int h, uint16_t c) {
        drawLine(x, y, x, y + h - 1, c);
    }

    void fillCircle(int x, int y, int r, uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_COVER; dsc.bg_color = rgb565_to_lv(c);
        dsc.radius = LV_RADIUS_CIRCLE;
        lv_area_t a;
        lv_area_set(&a, x - r, y - r, x + r, y + r);
        lv_draw_rect(layer, &dsc, &a);
    }

    void fillCircleOpa(int x, int y, int r, uint16_t c, lv_opa_t opa) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = opa; dsc.bg_color = rgb565_to_lv(c);
        dsc.radius = LV_RADIUS_CIRCLE;
        lv_area_t a;
        lv_area_set(&a, x - r, y - r, x + r, y + r);
        lv_draw_rect(layer, &dsc, &a);
    }

    void drawCircle(int x, int y, int r, uint16_t c) {
        if (!layer) return;
        lv_draw_arc_dsc_t dsc;
        lv_draw_arc_dsc_init(&dsc);
        dsc.color = rgb565_to_lv(c);
        dsc.width = 1;
        dsc.center.x = x;
        dsc.center.y = y;
        dsc.radius = r;
        dsc.start_angle = 0;
        dsc.end_angle = 360;
        lv_draw_arc(layer, &dsc);
    }

    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_COVER; dsc.bg_color = rgb565_to_lv(c);
        dsc.radius = r;
        lv_area_t a;
        lv_area_set(&a, x, y, x + w - 1, y + h - 1);
        lv_draw_rect(layer, &dsc, &a);
    }
    
    void drawRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_TRANSP;
        dsc.border_color = rgb565_to_lv(c);
        dsc.border_width = 1;
        dsc.border_post = true;
        dsc.radius = r;
        lv_area_t a;
        lv_area_set(&a, x, y, x + w - 1, y + h - 1);
        lv_draw_rect(layer, &dsc, &a);
    }

    void drawRect(int x, int y, int w, int h, uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_TRANSP;
        dsc.border_color = rgb565_to_lv(c);
        dsc.border_width = 1;
        dsc.border_post = true;
        lv_area_t a;
        lv_area_set(&a, x, y, x + w - 1, y + h - 1);
        lv_draw_rect(layer, &dsc, &a);
    }

    void drawLine(int x1, int y1, int x2, int y2, uint16_t c) {
        if (!layer) return;
        lv_draw_line_dsc_t dsc;
        lv_draw_line_dsc_init(&dsc);
        dsc.color = rgb565_to_lv(c);
        dsc.width = 1;
        dsc.p1.x = x1; dsc.p1.y = y1;
        dsc.p2.x = x2; dsc.p2.y = y2;
        lv_draw_line(layer, &dsc);
    }

    void drawPixel(int x, int y, uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_COVER; dsc.bg_color = rgb565_to_lv(c);
        lv_area_t a;
        lv_area_set(&a, x, y, x, y);
        lv_draw_rect(layer, &dsc, &a);
    }

    void draw16bitRGBBitmap(int x, int y, const uint16_t *bitmap, int w, int h) {
        if (!layer) return;
        lv_draw_image_dsc_t dsc;
        lv_draw_image_dsc_init(&dsc);
        // In LVGL 9, draw tasks are asynchronous. The image descriptor must persist.
        // We use a rotating array of descriptors since multiple images might be drawn per frame.
        static lv_image_dsc_t img_dscs[32];
        static int img_dsc_idx = 0;
        lv_image_dsc_t* img_dsc = &img_dscs[img_dsc_idx];
        img_dsc_idx = (img_dsc_idx + 1) % 32;
        
        img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
        img_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
        img_dsc->header.w = w;
        img_dsc->header.h = h;
        img_dsc->header.stride = w * 2;
        img_dsc->header.flags = 0;
        img_dsc->data_size = w * h * 2;
        img_dsc->data = (const uint8_t*)bitmap;
        
        dsc.src = img_dsc;
        lv_area_t a;
        lv_area_set(&a, x, y, x + w - 1, y + h - 1);
        lv_draw_image(layer, &dsc, &a);
    }

    void setTextWrap(bool w) {}
    
    void fillRect(int x, int y, int w, int h, uint16_t c) {
        if (!layer) return;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_opa = LV_OPA_COVER; dsc.bg_color = rgb565_to_lv(c);
        lv_area_t a;
        lv_area_set(&a, x, y, x + w - 1, y + h - 1);
        lv_draw_rect(layer, &dsc, &a);
    }

    void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t c) {
        if (!layer) return;
        lv_draw_triangle_dsc_t dsc;
        lv_draw_triangle_dsc_init(&dsc);
        dsc.color = rgb565_to_lv(c);
        dsc.p[0].x = x1; dsc.p[0].y = y1;
        dsc.p[1].x = x2; dsc.p[1].y = y2;
        dsc.p[2].x = x3; dsc.p[2].y = y3;
        lv_draw_triangle(layer, &dsc);
    }
};

extern Arduino_GFX* gfx;
