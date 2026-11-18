#include <maia/kernel.h>

#define MAIA_MEM_MAGIC_FREE 0x4D465245UL /* 'MFRE' */
#define MAIA_MEM_MAGIC_USED 0x4D555344UL /* 'MUSD' */
#define MAIA_MEM_MIN_SPLIT  32U
#define MAIA_MEM_MAX_REGIONS 4U

typedef struct maia_mem_block {
    uint32_t               magic;
    uint32_t               used;
    size_t                 size; /* payload bytes following this header */
    struct maia_mem_block *next;
    struct maia_mem_block *prev;
} maia_mem_block_t;

typedef struct maia_mem_region {
    uintptr_t start;
    uintptr_t end;
    maia_mem_block_t *head;
} maia_mem_region_t;

static maia_mem_region_t g_regions[MAIA_MEM_MAX_REGIONS];
static size_t            g_region_count;
static maia_mem_stats_t  g_stats;

static size_t align_up(size_t val, size_t align) {
    return (val + (align - 1U)) & ~(align - 1U);
}

static maia_status_t region_layout(void *pool_base, size_t pool_bytes,
                                   maia_mem_region_t *region) {
    if (pool_base == NULL || region == NULL) {
        return MAIA_ERR_INVALID;
    }
    uintptr_t raw = (uintptr_t)pool_base;
    if (raw > UINTPTR_MAX - 7U || pool_bytes > UINTPTR_MAX - raw) {
        return MAIA_ERR_INVALID;
    }
    uintptr_t aligned = (raw + 7U) & ~(uintptr_t)7U;
    size_t diff = (size_t)(aligned - raw);
    if (diff > pool_bytes || pool_bytes - diff <= sizeof(maia_mem_block_t) + 16U) {
        return MAIA_ERR_INVALID;
    }
    size_t usable = (pool_bytes - diff) & ~(size_t)7U;
    if (usable > UINTPTR_MAX - aligned) {
        return MAIA_ERR_INVALID;
    }

    region->start = aligned;
    region->end = aligned + usable;
    region->head = (maia_mem_block_t *)aligned;
    return MAIA_OK;
}

static void init_region_block(maia_mem_region_t *region) {
    maia_mem_block_t *head = region->head;
    head->magic = MAIA_MEM_MAGIC_FREE;
    head->used  = 0U;
    head->size  = (size_t)(region->end - region->start) - sizeof(maia_mem_block_t);
    head->next  = NULL;
    head->prev  = NULL;
}

static int block_in_region(const maia_mem_region_t *region,
                           const maia_mem_block_t *block) {
    uintptr_t address = (uintptr_t)block;
    return address >= region->start &&
           address <= region->end - sizeof(maia_mem_block_t) &&
           address % _Alignof(maia_mem_block_t) == 0U;
}

maia_status_t maia_mem_init(void *pool_base, size_t pool_bytes) {
    maia_mem_region_t region;
    maia_status_t result = region_layout(pool_base, pool_bytes, &region);
    if (result != MAIA_OK) {
        return result;
    }

    g_region_count = 0U;
    g_stats.total_bytes     = 0U;
    g_stats.used_bytes      = 0U;
    g_stats.free_bytes      = 0U;
    g_stats.peak_used_bytes = 0U;
    g_stats.alloc_count     = 0U;
    g_stats.free_count     = 0U;
    init_region_block(&region);
    g_regions[0] = region;
    g_region_count = 1U;
    g_stats.total_bytes = region.head->size;
    g_stats.free_bytes = region.head->size;
    return MAIA_OK;
}

maia_status_t maia_mem_add_region(void *pool_base, size_t pool_bytes) {
    maia_mem_region_t region;
    maia_status_t result = region_layout(pool_base, pool_bytes, &region);
    if (result != MAIA_OK || g_region_count == 0U) {
        return result != MAIA_OK ? result : MAIA_ERR_INVALID;
    }
    for (size_t i = 0U; i < g_region_count; ++i) {
        if (region.start < g_regions[i].end && g_regions[i].start < region.end) {
            return MAIA_ERR_INVALID;
        }
    }
    if (g_region_count >= MAIA_MEM_MAX_REGIONS) {
        return MAIA_ERR_NOMEM;
    }
    size_t payload_bytes =
        (size_t)(region.end - region.start) - sizeof(maia_mem_block_t);
    if (g_stats.total_bytes > SIZE_MAX - payload_bytes) {
        return MAIA_ERR_NOMEM;
    }

    init_region_block(&region);
    g_regions[g_region_count++] = region;
    g_stats.total_bytes += payload_bytes;
    g_stats.free_bytes += payload_bytes;
    return MAIA_OK;
}

void *maia_mem_alloc(size_t size) {
    return maia_mem_alloc_aligned(size, 8U);
}

void *maia_mem_alloc_aligned(size_t size, size_t alignment) {
    if (g_region_count == 0U || size == 0U || size > SIZE_MAX - 7U ||
        alignment == 0U || (alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }
    if (alignment < 8U) {
        alignment = 8U;
    }
    size_t req = align_up(size, 8U);

    for (size_t region_index = 0U; region_index < g_region_count; ++region_index) {
        for (maia_mem_block_t *cur = g_regions[region_index].head;
             cur != NULL; cur = cur->next) {
            if (cur->used != 0U || cur->magic != MAIA_MEM_MAGIC_FREE) {
                continue;
            }
            uintptr_t payload_start = (uintptr_t)cur + sizeof(maia_mem_block_t);
            if (payload_start > UINTPTR_MAX - (alignment - 1U)) {
                continue;
            }
            uintptr_t aligned_start = (payload_start + (alignment - 1U)) & ~(uintptr_t)(alignment - 1U);
            size_t pad = (size_t)(aligned_start - payload_start);

            if (pad > 0U && pad < sizeof(maia_mem_block_t) + 8U) {
                size_t step = align_up(sizeof(maia_mem_block_t) + 8U, alignment);
                if (aligned_start > UINTPTR_MAX - step) {
                    continue;
                }
                aligned_start += step;
                pad = (size_t)(aligned_start - payload_start);
            }

            if (pad > cur->size || req > cur->size - pad) {
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
            if (req <= SIZE_MAX - sizeof(maia_mem_block_t) - MAIA_MEM_MIN_SPLIT &&
                cur->size >= req + sizeof(maia_mem_block_t) + MAIA_MEM_MIN_SPLIT) {
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
    }

    return NULL;
}

maia_status_t maia_mem_free(void *ptr) {
    if (ptr == NULL || g_region_count == 0U) {
        return MAIA_ERR_INVALID;
    }
    uintptr_t address = (uintptr_t)ptr;
    for (size_t i = 0U; i < g_region_count; ++i) {
        maia_mem_region_t *region = &g_regions[i];
        if (address < region->start + sizeof(maia_mem_block_t) ||
            address >= region->end || (address & 7U) != 0U) {
            continue;
        }
        uintptr_t block_address = address - sizeof(maia_mem_block_t);
        if (block_address < region->start ||
            block_address > region->end - sizeof(maia_mem_block_t)) {
            continue;
        }
        maia_mem_block_t *blk = (maia_mem_block_t *)block_address;
        if (address != block_address + sizeof(maia_mem_block_t) ||
            blk->magic != MAIA_MEM_MAGIC_USED || blk->used == 0U ||
            blk->size > region->end - address) {
            return MAIA_ERR_INVALID;
        }
        if ((blk->next != NULL &&
             (!block_in_region(region, blk->next) || blk->next->prev != blk)) ||
            (blk->prev != NULL &&
             (!block_in_region(region, blk->prev) || blk->prev->next != blk))) {
            return MAIA_ERR_INVALID;
        }

        blk->magic = MAIA_MEM_MAGIC_FREE;
        blk->used  = 0U;
        g_stats.used_bytes -= blk->size;
        g_stats.free_bytes += blk->size;
        g_stats.free_count += 1U;

        /* Coalesce only inside the owning region. */
        if (blk->next != NULL && blk->next->used == 0U && blk->next->magic == MAIA_MEM_MAGIC_FREE) {
            maia_mem_block_t *nxt = blk->next;
            blk->size += sizeof(maia_mem_block_t) + nxt->size;
            blk->next  = nxt->next;
            if (nxt->next != NULL) {
                nxt->next->prev = blk;
            }
            g_stats.free_bytes += sizeof(maia_mem_block_t);
        }

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
    return MAIA_ERR_INVALID;
}

maia_status_t maia_mem_get_alloc_size(const void *ptr, size_t *out_size) {
    if (ptr == NULL || out_size == NULL || g_region_count == 0U) {
        return MAIA_ERR_INVALID;
    }

    uintptr_t address = (uintptr_t)ptr;
    for (size_t i = 0U; i < g_region_count; ++i) {
        maia_mem_region_t *region = &g_regions[i];
        if (address < region->start + sizeof(maia_mem_block_t) ||
            address >= region->end || (address & 7U) != 0U) {
            continue;
        }

        uintptr_t block_address = address - sizeof(maia_mem_block_t);
        if (block_address < region->start ||
            block_address > region->end - sizeof(maia_mem_block_t)) {
            continue;
        }
        maia_mem_block_t *block = (maia_mem_block_t *)block_address;
        if (address != block_address + sizeof(maia_mem_block_t) ||
            block->magic != MAIA_MEM_MAGIC_USED || block->used == 0U ||
            block->size > region->end - address ||
            (block->next != NULL &&
             (!block_in_region(region, block->next) || block->next->prev != block)) ||
            (block->prev != NULL &&
             (!block_in_region(region, block->prev) || block->prev->next != block))) {
            return MAIA_ERR_INVALID;
        }
        *out_size = block->size;
        return MAIA_OK;
    }
    return MAIA_ERR_INVALID;
}

maia_status_t maia_mem_get_stats(maia_mem_stats_t *out_stats) {
    if (out_stats == NULL) {
        return MAIA_ERR_INVALID;
    }
    *out_stats = g_stats;
    return MAIA_OK;
}
