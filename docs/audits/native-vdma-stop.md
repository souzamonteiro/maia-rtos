# Bounded native VDMA stop — 2026-11-12

**HARDWARE-TESTED, LIMITED: empty controller. HOST-TESTED: active channels.** New boot-only primitive;
no production/default firmware change, FreeRTOS API/runtime or parser change.
MIPI/PHY shutdown, clock gating and VO3 power cycle remain separate gates.

## Defects and primary evidence before implementation

The read-only [MIPI snapshot](native-mipi-state.md) established disabled
controller/channels with active bus clocks, not a generic shutdown protocol.
Checking only channel-enable bits or FIFO0 cannot replace documented completion.
SDK deletion relies on stale handles; reference abort/reset loops are unbounded.

Primary silicon source: [ESP32-P4 Chip Revision v1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
local pinned PDF `/tmp/maia-esp32p4-rev13-trm.pdf` (hash in manifest). Section
5.5.7 pp337–338 describes non-suspended channel disable, completing initiated
source AXI transactions and handling/discarding residual partial FIFO data.
Most importantly, **register5.3 DMAC_CFG0 p349** specifies that after EN is cleared,
readback stays1 while any channel activity remains and becomes0 after activity
terminates. Thus verified global EN0, rather than a channel flag alone, is the
controller-wide completion boundary. This is not a lossless-delivery guarantee.
Section5.8 pp344–347 and register5.4 p350 pin offsets and EN write-enable strobes;
RESET0 at+58 and channel STATUS1 at+134/+234/+334/+434 complete the read set.

Cross-check revision **ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0**:
[dw_gdma_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
`dw_gdma_ll_channel_enable` uses `(0x100 << channel)` for disable;
`dw_gdma_ll_enable_controller` documents the global completion behavior;
`dw_gdma_ll_is_bus_clock_enabled` requires CPU13 and SYS5. Register layout:
[dw_gdma_reg.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h),
CFG+10/CHEN+18/ABORT+1C/RESET+58, channel FIFO bits14:0;
[peripheral symbols](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/ld/esp32p4.peripherals.ld)
DW_GDMA50081000; pinned hp_sys_clkrst_reg.h CPU+14/SYS+18. These are the same
bus gates verified in the preceding audit. Implementation is original MIT from
these facts, no foreign function bodies, headers, object handles or runtime.

## Native contract and scope

`maia_esp32p4_vdma_stop_all(limit,report)` reserves ALL four VDMA channels until
reset. Caller exclusively owns the entire controller, before scheduler startup,
core0/IRQs-off, all other cores/LP/external producers stopped. Source/destination
clocks, buses, supplies and handshakes must remain available until completion.
No runtime mutex/SMP/ISR/task use. Hart/MIE are checked; exclusive ownership is
still a caller obligation. This is not the other GDMA-AHB/AXI or 2D-DMA engines.

Preflight checks both existing clocks before peripheral reads, initial config/
channels/abort/reset and a stable second view. Aborting/resetting or global-off/
channel-on states reject before writes. A stable controller already off is
adopted read-only; no claim of stopping a previously active transaction.

Stop phase sends EN_WE strobes only to initially enabled channels, then clears
global EN with interrupt/unrelated bits preserved. It polls completion with
full clock-word preservation, global config, channel/abort/reset checks. Global
EN0 AND no enabled channel is required. Final FIFO residue is reported as a
bitmap, with final clock/config/channel/abort recheck. It never follows pointers
or writes FIFO/memory/descriptor/channel-configuration/IRQ-clear/abort/reset/clock/power.
Partial transfer data may be lost; FIFO residue is not claimed delivered/empty.

One shared limit1..4096 bounds poll iterations, not microseconds or a hard
worst-case bound. MMIO requires a responding bus; this cannot recover a stalled
bus access. Report tracks actual polls, requested channel mask, initial/final
words, last completed phase and conservative may_have_changed. Once stop starts,
a failure is sticky until reset; later calls FAILED without MMIO. No rollback/
retry/restart. Success makes later calls BUSY without MMIO. Preflight failures
leave reservation unclaimed. OK verifies controller termination only, not all
SoC AXI traffic or DSI/CSI PHY shutdown, display readiness or cold initialization.

Changed files: public SoC header; private MMIO/context seam; native stop source;
opt-in CMake/probe/runner; host fixtures; this audit/evidence/status documents.
The complete working desktop remains the preserved FreeRTOS-based firmware.

## Test scope and target procedure

Host fixture compiles actual native source with modeled MMIO. It checks all16
channel masks, exact disable strobes/order, preserved config, global handshake
remaining busy after channels go off, poll budget, no-op adoption, FIFO residue,
invalid context/clocks/state, dropped writes, changing clocks/config/reset and
sticky failure/no further I/O. Capture validator distinguishes read-only/adopt/
empty-arm modes and rejects incomplete or inconsistent results.

`MAIA_NATIVE_PROBE_VDMA_STOP` extends the prior isolated MIPI diagnostic. Before
stop it requires stable accessible controller CFG/CHEN/ABORT0, no reset, all four
FIFO/status/address/LLP0. Optional `MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY` writes CFG0=1
and verifies it, **keeping every channel disabled**. No DMA transfer is started.
Then native stop must restore CFG0, complete in one poll, preserve every sampled
MIPI/clock/channel word and reject duplicate acquisition. Even a target PASS
only tests empty-controller global enable/disable; active-transfer stopping is
HOST-TESTED until a separate DMA-visible SRAM fixture and target test exist.

```sh
cmake -S . -B /tmp/maia-native-vdma-stop-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_VDMA_STOP=ON -DMAIA_NATIVE_PROBE_VDMA_ARM_EMPTY=ON
cmake --build /tmp/maia-native-vdma-stop-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-vdma-stop-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-vdma-stop-20261112/maia_sched_probe_ram.bin --log /tmp/maia-vdma-hardware.log --vdma-arm-empty --capture-seconds 12
```

## Next small task

Verify active native DMA stop with explicitly reserved DMA-visible buffers/cache
ownership before claiming active-stream shutdown. Separately inspect/reset/stop
DSI/CSI PHY and bridge under a documented contract, preserving bus clocks until
VDMA completion. No clock gating or power/reset guard relaxation in this unit.

## Final validation and hardware evidence

- Host **25/25 CTest PASS**, ASan/UBSan fixture PASS, public header C++98 PASS.
  Four compiled negative mutants are rejected: ignoring global completion,
  wrong enable strobes, clobbering config and lost sticky-failure state. Two
  invalid CMake option combinations reject. Host fixture covers active masks/
  failures; these do not substitute for an active-transfer device test.
- Strict GCC14.2/RV32IMAC build/runtime gate PASS, project objects+libgcc only,
  no host test hook in target ELF. Armed-empty image8528B in L2MEM_LOW, entry
  4FF00100. Final source comments produce a byte-identical hardware-tested image.
  Adoption-only candidate builds/gates; its separate hardware run NOT RUN.
  Existing linker RWX/empty removed flash-segment warnings remain unchanged.
- Actual Tab5 ESP32-P4 rev1.3/40MHz XTAL on `/dev/cu.usbmodem1301`: eight baseline
  MIPI observations pass, then global CFG written0->1 with channels disabled and
  read back1; native stop restores0 in one poll. Report: stage3,armed1,adopted0,
  changed1,requested0,channels0,abort0,fifo0,preserved1,duplicate_busy1. All sampled
  clock/MIPI/channel words compare unchanged before/after. This validates empty
  global enable/disable and lifetime behavior, **not active DMA/AXI draining**.
- Runner reset returns to preserved flash. Paced maia.py check passes shell,
  SciCalc `2+3*4=14`, module exit0 and desktop active. No flash/SD, MIPI/clock,
  regulator or panel reset writes. The only peripheral control writes added by
  this actual diagnostic were VDMA CFG0=1 then CFG0=0; no DMA transfer initiated.
  No physical voltage/waveform/pixel/real-time/SMP/native-desktop claim.

The native API retains pending interrupt flags. Future boot integration must
handle/mask those via the native interrupt owner before enabling IRQs; it must
not infer data delivery from controller quiescence. Full desktop remains
FreeRTOS-dependent; this API does not change its default driver path.

[Actual target capture](native-vdma-stop-hardware.txt),
[desktop recovery check](native-vdma-stop-desktop.txt),
[reference/artifact manifest](native-vdma-stop-manifest.json). Full source,
builds, reference PDF/SDK snapshots, raw logs, sanitizers and mutation evidence:
`../maia-rtos-recovery/native-vdma-stop-20261112/`.

Host validation:

```sh
cmake -S . -B /tmp/maia-vdma-host-20261112
cmake --build /tmp/maia-vdma-host-20261112 -j4
ctest --test-dir /tmp/maia-vdma-host-20261112 --output-on-failure
```
