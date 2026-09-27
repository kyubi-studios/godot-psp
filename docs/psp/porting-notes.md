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
| platform/psp/detect.py | `disable_advanced_gui=True` | EBOOT −2.0 MB, heap −0.9 MB; demo gelişmiş GUI kullanmıyor |
| servers/rendering/SCsub (+ detect.py `disable_renderer_rd`, `RENDERER_RD_DISABLED`) | `renderer_rd/` derlenmez; yalnızca `spirv-reflect` | Vulkan/RD renderer'ı PSP'de kullanılmaz (bağlama sonrası kazanç 86 KB) |
| servers/register_server_types.cpp | `*RD` sınıf kayıtları `#ifndef RENDERER_RD_DISABLED` | `renderer_rd/` derlenmeyince tanımsız |
| platform/psp/display_server_psp.* | DisplayServerHeadless tabanlı; sceCtrl → joypad 0, 480×272, vblank | Girdi + vsync |
| platform/psp/os_psp.cpp (run) | Her iterasyonda `swap_buffers()` (vblank) | Dummy renderer swap etmez; Faz 2'de renderer'a taşınacak |
| platform/psp/psp_exit.* | Exit callback → global bayrak → DisplayServerPSP `WINDOW_EVENT_CLOSE_REQUEST`; 3 sn watchdog → `sceKernelExitGame` | HOME → Çık açılışta ya da takılmada kaybolmasın; oyun WM_CLOSE_REQUEST alsın (inceleme I1/I2) |
| platform/psp/psp_wraps.cpp (+ `-Wl,--wrap=malloc,realloc,calloc,getcwd`) | NULL tahsis → `[PSP] FAIL OOM`; getcwd cihaz kökü `umd0:/` | OOM görünür olsun; DirAccessUnix önceki dizine `chdir` edebilsin (inceleme I3/I4) |
| platform/psp/godot_psp.cpp | `mallopt(M_TRIM_THRESHOLD, max)` | Arena küçülmesin; `mallinfo().arena` gerçek peak olsun (inceleme I3) |
| platform/psp/psp_paths.cpp | `psp_game_dir` boş/NULL/eğik çizgisiz argv[0] → "." | Sınır dışı okuma (inceleme I5) |
| tools/psp/run_test.sh (+ known_errors.txt) | Godot `ERROR:` satırları testi düşürür; `RUN_TEST_EXPECT_EXIT=1` zaman aşımını hata sayar | Sessiz hatalar ve çıkışta takılma yakalanır (inceleme I1/I4) |
| drivers/psp_gu/* (+ drivers/SCsub) | PSP GE renderer: `RasterizerPSP : RasterizerDummy`, GU bağlamı (5650 fb×2, 16-bit z, 2×64 KB liste) | Faz 2 |
| drivers/psp_gu/storage/texture_storage_psp.* | Gerçek render target (boyut + temizleme isteği) | Dummy boş RID döndürür → viewport çizimi atlanır |
| drivers/psp_gu/rasterizer_scene_psp.* | `RenderSceneBuffersPSP`; render_scene temizleme isteğini tüketir | RendererSceneCull null buffer'da sessizce döner; viewport 3D'den sonra temizler |
| platform/psp/display_server_psp.h | `can_any_window_draw`/`window_can_draw`=true, tam ekran, 60 Hz | Headless tabanı "çizilemez" bildirir → Main hiç çizmez |
| platform/psp/os_psp.cpp | `swap_buffers()` çağrısı kaldırıldı; `psp_screenshot_at_frame` test kancası | Vblank + swap artık `RasterizerPSP::end_frame`'de |
| drivers/psp_gu/storage/mesh_storage_psp.* | GE vertex formatı (16-bit poz. AABB ölçekli, 8-bit normal, 16-bit UV, ops. 8888 renk, 16-bit indeks); Godot dizileri saklanmaz; `mesh_get_surface` yalnızca meta veri | Bellek; `surface_get_arrays` PSP'de boş döner |
| drivers/psp_gu/storage/material_storage_psp.* | ShaderLanguage derleyicisi yok; BaseMaterial3D kodundan bayrak çıkarımı; whitelist parametreler | CPU/RAM |
| drivers/psp_gu/storage/utilities_psp.* | Base type/free yönlendirme | PSP depoları |
| drivers/psp_gu/storage/light_storage_psp.* | Directional/omni/spot parametreleri, AABB'ler, ışık instance dönüşümleri | GE ışıkları + culling/eşleştirme |
| drivers/psp_gu/rasterizer_scene_psp.cpp | GE aydınlatma (≤4 ışık/nesne), ambient, doğrusal fog (üstel → yaklaşım), arka yüz kırpma (`GU_CW` = Godot ön yüzü, test ile doğrulandı), saydam sıralama | Faz 2 Task 3 |
| drivers/psp_gu/psp_texture.* | Image → RGBA8 → en yakın 2^n (8..256) → 5650/5551/4444 → swizzle (yükseklik ≥ 8); orijinal Image saklanmaz | Bellek; `texture_2d_get` boş döner |
| drivers/psp_gu/storage/texture_storage_psp.* | Texture API (tüm başlatıcılar RID'i başlatır), `texture_replace` veri taşır | Dummy başlatmadığı RID'lerde `texture_free` hata veriyordu |
| platform/psp/godot_psp.cpp | EBOOT yanında `game.pck` varsa `--main-pack` | Export edilmiş oyunlar |
| platform/psp/psp_behaviors.* | `psp_behavior` metadata: spinner, orbit_camera (C++) | GDScript yok |
| platform/psp/os_psp.cpp + drivers/psp_gu/psp_gu.cpp | `psp/show_stats` → debug font ile FPS/RAM/draw yazısı (InitEx bir kez, sonra SetBase) | InitEx her çağrıda buffer'ı temizler |
| tools/psp/export_pck.sh, stage_game.sh (.pck), check_fps.sh | Demo export ve test zinciri | Faz 2 Task 5 |
| drivers/psp_gu/psp_gu.* | `ensure_list_space`: liste dolmadan GE bitirilip aynı buffer'da yeniden başlatılır | 64 KB liste ~180 ışıklı çizimde taşıyordu (inceleme C1) |
| drivers/psp_gu/rasterizer_scene_psp.* | Omni/spot yuvaları `placement_idx` ile; çizimde katkıya göre en iyi 4; ışıklar instance başına bir kez | RendererSceneCull yer değiştirmeleri yok sayılıyordu (C2) |
| drivers/psp_gu/storage/mesh_storage_psp.cpp | Normal `normalize(n / pos_half)` olarak depolanır; `draw_chunk_size` ile >65535 çizim parçalanır | Küp olmayan AABB'de normal bozuluyordu (I1); prim sayı alanı 16 bit (I3) |
| drivers/psp_gu/rasterizer_scene_psp.cpp | Unshaded + vertex rengi (materyal kullanmıyorsa): ışıklı/ışıksız/beyaz ambient ile albedo | GE ışıksız modda vertex rengini kullanır (I2) |
| drivers/psp_gu/psp_texture.cpp | Mipmap'li görüntüde hedefe yeten en küçük seviye ayrılır, sonra açılır | Büyük texture'da tam boyut açılması (I4) |
| drivers/psp_gu/rasterizer_scene_psp.cpp | Normali olmayan ışıklı yüzey → ışıksız (albedo) + tek seferlik uyarı | GE'de tanımsız ışıklanma (inceleme M3) |
| platform/psp/os_psp.cpp, psp_behaviors.cpp, mesh_storage_psp.cpp | `GLOBAL_DEF("psp/show_stats")`; kamera olmayan orbit_camera uyarı ile atlanır; s16 ×32768 | İnceleme M1/M5/M8 |
| tests/psp/projects/{tex_swizzle,lit_rotated_cam,many_lights,shader_material,position_only} | Swizzle'lı texture + UV offset, döndürülmüş kamera ışığı, >4 ışık, ShaderMaterial, pozisyon-only mesh | İnceleme test boşlukları |
| drivers/psp_gu/rasterizer_canvas_psp.* | 2D canvas: RECT/NINEPATCH/POLYGON/PRIMITIVE (through modu, vertex'ler display list'ten), MESH (ortografik 3D yolu), kırpma, filtre/repeat; temizleme isteği 2D öncesi | Label/Sprite/UI görünmüyordu |
| drivers/psp_gu/storage/mesh_storage_psp.cpp | 2D vertex dizileri (z=0) kabul edilir | Polygon2D 4.7'de 2D mesh kullanır |
| platform/psp/detect.py | `module_text_server_fb_enabled`, `module_freetype_enabled`, `brotli=True` | Label yazıları; gömülü font WOFF2 (Brotli). ELF +1.4 MB |
| drivers/psp_gu/psp_gu.cpp (texture_address) | Texture VRAM önbelleği (~1.2 MB, LRU, first-fit, bu karede kullanılanlar atılmaz; mutlak 0x04000000 adresi) | GE VRAM'den çok daha hızlı okur |
| platform/psp/detect.py | `module_gdscript_enabled=True` | GDScript (+0.9 MB kod); demo `bob.gd` ile export yolu test edilir |
| drivers/psp_gu/rasterizer_canvas_psp.cpp | `final_transform`/`final_clip_rect` doğrudan (canvas dönüşümü zaten dahil); `sceGuScissor(x,y,w,h)`; CLIP_IGNORE geri yükleme; büyük poligonlar parçalı (şerit sürekliliği korunur); vertex renkli 2D mesh modulate'i ambient hilesiyle; filtre/wrap yalnızca değişince | 3. inceleme C1/C2/I1/I2/I3/I5 |
| drivers/psp_gu/psp_gu.cpp | `texture_forget` kare içinde, bu karede kullanılan VRAM alanını kare sonuna kadar tutar | 3. inceleme I4 |
