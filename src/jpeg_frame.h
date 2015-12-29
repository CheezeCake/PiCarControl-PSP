#ifndef _JPEG_FRAME_H_
#define _JPEG_FRAME_H_

#include <stdlib.h>
#include <string.h>

struct Jpeg_frame
{
	char* frame;
	size_t size;
};

static inline void jpeg_frame_init(struct Jpeg_frame* frame)
{
	memset(frame, 0, sizeof(struct Jpeg_frame));
}

static inline void jpeg_frame_destroy(struct Jpeg_frame* frame)
{
	if (frame)
		free(frame->frame);
}

#endif
