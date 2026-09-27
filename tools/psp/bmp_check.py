#!/usr/bin/env python3
"""Kullanım: bmp_check.py <in.bmp> <out.png> [--nonblack X,Y ...] [--rgb X,Y,R,G,B,TOL ...]
   [--brighter X1,Y1,X2,Y2,MINDIFF ...]  (X1,Y1 parlaklığı X2,Y2'den en az MINDIFF fazla)
   [--region-has X0,Y0,X1,Y1,R,G,B,TOL ...]  (dikdörtgende bu renge yakın en az bir piksel var)
   [--region-lacks X0,Y0,X1,Y1,R,G,B,TOL ...]  (dikdörtgende bu renge yakın hiç piksel yok)
   [--chgt X,Y,A,B,MINDIFF ...]  (X,Y pikselinde kanal A (0=R,1=G,2=B) kanal B'den en az MINDIFF büyük)"""
import sys
from PIL import Image

src, dst, *checks = sys.argv[1:]
img = Image.open(src).convert("RGB").crop((0, 0, 480, 272))
img.save(dst)
ok = True
i = 0
while i < len(checks):
    kind, arg = checks[i], checks[i + 1]
    i += 2
    if kind == "--nonblack":
        x, y = map(int, arg.split(","))
        px = img.getpixel((x, y))
        if px == (0, 0, 0):
            print(f"[bmp_check] FAIL pixel {x},{y} is black"); ok = False
    elif kind == "--rgb":
        x, y, r, g, b, tol = map(int, arg.split(","))
        px = img.getpixel((x, y))
        if max(abs(px[0] - r), abs(px[1] - g), abs(px[2] - b)) > tol:
            print(f"[bmp_check] FAIL pixel {x},{y} = {px}, want ({r},{g},{b})±{tol}"); ok = False
    elif kind == "--brighter":
        x1, y1, x2, y2, d = map(int, arg.split(","))
        lum = lambda p: (p[0] * 299 + p[1] * 587 + p[2] * 114) // 1000
        a, b = img.getpixel((x1, y1)), img.getpixel((x2, y2))
        if lum(a) - lum(b) < d:
            print(f"[bmp_check] FAIL brighter {x1},{y1}={a} vs {x2},{y2}={b} (need +{d})"); ok = False
    elif kind == "--chgt":
        x, y, a, b, d = map(int, arg.split(","))
        px = img.getpixel((x, y))
        if px[a] - px[b] < d:
            print(f"[bmp_check] FAIL chgt pixel {x},{y}={px}: ch{a} - ch{b} < {d}"); ok = False
    elif kind in ("--region-has", "--region-lacks"):
        x0, y0, x1, y1, r, g, b, tol = map(int, arg.split(","))
        found = any(max(abs(p[0] - r), abs(p[1] - g), abs(p[2] - b)) <= tol
                    for y in range(y0, y1) for x in range(x0, x1) for p in [img.getpixel((x, y))])
        if found != (kind == "--region-has"):
            print(f"[bmp_check] FAIL {kind} {x0},{y0}-{x1},{y1} color ({r},{g},{b})±{tol}"); ok = False
print("[bmp_check] OK" if ok else "[bmp_check] FAILED")
sys.exit(0 if ok else 1)
