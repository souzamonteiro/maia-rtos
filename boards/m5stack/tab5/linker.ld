/*
 * MaiaRTOS Bare-Metal Linker Script for ESP32-P4 / M5Stack Tab5 (M0)
 *
 * Memory Map:
 * - Placed entirely in internal HP L2MEM [0x4FF00000 .. 0x4FF28000) (160 KiB),
 *   strictly below the ESP32-P4 ROM download/stack window [0x4FF296B8 .. 0x4FF40000).
 * - RAM/probe layout for `esptool.py load_ram`; this is not a validated
 *   second-stage application-image linker layout.
 * - The flash-address sections and application descriptor placement below
 *   must not be treated as proof of direct flash boot.
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    FLASH_RODATA (r)  : ORIGIN = 0x40000000, LENGTH = 64K
    FLASH_TEXT   (rx) : ORIGIN = 0x40010000, LENGTH = 64K
    L2MEM_LOW    (rwx): ORIGIN = 0x4FF00000, LENGTH = 160K
}

SECTIONS
{
    .flash.rodata : ALIGN(16)
    {
        KEEP(*(.flash.rodata))
    } > FLASH_RODATA

    .flash.text : ALIGN(16)
    {
        _flash_text_start = ABSOLUTE(.);
        KEEP(*(.flash.text))
        . = ALIGN(16);
    } > FLASH_TEXT

    .rodata_desc : ALIGN(16)
    {
        _rodata_desc_start = ABSOLUTE(.);
        KEEP(*(.rodata_desc))
        . = ALIGN(16);
    } > L2MEM_LOW

    .text : ALIGN(64)
    {
        _text_start = ABSOLUTE(.);
        KEEP(*(.text.entry))
        . = ALIGN(64);
        KEEP(*(.text.trap))
        *(.text .text.*)
        . = ALIGN(16);
        _text_end = ABSOLUTE(.);
    } > L2MEM_LOW

    .rodata : ALIGN(16)
    {
        _rodata_start = ABSOLUTE(.);
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        KEEP (*(SORT_BY_INIT_PRIORITY(.preinit_array.*)))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(16);
        _rodata_end = ABSOLUTE(.);
    } > L2MEM_LOW

    .data : ALIGN(16)
    {
        _data_start = ABSOLUTE(.);
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(16);
        _data_end = ABSOLUTE(.);
    } > L2MEM_LOW

    .tdata : ALIGN(16)
    {
        __maia_tls_tdata_start = ABSOLUTE(.);
        *(.tdata .tdata.*)
        . = ALIGN(16);
        __maia_tls_tdata_end = ABSOLUTE(.);
    } > L2MEM_LOW

    .tbss (NOLOAD) : ALIGN(16)
    {
        __bss_start = ABSOLUTE(.);
        *(.tbss .tbss.*)
        *(.tcommon)
        . = ALIGN(16);
        __maia_tls_block_end = ABSOLUTE(.);
    } > L2MEM_LOW

    .bss (NOLOAD) : ALIGN(16)
    {
        __global_pointer$ = . + 0x800;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = ABSOLUTE(.);
    } > L2MEM_LOW

    .stack (NOLOAD) : ALIGN(16)
    {
        __stack_bottom = ABSOLUTE(.);
        . += 16384; /* 16 KiB primary boot stack */
        . = ALIGN(16);
        __stack_top = ABSOLUTE(.);
    } > L2MEM_LOW

    /DISCARD/ :
    {
        *(.eh_frame .eh_frame.*)
        *(.comment)
    }
}

__maia_tls_alignment = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss));
ASSERT(__maia_tls_tdata_end - __maia_tls_tdata_start <= 1024,
       "TLS initialized template exceeds 1024 bytes")
ASSERT(__maia_tls_block_end - __maia_tls_tdata_start <= 1024,
       "TLS block exceeds 1024 bytes")
ASSERT(__maia_tls_alignment <= 64,
       "TLS alignment exceeds the supported 64-byte limit")
