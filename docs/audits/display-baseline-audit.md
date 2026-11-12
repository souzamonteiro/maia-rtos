# Display baseline preservation and native migration audit — 2026-11-11

**AUDITED; legacy boot/application rechecked on hardware. Native display NOT RUN.**
This unit changes documentation/evidence only. It does not remove a runtime
component or replace the complete desktop. No firmware load, flash or SD write.
A USB/ROM connection and hardware reset restarted the preserved flash image.

## Reproducible reference and recovery

Functional source baseline: `bf044d37e43c3b8db2011d6cbf81c9df2d2e1649`.
[Manifest](display-baseline-manifest.json) pins artifacts and inspected sources.
Verified copies of every original recovery-manifest artifact, baseline/current
sources, four SDK references, capture scripts and full raw logs are in:
`../maia-rtos-recovery/display-baseline-audit-20261111/` (outside Git).
This is an artifact checkpoint, **not a full flash/SD backup**.

- Preserved application binary SHA256:
  `21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd`.
- Preserved ELF SHA256:
  `f486955925d7fa85539d4f5b8381ea187a04c9d7bb5df37e175a7260f6f1e45c`.
- Current `platforms/tab5/build/m5stack_tab5.bin` is a different experimental
  artifact (SHA256 in manifest); **do not use it as the preserved baseline**.
- Boot reports ELF prefix `f48695592`, consistent with the preserved ELF.
  No full flash readback/hash was performed in this unit.
- Active BSP/ST7121 driver/sdkconfig equal baseline bytes. Current app_main
  changes worker affinity; its display initialization and fb_sync are unchanged.
  Native display/board-I/O files equal the experimental checkout's versions;
  that checkout is not an independently validated display correction.

## Actual selected path and configuration

Boot reports touch address 0x55 firmware **1**, and explicitly selects **ST7121**.
The later “ST7123 Display initialized” log is a generic message in the BSP.
This is firmware-based detection, not an independent LCD-ID/package measurement.
Boot also reports 32 MiB PSRAM at 200 MHz and CPU at 360 MHz. These are legacy
observations, not established native cold-start settings.

The active call in `platforms/tab5/main/app_main.cpp` is
`bsp_display_new_with_handles_to_st7123`, despite the generic name. It detects
the variant before creating the panel. Its explicit configuration overrides
other generic macros; do not port parameters from an unused header macro.

| Property | Functional ST7121 branch | Existing native attempt |
|---|---|---|
| Image | Physical 720×1280, logical 1280×720, rotation 3 | Same intended geometry |
| Pixel storage | RGB565, 1 framebuffer, 1,843,200 bytes | Fixed PSRAM address plus SRAM bands |
| DSI | 2 lanes, requested 965 Mbps/lane | Source config/comments target 960 Mbps |
| DPI clock | Requested 70 MHz | Source config/comments target 80 MHz |
| Horizontal sync/back/front | 2 / 40 / 40 | 2 / 40 / 40 |
| Vertical sync/back/front | 20 / 24 / 200 | 20 / 24 / 200 for grouped ST712x branch |
| Framebuffer | SDK-aligned PSRAM allocation returned by panel API | Hardcoded 0x48100000 |
| Cache publication | esp_cache_msync C2M at creation/draw and app fb_sync | Global ROM writeback + fence |
| Refresh | DW_GDMA full-frame completion ISR rearms descriptor | Circular 10-block path and refresh polling |

These clock values describe source requests; no oscilloscope or native register
readback establishes their actual output frequencies in this audit. The baseline
panel-device config also says bits_per_pixel=24, but the ST7121 wrapper forwards
the DPI RGB565 configuration; its framebuffer remains 16 bits/pixel. Changing
that field alone is not a justified fix.

A host C compiler evaluated isolated **data declarations only**, producing a
canonical command/length/delay/payload listing for each table. All **30 ST7121
entries match exactly**, including SLPOUT 80 ms and DISPON 800 ms. Canonical
SHA256 is in the manifest. Outside the table, the baseline requests an extra
5 ms per command while native delays 1 ms; tick rounding and the native delay
clock assumption must be accounted for. Comparison inputs, host executables and
output listings remain in recovery. No vendor driver implementation was linked
into MaiaRTOS by this comparison.

ELF `nm` confirms retained ST7121/ST7123 constructors and the BSP wrapper;
`lv_init` is absent. This corrects the initial status shorthand that grouped
vendor panel drivers with discarded LVGL. The working desktop draws directly
into the framebuffer; migrating LVGL is not a prerequisite established here.

## Dependencies and concrete native gaps

1. **Identification, reset, power and timing.** Baseline detects touch firmware
   over system I2C (GPIO31/32), preserves other expander bits, and releases LCD
   reset P4 using input/pull-up. Its reset sequence includes 100 ms stages.
   Manufacturer documentation independently requires this high-state mode.
   Baseline acquires LDO channel 3 at 2500 mV; native fixed programming has not
   established equivalent calibration or failure handling. Native bit-bang delay
   assumes 400 MHz; baseline's observed 360 MHz is not a native clock measurement.
   I2C stretch timeout/error propagation and ownership need explicit contracts.
2. **PSRAM startup and allocation.** Legacy CPU startup calls esp_psram_init and
   uses configured memory mapping/XIP. Native RAM entry does not execute that
   startup. A write/read probe at a fixed PSRAM address is neither initialization
   nor ownership proof, and can fault before its fallback. A warm success cannot
   establish independent cold boot. The preserved configuration sets L2 lines
   to **128 bytes**, L1 to 64; the old universal 64-byte L2 assumption is withdrawn.
3. **SRAM and DMA ownership.** Native bands start at 0x4FF20000, 0x4FF4D000 and
   0x4FF7A000, each 0x2D000 bytes. The first extends past the diagnostic RAM-load
   window and intersects its ROM-reserved interval 0x4FF296B8..0x4FF40000. The
   current native linker does not reserve these bands. Do not run this display
   alongside the validated RAM scheduler probes without a reviewed memory map.
   Native CPU/DMA aliases and descriptor placement also need proof.
4. **Cache call: verified symbol, unresolved policy.** SDK ROM table binds
   Cache_WriteBack_All to **0x4FC00414**; header declares
   `int Cache_WriteBack_All(uint32_t map)`, with bit4 L1 data / bit5 L2. Native's
   address/signature/0x30 mask agree. This audit does not establish the call's
   complete revision-specific safety, latency or DMA coherency contract; it is
   incorrect to diagnose an address mismatch from these sources.
5. **Bounded failure and refresh ownership.** Native command/PHY waits can expire
   without propagating initialization failure; readiness is not a reliable
   success contract. ST7123 and unknown detection use the ST7121 table, although
   the BSP has distinct variants. Native refresh is reachable from console and
   timer IRQ paths; a single DMA owner and bounded IRQ behavior remain undefined.
   SDK refresh handles completion/underrun and rearms a full-frame transfer.
6. **Runtime boundary.** Baseline still requires heap capabilities, cache/MMU,
   clocks/power, interrupt allocation, DMA channels/descriptors and (when DMA2D
   is configured) FreeRTOS draw semaphore/ISR signaling. Port the required
   behavior through independent native contracts, not compatibility shims or
   transplantation of the SDK implementation.

## Validation and next small unit

- Verified all preserved artifact hashes against the original manifest.
- Compared native/vendor ST7121 tables with host `cc -std=c11 -Wall -Wextra
  -Werror`: **30/30 identical entries**. SDK reference files match their pinned
  HEAD bytes. This is a source/data comparison, not a device display test.
- Captured actual preserved-image boot after reset; relevant unmodified lines
  are retained in [filtered boot evidence](display-baseline-boot.txt). Full raw
  capture, including unrelated device enumeration, stays in recovery.
- Existing maia.py control verification: shell echo, SciCalc `2+3*4=14`, module
  exit 0, desktop left active: **PASS**. This verifies serial behavior, not pixels.
  [Control capture](display-baseline-control.txt).
- No runtime code/config/parser changed. Host kernel regressions were not rerun
  for this documentation-only unit. Native display/PSRAM/cold-boot tests: **NOT RUN**.

Next: audit and implement a bounded, status-returning native system-I2C
identification transaction, then test touch firmware detection in an isolated RAM
probe without expander writes, LDO changes, panel commands or DMA. Verify timeout
and missing-device cases before proceeding separately to reset/power, memory/cache
and display stages. Preserve/reset to the known flash desktop after each probe.

## Primary references

- [M5Stack Tab5 documentation](https://docs.m5stack.com/en/core/Tab5), unversioned
  page consulted 2026-11-11: Specifications, PinMap, Tab5 Screen Reset, Version
  Change. Documents display variants, I2C pins and safe reset release; the
  2026-04-28 variant change is not proof of this unit's physical PCB date.
- [Preserved BSP source](https://github.com/souzamonteiro/maia-rtos/blob/bf044d37e43c3b8db2011d6cbf81c9df2d2e1649/platforms/tab5/components/m5stack_tab5/m5stack_tab5.c),
  functions bsp_detect_display_type, bsp_reset_tp,
  bsp_display_new_with_handles_to_st7123; companion ST7121 driver and app_main
  pinned in manifest. Primary evidence for this firmware's requested parameters.
- ESP-IDF revision `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
  [DPI driver](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c),
  sections dma_trans_done_cb, framebuffer allocation and draw semaphore;
  cpu_start.c PSRAM startup; esp32p4.rom.ld Cache_WriteBack_All;
  esp32p4/include/esp32p4/rom/cache.h writeback declaration/map. These are reference
  observations, not permission to copy runtime code. Exact hashes in manifest.
