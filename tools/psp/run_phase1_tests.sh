#!/usr/bin/env bash
# Faz 1 test paketi: hello, boot, temiz çıkış + vsync + peak bellek, self-test.
set -uo pipefail
cd "$(dirname "$0")/../.."
source tools/psp/env.sh
rc=0
bash tests/psp/hello/build.sh >/dev/null && tools/psp/run_test.sh bin/psp_tests/hello 10 '\[PSP\] hello frame=10' -- --nonblack 240,160 --rgb 10,10,32,32,64,8 || rc=1
scons platform=psp -j12 >bin/psp_build.log 2>&1 || { echo "[phase1] BUILD FAIL"; exit 1; }
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/boot_empty >/dev/null 2>&1
tools/psp/run_test.sh bin/psp_tests/boot_empty 30 '\[PSP\] main enter' '\[PSP\] setup err=0' '\[PSP\] start OK' '\[PSP\] frame 120' || rc=1
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/quit >/dev/null 2>&1 && echo 180 > bin/psp_tests/quit/psp_quit_after_frames
tools/psp/run_test.sh bin/psp_tests/quit 60 '\[PSP\] display_driver=psp' '\[PSP\] exit clean frames=180' '\[PSP\] cleanup done' || rc=1
tools/psp/check_vsync.sh bin/psp_tests/quit/test.log || rc=1
peak=$(grep -oE 'peak=[0-9]+' bin/psp_tests/quit/test.log | tail -1 | cut -d= -f2)
if [ "${peak:-99999999999}" -le 46137344 ]; then echo "[phase1] peak $peak <= 44 MB PASS"; else echo "[phase1] peak $peak FAIL"; rc=1; fi
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/selftest >/dev/null 2>&1 && touch bin/psp_tests/selftest/psp_selftest
tools/psp/run_test.sh bin/psp_tests/selftest 20 '\[PSP\] selftest done fails=0' || rc=1
echo "[phase1] $([ $rc -eq 0 ] && echo ALL PASS || echo FAILURES)"
exit $rc
