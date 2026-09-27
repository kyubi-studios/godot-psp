#!/usr/bin/env python3
"""Kullanım: bmp_check.py <in.bmp> <out.png> [--nonblack X,Y ...] [--rgb X,Y,R,G,B,TOL ...]"""
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
print("[bmp_check] OK" if ok else "[bmp_check] FAILED")
sys.exit(0 if ok else 1)
