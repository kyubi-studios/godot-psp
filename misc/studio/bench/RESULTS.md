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
