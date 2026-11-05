#include <maia/kernel.h>

#define MAIA_MEM_MAGIC_FREE 0x4D465245UL /* 'MFRE' */
#define MAIA_MEM_MAGIC_USED 0x4D555344UL /* 'MUSD' */
#define MAIA_MEM_MIN_SPLIT  32U

typedef struct maia_mem_block {
    uint32_t               magic;
    uint32_t               used;
    size_t                 size; /* payload bytes following this header */
    struct maia_mem_block *next;
    struct maia_mem_block *prev;
} maia_mem_block_t;

static maia_mem_block_t *g_head = NULL;
static maia_mem_stats_t  g_stats;

static size_t align_up(size_t val, size_t align) {
    return (val + (align - 1U)) & ~(align - 1U);
}

maia_status_t maia_mem_init(void *pool_base, size_t pool_bytes) {
    if (pool_base == NULL) {
        return MAIA_ERR_INVALID;
    }
    uintptr_t raw = (uintptr_t)pool_base;
    uintptr_t aligned = (raw + 7U) & ~(uintptr_t)7U;
    size_t diff = (size_t)(aligned - raw);
    if (pool_bytes <= diff + sizeof(maia_mem_block_t) + 16U) {
        return MAIA_ERR_INVALID;
    }
    size_t usable = (pool_bytes - diff) & ~(size_t)7U;

    g_head = (maia_mem_block_t *)aligned;
    g_head->magic = MAIA_MEM_MAGIC_FREE;
    g_head->used  = 0U;
    g_head->size  = usable - sizeof(maia_mem_block_t);
    g_head->next  = NULL;
    g_head->prev  = NULL;

    g_stats.total_bytes     = g_head->size;
    g_stats.used_bytes      = 0U;
    g_stats.free_bytes      = g_head->size;
    g_stats.peak_used_bytes = 0U;
    g_stats.alloc_count     = 0U;
    g_stats.free_count      = 0U;
    return MAIA_OK;
}

void *maia_mem_alloc(size_t size) {
    return maia_mem_alloc_aligned(size, 8U);
}

void *maia_mem_alloc_aligned(size_t size, size_t alignment) {
    if (g_head == NULL || size == 0U || alignment == 0U || (alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }
    if (alignment < 8U) {
        alignment = 8U;
    }
    size_t req = align_up(size, 8U);

    for (maia_mem_block_t *cur = g_head; cur != NULL; cur = cur->next) {
        if (cur->used != 0U || cur->magic != MAIA_MEM_MAGIC_FREE) {
            continue;
        }
        uintptr_t payload_start = (uintptr_t)cur + sizeof(maia_mem_block_t);
        uintptr_t aligned_start = (payload_start + (alignment - 1U)) & ~(uintptr_t)(alignment - 1U);
        size_t pad = (size_t)(aligned_start - payload_start);

        if (pad > 0U && pad < sizeof(maia_mem_block_t) + 8U) {
            size_t step = align_up(sizeof(maia_mem_block_t) + 8U, alignment);
            aligned_start += step;
            pad = (size_t)(aligned_start - payload_start);
        }

        if (cur->size < pad + req) {
            continue;
        }

        if (pad > 0U) {
            /* Split front padding block so aligned_start has its own header */
            maia_mem_block_t *aligned_blk = (maia_mem_block_t *)(aligned_start - sizeof(maia_mem_block_t));
            aligned_blk->magic = MAIA_MEM_MAGIC_FREE;
            aligned_blk->used  = 0U;
            aligned_blk->size  = cur->size - pad;
            aligned_blk->next  = cur->next;
            aligned_blk->prev  = cur;
            if (cur->next != NULL) {
                cur->next->prev = aligned_blk;
            }
            cur->size = pad - sizeof(maia_mem_block_t);
            cur->next = aligned_blk;
            g_stats.free_bytes -= sizeof(maia_mem_block_t);
            cur = aligned_blk;
        }

        /* Split trailing remainder if large enough */
        if (cur->size >= req + sizeof(maia_mem_block_t) + MAIA_MEM_MIN_SPLIT) {
            uintptr_t next_addr = (uintptr_t)cur + sizeof(maia_mem_block_t) + req;
            maia_mem_block_t *rem = (maia_mem_block_t *)next_addr;
            rem->magic = MAIA_MEM_MAGIC_FREE;
            rem->used  = 0U;
            rem->size  = cur->size - req - sizeof(maia_mem_block_t);
            rem->next  = cur->next;
            rem->prev  = cur;
            if (cur->next != NULL) {
                cur->next->prev = rem;
            }
            cur->size = req;
            cur->next = rem;
            g_stats.free_bytes -= sizeof(maia_mem_block_t);
        }

        cur->magic = MAIA_MEM_MAGIC_USED;
        cur->used  = 1U;
        g_stats.used_bytes += cur->size;
        g_stats.free_bytes -= cur->size;
        if (g_stats.used_bytes > g_stats.peak_used_bytes) {
            g_stats.peak_used_bytes = g_stats.used_bytes;
        }
        g_stats.alloc_count += 1U;
        return (void *)((uintptr_t)cur + sizeof(maia_mem_block_t));
    }

    return NULL;
}

maia_status_t maia_mem_free(void *ptr) {
    if (ptr == NULL || g_head == NULL) {
        return MAIA_ERR_INVALID;
    }
    maia_mem_block_t *blk = (maia_mem_block_t *)((uintptr_t)ptr - sizeof(maia_mem_block_t));
    if (blk->magic != MAIA_MEM_MAGIC_USED || blk->used == 0U) {
        return MAIA_ERR_INVALID;
    }

    blk->magic = MAIA_MEM_MAGIC_FREE;
    blk->used  = 0U;
    g_stats.used_bytes -= blk->size;
    g_stats.free_bytes += blk->size;
    g_stats.free_count += 1U;

    /* Coalesce with next free block */
    if (blk->next != NULL && blk->next->used == 0U && blk->next->magic == MAIA_MEM_MAGIC_FREE) {
        maia_mem_block_t *nxt = blk->next;
        blk->size += sizeof(maia_mem_block_t) + nxt->size;
        blk->next  = nxt->next;
        if (nxt->next != NULL) {
            nxt->next->prev = blk;
        }
        g_stats.free_bytes += sizeof(maia_mem_block_t);
    }

    /* Coalesce with previous free block */
    if (blk->prev != NULL && blk->prev->used == 0U && blk->prev->magic == MAIA_MEM_MAGIC_FREE) {
        maia_mem_block_t *prv = blk->prev;
        prv->size += sizeof(maia_mem_block_t) + blk->size;
        prv->next  = blk->next;
        if (blk->next != NULL) {
            blk->next->prev = prv;
        }
        g_stats.free_bytes += sizeof(maia_mem_block_t);
    }

    return MAIA_OK;
}

maia_status_t maia_mem_get_stats(maia_mem_stats_t *out_stats) {
    if (out_stats == NULL) {
        return MAIA_ERR_INVALID;
    }
    *out_stats = g_stats;
    return MAIA_OK;
}

