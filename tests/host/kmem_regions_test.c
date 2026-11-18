#include <maia/kernel.h>
#include <stdint.h>

#define REGION_BYTES 2048U
#define REGION_COUNT 5U

static _Alignas(64) uint8_t pools[REGION_COUNT][REGION_BYTES];

static int in_region(const void *ptr, unsigned index) {
    uintptr_t address = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)&pools[index][0];
    return address >= start && address < start + REGION_BYTES;
}

int main(void) {
    if (maia_mem_add_region(pools[0], REGION_BYTES) != MAIA_ERR_INVALID) return 1;
    if (maia_mem_init(pools[0], REGION_BYTES) != MAIA_OK) return 2;
    if (maia_mem_add_region(NULL, REGION_BYTES) != MAIA_ERR_INVALID) return 3;
    if (maia_mem_add_region(pools[0] + 512, 512) != MAIA_ERR_INVALID) return 4;
    if (maia_mem_add_region(pools[0], REGION_BYTES) != MAIA_ERR_INVALID) return 5;
    if (maia_mem_add_region(pools[1], 16) != MAIA_ERR_INVALID) return 6;

    maia_mem_stats_t initial;
    if (maia_mem_get_stats(&initial) != MAIA_OK || initial.total_bytes == 0U) return 7;
    if (maia_mem_add_region(pools[1], REGION_BYTES) != MAIA_OK) return 8;

    maia_mem_stats_t combined;
    if (maia_mem_get_stats(&combined) != MAIA_OK ||
        combined.total_bytes <= initial.total_bytes ||
        combined.free_bytes != combined.total_bytes) return 9;

    void *aligned = maia_mem_alloc_aligned(128U, 64U);
    void *second_region = maia_mem_alloc(1800U);
    if (aligned == NULL || second_region == NULL) return 10;
    if (((uintptr_t)aligned & 63U) != 0U || !in_region(aligned, 0U) ||
        !in_region(second_region, 1U)) return 11;
    if (maia_mem_alloc(SIZE_MAX) != NULL ||
        maia_mem_alloc_aligned(32U, 3U) != NULL) return 12;

    if (maia_mem_free((void *)(uintptr_t)(UINTPTR_MAX - 15U)) != MAIA_ERR_INVALID ||
        maia_mem_free(pools[0] + 8U) != MAIA_ERR_INVALID) return 13;
    if (maia_mem_free(aligned) != MAIA_OK) return 14;
    if (maia_mem_free(aligned) != MAIA_ERR_INVALID) return 15;
    if (maia_mem_free(second_region) != MAIA_OK) return 16;
    maia_mem_stats_t released;
    if (maia_mem_get_stats(&released) != MAIA_OK ||
        released.used_bytes != 0U || released.free_bytes != released.total_bytes) return 17;

    if (maia_mem_add_region(pools[2], REGION_BYTES) != MAIA_OK ||
        maia_mem_add_region(pools[3], REGION_BYTES) != MAIA_OK) return 18;
    maia_mem_stats_t full;
    if (maia_mem_get_stats(&full) != MAIA_OK) return 19;
    if (maia_mem_add_region(pools[4], REGION_BYTES) != MAIA_ERR_NOMEM) return 20;
    maia_mem_stats_t unchanged;
    if (maia_mem_get_stats(&unchanged) != MAIA_OK ||
        unchanged.total_bytes != full.total_bytes) return 21;

    if (maia_mem_init(NULL, REGION_BYTES) != MAIA_ERR_INVALID) return 22;
    if (maia_mem_init((void *)(uintptr_t)(UINTPTR_MAX - 15U), 64U) != MAIA_ERR_INVALID) return 23;
    if (maia_mem_init(pools[0] + 1U, REGION_BYTES - 1U) != MAIA_OK) return 24;
    maia_mem_stats_t reset;
    if (maia_mem_get_stats(&reset) != MAIA_OK ||
        reset.total_bytes >= full.total_bytes || reset.used_bytes != 0U) return 25;

    return 0;
}
