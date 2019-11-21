#define __VPROC_SYS__ 1
#define __NEED_VPROC_SM_CORE__ 1
#define __NEED_VP_STREAM_SYS__ 1
#include "vproc.h"
#include <xmmintrin.h>
#include <string.h>
#include <fcntl.h>

static uint16_t sm_stream_count = 0;
static VP_STREAM *sm_streams[SM_MAX_STREAMS];
static VP_STREAM *sm_free_chain = NULL;

/**************************************************************/

static VP_STREAM *find_stream (const char *name);
static VP_STREAM *create_stream (const char *name, uint32_t flags);

/**************************************************************/

int vproc_sm_init () {

    /* allocate blank vp_stream buffers and chain them together */
    sm_free_chain = calloc_chain(SM_MAX_STREAMS, sizeof(VP_STREAM));

    /* clear stream pointers */
    int i;
    for (i=0; i<SM_MAX_STREAMS; sm_streams[i++] = NULL);

    return 0;
}

/**************************************************************/

VP_STREAM *open_stream (const char *name, uint32_t flags) {
    VP_STREAM *stream = find_stream(name);
    if (stream) {
        /* existing stream found */
        if (flags & (O_CREAT | O_EXCL)) {
            /* stream exists, but caller insists it wanted to create it */
            errno = EEXIST;
            return NULL;
        }
        return stream;
    }

    /* no stream exists with name */
    if (!(flags & O_CREAT)) {
        /* not creating stream */
        errno = ENOENT;
        return NULL;
    }

    /* create stream */
    stream = create_stream(name, flags);
    return stream;
}

/**************************************************************/

static VP_STREAM *find_stream (const char *name) {
    /* Find an existing stream by name... this needs indexing, but it'll do for now */

    int i;
    for (i = 0; i < sm_stream_count; i++) {
        if (!strncmp(sm_streams[i]->name, name, 31)) 
            /* found a match! */
            return sm_streams[i];
    }

    /* not found */
    return NULL;
}

/**************************************************************/

static VP_STREAM *create_stream (const char *name, uint32_t flags) {
    VP_STREAM *stream = vp_mm_chain_pop((void**)&sm_free_chain);
    if (!stream) {
        /* no vp_stream buffers left in free chain... must have exceeded SM_MAX_STREAMS */
        errno = ENOMEM;
        return NULL;
    }

    sm_streams[sm_stream_count++] = stream;

    strncpy(stream->name, name, 31);
    stream->name[31] = '\0';

    stream->count.feeds = stream->count.formats = 0;
    stream->feeds = NULL;

    return stream;
}

/**************************************************************/

VP_FEED *open_feed (VP_STREAM *stream, const char *name, VP_FMT *fmt) {
    VP_FEED *feed = stream->feeds;

    while (feed != NULL) {
        if (!strncmp(feed->name, name, 31)) break;
        feed = vp_mm_chain_next(feed);
    }

    if (!feed) {
        /* existing named feed not found in stream, creating... */
        feed = create_feed(name, fmt);
        vp_mm_chain_push(&(stream->feeds), feed);
    } else {
        /* feed found, making feed ref... */
        feed = reopen_feed(feed);
    }

    return feed;
}



