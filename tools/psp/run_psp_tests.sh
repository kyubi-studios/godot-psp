#!/usr/bin/env bash
# Faz 1 test paketi: hello, boot, temiz çıkış + vsync + peak bellek, self-test.
set -uo pipefail
cd "$(dirname "$0")/../.."
source tools/psp/env.sh
rc=0
bash tests/psp/hello/build.sh >/dev/null && tools/psp/run_test.sh bin/psp_tests/hello 10 '\[PSP\] hello frame=10' -- --nonblack 240,160 --rgb 10,10,32,32,64,8 || rc=1
scons platform=psp -j12 >bin/psp_build.log 2>&1 || { echo "[psp_tests] BUILD FAIL"; exit 1; }
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/boot_empty >/dev/null 2>&1
tools/psp/run_test.sh bin/psp_tests/boot_empty 30 '\[PSP\] main enter' '\[PSP\] game_dir=umd0:/$' '\[PSP\] setup err=0' '\[PSP\] start OK' '\[PSP\] frame 120' || rc=1
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/quit >/dev/null 2>&1 && echo 180 > bin/psp_tests/quit/psp_quit_after_frames
RUN_TEST_EXPECT_EXIT=1 tools/psp/run_test.sh bin/psp_tests/quit 60 '\[PSP\] display_driver=psp' '\[PSP\] exit callback' '\[PSP\] close request sent' '\[PSP\] exit clean frames=18[0-9]' '\[PSP\] cleanup done' || rc=1
# auto_accept_quit=false: oyun kapanma isteğini yok sayar; watchdog çıkışı zorlamalı.
tools/psp/stage_game.sh tests/psp/projects/boot_noquit bin/psp_tests/noquit >/dev/null 2>&1 && echo 120 > bin/psp_tests/noquit/psp_quit_after_frames
RUN_TEST_EXPECT_EXIT=1 tools/psp/run_test.sh bin/psp_tests/noquit 60 '\[PSP\] exit callback' '\[PSP\] watchdog exit' || rc=1
tools/psp/check_vsync.sh bin/psp_tests/quit/test.log || rc=1
peak=$(grep -oE 'peak=[0-9]+' bin/psp_tests/quit/test.log | tail -1 | cut -d= -f2)
if [ "${peak:-99999999999}" -le 46137344 ]; then echo "[psp_tests] peak $peak <= 44 MB PASS"; else echo "[psp_tests] peak $peak FAIL"; rc=1; fi
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/selftest >/dev/null 2>&1 && touch bin/psp_tests/selftest/psp_selftest
RUN_TEST_EXPECT_EXIT=1 tools/psp/run_test.sh bin/psp_tests/selftest 20 '\[PSP\] OOM \(expected\)' '\[PSP\] selftest done fails=0' || rc=1
# Faz 2 — renderer
# psp_scene_test <proje> <ss_kare> <çıkış_kare> <bmp_check argümanları...>
psp_scene_test() {
  local name="$1" ss="$2" quit="$3"; shift 3
  tools/psp/stage_game.sh "tests/psp/projects/$name" "bin/psp_tests/$name" >/dev/null 2>&1
  echo "$ss" > "bin/psp_tests/$name/psp_screenshot_at_frame"; echo "$quit" > "bin/psp_tests/$name/psp_quit_after_frames"
  RUN_TEST_EXPECT_EXIT=1 tools/psp/run_test.sh "bin/psp_tests/$name" 40 "\[PSP\] screenshot frame=$ss" '\[PSP\] exit clean' -- "$@" || rc=1
}
psp_scene_test bg_color 30 40 --rgb 240,136,0,255,0,16 --rgb 5,5,0,255,0,16
# Kamera z=3'te, 1 birimlik küp: ekranın ortası kırmızı (unshaded), köşeler mavi arka plan.
psp_scene_test mesh_unshaded 30 40 --rgb 240,136,255,0,0,16 --rgb 5,5,0,0,255,16 --rgb 240,40,0,0,255,16
# 45° dönük beyaz küp, ışık +X yönünden: sağ yüz sol yüzden parlak (ters culling/normal bunu tersine çevirir).
psp_scene_test lit_box 30 40 --brighter 270,136,210,136,60 --rgb 5,5,0,0,0,8
# Işık kameranın arkasından (+Z): doğru culling'de görünen ön yüzler aydınlık; ters culling'de karanlık arka yüzler görünür.
psp_scene_test cull_check 30 40 --brighter 220,136,5,5,100 --brighter 260,136,5,5,100
# %50 saydam kırmızı, mavi arka plan önünde → mor.
psp_scene_test alpha_blend 30 40 --rgb 240,136,128,0,128,32 --rgb 5,5,0,0,255,16
# Yoğun beyaz derinlik fog'u: kırmızı küp neredeyse beyaz.
psp_scene_test fog 30 40 --rgb 240,136,255,255,255,80
# 2x2 dama texture'lı düzlem (nearest): sol üst kırmızı, sağ üst yeşil, sol alt yeşil.
psp_scene_test textured 30 40 --rgb 210,106,255,0,0,24 --rgb 270,106,0,255,0,24 --rgb 210,166,0,255,0,24 --rgb 5,5,0,0,255,16
echo "[psp_tests] $([ $rc -eq 0 ] && echo ALL PASS || echo FAILURES)"
exit $rc
