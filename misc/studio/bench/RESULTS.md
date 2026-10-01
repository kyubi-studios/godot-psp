# Studio benchmark results

Generated with `misc/studio/bench/make_project.py <dir>` (defaults: 1000 textures of 512×512 RGBA
noise, 50 glTF boxes, a 3000-node scene referencing 300 textures; seed 1, 861 MB) and measured with
`misc/studio/bench/run_bench.py <godot> <dir> --runs 3`. Times are medians in milliseconds.

Noise textures are a worst case for lossless encoders, so absolute import times overstate real
projects; compare rows with each other, not with other machines.

**Machine:** AMD Ryzen 5 5600 6-Core Processor, 12 threads, project on an NTFS (ntfs3) data disk, Linux 6.8.0-134-generic.
**Build:** `scons platform=linuxbsd target=editor tests=yes debug_symbols=no` (optimize=speed_trace default).

| Label | Commit | cold_import | warm_open | scene_load | Notes |
|---|---|---:|---:|---:|---|
| baseline | 5aed80b50d | 10645 | 2637 | 1353 | upstream 4.7 + Studio phase 0/1 (no perf changes) |
| idle-wait (rejected) | 8031cc11b7 | 10417 | — | — | 1 ms sleep instead of spinning on `imported_sem.try_wait()` in threaded import. Interleaved A/B, 5 runs each: base 10473 vs 10417 (−0.5%, noise). A first non-interleaved run suggested −9.6%, an ordering/thermal artifact. Not merged. |

**Method note:** compare candidates with interleaved A/B runs of two saved binaries
(`base, cand, base, cand, …`), never with back-to-back batches.

## Profile: warm headless editor startup (2026-10-01)

`profile/gdb_sample.py` on an unstripped relink of 8031cc11b, 3 runs × ~2.7 s, 5 ms interval,
1909 main-thread samples (`--headless --path <bench project> --import`, warm caches).
Inclusive share of main-thread time:

| Area | Share | Notes |
|---|---:|---|
| `EditorNode::EditorNode()` | 24% | building the whole editor UI; `FileDialog::FileDialog()` alone 6% (dialogs created eagerly) |
| `Node::_propagate_enter_tree()` / `_propagate_ready()` | 23% / 19% | theme lookups (`ThemeDB::update_class_instance_items` 16%), minimum-size computation, **text shaping in headless mode** (`TextServerAdvanced::_shaped_text_shape` 8%) |
| `SceneTree::finalize()` (quit) | 17% | ~0.45 s tearing down every editor node at exit |
| `Main::setup` | 7% | incl. SDL joypad init 4% |
| `EditorThemeManager::generate_theme` | 4% | |

Import itself (cold run, verbose timestamps): ~7.3 s of the 10.6 s cold import is the texture
importer on 12 threads (lossless encoding of noise, worst case); ~2.7 s startup; ~1 s quit.

**Candidates for the next experiments** (each needs an interleaved A/B, ≥5% gain to keep):
1. Lazily create the editor's `EditorFileDialog`s on first use (≈6% of startup).
2. Skip text shaping / min-size work for controls that are not visible yet (deep in `scene/gui`;
   better as an upstream PR than a Studio hook).
3. Faster quit: skip freeing the editor UI tree when the process is exiting (risky: destructors
   also save state; needs an audit).
4. Phase 2 scene cache: the 3000-node bench scene takes 1.35 s to load, so keeping recently
   closed scenes loaded would make reopening near-instant.
