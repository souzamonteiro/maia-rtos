#!/usr/bin/env python3
import struct
import sys
from pathlib import Path


IMAGE_MAGIC = 0xE9
APP_DESC_MAGIC = 0xABCD5432
FLASH_MAP_START = 0x40000000
FLASH_MAP_END = 0x44000000
FLASH_MMU_PAGE_SIZE = 0x10000


def fail(message):
    raise SystemExit(f"F0 flash image validation failed: {message}")


def main():
    if len(sys.argv) != 2:
        fail("usage: f0_flash_image_test.py IMAGE")

    image = Path(sys.argv[1]).read_bytes()
    if len(image) < 32 or image[0] != IMAGE_MAGIC:
        fail("missing ESP image header")

    segment_count = image[1]
    entry = struct.unpack_from("<I", image, 4)[0]
    if segment_count == 0:
        fail("image has no loadable segments")

    segments = []
    offset = 24
    for index in range(segment_count):
        if offset + 8 > len(image):
            fail(f"segment {index} header is truncated")
        load_address, length = struct.unpack_from("<II", image, offset)
        data_start = offset + 8
        data_end = data_start + length
        if data_end > len(image):
            fail(f"segment {index} data is truncated")
        segments.append((load_address, length, data_start, image[data_start:data_end]))
        offset = data_end

    first_address, first_length, _, first_data = segments[0]
    if not FLASH_MAP_START <= first_address < FLASH_MAP_END:
        fail(f"first segment is not in the flash-mapped window: {first_address:#x}")
    if first_length < 256 or struct.unpack_from("<I", first_data)[0] != APP_DESC_MAGIC:
        fail("first DROM segment does not begin with esp_app_desc_t")

    mapped_segments = []
    for load_address, _, data_start, _ in segments:
        if FLASH_MAP_START <= load_address < FLASH_MAP_END:
            if data_start % FLASH_MMU_PAGE_SIZE != load_address % FLASH_MMU_PAGE_SIZE:
                fail(f"flash MMU alignment mismatch at segment {load_address:#x}")
            mapped_segments.append(load_address)

    if len(mapped_segments) != 2:
        fail(f"expected exactly two shared D/I flash mappings, found {len(mapped_segments)}")

    if not FLASH_MAP_START <= entry < FLASH_MAP_END:
        fail(f"entry point is not flash mapped: {entry:#x}")
    if not any(
        start <= entry < start + length
        for start, length, _, _ in segments
    ):
        fail(f"entry point {entry:#x} is not inside a loadable segment")

    print(
        f"F0 flash image structure PASS: {segment_count} segments, "
        f"entry={entry:#010x}, app_desc={first_address:#010x}"
    )


if __name__ == "__main__":
    main()
