#define __VPROC_SYS__ 1
#define __NEED_VPROC_MM_CORE__ 1
#define __NEED_VP_BUF_SYS__ 1
#define __NEED_VP_LOCK_SYS__ 1
#include "vproc.h"

#include <xmmintrin.h>
#include <stdio.h>
#include <string.h>

static mempool pools[MM_POOL_COUNT];

static int allocate_pool (mempool *pool) {
    int ret;
    uint32_t total = (pool->pages + 1) * pool->count * MM_PAGE_SIZE; /* 1 extra page for segment info */

    if ((ret = posix_memalign(&(pool->base_addr), MM_PAGE_SIZE, total))) {
        _show_error("MM", "Failed to allocate pool memory of %u bytes: %s", total, strerror(ret));
        return 1;
    }

    pool->used = 0;
    pool->free = NULL;

    char *mem = (char*)pool->base_addr;
    memseg **seg = (memseg**)&mem;

    for (ret = pool->count; ret; ret--) {
        (*seg)->flags = 0;
        (*seg)->poolid = pool->id;
        (*seg)->next = pool->free;
        pool->free = *seg;
        mem += sizeof(memseg) + (pool->pages << MM_PAGE_BITS);
    }

    return 0;
}

int vproc_mm_init () {
    int i;
    uint32_t page_counts[MM_POOL_COUNT] = MM_POOL_SEG_PAGES;
    uint32_t seg_counts[MM_POOL_COUNT] = MM_POOL_SEG_COUNTS;

    _debugger("MM", "Initializing MM:  %u Pools", MM_POOL_COUNT);

    for (i = 0; i < MM_POOL_COUNT; i++) {
        pools[i].id = i;
        pools[i].count = seg_counts[i];
        pools[i].pages = page_counts[i];
        vp_lock_init((&(pools[i].lock)));

        _debugger("MM", "Allocating pool %u: %u segments, %u pages per segment, %u bytes per page...",
                        i, pools[i].count, pools[i].pages, MM_PAGE_SIZE);

        if (allocate_pool(&(pools[i]))) {
            _show_error("MM", "Failed to allocate pool %u", i);
            return 1;
        }
    }
    return 0;
}


static inline void free_segment (memseg * restrict seg) {
    mempool * restrict pool = &pools[seg->poolid];
    VP_LOCK * restrict plock = &pool->lock;

    vproc_lock(plock);

    seg->next = pool->free;
    pool->free = seg;

    vproc_unlock(plock);
    unset_seg_alloc(seg);

    return;
}

inline void vproc_free (void *ptr) {
    _debugger("MM", "Returning page at 0x%08X", ptr);

    if (!ptr) return;

    memseg *seg = segment_addr(ptr);

    if (!is_seg_alloc(seg)) {
        _show_error("MM", "Attempted to free an unallocated segment at 0x%X!", seg);
        return;
    }

    free_segment(seg);

    return;
}


static inline void *alloc_pages (size_t pages) {
    memseg * restrict seg;
    memseg ** restrict pfree;
    memseg ** restrict mnext;
    uint32_t * restrict pused;
    VP_LOCK * restrict plock;
//    _debugger("MM", "Attempting to allocate %u pages...", pages);
    int i;
    for (i = 0; i < MM_POOL_COUNT; i++) {
        if (pages > pools[i].pages) continue;
        if (!pools[i].free) continue;
        goto mm_alloc_from_pool;
    }
    _show_error("MM", "Failed to allocate %u pages: Too large for any pool!", pages);
    return NULL;


mm_alloc_from_pool:
     pfree = &(pools[i].free);
     plock = &(pools[i].lock);
     pused = &(pools[i].used);
     seg = *pfree;
     mnext = &seg->next;

//    _debugger("MM", "... Allocating from pool %u", i);

    vproc_lock(plock);
    *pfree = *mnext;
    *mnext = NULL;
    (*pused)++;
    vproc_unlock(plock);

    set_seg_alloc(seg);

    return ((void*)seg)+sizeof(memseg);
}

void *vproc_malloc (size_t size) {
    return alloc_pages(page_count(size));
}

void *vproc_calloc (size_t count, size_t size) {
    size *= count;
    int pages = page_count(size);
    __m128i *addr = alloc_pages(pages);
    if (!addr) return NULL;
    for (; pages; pages--)
        addr[pages-1] = addr[pages-1] ^ addr[pages-1];

    return (void*)addr;
}

void *vproc_calloc_chain (size_t count, size_t size) {
    void *chain = NULL;

    /* calloc and chain 'count' buffers */
    while (count--)
        vp_mm_chain_push(&chain, vproc_calloc(1, size));

    /* return chain */
    return chain;
}

void *vproc_dup_segment (void *addr) {
    memseg *seg = segment_addr(addr);
    memseg *newseg = alloc_pages(pools[seg->poolid].pages);
    __m128 *src = addr;
    __m128 *dst = segment_offset(newseg);
    int i = pools[seg->poolid].pages - 1;
    for (; i; i--) 
        dst[i] = src[i];

    return dst;
}

void *vproc_realloc(void *ptr, size_t size) {
    /* if no pointer passed, just use vproc_malloc */
    if (ptr == NULL) return vproc_malloc(size);

    memseg *seg = (ptr - sizeof(memseg));
    mempool * restrict pool = &pools[seg->poolid];

    uint32_t pages = page_count(size);

    if (pages <= pool->pages)
        return ptr;  /* segment is already big enough... */

    /* get new segment */
    __m128i *addr = alloc_pages(pages);

    /* copy old segment to new segment */
    uint32_t i = pool->pages;
    for (; i; i--, addr[i] ^= addr[i]); //addr[i]);

    /* free old segment */
    free_segment(seg);

    return addr;
}

inline void vp_mm_chain_push (void **chain, void *addr) {
    if (!*chain) {
        /* the chain is empty, unset new segment's next link ptr */
        segment_addr(addr)->next = NULL;
    } else {
        /* push buffer 'addr' onto the top of the chain */
        segment_addr(addr)->next = segment_addr(*chain);
    }

    /* set new buffer address to chain head */
    *chain = addr;

    #if __DEBUG_CHAINS__
        _debugger("Chain", "### Chain stack: ###", 0);
        addr = *chain;
        memseg *seg = segment_addr(addr);
        while (seg) {
            _debugger("Chain", "--> (addr: 0x%X [seg: 0x%X])  (next: [seg: 0x%X])", addr, seg, seg->next);
            seg = seg->next;
            addr = segment_offset(seg);
        }
        _debugger("Chain", "####################", 0);
    #endif
    return;
}

inline void *vp_mm_chain_pop (void **chain) {
    if (!*chain)
        /* chain is empty */
        return NULL;

    /* copy last buffer address added to chain */
    void *addr = *chain;
    memseg *seg = segment_addr(addr);

    /* get memseg address of previous buffer in chain */
    seg = seg->next;

    if (!seg)
        /* no previous buffer in chain, chain is empty */
        *chain = NULL;
    else
        /* point chain to previous buffer offset */
        *chain = segment_offset(seg);

    /* return popped buffer address */
    return addr;
}

inline void *vp_mm_chain_shift (void **chain) {
    if (!*chain)
        /* chain is empty */
        return NULL;

    memseg **last = NULL;
    memseg *seg = segment_addr(*chain);

    /* seek to begining of chain */
    while (seg->next) {
        last = &seg->next;
        seg = seg->next;
    }

    if (!last) 
        /* only one segment, chain is empty */
        *chain = NULL;
    else
        /* unset the second segment's next ptr, making it the first */
        *last = NULL;
    
    /* return the shifted segment's buffer offset */
    return segment_offset(seg);
}

inline void vp_mm_chain_unshift (void **chain, void *addr) {
    if (!*chain) {
        /* chain is empty */
        *chain = addr;
        segment_addr(addr)->next = NULL;
        return;
    }

    /* get segment ptr for buffer addr */
    memseg *seg = segment_addr(addr);

    /* unset new segment's next ptr */
    seg->next = NULL;

    /* get segment ptr for chain */
    memseg *chain_seg = segment_addr(*chain);

    /* seek to end of chain */
    while (chain_seg->next) chain_seg = chain_seg->next;

    /* slide new segment first in the chain */
    chain_seg->next = seg;

    return;
}

inline void *vp_mm_chain_next (void *addr) {
    memseg *seg = segment_addr(addr);
    if (!seg->next)
        /* last segment... */
        return NULL;

    return segment_offset(seg->next);
}

const static char szsuffix[4] = { 'b', 'k', 'M', 'G' };
static uint32_t _readable_size (uint32_t size, char *suf) {
    char i;
    for (i = 0; i < 4 && size > 2000; i++, size /= 1000);
    *suf = szsuffix[i];
    return size;
}

void display_mm_summary (FILE *output) {
    int i;
    char bt = ' ';
    fprintf(output, "\n Pool  | SegSz     | Count  | Used  | Free  \n"
                    "-------+-----------+--------+-------+-------\n"
    );
    for (i = 0; i < MM_POOL_COUNT; i++) {
        mempool * restrict pool = &(pools[i]);
        uint32_t segsz = pool->pages << MM_PAGE_BITS;
        segsz = _readable_size(segsz, &bt);
        fprintf(output, " %5u | %8u%c | %6u | %5u | %5u \n",
            i, 
            segsz, bt,
            pool->count,
            pool->used,
            pool->count - pool->used
        );
    }

//    fprintf(output, "-------+-----------+--------+-------+-------\n");
    return;
}

void display_mm_usage (FILE *output) {

    fprintf(output, "VProc Memory Usage:\n==============================\n"
      "    Page Size: %u\n"
      "   # of Pools: %u\n\n",
        MM_PAGE_SIZE, MM_POOL_COUNT
    );

    int i;
    uint32_t total_bytes = 0;
    uint32_t total_used = 0;
    uint32_t total_free = 0;
    char lb = ' ';
    char lt = ' ';
    char lu = ' ';
    char lf = ' ';


    for (i = 0; i < MM_POOL_COUNT; i++) {
        mempool * restrict pool = &(pools[i]);
        uint32_t bytes = pool->pages << MM_PAGE_BITS;
        uint32_t total = bytes * pool->count;
        uint32_t used = bytes * pool->used;
        uint32_t bfree = (total - used);
        total_used += used;
        total_bytes += total;
        total_free += bfree;
        bytes = _readable_size(bytes, &lb);
        total = _readable_size(total, &lt);
        used  = _readable_size(used, &lu);
        bfree = _readable_size(bfree, &lf);
 
        fprintf(output, "  Pool %u:\n-------------------------\n"
          "   Segments: %u  (%u used, %u free)\n"
          "   Seg size: %u%c\n"
          " Pool total: %u%c  (%u%c used, %u%c free)\n\n", i, 
            pool->count, pool->used, (pool->count - pool->used),
            bytes, lb,
            total, lt,
            used, lu,
            bfree, lf
        );
    }

    total_bytes = _readable_size(total_bytes, &lt);
    total_used  = _readable_size(total_used, &lu);
    total_free = _readable_size(total_free, &lf);

    fprintf(output, "VProc Total: %u%c   (%u%c used, %u%c free)\n\n",
        total_bytes, lt, total_used, lu, total_free, lf
    );

    return;
}

