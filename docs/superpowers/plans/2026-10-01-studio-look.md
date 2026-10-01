# Studio görünümü (Faz 5a) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** GDstudio benzeri "Studio" tema stili + canlı renk ayarları + başlıkta "Son dosyalar" butonu.

**Architecture:** Studio stili = Modern stili + `StudioTheme::populate_overrides()`. Studio renk preset'i ton/canlılık ayarlarından `StudioThemeColors` ile hesaplanır. Çekirdeğe yalnızca `editor_settings.cpp` ve `editor_theme_manager.cpp`'de `// STUDIO: look` hook'ları.

**Tech Stack:** C++ (Godot editor theme API: `EditorTheme`, `StyleBoxFlat`), doctest.

**Spec:** `docs/superpowers/specs/2026-10-01-studio-look-design.md`

## Global Constraints

- Ayar yolları: `interface/theme/studio/base_hue` (0–1, 0.62), `interface/theme/studio/accent_hue` (0–1, 0.25), `interface/theme/studio/vividness` (0–1, 0.30).
- Style enum: `"Modern,Classic,Studio"`; color preset enum'una `Studio` eklenir.
- Hook'lar `// STUDIO: look` ve STUDIO_HOOKS.md'de.
- Modern/Classic stillerinin çıktısı değişmez.

## Review Focus

1. Studio dışındaki stiller/presetler etkilenmemeli (Modern temada popup opak kalmalı).
2. Ton ayarları uç değerlerde (0, 1) geçerli renk üretmeli; canlılık 0 → tam gri.
3. Kullanıcı "Custom" preset'e geçerse Studio ton ayarları renkleri ezmemeli.
4. Hafif (light) baz rengi yok: Studio daima koyu tema; ikon/font rengi "Auto" ile doğru seçilmeli.
5. "Son dosyalar" listesi boşken/silinmiş dosya içerdiğinde çökmemeli; silinenler listelenmemeli.

---

### Task 1: `StudioThemeColors` (saf renk hesabı)

**Files:** Create `editor/studio/studio_theme.h/.cpp`, `tests/editor/studio/test_studio_theme.cpp`

**Interfaces (Produces):**
- `static Color StudioThemeColors::base_color(float p_hue, float p_vividness)` — HSV(h, 0.12 + 0.35·v when v>0 else 0, 0.16), alfa 1.
- `static Color StudioThemeColors::accent_color(float p_hue, float p_vividness)` — HSV(h, 0.45 + 0.35·v, 0.85).
- Girdiler [0,1]'e kırpılır.

- [ ] Testler: hue 0.62 → mavi kanal baskın; vividness 0 → r=g=b; hue 0/1 aynı; değerler [0,1]; base koyu (luminance < 0.25), accent açık (> 0.35).
- [ ] RED → uygula → GREEN → commit `studio(look): theme color math`.

### Task 2: Studio stili + preset hook'ları

**Files:** Modify `editor/settings/editor_settings.cpp`, `editor/themes/editor_theme_manager.cpp`, `editor/studio/studio_theme.*`, STUDIO_HOOKS.md; Test `tests/editor/studio/test_studio_theme.cpp`

**Interfaces (Produces):**
- `static void StudioTheme::apply_color_preset(EditorThemeManager::ThemeConfiguration &r_config)` — base/accent/contrast(0.25)/icon_saturation(default) ayarlardan.
- `static void StudioTheme::populate_overrides(const Ref<EditorTheme> &p_theme, const EditorThemeManager::ThemeConfiguration &p_config)`.
- `generate_theme`: `is_default_style = style == "Modern" || style == "Studio"`; sonda `if (style == "Studio") StudioTheme::populate_overrides(...)`.
- Stil işleme dalı: Studio → corner_radius 5, relationship lines default.

- [ ] `[Editor][Studio]` test: style=Studio + preset=Studio ile `EditorThemeManager::generate_theme()`; PopupMenu `panel` StyleBoxFlat alfa < 1 ve köşe > 0; style=Modern ile alfa == 1. (generate_theme testte çalışmazsa: `populate_overrides`'ı boş `EditorTheme` + sentetik config ile test et ve Ruling yaz.)
- [ ] RED → uygula → GREEN; smoke; check_hooks → commit `studio(look): Studio theme style and color preset`.

### Task 3: Studio override içeriği + görsel doğrulama

- [ ] Popup/menü/tooltip, dock TabContainer başlık bandı + seçili sekme vurgu çizgisi, Tree seçim dolgusu.
- [ ] Testler: TabContainer `tab_selected` StyleBoxFlat üst kenarlık > 0 ve renk = accent.
- [ ] Gerçek editörde ekran görüntüsü (Modern vs Studio), ton değiştirip yeniden görüntü.
- [ ] Commit `studio(look): popup, dock header and selection styling`.

### Task 4: Başlıkta "Son dosyalar" butonu

**Files:** Create `editor/studio/studio_recent_button.h/.cpp`, test; Modify `studio_editor.cpp`

**Interfaces:** `static PackedStringArray StudioRecentButton::collect(const Array &p_scenes, const Array &p_scripts, const Callable &p_exists, int p_max)` — sahneler önce, tekrarlar ve mevcut olmayanlar çıkarılır. Tıklama: sahne → `EditorNode::open_request`, script → `EditorInterface::edit_resource(load(path))`.

- [ ] Testler: sıralama, tekrar, silinmiş dosya, boş liste, max sınırı.
- [ ] RED → uygula → GREEN; smoke → commit `studio(look): recent files button in title bar`.

### Task 5: Dokümantasyon + push
- [ ] STUDIO.md'ye "Görünüm" bölümü, tam test paketi, push.
