/*************************************************************************\
 *                                                                       *
 * VProc ${UNIT}                                                         *
 *                                                                       *
 *    SYS header file                                                    *
 *                                                                       *
\*************************************************************************/

#ifndef __${UNIT}_SYS_H__
#define __${UNIT}_SYS_H__
/* start of header */

/* this should have been included from '${unit}.h', with __NEED_${UNIT}_SYS__ or __NEED_${UNIT}_CORE__ defined */
#ifndef __${UNIT}_H__
    #error "Never include 'sys/${unit}_sys.h' directly!  Define __NEED_${UNIT}_SYS__ and include '${UNITLC}.h'"
#endif


/* $(UNIT) pre-CORE common SYS/CORE definitions */


#ifdef __NEED_${UNIT}_CORE__

    /* ${UNIT} CORE definitions */
    #include "core/${unit}_core.h"

#else

    /* ${UNIT} SYS specific definitions */


#endif


/* ${UNIT} post-CORE common SYS/CORE definitions */



/* end of header */
#endif

