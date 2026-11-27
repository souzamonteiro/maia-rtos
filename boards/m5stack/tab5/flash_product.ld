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
    /* Runtime-only L2MEM (never loaded by the bootloader):
     * - RECLAIM: 2nd-stage bootloader area, free once the app runs; still in
     *   the executable range and below the ROM stack/data (0x4FF3AFC0).
     * - HIGH: above the ROM-reserved window and below the 256 KiB L2 cache
     *   (0x4FF80000..0x4FFC0000). .maia_high_bss is zeroed by start.S; the
     *   heap part is executable under the bootloader PMP (whole L2MEM RWX)
     *   and is probed at boot before being handed to the allocator. */
    SRAM_RECLAIM (rwx) : ORIGIN = 0x4FF28000, LENGTH = 0x12FC0
    SRAM_HIGH  (rwx) : ORIGIN = 0x4FF40000, LENGTH = 0x40000
}

__maia_sram_reclaim_start = ORIGIN(SRAM_RECLAIM);
__maia_sram_reclaim_end = ORIGIN(SRAM_RECLAIM) + LENGTH(SRAM_RECLAIM);

__maia_psram_window_start = 0x4B800000;
__maia_psram_window_end = 0x4BC00000;

PHDRS
{
    flash_text PT_LOAD FLAGS(5);
    flash_rodata PT_LOAD FLAGS(4);
    flash_iram PT_LOAD FLAGS(5);
    psram_iram PT_LOAD FLAGS(5);
    sram_data PT_LOAD FLAGS(6);
    tls PT_TLS FLAGS(6);
}

SECTIONS
{
    .maia_flash_iram : ALIGN(16)
    {
        _maia_flash_iram_start = ABSOLUTE(.);
        KEEP(*soc_flash.c.obj(.maia_flash_iram))
        KEEP(*soc_smp.c.obj(.maia_flash_iram))
        . = ALIGN(16);
        _maia_flash_iram_end = ABSOLUTE(.);
    } > SRAM_LOW :flash_iram

    /* Core-1 SRAM-only worker: shares the flash_iram load segment so the
     * product keeps its validated segment layout. */
    .maia_core1 : ALIGN(64)
    {
        KEEP(*(.maia_core1.text))
        . = ALIGN(16);
    } > SRAM_LOW :flash_iram

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

    .maia_high_bss (NOLOAD) : ALIGN(16)
    {
        __maia_high_bss_start = ABSOLUTE(.);
        *(.maia_high_bss .maia_high_bss.*)
        . = ALIGN(16);
        __maia_high_bss_end = ABSOLUTE(.);
    } > SRAM_HIGH :NONE

    /* Brings the total heap to ~256 KiB; the rest of SRAM_HIGH stays free. */
    .maia_high_heap (NOLOAD) : ALIGN(16)
    {
        __maia_high_heap_start = ABSOLUTE(.);
        . += 0x1A000;
        __maia_high_heap_end = ABSOLUTE(.);
    } > SRAM_HIGH :NONE

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
ASSERT(SIZEOF(.maia_flash_iram) > 0, "Flash transaction closure is empty")
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
ASSERT(__maia_high_heap_end <= ORIGIN(SRAM_HIGH) + LENGTH(SRAM_HIGH),
       "High SRAM data overlaps the L2 cache")
ASSERT(__maia_psram_window_end - __maia_psram_window_start == 0x400000,
       "PSRAM runtime window must reserve 4 MiB for LCD double buffering")

NOCROSSREFS_TO(.flash.text .maia_psram_iram)
NOCROSSREFS_TO(.flash.rodata .maia_psram_iram)
NOCROSSREFS_TO(.flash.text .maia_flash_iram)
NOCROSSREFS_TO(.flash.rodata .maia_flash_iram)
