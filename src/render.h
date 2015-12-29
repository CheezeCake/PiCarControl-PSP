#ifndef _RENDER_H_
#define _RENDER_H_

#include "jpeg_frame.h"

#define BUFFER_WIDTH 512
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

int render_init(void);
void render_jpeg_frame(struct Jpeg_frame* frame);
void render_term(void);

#endif
