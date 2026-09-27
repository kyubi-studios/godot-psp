#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
PROJ="$(cd "$1" && pwd)"; OUT="$2"
ELF="$GODOT_PSP_ROOT/bin/godot.psp.template_release.mips32.nothreads.elf"
rm -rf "$OUT"; mkdir -p "$OUT"
"$GODOT_PSP_ROOT/tools/psp/make_eboot.sh" "$ELF" "$OUT" "Godot PSP"
cp -r "$PROJ"/. "$OUT"/
