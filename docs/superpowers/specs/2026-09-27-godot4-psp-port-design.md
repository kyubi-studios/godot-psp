# Godot 4.7.2 → PSP (PSP-2001) Portu — Tasarım

Tarih: 2026-09-27
Durum: Onay bekliyor

## 1. Amaç ve kapsam

**Hedef (kullanıcı):** Öğrenme/deneme projesi. Godot 4.7.2 kaynağı PSP-2001 (Slim, 64 MB RAM)
üzerinde, önce PPSSPP emülatöründe çalışacak hale getirilecek; 3B bir demo sahnesi gösterilecek.
Godot'un her yeri değiştirilebilir. Bellek kullanımında son derece cimri olunacak.

**Kararlar:**
- Demo sahnesi `.tscn` (Godot 4.7 editöründe hazırlanır), oyun mantığı **C++** (motora gömülü
  küçük node'lar). GDScript modülü derlenmez.
- Otomatik test: kaynaktan derlenen **headless PPSSPP** (SDL2 `~/ppsspp-deps` içine kaynaktan derlenir,
  sisteme kurulum yapılmaz).
- Render katmanı: Godot 4'ün `RenderingDevice` arayüzü shader tabanlı API'ler (Vulkan/D3D12/Metal)
  içindir; PSP GE sabit işlevlidir, shader yoktur. Bu yüzden "PSP rendering katmanı",
  GLES3 sürücüsünün durduğu seviyede, **`RendererCompositor`** olarak yazılır (`drivers/psp_gu`).

**Kapsam dışı (bu spec):** GDScript, fizik, ses, navigasyon, XR, 2D canvas çizimi (FPS/RAM
yazısı hariç), gölgeler, custom ShaderMaterial, editör export eklentisi, gerçek donanım testi
(EBOOT hazırlanır, donanım testi kullanıcıda).

**Varsayımlar:** PSP-2001 = Slim; `MEMSIZE=1` ile ~52 MB kullanıcı belleği.

## 2. Mimari ve platform katmanı

```
godot4-psp/
  platform/psp/
    detect.py              SCons: psp-g++ araç zinciri, -Os, LTO, -fno-exceptions -fno-rtti, budama bayrakları
    godot_psp.cpp          main(): PSP_MODULE_INFO, PSP_HEAP_SIZE_KB(-1024), exit callback,
                           Main::setup → Main::start → Main::iteration döngüsü
    os_psp.{h,cpp}         OS_Unix tabanlı: zaman (sceRtc / sceKernelGetSystemTimeWide), bellek sorgusu, stdout log
    display_server_psp.*   tek pencere 480×272, vsync, sceCtrl → Input (d-pad, analog, tuşlar)
    dosya erişimi          Unix FileAccess/DirAccess; res:// → EBOOT klasöründeki game.pck
    demo/                  PSPDemoSpinner, PSPOrbitCamera C++ node'ları
  drivers/psp_gu/          PSP renderer (bölüm 3)
  tools/psp/
    run_test.sh            derle → EBOOT paketle → headless PPSSPP → log + ekran görüntüsü kontrolü
    export_pck.sh          Godot 4.7 editörü --headless --export-pack → game.pck
  tests/psp/               EBOOT içi birim kontrolleri
```

**Açılış akışı:** `main()` → `Main::setup()` (proje verisi `game.pck`) → `Main::start()` (ana `.tscn`)
→ her kare `Main::iteration()`; HOME tuşu → temiz çıkış.

**SCons:** `platform=psp target=template_release`, `modules_enabled_by_default=no`,
`disable_physics_3d=yes`, `disable_navigation_3d=yes`, `disable_xr=yes`, `deprecated=no`,
`minizip=no`, dummy ses sürücüsü. Godot 4.7'de bulunmayan bayraklar kendi `#ifdef`'lerimizle
kapatılır.

**Thread'ler:** PSP pthread (pthread-glue) kullanılır. RenderingServer tek thread
(`thread_model=single-safe`), WorkerThreadPool en fazla 1 thread, thread yığınları 64 KB.

## 3. PSP GU renderer (`drivers/psp_gu/`)

`RasterizerPSP : RendererCompositor`, dummy renderer iskeletinden türetilir. Yalnızca aşağıdakiler
gerçek olarak uygulanır; decal, GI, fog volume, particles, reflection probe için veri tutulmaz (stub).

| Birim | Görevi | PSP karşılığı |
|---|---|---|
| `psp_vram` | 2 MB VRAM yönetimi (bump + serbest liste) | Sabit: 2× framebuffer 16-bit 565 (2×261 KB) + 16-bit z-buffer (261 KB); kalan ~1.2 MB LRU texture önbelleği |
| `psp_gu_context` | Display list, çift tamponlama | 2×64 KB statik, 16 byte hizalı liste |
| `TextureStoragePSP` | `Image` → PSP texture | 2'nin kuvvetine yuvarlama, en fazla 256×256; 4444/5551/565 veya 8-bit CLUT; swizzle. Orijinal `Image` saklanmaz |
| `MeshStoragePSP` | Surface → GU vertex formatı | 16-bit pozisyon/normal/UV (gerekirse renk), tek interleaved buffer, 16-bit indeks. Godot dizileri dönüşüm sonrası serbest |
| `MaterialStoragePSP` | Material → GU state | ShaderLanguage derleyicisi **kullanılmaz**. `BaseMaterial3D` parametreleri (albedo, albedo texture, transparency, cull, unshaded) okunur; `render_mode` satırından basit metin aramasıyla bayraklar. Custom ShaderMaterial → beyaz albedo + tek seferlik uyarı |
| `LightStoragePSP` | Directional/omni/spot | GU donanım ışıkları (en fazla 4); nesne başına en etkili 4 ışık; gölge yok |
| `RasterizerScenePSP::render_scene` | Çizim | Kamera matrisleri → GU; opaklar material'e göre sıralı, saydamlar arkadan öne. Culling `RendererSceneCull`'dan. Environment'tan arka plan rengi, ambient, GU fog |
| `RasterizerCanvasPSP` | 2D | İlk aşama: temizleme, boot splash; FPS/RAM yazısı PSP debug fontuyla |

**Veri akışı:** Mesh/texture PSP formatına **yüklemede bir kez** çevrilir; her karede yalnızca
matrisler, material state'i ve draw call'lar display list'e yazılır. Render main thread'de çalışır.

**Hatalar:** VRAM dolarsa texture ana RAM'den çizilir (GE destekler, yavaş). Desteklenmeyen
özellikler için `WARN_PRINT_ONCE`, çökme yok.

## 4. Bellek bütçesi

| Kalem | Hedef |
|---|---|
| EBOOT kodu + statik veri | ≤ 18 MB |
| Godot çekirdeği çalışma zamanı | ≤ 8 MB |
| Demo sahnesi (mesh + texture + node) | ≤ 4 MB |
| Display list + thread yığınları | ≤ 1 MB |
| Güvenlik payı | ≥ 10 MB |

`PSP_HEAP_SIZE_KB(-1024)`; tüm tahsisler Godot `Memory` sayaçlarından geçer; her 60 karede
`static / peak / free` loglanır. Peak > 44 MB → test başarısız.

## 5. Export zinciri ve demo

- Demo projesi: `godot_projects/psp_demo3d/` (Godot 4.7 editörüyle açılabilir).
- `tools/psp/export_pck.sh` → `game.pck`. Texture import "Lossless"; PSP tarafı PNG/WebP decoder
  taşımasın diye gerekirse ham format — spike'ta netleşir.
- `EBOOT.PBP` + `game.pck` aynı klasörde (`ms0:/PSP/GAME/GodotDemo/`).
- Sahne: damalı texture'lı PlaneMesh zemin, farklı material'li dönen BoxMesh'ler, SphereMesh,
  bir saydam nesne, 1 DirectionalLight3D, 1 OmniLight3D, fog, analog/d-pad yörünge kamerası,
  ekranda FPS + RAM.
- C++ mantık: `PSPDemoSpinner`, `PSPOrbitCamera`. Editör bu sınıfları bilmediği için `.tscn`'de
  Node3D üzerindeki metadata (`psp_behavior = "spinner"` vb.) ile işaretlenir; PSP tarafı açılışta
  sahneyi tarayıp davranışı bağlar. Editörü yeniden derlemek gerekmez.

## 6. Test ve başarı ölçütleri

- `run_test.sh`: headless PPSSPP ile EBOOT'u zaman aşımıyla çalıştırır; stdout'taki checkpoint
  satırlarını (`[PSP] setup OK`, `[PSP] frame 60 fps=.. peak=..`) ve ekran görüntüsünü kontrol eder.
- Görsel doğrulama: ekran görüntüsü, referans görüntüyle kaba karşılaştırma.

**Başarı ölçütleri:**
1. Headless PPSSPP'de EBOOT açılır, sahne görünür.
2. Emülatörde ≥ 30 FPS.
3. Peak RAM < 44 MB.
4. HOME ile temiz çıkış.

## 7. Uygulama sırası ve karar noktası

1. Headless PPSSPP (SDL2 dahil) + `run_test.sh`.
2. **Spike:** budanmış çekirdek + dummy renderer → ikili boyut ve boşta RAM ölçümü.
   Hedef: ikili < 20 MB, boşta RAM < 32 MB. **İkili > 30 MB ise durup kullanıcıyla yeniden karar.**
3. `platform/psp` tamamlanır (girdi, dosya, zaman).
4. Renderer: VRAM → mesh → texture → material → ışık.
5. Demo + export zinciri.
