#define __VPROC_SYS__ 1
#define __NEED_VP_FMT_CORE__ 1
#include "vproc.h"
#include "vp_feed.h"
#include "vp_fmt.h"
#include <string.h>

#define VP_FM_MAX_FORMATS 64

static VP_FMT *formats = NULL;

int vproc_fm_init () {
    return 0;
}
    
VP_FMT *create_format (const char *name, unsigned int width, unsigned int height, uint8_t channels, uint8_t bpp) {
    VP_FMT *format = calloc(1, sizeof(VP_FMT));
    vp_mm_chain_push(&formats, format);
    strncpy(format->name, name, 15);
    format->width = width;
    format->height = height;
    format->channels = channels;
    format->bpp = bpp;
    format->pixels = (width * height);
    format->size = format->pixels * bpp / 8;
    return format;
}

