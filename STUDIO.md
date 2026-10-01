# Godot Studio (editor workflow fork)

This branch (`main`) is Godot **4.7** plus editor workflow features inspired by
[GDstudio](https://gdstudio.dev/). It stays a thin layer on top of upstream:
upstream `4.7` is merged in every night, and projects remain 100% compatible with
stock Godot 4.7 (no project file format changes).

The PSP port lives on its own branch, `psp-port`. Editor features can be merged
into it by hand when wanted.

```
upstream/4.7 ──(nightly, automatic)──> main  (4.7.x + Studio editor features)
      │                                  │ (optional, manual merge)
      └────────────(manual)──────────> psp-port
```

## Features

| Feature | How to use | Setting |
|---------|------------|---------|
| **Layout pages** | Title bar, right side: one button per saved layout. Click to switch (clicking the active page does nothing — use *Save Current Layout Here* to keep changes); `+` saves the current dock layout as a new page; right-click a page to save over it, rename or delete it. Pages are the same layouts as *Editor > Editor Layout*, so both menus stay in sync. Open scenes and tabs are not affected by switching. | `interface/studio/pages/auto_save_on_switch` (default on): store the current dock layout into the active page before switching away. |
| **Page shortcuts** | `Ctrl+Alt+1` … `Ctrl+Alt+9` switch to page 1–9. | Editor Settings > Shortcuts > `studio/page_N` |
| **Dock region toggles** | Two buttons next to the pages show/hide all **left** or all **right** docks. `Ctrl+Alt+,` (left) and `Ctrl+Alt+.` (right). Focusing a dock in a hidden region shows the region again. Works together with distraction-free mode. | Shortcuts `studio/toggle_left_docks`, `studio/toggle_right_docks` |
| **Bottom drawer** | Optional: the bottom panel (Output, Debugger, …) closes by itself when you click/focus elsewhere in the main window, unless it is pinned. Open it as usual (`Ctrl+J` or its tabs). | `interface/studio/bottom_drawer/auto_hide` (default off) |

Current page is remembered per project (project metadata, written on save and on
clean exit). Dock region visibility is per session.

### Roadmap

See `docs/superpowers/specs/2026-10-01-editor-studio-design.md`:
Phase 2 — tab improvements (reopen closed tab, scene cache, unload, floating tabs);
Phase 3 — measured import/startup speed-ups; Phase 4 — split-pane multi-scene editing.

## Staying in sync with upstream

- **Rule:** Studio code lives in `editor/studio/`, `tests/editor/studio/`, `misc/studio/`.
  Any change to an upstream file is marked `// STUDIO: <feature>` and listed in
  [`STUDIO_HOOKS.md`](STUDIO_HOOKS.md). `misc/studio/check_hooks.py` enforces this in CI.
- **Nightly:** `.github/workflows/studio_upstream_sync.yml` merges `upstream/4.7` into a
  `sync/upstream-4.7` branch, builds it and runs all tests. Green → `main` fast-forwards.
  Conflict, red, or `main` moved meanwhile → one pull request is opened (or commented on) and `main` is left untouched.
- **By hand:** `misc/studio/sync_upstream.sh` (fetch + merge + build + tests + smoke test).
  Exit code 2 means conflicts; resolve them, re-check every file in `STUDIO_HOOKS.md`,
  commit. `git rerere` is enabled by the script so a resolution is reused next time.
- **Moving to 4.8:** a deliberate step — change `UPSTREAM_BRANCH` in the workflow and
  the `--branch` default in the script, then merge and fix.

### One-time GitHub settings

1. Make `main` the default branch (scheduled workflows only run on the default branch).
2. Set the repository variable `DISABLE_GODOT_CI=true` so upstream's heavy all-platform
   CI (`runner.yml`) does not run on every push; Studio CI covers the Linux editor.
3. Allow GitHub Actions to create pull requests
   (Settings > Actions > General > Workflow permissions).
4. Add the repository secret `STUDIO_SYNC_TOKEN`: a fine-grained token for this repo with
   *Contents*, *Pull requests* and *Workflows* read/write. The default `GITHUB_TOKEN` cannot
   push merges that touch `.github/workflows/`, which upstream changes regularly; without
   the secret the nightly sync fails loudly instead of silently.

## Build and test

```bash
scons platform=linuxbsd target=editor tests=yes debug_symbols=no -j$(nproc)
bin/godot.linuxbsd.editor.x86_64 --headless --test --test-case="*[Studio]*"   # Studio tests
bin/godot.linuxbsd.editor.x86_64 --headless --test                            # everything
misc/studio/smoke_test.sh bin/godot.linuxbsd.editor.x86_64                    # editor boots
python3 misc/studio/check_hooks.py                                            # hook list
```
