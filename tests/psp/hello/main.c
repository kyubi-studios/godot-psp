#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include "../common/psp_test_log.h"

PSP_MODULE_INFO("psphello", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static unsigned int __attribute__((aligned(16))) list[4096];
typedef struct { unsigned int color; float x, y, z; } Vertex;
static Vertex __attribute__((aligned(16))) tri[3] = {
	{ 0xff0000ff, -1, -1, 0 }, { 0xff00ff00, 1, -1, 0 }, { 0xffff0000, 0, 1, 0 }
};

int main(void) {
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
	sceGuDispBuffer(480, 272, (void *)0x88000, 512);
	sceGuDepthBuffer((void *)0x110000, 512);
	sceGuOffset(2048 - 240, 2048 - 136);
	sceGuViewport(2048, 2048, 480, 272);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
	for (int f = 0; f <= 10; f++) {
		sceGuStart(GU_DIRECT, list);
		sceGuClearColor(0xff402020);
		sceGuClear(GU_COLOR_BUFFER_BIT);
		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumPerspective(60, 480.0f / 272.0f, 0.5f, 100);
		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();
		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		ScePspFVector3 p = { 0, 0, -3 };
		sceGumTranslate(&p);
		sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 3, 0, tri);
		sceGuFinish();
		sceGuSync(0, 0);
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}
	psp_test_log("[PSP] hello frame=10");
	psp_test_screenshot();
	sceKernelExitGame();
	return 0;
}
