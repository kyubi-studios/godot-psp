#!/usr/bin/env bash
# Kullanım: run_test.sh <game_dir> <timeout_s> <regex>... [-- <bmp_check args>]
set -uo pipefail
source "$(dirname "$0")/env.sh"
[ -d "$1" ] || { echo "[run_test] FAIL: no game dir $1"; exit 1; }
GAME="$(cd "$1" && pwd)"; TIMEOUT="$2"; shift 2
PATTERNS=(); BMPARGS=()
while [ $# -gt 0 ]; do
  if [ "$1" = "--" ]; then shift; BMPARGS=("$@"); break; fi
  PATTERNS+=("$1"); shift
done
TOOLS="$(cd "$(dirname "$0")" && pwd)"
[ -f "$TOOLS/black.bmp" ] || python3 "$TOOLS/gen_black_bmp.py" "$TOOLS/black.bmp"
WORK="$(mktemp -d)"
( cd "$WORK" && timeout $((TIMEOUT + 30)) "$PPSSPP_HEADLESS" --graphics=software --timeout="$TIMEOUT" \
    --screenshot="$TOOLS/black.bmp" --max-mse=0 "$GAME/EBOOT.PBP" ) > "$GAME/test.log" 2>&1
rc=0
for p in "${PATTERNS[@]}"; do
  if ! grep -Eq "$p" "$GAME/test.log"; then echo "[run_test] MISSING: $p"; rc=1; fi
done
if grep -q "\[PSP\] FAIL" "$GAME/test.log"; then grep "\[PSP\] FAIL" "$GAME/test.log"; rc=1; fi
# Godot'un kendi hata satırları (bilinen zararsızlar tools/psp/known_errors.txt'de) testi düşürür.
ERRS=$(grep -A1 -E "^(USER )?(SCRIPT )?ERROR:" "$GAME/test.log" | grep -vE "^--$" | grep -vFf "$TOOLS/known_errors.txt" | grep -E "^(USER )?(SCRIPT )?ERROR:" || true)
if [ -n "$ERRS" ]; then echo "[run_test] GODOT ERRORS:"; echo "$ERRS"; rc=1; fi
# RUN_TEST_EXPECT_EXIT=1: EBOOT kendi çıkmalı; PPSSPP zaman aşımı başarısızlıktır.
if [ "${RUN_TEST_EXPECT_EXIT:-0}" = "1" ] && grep -q "TIMEOUT" "$GAME/test.log"; then echo "[run_test] TIMEOUT (EBOOT did not exit)"; rc=1; fi
if [ -f "$WORK/__testfailure.bmp" ]; then
  python3 "$TOOLS/bmp_check.py" "$WORK/__testfailure.bmp" "$GAME/screenshot.png" "${BMPARGS[@]}" || rc=1
elif [ ${#BMPARGS[@]} -gt 0 ]; then
  echo "[run_test] MISSING screenshot"; rc=1
fi
rm -rf "$WORK"
echo "[run_test] $([ $rc -eq 0 ] && echo PASS || echo FAIL) (log: $GAME/test.log)"
exit $rc
