/*************************************************************************\
 *                                                                       *
 * VProc VPROC_SM                                                         *
 *                                                                       *
 *    CORE header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __VPROC_SM_CORE_H__
#define __VPROC_SM_CORE_H__
/* start of header */

/* this should have been included from 'sys/vproc_sm_sys.h' via 'vproc_sm.h', with __NEED_VPROC_SM_CORE__ defined */
#ifndef __VPROC_SM_SYS_H__
    #error "Never include 'core/vproc_sm_core.h' directly!  Define __NEED_VPROC_SM_CORE__ and include '${UNITLC}.h'"
#endif


/* VPROC_SM core specific definitions */

#define SM_MAX_STREAMS 4



/* end of header */
#endif

