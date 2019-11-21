/*************************************************************************\
 *                                                                       *
 * VProc VPROC_MM                                                         *
 *                                                                       *
 *    SYS header file                                                    *
 *                                                                       *
\*************************************************************************/

#ifndef __VPROC_MM_SYS_H__
#define __VPROC_MM_SYS_H__
/* start of header */

/* this should have been included from 'vproc_mm.h', with __NEED_VPROC_MM_SYS__ or __NEED_VPROC_MM_CORE__ defined */
#ifndef __VPROC_MM_H__
    #error "Never include 'sys/vproc_mm_sys.h' directly!  Define __NEED_VPROC_MM_SYS__ and include 'vproc.h'"
#endif


#ifdef __NEED_VPROC_MM_CORE__

    /* VPROC_MM CORE definitions */
    #include "core/vproc_mm_core.h"

#else

    /* VPROC_MM SYS specific definitions */


#define MM_SETT_SINGLE 1
#define MM_SETT_TOGGLE 2
#define MM_SETT_POOL   3

#define VP_BUF_ACTIVE 0
#define VP_BUF_NEXT   1
#define VP_BUF_LATEST 2
#define VP_BUF_FREE   3

#include "vp_lock.h"
#include "vp_feed.h"


typedef union _mm_single_sys_s {
    struct {
        VP_BUF *active;
    } by_pos;
    VP_BUF *by_index[1];
} MM_SINGLE;

typedef union _mm_toggle_sys_s {
    struct {
        VP_BUF *active;
        VP_BUF *next;
    } pos;
    VP_BUF *index[2];
} MM_TOGGLE;

typedef union _mm_pool_sys_s {
    struct {
        VP_BUF *active;  /* active buffer... the one currently being written to */
        VP_BUF *next;    /* next available buffer... the next one to be "active") */
        VP_BUF *latest;  /* latest buffer... the most recent buffer completed (last one to be "active") */
        VP_BUF *free;    /* free buffer chain */
    } pos;
    /* list form of addressing the above buffers */
    VP_BUF *index[4];  /* 0: active, 1:next, 2:latest, 3:free */
} MM_POOL;

typedef struct _mm_set_s {
    uint8_t type;
    uint32_t flags;
    VP_LOCK lock;
    VP_FEED *feed;
    MM_POOL buffers;
} MM_SET;


MM_SET *create_mm_set(uint8_t type, VP_FEED *feed);


/* from mm_set.c */
uint32_t cycle_buffer_states (MM_SET *mmset);
void vp_mm_push_free_chain (VP_BUF *buf);
uint32_t add_buffer_to_free_chain (MM_SET *mmset);


/* from vproc_mm.c */
void display_mm_summary_header (FILE *output);
void display_mm_summary (FILE *output);
void display_mm_usage (FILE *output);

#endif


/* VPROC_MM common SYS/CORE definitions */



/* end of header */
#endif

