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
