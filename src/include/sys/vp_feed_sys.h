#ifndef __VP_FEED_SYS_H__
#define __VP_FEED_SYS_H__

/****************************************************************
 * Vproc Feed Manager's internal structure and function defs.
 * This header should *only* be included by vp_feed.h, and only
 * if __NEED_VP_FEED_SYS__ is defined 
 */
#ifndef __VP_FEED_H__
    #error "You should NEVER include sys/vp_feed_sys.h directly! Define __NEED_VP_FEED_SYS__ and include 'vproc.h' instead"
#endif


#ifndef __NEED_VP_FEED_CORE__
    /*
     * the internal is needed, but not the core,
     * currently there is no internal definition 
     * for feeds... so we'll re-include the implementor's
     * version
     */
    #include "lib/vp_feed_lib.h"
#else
    /* the core is needed, not the internal */
    #include "core/vp_feed_core.h"
#endif


/* end if header */
#endif 

