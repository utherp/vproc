/*************************************************************************\
 *                                                                       *
 * VProc VPROC_MM  (Memory Management Unit)                              *
 *                                                                       *
 *    Base header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __VPROC_MM_H__
#define __VPROC_MM_H__
/* start of header */

/* core headers are included through sys headers */
#ifdef __NEED_VPROC_MM_CORE__
    #define __NEED_VPROC_MM_SYS__
#endif


/* vproc_mm pre-lib/sys/core common definitions */
#include <sys/types.h>


/* are sys/core headers needed: */
#ifdef __NEED_VPROC_MM_SYS__
   
    /* include sys headers */
    #include "sys/vproc_mm_sys.h"

#else 

    /* include lib headers */
    #include "lib/vproc_mm_lib.h"

#endif


/* VPROC_MM common definitions */

int vproc_mm_init ();

#define malloc(s) vproc_malloc(s)
#define calloc(c,s) vproc_calloc(c,s)
#define calloc_chain(c,s) vproc_calloc_chain(c,s)
#define realloc(p,s) vproc_realloc(p,s)
#define free(s) vproc_free(s)
    
void *vproc_malloc (size_t size);
void *vproc_calloc (size_t count, size_t size);
void *vproc_calloc_chain (size_t count, size_t size);
void *vproc_realloc (void *ptr, size_t size);
void vproc_free(void *ptr);
void *vproc_dup_segment (void *addr);
 
/* end of header */
#endif

