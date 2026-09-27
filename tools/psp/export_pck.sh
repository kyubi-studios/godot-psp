#!/usr/bin/env bash
# Godot 4.7 editörüyle (headless) projeyi import edip PSP için game.pck üretir.
# Kullanım: export_pck.sh <proje_klasörü> <çıktı.pck>
# Proje, adı "PSP" olan bir export preset'i içermelidir (herhangi bir masaüstü platformu; yalnızca .pck üretilir).
# Texture'lar "VRAM Uncompressed" (compress/mode=3) import edilmelidir: PSP'de PNG/WebP decoder yok.
set -euo pipefail
PROJ="$(cd "$1" && pwd)"; OUT="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
GODOT="${GODOT_EDITOR:-$(cd "$(dirname "$0")/../../.." && pwd)/Godot_v4.7-stable_mono_linux.x86_64}"
[ -x "$GODOT" ] || { echo "[export_pck] FAIL: Godot editor not found: $GODOT (set GODOT_EDITOR)"; exit 1; }
LOG="$(mktemp)"
timeout 600 "$GODOT" --headless --path "$PROJ" --import >"$LOG" 2>&1 || { tail -20 "$LOG"; exit 1; }
timeout 600 "$GODOT" --headless --path "$PROJ" --export-pack "PSP" "$OUT" >>"$LOG" 2>&1 || { tail -20 "$LOG"; exit 1; }
if grep -q "^ERROR" "$LOG"; then grep -A1 "^ERROR" "$LOG" | head -20; fi
rm -f "$LOG"
echo "[export_pck] $(stat -c %s "$OUT") bytes -> $OUT"
