from PIL import ImageFont, Image, ImageDraw
import os

TTF_PATH = './.pio/libdeps/esp32-s3-touch-lcd-21/lvgl/scripts/generators/built_in_font/Montserrat-Medium.ttf'
OUT_FILE = 'MeteoPlaneRadar/lv_font_montserrat_custom.c'

def generate_font_data(font_path, pt_size, var_name):
    font = ImageFont.truetype(font_path, pt_size)
    ascent, descent = font.getmetrics()
    line_height = ascent + descent
    base_line = descent

    glyphs = []
    bitmaps = bytearray()

    # Glyph 0 is dummy
    glyphs.append({'bitmap_index': 0, 'adv_w': 0, 'box_w': 0, 'box_h': 0, 'ofs_x': 0, 'ofs_y': 0})

    ranges = [
        (32, 95),   # ASCII 32..126
        (160, 96),  # Latin-1 160..255
        (256, 128), # Latin Extended-A 256..383
    ]

    for r_start, r_len in ranges:
        for code in range(r_start, r_start + r_len):
            ch = chr(code)
            adv = font.getlength(ch)
            adv_w = int(round(adv * 16))
            bbox = font.getbbox(ch) # (left, top, right, bottom)
            if not bbox or ch == ' ' or bbox[2] <= bbox[0] or bbox[3] <= bbox[1]:
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

    return {
        'var_name': var_name,
        'pt_size': pt_size,
        'line_height': line_height,
        'base_line': base_line,
        'glyphs': glyphs,
        'bitmaps': bitmaps
    }

def main():
    fonts_to_gen = [
        (10, 'lv_font_montserrat_custom_10'),
        (14, 'lv_font_montserrat_custom_14'),
        (20, 'lv_font_montserrat_custom_20'),
        (28, 'lv_font_montserrat_custom_28'),
    ]

    with open(OUT_FILE, 'w', encoding='utf-8') as f:
        f.write('#include "lvgl.h"\n\n')
        f.write('// Auto-generated Montserrat fonts with full Czech and Slovak (Latin-1 + Latin Extended-A)\n\n')

        for pt_size, var_name in fonts_to_gen:
            print(f'Generating {var_name} (size {pt_size})...')
            fdata = generate_font_data(TTF_PATH, pt_size, var_name)
            bitmaps = fdata['bitmaps']
            glyphs = fdata['glyphs']

            f.write(f'// =============================================================================\n')
            f.write(f'//  {var_name} - {pt_size}px\n')
            f.write(f'// =============================================================================\n')
            f.write(f'static const uint8_t {var_name}_bitmap[] = {{\n')
            for i in range(0, len(bitmaps), 16):
                chunk = bitmaps[i:i+16]
                f.write('    ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
            f.write('};\n\n')

            f.write(f'static const lv_font_fmt_txt_glyph_dsc_t {var_name}_glyph_dsc[] = {{\n')
            for g in glyphs:
                f.write(f"    {{.bitmap_index = {g['bitmap_index']}, .adv_w = {g['adv_w']}, .box_w = {g['box_w']}, .box_h = {g['box_h']}, .ofs_x = {g['ofs_x']}, .ofs_y = {g['ofs_y']}}},\n")
            f.write('};\n\n')

            f.write(f'static const lv_font_fmt_txt_cmap_t {var_name}_cmaps[] = {{\n')
            f.write('    {\n')
            f.write('        .range_start = 32, .range_length = 95, .glyph_id_start = 1,\n')
            f.write('        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0,\n')
            f.write('        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY\n')
            f.write('    },\n')
            f.write('    {\n')
            f.write('        .range_start = 160, .range_length = 96, .glyph_id_start = 96,\n')
            f.write('        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0,\n')
            f.write('        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY\n')
            f.write('    },\n')
            f.write('    {\n')
            f.write('        .range_start = 256, .range_length = 128, .glyph_id_start = 192,\n')
            f.write('        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0,\n')
            f.write('        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY\n')
            f.write('    }\n')
            f.write('};\n\n')

            f.write(f'static const lv_font_fmt_txt_dsc_t {var_name}_dsc = {{\n')
            f.write(f'    .glyph_bitmap = {var_name}_bitmap,\n')
            f.write(f'    .glyph_dsc = {var_name}_glyph_dsc,\n')
            f.write(f'    .cmaps = {var_name}_cmaps,\n')
            f.write('    .kern_dsc = NULL,\n')
            f.write('    .kern_scale = 0,\n')
            f.write('    .cmap_num = 3,\n')
            f.write('    .bpp = 4,\n')
            f.write('    .kern_classes = 0,\n')
            f.write('    .bitmap_format = 0,\n')
            f.write('};\n\n')

            f.write(f'const lv_font_t {var_name} = {{\n')
            f.write('    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,\n')
            f.write('    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,\n')
            f.write(f"    .line_height = {fdata['line_height']},\n")
            f.write(f"    .base_line = {fdata['base_line']},\n")
            f.write('    .subpx = LV_FONT_SUBPX_NONE,\n')
            f.write('    .underline_position = -4,\n')
            f.write('    .underline_thickness = 2,\n')
            f.write(f'    .dsc = &{var_name}_dsc,\n')
            f.write('    .fallback = NULL,\n')
            f.write('    .user_data = NULL,\n')
            f.write('};\n\n')

    print(f'Saved all fonts to {OUT_FILE}')

if __name__ == '__main__':
    main()
