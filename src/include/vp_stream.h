#ifndef __VP_STREAM_H__
#define __VP_STREAM_H__

/* core headers are included through sys headers */
#ifdef __NEED_VP_STREAM_CORE__
    #define __NEED_VP_STREAM_SYS__
#endif

/* are sys/core headers needed: */
#ifdef __NEED_VP_STREAM_SYS__

    /* include sys headers */
    #include "sys/vp_stream_sys.h"

#else 

    /* include lib headers */
    #include "lib/vp_stream_lib.h"

#endif


/* the rest of the header is common for lib, sys and core units */

#endif
