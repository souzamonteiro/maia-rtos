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
__maia_psram_window_end = 0x4BC00000;

PHDRS
{
    flash_text PT_LOAD FLAGS(5);
    flash_rodata PT_LOAD FLAGS(4);
    psram_iram PT_LOAD FLAGS(5);
    sram_data PT_LOAD FLAGS(6);
    tls PT_TLS FLAGS(6);
}

SECTIONS
{
    .maia_psram_iram : ALIGN(16)
    {
        _maia_psram_iram_start = ABSOLUTE(.);
        KEEP(*soc_psram_boot.c.obj(.text.maia_esp32p4_psram_boot_initialize200))
        KEEP(*soc_psram_window.c.obj(.text.maia_esp32p4_psram_cache_boot))
        KEEP(*soc_psram_window.c.obj(.text.maia_esp32p4_psram_window_start))
        *soc_psram_boot.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_clock.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_supply.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_mpll.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_array.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_mode.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_snapshot.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        *soc_psram_window.c.obj(.text .text.* .rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(16);
        _maia_psram_iram_end = ABSOLUTE(.);
    } > SRAM_LOW :psram_iram

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

ASSERT(_start >= ADDR(.flash.text) && _start < ADDR(.flash.text) + SIZEOF(.flash.text),
       "Entry point must be in the flash-mapped IROM section")
ASSERT(SIZEOF(.maia_psram_iram) > 0, "PSRAM SRAM-resident startup closure is empty")
ASSERT(maia_esp32p4_psram_boot_initialize200 >= _maia_psram_iram_start &&
       maia_esp32p4_psram_boot_initialize200 < _maia_psram_iram_end,
       "PSRAM boot composition must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_cache_boot >= _maia_psram_iram_start &&
       maia_esp32p4_psram_cache_boot < _maia_psram_iram_end,
       "PSRAM cache bootstrap must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_cache_adopt >= _maia_psram_iram_start &&
       maia_esp32p4_psram_cache_adopt < _maia_psram_iram_end,
       "PSRAM cache adoption must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_window_start >= _maia_psram_iram_start &&
       maia_esp32p4_psram_window_start < _maia_psram_iram_end,
       "PSRAM MMU window setup must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_snapshot >= _maia_psram_iram_start &&
       maia_esp32p4_psram_snapshot < _maia_psram_iram_end,
       "PSRAM snapshot dependency must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_supply1900 >= _maia_psram_iram_start &&
       maia_esp32p4_psram_supply1900 < _maia_psram_iram_end,
       "PSRAM supply setup must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_mpll_initialize400 >= _maia_psram_iram_start &&
       maia_esp32p4_psram_mpll_initialize400 < _maia_psram_iram_end,
       "PSRAM MPLL setup must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_profile200 >= _maia_psram_iram_start &&
       maia_esp32p4_psram_profile200 < _maia_psram_iram_end,
       "PSRAM profile setup must execute from internal SRAM")
ASSERT(maia_esp32p4_psram_read_mode >= _maia_psram_iram_start &&
       maia_esp32p4_psram_read_mode < _maia_psram_iram_end,
       "PSRAM mode-register reads must execute from internal SRAM")
ASSERT(maia_psram_clock_mhz >= _maia_psram_iram_start &&
       maia_psram_clock_mhz < _maia_psram_iram_end,
       "PSRAM clock-state query must execute from internal SRAM")
ASSERT(maia_psram_x8_claimed >= _maia_psram_iram_start &&
       maia_psram_x8_claimed < _maia_psram_iram_end,
       "PSRAM mode-state query must execute from internal SRAM")
ASSERT(__bss_end <= __stack_bottom, "BSS overlaps the primary boot stack")
ASSERT(__stack_top <= ORIGIN(SRAM_LOW) + LENGTH(SRAM_LOW),
       "Internal SRAM image exceeds the validated low-memory window")
ASSERT(__maia_psram_window_end - __maia_psram_window_start == 0x400000,
       "PSRAM runtime window must reserve 4 MiB for LCD double buffering")

NOCROSSREFS_TO(.flash.text .maia_psram_iram)
NOCROSSREFS_TO(.flash.rodata .maia_psram_iram)
