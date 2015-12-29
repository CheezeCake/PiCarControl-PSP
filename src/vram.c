#include <pspgu.h>

#include "vram.h"

// taken from pspsdk samples

static unsigned int static_offset = 0;

static unsigned int get_memory_size(unsigned int width, unsigned int height, unsigned int psm)
{
	switch (psm) {
		case GU_PSM_T4:
			return (width * height) >> 1;

		case GU_PSM_T8:
			return width * height;

		case GU_PSM_5650:
		case GU_PSM_5551:
		case GU_PSM_4444:
		case GU_PSM_T16:
			return 2 * width * height;

		case GU_PSM_8888:
		case GU_PSM_T32:
			return 4 * width * height;

		default:
			return 0;
	}
}

void* get_static_vram_buffer(unsigned int width, unsigned int height, unsigned int psm)
{
	unsigned int mem_size = get_memory_size(width, height, psm);
	void* result = (void*)static_offset;
	static_offset += mem_size;

	return result;
}
