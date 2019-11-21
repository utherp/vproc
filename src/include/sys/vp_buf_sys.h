#ifndef __VPI_BUF_H__
#define __VPI_BUF_H__

#ifndef __VP_BUF_H__
    #error "You should NEVER include internal/vpi_buf.h directly! Define __NEED_VPI_BUF__ and include 'vp_buf.h' instead."
#endif

/* state buffers' index macros */
#include "vp_lock.h"


struct _vp_buf_sys_s {
    struct _mm_set_s *id;
    uint64_t seq_id;
    VP_LOCK lock;
    uint32_t size;
    int32_t refs;
};
typedef struct _vp_buf_sys_s VP_BUF;

#define buffer_data_offset(b) (((char*)b) + sizeof(VP_BUF))
#define buffer_header_offset(b) ((VP_BUF*)(((char*)b) - sizeof(VP_BUF)))

/**************************************************
 * increment buffer reference
 */

#define ref_buffer(b) \
    do { \
        _debugger("MM", "Referencing buffer (0x%X)", b); \
        vproc_lock((&(b->lock))); \
        b->refs++; \
        vproc_unlock((&(b->lock))); \
    } while(0)
    

/**************************************************
 * decrement buffer reference, and return it to
 * free chain if count is 0
 */

#define unref_buffer(b) \
    do { \
        _debugger("MM", "Unreferencing buffer (0x%X)", b); \
        vproc_lock((&(b->lock))); \
        b->refs--; \
        if (b->refs < 1) { \
            _debugger("MM", "--> Returning buffer to free chain...", 0); \
            vproc_unlock((&(b->lock))); \
            vp_mm_push_free_chain(b); \
            break; \
        } \
        vproc_unlock((&(b->lock))); \
        _debugger("MM", "--> New ref count: %d", b->refs); \
    } while(0)


/* end of header */
#endif

