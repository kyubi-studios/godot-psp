# Scene Preview dock (Faz 4 aşama 1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Başka bir sahneyi canlı, salt-okunur olarak yan yana gösteren yüzdürülebilir dock.

**Architecture:** `StudioScenePreview : EditorDock` + saf kamera/sığdırma yardımcıları; StudioEditor dock'u stok `add_dock` ile ekler.

**Tech Stack:** C++ (SubViewport, Camera3D, EditorDock), doctest.

**Spec:** `docs/superpowers/specs/2026-10-01-studio-scene-preview-design.md`

## Global Constraints
- Yeni çekirdek hook yok (gerekirse Ruling + STUDIO_HOOKS.md).
- Dock adı "Scene Preview", layout key `StudioScenePreview`, varsayılan slot `DOCK_SLOT_RIGHT_BL`, tüm layout'lar (dikey, yatay, yüzen).
- Önizlenen sahne `CACHE_MODE_IGNORE` ile yüklenir.

## Review Focus
1. Önizlenen dosya silinir/bozuk olursa: çökme yok, dock'ta okunur hata mesajı.
2. Boş sahne / sınırı olmayan sahne (AABB boş): kamera geçerli bir varsayılan konuma gider (NaN yok).
3. Önizleme, düzenlenen sahneyi değiştirmemeli (ayrı örnek, ayrı dünya).
4. Dock kapalıyken/gizliyken render ve yenileme maliyeti olmamalı.
5. `@tool` script içeren sahneler: düzenleyicide nasıl çalışıyorsa öyle (yeni risk eklememeli); script hatası dock'u bozmamalı.

---

### Task 1: Saf yardımcılar
**Files:** `editor/studio/studio_scene_preview.{h,cpp}` (yalnız statikler), `tests/editor/studio/test_studio_scene_preview.cpp`
**Produces:**
- `static Transform3D StudioScenePreview::orbit_transform(const AABB &p_bounds, float p_yaw, float p_pitch, float p_distance_scale)` — hedef = AABB merkezi (boşsa orijin), mesafe = max(0.5, uzunluk·distance_scale), kamera hedefe bakar.
- `static Transform2D StudioScenePreview::fit_canvas_transform(const Rect2 &p_content, const Size2 &p_viewport, float p_zoom, const Vector2 &p_pan)` — içeriği %90 dolduracak ölçek × zoom, ortala, + pan; içerik boşsa ölçek 1.
- [ ] Testler (merkez, mesafe, boş AABB, kameranın hedefe bakması; 2D ölçek/merkez/boş içerik/sıfır viewport) → RED → uygula → GREEN → commit.

### Task 2: Dock + yükleme/yenileme
**Produces:** `void set_scene_path(const String &)`, `String get_scene_path() const`, `void refresh()`, `Node *get_preview_root() const`, `String get_error() const`.
- [ ] `[SceneTree][Studio]` testleri: geçici 3D `.tscn` → kök oluşur, `is_3d`; yenileme eski kökü serbest bırakır (ObjectID geçersiz); olmayan dosya → `get_error()` dolu, kök yok.
- [ ] Varsayılan ışık/ortam (sahnede yoksa), kamera yerleşimi, fare yörünge/zoom/pan.
- [ ] Görünür değilken `SubViewport::UPDATE_DISABLED`.
- [ ] RED → uygula → GREEN → commit.

### Task 3: Editör entegrasyonu + görsel doğrulama
- [ ] Üst şerit (sahne seçici: açık sekmeler + dosya seç; Yenile; Editörde aç; Kaydedince yenile), `scene_saved` bağlantısı, StudioEditor'da `add_dock`.
- [ ] Smoke + gerçek editörde ekran görüntüsü (main.tscn düzenlenirken başka sahnenin önizlemesi; yüzen pencere).
- [ ] STUDIO.md, tam test paketi, review, push.
