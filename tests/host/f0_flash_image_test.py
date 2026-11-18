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
    args = sys.argv[1:]
    sram_window = None
    if "--sram-window" in args:
        index = args.index("--sram-window")
        if index + 1 >= len(args):
            fail("--sram-window requires START:END")
        try:
            start_text, end_text = args[index + 1].split(":", 1)
            sram_window = (int(start_text, 0), int(end_text, 0))
        except ValueError:
            fail("--sram-window must be START:END")
        del args[index:index + 2]
    if len(args) != 1:
        fail("usage: f0_flash_image_test.py IMAGE [--sram-window START:END]")

    image = Path(args[0]).read_bytes()
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

    if sram_window is not None:
        sram_start, sram_end = sram_window
        if sram_start >= sram_end:
            fail("SRAM window must have a positive size")
        for load_address, length, _, _ in segments:
            if load_address < 0x10000000 or FLASH_MAP_START <= load_address < FLASH_MAP_END:
                continue
            if not sram_start <= load_address < load_address + length <= sram_end:
                fail(
                    f"non-mapped segment {load_address:#x}+{length:#x} "
                    f"falls outside SRAM window {sram_start:#x}:{sram_end:#x}"
                )

    print(
        f"F0 flash image structure PASS: {segment_count} segments, "
        f"entry={entry:#010x}, app_desc={first_address:#010x}"
    )


if __name__ == "__main__":
    main()
