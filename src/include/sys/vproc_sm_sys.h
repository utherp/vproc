/*************************************************************************\
 *                                                                       *
 * VProc VPROC_SM                                                         *
 *                                                                       *
 *    SYS header file                                                    *
 *                                                                       *
\*************************************************************************/

#ifndef __VPROC_SM_SYS_H__
#define __VPROC_SM_SYS_H__
/* start of header */

/* this should have been included from 'vproc_sm.h', with __NEED_VPROC_SM_SYS__ or __NEED_VPROC_SM_CORE__ defined */
#ifndef __VPROC_SM_H__
    #error "Never include 'sys/vproc_sm_sys.h' directly!  Define __NEED_VPROC_SM_SYS__ and include '${UNITLC}.h'"
#endif

#include "vp_stream.h"

#ifdef __NEED_VPROC_SM_CORE__

    /* VPROC_SM CORE definitions */
    #include "core/vproc_sm_core.h"

#else

    /* VPROC_SM SYS specific definitions */


#endif


/* VPROC_SM common SYS/CORE definitions */




/* end of header */
#endif

