# PSP Port — Değişiklik Notları

Her satır: `dosya:satır` — değişiklik — neden.

| Dosya | Değişiklik | Neden |
|---|---|---|
| platform_methods.py | `mips32` mimarisi | PSP Allegrex (MIPS32, little endian) |
| core/os/spin_lock.h | MIPS `_cpu_pause` → `nop` | Allegrex'te pause/yield yok |
| platform/psp/detect.py | `#platform/psp` CPPPATH'e (Prepend) | `platform_config.h` her platformun kendi klasöründen bulunur |
| platform/psp/detect.py | `-Umips` | GCC MIPS hedefi `mips=1` makrosu tanımlar; `thirdparty/amd-fsr2` içindeki `mips` alanı bozuluyordu |
| platform/psp/psp_stdint_fix.h (+ detect.py `-include`) | int32_t/uint32_t → int/unsigned int | pspdev newlib'de int32_t = long; Godot'un tip sistemi (GetTypeInfo<int>, Variant) int32_t == int varsayar |
| core/templates/safe_refcount.h:65 | PSP + threads=no iken `is_always_lock_free` static_assert kapalı | MIPS32'de 64-bit atomik kilitsiz değil; tek thread'de libatomic yeterli |
| drivers/unix/file_access_unix.cpp | xattr yolları `PSP_ENABLED` için "desteklenmiyor" (Web ile aynı) | pspdev'de `sys/xattr.h` yok |
| drivers/unix/syslog_logger.cpp | `!defined(PSP_ENABLED)` | pspdev'de `syslog.h` yok; OS_PSP bu logger'ı kullanmaz |
| drivers/unix/file_access_unix_pipe.cpp | `get_length()` PSP'de 0 | `FIONREAD` yok; PSP'de alt süreç/pipe yok |
| platform/psp/posix_shim/{dlfcn,poll}.h | Başarısız dönen yedek başlıklar | pspdev'de yok; OS_Unix'in dlopen/alt süreç yolları PSP'de kullanılmaz |
| platform/psp/platform_config.h | `PTHREAD_NO_RENAME` | `pthread_setname_np` yok |
| drivers/unix/os_unix.cpp (get_memory_info) | `getrlimit` bloğu PSP'de atlanır | pspdev'de `getrlimit` yok |
| platform/psp/psp_posix_stubs.cpp | dup2/waitpid/setsid/vfork/sigaction/mkfifo/ftruncate → ENOSYS | newlib bildiriyor, pspdev libc uygulamıyor; bağlama hatası |
| platform/psp/psp_gc.ld (+ detect.py `-T`) | Varsayılan PSP linker betiği + `KEEP` (sceStub, lib.ent/stub, sceModuleInfo/Resident/Nid/Vstub) | `--gc-sections` NID tablolarını siliyordu; `psp-fixup-imports`: "no nid section found" |
| platform/psp/detect.py (LIBS) | `c`, `m`, `pspuser`, `psprtc`, `pspnet*`, `pspkernel` çıkarıldı | Specs bunları doğru sırayla ekler; açık `-lc` newlib chdir/getcwd'yi libcglue'dan önce bağlıyordu (cwd boş) |
| platform/psp/detect.py | `disable_path_overrides=False` | EBOOT oyun klasörünü `--path` ile verir |
| platform/psp/psp_logger.* | Godot logger → `psp_log_raw` (stdout + devctl) | Headless PPSSPP yalnızca devctl çıktısını gösterir |
| platform/psp/godot_psp.cpp | Cihaz kökü `umd0:` → `umd0:/` | libcglue `chdir("umd0:")` ENOTDIR verir |
| platform/psp/os_psp.h | `get_executable_path()` = argv[0] | OS_Unix'te PSP için uygulama yok |
