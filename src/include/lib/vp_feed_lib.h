/*************************************************************************\
 *                                                                       *
 * VProc VP_FEED                                                         *
 *                                                                       *
 *    LIB header file                                                    *
 *                                                                       *
\*************************************************************************/

#ifndef __VP_FEED_LIB_H__
#define __VP_FEED_LIB_H__


/* VP_FEED LIB specific definitions */
#include "vp_fmt.h"
#include "vp_buf.h"
typedef struct _vp_feed_s VP_FEED;
#include "vproc_sm.h"

#define __CTRLS_CONST__ const



/**************************************************************
 * VP_FEED_CTRLS:
 *   This structure defines the callbacks for interacting
 *   with the feed.  It is contained within the VP_FEED
 *   structure below.
 */
typedef struct _vproc_feed_ctrl_s {
    /* NOTE: the pointers are constant as we don't want the implementers changing them */


    /********************************************************************************
     *  set_format: 
     *    sets the feed format to format_ptr, as returned by a
     *    call to create_format, or resolve_format. This not 
     *    only communicates the format of the feed, but based on 
     *    the type and parameters set can be used as an optimization
     *    hint to the Memory Manager and Feed Manager.
     */
    int (* __CTRLS_CONST__ set_format)(VP_FEED *feed, VP_FMT *format_ptr);


    /********************************************************************************
     *  get_format:
     *    returns a copy of the format reference which was
     *    set using set_format above, or NULL if none has
     *    been set
     */
    VP_FMT *(* __CTRLS_CONST__ get_format) (VP_FEED *feed);


    /********************************************************************************
     * aquire_buffer:
     *
     *   If called by a reader:
     *
     *   Aquires the buffer in the "latest"
     *   buffer position.  If a buffer is already aquired, it is
     *   auto-released.
     *
     *   If no new buffer is available and the O_NONBLOCK feed flag 
     *   is set, -1 is returned and errno is set to EWOULDBLOCK. 
     *
     *   If no new buffer is available and the feed IS blocking, 
     *   then the caller's thread will block until:
     *      - a new buffer becomes available, at which time 0 is returned
     *    * or *
     *      - the feed has been closed, at which time errno is set
     *      to EIDRM and -1 is returned.
     *
     *
     *   If called by a writer:
     *
     *   Aquires the buffer in the "next" position and places it in
     *   the "active" position.  If a buffer is already in the "active"
     *   position, it is auto-released, (see release_buffer below),
     *   and moved into the "latest" position.
     *
     *   NOTE: based on the semantics of the Memory Manager, a
     *   call to aquire_buffer by a writer should *NEVER* block.
     *
     *   returns a reference to the latest completed buffer for this feed.
     *   If a buffer is currently aquired, it will be released prior to aquiring 
     *   the latest buffer.
     *
     *   If a new buffer is available, it is set in 'buf' and the number 
     *   returned is a count of read misses (number of buffers completed and
     *   recycled since last aquire).
     *
     */
    int (* __CTRLS_CONST__ aquire)(VP_FEED *feed, void **buf);


    /********************************************************************************
     *  release_buffer:
     *    Release the currently aquired buffer. 
     *    Use this if you won't be aquiring a new buffer right away, so the
     *    Memory Manager can release it and possibly return it to the pool.
     *
     *    If called by a writer, it marks the "active" buffer as complete,
     *    and moves it to the "latest" buffer position.  The "next" buffer
     *    will be moved to the "active" position on the next call to 
     *    aquire_buffer.
     *
     *    If you are going to aquire a new buffer right away, skip this call,
     *    as a call to aquire_buffer will check and release any buffer which
     *    is currently aquired.
     *
     *    return 0 on success, or -1 on error and errno will be set.
     *
     */
    int (* __CTRLS_CONST__ release)(VP_FEED *feed);


    /********************************************************************************
     *  current_buffer:
     *    Return reference to the currently aquired buffer, or NULL if non aquired
     */
    void *(* __CTRLS_CONST__ current)(VP_FEED *feed);


    /********************************************************************************
     * close_feed:
     *   Close the feed.
     *
     *   If closed by a reader, the Memory Manager will mark a spare buffer
     *   in the free pool as reclaimable (semantics defined in Memory Manager docs).
     *
     *   If closed by a writer, the feed will be marked as removed.  Any subsequent
     *   calls to aquire_frame on this feed will return -1 and set errno to EIDRM.
     *   The buffer pool itself will be marked as removed.  When all buffers have
     *   been returned and set reclaimable, then the feed and all its remaining
     *   resources will be released.
     */
    int (* __CTRLS_CONST__ close_feed)(VP_FEED *feed);


    /* more calls to come */
} VP_FEED_CTRLS;


/* end of header */
#endif

