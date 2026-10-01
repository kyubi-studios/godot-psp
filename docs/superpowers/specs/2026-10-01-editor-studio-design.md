# Editor Studio — Tasarım (Spec)

Tarih: 2026-10-01
Durum: Onaylandı (sohbet içinde), yazılı spec incelemesi bekleniyor

## 1. Amaç

Godot 4.7 editörüne ekibin günlük iş akışını hızlandıran özellikler eklemek
(GDstudio'dan esinlenerek: layout sayfaları, dock kolaylıkları, gelişmiş sekmeler,
daha hızlı import/açılış, yan yana çoklu sahne) — **upstream Godot'tan ayrışmadan**.
Upstream 4.7 dalındaki her düzeltme otomatik olarak bize gelmeye devam etmeli.

### Başarı kriterleri

- `upstream/4.7` her gece `main`'e çakışmasız (veya rerere ile otomatik çözülerek) merge edilir;
  çakışma olursa main kırılmaz, PR/issue açılır.
- Godot çekirdek dosyalarındaki değişikliklerimiz `STUDIO_HOOKS.md`'de listelenmiş,
  küçük ve `// STUDIO:` ile işaretlidir. Yeni kodun büyük kısmı `editor/studio/` altındadır.
- Projeler stock Godot 4.7 ile birebir uyumlu kalır (proje dosya formatı değişmez;
  ek durum yalnızca editör ayarlarında / `.godot/editor/` altında tutulur).
- Her faz unit test + headless editor smoke test ile doğrulanır; Faz 3 sayısal benchmark ile.

### Kapsam dışı

- GDstudio'nun tema/frosted-glass görselleri, kendi layout framework'ü, auto-update, telemetri.
- Proje dosya formatında değişiklik.
- PSP port'una editör özelliği getirmek (isteğe bağlı merge ile sonradan yapılabilir).

## 2. Dal ve upstream stratejisi (Faz 0)

```
upstream/4.7 ──(gece otomatik merge)──> main  (4.7.x + editör özellikleri)
      │                                   │ (isteğe bağlı, elle merge)
      └──────────(elle merge)──────────> psp-port  (PSP portu)
```

- `psp-port`: bugünkü `main` (4.7.2-stable + 26 PSP commit'i, `origin/psp-port` ile aynı).
  Yerel `psp-port` dalı oluşturulur ve `origin/psp-port`'u takip eder.
- `editor-studio`: `4.7.2-stable` tag'inden açılır; tüm editör işi burada başlar.
- `main`'in `editor-studio`'ya taşınması `origin/main`'e **force-push** gerektirir.
  Bu adım ayrıca kullanıcı onayı ile yapılır; öncesinde PSP commit'lerinin
  `origin/psp-port`'ta bulunduğu doğrulanır ve `origin/main` eski hali
  `archive/main-psp-2026-10-01` tag'i ile işaretlenir.
- Upstream takibi: `upstream/4.7` (kararlı 4.7.x cherry-pick dalı). 4.8 çıkınca bilinçli,
  elle yapılan bir geçiş olur (ayrı spec).

### Senkron otomasyonu

- `.github/workflows/studio_upstream_sync.yml` (cron, günlük):
  1. `upstream/4.7` fetch, `main`'den `sync/upstream-<tarih>` dalı aç, `git merge upstream/4.7`.
  2. Çakışma yoksa: Linux editor build (`scons platform=linuxbsd target=editor tests=yes`),
     `godot --test` ve headless smoke test (`--headless --editor --quit` boş proje ile).
  3. Hepsi geçerse `main`'e fast-forward push; geçmezse PR açılır (çakışma/kırılma raporu ile).
- Yerelde `git config rerere.enabled true`; `rr-cache` paylaşımı gerekmez (CI çakışmada PR açar).
- `STUDIO_HOOKS.md`: dosya → hook → amaç tablosu. Merge sırasında kontrol listesi.
- Upstream'in kendi CI workflow'ları (android/ios/web/...) korunur ama fork'ta
  sadece `linux_builds` ve `static_checks` tetiklenir (gereksiz CI dakikası harcamamak için
  workflow'lar silinmez, `if: github.repository == 'godotengine/godot'` yerine
  ayrı bir `studio_ci.yml` eklenir; upstream dosyalarına dokunulmaz).

## 3. Mimari ilke: modül + ince hook

- Yeni kod: `editor/studio/` (SCsub ile editor build'ine dahil). Her özellik ayrı bir sınıf:
  `StudioLayoutPages`, `StudioDrawer`, `StudioClosedTabStack`, `StudioSceneCache`, ...
- Çekirdek dosyalara (ör. `editor_node.cpp`, `editor_dock_manager.cpp`) yalnızca:
  - sinyal yayma / sanal çağrı noktası ekleme,
  - `StudioXxx` nesnesini oluşturup ağaca ekleyen 1–5 satırlık blok.
  Her blok `// STUDIO: <özellik>` ile başlar ve `STUDIO_HOOKS.md`'ye yazılır.
- Mümkün olan yerde mevcut `EditorPlugin` / `EditorInterface` API'si kullanılır (hook gerekmez).
- Genel faydalı ve temiz iyileştirmeler (özellikle Faz 3 performans) upstream'e PR olarak
  da gönderilir; kabul edilirse bizdeki fark silinir.

## 4. Faz 1 — Layout Pages + dock UX

Mevcut altyapı: `EditorDockManager::save_docks_to_config / load_docks_from_config`
ve `editor_layouts.cfg` (EditorNode `LAYOUT_SAVE/LAYOUT_DELETE`).

- **Pages**: adlandırılmış layout preset'leri, `editor_layouts.cfg` içindeki mevcut
  bölümlerle aynı formatta saklanır (stock Godot'un "Editor Layout" menüsü ile uyumlu).
  - Başlık çubuğunda sayfa seçici (sekme şeridi), sayfa ekle/yeniden adlandır/sil.
  - `Ctrl+Alt+1..9` kısayolları (EditorSettings shortcut, değiştirilebilir).
  - Sayfa değiştirmek açık sahneleri/sekmeleri etkilemez; sadece dock düzeni + panel boyutları.
  - Aktif sayfadan çıkarken düzen otomatik o sayfaya kaydedilir (ayar ile kapatılabilir).
- **Dock grupları toggle**: sol / sağ / alt dock bölgeleri için ayrı gizle-göster
  kısayolları ve başlık butonları ("distraction free"ın parçalı hali).
- **Sürükle-bırak önizleme**: dock sürüklenirken hedef slot vurgulanır (dock_tab_container
  drop hover).
- **Alt drawer**: alt paneli (Output, Debugger, FileSystem taşınırsa) workspace'i
  küçültmeden üstüne kayan overlay olarak açma modu; kısayol ile aç/kapa; pin ile
  klasik moda dönülür.

Testler: Pages serialize/deserialize (ConfigFile round-trip), kısayol eşleme, sayfa adı
doğrulama; smoke test ile editörün sayfa değiştirip kapanması.

## 5. Faz 2 — Sekme iyileştirmeleri

- **Kapatılan sekmeyi geri aç** (`Ctrl+Shift+T`): sahne, script ve yardım sekmeleri için
  ortak LIFO yığın (`StudioClosedTabStack`, en fazla 20 kayıt; tür + yol + sekme indeksi).
  Hook: `EditorNode::_scene_tab_closed`, ScriptEditor sekme kapatma.
- **Sahne cache'i**: son kapatılan N (varsayılan 5, ayarlanabilir) sahnenin yüklü
  `PackedScene`'i bellekte tutulur; dosyanın mtime/md5'i değişmişse veya kaynak reimport
  edilmişse geçersizlenir. Yeniden açma diskten okumadan olur.
- **Taşıma/yeniden adlandırmada sekme koruma**: FileSystemDock taşıma işlemi açık sahne
  sekmelerinin yolunu günceller (stock davranışta eksik kalan durumlar doğrulanıp düzeltilir).
- **Sekme unload**: sağ tık → "Unload" — sekme yerinde kalır, sahne bellekten atılır,
  tıklanınca yeniden yüklenir. Kaydedilmemiş değişiklik varsa izin verilmez.
- **Floating sekme**: sekmeyi ayrı pencereye çıkarma; mevcut `WindowWrapper` altyapısı
  ile (script editor ve game view zaten kullanıyor). İlk aşamada script/shader sekmeleri;
  sahne sekmeleri Faz 4'e bağlıdır.

Testler: kapatılan-sekme yığını, cache geçersizleme (mtime değişimi), unload/reload durumu.

## 6. Faz 3 — Daha hızlı import ve açılış

Önce ölçüm, sonra değişiklik. Değişiklik ancak benchmark'ta ölçülebilir kazanç
gösterirse kalır.

- **Benchmark seti** (`misc/studio/bench/`): büyük örnek proje üreten script
  (binlerce PNG, onlarca glTF, büyük .tscn), ölçümler:
  soğuk açılış (`.godot` yok), sıcak açılış, tam reimport, büyük sahne açma süresi.
  Script `--headless --editor --quit` süresini ve `--verbose` import loglarını toplar.
- **Aday iyileştirmeler** (profil sonucuna göre önceliklenecek):
  - `EditorFileSystem::_reimport_files`: thread'siz importer'lar (scene/glTF) için dosyalar
    arası paralellik — thread-safe olmayan kısımların (ResourceCache, EditorFileSystem
    güncellemeleri) ana thread'de kalması şartıyla.
  - Açılış scan'ında gereksiz md5/stat çağrılarını azaltma (mtime + boyut ön kontrolü).
  - Değişmeyen editör UI'ının (sekme çubuğu, başlık) her karede yeniden çizilmesini önleme.
  - 3D gizmo güncellemelerini kare başına birleştirme (deferred/coalesced).
- Her iyileştirme ayrı commit + benchmark sonucu commit mesajında.

## 7. Faz 4 — Split-pane çoklu sahne

En riskli faz; Faz 1–3 tamamlandıktan sonra **kendi ayrıntılı spec'i** yazılacak.
Ön tasarım:

- Sorun: `EditorNode`, `EditorData`, `InspectorDock`, `SceneTreeDock`, 2D/3D editör
  plugin'leri "tek düzenlenen sahne" varsayar.
- `SceneSpace`: pane başına edited scene, seçim (`EditorSelection`), undo history
  (EditorUndoRedoManager zaten sahne başına history tutar) ve kendi SubViewport'u.
- Aşamalar:
  1. İkinci pane'de salt-okunur önizleme (aynı ya da farklı sahne).
  2. Fokus değişiminde aktif sahne geçişi — inspector/scene tree fokuslu pane'e bağlanır
     (tek inspector, hızlı geçiş).
  3. Pane başına inspector + scene tree.
- Risk: çekirdekte en büyük hook yüzeyi; her aşama sonunda upstream merge maliyeti
  ölçülür, kabul edilemezse aşama 2'de durulur.

## 8. Test ve doğrulama

- `tests/editor/studio/` altında doctest testleri (editor build `tests=yes`).
- Headless smoke test: boş proje ile `--headless --editor --quit-after N`.
- CI: `studio_ci.yml` (push/PR'da Linux editor build + testler), gece senkron workflow'u.
- Faz 3: benchmark sonuçları `misc/studio/bench/RESULTS.md`'de tutulur.

## 9. Faz sırası

| Faz | İçerik | Risk |
|-----|--------|------|
| 0 | Dallar, upstream senkron CI, `editor/studio/` iskeleti, STUDIO_HOOKS.md | Düşük |
| 1 | Layout Pages, dock toggle, sürükleme önizleme, alt drawer | Düşük |
| 2 | Ctrl+Shift+T, sahne cache, taşımada sekme koruma, unload, floating | Orta |
| 3 | Benchmark + import/açılış/redraw optimizasyonları | Orta |
| 4 | Split-pane çoklu sahne (ayrı spec) | Yüksek |
