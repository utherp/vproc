#define __VPROC_SYS__ 1
#define __NEED_VPROC_MM_SYS__ 1
#define __NEED_VPROC_SM__ 1
#define __NEED_VP_BUF_SYS__ 1
#define __NEED_VP_LOCK_SYS__ 1
#define __NEED_VP_FEED__ 1
#include "vproc.h"

static uint32_t locate_next_buffer (MM_SET *mmset);
static void *pop_free_chain (MM_SET *mmset);
static inline MM_SET *init_mm_pool (MM_SET *mmset);
static inline MM_SET *init_mm_toggle (MM_SET *mmset);
static inline MM_SET *init_mm_single (MM_SET *mmset);
static VP_BUF *create_vp_buf (size_t bufsize, void *set);


/*****************************************************
 * Create a Memory Managed Set: A set of buffers used
 * by feeds or intermediate formats.
 * type is one of:
 *       MM_SETT_POOL:  Buffer pool   (3+ buffers)
 *     MM_SETT_TOGGLE:  Buffer toggle (2 buffers)
 *     MM_SETT_SINGLE:  A single buffer
 *
 * Should the need arise, a set can change its type.
 * For example, if an intermediate format is opened
 * by another reader, then the toggle will have
 * buffers added to it and become a pool.
 */
MM_SET *create_mm_set(uint8_t type, VP_FEED *feed) {
    MM_SET *mmset = malloc(sizeof(MM_SET));
    mmset->type = type;
    mmset->feed = feed;

    switch (type) {
        case(MM_SETT_POOL):  
            _debugger("MM", "Creating mm_set of type POOL", 0);
            return init_mm_pool(mmset);
        case(MM_SETT_TOGGLE): 
            _debugger("MM", "Creating mm_set of type TOGGLE", 0);
            return init_mm_toggle(mmset);
        case(MM_SETT_SINGLE):
            _debugger("MM", "Creating mm_set of type SINGLE", 0);
            return init_mm_single(mmset);
    }

    _show_error("MM", "Unknown MM_SET type %d", type);
    /* unknown set type */
    errno = EINVAL;
    return NULL;
}

/*******************************************************
 * Initialize buffer pool...
 */
static inline MM_SET *init_mm_pool (MM_SET *mmset) {
    uint32_t bufsz = vp_feed_frame_sz(mmset->feed);

    mmset->buffers.pos.active = create_vp_buf(bufsz, mmset);
    mmset->buffers.pos.next = create_vp_buf(bufsz, mmset);
    mmset->buffers.pos.latest = NULL;
    mmset->buffers.pos.free = NULL;

    _debugger("SET", "--> Added buffer (0x%X) to active position", mmset->buffers.pos.active);
    _debugger("SET", "--> Added buffer (0x%X) to next position", mmset->buffers.pos.next);

    mmset->buffers.pos.active->refs = 1;
    mmset->buffers.pos.next->refs = 1;

    vp_lock_init((&(mmset->lock)));
    add_buffer_to_free_chain(mmset);

    _debugger("SET", "--> Added buffer (0x%X) to free position", mmset->buffers.pos.free);

    return mmset;

}

/*******************************************************
 * Initialize buffer toggle...
 */
static inline MM_SET *init_mm_toggle (MM_SET *mmset) {
    uint32_t bufsz = vp_feed_frame_sz(mmset->feed);

    mmset->buffers.pos.active = create_vp_buf(bufsz, mmset);
    mmset->buffers.pos.next = create_vp_buf(bufsz, mmset);
    vp_lock_init((&(mmset->lock)));

    return mmset;
}


/*******************************************************
 * Initialize buffer single...
 */
static inline MM_SET *init_mm_single (MM_SET *mmset) {
    uint32_t bufsz = vp_feed_frame_sz(mmset->feed);
    mmset->buffers.pos.active = create_vp_buf(bufsz, mmset);
    return mmset;
}


/**************************************************
 * Allocate and add a new buffer to 
 * the pool's free buffers chain
 */
uint32_t add_buffer_to_free_chain (MM_SET *mmset) {
    if (mmset->type != MM_SETT_POOL) {
        _show_error("POOL", "Unable to add buffer to free chain: MM_SET is not a POOL (type: %u)!", mmset->type);
        return -1;
    }

    void *buf = create_vp_buf(vp_feed_frame_sz(mmset->feed), mmset);
    _debugger("MM", "adding buffer (0x%X) to free chain of mmset (current free: 0x%X)", buf, mmset->buffers.pos.free);
    vp_mm_push_free_chain(buf);
    _debugger("MM", "--> added buffer to free chain (current free: 0x%X)", mmset->buffers.pos.free);

    return 0;
}


/****************************************************
 * cycle buffers in a set
 */
uint32_t cycle_buffer_states (MM_SET *mmset) {
    VP_LOCK * restrict plock = &mmset->lock;

    if (mmset->type == MM_SETT_TOGGLE) {
        /*********************************
         * mmset is a toggle, we'll just
         * switch the buffers around...
         */
        VP_BUF ** restrict tactive = &mmset->buffers.pos.active;
        VP_BUF ** restrict tnext = &mmset->buffers.pos.next;

        vproc_lock(plock);
        void *tmp = *tnext;
        *tnext = *tactive;
        *tactive = tmp;
        vproc_unlock(plock);

        return 0;
    }

    if (mmset->type == MM_SETT_POOL) {
        /*********************************
         * mmset is a pool, move active to
         * latest, and next to active
         */
        VP_BUF ** restrict pactive = &mmset->buffers.pos.active;
        VP_BUF ** restrict pnext = &mmset->buffers.pos.next;
        VP_BUF ** restrict platest = &mmset->buffers.pos.latest;


        _debugger("MM", "States before cycle: NEXT(0x%X [id: %u, ref: %d]) ACT(0x%X [id: %u, ref: %d]) LAST(0x%X [id: %u, ref: %d])", 
                    *pnext, (*pnext)?(*pnext)->seq_id:0, (*pnext)?(*pnext)->refs:1,
                    *pactive, (*pactive)?(*pactive)->seq_id:0, (*pactive)?(*pactive)->refs:-1,
                    *platest, (*platest)?(*platest)->seq_id:0, (*platest)?(*platest)->refs:-1
                    );
        /* lock set */
        vproc_lock(plock);

        /* decrement latest buffer reference */
        if (*platest)
            unref_buffer((*platest));

        /* move active buffer to latest position */
        *platest = *pactive;

        /* if no buffer in next position, find one */
        if (!*pnext) locate_next_buffer(mmset);
        /* reset pnext reference... being 'restrict', the value would not be resolved again if it changed */
        pnext = &mmset->buffers.pos.next;

        /* move next buffer to active position */
        *pactive = *pnext;

        /* unset next buffer position and find a new one */
        *pnext = NULL;
        locate_next_buffer(mmset);

        vproc_unlock(plock);

        _debugger("MM", "States before cycle: NEXT(0x%X [id: %u, ref: %d]) ACT(0x%X [id: %u, ref: %d]) LAST(0x%X [id: %u, ref: %d])", 
                    *pnext, (*pnext)?(*pnext)->seq_id:0, (*pnext)?(*pnext)->refs:1,
                    *pactive, (*pactive)?(*pactive)->seq_id:0, (*pactive)?(*pactive)->refs:-1,
                    *platest, (*platest)?(*platest)->seq_id:0, (*platest)?(*platest)->refs:-1
                    );

        return 0;
    }

    _show_error("MM", "Attempted to cycle buffers for unsupported MM_SET type %u", mmset->type);
    return -1;

}

/****************************************************
 * locates or creates the next buffer in a pool
 */
static uint32_t locate_next_buffer (MM_SET *mmset) {
    VP_BUF ** restrict pnext = &mmset->buffers.pos.next;
    VP_LOCK * restrict plock = &mmset->lock;
    uint8_t * restrict stype = &mmset->type;

    if (*stype != MM_SETT_POOL) {
        /* mmset is not a pool! */
        _show_error("POOL", "Attempted to locate next buffer for a set which is NOT a pool (type: %u)", *stype);
        return -1;
    }

    if (*pnext) {
        _debugger("SET", "Next buffer already set (0x%X)", *pnext);
        return 0; /* next buffer already exists */
    }

    vproc_lock(plock);

    /* no buffer in next position, fetch one from free chain */
    _debugger("MM", "Popping buffer from free chain (current free: 0x%X)", mmset->buffers.pos.free);
    VP_BUF *buf = pop_free_chain(mmset);
    _debugger("MM", "--> Popped buffer (0x%X) from free chain (current free: 0x%X)", buf, mmset->buffers.pos.free);
    if (!buf) {
        /* no free buffer available!? this should not happen, but lets catch it for debugging */
        _show_error("POOL", "When cycling buffers, no buffer in next position, and no buffers in free chain: (set: 0x%08X)!", mmset);
        /* now we'll just add a new one... adding buffer automatically puts it in the next position if not set */
        add_buffer_to_free_chain(mmset);
        vproc_unlock(plock);
        return 0;
    }
    
    _debugger("SET", "--> Setting buffer (0x%X) to next position", buf);
    _debugger("SET", "----> buffer: seq: %u, size: %u, refs: %d", buf->seq_id, buf->size, buf->refs);
    *pnext = buf;
    ref_buffer(buf); /* increase reference of next buffer */
    _debugger("SET", "--> finished referencing new next buffer", 0);

    vproc_unlock(plock);

    return 0;
}


/**************************************************
 * push buffer onto the pool's free buffers chain
 */
void vp_mm_push_free_chain (VP_BUF *buf) {
    MM_SET *  mmset = (MM_SET*)buf->id;
    VP_LOCK * restrict plock = &mmset->lock;
    MM_POOL * restrict mmpool = &mmset->buffers;
    VP_BUF ** restrict pfree = &mmpool->pos.free;
    VP_BUF ** restrict pnext = &mmpool->pos.next;

    if (!mmset) {
        _debugger("MM", "Buffer has be removed from set, freeing...", 0);
        free(buf);
        return;
    }

    vproc_lock(plock);

    _debugger("MM", "Pushing buffer into free chain of mmset", 0);
    if (!*pnext) {
        /* there is no buffer in next, we'll put it there instead */
        _debugger("MM", "-->putting buffer in next position", 0);
        ref_buffer(buf);
        *pnext = buf; /* ... next is a VP_BUF */
    } else {
        _debugger("MM", "-->putting buffer in free chain", 0);
        vp_mm_chain_push((void**)pfree, buf);
    }

    vproc_unlock(plock);

    return;
}


/**************************************************
 * pop and return a buffer from the pool's free
 * buffers chain
 */
static void *pop_free_chain (MM_SET *mmset) {
    VP_LOCK * restrict plock = &mmset->lock;
    MM_POOL * restrict mmpool = &mmset->buffers;
    VP_BUF ** restrict pfree = &mmpool->pos.free;

    /* no buffers left in free chain */
    if (!*pfree) {
        _show_error("POOL", "Unable to pop a buffer from the free chain: No buffers left in chain. (set: 0x%08X)", mmset);
        return NULL;
    }

    vproc_lock(plock);

    _debugger("SET", "Free chain is 0x%X", *pfree);
    void *addr = vp_mm_chain_pop((void**)pfree);

    vproc_unlock(plock);

    return addr;
}

static VP_BUF *create_vp_buf (size_t bufsize, void *set) {
    VP_BUF *buf = calloc(1, bufsize + sizeof(VP_BUF));
    buf->id = set;
    buf->seq_id = 0;
    buf->size = bufsize;
    buf->refs = 0;
    vp_lock_init((&buf->lock));
    return buf;
}

