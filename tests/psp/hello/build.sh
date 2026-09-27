#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/../../../tools/psp/env.sh"
D="$(cd "$(dirname "$0")" && pwd)"; OUT="$GODOT_PSP_ROOT/bin/psp_tests/hello"
mkdir -p "$OUT"
psp-gcc -O2 -G0 -I"$PSPDEV/psp/sdk/include" -D_PSP_FW_VERSION=600 "$D/main.c" -o "$OUT/hello.elf" \
  -L"$PSPDEV/psp/sdk/lib" -lpspgum -lpspgu -lpspdisplay -lpspge -lpspctrl -lm
"$GODOT_PSP_ROOT/tools/psp/make_eboot.sh" "$OUT/hello.elf" "$OUT" "PSP Hello"
