# Yan yana sahne: Scene Preview alanı (Faz 4, aşama 1) — Tasarım

Tarih: 2026-10-01 · Durum: kullanıcı "devam et" (karar bana bırakıldı)

## Neden bu kapsam
Faz 4'ün tam hali (iki sahneyi aynı anda *düzenlemek*: pane başına inspector, seçim, undo)
EditorNode/EditorData/InspectorDock/SceneTreeDock ve 2D/3D editör eklentilerinin "tek düzenlenen
sahne" varsayımını kırmayı gerektirir; bu, Godot'tan ayrışmamak hedefiyle çelişen yüzlerce satırlık
çekirdek değişiklik demek. Kod incelemesi: `EditorNode::set_edited_scene_root` başka bir parent'a
sahip kökü reddeder, aktif olmayan sekmelerin kökleri ağaç dışında tutulur — yani başka bir
sekmenin canlı köküyle ikinci bir viewport'u beslemek güvenli değildir.

Spec'teki aşama 1 ("ikinci pane'de salt-okunur önizleme") bu yüzden önce yapılır ve tek başına
değerlidir: düzenlediğin sahneyi (ör. bir düşman) kullanıldığı yerde (ör. level) aynı anda görmek,
ikinci monitörde canlı önizleme tutmak.

## Ne yapar
- **Scene Preview** dock'u (stok dock sistemi: herhangi bir slota sürüklenir, yüzen pencereye/ikinci
  monitöre alınır, düzen sayfalarına kaydedilir).
- Üst şerit: sahne seçici (açık sahne sekmeleri + "Dosya seç..."), Yenile, "Editörde aç",
  "Kaydedilince yenile" (varsayılan açık).
- İçerik: kendi dünyası olan `SubViewport`. Sahne diskteki halinden örneklenir (düzenlenen
  sahnenin kendisine dokunulmaz).
  - 3D: sahnede ışık/ortam yoksa varsayılan güneş + ortam eklenir; fare ile yörünge (sağ/orta
    sürükle), tekerlek ile yakınlaş; kamera sahnenin sınır kutusuna otomatik sığdırılır.
  - 2D: içerik sınırlarına sığdır; sürükle ile kaydır, tekerlek ile yakınlaş.
- Kaydedildiğinde (EditorNode `scene_saved`) ve yeniden import'ta önizlenen sahne yenilenir.

## Mimari
- `editor/studio/studio_scene_preview.{h,cpp}`: `StudioScenePreview : EditorDock`.
- Saf yardımcılar (birim test): `orbit_transform(AABB, yaw, pitch, distance_scale)`,
  `fit_canvas_transform(Rect2 content, Size2 viewport, float zoom, Vector2 pan)`,
  `is_3d_scene(Node*)`.
- StudioEditor `setup()` içinde `EditorDockManager::add_dock()` — yeni çekirdek hook yok.
- Yükleme `CACHE_MODE_IGNORE` (sahne dosyası diskten taze; bağımlılıklar önbellekten).

## Kapsam dışı
Önizleme içinde düzenleme/seçim (aşama 2–3), oynatma/animasyon önizleme.

## Test
Saf yardımcılar için birim test; `[SceneTree]` testi: geçici `.tscn` önizlemeye yüklenir,
SubViewport'ta tek kök olur, yenileme eskisini serbest bırakır, olmayan dosya hata mesajı gösterir.
Gerçek editörde ekran görüntüsü.
