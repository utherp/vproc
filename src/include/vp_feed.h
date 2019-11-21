#ifndef __VP_FEED_H__
#define __VP_FEED_H__
/* start of header */

/* core headers are included through sys headers */
#ifdef __NEED_VP_FEED_CORE__
    #define __NEED_VP_FEED_SYS__
#endif


/* are sys/core headers needed: */
#ifdef __NEED_VP_FEED_SYS__

    /* include sys headers */
    #include "sys/vp_feed_sys.h"

#else 

    /* include lib headers */
    #include "lib/vp_feed_lib.h"

#endif


/**********************************************************************
 * The feed structure.. returned from a call to open_feed... it should,
 * for the most part, be treated like a FILE object would, just passed
 * to feed function calls, and left alone beyond that.
 */

struct _vp_feed_s {
    /* feed name */
    char name[32];

    /* pointer to currently aquired buffer segment */
    void *buffer;

    /* see Feed Flags above.*/
    struct {
        uint32_t local;     /* flags for local handle */
        uint32_t *global;   /* global feed flags (reference to local flags of owner's handle */
    } flags;

    /* pointer to stream which feed belongs to */
    VP_STREAM *stream;

    /* feed format */
    VP_FMT *format;

    /* feed control functions... */
    VP_FEED_CTRLS ctrl;

    /* private pointer, for implementors to use however they wish */
    void *priv;

    /******************************************************
     * internal data is beyond this point... but we don't
     * include it in this struct, because the implementers
     * should never touch it... the implementers should
     * also never try to make a duplicate of the structure,
     * the internal data would not be copied as it is
     * actually bigger than it appears... maybe I should
     * make this a union, adding a data padding... hrm.
     */

#if ! __NEED_VP_FEED_CORE__
    /******************************************************
     * this is just padding so everyone knows how big it
     * really is (although, implementors should never
     * copy vproc handles anyway...)
     */

    /* there are two 32 bit ints, and one 64 bit int,
     * thats 12 bytes, plus 3 pointers, these directives
     * make sure the right one is specified*/
    #if __SIZEOF_POINTER__ == 4
        char __core[28];    /* 3 pointers * 4 bytes each */
    #elif __SIZEOF_POINTER__ == 8
        char __core[40];
    #else
        #error "UNKNOWN POINTER SIZE!  Please define __SIZEOF_POINTER__ to the number of bytes a pointer uses"
    #endif

#else

    /******************************************************
     * internal data is beyond this point...  the following
     * members are not available to the implementors
     */

    /*****************************************************/
    uint32_t mode;                    
    /* permissions mode (see chmod(2)).  I put this here as
     * the first internal value on purpose, so in the case
     * an implementor accidently overwrites it (maybe using
     * an incorrect cast of an integer to the priv pointer of a 
     * different size, for example.  If this mode doesn't 
     * match that of the owner, we can know by checking that
     * an internal data change has happened 
     */

    /*****************************************************/
    uint32_t missed;
    /* number of readers which are blocked and awaiting a new
     * buffer to be ready (readers which are reading buffers faster
     * than they are completed. this value is returned by a
     * call to VP_FEED.ctrl.aquire and only applicable to writers. */

    /******************************************************/
    uint64_t last_id;
    /* Id of last buffer aquired. This is used to determine
     * the number of buffers which a reader has missed since
     * the last aquire and is only applicable to readers */

    /******************************************************/
    VP_SIGNAL *signal;
    /* signal is broadcast each time a buffer is completed.
     * when a reader calls to VP_FEED.ctrl.aquire but a new
     * buffer has not been completed since the last aquire
     * (i.e.: VP_FEED.set->buffers.pos.latest->id == VP_FEED.last_id )
     * and the reader is in BLOCKING mode (i.e. O_NONBLOCK flag
     * is *NOT* set), then VP_FEED.owner->missed is incremented,
     * and the reader blocks waiting for this signal
     */

    /******************************************************/
    VP_FEED *owner;
    /* pointer to the owner's VP_FEED handle, used internaly
     * to reference global feed flags and the "missed" counter
     * above
     */

    /******************************************************/
    MM_SET *set;
    /* pointer to the MM_SET handle for this feed.  Controlled
     * by the Memory Managment unit which manages the
     * buffers and is responsible for their referencing and
     * cycling.  (see include/sys/vproc_mm_sys.h for more
     * details).
     */
#endif

};


/* the rest of the header is common for lib, sys and core units */


#define current_buffer(feed)        feed->ctrl.current(feed)
#define aquire_buffer(feed,bufptr)  feed->ctrl.aquire(feed, bufptr)
#define release_buffer(feed)        feed->ctrl.release(feed)
#define close_feed(feed)            feed->ctrl.close_feed(feed)
#define get_feed_format(feed)       feed->ctrl.get_format(feed)

int vp_feed_frame_sz (VP_FEED *feed);
VP_FEED *create_feed (const char *name, VP_FMT *format);

int vp_feed_set_format (VP_FEED *feed, VP_FMT *fmt);
VP_FMT *vp_feed_get_format (VP_FEED *feed);
int vp_feed_aquire_buffer (VP_FEED *feed, void **buf);
int vp_feed_release_buffer (VP_FEED *feed);
void *vp_feed_current_buffer (VP_FEED *feed);
int vp_feed_close (VP_FEED *feed);
VP_FEED *reopen_feed (VP_FEED *feed);

/* end of header */
#endif

