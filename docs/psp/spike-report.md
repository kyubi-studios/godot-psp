# Spike Raporu — 2026-09-27

Godot 4.7.2 (`platform=psp target=template_release threads=no`, dummy renderer, `DisplayServerHeadless`)
PPSSPP 1.20.4 headless'te açıldı: `Main::setup` → `Main::start` → ana döngü (boş Node3D sahnesi).

## Ölçümler

| Ölçüm | Değer | Hedef | Durum |
|---|---|---|---|
| ELF text | 21 358 668 B (20.37 MB) | — | — |
| ELF data + bss | 14 736 + 968 592 B (0.94 MB) | — | — |
| ELF toplam (belleğe yüklenen) | 22 341 996 B (21.31 MB) | — | — |
| EBOOT.PBP boyutu | 21 379 644 B (20.39 MB) | < 20 MB | **Hafif aşım (~0.4 MB)**, 30 MB durma eşiğinin altında |
| `setup` sonrası heap (kullanılan / arena) | 11.40 MB / 11.46 MB | — | — |
| `start` sonrası heap | 11.62 MB / 11.66 MB | — | — |
| 120. kare heap / peak (arena) | 11.64 MB / 11.66 MB | peak < 32 MB | OK |
| Boş döngü hızı (vsync yok, emülatör) | 60 kare ≈ 414 ms (~145 FPS) | — | — |

Bellek ölçümü newlib `mallinfo()` ile: kullanılan = `uordblks`, peak = `arena`'nın görülen en yüksek değeri
(Godot'un `Memory` sayaçları release derlemesinde 0 döner).

## Toplam RAM tablosu (PSP-2001, ~52 MB kullanıcı belleği)

| Kalem | Spec bütçesi | Ölçülen |
|---|---|---|
| EBOOT kodu + statik veri | ≤ 18 MB | 21.3 MB |
| Godot çekirdeği çalışma zamanı (heap) | ≤ 8 MB | 11.7 MB |
| **Ara toplam** | ≤ 26 MB | **33.0 MB** |
| Kalan (sahne + renderer + güvenlik payı için) | ≥ 15 MB | **≈ 19 MB** (52 − 33) |

Toplam sığıyor, ancak spec'in alt bütçelerinin ikisi de aşılıyor (kod +3.3 MB, heap +3.7 MB).

## Boyut dağılımı (bağlama öncesi, `--gc-sections` öncesi .o toplamları)

| Bölüm | Boyut |
|---|---|
| scene/ (toplam) | 14.9 MB — gui 4.2, resources 4.7, 3d 2.4, 2d 1.0, animation 0.8 |
| servers/ (toplam) | 9.7 MB — **rendering/renderer_rd 5.2 MB** (PSP'de hiç kullanılmayan Vulkan/RD renderer'ı), audio 0.6, text 0.5 |
| core/ | 7.1 MB |
| thirdparty | 0.8 MB — zstd, libpng, zlib, mbedtls, clipper2 |

Bağlamada 32 MB → 21.3 MB'a iniyor; kalan kodun bir kısmı yine `renderer_rd` ve `scene/gui` gibi demoda
kullanılmayan ama `ClassDB` kaydıyla bağlı tutulan sınıflardır.

## Gözlemler

- `int32_t` pspdev'de `long` — `platform/psp/psp_stdint_fix.h` ile `int`'e yönlendirildi (27 647 derleme hatasının kökü).
- Açık `-lc` bağlama sırası newlib'in `chdir/getcwd/strtol`'unu libcglue'dan önce bağlıyordu; kaldırıldı.
- `--gc-sections` PSP NID tablolarını siliyordu; `platform/psp/psp_gc.ld` ile KEEP.
- PPSSPP headless, EBOOT klasörünü `umd0:/` olarak bağlar; gerçek PSP'de `ms0:/PSP/GAME/<ad>/` olacak.
- Bilinen zararsız hatalar (log'da): `IP::create` null (ağ kapalı); `DirAccessUnix::change_dir` önceki dizine dönerken
  `chdir("umd0:")` başarısız (libcglue `getcwd` cihaz kökünü `/` olmadan döndürüyor); global script cache yok.
- Tüm değişiklikler: `docs/psp/porting-notes.md`.

## Budama seçenekleri (karar noktası için)

1. **`renderer_rd`'yi derlemeden çıkarmak** — en büyük tek kazanç (bağlama öncesi 5.2 MB). `RenderingServerDefault`,
   `main.cpp` ve birkaç sahne sınıfının RD'ye doğrudan referanslarının `#ifdef` ile ayrılmasını gerektirir.
2. **`disable_advanced_gui=yes`** — SCons'ta hazır seçenek; `scene/gui`'nin büyük kısmı (demo UI kullanmıyor).
3. **Sınıf kaydı kısıtlama** (`build_profile` ile kullanılmayan sınıfları devre dışı bırakma) — Godot'un kendi
   mekanizması; 2D fizik/navigasyon/animasyon ağacı vb.
4. **thirdparty**: mbedtls (ağ yok), zstd/minizip gereksiz olabilir — küçük kazanç (< 0.8 MB).

## Budama sonuçları

| Adım | EBOOT.PBP | ELF toplam | Heap peak (120. kare) |
|---|---|---|---|
| Spike (başlangıç) | 21 379 644 B (20.39 MB) | 21.31 MB | 11.66 MB |
| 3b: `disable_advanced_gui=yes` | 19 364 060 B (18.47 MB) | 19.38 MB | 10.73 MB |
