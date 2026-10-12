#!/usr/bin/env python3
"""
Generate global Natural Earth boundary dataset with pre-baked bounding box spatial index.
Replaces European-only closed rings in EuMapData.h with worldwide TopoJSON boundary arcs.
"""
import urllib.request
import json
import math
import re
import os

URL_50M = "https://unpkg.com/world-atlas@2/countries-50m.json"

def d_pt_seg(px, py, x1, y1, x2, y2):
    dx = x2 - x1
    dy = y2 - y1
    if dx == 0 and dy == 0:
        return math.hypot(px - x1, py - y1)
    t = ((px - x1) * dx + (py - y1) * dy) / (dx * dx + dy * dy)
    t = max(0.0, min(1.0, t))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))

def douglas_peucker(pts, tol):
    if len(pts) <= 2:
        return pts
    max_d = 0.0
    idx = 0
    x1, y1 = pts[0]
    x2, y2 = pts[-1]
    for i in range(1, len(pts) - 1):
        d = d_pt_seg(pts[i][0], pts[i][1], x1, y1, x2, y2)
        if d > max_d:
            max_d = d
            idx = i
    if max_d > tol:
        left = douglas_peucker(pts[:idx+1], tol)
        right = douglas_peucker(pts[idx:], tol)
        return left[:-1] + right
    else:
        return [pts[0], pts[-1]]

def is_eu(pts):
    for lon, lat in pts:
        if 34.0 <= lat <= 72.0 and -32.0 <= lon <= 45.0:
            return True
    return False

def main():
    print(f"Fetching {URL_50M}...")
    req = urllib.request.Request(URL_50M, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req, timeout=30) as resp:
        data = json.loads(resp.read().decode('utf-8'))

    scale = data['transform']['scale']
    translate = data['transform']['translate']

    decoded_arcs = []
    for arc in data['arcs']:
        curr_x, curr_y = 0, 0
        pts = []
        for dx, dy in arc:
            curr_x += dx
            curr_y += dy
            lon = curr_x * scale[0] + translate[0]
            lat = curr_y * scale[1] + translate[1]
            pts.append((lon, lat))
        decoded_arcs.append(pts)

    print(f"Decoded {len(decoded_arcs)} raw arcs.")

    # Simplify
    simp_arcs = []
    for a in decoded_arcs:
        if len(a) < 2:
            continue
        tol = 0.008 if is_eu(a) else 0.025
        s = douglas_peucker(a, tol)
        if len(s) >= 2:
            simp_arcs.append(s)

    total_pts = sum(len(a) for a in simp_arcs)
    print(f"Simplified to {len(simp_arcs)} arcs, {total_pts} total points.")

    # Extract existing cities from MeteoPlaneRadar/EuMapData.h
    header_path = os.path.join(os.path.dirname(__file__), "..", "MeteoPlaneRadar", "EuMapData.h")
    cities_code = ""
    if os.path.exists(header_path):
        with open(header_path, "r", encoding="utf-8") as f:
            content = f.read()
            m = re.search(r'(struct EuCity\s*\{.*?const EuCity EU_CITIES\[\d+\]\s*=\s*\{.*?\};)', content, re.DOTALL)
            if m:
                cities_code = m.group(1)
                print("Extracted existing EU_CITIES successfully.")

    if not cities_code:
        raise RuntimeError("Failed to extract EU_CITIES from existing EuMapData.h!")

    # Format header output
    out = []
    out.append("// =============================================================================")
    out.append("//  MeteoPlaneRadar - Global Map Underlay Data")
    out.append("//  Worldwide country outlines and borders derived from Natural Earth (public domain).")
    out.append("//  GENERATED FILE - do not edit by hand.")
    out.append("// =============================================================================")
    out.append("#pragma once")
    out.append("#include <Arduino.h>")
    out.append("")
    out.append(f"#define GLOBAL_ARC_COUNT   {len(simp_arcs)}")
    out.append(f"#define GLOBAL_POINT_COUNT {total_pts}")
    out.append("#define EU_CITY_COUNT      1100")
    out.append("")
    out.append("// Fixed-point coordinate conversion:")
    out.append("// lon in degrees [-180, +180] maps to uint16 [0, 65535]")
    out.append("// lat in degrees [-90, +90]   maps to uint16 [0, 65535]")
    out.append("static inline float globalPtLon(uint16_t v) { return -180.0f + (float)v * (360.0f / 65535.0f); }")
    out.append("static inline float globalPtLat(uint16_t v) { return  -90.0f + (float)v * (180.0f / 65535.0f); }")
    out.append("")
    out.append("struct GlobalArcBox {")
    out.append("  int16_t  lonMin100; // degrees * 100")
    out.append("  int16_t  lonMax100;")
    out.append("  int16_t  latMin100;")
    out.append("  int16_t  latMax100;")
    out.append("  uint32_t startIdx;  // Index in GLOBAL_BORDER_PTS")
    out.append("  uint16_t count;     // Vertex count")
    out.append("};")
    out.append("")

    # Arc boxes
    out.append(f"const GlobalArcBox GLOBAL_ARC_BOXES[{len(simp_arcs)}] = {{")
    start_idx = 0
    all_quantized_pts = []
    for a in simp_arcs:
        min_lon = min(p[0] for p in a)
        max_lon = max(p[0] for p in a)
        min_lat = min(p[1] for p in a)
        max_lat = max(p[1] for p in a)
        cnt = len(a)
        out.append(f"  {{{int(round(min_lon*100))}, {int(round(max_lon*100))}, {int(round(min_lat*100))}, {int(round(max_lat*100))}, {start_idx}, {cnt}}},")
        start_idx += cnt
        for p in a:
            qx = int(round((p[0] + 180.0) * (65535.0 / 360.0)))
            qy = int(round((p[1] + 90.0) * (65535.0 / 180.0)))
            qx = max(0, min(65535, qx))
            qy = max(0, min(65535, qy))
            all_quantized_pts.append((qx, qy))
    out.append("};")
    out.append("")

    # Border points
    out.append(f"const uint16_t GLOBAL_BORDER_PTS[{len(all_quantized_pts)}][2] = {{")
    chunk = []
    for qx, qy in all_quantized_pts:
        chunk.append(f"{{{qx},{qy}}}")
        if len(chunk) >= 8:
            out.append("  " + ", ".join(chunk) + ",")
            chunk = []
    if chunk:
        out.append("  " + ", ".join(chunk) + ",")
    out.append("};")
    out.append("")

    # Append cities
    out.append("// --- Cities dataset ---")
    out.append(cities_code)
    out.append("")

    print(f"Writing to {header_path}...")
    with open(header_path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print("Done! EuMapData.h generated successfully.")

if __name__ == "__main__":
    main()
