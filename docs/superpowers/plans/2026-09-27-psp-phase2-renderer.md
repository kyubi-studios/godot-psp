# PSP Port — Faz 2: PSP GU Renderer ve 3D Demo — Uygulama Planı

> Yürütme: superpowers:executing-plans (inline). Kullanıcı ara onay istemiyor; kararlar ledger'da `Ruling:` olarak.

**Goal:** Godot 4.7.2'nin 3D sahnelerini PSP GE (sceGu) ile çizen `drivers/psp_gu` renderer'ı ve editörde hazırlanan
`.tscn` tabanlı 3D demo; PPSSPP'de ekran görüntüsüyle doğrulanır.

**Spec:** `docs/superpowers/specs/2026-09-27-godot4-psp-port-design.md` (bölüm 3–6).

**Architecture:** `RasterizerPSP : RendererCompositor`. Fog/GI/particles/canvas için Godot'un dummy sınıfları
aynen kullanılır. Mesh, texture, material, light, utilities ve sahne sınıfları PSP'ye özel:
- Texture/Light/Material/Mesh/Utilities: `RendererDummy::*` alt sınıfları (dummy stub'ları miras, gerekli API'ler ezilir).
- Sahne: `RasterizerSceneDummy` alt sınıfı; `render_scene` GU çizimi yapar.
- Material: ShaderLanguage derleyicisi YOK — shader kodundan `render_mode`/ipucu metin aramasıyla bayraklar,
  yalnızca whitelist parametreler (`albedo`, `texture_albedo`, `uv1_scale`, `uv1_offset`, `alpha_scissor_threshold`).
- Mesh: yüklemede bir kez GU formatına: 16-bit pozisyon (yüzey AABB'sine göre ölçekli), 8-bit normal,
  16-bit UV (TexScale/Offset), opsiyonel 8888 renk, 16-bit indeks. Godot dizileri saklanmaz.
- Texture: RGBA8'e çevir → 2'nin kuvveti, en fazla 256 → alfa yok: 5650, ikili alfa: 5551, diğer: 4444 → swizzle.
  Ana RAM'de (16 byte hizalı). VRAM önbelleği (LRU) sonraki adım (YAGNI: önce ölç).
- VRAM: fb0 0x000000, fb1 0x044000 (5650, stride 512), z 0x088000 (16-bit). Display list 2×64 KB.
- Kare: `begin_frame` → sceGuStart; `render_scene` → temizle + çiz; `end_frame` → finish/sync, debug yazı,
  vblank, swap. Faz 1'deki `OS_PSP::run` içindeki `swap_buffers()` kaldırılır.

## Tasks

1. **İskelet + temizleme:** `drivers/psp_gu` (rasterizer, GU context, storage alt sınıfları dummy davranışında),
   `DisplayServerPSP` bu renderer'ı seçer. Test: boş sahne + Environment bg rengi → ekran görüntüsünde o renk;
   vsync ≈ 1000 ms/60 kare; boot/quit testleri PASS.
2. **Mesh + kamera:** `MeshStoragePSP`, geometry instance, projeksiyon/görünüm/model matrisleri, unshaded çizim.
   Test: `mesh_unshaded` projesi (kamera + kırmızı unshaded BoxMesh) → merkez piksel kırmızı, köşe bg.
3. **Material + ışık:** albedo rengi, cull, saydamlık (sıralı), GU ışıkları (directional/omni/spot, en fazla 4),
   ambient, fog. Test: `lit_scene` projesi — ışıklı yüz ile gölgede yüz parlaklık farkı; saydam nesne karışımı.
4. **Texture:** `TextureStoragePSP` (dönüştürme + swizzle), material texture bağlama. Test: self-test swizzle/format
   birim kontrolleri + `textured` projesi (damalı texture) → iki bilinen pikselde iki farklı renk.
5. **Demo:** `godot_projects/psp_demo3d` (Godot 4.7 editörüyle açılır), `.tscn` + `psp_behavior` metadata
   (spinner / orbit camera) C++ davranışları, FPS/RAM yazısı, `tools/psp/export_pck.sh` + `--main-pack`.
   Test: demo EBOOT ekran görüntüsü, ≥30 FPS (emülatör), peak < 44 MB, temiz çıkış.
6. **Final inceleme** (fresh reviewer) + düzeltmeler + `main`'e birleştirme.

## Review Focus

1. Mesh yüzeyinde normal/UV/renk olmaması (ör. yalnızca pozisyonlu mesh) — vertex formatı buna göre daralmalı, çökmemeli.
2. Texture boyutu 2'nin kuvveti değil / 256'dan büyük / sıkıştırılmış (VRAM compressed) — güvenli dönüşüm veya uyarı.
3. Sahnede 4'ten fazla ışık — en etkili 4 seçilmeli, GU ışık yuvaları taşmamalı.
4. Display list taşması (çok sayıda draw) — liste boyutu kontrol edilmeli, taşma loglanmalı.
5. Materyal parametresi olmayan/ShaderMaterial — beyaz, tek seferlik uyarı, çökme yok.
