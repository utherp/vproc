/*************************************************************************\
 *                                                                       *
 * VProc VP_LOCK                                                         *
 *                                                                       *
 *    Base header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __VP_LOCK_H__
#define __VP_LOCK_H__
/* start of header */

/* core headers are included through sys headers */
#ifdef __NEED_VP_LOCK_CORE__
    #define __NEED_VP_LOCK_SYS__
#endif


/* VP_LOCK common definitions */

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
struct _vp_lock_s {
    pthread_mutex_t handle;
    pthread_t locker;
    int32_t count;
    /*******************************************
     * Do not call these handles directly... 
     * they are mearly for portability and the
     * ability to implement multiple lock types.
     * for porters or lock-type implementors:
     * the semantics of these functions should 
     * match those of 'fast' mutexes defined in
     * pthreads
     */
    int (*_lock)(void *handle);    /* secure lock */
    int (*_unlock)(void *handle);  /* unlock if locked by self */
    int (*_trylock)(void *handle); /* attempt to lock but don't block if locked by another */
};
typedef struct _vp_lock_s VP_LOCK;

typedef struct _vp_signal_s VP_SIGNAL;
struct _vp_signal_s {
    pthread_cond_t handle;
    VP_LOCK lock;
    int32_t count;
    /*******************************************
     * Do not call these handles directly... 
     * they are mearly for portability and the
     * ability to implement multiple lock types.
     * for porters or lock-type implementors:
     * the semantics of these functions should 
     * match those of 'fast' mutexes defined in
     * pthreads
     */
    int (*_one)(VP_SIGNAL *handle);    /* signal one instance waiting on signal */
    int (*_all)(VP_SIGNAL *handle);    /* signal all instances waiting on signal */
    int (*_wait)(VP_SIGNAL *handle, int msec);   /* wait on signal for at most msec milliseconds, or forever if 0 */
};


/* are sys/core headers needed: */
#ifdef __NEED_VP_LOCK_SYS__

    /* include sys headers */
    #include "sys/vp_lock_sys.h"

#else 

    /* include lib headers */
    #include "lib/vp_lock_lib.h"

#endif



/* end of header */
#endif

