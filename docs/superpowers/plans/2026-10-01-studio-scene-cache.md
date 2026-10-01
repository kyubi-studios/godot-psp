# Studio sahne önbelleği (Faz 2) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Kapatılan son N sahnenin `PackedScene`'ini (ve dolayısıyla dokularını/alt kaynaklarını) bellekte tutarak yeniden açmayı anlık yapmak.

**Architecture:** `StudioSceneCache` (saf LRU, `Ref<Resource>` tutar). EditorNode `_remove_scene()` başında tek hook: `StudioEditor::notify_scene_closing(path)` → sahne hâlâ açıkken `ResourceLoader::load(path, CACHE_MODE_REUSE)` ile `PackedScene` alınıp LRU'ya konur (bağımlılıklar önbellekte olduğu için ~25 ms). Yeniden açma stok `load_scene` (`CACHE_MODE_REPLACE`) ile olur; dış bağımlılıklar önbellekten gelir.

**Tech Stack:** C++, doctest.

**Spec:** `docs/superpowers/specs/2026-10-01-editor-studio-design.md` §5 (kapsam güncellemesi: sahne cache'i, kazanç kanıtlanırsa). Kanıt: bench projesinde 3000 node'lu sahne soğuk 1290 ms, bağımlılıklar tutulurken 25 ms (3 koşu).

## Global Constraints

- Ayar: `interface/studio/scene_cache/max_scenes` (int 0–32, varsayılan 5; 0 = kapalı, mevcut önbelleği boşaltır).
- Hook `// STUDIO: scene cache`, STUDIO_HOOKS.md'de.
- Kaydedilmemiş (yolu boş) sahneler önbelleğe alınmaz.

## Review Focus

1. Diskte değişen sahne: yeniden açılış diskteki güncel hali göstermeli (REPLACE ana kaynağı yeniden okur).
2. Silinen/taşınan dosya: önbellek girdisi bayatlasa da açılış hatası olmamalı.
3. Kapasite küçültülünce fazla girdiler hemen atılmalı (bellek geri verilmeli).
4. Aynı sahne iki kez kapatılırsa tekrar girdi oluşmamalı, en yeniye taşınmalı.
5. Editör kapanırken (tüm sahneler kapanır) önbellek gereksiz yükleme yapmamalı.

---

### Task 1: `StudioSceneCache` LRU

**Files:** Create `editor/studio/studio_scene_cache.h/.cpp`, `tests/editor/studio/test_studio_scene_cache.cpp`

**Interfaces:** `void set_capacity(int)`, `int get_capacity() const`, `void put(const String &path, const Ref<Resource> &res)`, `bool has(const String &path) const`, `void erase(const String &path)`, `void clear()`, `PackedStringArray get_paths() const` (en yeni önce), `int size() const`.

- [ ] Testler: kapasite aşımında en eski atılır; tekrar put en yeniye taşır, boyut artmaz; kapasite 0 → put yok sayılır ve mevcutlar silinir; kapasite küçültme fazlaları atar; null kaynak yok sayılır; atılan kaynak serbest kalır (refcount testi).
- [ ] RED → uygula → GREEN → commit `studio(cache): LRU scene cache`.

### Task 2: EditorNode hook + ayar + kapanış koruması

**Files:** Modify `editor/editor_node.cpp` (`_remove_scene` başı), `editor/studio/studio_editor.*`, STUDIO_HOOKS.md

**Interfaces:** `StudioEditor::notify_scene_closing(const String &p_path)`; `StudioEditor::get_scene_cache()`.

- [ ] Test: `register_settings()` sonrası ayar var ve varsayılan 5.
- [ ] Davranış: path boş → yok; `EditorNode::get_singleton()->is_exiting()` → yok; ayar değişince `set_capacity`.
- [ ] Doğrulama: gerçek editörde bench sahnesini aç/kapat/yeniden aç; verbose log zaman damgalarıyla yeniden açma süresi ölç (hedef < 200 ms; önce > 1 s).
- [ ] Commit `studio(cache): keep recently closed scenes loaded`.

### Task 3: Docs + push
