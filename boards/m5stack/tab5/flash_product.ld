/*
 * Flash-XIP product layout baseline for ESP32-P4 / M5Stack Tab5.
 * PSRAM is runtime-only: no loadable section is placed there before its
 * native startup, cache and MMU mapping have been accepted on the target.
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    FLASH_IROM (rx)  : ORIGIN = 0x40000020, LENGTH = 0x04000000 - 0x20
    FLASH_DROM (r)   : ORIGIN = 0x40000020, LENGTH = 0x04000000 - 0x20
    SRAM_LOW   (rwx) : ORIGIN = 0x4FF00000, LENGTH = 0x28000
}

__maia_psram_window_start = 0x4B800000;
__maia_psram_window_end = 0x4BA00000;

SECTIONS
{
    .flash.text ORIGIN(FLASH_IROM) : SUBALIGN(16)
    {
        _text_start = ABSOLUTE(.);
        KEEP(*(.text.entry))
        . = ALIGN(64);
        KEEP(*(.text.trap))
        *(.text .text.*)
        KEEP(*(.flash.text))
        . += 16;
        . = ALIGN(16);
        _text_end = ABSOLUTE(.);
    } > FLASH_IROM

    .flash_rodata_dummy ORIGIN(FLASH_DROM) (NOLOAD) : SUBALIGN(16)
    {
        _flash_rodata_dummy_start = ABSOLUTE(.);
        . = ALIGN(ALIGNOF(.flash.text)) + SIZEOF(.flash.text);
        . = ALIGN(0x10000) + 0x20;
    } > FLASH_DROM

    .flash.appdesc : ALIGN(16)
    {
        _rodata_desc_start = ABSOLUTE(.);
        KEEP(*(.rodata_desc .rodata_desc.*))
        KEEP(*(.rodata_custom_desc .rodata_custom_desc.*))
        . = ALIGN(16);
    } > FLASH_DROM

    .flash.rodata : ALIGN(16)
    {
        _flash_rodata_start = ABSOLUTE(.);
        KEEP(*(.flash.rodata))
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        . = ALIGN(16);
        _flash_rodata_end = ABSOLUTE(.);
    } > FLASH_DROM

    ASSERT(ADDR(.flash.appdesc) == ADDR(.flash_rodata_dummy) + SIZEOF(.flash_rodata_dummy),
           "Application descriptor must follow the reserved DROM page")
    ASSERT(ADDR(.flash.rodata) == ADDR(.flash.appdesc) + SIZEOF(.flash.appdesc),
           "Application descriptor must be contiguous with DROM read-only data")
    ASSERT(SIZEOF(.flash.appdesc) >= 256,
           "ESP application descriptor is missing or truncated")

    .data : ALIGN(16)
    {
        _data_start = ABSOLUTE(.);
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(16);
        _data_end = ABSOLUTE(.);
    } > SRAM_LOW

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = ABSOLUTE(.);
        __global_pointer$ = . + 0x800;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = ABSOLUTE(.);
    } > SRAM_LOW

    .stack (NOLOAD) : ALIGN(16)
    {
        __stack_bottom = ABSOLUTE(.);
        . += 16384;
        . = ALIGN(16);
        __stack_top = ABSOLUTE(.);
    } > SRAM_LOW

    /DISCARD/ :
    {
        *(.eh_frame .eh_frame.*)
        *(.comment)
    }
}

ASSERT(_start >= ADDR(.flash.text) && _start < ADDR(.flash.text) + SIZEOF(.flash.text),
       "Entry point must be in the flash-mapped IROM section")
ASSERT(__bss_end <= __stack_bottom, "BSS overlaps the primary boot stack")
ASSERT(__stack_top <= ORIGIN(SRAM_LOW) + LENGTH(SRAM_LOW),
       "Internal SRAM image exceeds the validated low-memory window")
ASSERT(__maia_psram_window_end - __maia_psram_window_start == 0x200000,
       "PSRAM runtime window must remain the qualified 2 MiB CPU mapping")
