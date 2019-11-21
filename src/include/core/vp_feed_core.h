#ifndef __VP_FEED_CORE_H__
#define __VP_FEED_CORE_H__

/****************************************************************
 * Vproc Feed Manager's internal structure and function defs.
 * This header should *only* be included by vp_feed.h, and only
 * if __NEED_VP_FEED_CORE__ is defined 
 */
#ifndef __VP_FEED_SYS_H__
    #error "You should NEVER include core/vp_feed_core.h directly! Define __NEED_VP_FEED_CORE__ and include 'vp_feed.h' instead"
#endif

typedef struct _vp_feed_core_s VP_FEED;

#include "vp_fmt.h"
#include "vp_stream.h"
#define __NEED_VPROC_MM_SYS__
#include "vproc_mm.h"


struct _vp_feed_ctrls_core_s {
    int     (*set_format)     (VP_FEED *feed, VP_FMT *fmt);
    VP_FMT *(*get_format)     (VP_FEED *feed);
    int     (*aquire)         (VP_FEED *feed, void **buf);
    int     (*release)        (VP_FEED *feed);
    void   *(*current)        (VP_FEED *feed);
    int     (*close_feed)     (VP_FEED *feed);
    /* more calls to come */
};
typedef struct _vp_feed_ctrls_core_s VP_FEED_CTRLS;


/* core feed definition */
struct _vp_feed_core_s {
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
     * internal data is beyond this point...  the following
     * members are not available to the implementors
     */

    /*****************************************************/
    int mode;                    
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
    double last_id;
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
     * by the Memory Managment unit which is contains the
     * buffers and is responsible for their referencing and
     * cycling.  (see include/sys/vproc_mm_sys.h for more
     * details).
     */

};


/* end if header */
#endif

