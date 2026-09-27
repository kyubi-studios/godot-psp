#include "psp_ctrl_map.h"
#include "psp_log.h"
#include "psp_paths.h"

#include "drivers/psp_gu/psp_gu.h"
#include "drivers/psp_gu/psp_texture.h"
#include "drivers/psp_gu/rasterizer_scene_psp.h"
#include "drivers/psp_gu/storage/mesh_storage_psp.h"

#include <pspgu.h>

#include <stdlib.h>
#include <string.h>

#include <pspctrl.h>

static int fails = 0;

#define PSP_CHECK(m_cond, m_msg)                   \
	if (!(m_cond)) {                               \
		psp_log("[PSP] FAIL selftest %s", m_msg); \
		fails++;                                   \
	}

int psp_selftest() {
	PSPJoyEvent ev[16];

	int n = psp_ctrl_map(0, PSP_CTRL_CROSS, ev, 16);
	PSP_CHECK(n == 1 && ev[0].button == (int)JoyButton::A && ev[0].pressed, "cross press");

	n = psp_ctrl_map(PSP_CTRL_CROSS, 0, ev, 16);
	PSP_CHECK(n == 1 && ev[0].button == (int)JoyButton::A && !ev[0].pressed, "cross release");

	n = psp_ctrl_map(0, PSP_CTRL_UP | PSP_CTRL_RTRIGGER | PSP_CTRL_START, ev, 16);
	PSP_CHECK(n == 3, "three buttons at once");

	n = psp_ctrl_map(PSP_CTRL_UP, PSP_CTRL_UP, ev, 16);
	PSP_CHECK(n == 0, "held button no event");

	n = psp_ctrl_map(0, 0xFFFFFFFF, ev, 2);
	PSP_CHECK(n == 2, "event buffer limit");

	PSP_CHECK(psp_axis_map(128) == 0.0f, "axis center dead zone");
	PSP_CHECK(psp_axis_map(0) <= -0.99f, "axis min");
	PSP_CHECK(psp_axis_map(255) >= 0.99f, "axis max");
	PSP_CHECK(psp_axis_map(140) == 0.0f, "axis small offset dead zone");

	// psp_game_dir: argv[0] → oyun klasörü (Review I5).
	char dir[64];
	psp_game_dir("umd0:/EBOOT.PBP", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, "umd0:/") == 0, "game_dir umd0 root");
	psp_game_dir("ms0:/PSP/GAME/GodotDemo/EBOOT.PBP", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, "ms0:/PSP/GAME/GodotDemo") == 0, "game_dir ms0 folder");
	psp_game_dir("", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, ".") == 0, "game_dir empty argv0");
	psp_game_dir(nullptr, dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, ".") == 0, "game_dir null argv0");
	psp_game_dir("EBOOT.PBP", dir, sizeof(dir));
	PSP_CHECK(strcmp(dir, ".") == 0, "game_dir no slash");

	// Peak, örnekler arasında serbest bırakılan tahsisleri de görmeli (Review I3).
	// GCC malloc/free çiftlerini silip sonucu katlayabilir; volatile işaretçiyle gerçek çağrıyı zorla.
	void *(*volatile do_malloc)(size_t) = malloc;
	void (*volatile do_free)(void *) = free;
	uint32_t used0, peak0, used1, peak1;
	psp_mem_stats(used0, peak0);
	const size_t spike = 2 * 1024 * 1024;
	void *p = do_malloc(spike);
	if (p) {
		memset(p, 0xAB, spike);
	}
	do_free(p);
	psp_mem_stats(used1, peak1);
	PSP_CHECK(p != nullptr && peak1 >= peak0 + spike, "peak sees freed spike");

	// Bellek tükenmesi ayırt edici bir log satırı üretmeli (Review I3).
	psp_oom_expected = true;
	void *huge = do_malloc(200 * 1024 * 1024);
	psp_oom_expected = false;
	PSP_CHECK(huge == nullptr, "huge malloc fails");
	PSP_CHECK(psp_oom_count == 1, "oom counted");
	do_free(huge);

	// Texture dönüştürme (Faz 2 Task 4).
	{
		PSPTextureData td;
		Ref<Image> opaque = Image::create_empty(8, 8, false, Image::FORMAT_RGBA8);
		opaque->fill(Color(1, 0, 0, 1));
		PSP_CHECK(PSPTexture::convert(opaque, td), "tex opaque converts");
		PSP_CHECK(td.psm == GU_PSM_5650 && td.width == 8 && td.height == 8, "tex opaque -> 5650 8x8");
		PSP_CHECK(td.pixels && ((uint16_t *)td.pixels)[0] == 0x001F, "tex 5650 red bits");
		PSPTexture::free_data(td);

		Ref<Image> binary = Image::create_empty(8, 8, false, Image::FORMAT_RGBA8);
		binary->fill(Color(1, 0, 0, 1));
		binary->set_pixel(1, 0, Color(0, 0, 0, 0));
		PSP_CHECK(PSPTexture::convert(binary, td) && td.psm == GU_PSM_5551, "tex binary alpha -> 5551");
		PSP_CHECK(td.pixels && ((uint16_t *)td.pixels)[0] == 0x801F && ((uint16_t *)td.pixels)[1] == 0x0000, "tex 5551 bits");
		PSPTexture::free_data(td);

		Ref<Image> soft = Image::create_empty(8, 8, false, Image::FORMAT_RGBA8);
		soft->fill(Color(1, 0, 0, 0.5f));
		PSP_CHECK(PSPTexture::convert(soft, td) && td.psm == GU_PSM_4444, "tex soft alpha -> 4444");
		PSP_CHECK(td.pixels && (((uint16_t *)td.pixels)[0] & 0x0FFF) == 0x000F && (((uint16_t *)td.pixels)[0] >> 12) >= 7, "tex 4444 bits");
		PSPTexture::free_data(td);

		Ref<Image> npot = Image::create_empty(9, 5, false, Image::FORMAT_RGB8);
		npot->fill(Color(0, 1, 0));
		// En yakın 2'nin kuvveti (bellek): 9 → 8, 5 → 4.
		PSP_CHECK(PSPTexture::convert(npot, td) && td.width == 8 && td.height == 4 && td.source_width == 9, "tex npot -> 8x4");
		PSPTexture::free_data(td);

		Ref<Image> big = Image::create_empty(300, 40, false, Image::FORMAT_RGB8);
		big->fill(Color(0, 0, 1));
		PSP_CHECK(PSPTexture::convert(big, td) && td.width == 256 && td.height == 32, "tex clamp 300x40 -> 256x32");
		PSPTexture::free_data(td);

		Ref<Image> tiny = Image::create_empty(2, 2, false, Image::FORMAT_RGBA8);
		tiny->set_pixel(0, 0, Color(1, 0, 0, 1));
		tiny->set_pixel(1, 0, Color(0, 1, 0, 1));
		tiny->set_pixel(0, 1, Color(0, 1, 0, 1));
		tiny->set_pixel(1, 1, Color(1, 0, 0, 1));
		// 2x2 → 8x2 (genişlik en az 8, nearest büyütme deseni korur): (0..3,0) kırmızı, (4..7,0) yeşil.
		PSP_CHECK(PSPTexture::convert(tiny, td) && td.width == 8 && td.height == 2, "tex tiny -> 8x2");
		PSP_CHECK(td.pixels && ((uint16_t *)td.pixels)[3] == 0x001F && ((uint16_t *)td.pixels)[4] == 0x07E0, "tex tiny nearest pattern");
		PSPTexture::free_data(td);

		// Swizzle: 16x8 piksel (32 byte x 8 satır) = iki 16x8 byte blok; blok1'in ilk satırı = doğrusal (8,0).
		uint16_t lin[16 * 8], swz[16 * 8];
		for (int i = 0; i < 16 * 8; i++) {
			lin[i] = (uint16_t)i;
		}
		PSPTexture::swizzle((uint8_t *)swz, (const uint8_t *)lin, 32, 8);
		PSP_CHECK(swz[0] == 0 && swz[7] == 7 && swz[8] == 16 && swz[64] == 8 && swz[127] == 127, "tex swizzle blocks");
	}

	// Işık eşleştirme: RendererSceneCull en iyi 4 omni ve 4 spot'u ayrı tutar, placement_idx ile yer değiştirir (inceleme C2).
	{
		RasterizerScenePSP::GeometryInstancePSP gi;
		RID a = RID::from_uint64(1), b = RID::from_uint64(2), c = RID::from_uint64(3), d = RID::from_uint64(4);
		RID e = RID::from_uint64(5), sp = RID::from_uint64(6);
		gi.pair_light_instance(a, RSE::LIGHT_OMNI, 0);
		gi.pair_light_instance(b, RSE::LIGHT_OMNI, 1);
		gi.pair_light_instance(c, RSE::LIGHT_OMNI, 2);
		gi.pair_light_instance(d, RSE::LIGHT_OMNI, 3);
		gi.pair_light_instance(e, RSE::LIGHT_OMNI, 1); // daha iyi ışık 1. yuvayı değiştirir
		gi.pair_light_instance(sp, RSE::LIGHT_SPOT, 0);
		PSP_CHECK(gi.omni_count == 4 && gi.omni_lights[1] == e && gi.omni_lights[0] == a, "light pair replace by placement");
		PSP_CHECK(gi.spot_count == 1 && gi.spot_lights[0] == sp, "spot kept separately from 4 omnis");
		gi.clear_light_instances();
		PSP_CHECK(gi.omni_count == 0 && gi.spot_count == 0, "light pair clear");
	}

	// Normaller: model matrisi yüzey AABB ölçeğini içerir; depolanan normal bunu telafi etmeli (inceleme I1).
	{
		Array arrays;
		arrays.resize(RSE::ARRAY_MAX);
		const Vector3 n = Vector3(1, 1, 0).normalized();
		arrays[RSE::ARRAY_VERTEX] = PackedVector3Array({ Vector3(-1, -0.1, -1), Vector3(1, 0.1, 1), Vector3(1, -0.1, -1) });
		arrays[RSE::ARRAY_NORMAL] = PackedVector3Array({ n, n, n });
		RendererPSP::Surface surf;
		const bool ok = RendererPSP::MeshStorage::build_surface(arrays, RSE::PRIMITIVE_TRIANGLES, AABB(Vector3(-1, -0.1, -1), Vector3(2, 0.2, 2)), surf);
		PSP_CHECK(ok, "thin surface builds");
		if (ok) {
			const int8_t *sn = (const int8_t *)surf.vertices; // düzen: normal (3+1) sonra pozisyon
			const Vector3 world = (Vector3(sn[0], sn[1], sn[2]) / 127.0f * surf.pos_half).normalized();
			PSP_CHECK(world.distance_to(n) < 0.05f, "normal survives non-uniform AABB scale");
			free(surf.vertices);
			free(surf.indices);
		}
	}

	// Büyük mipmap'li texture: dönüştürme tam boyutu açmamalı (inceleme I4).
	{
		Ref<Image> big = Image::create_empty(1024, 1024, true, Image::FORMAT_RGBA8);
		big->fill(Color(0.5, 0.5, 0.5, 1));
		uint32_t u0, p0, u1, p1;
		psp_mem_stats(u0, p0);
		PSPTextureData td;
		const bool ok = PSPTexture::convert(big, td);
		psp_mem_stats(u1, p1);
		PSP_CHECK(ok && td.width == 256 && td.height == 256, "big mipmapped texture -> 256");
		PSP_CHECK(p1 - p0 < 2 * 1024 * 1024, "big texture conversion peak < 2 MB");
		psp_log("[PSP] selftest big texture peak delta=%u", (unsigned)(p1 - p0));
		PSPTexture::free_data(td);
	}

	// Texture VRAM önbelleği: LRU, bu karede kullanılanlar atılmaz (GE okuyor olabilir).
	{
		static uint8_t blocks[5][16];
		const uint32_t sz = 300 * 1024; // 4 tanesi ~1.2 MB boş VRAM'e sığar
		auto in_vram = [](const void *p) { return (uintptr_t)p >= 0x04000000 && (uintptr_t)p < 0x04200000; };
		const uint32_t saved_frame = PSPGU::stats.frames;
		PSPGU::stats.frames = 1000;
		bool all4 = true;
		for (int i = 0; i < 4; i++) {
			all4 = all4 && in_vram(PSPGU::texture_address(blocks[i], sz));
		}
		PSP_CHECK(all4 && PSPGU::stats.vram_textures == 4, "vram cache holds 4 blocks");
		PSP_CHECK(!in_vram(PSPGU::texture_address(blocks[4], sz)), "vram full this frame -> RAM fallback");
		PSPGU::stats.frames = 1001;
		PSPGU::texture_address(blocks[1], sz); // 1..3 bu karede kullanıldı, 0 en eski
		PSPGU::texture_address(blocks[2], sz);
		PSPGU::texture_address(blocks[3], sz);
		PSP_CHECK(in_vram(PSPGU::texture_address(blocks[4], sz)), "new frame evicts LRU block");
		PSP_CHECK(!in_vram(PSPGU::texture_address(blocks[0], sz)), "evicted block not resident when rest in use");
		for (int i = 0; i < 5; i++) {
			PSPGU::texture_forget(blocks[i]);
		}
		PSP_CHECK(PSPGU::stats.vram_textures == 0 && PSPGU::stats.vram_bytes == 0, "vram cache forget");
		PSPGU::stats.frames = saved_frame;
	}

	psp_log("[PSP] selftest done fails=%d", fails);
	return fails;
}
