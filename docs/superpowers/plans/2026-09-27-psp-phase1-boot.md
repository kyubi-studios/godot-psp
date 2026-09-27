# PSP Port — Faz 1: Test Altyapısı, Spike ve Platform Katmanı — Uygulama Planı

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Godot 4.7.2'yi PSP için derleyip headless PPSSPP'de açmak, ikili boyutu ve RAM kullanımını ölçmek (karar noktası), ardından girdi/vsync/çıkış destekli `platform/psp` katmanını tamamlamak.

**Architecture:** Yeni `mips32` mimarisi ve `platform/psp` SCons platformu. `OS_PSP : OS_Unix` (`UNIX_ENABLED` tanımlı; newlib/pspdev'de olmayan POSIX çağrıları `#ifdef PSP_ENABLED` ile korunur). `threads=no` ile tek thread. Render bu fazda Godot'un `dummy` renderer'ıdır; PSP GU renderer'ı Faz 2 planıdır. Test, kaynaktan derlenmiş `PPSSPPHeadless` ile yapılır: EBOOT `emulator:` devctl ile log satırı ve ekran görüntüsü gönderir.

**Tech Stack:** pspdev (GCC 15.2, newlib, pspsdk), SCons, Godot 4.7.2-stable, PPSSPP 1.20.4 headless, SDL2 2.30.9 (yalnızca PPSSPP için), Python 3 + Pillow (test betikleri).

**Spec:** `docs/superpowers/specs/2026-09-27-godot4-psp-port-design.md`

**Kapsam notu:** Spec'teki uygulama sırasının 1–3. adımları bu plandadır. 4. adım (renderer) ve 5. adım (demo + export) spike'ın karar noktasından sonra ayrı bir Faz 2 planında yazılacak; çünkü renderer tasarımının ayrıntıları spike'ta ölçülecek bellek ve boyut değerlerine bağlıdır.

## Global Constraints

- Godot sürümü: 4.7.2-stable, dal `psp-port`.
- Hedef: PSP-2001 (Slim), `MEMSIZE=1` ile ~52 MB kullanıcı belleği; `PSP_HEAP_SIZE_KB(-1024)`.
- SCons: `platform=psp target=template_release arch=mips32 threads=no modules_enabled_by_default=no disable_physics_3d=yes disable_navigation_3d=yes disable_xr=yes deprecated=no minizip=no optimize=size`.
- Derleme bayrakları: `-Os`, `-G0`, `-fno-exceptions` (`disable_exceptions=yes`), RTTI kapalı olabiliyorsa kapalı.
- GDScript, fizik, ses, navigasyon, XR derlenmez.
- Spike hedefi: ikili < 20 MB, boşta RAM < 32 MB. **İkili > 30 MB → dur, kullanıcıya sor.**
- Bütçe sınırı: peak RAM > 44 MB → test başarısız.
- Log satırı biçimi: `[PSP] <anahtar> <değerler>`; hata satırı `[PSP] FAIL <neden>`.
- Araçlar repo dışında: `~/pspdev`, `~/ppsspp-deps`, `~/ppsspp-build`. Sisteme (sudo) kurulum yok.
- Her commit mesajı şu satırla biter: `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`

## Review Focus

1. EBOOT, çalışma dizini olmadan (`ms0:/PSP/GAME/X/` veya PPSSPP'nin `umd0:`/`disc0:` eşlemesiyle) açıldığında proje yolunun `argv[0]`'dan doğru çözülmesi — Task 3'te `--path` argv[0] dizininden türetilir ve log'a basılır; test bu satırı doğrular.
2. `threads=no` iken Godot'un `WorkerThreadPool` veya `ResourceLoader` thread'li yolları hâlâ çağırması — Task 3 testi `Main::start()` + 120 kare sonrası `FAIL` satırı olmadığını doğrular.
3. Bellek tükenmesi (`malloc` NULL) — Task 3'te `godot_psp.cpp` açılışta ve her 60 karede `Memory::get_mem_usage()` ile `get_mem_max_usage()` loglar; test peak ≤ 44 MB kontrol eder.
4. HOME tuşu / `sceKernelExitGame` ile çıkışta takılma — Task 4 testi, exit callback'ini taklit eden `--psp-quit-after-frames=N` ile döngünün temiz bittiğini (`[PSP] exit clean`) doğrular.
5. Aynı anda basılı birden çok PSP tuşunun Godot joypad olaylarına eşlenmesi — Task 4'teki saf fonksiyon `psp_ctrl_map()` için EBOOT içi self-test birden çok tuş kombinasyonunu doğrular.

---

### Task 1: Headless PPSSPP ve test betikleri

**Files:**
- Create: `tools/psp/env.sh`
- Create: `tools/psp/build_ppsspp_headless.sh`
- Create: `tools/psp/make_eboot.sh`
- Create: `tools/psp/run_test.sh`
- Create: `tools/psp/gen_black_bmp.py`
- Create: `tools/psp/bmp_check.py`
- Create: `tests/psp/common/psp_test_log.h`
- Create: `tests/psp/hello/main.c`
- Create: `tests/psp/hello/build.sh`

**Interfaces:**
- Produces:
  - `source tools/psp/env.sh` → `PSPDEV`, `PATH`, `PPSSPP_HEADLESS` (tam yol) ortam değişkenleri.
  - `tools/psp/make_eboot.sh <elf> <out_dir> <title>` → `<out_dir>/EBOOT.PBP` (MEMSIZE=1).
  - `tools/psp/run_test.sh <out_dir> <timeout_s> <regex>...` → çıkış kodu 0: tüm regex'ler log'da var ve `[PSP] FAIL` yok. Log: `<out_dir>/test.log`, ekran görüntüsü: `<out_dir>/screenshot.png`.
  - `tests/psp/common/psp_test_log.h`: `psp_test_log(const char *fmt, ...)`, `psp_test_screenshot(void)` (C ve C++'tan kullanılabilir).

- [ ] **Step 1: Ortam betiği**

`tools/psp/env.sh`:
```bash
#!/usr/bin/env bash
# Kaynaklayın: source tools/psp/env.sh
export PSPDEV="$HOME/pspdev"
export PATH="$PSPDEV/bin:$PATH"
export PPSSPP_HEADLESS="$HOME/ppsspp-build/PPSSPPHeadless"
export GODOT_PSP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
```

- [ ] **Step 2: PPSSPP derleme betiği**

`tools/psp/build_ppsspp_headless.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
SRC="${PPSSPP_SRC:-$(cd "$(dirname "$0")/../../.." && pwd)/godot4-psp-tools/ppsspp-1.20.4}"
DEPS="$HOME/ppsspp-deps"
BUILD="$HOME/ppsspp-build"
mkdir -p "$DEPS" "$BUILD"
if [ ! -f "$DEPS/inst/lib/cmake/SDL2/SDL2Config.cmake" ]; then
  cd "$DEPS"
  [ -d SDL2-2.30.9 ] || curl -sL https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz | tar xz
  cmake -G Ninja -S SDL2-2.30.9 -B sdlb -DCMAKE_INSTALL_PREFIX="$DEPS/inst" -DCMAKE_BUILD_TYPE=Release -DSDL_TEST=OFF
  ninja -C sdlb install
fi
cd "$BUILD"
cmake -G Ninja "$SRC" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Release -DUSING_QT_UI=OFF -DUNITTEST=OFF \
  -DUSE_DISCORD=OFF -DUSE_FFMPEG=OFF -DCMAKE_PREFIX_PATH="$DEPS/inst"
ninja PPSSPPHeadless
ls -la "$BUILD/PPSSPPHeadless"
```

Run: `bash tools/psp/build_ppsspp_headless.sh` (uzun sürer; `timeout` 600000 ms, gerekirse arka planda).
Expected: son satırda `PPSSPPHeadless` dosyası listelenir.
Hata olursa: CMake'in hangi paketi bulamadığını oku; eksik bir `USE_*`/`ENABLE_*` seçeneği kapatılır ya da bağımlılık `$DEPS/inst` altına aynı yöntemle derlenir. Sisteme paket kurulmaz.

- [ ] **Step 3: EBOOT paketleme betiği**

`tools/psp/make_eboot.sh`:
```bash
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
```

- [ ] **Step 4: Siyah referans BMP ve BMP kontrolü**

`tools/psp/gen_black_bmp.py` (PPSSPP `Compare.cpp` 54 byte başlık + 512×272×4 byte bekler):
```python
#!/usr/bin/env python3
import struct, sys
w, h = 512, 272
data = b"\x00" * (w * h * 4)
hdr = b"BM" + struct.pack("<IHHI", 54 + len(data), 0, 0, 54)
dib = struct.pack("<IiiHHIIiiII", 40, w, h, 1, 32, 0, len(data), 2835, 2835, 0, 0)
open(sys.argv[1], "wb").write(hdr + dib + data)
```

`tools/psp/bmp_check.py` (PPSSPP'nin yazdığı `__testfailure.bmp`'yi PNG'ye çevirir, isteğe bağlı piksel kontrolü yapar):
```python
#!/usr/bin/env python3
"""Kullanım: bmp_check.py <in.bmp> <out.png> [--nonblack X,Y ...] [--rgb X,Y,R,G,B,TOL ...]"""
import sys
from PIL import Image

src, dst, *checks = sys.argv[1:]
img = Image.open(src).convert("RGB").crop((0, 0, 480, 272))
img.save(dst)
ok = True
i = 0
while i < len(checks):
    kind, arg = checks[i], checks[i + 1]
    i += 2
    if kind == "--nonblack":
        x, y = map(int, arg.split(","))
        px = img.getpixel((x, y))
        if px == (0, 0, 0):
            print(f"[bmp_check] FAIL pixel {x},{y} is black"); ok = False
    elif kind == "--rgb":
        x, y, r, g, b, tol = map(int, arg.split(","))
        px = img.getpixel((x, y))
        if max(abs(px[0] - r), abs(px[1] - g), abs(px[2] - b)) > tol:
            print(f"[bmp_check] FAIL pixel {x},{y} = {px}, want ({r},{g},{b})±{tol}"); ok = False
print("[bmp_check] OK" if ok else "[bmp_check] FAILED")
sys.exit(0 if ok else 1)
```

- [ ] **Step 5: Test çalıştırıcı**

`tools/psp/run_test.sh`:
```bash
#!/usr/bin/env bash
# Kullanım: run_test.sh <game_dir> <timeout_s> <regex>... [-- <bmp_check args>]
set -uo pipefail
source "$(dirname "$0")/env.sh"
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
if [ -f "$WORK/__testfailure.bmp" ]; then
  python3 "$TOOLS/bmp_check.py" "$WORK/__testfailure.bmp" "$GAME/screenshot.png" "${BMPARGS[@]}" || rc=1
elif [ ${#BMPARGS[@]} -gt 0 ]; then
  echo "[run_test] MISSING screenshot"; rc=1
fi
rm -rf "$WORK"
echo "[run_test] $([ $rc -eq 0 ] && echo PASS || echo FAIL) (log: $GAME/test.log)"
exit $rc
```

- [ ] **Step 6: Başarısız testi çalıştır**

Run: `chmod +x tools/psp/*.sh && tools/psp/run_test.sh /tmp/psp_hello_missing 5 'hello frame=10'`
Expected: FAIL (`cd` hatası / `MISSING: hello frame=10`). Betik zinciri çalışıyor, test edilecek EBOOT henüz yok.

- [ ] **Step 7: Log/screenshot başlığı**

`tests/psp/common/psp_test_log.h`:
```c
#pragma once
#include <pspiofilemgr.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// PPSSPP "emulator:" devctl komutları (Core/HLE/sceIo.cpp). Gerçek PSP'de sessizce başarısız olur.
#define PSP_EMU_DEVCTL_SEND_OUTPUT 2
#define PSP_EMU_DEVCTL_EMIT_SCREENSHOT 0x20

static inline void psp_test_log(const char *fmt, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	if (n < 0) {
		return;
	}
	if (n > (int)sizeof(buf) - 2) {
		n = (int)sizeof(buf) - 2;
	}
	buf[n++] = '\n';
	buf[n] = 0;
	fputs(buf, stdout);
	sceIoDevctl("emulator:", PSP_EMU_DEVCTL_SEND_OUTPUT, buf, n, NULL, 0);
}

static inline void psp_test_screenshot(void) {
	sceIoDevctl("emulator:", PSP_EMU_DEVCTL_EMIT_SCREENSHOT, NULL, 0, NULL, 0);
}
```

- [ ] **Step 8: Hello EBOOT (GU üçgeni)**

`tests/psp/hello/main.c`:
```c
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include "../common/psp_test_log.h"

PSP_MODULE_INFO("psphello", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[4096];
typedef struct { unsigned int color; float x, y, z; } Vertex;
static Vertex __attribute__((aligned(16))) tri[3] = {
	{ 0xff0000ff, -1, -1, 0 }, { 0xff00ff00, 1, -1, 0 }, { 0xffff0000, 0, 1, 0 }
};

int main(void) {
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
	sceGuDispBuffer(480, 272, (void *)0x88000, 512);
	sceGuDepthBuffer((void *)0x110000, 512);
	sceGuOffset(2048 - 240, 2048 - 136);
	sceGuViewport(2048, 2048, 480, 272);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
	for (int f = 0; f <= 10; f++) {
		sceGuStart(GU_DIRECT, list);
		sceGuClearColor(0xff402020);
		sceGuClear(GU_COLOR_BUFFER_BIT);
		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumPerspective(60, 480.0f / 272.0f, 0.5f, 100);
		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();
		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		ScePspFVector3 p = { 0, 0, -3 };
		sceGumTranslate(&p);
		sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 3, 0, tri);
		sceGuFinish();
		sceGuSync(0, 0);
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}
	psp_test_log("[PSP] hello frame=10");
	psp_test_screenshot();
	sceKernelExitGame();
	return 0;
}
```

`tests/psp/hello/build.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/../../../tools/psp/env.sh"
D="$(cd "$(dirname "$0")" && pwd)"; OUT="$GODOT_PSP_ROOT/bin/psp_tests/hello"
mkdir -p "$OUT"
psp-gcc -O2 -G0 -I"$PSPDEV/psp/sdk/include" -D_PSP_FW_VERSION=600 "$D/main.c" -o "$OUT/hello.elf" \
  -L"$PSPDEV/psp/sdk/lib" -lpspgum -lpspgu -lpspdisplay -lpspge -lpspctrl -lm
"$GODOT_PSP_ROOT/tools/psp/make_eboot.sh" "$OUT/hello.elf" "$OUT" "PSP Hello"
```

- [ ] **Step 9: Testi geçir**

Run: `bash tests/psp/hello/build.sh && tools/psp/run_test.sh bin/psp_tests/hello 10 '\[PSP\] hello frame=10' -- --nonblack 240,160 --rgb 10,10,32,32,64,8`
Expected: `[bmp_check] OK` ve `[run_test] PASS`. `screenshot.png`'yi Read aracıyla açıp üçgenin göründüğünü gözle doğrula.
Not: `--rgb` değeri koyu mavi arka plandır (clear rengi `0xff402020` → ABGR → R=0x20, G=0x20, B=0x40). Değer uymazsa önce ekran görüntüsüne bak; renk kanalı sırası yanlışsa `bmp_check.py`'yi düzelt, testi değil.

- [ ] **Step 10: Commit**

```bash
git add tools/psp tests/psp/common tests/psp/hello
git commit -m "psp: headless PPSSPP test harness and hello EBOOT

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

`bin/` zaten Godot'un `.gitignore`'unda; `tools/psp/black.bmp` üretilen dosyadır, commit edilmez (`tools/psp/.gitignore` içine `black.bmp` ekle).

---

### Task 2: `mips32` mimarisi, `platform/psp` iskeleti ve motorun derlenmesi

**Files:**
- Modify: `platform_methods.py:20` (`architectures` listesi)
- Modify: `core/os/spin_lock.h:81` (MIPS `_cpu_pause`)
- Create: `platform/psp/detect.py`
- Create: `platform/psp/SCsub`
- Create: `platform/psp/platform_config.h`
- Create: `platform/psp/os_psp.h`
- Create: `platform/psp/os_psp.cpp`
- Create: `platform/psp/godot_psp.cpp`
- Create: `platform/psp/psp_log.h`
- Create: `docs/psp/porting-notes.md`
- Modify (derleme hatalarına göre): `drivers/unix/*.cpp`, `core/**` — her değişiklik `#ifdef PSP_ENABLED` ile ve porting-notes'a bir satırla.

**Interfaces:**
- Consumes: Task 1 `make_eboot.sh`, `run_test.sh`.
- Produces:
  - `bin/godot.psp.template_release.mips32.elf`
  - `platform/psp/psp_log.h`: `void psp_log(const char *fmt, ...)` (Task 1 başlığının C++ karşılığı, `[PSP]` önekini çağıran yazar).
  - `class OS_PSP : public OS_Unix` — `void run()`, `bool quit_requested` (public), `String get_name() const override` → `"PSP"`.
  - Önişlemci tanımları: `PSP_ENABLED`, `UNIX_ENABLED`.

- [ ] **Step 1: Başarısız derleme**

Run: `source tools/psp/env.sh && scons platform=psp 2>&1 | tail -3`
Expected: FAIL — `Invalid target platform "psp"` benzeri hata.

- [ ] **Step 2: Mimariyi ekle**

`platform_methods.py` içinde:
```python
architectures = ["x86_32", "x86_64", "arm32", "arm64", "rv64", "ppc64", "wasm32", "wasm64", "loongarch64", "mips32"]
```
ve `architecture_aliases` sözlüğüne `"mips": "mips32",` ekle.

`core/os/spin_lock.h` içinde RISC-V satırından sonra:
```cpp
#elif defined(__mips__) // MIPS (PSP Allegrex).
	asm volatile("nop");
```

- [ ] **Step 3: `detect.py`**

`platform/psp/detect.py`:
```python
import os
import sys
from typing import TYPE_CHECKING

from methods import print_error
from platform_methods import validate_arch

if TYPE_CHECKING:
    from SCons.Script.SConscript import SConsEnvironment


def get_name():
    return "PSP"


def get_pspdev():
    return os.environ.get("PSPDEV", os.path.expanduser("~/pspdev"))


def can_build():
    return os.path.exists(os.path.join(get_pspdev(), "bin", "psp-g++"))


def get_tools(env: "SConsEnvironment"):
    return ["cc", "c++", "ar", "link", "textfile"]


def get_opts():
    return []


def get_doc_classes():
    return []


def get_doc_path():
    return "doc_classes"


def get_flags():
    return {
        "arch": "mips32",
        "target": "template_release",
        "threads": False,
        "modules_enabled_by_default": False,
        "disable_physics_2d": True,
        "disable_physics_3d": True,
        "disable_navigation_2d": True,
        "disable_navigation_3d": True,
        "disable_xr": True,
        "deprecated": False,
        "minizip": False,
        "brotli": False,
        "vulkan": False,
        "opengl3": False,
        "disable_exceptions": True,
        "optimize": "size",
        "accesskit": False,
        "sdl": False,
    }


def configure(env: "SConsEnvironment"):
    validate_arch(env["arch"], get_name(), ["mips32"])
    if env.dev_build or env["target"] != "template_release":
        print_error("PSP: only target=template_release is supported.")
        sys.exit(255)

    pspdev = get_pspdev()
    psp_sdk = os.path.join(pspdev, "psp", "sdk")
    bin_dir = os.path.join(pspdev, "bin")

    env["CC"] = os.path.join(bin_dir, "psp-gcc")
    env["CXX"] = os.path.join(bin_dir, "psp-g++")
    env["AR"] = os.path.join(bin_dir, "psp-ar")
    env["RANLIB"] = os.path.join(bin_dir, "psp-ranlib")
    env["LINK"] = os.path.join(bin_dir, "psp-g++")
    env["PROGSUFFIX"] = ".elf"
    env["ENV"]["PATH"] = bin_dir + os.pathsep + env["ENV"]["PATH"]

    env.Append(CPPPATH=[os.path.join(psp_sdk, "include"), os.path.join(pspdev, "psp", "include")])
    env.Append(CPPDEFINES=["PSP_ENABLED", "UNIX_ENABLED", "UNIX_SOCKET_UNAVAILABLE", "_PSP_FW_VERSION=600", "NO_SAFE_CAST"])
    env.Append(CCFLAGS=["-G0", "-ffunction-sections", "-fdata-sections"])
    env.Append(LINKFLAGS=["-G0", "-Wl,--gc-sections"])
    env.Append(LIBPATH=[os.path.join(psp_sdk, "lib")])
    env.Append(
        LIBS=[
            "pspgum", "pspgu", "pspge", "pspdisplay", "pspctrl", "psppower", "psprtc",
            "pspdebug", "pspnet_inet", "pspnet", "pspuser", "pspkernel", "atomic", "m", "c",
        ]
    )
```

Not: `NO_SAFE_CAST` Godot'un RTTI'siz `Object::cast_to` yolunu seçer; derleme hata verirse bu tanımı kaldır ve porting-notes'a yaz.

- [ ] **Step 4: `SCsub`, `platform_config.h`, `psp_log.h`**

`platform/psp/SCsub`:
```python
#!/usr/bin/env python
from misc.utility.scons_hints import *

Import("env")

psp_files = [
    "godot_psp.cpp",
    "os_psp.cpp",
]

prog = env.add_program("#bin/godot", psp_files)
```

`platform/psp/platform_config.h`:
```cpp
#pragma once

#include <alloca.h>
```

`platform/psp/psp_log.h`:
```cpp
#pragma once

// PSP log: stdout + PPSSPP "emulator:" SEND_OUTPUT devctl. Satır başına "[PSP] " yazmak çağıranın işidir.
void psp_log(const char *p_format, ...) __attribute__((format(printf, 1, 2)));
void psp_screenshot();
```

- [ ] **Step 5: `OS_PSP`**

`platform/psp/os_psp.h`:
```cpp
#pragma once

#include "drivers/unix/os_unix.h"

class OS_PSP : public OS_Unix {
	MainLoop *main_loop = nullptr;

protected:
	void initialize() override;
	void set_main_loop(MainLoop *p_main_loop) override;
	void delete_main_loop() override;
	void finalize() override;
	bool _check_internal_feature_support(const String &p_feature) override;

public:
	volatile bool quit_requested = false;
	int quit_after_frames = -1; // --psp-quit-after-frames=N (test için)

	String get_name() const override { return "PSP"; }
	String get_distribution_name() const override { return "PSP"; }
	String get_version() const override { return "6.61"; }
	MainLoop *get_main_loop() const override { return main_loop; }
	Vector<String> get_video_adapter_driver_info() const override { return Vector<String>(); }
	void initialize_joypads() override {}
	Error get_entropy(uint8_t *r_buffer, int p_bytes) override;
	void run();

	OS_PSP();
};
```

`platform/psp/os_psp.cpp`:
```cpp
#include "os_psp.h"

#include "psp_log.h"

#include "core/config/project_settings.h"
#include "main/main.h"
#include "servers/display/display_server.h"

#include <pspkernel.h>
#include <psprtc.h>

void OS_PSP::initialize() {
	OS_Unix::initialize_core();
}

void OS_PSP::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

void OS_PSP::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

void OS_PSP::finalize() {
	delete_main_loop();
}

bool OS_PSP::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "psp" || p_feature == "mobile";
}

Error OS_PSP::get_entropy(uint8_t *r_buffer, int p_bytes) {
	// Kriptografik değil; PSP'de güvenli kaynak kullanılmıyor.
	u64 tick = 0;
	sceRtcGetCurrentTick(&tick);
	for (int i = 0; i < p_bytes; i++) {
		tick = tick * 6364136223846793005ULL + 1442695040888963407ULL;
		r_buffer[i] = uint8_t(tick >> 56);
	}
	return OK;
}

void OS_PSP::run() {
	if (!main_loop) {
		return;
	}
	main_loop->initialize();
	int frame = 0;
	while (!quit_requested) {
		DisplayServer::get_singleton()->process_events();
		if (Main::iteration()) {
			break;
		}
		frame++;
		if ((frame % 60) == 0) {
			psp_log("[PSP] frame %d mem=%llu peak=%llu", frame,
					(unsigned long long)Memory::get_mem_usage(), (unsigned long long)Memory::get_mem_max_usage());
		}
		if (quit_after_frames > 0 && frame >= quit_after_frames) {
			break;
		}
	}
	main_loop->finalize();
	psp_log("[PSP] exit clean frames=%d", frame);
}

OS_PSP::OS_PSP() {
}
```

- [ ] **Step 6: `godot_psp.cpp` (spike sürümü; `DisplayServerHeadless` + dummy renderer)**

`platform/psp/godot_psp.cpp`:
```cpp
#include "os_psp.h"
#include "psp_log.h"

#include "main/main.h"

#include <pspiofilemgr.h>
#include <pspkernel.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("GodotPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);

static OS_PSP *g_os = nullptr;

void psp_log(const char *p_format, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, p_format);
	int n = vsnprintf(buf, sizeof(buf) - 1, p_format, ap);
	va_end(ap);
	if (n < 0) {
		return;
	}
	if (n > (int)sizeof(buf) - 2) {
		n = (int)sizeof(buf) - 2;
	}
	buf[n++] = '\n';
	buf[n] = 0;
	fputs(buf, stdout);
	sceIoDevctl("emulator:", 2, buf, n, nullptr, 0);
}

void psp_screenshot() {
	sceIoDevctl("emulator:", 0x20, nullptr, 0, nullptr, 0);
}

static int psp_exit_callback(int, int, void *) {
	if (g_os) {
		g_os->quit_requested = true;
	}
	return 0;
}

static int psp_callback_thread(SceSize, void *) {
	int cbid = sceKernelCreateCallback("exit_cb", psp_exit_callback, nullptr);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

static void psp_setup_callbacks() {
	int thid = sceKernelCreateThread("cb_thread", psp_callback_thread, 0x11, 0x1000, THREAD_ATTR_USER, nullptr);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, nullptr);
	}
}

// argv[0]'ın dizini (ör. "ms0:/PSP/GAME/GodotDemo/EBOOT.PBP" → "ms0:/PSP/GAME/GodotDemo").
static void psp_game_dir(const char *p_argv0, char *r_out, size_t p_size) {
	strncpy(r_out, p_argv0 ? p_argv0 : "", p_size - 1);
	r_out[p_size - 1] = 0;
	char *slash = strrchr(r_out, '/');
	if (slash) {
		*slash = 0;
	}
}

int main(int argc, char *argv[]) {
	psp_setup_callbacks();
	psp_log("[PSP] main enter argv0=%s", argc > 0 ? argv[0] : "(none)");

	OS_PSP os;
	g_os = &os;

	char game_dir[256];
	psp_game_dir(argc > 0 ? argv[0] : nullptr, game_dir, sizeof(game_dir));
	psp_log("[PSP] game_dir=%s", game_dir);

	char *args[] = {
		(char *)"--path", game_dir,
		(char *)"--display-driver", (char *)"headless",
		(char *)"--rendering-method", (char *)"dummy",
		(char *)"--audio-driver", (char *)"Dummy",
	};
	int nargs = sizeof(args) / sizeof(args[0]);

	Error err = Main::setup(argc > 0 ? argv[0] : "godot", nargs, args);
	psp_log("[PSP] setup err=%d mem=%llu", (int)err, (unsigned long long)Memory::get_mem_usage());
	if (err != OK) {
		psp_log("[PSP] FAIL setup");
		sceKernelExitGame();
		return 1;
	}
	if (Main::start() == EXIT_SUCCESS) {
		psp_log("[PSP] start OK mem=%llu", (unsigned long long)Memory::get_mem_usage());
		os.run();
	} else {
		psp_log("[PSP] FAIL start");
	}
	Main::cleanup();
	psp_log("[PSP] cleanup done");
	sceKernelExitGame();
	return 0;
}
```

- [ ] **Step 7: Porting notları dosyası**

`docs/psp/porting-notes.md`:
```markdown
# PSP Port — Değişiklik Notları

Her satır: `dosya:satır` — değişiklik — neden.

| Dosya | Değişiklik | Neden |
|---|---|---|
| platform_methods.py | `mips32` mimarisi | PSP Allegrex (MIPS32, little endian) |
| core/os/spin_lock.h | MIPS `_cpu_pause` → `nop` | Allegrex'te pause/yield yok |
```

- [ ] **Step 8: Derle ve hataları döngüyle düzelt**

Run: `source tools/psp/env.sh && scons platform=psp -j12 2>&1 | tee bin/psp_build.log | grep -E "error|Error" | head -30`

Döngü kuralları (derleme temiz bitene kadar tekrarla):
1. İlk hatayı oku, dosya bağlamını Read ile incele.
2. Sınıflandır ve aşağıdaki düzeltme yollarından birini uygula:
   - **newlib/pspdev'de olmayan POSIX çağrısı** (`fork`, `execvp`, `waitpid`, `dlopen`, `getpwuid`, `setenv` vb., genelde `drivers/unix/os_unix.cpp` içinde): çağrıyı `#ifdef PSP_ENABLED ... return ERR_UNAVAILABLE; #else ... #endif` ile koru.
   - **Eksik başlık** (`<sys/wait.h>`, `<dlfcn.h>`, `<netdb.h>` vb.): include'u `#ifndef PSP_ENABLED` içine al.
   - **Ağ sürücüsü** (`drivers/unix/ip_unix.cpp`, `net_socket_unix.cpp`): Godot'un hazır `UNIX_SOCKET_UNAVAILABLE` tanımı `detect.py`'de açık; `OS_Unix::initialize_core()` bu durumda `NetSocketUnix`/`IPUnix` kaydını atlar. Bu dosyalar yine de derlenmeye çalışılıp hata verirse, dosya başındaki koşula `&& !defined(UNIX_SOCKET_UNAVAILABLE)` ekle.
   - **`FileAccessUnixPipe`** (`initialize_core()` içinde kayıtlı; `mkfifo` gerektirir): derlenmezse `file_access_unix_pipe.cpp` gövdesini ve `initialize_core()`'daki kaydını `#ifndef PSP_ENABLED` ile koru.
   - **Mimari koşulu** (`#error Unsupported architecture` vb.): `__mips__` dalı ekle.
   - **İstisna/RTTI** (`typeid`, `dynamic_cast`): `NO_SAFE_CAST` yolunu kullan; olmazsa `-frtti` açık bırakılır ve not düşülür.
3. Her düzeltmeyi `docs/psp/porting-notes.md` tablosuna bir satır olarak ekle.
4. Bağlama (link) aşamasında tanımsız sembol: sembolün hangi pspsdk kütüphanesinde olduğunu `psp-nm -A $PSPDEV/psp/sdk/lib/*.a $PSPDEV/psp/lib/*.a 2>/dev/null | grep ' T <sembol>'` ile bul, `detect.py` `LIBS` listesine ekle.

Expected (döngü sonu): `bin/godot.psp.template_release.mips32.elf` oluşur.
Run: `ls -la bin/godot.psp.template_release.mips32.elf && psp-size bin/godot.psp.template_release.mips32.elf`
Expected: `text+data+bss` toplamı yazdırılır. **Toplam > 30 MB ise dur, kullanıcıya bildir** (Global Constraints). 20–30 MB arasıysa devam et ama notu Task 3 raporuna yaz.

- [ ] **Step 9: Commit**

```bash
git add platform_methods.py core platform/psp drivers docs/psp
git commit -m "psp: mips32 arch, platform/psp skeleton, engine builds for PSP

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 3: Spike — Godot'u PPSSPP'de açmak ve ölçmek (KARAR NOKTASI)

**Files:**
- Create: `tests/psp/projects/boot_empty/project.godot`
- Create: `tests/psp/projects/boot_empty/main.tscn`
- Create: `tools/psp/stage_game.sh`
- Create: `docs/psp/spike-report.md`
- Modify (gerekirse): Task 2'deki düzeltme döngüsüyle aynı kurallarla çalışma zamanı hataları.

**Interfaces:**
- Consumes: `bin/godot.psp.template_release.mips32.elf`, `make_eboot.sh`, `run_test.sh`, `OS_PSP::run()` log satırları (`[PSP] frame N mem=.. peak=..`, `[PSP] exit clean frames=N`).
- Produces: `tools/psp/stage_game.sh <project_dir> <out_dir> [extra args]` → `<out_dir>/EBOOT.PBP` + projenin kopyası aynı klasörde. `docs/psp/spike-report.md`.

- [ ] **Step 1: Boş test projesi**

`tests/psp/projects/boot_empty/project.godot`:
```ini
config_version=5

[application]

config/name="PSP Boot Empty"
run/main_scene="res://main.tscn"

[rendering]

renderer/rendering_method="dummy"
driver/threads/thread_model=0
```

`tests/psp/projects/boot_empty/main.tscn`:
```
[gd_scene format=3]

[node name="Main" type="Node3D"]

[node name="Child" type="Node3D" parent="."]
```

- [ ] **Step 2: Oyun klasörü hazırlama betiği**

`tools/psp/stage_game.sh`:
```bash
#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/env.sh"
PROJ="$(cd "$1" && pwd)"; OUT="$2"
ELF="$GODOT_PSP_ROOT/bin/godot.psp.template_release.mips32.elf"
rm -rf "$OUT"; mkdir -p "$OUT"
"$GODOT_PSP_ROOT/tools/psp/make_eboot.sh" "$ELF" "$OUT" "Godot PSP"
cp -r "$PROJ"/. "$OUT"/
```

- [ ] **Step 3: Başarısız testi çalıştır (ya da ilk çalıştırma)**

Run: `chmod +x tools/psp/stage_game.sh && tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/boot_empty && tools/psp/run_test.sh bin/psp_tests/boot_empty 60 '\[PSP\] main enter' '\[PSP\] setup err=0' '\[PSP\] start OK' '\[PSP\] frame 120'`
Expected: büyük olasılıkla FAIL. `bin/psp_tests/boot_empty/test.log`'a bak.

Not: `OS_PSP::run()` bu task'ta çıkmaz; test `--timeout=60` ile kesilir. Task 4 temiz çıkışı ekler.

- [ ] **Step 4: Çalışma zamanı hatalarını döngüyle düzelt**

Döngü (test geçene kadar):
1. `test.log`'daki son `[PSP]` satırını ve PPSSPP hata satırlarını (`Bad`, `Invalid`, `crash`, `exception`) bul.
2. Çökme adresi varsa: `psp-addr2line -f -C -e bin/godot.psp.template_release.mips32.elf <adres>`.
3. Bilinen olası nedenler ve çözümleri:
   - `game_dir` beklenenden farklı (ör. `umd0:` ya da boş): log'daki `[PSP] game_dir=` değerine bak; `psp_game_dir()` içinde PPSSPP'nin döndürdüğü biçimi destekle.
   - `project.godot` bulunamıyor: `FileAccessUnix` yolu `ms0:/...` biçimini `/` ile birleştiriyor mu kontrol et; gerekirse `--path` yerine `--main-pack` değil, `chdir(game_dir)` + `--path .` dene.
   - Thread yaratma: `threads=no` rağmen bir yol thread açıyorsa `[PSP] FAIL` log'u ekle ve çağıran yeri `#ifdef` ile koru.
   - Bellek: `setup mem=` değeri 32 MB'yi aşıyorsa Task 3 raporuna yaz, kod değiştirme (karar noktası).
4. Her düzeltmeyi `docs/psp/porting-notes.md`'ye ekle.

Expected (döngü sonu): `[run_test] PASS`.

- [ ] **Step 5: Spike raporu**

Run: `psp-size bin/godot.psp.template_release.mips32.elf; ls -la bin/psp_tests/boot_empty/EBOOT.PBP; grep -E '\[PSP\] (setup|start|frame 120)' bin/psp_tests/boot_empty/test.log`

`docs/psp/spike-report.md` — gerçek sayılarla doldur:
```markdown
# Spike Raporu — 2026-09-27

| Ölçüm | Değer | Hedef | Durum |
|---|---|---|---|
| ELF text | <psp-size text> | — | — |
| ELF data+bss | <psp-size data+bss> | — | — |
| EBOOT.PBP boyutu | <bytes> | < 20 MB | <OK/AŞIM> |
| setup sonrası mem | <[PSP] setup mem=> | — | — |
| start sonrası mem | <[PSP] start OK mem=> | — | — |
| 120. kare mem / peak | <[PSP] frame 120 mem=.. peak=..> | peak < 32 MB | <OK/AŞIM> |

## En büyük 20 sembol
<psp-nm --size-sort -S -C bin/godot.psp.template_release.mips32.elf | tail -20 çıktısı>

## Gözlemler
<porting-notes'tan öne çıkanlar; beklenmeyen davranışlar>
```

- [ ] **Step 6: Commit**

```bash
git add tests/psp/projects tools/psp/stage_game.sh docs/psp platform/psp core drivers
git commit -m "psp: Godot boots in PPSSPP with dummy renderer (spike)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 7: KARAR NOKTASI — dur ve kullanıcıya rapor et**

Spike raporundaki tabloyu kullanıcıya göster. Hedefler tutuyorsa Task 4'e devam etmek için onay iste; tutmuyorsa (EBOOT > 30 MB veya peak > 32 MB) seçenekleri sun (ör. modül/sınıf budama, `-flto`, sınıf kaydını kısıtlama) ve kullanıcının seçimini bekle. **Onaysız Task 4'e geçme.**

---

### Task 4: `platform/psp` katmanını tamamla — DisplayServerPSP, girdi, vsync, temiz çıkış

**Files:**
- Create: `platform/psp/psp_ctrl_map.h`
- Create: `platform/psp/psp_ctrl_map.cpp`
- Create: `platform/psp/display_server_psp.h`
- Create: `platform/psp/display_server_psp.cpp`
- Create: `platform/psp/psp_selftest.cpp`
- Modify: `platform/psp/SCsub` (yeni dosyalar)
- Modify: `platform/psp/godot_psp.cpp` (DisplayServerPSP kaydı, `--psp-quit-after-frames`, `--psp-selftest`)
- Modify: `platform/psp/os_psp.cpp` (`initialize()` içinde `DisplayServerPSP::register_psp_driver()`)
- Test: `tests/psp/projects/boot_empty` (yeniden kullanılır)

**Interfaces:**
- Consumes: `OS_PSP::quit_requested`, `OS_PSP::quit_after_frames`, `psp_log()`, `stage_game.sh`, `run_test.sh`.
- Produces:
  - `struct PSPJoyEvent { int button; bool pressed; };`
  - `int psp_ctrl_map(uint32_t p_prev, uint32_t p_cur, PSPJoyEvent *r_events, int p_max)` — değişen tuşlar için olay üretir, olay sayısını döndürür. Eşleme: CROSS→`JoyButton::A`, CIRCLE→`B`, SQUARE→`X`, TRIANGLE→`Y`, LTRIGGER→`LEFT_SHOULDER`, RTRIGGER→`RIGHT_SHOULDER`, SELECT→`BACK`, START→`START`, UP/DOWN/LEFT/RIGHT→`DPAD_*`.
  - `float psp_axis_map(uint8_t p_raw)` — 0..255 → -1..1, ±0.15 ölü bölge.
  - `class DisplayServerPSP : public DisplayServerHeadless` — `static void register_psp_driver()`, sürücü adı `"psp"`, `process_events()` sceCtrl okur ve `Input`'a joypad 0 olayları verir, `swap_buffers()` vsync bekler, pencere boyutu 480×272.
  - `int psp_selftest()` — 0 = başarılı; `[PSP] selftest ...` satırları.

- [ ] **Step 1: Başarısız self-test**

`platform/psp/psp_selftest.cpp`:
```cpp
#include "psp_ctrl_map.h"
#include "psp_log.h"

#include <pspctrl.h>

static int fails = 0;

#define PSP_CHECK(m_cond, m_msg)                   \
	if (!(m_cond)) {                               \
		psp_log("[PSP] FAIL selftest %s", m_msg); \
		fails++;                                   \
	}

int psp_selftest() {
	PSPJoyEvent ev[16];

	int n = psp_ctrl_map(0, PSP_CTRL_CROSS, ev, 16);
	PSP_CHECK(n == 1 && ev[0].button == (int)JoyButton::A && ev[0].pressed, "cross press");

	n = psp_ctrl_map(PSP_CTRL_CROSS, 0, ev, 16);
	PSP_CHECK(n == 1 && ev[0].button == (int)JoyButton::A && !ev[0].pressed, "cross release");

	n = psp_ctrl_map(0, PSP_CTRL_UP | PSP_CTRL_RTRIGGER | PSP_CTRL_START, ev, 16);
	PSP_CHECK(n == 3, "three buttons at once");

	n = psp_ctrl_map(PSP_CTRL_UP, PSP_CTRL_UP, ev, 16);
	PSP_CHECK(n == 0, "held button no event");

	n = psp_ctrl_map(0, 0xFFFFFFFF, ev, 2);
	PSP_CHECK(n == 2, "event buffer limit");

	PSP_CHECK(psp_axis_map(128) == 0.0f, "axis center dead zone");
	PSP_CHECK(psp_axis_map(0) <= -0.99f, "axis min");
	PSP_CHECK(psp_axis_map(255) >= 0.99f, "axis max");
	PSP_CHECK(psp_axis_map(140) == 0.0f, "axis small offset dead zone");

	psp_log("[PSP] selftest done fails=%d", fails);
	return fails;
}
```

`platform/psp/psp_ctrl_map.h` (yalnızca bildirimler):
```cpp
#pragma once

#include "core/input/input_enums.h"

#include <cstdint>

struct PSPJoyEvent {
	int button;
	bool pressed;
};

int psp_ctrl_map(uint32_t p_prev, uint32_t p_cur, PSPJoyEvent *r_events, int p_max);
float psp_axis_map(uint8_t p_raw);
```

`godot_psp.cpp` içinde, `Main::setup` çağrısından önce:
```cpp
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--psp-selftest") == 0) {
			int fails = psp_selftest();
			sceKernelExitGame();
			return fails;
		}
	}
```
ve dosya başına `int psp_selftest();` bildirimi.

Self-test EBOOT'u argüman alamadığı için test modu, oyun klasöründe `psp_selftest` adlı boş bir dosya varsa da açılır: `main()` içinde `game_dir + "/psp_selftest"` için `sceIoGetstat` başarılıysa aynı yolu çalıştır.

`SCsub`'a `psp_selftest.cpp` ve `psp_ctrl_map.cpp` ekle; `psp_ctrl_map.cpp`'yi yalnızca şu gövdeyle oluştur (henüz uygulama yok):
```cpp
#include "psp_ctrl_map.h"

int psp_ctrl_map(uint32_t, uint32_t, PSPJoyEvent *, int) {
	return 0;
}

float psp_axis_map(uint8_t) {
	return 0.0f;
}
```

Run: `scons platform=psp -j12 && tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/selftest && touch bin/psp_tests/selftest/psp_selftest && tools/psp/run_test.sh bin/psp_tests/selftest 20 '\[PSP\] selftest done fails=0'`
Expected: FAIL — `[PSP] FAIL selftest cross press` vb.

- [ ] **Step 2: Eşlemeyi uygula**

`platform/psp/psp_ctrl_map.cpp`:
```cpp
#include "psp_ctrl_map.h"

#include <pspctrl.h>

struct PSPButtonMap {
	uint32_t psp;
	JoyButton godot;
};

static const PSPButtonMap BUTTONS[] = {
	{ PSP_CTRL_CROSS, JoyButton::A },
	{ PSP_CTRL_CIRCLE, JoyButton::B },
	{ PSP_CTRL_SQUARE, JoyButton::X },
	{ PSP_CTRL_TRIANGLE, JoyButton::Y },
	{ PSP_CTRL_LTRIGGER, JoyButton::LEFT_SHOULDER },
	{ PSP_CTRL_RTRIGGER, JoyButton::RIGHT_SHOULDER },
	{ PSP_CTRL_SELECT, JoyButton::BACK },
	{ PSP_CTRL_START, JoyButton::START },
	{ PSP_CTRL_UP, JoyButton::DPAD_UP },
	{ PSP_CTRL_DOWN, JoyButton::DPAD_DOWN },
	{ PSP_CTRL_LEFT, JoyButton::DPAD_LEFT },
	{ PSP_CTRL_RIGHT, JoyButton::DPAD_RIGHT },
};

int psp_ctrl_map(uint32_t p_prev, uint32_t p_cur, PSPJoyEvent *r_events, int p_max) {
	uint32_t changed = p_prev ^ p_cur;
	int n = 0;
	for (const PSPButtonMap &b : BUTTONS) {
		if (n >= p_max) {
			break;
		}
		if (changed & b.psp) {
			r_events[n].button = (int)b.godot;
			r_events[n].pressed = (p_cur & b.psp) != 0;
			n++;
		}
	}
	return n;
}

float psp_axis_map(uint8_t p_raw) {
	float v = (float(p_raw) - 127.5f) / 127.5f;
	if (v > 1.0f) {
		v = 1.0f;
	} else if (v < -1.0f) {
		v = -1.0f;
	}
	if (v > -0.15f && v < 0.15f) {
		return 0.0f;
	}
	return v;
}
```

Run: aynı komut (Step 1).
Expected: `[PSP] selftest done fails=0`, `[run_test] PASS`.

- [ ] **Step 3: Commit (eşleme)**

```bash
git add platform/psp
git commit -m "psp: controller mapping with in-EBOOT selftest

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 4: Temiz çıkış için başarısız test**

Run: `tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/quit && echo 180 > bin/psp_tests/quit/psp_quit_after_frames && tools/psp/run_test.sh bin/psp_tests/quit 60 '\[PSP\] display_driver=psp' '\[PSP\] exit clean frames=180' '\[PSP\] cleanup done'`
Expected: FAIL — `display_driver=psp` ve `exit clean` yok.

- [ ] **Step 5: `DisplayServerPSP`**

`platform/psp/display_server_psp.h`:
```cpp
#pragma once

#include "servers/display/display_server_headless.h"

class DisplayServerPSP : public DisplayServerHeadless {
	GDSOFTCLASS(DisplayServerPSP, DisplayServerHeadless);

	uint32_t prev_buttons = 0;
	float prev_axis[2] = { 0.0f, 0.0f };

	static DisplayServer *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

public:
	static void register_psp_driver();

	String get_name() const override { return "psp"; }
	void process_events() override;
	void swap_buffers() override;
	Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return Size2i(480, 272); }
	Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Size2i(480, 272); }

	DisplayServerPSP();
};
```

`platform/psp/display_server_psp.cpp`:
```cpp
#include "display_server_psp.h"

#include "psp_ctrl_map.h"
#include "psp_log.h"

#include "core/input/input.h"

#include <pspctrl.h>
#include <pspdisplay.h>

DisplayServerPSP::DisplayServerPSP() {
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	psp_log("[PSP] display_driver=psp");
}

Vector<String> DisplayServerPSP::get_rendering_drivers_func() {
	Vector<String> drivers;
	drivers.push_back("dummy");
	return drivers;
}

DisplayServer *DisplayServerPSP::create_func(const String &, DisplayServerEnums::WindowMode, DisplayServerEnums::VSyncMode, uint32_t, const Vector2i *, const Vector2i &, int, DisplayServerEnums::Context, int64_t, Error &r_error) {
	r_error = OK;
	RasterizerDummy::make_current();
	return memnew(DisplayServerPSP());
}

void DisplayServerPSP::register_psp_driver() {
	register_create_function("psp", create_func, get_rendering_drivers_func);
}

void DisplayServerPSP::process_events() {
	SceCtrlData pad;
	if (sceCtrlPeekBufferPositive(&pad, 1) <= 0) {
		return;
	}
	Input *input = Input::get_singleton();
	PSPJoyEvent events[12];
	int n = psp_ctrl_map(prev_buttons, pad.Buttons, events, 12);
	for (int i = 0; i < n; i++) {
		input->joy_button(0, (JoyButton)events[i].button, events[i].pressed);
	}
	prev_buttons = pad.Buttons;

	float ax = psp_axis_map(pad.Lx);
	float ay = psp_axis_map(pad.Ly);
	if (ax != prev_axis[0]) {
		input->joy_axis(0, JoyAxis::LEFT_X, ax);
		prev_axis[0] = ax;
	}
	if (ay != prev_axis[1]) {
		input->joy_axis(0, JoyAxis::LEFT_Y, ay);
		prev_axis[1] = ay;
	}
	input->flush_buffered_events();
}

void DisplayServerPSP::swap_buffers() {
	sceDisplayWaitVblankStart();
}
```

Derleme notları:
- `RasterizerDummy::make_current()` için `#include "servers/rendering/dummy/rasterizer_dummy.h"` ekle.
- `DisplayServerHeadless` metotlarından biri `override` edilemiyorsa (virtual değil ya da imza farklı) `servers/display/display_server_headless.h`'deki gerçek imzayı Read ile kontrol et ve birebir kopyala.
- `Input::joy_button` / `joy_axis` imzası 4.7'de farklıysa `core/input/input.h`'den kontrol et. Joypad 0'ın bağlı sayılması için `DisplayServerPSP` kurucusunun sonunda (Input hazırsa) `Input::get_singleton()->joy_connection_changed(0, true, "PSP")` çağır; Input o anda `nullptr` ise bu çağrıyı ilk `process_events()`'e taşı (bir kez).

`os_psp.cpp` `initialize()` sonuna:
```cpp
	DisplayServerPSP::register_psp_driver();
```
(ve `#include "display_server_psp.h"`).

`godot_psp.cpp` değişiklikleri:
- `args` içinde `"headless"` → `"psp"`.
- `game_dir + "/psp_quit_after_frames"` dosyası varsa içindeki sayıyı oku (`sceIoOpen/sceIoRead`, en fazla 15 byte, `atoi`) ve `os.quit_after_frames`'e ata; `[PSP] quit_after_frames=N` logla.

`SCsub`'a `display_server_psp.cpp` ekle.

- [ ] **Step 6: Testleri geçir**

Run:
```bash
scons platform=psp -j12 && \
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/quit && echo 180 > bin/psp_tests/quit/psp_quit_after_frames && \
tools/psp/run_test.sh bin/psp_tests/quit 60 '\[PSP\] display_driver=psp' '\[PSP\] exit clean frames=180' '\[PSP\] cleanup done' && \
tools/psp/stage_game.sh tests/psp/projects/boot_empty bin/psp_tests/selftest && touch bin/psp_tests/selftest/psp_selftest && \
tools/psp/run_test.sh bin/psp_tests/selftest 20 '\[PSP\] selftest done fails=0'
```
Expected: iki `[run_test] PASS`.

Peak bellek kontrolü:
Run: `grep -oE 'peak=[0-9]+' bin/psp_tests/quit/test.log | tail -1`
Expected: değer ≤ 46137344 (44 MB). Aşıyorsa `[PSP] FAIL` sayılır; dur ve kullanıcıya bildir.

- [ ] **Step 7: Emülatörde vsync doğrulaması**

`[PSP] frame 60` ve `[PSP] frame 120` satırlarının PPSSPP zaman damgaları arasındaki fark yaklaşık 1 saniye olmalı (headless log satırları zaman damgası taşımıyorsa bu adımda `OS_PSP::run()` içindeki 60 karelik log satırına `t=%llu` ekle: `OS::get_singleton()->get_ticks_msec()`).
Run: `grep -E '\[PSP\] frame (60|120) ' bin/psp_tests/quit/test.log`
Expected: t farkı 900–1100 ms arası.

- [ ] **Step 8: Commit**

```bash
git add platform/psp docs/psp
git commit -m "psp: DisplayServerPSP with vsync, joypad input and clean exit

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 9: Faz 1 sonu raporu**

Kullanıcıya: spike tablosu, peak bellek, `docs/psp/porting-notes.md` özeti. Faz 2 (renderer) planının yazılması için onay iste.
