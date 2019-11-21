/*************************************************************************\
 *                                                                       *
 * VProc ${UNIT}                                                         *
 *                                                                       *
 *    CORE header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __${UNIT}_CORE_H__
#define __${UNIT}_CORE_H__
/* start of header */

/* this should have been included from 'sys/${unit}_sys.h' via '${unit}.h', with __NEED_${UNIT}_CORE__ defined */
#ifndef __${UNIT}_SYS_H__
    #error "Never include 'core/${unit}_core.h' directly!  Define __NEED_${UNIT}_CORE__ and include '${UNITLC}.h'"
#endif


/* ${UNIT} core specific definitions */



/* end of header */
#endif

