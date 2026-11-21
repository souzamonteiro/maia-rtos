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

typedef struct mock_block_device {
    uint32_t reads;
    uint32_t writes;
    uint32_t first_block;
    uint32_t count;
    uint32_t completion;
    maia_sd_result_t result;
} mock_block_device_t;

static maia_sd_result_t read_block(void *context, uint32_t block,
                                   uint8_t *buffer) {
    mock_transport_t *mock = (mock_transport_t *)context;
    ++mock->calls;
    mock->block = block;
    memset(buffer, 0xA5, MAIA_SD_BLOCK_BYTES);
    return mock->result;
}

static maia_sd_result_t read_blocks(void *context, uint32_t first_block,
                                    uint32_t count, uint8_t *buffer,
                                    uint32_t *completed_blocks) {
    mock_block_device_t *mock = (mock_block_device_t *)context;
    ++mock->reads;
    mock->first_block = first_block;
    mock->count = count;
    uint32_t completed = mock->completion == UINT32_MAX ? count : mock->completion;
    for (uint32_t block = 0U; block < completed && block < count; ++block) {
        memset(buffer + (size_t)block * MAIA_SD_BLOCK_BYTES,
               (int)(0x40U + block), MAIA_SD_BLOCK_BYTES);
    }
    *completed_blocks = completed;
    return mock->result;
}

static maia_sd_result_t write_blocks(void *context, uint32_t first_block,
                                     uint32_t count, const uint8_t *buffer,
                                     uint32_t *completed_blocks) {
    mock_block_device_t *mock = (mock_block_device_t *)context;
    ++mock->writes;
    mock->first_block = first_block;
    mock->count = count;
    if (count != 0U && buffer[0] != 0xA7U) return MAIA_SD_IO_ERROR;
    uint32_t completed = mock->completion == UINT32_MAX ? count : mock->completion;
    *completed_blocks = completed;
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

    mock_block_device_t block_mock = {0U, 0U, 0U, 0U, UINT32_MAX, MAIA_SD_OK};
    maia_sd_block_device_t device = {0};
    assert(maia_sd_block_device_init(&device, &geometry, read_blocks,
                                    write_blocks, &block_mock) == MAIA_SD_OK);
    uint8_t multi_buffer[3U * MAIA_SD_BLOCK_BYTES] = {0};
    uint32_t completed = UINT32_MAX;
    assert(maia_sd_block_device_read(&device, 17U, 3U, multi_buffer,
                                     &completed) == MAIA_SD_OK);
    assert(completed == 3U && block_mock.reads == 1U &&
           block_mock.first_block == 17U && block_mock.count == 3U);
    for (size_t block = 0U; block < 3U; ++block) {
        for (size_t byte = 0U; byte < MAIA_SD_BLOCK_BYTES; ++byte) {
            assert(multi_buffer[block * MAIA_SD_BLOCK_BYTES + byte] ==
                   (uint8_t)(0x40U + block));
        }
    }
    memset(multi_buffer, 0xA7, sizeof multi_buffer);
    assert(maia_sd_block_device_write(&device, 19U, 3U, multi_buffer,
                                      &completed) == MAIA_SD_OK);
    assert(completed == 3U && block_mock.writes == 1U &&
           block_mock.first_block == 19U && block_mock.count == 3U);

    uint32_t reads_before = block_mock.reads;
    completed = UINT32_MAX;
    assert(maia_sd_block_device_read(&device, geometry.block_count - 2U, 3U,
                                     multi_buffer, &completed) ==
           MAIA_SD_OUT_OF_RANGE);
    assert(completed == 0U && block_mock.reads == reads_before);
    assert(maia_sd_block_device_read(&device, 0U, 0U, multi_buffer,
                                     &completed) == MAIA_SD_INVALID);
    assert(completed == 0U && block_mock.reads == reads_before);
    assert(maia_sd_block_device_read(NULL, 0U, 1U, multi_buffer,
                                     &completed) == MAIA_SD_INVALID);
    assert(maia_sd_block_device_read(&device, 0U, 1U, NULL,
                                     &completed) == MAIA_SD_INVALID);
    assert(maia_sd_block_device_read(&device, 0U, 1U, multi_buffer,
                                     NULL) == MAIA_SD_INVALID);
    assert(block_mock.reads == reads_before);

    block_mock.result = MAIA_SD_TIMEOUT;
    block_mock.completion = 1U;
    assert(maia_sd_block_device_read(&device, 20U, 3U, multi_buffer,
                                     &completed) == MAIA_SD_TIMEOUT);
    assert(completed == 1U);
    block_mock.result = MAIA_SD_OK;
    assert(maia_sd_block_device_read(&device, 20U, 3U, multi_buffer,
                                     &completed) == MAIA_SD_IO_ERROR);
    assert(completed == 1U);
    block_mock.completion = 4U;
    assert(maia_sd_block_device_read(&device, 20U, 3U, multi_buffer,
                                     &completed) == MAIA_SD_IO_ERROR);
    assert(completed == 0U);
    block_mock.result = MAIA_SD_NO_MEDIA;
    block_mock.completion = 0U;
    assert(maia_sd_block_device_read(&device, 20U, 3U, multi_buffer,
                                     &completed) == MAIA_SD_NO_MEDIA);
    assert(completed == 0U);
    block_mock.result = MAIA_SD_BUSY;
    assert(maia_sd_block_device_read(&device, 20U, 3U, multi_buffer,
                                     &completed) == MAIA_SD_BUSY);
    assert(completed == 0U);
    memset(multi_buffer, 0xA7, sizeof multi_buffer);
    block_mock.result = MAIA_SD_TIMEOUT;
    block_mock.completion = 2U;
    assert(maia_sd_block_device_write(&device, 20U, 3U, multi_buffer,
                                      &completed) == MAIA_SD_TIMEOUT);
    assert(completed == 2U);
    block_mock.result = MAIA_SD_OK;
    block_mock.completion = UINT32_MAX;

    maia_sd_block_device_t uninitialized = {0};
    assert(maia_sd_block_device_read(&uninitialized, 0U, 1U, multi_buffer,
                                     &completed) == MAIA_SD_NOT_READY);
    assert(completed == 0U);
    assert(maia_sd_block_device_init(&uninitialized, &geometry, NULL,
                                    write_blocks, &block_mock) == MAIA_SD_INVALID);
    maia_sd_geometry_t invalid_geometry = {0U};
    assert(maia_sd_block_device_init(&uninitialized, &invalid_geometry,
                                    read_blocks, write_blocks, &block_mock) ==
           MAIA_SD_INVALID);
    maia_sd_geometry_t max_geometry = {UINT64_C(1) << 32U};
    assert(maia_sd_block_device_init(&uninitialized, &max_geometry, read_blocks,
                                    write_blocks, &block_mock) == MAIA_SD_OK);
    assert(maia_sd_block_device_read(&uninitialized, UINT32_MAX, 1U,
                                     multi_buffer, &completed) == MAIA_SD_OK);
    assert(completed == 1U && block_mock.first_block == UINT32_MAX);
    assert(maia_sd_block_device_read(&uninitialized, UINT32_MAX, 2U,
                                     multi_buffer, &completed) ==
           MAIA_SD_OUT_OF_RANGE);
    assert(completed == 0U);
    max_geometry.block_count = (UINT64_C(1) << 32U) + 1U;
    assert(maia_sd_block_device_init(&uninitialized, &max_geometry, read_blocks,
                                    write_blocks, &block_mock) == MAIA_SD_INVALID);

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
