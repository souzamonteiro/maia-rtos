# Native SRAM copy and active VDMA stop — 2026-11-12

**HARDWARE-TESTED, LIMITED: channel0 SRAM single-block copy and active stop.** Diagnostic-only channel0 fixture. Does not
change the production display, clocks, PHY, PSRAM, flash or SD. No parser change.

## Prerequisites, defects and pinned primary evidence

The previous [bounded stop](native-vdma-stop.md) tested only an empty controller.
A real transfer needs explicit memory reservation, cache ownership and proof that
the channel remains enabled when the native stop routine inspects it. A fully
completed copy cannot pass as an interrupted transfer.

[ESP32-P4 chip1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf):
section7.3.1 pp451–454 defines CPU non-cacheable L2MEM alias8FF00000–8FFBFFFF,
physical/cached L2MEM4FF00000–4FFBFFFF, DMA access to SRAM. Section5.7.4 p343
specifies contiguous single-block programming. Section5.5.7 pp337–338 and
register5.3 p349 establish early disable/global completion, as pinned before.
Register5.9 onward supplies channel layout; source/destination memory-master1,
8-bit width, increment mode and burst1 are cross-checked with the SDK below.

Pinned SDK **f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0**, local owner references:

- [test_dw_gdma.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/test_apps/dma/main/test_dw_gdma.c),
  M2M contiguous case: internal SRAM, cache publication and validation model.
  No Unity, FreeRTOS task/semaphore/callback or heap code imported.
- `components/hal/esp32p4/include/hal/dw_gdma_ll.h` and
  `components/soc/esp32p4/register/soc/dw_gdma_reg.h`: CH0 SAR/DAR+100/+108,
  address-high+104/+10C, BLOCK_TS+110=size-1, CTL0+118=5 (both master1, width8,
  increment, one item), CTL1+11C=0, CFG0+120=0 (single block), CFG1+124=18
  (M2M, software handshake fields ignored for memory, outstanding1), LLP0/1=0.
  Channel INTSIGNAL+190 writable maskF83F7FFB clears only R/W fields, preserving
  reserved/RO bits; global INT_EN remains0, preventing IRQ propagation. Initial STATUS+188 must be0;
  only owned-channel completion/disabled flags cleared via INTCLEAR+198=4000001B.
- `components/hal/include/hal/dw_gdma_types.h`: width8/increment/contiguous enums;
  `components/hal/esp32p4/include/hal/cache_ll.h` and
  `components/soc/esp32p4/include/soc/ext_mem_defs.h`: NC alias offset40000000.
- `components/esp_rom/esp32p4/include/esp32p4/rom/cache.h`:
  `int Cache_WriteBack_Invalidate_Addr(uint32_t map,uint32_t addr,uint32_t size)`,
  success0, L1D mapbit4; `uint32_t Cache_Get_L1_DCache_Line_Size(void)`.
  `components/esp_rom/esp32p4/ld/esp32p4.rom.ld` binds these to4FC003FC/4FC003C8.
  Native diagnostic calls immutable silicon ROM entries, with no SDK library or
  header linkage. This is an explicit ROM dependency, not an OS/runtime import.

## Memory and ownership contract

Only opt-in transfer profile generates a linker script from the canonical
script, inserting `.maia_dma_probe` NOLOAD before stack. Default linker unchanged.
66,048B is reserved: two32KiB buffers with128B guards before/after each. Link/runtime
checks put it below4FF28000, outside BSS/stack/ROM reservation, aligned128B.
ROM cache line size must be16/32/64/128; range maintenance writes back/invalidates
old L1D aliases **only in this owned reservation** before initialization. It is
not assumed that ROM entry had purged former firmware's dirty lines.

Startup does not zero the reservation through its cached alias. CPU thereafter
initializes/observes it only through the NC alias; DMA addresses use the physical
4FF range. No cached access, descriptor, PSRAM or dynamic allocation. Fences
publish initialization before enable and precede post-completion observation.
Core0/IRQs-off, exclusive controller/bus/region ownership and other producers
quiescent remain caller preconditions. This fixture is not a general DMA memory
allocator or concurrent cache-coherency API. ROM cache maintenance itself has no
new software time bound; no hard-time/latency claim is made.

## Diagnostic sequence and acceptance

Preflight snapshots stable MIPI/clock state and all DMA channels disabled,
global/abort/reset/common errors0. First program/read back every CH0 configuration
word, enable global then channel, and complete32KiB M2M copy. Global disable must
complete before CPU validates all bytes/source/guards. Then reinitialize with a
different nonzero source pattern and zero destination, configure/start another
copy. CPU waits for destination byte1023 and CHEN1, then calls native stop with
no console traffic in between. Native report must itself show initialCFG1 and
requested channel mask1, completion stage3 and disabled final controller/channels.

Post-stop data must be a correct **partial** prefix (1..32767 bytes) with unchanged
zero tail, original source and all four guards. Eight full NC scans must remain
identical. Other channels and sampled MIPI/clock words compare unchanged. No
reported channel/common error; duplicate stop BUSY. This is bounded polling and
finite scans, not interrupt-driven DMA or proof of lossless peripheral delivery.
Single-block transfer bounds remain inside reserved buffers even on stop failure;
runner always resets to preserved flash. No abort/clock/power fallback.

Reproducible build/run procedure:

```sh
cmake -S . -B /tmp/maia-native-vdma-transfer-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_VDMA_STOP=ON -DMAIA_NATIVE_PROBE_VDMA_TRANSFER=ON
cmake --build /tmp/maia-native-vdma-transfer-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-vdma-transfer-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-vdma-transfer-20261112/maia_sched_probe_ram.bin --log /tmp/maia-vdma-transfer-hardware.log --vdma-transfer --capture-seconds 12
```

## Final validation and actual device findings

- Host **26/26 CTest PASS**, ASan/UBSan fixture PASS. Fixed address/value tests
  check reservation overlap/overflow, exact12 configuration writes, failures at
  each readback, source/destination pattern distinction, reserved/RO preservation.
  Capture negatives reject completed/idle/stale copies, missing records, invalid
  addresses/cache preparation, changed guards/data and reported errors. Four
  compiled mutants rejected: off-by-one size, wrong master port, clobbered signal
  bits and accepted overlap. Two invalid CMake profile combinations reject.
- Strict GCC14.2/RV32IMAC build/runtime-symbol gate PASS (project objects+libgcc).
  Explicit immutable ROM calls above are included, no SDK headers/library or
  FreeRTOS linkage. Final image9824B, entry4FF00100, entirely L2MEM_LOW. Private
  NOLOAD reservation4FF03500–4FF13700, source4FF03580, destination4FF0B680;
  stack begins4FF13700, BSS ends4FF03500, CPU alias8FF03500. No new cached buffer
  stores in startup; ROM maintenance reports success, actual L1D line64B.
  Existing linker RWX/removed empty flash-segment warnings remain. Default linker
  is unchanged; prior empty-controller target rebuild retains the exact tested
  image hash, no new device run needed for that unchanged binary.
- Actual Tab5 ESP32-P4 rev1.3/40MHz XTAL, `/dev/cu.usbmodem1301`: complete copy
  **32768/32768 bytes**. Second transfer: activity observed and native stop itself
  snapshots initialCFG1/CHEN1, requested1; stage3, **two polls**, finalCFG/CHEN/
  ABORT0, FIFO residue0. Correct partial prefix **1152 bytes**, zero remaining
  tail31616 bytes, bad0/source and guards intact; eight complete subsequent scans
  stable. All sampled MIPI/clock/other-channel words preserved, duplicate BUSY.
  Channel/common status words0: no reported errors; interrupt delivery is not
  tested, global INT_EN remains0. This is measured SRAM M2M evidence, not display/
  peripheral handshake, PSRAM, SMP, worst-case timing or lossless-stop proof.
- Initial readback refused before channel enable: INTSIGNAL read07C08004 after
  a zero write. Pinned register fields identify reserved/RO ones (including RO
  parity enable). Fixed by masked R/W update with exact full readback; all guards
  retained. That diagnostic image/log is preserved. First post-fix target run
  passed data/stop checks but had a truncated early schema banner; strict host
  capture correctly FAILed. The required schema marker now follows the completed
  first copy (early boot banner optional); final full capture PASS, same1152-byte
  partial outcome. Validator completeness was not relaxed or missing logs filled.
- Automatic reset returned to preserved flash; paced maia.py verifies shell,
  SciCalc `2+3*4=14`, module return0 and desktop active: PASS. No flash/SD writes.
  Diagnostic writes only owned SRAM/cache state and owned VDMA configuration/
  completion flags/global/channel enable; no display, MIPI clock, LDO or panel
  reset writes. On a failed stop, it does not access buffers before completion;
  no abort/clock/power fallback. Single-block destination stays inside reservation.

[Final target capture](native-vdma-transfer-hardware.txt),
[desktop recovery](native-vdma-transfer-desktop.txt),
[initial configuration refusal](native-vdma-transfer-initial-refusal.txt),
[truncated capture](native-vdma-transfer-truncated-capture.txt),
[manifest](native-vdma-transfer-manifest.json). Sources/builds/primary references,
raw logs and earlier candidates retained in
`../maia-rtos-recovery/native-vdma-transfer-20261112/`.

Changed files: isolated transfer probe/layout/fixture; CMake opt-in source/linker
selection; runner and host fixtures; audit/evidence/status. No production driver,
public DMA API, scheduler, parser or canonical linker modifications. Existing
native stop API is exercised unchanged. Complete desktop still uses FreeRTOS.

Next small task: native DSI/CSI PHY/bridge stop and failure contract, keeping bus
clocks live until VDMA completion. Peripheral destination handshakes, multiple
simultaneous channels and physical failures are separate tests. Do not treat
this SRAM result as a full MIPI shutdown or relax power/reset guards yet.
