/*************************************************************************\
 *                                                                       *
 * VProc VPROC_MM                                                         *
 *                                                                       *
 *    CORE header file                                                   *
 *                                                                       *
\*************************************************************************/

#ifndef __VPROC_MM_CORE_H__
#define __VPROC_MM_CORE_H__
/* start of header */

/* this should have been included from 'sys/vproc_mm_sys.h' via 'vproc_mm.h', with __NEED_VPROC_MM_CORE__ defined */
#ifndef __VPROC_MM_SYS_H__
    #error "Never include 'core/vproc_mm_core.h' directly!  Define __NEED_VPROC_MM_CORE__ and include '${UNITLC}.h'"
#endif


/* VPROC_MM core specific definitions */


#define _XOPEN_SOURCE 600
#include <stdlib.h>

#ifndef __NEED_VP_BUF_CORE__
    #define __NEED_VP_BUF_CORE__
#endif
#ifndef __NEED_VP_LOCK_SYS__
    #define __NEED_VP_LOCK_SYS__
#endif
#include "vp_buf.h"
#include "vp_lock.h"


#define segment_addr(ptr) ((memseg*)(((char*)ptr) - sizeof(memseg)))
#define segment_offset(ptr) ((void*)(((char*)ptr) + sizeof(memseg)))

/* from vproc_mm.c */
inline void vp_mm_link_chain (void *addr1, void *addr2);
inline void vp_mm_unchain (void *addr);
inline void *vp_mm_next_in_chain (void *addr);

inline void vp_mm_chain_push (void **chain, void *addr);
inline void *vp_mm_chain_pop (void **chain);
inline void *vp_mm_chain_shift (void **chain);
inline void vp_mm_chain_unshift (void **chain, void *addr);
inline void *vp_mm_chain_next (void *addr);

typedef struct memseg_s {
    uint16_t flags;
    uint16_t poolid;
    struct memseg_s *next;
} memseg;


typedef struct mempool_s {
    uint16_t id;
    uint32_t count;
    uint32_t pages;
    uint32_t used;
    void *base_addr;
    VP_LOCK lock;
    memseg *free;
} mempool;

#define MM_PAGE_SIZE 16
#define MM_PAGE_BITS 4
#define MM_PAGE_MASK 0x0F

#define page_count(s) ( (s >> MM_PAGE_BITS) + ((s & MM_PAGE_MASK)?1:0) )

#define MM_SEGFL_ALLOC 1

#define set_seg_alloc(s)   s->flags |= MM_SEGFL_ALLOC
#define unset_seg_alloc(s) s->flags &= ~MM_SEGFL_ALLOC
#define is_seg_alloc(s) (s->flags & MM_SEGFL_ALLOC)


#define MM_POOL_COUNT 7
#define MM_POOL_SEG_PAGES  {    32,    64,    256,   1024,    8192,     24576,  128000 } 
                        /*              * MM_PAGE_SIZE                                    */
                        /*   =========================================================    */
                        /*     512,  1024,    4096, 16384,  131072,    393216, 2048000    */
                        /*              * Page counts...                                  */
#define MM_POOL_SEG_COUNTS {   128,    64,     64,     64,      32,        32,       8 }
// #define MM_POOL_SEG_COUNTS {    32,    32,     32,     16,      16,        32,       2 }
                        /*   =========================================================    */
                        /*   16384, 32768, 131072, 524288, 2097152,   6291456, 4096000    */
                        /*   ---------------------------------------------------------    */
                        /*   Total:  13,189,120                                           */




/* end of header */
#endif

