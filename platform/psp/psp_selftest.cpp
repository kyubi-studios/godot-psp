#include "psp_ctrl_map.h"
#include "psp_log.h"
#include "psp_paths.h"

#include "drivers/psp_gu/psp_texture.h"

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

	psp_log("[PSP] selftest done fails=%d", fails);
	return fails;
}
