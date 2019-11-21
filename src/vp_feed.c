
/****************************************************************
 * Feed flags: 
 *   logically ORd values making up vproc_feed_private->flags
 *   TODO: define more flags 
 */

#define VP_FEED_FMT_CHANGED  (1<<8)  /* signifies the format of the feed has changed */
#define VP_FEED_POOL_CHANGED (1<<9) /* signifies the buffer pool of the feed has changed */

#define __VPROC_SYS__ 1
#define __NEED_VP_FEED_CORE__ 1
#define __NEED_VP_BUF_SYS__ 1
#define __NEED_VP_LOCK_SYS__ 1 
#define FEED_CLOSING 4
#include "vproc.h"
#include <fcntl.h>

#define vp_feed_init_reader_ctrls(f) { \
    f->ctrl.set_format    = NULL;      \
    f->ctrl.get_format    = vp_feed_get_format;      \
    f->ctrl.aquire        = vp_feed_aquire_buffer;   \
    f->ctrl.release       = vp_feed_release_buffer;  \
    f->ctrl.current       = vp_feed_current_buffer;  \
    f->ctrl.close_feed    = vp_feed_close;           \
}

#define vp_feed_init_writer_ctrls(f) { \
    f->ctrl.set_format    = vp_feed_set_format;      \
    f->ctrl.get_format    = vp_feed_get_format;      \
    f->ctrl.aquire        = vp_feed_aquire_buffer;   \
    f->ctrl.release       = vp_feed_release_buffer;  \
    f->ctrl.current       = vp_feed_current_buffer;  \
    f->ctrl.close_feed    = vp_feed_close;           \
}

#include <string.h>


int vp_feed_set_format (VP_FEED *feed, VP_FMT *fmt) {
    _debugger("SM", "Setting format for feed '%s'...", feed->name);
    return 0;
}
VP_FMT *vp_feed_get_format (VP_FEED *feed) {
    _debugger("SM", "Getting format for feed '%s'...", feed->name);
    return NULL;
}
int vp_feed_aquire_buffer (VP_FEED *feed, void **buf_ref) {
    int ret = 0;

    if (feed->buffer) ret = release_buffer(feed);
    _debugger("SM", "Aquiring buffer for feed '%s'...", feed->name);
    VP_BUF * restrict buf;
    if (feed == feed->owner) {
        _debugger("SM", "--> Aquiring active buffer..", 0);
        buf = feed->set->buffers.pos.active;
        buf->seq_id = feed->last_id++;
    } else {
        if (*feed->flags.global & FEED_CLOSING) {
            _debugger("SM", "--> feed is closing!", 0);
            *buf_ref = NULL;
            return -1;
        }
        uint64_t last_id = feed->last_id;
        _debugger("SM", "--_> Aquiring latest buffer...", 0);
        buf = feed->set->buffers.pos.latest;
        while (!buf || (last_id == buf->seq_id)) {
            /* new frame not ready */
            _debugger("SM", "NOTE: No new frame available yet...", 0);
            if (vproc_signal_wait(feed->signal, 1000)) {
                _debugger("SM", "NOTE: Timeout while aquiring buffer...",0);
            }
            if (*feed->flags.global & FEED_CLOSING) {
                _debugger("SM", "NOTE: Feed closed while waiting for signal!", 0);
                return -1;
            }
            buf = feed->set->buffers.pos.latest;
        }
        ret = 1;
    }
    ref_buffer(buf);
    feed->buffer = buffer_data_offset(buf);
    *buf_ref = feed->buffer;
    _debugger("SM", "--> buffer's new ref count: %d", buf->refs);
    return ret;
}

int vp_feed_release_buffer (VP_FEED *feed) {
    _debugger("SM", "Releasing buffer for feed '%s'...", feed->name);
    if (!feed->buffer) return 0;
    int ret = 0;
    VP_BUF *buf = buffer_header_offset(feed->buffer);
    if (feed->owner != feed)
        feed->last_id = buf->seq_id;
    feed->buffer = NULL;
    unref_buffer(buf);
    if (buf == feed->set->buffers.pos.active) {
        _debugger("SM", "Completed active buffer, cycling buffer positions..", 0);
        cycle_buffer_states(feed->set);
        /* signal any instances which may be waiting on a new buffer */
        ret = vproc_signal_all(feed->signal);
    }
    _debugger("SM", "--> buffer's new ref count: %d", buf->refs);
    return ret;
}
void *vp_feed_current_buffer (VP_FEED *feed) {
    _debugger("SM", "Getting current buffer for feed '%s'...", feed->name);
    return feed->buffer; //feed->pool->of.pool.by_pos.active;
}


int vp_feed_close (VP_FEED *feed) {
    _debugger("SM", "Closing feed '%s'...", feed->name);
    void *tmp;
    feed->flags.local |= FEED_CLOSING;
    if (feed->owner == feed) {
        _debugger("SM", "destroying signaler...", 0);
        while (pthread_cond_destroy((pthread_cond_t*)feed->signal)) {
            _debugger("SM", "--> still waiting threads on feed...", 0);
            vproc_unlock((&feed->signal->lock));
            vproc_signal_all(feed->signal);
            usleep(1000);
        }
    } else {
        /* remove one buffer from the pool */
        if (!feed->buffer) {
            feed->last_id = 1;
            aquire_buffer(feed, &tmp);
            if (!feed->buffer) {
                /* try again, with one less than last id which must have matched latest seq id */
                aquire_buffer(feed, &tmp);
            }
        }
        /******************************************************
         * clear buffer's set reference, so when it is released
         * and its reference count reaches 0, it will be freed
         * back to the MM
         */
        VP_BUF *buf = buffer_header_offset(feed->buffer);
        feed->buffer = NULL;
        buf->id = NULL;
        unref_buffer(buf);
        free(feed);
    }
    return 0;
}


int vp_feed_frame_sz (VP_FEED *feed) {
    _debugger("SM", "Getting frame size for feed '%s' (format: '%s' [%u])...", feed->name, feed->format->name, feed->format->size);
    return feed->format->size;
    return 32*1024;
}

VP_FEED *reopen_feed (VP_FEED *feed) {
    _debugger("SM", "Opening existing feed '%s'", feed->name);
    VP_FEED *new_feed = vproc_dup_segment(feed);
    new_feed->buffer = NULL;
    new_feed->priv = NULL;
    new_feed->missed = 0;
    new_feed->mode = O_RDONLY;
    new_feed->last_id = 0;
    add_buffer_to_free_chain(new_feed->set);
    return new_feed;
}


VP_FEED *create_feed (const char *name, VP_FMT *format) {
    _debugger("SM", "Creating feed '%s'...", name);

    VP_FEED *feed = calloc(1, sizeof(VP_FEED));
    strncpy(feed->name, name, 31);

    vp_feed_init_writer_ctrls(feed);

    feed->signal = calloc(1, sizeof(VP_SIGNAL));
    vp_signal_init(feed->signal);

    feed->format = format;

    _debugger("SM", "--> calling to create mmset of type POOL", 0);
    feed->set = create_mm_set(MM_SETT_POOL, feed);

    feed->flags.global = &(feed->flags.local);

    feed->owner = feed;
    feed->mode = O_RDWR;
    return feed;
}


