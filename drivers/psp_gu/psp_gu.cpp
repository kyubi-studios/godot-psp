#include "psp_gu.h"

#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <string.h>

namespace PSPGU {

Stats stats;

static unsigned int __attribute__((aligned(16))) lists[2][LIST_BYTES / sizeof(unsigned int)];
static int cur_list = 0;
static bool frame_open = false;
static uint32_t draw_fb = FB0;
static char overlay[96] = {};

void init() {
	sceGuInit();
	sceGuStart(GU_DIRECT, lists[0]);
	sceGuDrawBuffer(GU_PSM_5650, (void *)FB0, BUF_STRIDE);
	sceGuDispBuffer(SCREEN_W, SCREEN_H, (void *)FB1, BUF_STRIDE);
	sceGuDepthBuffer((void *)ZBUF, BUF_STRIDE);
	sceGuOffset(2048 - (SCREEN_W / 2), 2048 - (SCREEN_H / 2));
	sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuClearDepth(0);
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
	draw_fb = FB0;
}

void frame_begin() {
	if (frame_open) {
		return;
	}
	sceGuStart(GU_DIRECT, lists[cur_list]);
	frame_open = true;
}

void ensure_list_space(int p_bytes) {
	if (!frame_open) {
		return;
	}
	if (sceGuCheckList() + p_bytes + LIST_MARGIN <= LIST_BYTES) {
		return;
	}
	sceGuFinish();
	sceGuSync(0, 0);
	sceGuStart(GU_DIRECT, lists[cur_list]);
	stats.list_flushes++;
}

bool in_frame() {
	return frame_open;
}

void clear(uint32_t p_abgr) {
	sceGuClearColor(p_abgr);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

void set_overlay_text(const char *p_text) {
	if (!p_text) {
		overlay[0] = 0;
		return;
	}
	strncpy(overlay, p_text, sizeof(overlay) - 1);
	overlay[sizeof(overlay) - 1] = 0;
}

static void draw_overlay() {
	if (!overlay[0]) {
		return;
	}
	// GE bitti; çizim buffer'ına CPU ile (önbelleksiz VRAM) debug fontuyla yaz.
	// InitEx buffer'ı temizler: yalnızca bir kez, sonra her kare sadece taban adresi değişir.
	static bool debug_inited = false;
	if (!debug_inited) {
		pspDebugScreenInitEx((void *)(0x44000000 + draw_fb), PSP_DISPLAY_PIXEL_FORMAT_565, 0);
		pspDebugScreenEnableBackColor(0);
		pspDebugScreenSetTextColor(0xffffffff);
		debug_inited = true;
	}
	pspDebugScreenSetBase((u32 *)(0x44000000 + draw_fb));
	pspDebugScreenSetXY(0, 0);
	pspDebugScreenPrintData(overlay, strlen(overlay));
}

void frame_end(bool p_present) {
	if (!frame_open) {
		return;
	}
	sceGuFinish();
	sceGuSync(0, 0);
	frame_open = false;
	cur_list ^= 1;
	stats.frames++;
	stats.draws = stats.draws_accum;
	stats.draws_accum = 0;
	if (!p_present) {
		return;
	}
	draw_overlay();
	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();
	draw_fb = (draw_fb == FB0) ? FB1 : FB0;
}

uint32_t color_to_abgr(float p_r, float p_g, float p_b, float p_a) {
	auto c = [](float v) -> uint32_t {
		if (v <= 0.0f) {
			return 0;
		}
		if (v >= 1.0f) {
			return 255;
		}
		return (uint32_t)(v * 255.0f + 0.5f);
	};
	return (c(p_a) << 24) | (c(p_b) << 16) | (c(p_g) << 8) | c(p_r);
}

} // namespace PSPGU
