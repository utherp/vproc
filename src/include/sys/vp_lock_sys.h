/*************************************************************************\
 *                                                                       *
 * VProc VP_LOCK                                                         *
 *                                                                       *
 *    SYS header file                                                    *
 *                                                                       *
\*************************************************************************/

#ifndef __VP_LOCK_SYS_H__
#define __VP_LOCK_SYS_H__
/* start of header */

/* this should have been included from 'vp_lock.h', with __NEED_VP_LOCK_SYS__ or __NEED_VP_LOCK_CORE__ defined */
#ifndef __VP_LOCK_H__
    #error "Never include 'sys/vp_lock_sys.h' directly!  Define __NEED_VP_LOCK_SYS__ and include '${UNITLC}.h'"
#endif


#ifdef __NEED_VP_LOCK_CORE__

    /* VP_LOCK CORE definitions */
    #include "core/vp_lock_core.h"

#else

    /* VP_LOCK SYS specific definitions */


    /* create a fast mutex lock, and monitor count manually (this is more portable) */
#define vp_lock_init(l) \
        do { \
            (l)->handle = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; \
            (l)->_lock   = (int(*)(void*))pthread_mutex_lock;    \
            (l)->_unlock = (int(*)(void*))pthread_mutex_unlock;  \
            (l)->_trylock= (int(*)(void*))pthread_mutex_trylock; \
            (l)->locker = 0; \
            (l)->count = 0; \
        } while(0)
    
    
    /* get thread id */
    #define vproc_id pthread_self
    
    /**************************************************
     * vproc_lock:
     *   this macro secures a lock not already secured 
     *   and sets counter to 1, or increments counter 
     *   for a lock which was already secured by caller
     */
    #define vproc_lock(l) \
        do { \
            /* secure lock only if not already secured by caller */ \
            if (l->locker != vproc_id()) { \
                _debugger("LOCK", "--> Not owned by caller, locking...", 0); \
                l->_lock(l);              /* secure lock */ \
                l->locker = vproc_id();  /* set locker id to caller */ \
                l->count = 0;            /* reset counter */ \
            } \
            l->count++;  /* increment counter */ \
        } while(0)
    
    
    /**************************************************
     * vproc_unlock:
     *   this macro decrements the lock counter and
     *   releases the lock if the counter reaches 0;
     *   ONLY IF the caller is the one who secured the 
     *   lock, otherwize it does nothing.
     */
    #define vproc_unlock(l) \
        do { \
            /* unlock if caller is locker and decremented counter reaches 0 */  \
            if (l->locker && l->locker == vproc_id() && !--l->count) { \
                _debugger("LOCK", "--> Locked and owned by caller, unlocking...", 0); \
                l->locker = 0;   /* clear locker id */ \
                l->_unlock(l);    /* remove lock */ \
            } \
        } while(0)
    
    /**************************************************
     * vproc_dlock:
     *   This macro will secure the lock for the caller,
     *   then lock it again, causing a dead-lock, which
     *   is normally prevented by the semantics of
     *   vproc_lock.  This is useful if you want to
     *   block the caller until some other thread
     *   releases it by calling vproc_poplock.
     */
    #define vproc_dlock(l) \
        do { \
            /* do the initial lock if its not already locked, or locked by another thread */ \
            if (!l->locker || !l->count || l->locker != vproc_id()) vproc_lock(l); \
            l->_lock(l);              /* deadlock */ \
            l->locker = vproc_id();  /* set locker's id  */ \
            l->count = 1;            /* set counter to 1 */ \
        } while (0)
    
    /**************************************************
     * vproc_poplock:
     *   If the lock is secured by ANYONE, then it is
     *   released by the caller, the locker is cleared
     *   and the counter is reset.  Useful for resuming
     *   a thread which is dead-locked by a call to
     *   vproc_dlock on this lock
     */
    #define vproc_poplock(l) \
        do { \
            if (l->locker && l->count) { \
                l->count = 0;  /* reset count     */ \
                l->locker = 0; /* clear locker id */ \
                l->_unlock(l);  /* release lock    */ \
            } \
        } while (0)
    
    
    /**************************************************
     * vproc_trylock:
     *   If the lock is not secured, it will secure it 
     *   with vproc_lock.  If the lock is secured by
     *   another thread, then it returns EBUSY. 
     *   If the caller is the one who had previously
     *   secured the lock then, assuming this function
     *   would not have been called by a thread who
     *   KNEW that it owned the lock already, the lock's
     *   count is reduced to 1.
     *   this is a function instead of a macro as
     *   a return value is potentially needed.
     */
    static inline int vproc_trylock(VP_LOCK *l)  {
        if (!l->locker) {  
            /* not secured */
            vproc_lock(l);   /* secure lock, set locker id, reset counter */
            return 0;
        }
        if (l->locker == vproc_id()) {
            /* secured by caller */
            l->count = 1;    /* reset counter to 1 */
            return 0;
        }
    
        /* secured by another thread */
        return EBUSY;
    }
    

#define vp_signal_init(s) \
    do {                                     \
        vp_lock_init(&s->lock);            \
        pthread_cond_init((pthread_cond_t*)s, NULL); \
        s->count = 0;                        \
        s->_one = __vp_signal_one;         \
        s->_all = __vp_signal_all;           \
        s->_wait = __vp_signal_wait;         \
    } while(0)

#define vproc_signal(s) s->_one(s)
#define vproc_signal_all(s) s->_all(s)
#define vproc_signal_wait(s,t) s->_wait(s,t)

    static inline int __vproc_signal (VP_SIGNAL *sig, int all) {
        VP_LOCK * restrict slock = &sig->lock;
        int * restrict count = &sig->count;
        if (!*count) return 0;

        int ret = *count;

        _debugger("SIG", "Sending %s signal to 0x%X", all?"one":"broadcast", sig);
        
        if (slock->locker != vproc_id()) {
            /* caller must have lock before signal */
            vproc_lock(slock);
            slock->count = 1;
        }

        if (all) pthread_cond_broadcast((pthread_cond_t*)sig);
        else pthread_cond_signal((pthread_cond_t*)sig);

        /* unlock mutex and let signaled instances run */
        vproc_unlock(slock);

        /* when signaling all, return number of instances that
         * were waiting, when signaling one, return number of
         * instances still waiting */
        if (!all) ret--;
        return ret;
    }


    static inline int __vp_signal_one(VP_SIGNAL *sig) { return __vproc_signal(sig, 0); }
    static inline int __vp_signal_all(VP_SIGNAL *sig) { return __vproc_signal(sig, 1); }

    static inline int __vp_signal_wait (VP_SIGNAL *sig, int msec) {
        VP_LOCK * restrict slock = &sig->lock;
        int * restrict count = &sig->count;
        int ret = 0;
        if (slock->locker != vproc_id()) {
            /* caller must have lock before wait */
            vproc_lock(slock);
        }
        /**********************************************
         * lock must be released after wait, since
         * pthread_cond_wait releases the mutex 
         * automatically, we must set the count to 0 
         * before the call
         * */
        slock->count = 0;

        /* increase count of waiting instances on signal */
        (*count)++;

        _debugger("SIG", "Waiting on signal 0x%X...", sig);
        /* wait on signal ... */
        if (!msec) 
            pthread_cond_wait((pthread_cond_t*)sig, (pthread_mutex_t*)slock);
        else {
            _debugger("SIG", "--> waiting for %u milliseconds max...", msec);
            struct timeval now;
            struct timespec to;
            gettimeofday(&now, NULL);
            to.tv_sec = now.tv_sec;
            if (msec > 1000000) {
                to.tv_sec += (msec / 1000000);
                msec %= 1000000;
            }
            to.tv_nsec = (now.tv_usec + msec) * 1000;

            if (pthread_cond_timedwait((pthread_cond_t*)sig, (pthread_mutex_t*)slock, &to)) {
                _debugger("SIG", "--> Timed out waiting for signal!", 0);
                ret = -1;
            } else {
                _debugger("SIG", "--> received signal before timeout...", 0);
            }
        }

        /**********************************************
         * if we get here, then according to pthread
         * spec we definatly have the mutex lock, so 
         * we'll force an unlock 
         */
        slock->locker = vproc_id();
        slock->count = 1;
        vproc_poplock(slock);
//        vproc_unlock(slock);

        return 0;

    }

#endif


/* VP_LOCK common SYS/CORE definitions */




/* end of header */
#endif

