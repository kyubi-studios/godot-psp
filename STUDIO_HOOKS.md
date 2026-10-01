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
| `editor/editor_node.cpp` | scaffold | Includes `studio_editor.h`; creates `StudioEditor` after the bottom panel and calls `setup(title_bar, right_menu_hb, bottom_panel)`; connects `layouts_changed` to `_update_layouts_menu`. `_update_layouts_menu()` starts with a `STUDIO: pages` block calling `notify_layouts_changed()`. |
| `editor/docks/editor_dock_manager.h` | dock regions | `DockRegion` enum, `get_slot_region()`, `set/is_dock_region_visible()`, `dock_region_visible[]` member, `_bind_methods()` declaration. |
| `editor/docks/editor_dock_manager.cpp` | dock regions | Region API + `dock_region_visibility_changed` signal; `_make_dock_visible()` re-shows a hidden region before focusing a dock in it. |
| `editor/docks/dock_tab_container.cpp` | dock regions | `update_visibility()` and `can_switch_dock()` also require the slot's region to be visible. |
