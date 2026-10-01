# Editor Studio — Faz 0 + Faz 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Dal/senkron altyapısını kurmak (Faz 0) ve Layout Pages + dock bölge toggle + alt drawer (Faz 1) özelliklerini upstream'e minimum çakışma yüzeyiyle eklemek.

**Architecture:** Tüm yeni kod `editor/studio/` altında. `StudioEditor` (Node) EditorNode'un çocuğu olarak yaşar, kısayolları ve UI'ı yönetir. Saf mantık (`StudioLayoutPages`, `StudioDrawer::should_auto_hide`, slot→bölge eşlemesi) statik fonksiyonlardır ve doctest ile test edilir. Çekirdek dosyalara sadece `// STUDIO:` işaretli küçük bloklar eklenir; `misc/studio/check_hooks.py` bunların `STUDIO_HOOKS.md`'de listelendiğini doğrular.

**Tech Stack:** C++17 (Godot 4.7 editor), SCons, doctest (`--test`), GitHub Actions, Python 3, bash.

**Spec:** `docs/superpowers/specs/2026-10-01-editor-studio-design.md`

## Global Constraints

- Upstream: `upstream/4.7` (godotengine/godot, dal `4.7`). Senkron günlük.
- Çekirdek dosyalardaki her değişiklik `// STUDIO: <feature>` yorumuyla başlar ve `STUDIO_HOOKS.md`'de listelenir.
- Proje dosya formatı değişmez; ek durum yalnızca EditorSettings / `editor_layouts.cfg` / project metadata'da.
- Upstream workflow dosyalarına (`.github/workflows/*` mevcutlar) dokunulmaz; bizimkiler `studio_*.yml`.
- Build: `scons platform=linuxbsd target=editor tests=yes debug_symbols=no -j11`; binary `bin/godot.linuxbsd.editor.x86_64`.
- Test etiketi: `[Studio]`; editor singleton gerekenler `[Editor][Studio]`.
- Dosya başlıkları Godot lisans başlığı ile (upstream stili), tab girinti, `#pragma once`.

## Review Focus

1. Sayfa adı geçersiz (boş, `/`, `\`) → kaydedilmez, hata gösterilir; mevcut config bozulmaz.
2. Sayfa yeniden adlandırılırken hedef ad zaten varsa → işlem reddedilir, kaynak sayfa korunur.
3. `editor_layouts.cfg` yoksa/bozuksa → Pages bar boş "Default" ile açılır, editör çökmez.
4. Gizli bir dock bölgesindeki dock'a kısayolla odaklanınca → bölge otomatik görünür olur.
5. Distraction-free modu ile bölge toggle etkileşimi → distraction-free kapandığında gizli bölgeler gizli kalır, diğerleri döner.

---

### Task 1: Faz 0 — `editor/studio` iskeleti, StudioEditor hook, hook denetleyici, smoke test

**Files:**
- Create: `editor/studio/SCsub`, `editor/studio/studio_editor.h`, `editor/studio/studio_editor.cpp`
- Create: `STUDIO_HOOKS.md`, `misc/studio/check_hooks.py`, `misc/studio/smoke_test.sh`
- Create: `tests/editor/studio/test_studio_editor.cpp`
- Modify: `editor/SCsub` (SConscript satırı), `editor/editor_node.h` (forward decl + üye), `editor/editor_node.cpp` (include + oluşturma)

**Interfaces:**
- Produces: `class StudioEditor : public Node` — `static StudioEditor *get_singleton()`, `void setup(EditorTitleBar *p_title_bar, Control *p_title_right_container, EditorBottomPanel *p_bottom_panel)`.
- Produces: `python3 misc/studio/check_hooks.py` → exit 0 / 1.
- Produces: `misc/studio/smoke_test.sh <godot_binary>` → exit 0 / 1.

- [ ] **Step 1:** `tests/editor/studio/test_studio_editor.cpp` yaz: `[Studio]` test — `StudioEditor` oluşturulabiliyor, `get_singleton()` onu döndürüyor, silinince `nullptr`.
- [ ] **Step 2:** Build → derleme hatası (sınıf yok) = beklenen FAIL.
- [ ] **Step 3:** `editor/studio/SCsub` (`env.add_source_files(env.editor_sources, "*.cpp")`), `editor/SCsub`'a `SConscript("studio/SCsub")  # STUDIO: scaffold`, `StudioEditor` sınıfı.
- [ ] **Step 4:** EditorNode hook: constructor'da bottom panel oluşturulduktan sonra
  ```cpp
  	// STUDIO: scaffold
  	studio_editor = memnew(StudioEditor);
  	add_child(studio_editor);
  	studio_editor->setup(title_bar, right_menu_hb, bottom_panel);
  ```
- [ ] **Step 5:** `check_hooks.py`: `git ls-files` içinde `editor/studio/`, `tests/editor/studio/`, `misc/studio/`, `docs/` dışındaki dosyalarda `STUDIO:` arar; her dosya yolu `STUDIO_HOOKS.md`'de geçmeli; ayrıca HOOKS'ta listelenip artık işaret içermeyen dosyaları raporlar.
- [ ] **Step 6:** `smoke_test.sh`: geçici dizinde minimal `project.godot` oluşturur, `--headless --editor --path <dir> --quit-after 600` çalıştırır, exit kodu ≠0 veya çıktıda `SCRIPT ERROR|Segmentation|CrashHandler|ERROR: .*[Ss]tudio` varsa FAIL.
- [ ] **Step 7:** Build, `bin/godot.linuxbsd.editor.x86_64 --test --test-case="*[Studio]*"` PASS, smoke PASS, check_hooks PASS.
- [ ] **Step 8:** Commit `studio: scaffold editor/studio module, StudioEditor hook, hook checker, smoke test`.

### Task 2: Faz 0 — CI ve upstream senkron otomasyonu

**Files:**
- Create: `.github/workflows/studio_ci.yml`, `.github/workflows/studio_upstream_sync.yml`, `misc/studio/sync_upstream.sh`

**Interfaces:**
- Consumes: Task 1'in `check_hooks.py`, `smoke_test.sh`.
- Produces: `misc/studio/sync_upstream.sh [--no-build]` — yerel kullanım: `upstream/4.7` fetch + `main`'e merge + build + test + smoke; çakışmada `git merge --abort` yapmaz, durumu raporlar ve exit 2.

- [ ] **Step 1:** `sync_upstream.sh` yaz; `bash -n` ile sözdizimi kontrolü; `--no-build` ile yerelde çalıştır (zaten güncel → "Already up to date", exit 0).
- [ ] **Step 2:** `studio_ci.yml`: `on: push (main, studio/**), pull_request (main), workflow_dispatch`. Tek job: ubuntu-22.04, checkout, `./.github/actions/godot-deps`, cache restore/save (`studio-linux-editor`), `./.github/actions/godot-build` (`target=editor tests=yes`, `scons-flags: debug_symbols=no`), sonra: `check_hooks.py`, `--test --test-case="*[Studio]*"`, tam `--test` (headless), `smoke_test.sh`.
- [ ] **Step 3:** `studio_upstream_sync.yml`: `schedule: cron '17 3 * * *'` + `workflow_dispatch`. Adımlar: checkout (`fetch-depth: 0`, `ref: main`), `git remote add upstream`, fetch `4.7`; `git merge-base --is-ancestor upstream/4.7 HEAD` ise çık; `sync/upstream-<run_id>` dalı aç, merge; çakışmada çakışan dosyaları listeleyip dalı çakışma işaretleriyle değil, merge'siz push edip `gh pr create` ile "Upstream 4.7 merge conflict" PR'ı aç ve job'u fail et; çakışma yoksa build+test+smoke (studio_ci ile aynı adımlar, reusable composite değil düz kopya), başarılıysa `git push origin HEAD:main` (fast-forward), başarısızsa dalı push edip PR aç.
- [ ] **Step 4:** YAML doğrulama: `python3 -c "import yaml,sys; [yaml.safe_load(open(f)) for f in sys.argv[1:]]" .github/workflows/studio_*.yml`.
- [ ] **Step 5:** Commit `studio: CI workflow and nightly upstream/4.7 sync`.

### Task 3: Faz 1 — `StudioLayoutPages` saf mantığı

**Files:**
- Create: `editor/studio/studio_layout_pages.h/.cpp`, `tests/editor/studio/test_studio_layout_pages.cpp`

**Interfaces:**
- Produces (hepsi statik, `Ref<ConfigFile>` üzerinde, `editor_layouts.cfg` formatı = stock "Editor Layout" menüsü ile aynı: sayfa = `/` içermeyen bölüm, alt bölümler `"<ad>/..."`):
  - `static String validate_page_name(const String &p_name)` → boş String = geçerli, aksi halde hata mesajı.
  - `static PackedStringArray list_pages(const Ref<ConfigFile> &p_config)` — config sırası korunur.
  - `static void erase_page(const Ref<ConfigFile> &p_config, const String &p_name)`.
  - `static Error rename_page(const Ref<ConfigFile> &p_config, const String &p_from, const String &p_to)` — `ERR_DOES_NOT_EXIST`, `ERR_ALREADY_EXISTS`, `ERR_INVALID_PARAMETER`.
  - `static Error copy_page(const Ref<ConfigFile> &p_config, const String &p_from, const String &p_to)` (rename bunun üstüne kurulur).

- [ ] **Step 1:** Testler: geçerli/geçersiz adlar (`""`, `"  "`, `"a/b"`, `"a\\b"`, `"Anim"`); list_pages alt bölümleri dışlar ve sırayı korur; erase alt bölümleri de siler, başka sayfaya dokunmaz (`"Anim"` silinince `"Animation"` kalır); rename tüm anahtarları ve alt bölümleri taşır; hedef varsa `ERR_ALREADY_EXISTS` ve kaynak aynen kalır; olmayan kaynak `ERR_DOES_NOT_EXIST`.
- [ ] **Step 2:** Build → FAIL (derleme).
- [ ] **Step 3:** Uygula.
- [ ] **Step 4:** `--test --test-case="*[Studio]*"` PASS.
- [ ] **Step 5:** Commit `studio: layout pages model (list/validate/rename/erase) with tests`.

### Task 4: Faz 1 — Pages bar UI, sayfa geçişi, kısayollar

**Files:**
- Create: `editor/studio/studio_pages_bar.h/.cpp`
- Modify: `editor/studio/studio_editor.h/.cpp`; `editor/editor_node.h/.cpp` (STUDIO hook: layout uygula/kaydet köprüsü)

**Interfaces:**
- Consumes: `StudioLayoutPages` (Task 3), `EditorDockManager::save_docks_to_config/load_docks_from_config`, `EditorSettings::get_editor_layouts_config()`, `EditorSettings::set_project_metadata("studio", "current_page", name)`.
- Produces: `StudioPagesBar : HBoxContainer` — `void refresh()`, `void switch_to_page(const String &p_name)`, `void save_current_page()`, `String get_current_page() const`; sinyal `page_changed(String)`.
- Kısayollar: `studio/page_1` … `studio/page_9` = `Ctrl+Alt+1..9`; `studio/save_page` (atanmamış).
- Ayar: `interface/studio/pages/auto_save_on_switch` (bool, varsayılan `true`).

Davranış:
- Bar, başlık çubuğunda `right_menu_hb`'nin başına eklenir: sayfa başına `Button` (toggle, button group), `+` butonu (yeni sayfa: mevcut düzeni yeni ada kaydeder, ad için `EditorLayoutsDialog` değil basit `AcceptDialog`+`LineEdit`), sağ tık menüsü: Rename / Delete / Save current layout here.
- Geçiş: auto_save açıksa mevcut sayfaya `save_docks_to_config` → hedef sayfayı `load_docks_from_config` → config kaydet → `EditorNode::save_editor_layout_delayed()` → metadata güncelle.
- Hiç sayfa yoksa bar yalnızca `+` gösterir; config okunamazsa sessizce boş kabul edilir (Review Focus #3).
- Stock "Editor Layout" menüsü değiştiğinde bar'ın güncellenmesi: EditorNode'daki `_update_layouts_menu()` sonuna `// STUDIO: pages` hook ile `StudioEditor::get_singleton()->notify_layouts_changed()` (singleton null-check).

- [ ] **Step 1:** `[Editor][Studio]` test: `StudioPagesBar` EditorDockManager olmadan oluşturulup, geçici `editor_layouts.cfg` içeriğiyle `refresh()` sonrası buton sayısı = sayfa sayısı + 1 (`+`). (Bar config yolunu `set_config_path_for_test(String)` ile alır.)
- [ ] **Step 2:** Build → FAIL.
- [ ] **Step 3:** Uygula (bar, StudioEditor'da oluşturma + `shortcut_input` ile page_N, EditorNode hook).
- [ ] **Step 4:** Testler PASS, smoke PASS, check_hooks PASS.
- [ ] **Step 5:** Commit `studio: layout pages bar in title bar with Ctrl+Alt+1..9 switching`.

### Task 5: Faz 1 — Dock bölgesi (sol/sağ) toggle

**Files:**
- Modify: `editor/docks/editor_dock_manager.h/.cpp`, `editor/docks/dock_tab_container.cpp` (STUDIO hook'ları)
- Modify: `editor/studio/studio_editor.h/.cpp` (kısayollar + başlık butonları)
- Test: `tests/editor/studio/test_studio_dock_regions.cpp`

**Interfaces:**
- Produces (EditorDockManager):
  - `enum DockRegion { DOCK_REGION_NONE = -1, DOCK_REGION_LEFT, DOCK_REGION_RIGHT, DOCK_REGION_BOTTOM, DOCK_REGION_MAX }`
  - `static DockRegion get_slot_region(int p_slot)` — LEFT_* → LEFT, RIGHT_* → RIGHT, BOTTOM* → BOTTOM, diğer → NONE.
  - `void set_dock_region_visible(DockRegion p_region, bool p_visible)`, `bool is_dock_region_visible(DockRegion p_region) const`.
- Hook: `DockTabContainer::update_visibility()` ve `can_switch_dock()` bölge görünürlüğünü de hesaba katar; `_make_dock_visible` gizli bölgeyi açar (Review Focus #4). BOTTOM bölgesi için toggle uygulanmaz (alt panel zaten `Ctrl+J`).
- Kısayollar: `studio/toggle_left_docks` = `Ctrl+Alt+[`, `studio/toggle_right_docks` = `Ctrl+Alt+]`.

- [ ] **Step 1:** Test: `get_slot_region` tüm `DOCK_SLOT_*` için doğru bölge; `DOCK_SLOT_NONE` ve `DOCK_SLOT_MAX` → NONE.
- [ ] **Step 2:** Build → FAIL.
- [ ] **Step 3:** Uygula + hook'ları `STUDIO_HOOKS.md`'ye ekle.
- [ ] **Step 4:** Testler, smoke, check_hooks PASS.
- [ ] **Step 5:** Commit `studio: per-region dock visibility toggles (Ctrl+Alt+, / .)`.

### Task 6: Faz 1 — Alt drawer (otomatik gizlenen alt panel)

**Files:**
- Create: `editor/studio/studio_drawer.h/.cpp`, `tests/editor/studio/test_studio_drawer.cpp`
- Modify: `editor/studio/studio_editor.cpp`; `editor/gui/editor_bottom_panel.h` (STUDIO hook: `is_pinned()` erişimcisi — mevcut `is_locked()` yeterliyse hook yok)

**Interfaces:**
- Produces: `class StudioDrawer : public Node` — `static bool should_auto_hide(bool p_enabled, bool p_pinned, bool p_panel_open, bool p_focus_inside_panel, bool p_focus_in_popup)`; `void setup(EditorBottomPanel *p_panel)`.
- Ayar: `interface/studio/bottom_drawer/auto_hide` (bool, varsayılan `false` — stock davranış korunur).
- Davranış: ana viewport'un `gui_focus_changed` sinyali; odak alt panel dışındaki bir kontrole geçerse ve panel pin'li değilse `hide_bottom_panel()`. Popup/dialog içine geçen odak gizlemez.

- [ ] **Step 1:** Test: `should_auto_hide` doğruluk tablosu (devre dışı → false; pin → false; panel kapalı → false; odak içeride → false; popup → false; aksi true).
- [ ] **Step 2:** Build → FAIL.
- [ ] **Step 3:** Uygula.
- [ ] **Step 4:** Testler, smoke PASS.
- [ ] **Step 5:** Commit `studio: optional auto-hiding bottom drawer`.

### Task 7: Yayın — dokümantasyon ve push

**Files:**
- Create: `STUDIO.md` (özellikler, kısayollar, ayarlar, dal modeli, senkron, GitHub ayarları: default branch = `main`, repo variable `DISABLE_GODOT_CI=true`)

- [ ] **Step 1:** `STUDIO.md` yaz.
- [ ] **Step 2:** Tam doğrulama: build, tüm `--test`, smoke, check_hooks, `sync_upstream.sh --no-build`.
- [ ] **Step 3:** Commit, `git push -u origin main` (yeni dal; origin'de `main` yoktu — force gerekmez).
