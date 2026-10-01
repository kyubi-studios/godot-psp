# Studio görünümü (Faz 5a) — Tasarım

Tarih: 2026-10-01 · Durum: kullanıcı onayladı ("yap, çalışmaya başla")

## Amaç
Editör GDstudio gibi görünsün: koyu mavimsi "slate" palet, sıkı yerleşim, panel başlığı gibi
dock sekmeleri, yarı saydam + yuvarlak köşeli popup/menü/tooltip'ler, belirgin vurgu rengi ve
renklerin canlı ayarlanabilmesi (baz ton, canlılık, vurgu tonu). Başlık çubuğunda "Son dosyalar"
butonu. Godot'tan ayrışmadan: çekirdeğe yalnızca küçük `// STUDIO:` hook'ları.

## Mimari
- Godot 4.7 tema stili takılabilir (`interface/theme/style`: Modern, Classic). Üçüncü stil
  **Studio** eklenir: önce Modern stili üretilir, sonra `StudioTheme::populate_overrides()` kendi
  stillerini üzerine yazar. Godot Modern'i geliştirdikçe Studio da otomatik güncellenir.
- Yeni renk preset'i **Studio**: `base_color`/`accent_color` sabit değil,
  `interface/theme/studio/base_hue`, `accent_hue`, `vividness` ayarlarından hesaplanır
  (`StudioThemeColors`, saf fonksiyonlar, birim test). `interface/theme/` altında oldukları için
  değiştirildiklerinde stok mekanizma temayı anında yeniden üretir.
- Hook'lar: `editor_settings.cpp` (enum'lara "Studio" + 3 ayar), `editor_theme_manager.cpp`
  (Studio = Modern + overrides; Studio renk preset dalı).
- Başlık çubuğu: `StudioRecentButton` (MenuButton) — son açılan sahneler; StudioEditor oluşturur.

## Görsel kurallar (Studio overrides)
- Popup/menü/tooltip: arka plan alfa 0.94, köşe yarıçapı `corner_radius + 2`, 1 px açık kenarlık,
  daha belirgin gölge. Saydamlık desteklenmeyen ekranlarda Godot zaten opak çizer.
- Dock sekmeleri (`TabContainer`): sekme şeridi koyu "başlık bandı"; seçili sekmenin üstünde
  vurgu renginde 2 px çizgi.
- Ağaç/liste seçimleri: vurgu renginin yarı saydam dolgusu (GDstudio'daki gibi belirgin seçim).
- Varsayılanlar: base_hue 0.62 (mavi-slate), vividness 0.30, accent_hue 0.25 (yeşil-zeytin).

## Kapsam dışı (sonraki adımlar)
Gerçek "buzlu cam" bulanıklığı (shader), çok-sayfa başlık yeniden tasarımı, split-pane.

## Test
- `StudioThemeColors` birim testleri (ton → baskın kanal, canlılık 0 → gri, aralıklar).
- `[Editor]` testi: Studio stiliyle üretilen temada PopupMenu paneli yarı saydam ve yuvarlak;
  Modern'de değişmemiş.
- Smoke test + gerçek editörde ekran görüntüsü.
