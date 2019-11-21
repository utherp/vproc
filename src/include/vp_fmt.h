#ifndef __VP_FMT_H__
#define __VP_FMT_H__

/*******************************************************************
 * VP_FMT is just wrapper to hold the type, which is public, and
 * needs to be know by other areas of the vproc engine which doesn't
 * care about the private internals.  This is returned when a format
 * is resolved or created... think of it like the FILE pointer you
 * get back from fopen... it IS a structure, but nothing the implementer
 * needs to know about
 */

/*********** vproc_feed_format **************
 * A union of the various data formats
 * for which a feed can manage. Optimizating
 * buffer handling can be greatly improved
 * using defined feed formats
 * ... this is a todo
 */

struct _vp_fmt_s {
    char name[16];
    uint32_t id;
    uint32_t flags;
    uint8_t channels;
    uint8_t bpp;
    uint16_t width;
    uint16_t height;
    uint32_t pixels;
    uint32_t size;
};
typedef struct _vp_fmt_s VP_FMT;

VP_FMT *create_format (const char *name, unsigned int width, unsigned int height, uint8_t channels, uint8_t bpp);

#endif
