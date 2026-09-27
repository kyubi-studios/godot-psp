#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
ELF="$1"; OUT="$2"; TITLE="${3:-Godot PSP}"
mkdir -p "$OUT"
TMP="$(mktemp -d)"
cp "$ELF" "$TMP/app.elf"
psp-fixup-imports "$TMP/app.elf"
psp-strip "$TMP/app.elf" -o "$TMP/app_s.elf"
mksfoex -d MEMSIZE=1 "$TITLE" "$TMP/PARAM.SFO"
pack-pbp "$OUT/EBOOT.PBP" "$TMP/PARAM.SFO" NULL NULL NULL NULL NULL "$TMP/app_s.elf" NULL >/dev/null
echo "[make_eboot] $(stat -c %s "$OUT/EBOOT.PBP") bytes -> $OUT/EBOOT.PBP"
rm -rf "$TMP"
