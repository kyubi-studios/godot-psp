# Studio hooks in upstream-owned files

Every change Studio makes to a file owned by upstream Godot is marked with a
`// STUDIO: <feature>` (or `# STUDIO:`) comment and listed here. When merging
`upstream/4.7`, re-check each listed file. `misc/studio/check_hooks.py` (run in CI)
fails if this table and the code disagree.

Only the first backticked path of each row is checked; keep one file per row.

| File | Feature | What the hook does |
|------|---------|--------------------|
| `editor/SCsub` | scaffold | Adds `SConscript("studio/SCsub")` so `editor/studio/*.cpp` is built. |
| `editor/editor_node.h` | scaffold | Forward-declares `StudioEditor` and adds the `studio_editor` member. |
| `editor/editor_node.cpp` | scaffold | Includes `studio_editor.h`; creates `StudioEditor` after the bottom panel and calls `setup(title_bar, right_menu_hb, bottom_panel)`. |
