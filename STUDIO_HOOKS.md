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
| `editor/editor_node.cpp` | scaffold | Includes `studio_editor.h`; creates `StudioEditor` after the bottom panel and calls `setup(title_bar, right_menu_hb, bottom_panel)`; connects `layouts_changed` to `_update_layouts_menu`. `_update_layouts_menu()` starts with a `STUDIO: pages` block calling `notify_layouts_changed()`. `_layout_menu_option()` calls `notify_layout_loaded()` after loading Default or a named layout. `_remove_scene()` starts with a `STUDIO: scene cache` block calling `notify_scene_closing()`. |
| `editor/docks/editor_dock_manager.h` | dock regions | `DockRegion` enum, `get_slot_region()`, `set/is_dock_region_visible()`, `dock_region_visible[]` member, `_bind_methods()` declaration. |
| `editor/docks/editor_dock_manager.cpp` | dock regions | Region API + `dock_region_visibility_changed` signal; `_make_dock_visible()` re-shows a hidden region before focusing a dock in it; `save_docks_to_config()` keeps the saved widths of hidden-region columns. |
| `editor/docks/dock_tab_container.cpp` | dock regions | `update_visibility()` and `can_switch_dock()` also require the slot's region to be visible. |
| `editor/settings/editor_settings.cpp` | look | Adds `Studio` to the theme style and color preset enums; registers `interface/theme/studio/{base_hue,accent_hue,vividness}`. |
| `editor/themes/editor_theme_manager.cpp` | look | Studio style = Modern + `StudioTheme::populate_overrides()`; Studio corner radius; Studio color preset via `StudioTheme::get_preset_colors()`. |
