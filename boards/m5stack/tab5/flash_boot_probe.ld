/*
 * Native second-stage flash-image smoke-test layout for ESP32-P4.
 * The DROM/IROM map is based on the pinned ESP-IDF v5.4.2 P4 linker template.
 * This is a bounded probe image, not the final product memory layout.
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    FLASH_IROM (rx)  : ORIGIN = 0x40000020, LENGTH = 0x04000000 - 0x20
    FLASH_DROM (r)   : ORIGIN = 0x40000020, LENGTH = 0x04000000 - 0x20
    SRAM_LOW   (rwx) : ORIGIN = 0x4FF00000, LENGTH = 0x28000
}

PHDRS
{
    flash_text PT_LOAD FLAGS(5);
    flash_rodata PT_LOAD FLAGS(4);
    core1_text PT_LOAD FLAGS(5);
    sram_data PT_LOAD FLAGS(6);
    tls PT_TLS FLAGS(6);
}

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
    } > FLASH_IROM :flash_text

    .flash_rodata_dummy ORIGIN(FLASH_DROM) (NOLOAD) : SUBALIGN(16)
    {
        _flash_rodata_dummy_start = ABSOLUTE(.);
        . = ALIGN(ALIGNOF(.flash.text)) + SIZEOF(.flash.text);
        . = ALIGN(0x10000) + 0x20;
    } > FLASH_DROM :NONE

    .flash.appdesc : ALIGN(16)
    {
        _rodata_desc_start = ABSOLUTE(.);
        KEEP(*(.rodata_desc .rodata_desc.*))
        KEEP(*(.rodata_custom_desc .rodata_custom_desc.*))
        . = ALIGN(16);
    } > FLASH_DROM :flash_rodata

    .flash.rodata : ALIGN(16)
    {
        _flash_rodata_start = ABSOLUTE(.);
        KEEP(*(.flash.rodata))
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        *(.gcc_except_table .gcc_except_table.*)
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
        _flash_rodata_end = ABSOLUTE(.);
    } > FLASH_DROM :flash_rodata

    ASSERT(ADDR(.flash.rodata) == ADDR(.flash.appdesc) + SIZEOF(.flash.appdesc),
           "Application descriptor must be contiguous with DROM read-only data")
    ASSERT(SIZEOF(.flash.appdesc) >= 256,
           "ESP application descriptor is missing or truncated")

    .maia_core1 : ALIGN(64)
    {
        KEEP(*(.maia_core1.text))
    } > SRAM_LOW :core1_text

    .data : ALIGN(16)
    {
        _data_start = ABSOLUTE(.);
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(16);
        _data_end = ABSOLUTE(.);
    } > SRAM_LOW :sram_data

    .tdata : ALIGN(16)
    {
        __maia_tls_tdata_start = ABSOLUTE(.);
        *(.tdata .tdata.*)
        . = ALIGN(16);
        __maia_tls_tdata_end = ABSOLUTE(.);
    } > SRAM_LOW :sram_data :tls

    .tbss (NOLOAD) : ALIGN(16)
    {
        __bss_start = ABSOLUTE(.);
        *(.tbss .tbss.*)
        *(.tcommon)
        . = ALIGN(16);
        __maia_tls_block_end = ABSOLUTE(.);
    } > SRAM_LOW :sram_data :tls

    .bss (NOLOAD) : ALIGN(16)
    {
        __global_pointer$ = . + 0x800;
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = ABSOLUTE(.);
    } > SRAM_LOW :sram_data

    .stack (NOLOAD) : ALIGN(16)
    {
        __stack_bottom = ABSOLUTE(.);
        . += 16384;
        . = ALIGN(16);
        __stack_top = ABSOLUTE(.);
    } > SRAM_LOW :sram_data

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
