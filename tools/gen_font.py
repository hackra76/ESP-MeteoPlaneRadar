from PIL import ImageFont, Image, ImageDraw
import os

def generate_font_c(font_path, pt_size, var_name, out_file):
    font = ImageFont.truetype(font_path, pt_size)
    ascent, descent = font.getmetrics()
    line_height = ascent + descent
    base_line = descent

    glyphs = []
    bitmaps = bytearray()

    # Glyph 0 is dummy
    glyphs.append({'bitmap_index': 0, 'adv_w': 0, 'box_w': 0, 'box_h': 0, 'ofs_x': 0, 'ofs_y': 0})

    for code in range(32, 127):
        ch = chr(code)
        adv = font.getlength(ch)
        adv_w = int(round(adv * 16))
        bbox = font.getbbox(ch) # (left, top, right, bottom)
        if not bbox or ch == ' ':
            glyphs.append({'bitmap_index': len(bitmaps), 'adv_w': adv_w, 'box_w': 0, 'box_h': 0, 'ofs_x': 0, 'ofs_y': 0})
            continue

        left, top, right, bottom = bbox
        bw = right - left
        bh = bottom - top
        ofs_x = left
        ofs_y = ascent - bottom # baseline to bottom of box

        # Render glyph to mask image
        img = Image.new('L', (bw, bh), 0)
        draw = ImageDraw.Draw(img)
        draw.text((-left, -top), ch, font=font, fill=255)

        # Convert to 4-bpp packed bytes
        pixels = list(img.getdata())
        b_idx = len(bitmaps)
        
        for i in range(0, len(pixels), 2):
            p0 = (pixels[i] * 15 + 127) // 255
            p1 = (pixels[i+1] * 15 + 127) // 255 if (i + 1 < len(pixels)) else 0
            bitmaps.append((p0 << 4) | (p1 & 0x0F))

        glyphs.append({
            'bitmap_index': b_idx,
            'adv_w': adv_w,
            'box_w': bw,
            'box_h': bh,
            'ofs_x': ofs_x,
            'ofs_y': ofs_y
        })

    with open(out_file, 'w', encoding='utf-8') as f:
        f.write('#include "lvgl.h"\n\n')
        f.write(f'// Font generated: {var_name}, size {pt_size}px, ASCII 32..126\n')
        f.write('static const uint8_t glyph_bitmap[] = {\n')
        for i in range(0, len(bitmaps), 16):
            chunk = bitmaps[i:i+16]
            f.write('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
        f.write('};\n\n')

        f.write('static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {\n')
        for g in glyphs:
            f.write(f"    {{.bitmap_index = {g['bitmap_index']}, .adv_w = {g['adv_w']}, .box_w = {g['box_w']}, .box_h = {g['box_h']}, .ofs_x = {g['ofs_x']}, .ofs_y = {g['ofs_y']}}},\n")
        f.write('};\n\n')

        f.write('static const lv_font_fmt_txt_cmap_t cmaps[] = {\n')
        f.write('    {\n')
        f.write('        .range_start = 32, .range_length = 95, .glyph_id_start = 1,\n')
        f.write('        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0,\n')
        f.write('        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY\n')
        f.write('    }\n')
        f.write('};\n\n')

        f.write('static const lv_font_fmt_txt_dsc_t font_dsc = {\n')
        f.write('    .glyph_bitmap = glyph_bitmap,\n')
        f.write('    .glyph_dsc = glyph_dsc,\n')
        f.write('    .cmaps = cmaps,\n')
        f.write('    .kern_dsc = NULL,\n')
        f.write('    .kern_scale = 0,\n')
        f.write('    .cmap_num = 1,\n')
        f.write('    .bpp = 4,\n')
        f.write('    .kern_classes = 0,\n')
        f.write('    .bitmap_format = 0,\n')
        f.write('};\n\n')

        f.write(f'const lv_font_t {var_name} = {{\n')
        f.write('    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,\n')
        f.write('    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,\n')
        f.write(f'    .line_height = {line_height},\n')
        f.write(f'    .base_line = {base_line},\n')
        f.write('    .subpx = LV_FONT_SUBPX_NONE,\n')
        f.write('    .underline_position = -4,\n')
        f.write('    .underline_thickness = 2,\n')
        f.write('    .dsc = &font_dsc,\n')
        f.write('    .fallback = NULL,\n')
        f.write('    .user_data = NULL,\n')
        f.write('};\n')

    print(f'Generated {out_file} with {len(bitmaps)} bytes of bitmaps.')

if __name__ == '__main__':
    font_path = 'C:/Windows/Fonts/segoeuib.ttf'
    if not os.path.exists(font_path):
        font_path = 'C:/Windows/Fonts/arialbd.ttf'
    generate_font_c(font_path, 76, 'lv_font_clock_76', 'MeteoPlaneRadar/Font_Clock_76.c')
    generate_font_c(font_path, 54, 'lv_font_hero_54', 'MeteoPlaneRadar/Font_Hero_54.c')
