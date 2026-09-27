#!/usr/bin/env bash
# Kullanım: check_vsync.sh <test.log> — "[PSP] frame 60 ... t=" ile "frame 120" arası 900–1100 ms olmalı.
set -euo pipefail
t60=$(grep -oE '\[PSP\] frame 60 .*t=[0-9]+' "$1" | grep -oE 't=[0-9]+$' | cut -d= -f2)
t120=$(grep -oE '\[PSP\] frame 120 .*t=[0-9]+' "$1" | grep -oE 't=[0-9]+$' | cut -d= -f2)
d=$((t120 - t60))
if [ $d -ge 900 ] && [ $d -le 1100 ]; then echo "[check_vsync] PASS 60 frames = ${d} ms"; else echo "[check_vsync] FAIL 60 frames = ${d} ms"; exit 1; fi
