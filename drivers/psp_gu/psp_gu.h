#pragma once

#include <cstdint>

// PSP GE (sceGu) bağlamı: VRAM yerleşimi, display list ve kare döngüsü.
//
// VRAM (2 MB, 0x04000000):
//   0x000000  framebuffer 0  (5650, stride 512, 480x272)  = 0x44000
//   0x044000  framebuffer 1  (5650)                        = 0x44000
//   0x088000  z-buffer       (16-bit)                      = 0x44000
//   0x0CC000  boş (~1.2 MB) — ileride texture önbelleği
namespace PSPGU {

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 272;
constexpr int BUF_STRIDE = 512;
constexpr uint32_t FB0 = 0x000000;
constexpr uint32_t FB1 = 0x044000;
constexpr uint32_t ZBUF = 0x088000;
constexpr uint32_t VRAM_FREE = 0x0CC000;
constexpr int LIST_BYTES = 64 * 1024;

void init();
void frame_begin();
// Listeyi bitirir, GE'yi bekler, (varsa) overlay yazısını basar, vblank bekler ve buffer'ları değiştirir.
void frame_end(bool p_present);
bool in_frame();

// Display list'te en az p_bytes yer bırakır: liste dolmak üzereyse GE'nin bitirmesini bekleyip aynı buffer'da
// yeni liste başlatır (GE durumu/matrisler donanımda kalır). Çok sayıda çizimde taşmayı önler.
void ensure_list_space(int p_bytes);
constexpr int LIST_MARGIN = 4096;

// Kare içi geçici bellek (display list içinden; kare sonunda geçersiz). Liste yeri önceden sağlanır.
void *frame_alloc(int p_bytes);

// ABGR8888 (PSP sırası) renkle renk ve derinlik buffer'ını temizle.
void clear(uint32_t p_abgr);

// Ekranın sol üstüne basılacak yazı (NULL/boş = yok). Kopyalanır.
void set_overlay_text(const char *p_text);

// Sayaçlar (log ve testler için): tamamlanan GE kareleri, render_scene çağrıları, çizim çağrıları.
struct Stats {
	uint32_t frames = 0;
	uint32_t scenes = 0;
	uint32_t draws = 0; // son karedeki çizim çağrısı
	uint32_t draws_accum = 0; // bu karede biriken
	uint32_t list_flushes = 0; // toplam liste yeniden başlatma
};
extern Stats stats;

// Godot Color (0..1) → PSP ABGR8888.
uint32_t color_to_abgr(float p_r, float p_g, float p_b, float p_a);

} // namespace PSPGU
