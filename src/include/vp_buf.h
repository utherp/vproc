#ifndef __VP_BUF_H__
#define __VP_BUF_H__

/***********************************************************
 * This is the "public" header for vproc's buffers and
 * buffer pools.  The internals (used only by vproc's Memory
 * Manager) are defined in 'internals/buffer.h'.
 *
 * TODO:  define VP_BUF and controls!
 *
 */

#ifdef __NEED_VP_BUF_CORE__
#define __NEED_VP_BUF_SYS__
#endif

#include "vp_lock.h"

#ifdef __NEED_VP_BUF_SYS__

    #include "sys/vp_buf_sys.h"

#else 

    struct _vp_buf_s {
        void *id;
        uint64_t seq_id;
        VP_LOCK lock;
        uint32_t size;
        int32_t refs;
    };
    typedef  struct _vp_buf_s VP_BUF;

#endif


/* end of header */
#endif

