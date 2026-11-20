#include <maia/drivers/sd_read.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define READY_SDHC UINT32_C(0xC0000000)

typedef struct mock_transport {
    uint32_t calls;
    uint32_t block;
    maia_sd_result_t result;
} mock_transport_t;

static maia_sd_result_t read_block(void *context, uint32_t block,
                                   uint8_t *buffer) {
    mock_transport_t *mock = (mock_transport_t *)context;
    ++mock->calls;
    mock->block = block;
    memset(buffer, 0xA5, MAIA_SD_BLOCK_BYTES);
    return mock->result;
}

int main(void) {
    static const uint32_t sizes[] = {0U, 1U, 0xFFFFU, 0x10000U, 0x3FFFFFU};
    /* Synthetic CSD, sized to match the sector count logged by the old BSP. */
    uint32_t csd[4] = {0U, UINT32_C(0xDBD30000),
                       UINT32_C(0x00490001), UINT32_C(0x40000000)};
    maia_sd_geometry_t geometry = {0U};
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_OK);
    assert(geometry.block_count == UINT64_C(124735488));

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        uint32_t fixture[4] = {
            UINT32_MAX, (sizes[i] << 16U) | UINT32_C(0xABCD),
            UINT32_C(0x00490000) | (sizes[i] >> 16U),
            UINT32_C(0x40000000)
        };
        assert(maia_sd_sdhc_geometry(fixture, READY_SDHC, &geometry) == MAIA_SD_OK);
        assert(geometry.block_count == ((uint64_t)sizes[i] + 1U) * 1024U);
    }
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_OK);

    mock_transport_t mock = {0U, 0U, MAIA_SD_OK};
    uint8_t buffer[MAIA_SD_BLOCK_BYTES] = {0};
    assert(maia_sd_read_block(&geometry, read_block, &mock, 0U, buffer) == MAIA_SD_OK);
    assert(mock.calls == 1U && mock.block == 0U);
    for (size_t i = 0; i < sizeof(buffer); ++i) assert(buffer[i] == 0xA5U);
    assert(maia_sd_read_block(&geometry, read_block, &mock,
                              geometry.block_count - 1U, buffer) == MAIA_SD_OK);
    assert(mock.calls == 2U && mock.block == 124735487U);
    assert(maia_sd_read_block(&geometry, read_block, &mock,
                              geometry.block_count, buffer) == MAIA_SD_OUT_OF_RANGE);
    assert(maia_sd_read_block(&geometry, read_block, &mock,
                              UINT64_MAX, buffer) == MAIA_SD_OUT_OF_RANGE);
    assert(mock.calls == 2U);

    mock.result = MAIA_SD_TIMEOUT;
    assert(maia_sd_read_block(&geometry, read_block, &mock, 1U, buffer) == MAIA_SD_TIMEOUT);
    mock.result = MAIA_SD_IO_ERROR;
    assert(maia_sd_read_block(&geometry, read_block, &mock, 1U, buffer) == MAIA_SD_IO_ERROR);
    assert(mock.calls == 4U);
    assert(maia_sd_read_block(NULL, read_block, &mock, 0U, buffer) == MAIA_SD_INVALID);
    assert(maia_sd_read_block(&geometry, NULL, &mock, 0U, buffer) == MAIA_SD_INVALID);
    assert(maia_sd_read_block(&geometry, read_block, &mock, 0U, NULL) == MAIA_SD_INVALID);
    assert(mock.calls == 4U);

    geometry.block_count = 0U;
    assert(maia_sd_read_block(&geometry, read_block, &mock, 0U, buffer) == MAIA_SD_INVALID);
    geometry.block_count = 1025U;
    assert(maia_sd_read_block(&geometry, read_block, &mock, 0U, buffer) == MAIA_SD_INVALID);
    geometry.block_count = (UINT64_C(1) << 32) + 1024U;
    assert(maia_sd_read_block(&geometry, read_block, &mock, 0U, buffer) == MAIA_SD_INVALID);
    assert(mock.calls == 4U);

    csd[1] = UINT32_C(0xFFFF0000);
    csd[2] = UINT32_C(0x0049003F);
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_OK);
    assert(geometry.block_count == (UINT64_C(1) << 32));
    mock.result = MAIA_SD_OK;
    assert(maia_sd_read_block(&geometry, read_block, &mock,
                              UINT32_MAX, buffer) == MAIA_SD_OK);
    assert(mock.block == UINT32_MAX);
    assert(maia_sd_read_block(&geometry, read_block, &mock,
                              UINT64_C(1) << 32, buffer) == MAIA_SD_OUT_OF_RANGE);

    geometry.block_count = 123U;
    assert(maia_sd_sdhc_geometry(NULL, READY_SDHC, &geometry) == MAIA_SD_INVALID);
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, NULL) == MAIA_SD_INVALID);
    assert(maia_sd_sdhc_geometry(csd, 0U, &geometry) == MAIA_SD_NOT_READY);
    assert(maia_sd_sdhc_geometry(csd, UINT32_C(0x80000000), &geometry) == MAIA_SD_UNSUPPORTED);
    csd[3] = 0U;
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_UNSUPPORTED);
    csd[3] = UINT32_C(0x80000000);
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_UNSUPPORTED);
    csd[3] = UINT32_C(0x40000000);
    csd[2] = UINT32_C(0x0048003F);
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_UNSUPPORTED);
    csd[2] = UINT32_C(0x0009003F);
    assert(maia_sd_sdhc_geometry(csd, READY_SDHC, &geometry) == MAIA_SD_UNSUPPORTED);
    assert(geometry.block_count == 123U);
    return 0;
}
