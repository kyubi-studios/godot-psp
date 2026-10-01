# Editor Studio — Faz 3a: Benchmark altyapısı + ilk import optimizasyonu

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Editör açılış/import/sahne yükleme sürelerini tekrarlanabilir şekilde ölçmek ve ilk aday optimizasyonu (threaded import sırasında ana thread'in meşgul beklemesi) ölçümle kabul/ret etmek.

**Architecture:** `misc/studio/bench/` altında yalnızca Python stdlib kullanan iki araç: `make_project.py` deterministik bir test projesi üretir (PNG, glTF, büyük .tscn), `run_bench.py` Godot binary'sini senaryolarla çalıştırıp medyan süreleri JSON + Markdown tablo olarak verir. Sonuçlar `misc/studio/bench/RESULTS.md`'de tutulur. Optimizasyon, ölçülen kazanç ≥ %5 değilse geri alınır.

**Tech Stack:** Python 3 stdlib (`zlib`, `struct`, `json`, `subprocess`, `unittest`), Godot CLI (`--headless --import`, `-s` script), C++ (editor_file_system.cpp).

**Spec:** `docs/superpowers/specs/2026-10-01-editor-studio-design.md` (§6 Faz 3, §5 kapsam güncellemesi)

## Global Constraints

- Araçlar dış bağımlılık kullanmaz (CI ve yerelde düz `python3` ile çalışmalı).
- Üretilen proje deterministik (aynı parametre → aynı baytlar), `--seed` ile.
- Her ölçüm senaryosu en az 3 kez çalışır, medyan raporlanır; soğuk senaryoda her koşudan önce `.godot/` silinir.
- Çekirdek dosya değişiklikleri `// STUDIO: perf` ile işaretlenir ve STUDIO_HOOKS.md'ye yazılır.
- Kazanç ölçülemezse değişiklik geri alınır ve sonuç RESULTS.md'ye "rejected" olarak yazılır.

## Review Focus

1. Bench projesi üretimi var olan bir dizinin üzerine yazarken kullanıcı dosyası silmemeli (yalnızca boş dizin veya `--force`).
2. Godot süreci çökerse/zaman aşımına uğrarsa `run_bench.py` sonucu "FAILED" olarak raporlamalı, sahte süre yazmamalı.
3. Threaded import değişikliği progress dialog'u dondurmamalı (UI en geç ~100 ms'de bir güncellenmeli).
4. Tek dosyalık ve importer'ı threaded olmayan yollar davranış değiştirmemeli.
5. Ölçüm makinesi/derleme bilgisi (CPU sayısı, commit, build bayrakları) sonuçla birlikte kaydedilmeli; aksi halde sayılar karşılaştırılamaz.

---

### Task 1: `make_project.py` — deterministik bench projesi üretici

**Files:**
- Create: `misc/studio/bench/make_project.py`, `misc/studio/bench/test_bench_tools.py`

**Interfaces:**
- Produces: `python3 misc/studio/bench/make_project.py <dir> [--textures N=400] [--texture-size S=256] [--gltf N=20] [--scene-nodes N=3000] [--seed N=1] [--force]`
  - `<dir>/project.godot`, `<dir>/textures/tex_XXXX.png`, `<dir>/models/model_XXX.gltf` (gömülü base64 buffer, tek mesh), `<dir>/scenes/big.tscn` (N node; her 10. node bir Sprite2D texture referansı), `<dir>/bench_load.gd` (sahneyi `ResourceLoader.load` ile CACHE_MODE_IGNORE yükleyip `BENCH_LOAD_MS=<ms>` basar ve çıkar).
  - Python API: `write_png(path, w, h, seed)`, `write_gltf(path, seed)`, `generate(dir, textures, texture_size, gltf, scene_nodes, seed, force) -> dict(counts)`.
- Hata: `<dir>` boş değilse ve `--force` yoksa exit 2, hiçbir dosyaya dokunmadan.

- [ ] **Step 1:** `test_bench_tools.py` (unittest): PNG imzası + IHDR boyutu doğru; aynı seed iki kez → aynı bayt; glTF JSON geçerli, `buffers[0].byteLength` gömülü veri uzunluğuna eşit; `generate` sayımları; boş olmayan dizinde `force=False` → `FileExistsError` ve mevcut dosya korunur.
- [ ] **Step 2:** `python3 -m unittest misc/studio/bench/test_bench_tools.py` → FAIL (modül yok).
- [ ] **Step 3:** Uygula.
- [ ] **Step 4:** Testler PASS; küçük bir proje üret ve `bin/godot.linuxbsd.editor.x86_64 --headless --path <dir> --import` ile hatasız import edildiğini doğrula (log'da `ERROR` yok).
- [ ] **Step 5:** Commit `studio(bench): deterministic benchmark project generator`.

### Task 2: `run_bench.py` — senaryo koşucu

**Files:**
- Create: `misc/studio/bench/run_bench.py`; Modify: `misc/studio/bench/test_bench_tools.py`

**Interfaces:**
- Produces: `python3 misc/studio/bench/run_bench.py <godot_bin> <project_dir> [--runs 3] [--scenarios cold_import,warm_open,scene_load] [--json out.json] [--label NAME]`
  - `cold_import`: `.godot/` sil → `--headless --path P --import` süresi.
  - `warm_open`: `.godot/` dokunmadan `--headless --path P --import` süresi (değişiklik yok → tarama maliyeti).
  - `scene_load`: `--headless --path P -s res://bench_load.gd` çıktısındaki `BENCH_LOAD_MS`.
  - Çıktı: her senaryo için medyan/min/max ms, `failed` sayısı; meta: label, `git rev-parse HEAD`, `nproc`, binary yolu, tarih. Markdown tablo stdout'a.
- Python API: `median(values)`, `parse_load_ms(text) -> float|None`, `summarize(samples: list[float|None]) -> dict`.

- [ ] **Step 1:** Testler: `median` tek/çift; `parse_load_ms` bulur/bulamaz; `summarize` içinde `None` (başarısız koşu) `failed` sayılır ve medyana girmez, hepsi başarısızsa `median=None`.
- [ ] **Step 2:** FAIL gör.
- [ ] **Step 3:** Uygula (subprocess timeout 1800 s; exit kodu ≠ 0 → None).
- [ ] **Step 4:** Testler PASS; küçük projede 1 koşu ile uçtan uca çalıştır.
- [ ] **Step 5:** Commit `studio(bench): scenario runner with median reporting`.

### Task 3: Baseline ölçümü

**Files:**
- Create: `misc/studio/bench/RESULTS.md`

- [ ] **Step 1:** Varsayılan parametrelerle proje üret (`$SCRATCH/benchproj`), `run_bench.py --runs 3 --label baseline --json baseline.json`.
- [ ] **Step 2:** RESULTS.md: makine/commit/build bilgisi + tablo.
- [ ] **Step 3:** Commit `studio(bench): baseline results`.

### Task 4: Threaded import ana thread meşgul beklemesini kaldırma (deney)

**Files:**
- Modify: `editor/file_system/editor_file_system.cpp` (`_reimport_files` threaded dalı), `STUDIO_HOOKS.md`, `misc/studio/bench/RESULTS.md`

Bulgu: `imported_sem.try_wait()` döngüsü ana thread'de boş döner (`ep->step(..., false)` + `try_wait` sıkı döngü), bir çekirdeği import worker'larından çalar.
Değişiklik: döngüde `try_wait` başarısızsa `OS::get_singleton()->delay_usec(1000)` (1 ms) uyku; progress güncellemesi korunur (Review Focus #3: 1 ms ≪ 100 ms).

- [ ] **Step 1:** Ölçüm = test: değişiklikten önceki `cold_import` medyanı (Task 3) referans.
- [ ] **Step 2:** Değişikliği uygula, build, `[Studio]` + tam test paketi + smoke.
- [ ] **Step 3:** `run_bench.py --runs 3 --label idle-wait --scenarios cold_import`.
- [ ] **Step 4:** Kazanç ≥ %5 → commit `studio(perf): sleep instead of spinning while threaded import runs` + RESULTS.md; değilse `git checkout` ile geri al, RESULTS.md'ye "rejected" yaz ve commit et.
