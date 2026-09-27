#!/usr/bin/env bash
# Kullanım: check_size.sh <max_bytes> [eboot]  — EBOOT.PBP boyutu max'ı aşarsa başarısız.
set -euo pipefail
MAX="$1"; F="${2:-$(dirname "$0")/../../bin/psp_tests/boot_empty/EBOOT.PBP}"
SIZE=$(stat -c %s "$F")
if [ "$SIZE" -le "$MAX" ]; then echo "[check_size] PASS $SIZE <= $MAX"; else echo "[check_size] FAIL $SIZE > $MAX"; exit 1; fi
