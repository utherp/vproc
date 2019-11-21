/*************************************************************************\
 *                                                                       *
 * VProc ${UNIT}                                                         *
 *                                                                       *
 *    Base header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __${UNIT}_H__
#define __${UNIT}_H__
/* start of header */

/* core headers are included through sys headers */
#ifdef __NEED_${UNIT}_CORE__
    #define __NEED_${UNIT}_SYS__
#endif


/* ${UNIT} pre-lib/sys/core common definitions */


/* are sys/core headers needed: */
#ifdef __NEED_${UNIT}_SYS__

    /* include sys headers */
    #include "sys/${unit}_sys.h"

#else 

    /* include lib headers */
    #include "lib/${unit}_lib.h"

#endif


/* ${UNIT} post-lib/sys/core common definitions */



/* end of header */
#endif

