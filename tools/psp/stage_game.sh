#!/usr/bin/env bash
# Kullanım: stage_game.sh <proje_klasörü | oyun.pck> <çıktı_klasörü>
# Klasör verilirse içerik EBOOT'un yanına kopyalanır (--path); .pck verilirse game.pck olarak (--main-pack).
set -euo pipefail
source "$(dirname "$0")/env.sh"
SRC="$1"; OUT="$2"
ELF="$GODOT_PSP_ROOT/bin/godot.psp.template_release.mips32.nothreads.elf"
rm -rf "$OUT"; mkdir -p "$OUT"
"$GODOT_PSP_ROOT/tools/psp/make_eboot.sh" "$ELF" "$OUT" "Godot PSP"
if [ -f "$SRC" ]; then
  cp "$SRC" "$OUT/game.pck"
else
  cp -r "$(cd "$SRC" && pwd)"/. "$OUT"/
fi
