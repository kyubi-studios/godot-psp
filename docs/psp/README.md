# Godot 4.7.2 — PSP portu

Godot 4.7.2'nin PSP (PSP-2001) portu. 3D sahneler PSP GE (sceGu) ile çizilir. Oyun mantığı GDScript değil, C++'tır:
`.tscn` sahnelerindeki node'lar `metadata/psp_behavior` ile işaretlenir.

## Gereksinimler

- pspdev araç zinciri: `~/pspdev` ([pspdev/pspdev](https://github.com/pspdev/pspdev) sürümleri)
- Godot 4.7 editörü (`../Godot_v4.7-stable_mono_linux.x86_64`, ya da `GODOT_EDITOR` ortam değişkeni): oyunu `.pck` olarak export etmek için
- Test için headless PPSSPP: `tools/psp/build_ppsspp_headless.sh` (SDL2 dahil, sudo gerektirmez; `~/ppsspp-build/PPSSPPHeadless`)

## Derleme

```bash
source tools/psp/env.sh
scons platform=psp -j12        # → bin/godot.psp.template_release.mips32.nothreads.elf
```

Varsayılan ayarları `platform/psp/detect.py` belirler: `template_release`, `threads=no`, modüller kapalı, fizik, navigasyon ve XR yok,
`disable_advanced_gui`, `optimize=size`.

## Oyun hazırlama

1. Projeyi Godot 4.7 editöründe oluşturun. Rendering ayarı "Compatibility" olabilir; PSP bu ayarı yok sayar.
2. Texture import ayarı: **Compress Mode = VRAM Uncompressed**, mipmap kapalı. PSP'de PNG/WebP decoder yoktur.
3. Projeye adı `PSP` olan bir export preset'i ekleyin (herhangi bir masaüstü platformu olabilir; yalnızca `.pck` üretilir).
4. `tools/psp/export_pck.sh <proje> game.pck`
5. `tools/psp/stage_game.sh game.pck out/GodotDemo` → `EBOOT.PBP` + `game.pck`

Hazır demo: `../psp_demo3d` (editörle açılabilir).

### Desteklenenler

- **Node'lar:** MeshInstance3D (Box/Sphere/Plane/Cylinder, ArrayMesh), Camera3D, DirectionalLight3D, OmniLight3D, SpotLight3D.
  Nesne başına en fazla 4 ışık kullanılır, gölge yoktur.
- **Environment:** arka plan rengi, ambient, fog (derinlik fog'u; üstel fog doğrusal olarak yaklaşıklanır).
- **StandardMaterial3D:** albedo rengi ve texture, unshaded, cull modları, saydamlık (alpha / alpha scissor / add),
  vertex rengi, uv1 ölçek ve kaydırma, nearest/linear filtre.
- **2D:** ColorRect, Sprite2D, NinePatch/Panel, Polygon2D, Line2D ve primitive'ler, MeshInstance2D, Control kırpma,
  Label/Button yazıları (FreeType text server, Godot'un gömülü varsayılan fontu veya proje fontları).
- **Custom shader'lar desteklenmez:** beyaz çizilir. Multimesh, partikül, iskelet, canvas ışıkları ve canvas
  shader'ları henüz yok.

### Davranış metadata'sı (GDScript yerine)

| Metadata | Etki |
|---|---|
| `psp_behavior = "spinner"`, `spin_speed = Vector3(derece/sn)` | Node3D'yi her karede döndürür |
| `psp_behavior = "orbit_camera"`, `orbit_target = Vector3` | Camera3D: analog/d-pad ile döner, L/R ile yakınlaşır, girdi yoksa kendiliğinden yavaşça döner |

Proje ayarı `psp/show_stats=true` ekranın sol üstüne FPS, RAM ve draw sayısını yazar.

## Test

```bash
tools/psp/run_psp_tests.sh
```

Her test EBOOT'u headless PPSSPP'de çalıştırır; `[PSP]` log satırlarını ve ekran görüntüsü piksellerini kontrol eder.
Ekran görüntüleri `bin/psp_tests/<test>/screenshot.png` altına yazılır.

Oyun klasörüne konan test kancası dosyaları:
- `psp_screenshot_at_frame`: içindeki kare numarasında ekran görüntüsü alır.
- `psp_quit_after_frames`: içindeki kareden sonra çıkış ister (HOME → Çık ile aynı yol).
- `psp_selftest`: EBOOT içi birim testlerini çalıştırır.

## Gerçek PSP'de çalıştırma

İmzasız homebrew çalıştırmak için PSP-2001'de özel firmware gerekir (ör. 6.61 PRO-C / LME).

1. `EBOOT.PBP` ve `game.pck` dosyalarını `ms0:/PSP/GAME/GodotDemo/` klasörüne kopyalayın.
2. XMB → Oyun → Memory Stick menüsünden açın.
3. HOME → Çık ile temiz kapanır. Oyun kapanmayı reddederse 3 saniye sonra zorla çıkılır.

EBOOT `MEMSIZE=1` ile paketlenir: PSP-2000 ve sonrası modellerde genişletilmiş belleği (~52 MB) kullanır.

## Bellek (demo, PPSSPP)

| Kalem | Değer |
|---|---|
| EBOOT.PBP | ~20.5 MB (text server + FreeType + Brotli dahil) |
| Heap peak (demo sahnesi, HUD yazısıyla) | ~14.2 MB |
| Kod + heap | ~35.6 MB (kullanılabilir ~52 MB) |

Değişikliklerin tam listesi: `docs/psp/porting-notes.md`.
