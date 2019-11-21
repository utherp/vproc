/*************************************************************************\
 *                                                                       *
 * VProc VPROC_SM                                                         *
 *                                                                       *
 *    Base header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __VPROC_SM_H__
#define __VPROC_SM_H__
/* start of header */

/* core headers are included through sys headers */
#ifdef __NEED_VPROC_SM_CORE__
    #define __NEED_VPROC_SM_SYS__
#endif

#include "vp_stream.h"

/* are sys/core headers needed: */
#ifdef __NEED_VPROC_SM_SYS__

    /* include sys headers */
    #include "sys/vproc_sm_sys.h"

#else 

    /* include lib headers */
    #include "lib/vproc_sm_lib.h"

#endif


/* VPROC_SM common definitions */

VP_STREAM *open_stream (const char *name, uint32_t flags);
VP_FEED   *open_feed (VP_STREAM *stream, const char *name, VP_FMT *fmt);
int vproc_sm_init();

/* end of header */
#endif

