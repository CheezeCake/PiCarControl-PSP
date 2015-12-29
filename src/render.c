// gu stuff taken from pspsdk samples
/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * Copyright (c) 2005 Jesper Svennevid
 */

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspjpeg.h>
#include <psputility_avmodules.h>

#include "render.h"
#include "vram.h"
#include "jpeg_frame.h"

static unsigned int __attribute__((aligned(16))) list[262144];
static void* fbp0 = NULL; // draw buffer
static u32 rgba[SCREEN_WIDTH * SCREEN_HEIGHT];
static u32 __attribute__((aligned(16))) screen[BUFFER_WIDTH * SCREEN_HEIGHT];

struct Vertex
{
	unsigned short u, v;
	unsigned short color;
	short x, y, z;
};

static int jpeg_init(void)
{
	if (sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) < 0) {
		pspDebugScreenPrintf("sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC) failed\n");
		return 1;
	}

	if (sceJpegInitMJpeg() < 0) {
		pspDebugScreenPrintf("sceJpegInitMJpeg() failed\n");
		return 1;
	}

	if (sceJpegCreateMJpeg(SCREEN_WIDTH,SCREEN_HEIGHT) < 0) {
		pspDebugScreenPrintf("sceJpegCreateMJpeg() failed\n");
		return 1;
	}
}

static void jpeg_term(void)
{
	sceJpegDeleteMJpeg();
	sceJpegFinishMJpeg();
	sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
}

static void advancedBlit(int sx, int sy, int sw, int sh, int dx, int dy, int slice)
{
	int start, end;

	// blit maximizing the use of the texture-cache

	for (start = sx, end = sx+sw; start < end; start += slice, dx += slice)
	{
		struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
		int width = (start + slice) < end ? slice : end-start;

		vertices[0].u = start; vertices[0].v = sy;
		vertices[0].color = 0;
		vertices[0].x = dx; vertices[0].y = dy; vertices[0].z = 0;

		vertices[1].u = start + width; vertices[1].v = sy + sh;
		vertices[1].color = 0;
		vertices[1].x = dx + width; vertices[1].y = dy + sh; vertices[1].z = 0;

		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
	}
}

int render_init(void)
{
	// setup jpeg
	if (jpeg_init() != 0)
		return 1;

	// setup GU
	fbp0 = get_static_vram_buffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
	void* fbp1 = get_static_vram_buffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
	void* zbp = get_static_vram_buffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_4444);

	sceGuInit();

	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFFER_WIDTH);
	sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, fbp1, BUFFER_WIDTH);
	sceGuDepthBuffer(zbp, BUFFER_WIDTH);
	sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuFrontFace(GU_CW); // clockwise
	sceGuEnable(GU_TEXTURE_2D);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(1);
}

static int decode_jpeg_frame_to_screen(struct Jpeg_frame* frame)
{
	if (sceJpegDecodeMJpeg(frame->frame, frame->size, rgba, 0) < 0) {
		pspDebugScreenPrintf("sceJpegDeleteMJpeg() failed\n");
		return 1;
	}

	for (int i = 0; i < SCREEN_HEIGHT; i++)
		memcpy(&screen[i * BUFFER_WIDTH], &rgba[i * SCREEN_WIDTH],  SCREEN_WIDTH * 4);

	return 0;
}

void render_jpeg_frame(struct Jpeg_frame* frame)
{
	int err = decode_jpeg_frame_to_screen(frame);
	if (err != 0)
		return;

	sceGuStart(GU_DIRECT, list);

	// 4th arg ?
	sceGuTexMode(GU_PSM_8888, 0, 0, 0); // 32-bit RGBA
	sceGuTexImage(0, 512, 512, 512, screen); // setup texture as a 512x512 texture, even though the buffer is only 512x272 (480 visible)
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA); // don't get influenced by any vertex colors
	sceGuTexFilter(GU_NEAREST, GU_NEAREST); // point-filtered sampling

	advancedBlit(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 32);

	sceGuFinish();
	sceGuSync(0, 0);

	fbp0 = sceGuSwapBuffers();
}

void render_term(void)
{
	jpeg_term();
	sceGuTerm();
}
