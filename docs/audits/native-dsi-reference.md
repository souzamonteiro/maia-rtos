# Native DSI nominal20MHz reference — 2026-11-13

**HARDWARE-TESTED, LIMITED.** Extend the boot-only DSI configuration-access
module with reference routing. Native PLL/analog/lane/video bring-up remains
separate. No production/default firmware or parser changes.

## Primary sources and boundaries

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[clk_tree_defs.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/clk_tree_defs.h)
identifies fixed SPLL480MHz and default MIPI PHY source PLL_F20M.
[clk_tree_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_tree_ll.h)
`clk_ll_pll_f20m_get_divider` decodes REF_CLK_CTRL1[23:16]+1;
[esp_clk_tree.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/port/esp32p4/esp_clk_tree.c)
derives nominal frequency480MHz/divider, and enables provider through
[clk_gate_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_gate_ll.h)
REF_CLK_CTRL2 bit8. Native policy requires divider24 and never changes it.

[Chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
10.2.2 p616 clock architecture; register10.11 p651 REF_CLK_CTRL1(+28),
10.12 p652 REF_CLK_CTRL2(+2C), 10.15 pp657–658 PERI_CLK_CTRL02(+38) and
10.16 p659 PERI_CLK_CTRL03(+3C). Source bits31:30 select0=PLL_F20M,
1=RC_FAST,2=PLL_F25M,3=invalid. **Mux supplies BOTH DSI configuration and PLL
reference clocks.** Gate config during a mux change, restore before DSI reads;
reference bit1 remains off until the final step. No MMIO behind disabled access.
The SDK DSI constructor sets mux before initially enabling config/ref clocks;
this native extension begins after configuration access has already been opened.

Pinned lp_clkrst_reg.h HP_CLK_CTRL(+40) at50111040 XTAL40 gate25/SPLL480 gate27
must already be on. Only their full read-only word is sampled. This is not cold
SPLL startup, lock verification or analog frequency measurement. Caller owns the
clock domain/consumers and guarantees stable inherited SPLL/40MHz XTAL. A gate
bit alone does not prove a PLL is stable. Model20MHz is published only on successful
routing with divisor24. Do not advertise a measured clock or initialized DSI PHY.

## Native API and failure contract

`maia_esp32p4_dsi_reference20` in existing `esp32p4_dsi_access.h`/`soc_dsi_access.c`
requires internal successful native `open_config` claim in this boot; a caller-
fabricated report cannot authorize it. Same core0/IRQs-off/exclusive boot contract,
active calibrated VO3, inactive clocked VDMA, DSI access pattern, CSI/ISP resets
held, no reset overrides, DSI analog/digital/host/bridge/DPI off/FIFO0. Reject
already-enabled reference or CSI/DPI clocks. No reference-counted runtime manager.

Fresh complete RCC/provider/divider/LP/eFuse/PMU/VDMA and readable DSI shutdown
guards precede writes. Six reported phases after initial guard: provider enable,
config-off, source select, config restore, reference enable, final verification.
Provider/mux/config steps skip writes when already suitable. Actual possible
writes max5: shared provider|100, PHY&~1, source&~C0000000, PHY|1, PHY|2.
Only the mux-change path gates config; DSI reads are skipped during that interval
and shutdown is verified again after restore. Caller prevents external changes
while registers cannot be inspected. Full readbacks preserve all unrelated bits.
Never change the shared divider, SPLL/power/LP controls, reset, CSI/ISP, DMA,
PHY test interface/PLL/analog/lane/video/panel or memory settings.

Before writes: null INVALID/context CONTEXT; missing claim/incompatible state
BUSY; unstable fresh state VERIFY/write-free/retryable. Once guards passed,
failure sticky FAILED until reset even after a skipped step; no retry/rollback.
Shared provider remains enabled; no runtime release/refcount. Duplicate success
BUSY without I/O. Last completed phase/conservative attempted mask0..4 returned.
Native open_config state and other module APIs unchanged.

## Verification

Host macOS AppleClang21/Python3.14.5; native GCC14.2 RV32IMAC/ILP32. Fixture
executes real native calibration/snapshot/access before reference; checks all five
writes, no gated DSI reads, no-op provider/mux branch, incompatible divider/LP/
RCC/supply/DMA, fresh changes, failures at every write and sticky duplicate.
DSI shutdown changes cannot be sampled while config clock off: tests account for
that window and final readable guard. Negative capture tests validate exact
nominal-frequency/divider/provider/mux/PHY transitions and mode isolation.

```sh
cmake -S . -B /tmp/maia-dsi-reference-host-20261113 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-reference-host-20261113 -j4
ctest --test-dir /tmp/maia-dsi-reference-host-20261113 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-reference-20261113 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON -DMAIA_NATIVE_PROBE_DSI_REFERENCE=ON
cmake --build /tmp/maia-native-dsi-reference-20261113 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-reference-20261113/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-reference-20261113/maia_sched_probe_ram.bin --log /tmp/maia-dsi-reference-hardware.log --dsi-reference --capture-seconds 10
```

Opt-in diagnostic extends previous native park/gate/VO3/access probe, samples16
stable root/provider/divider/mux/PHY/supply words and observes shutdown controls,
then resets to preserved flash in runner finally. On reference failure no DSI
register reads are attempted by diagnostics (config may be off).

## Actual validation — 2026-11-13

[Hardware capture](native-dsi-reference-hardware.txt),
[desktop recovery](native-dsi-reference-desktop.txt),
[manifest](native-dsi-reference-manifest.json). Tracked captures normalize line
endings/trailing whitespace; raw bytes remain in recovery. Tab5 ESP32-P4 rev1.3,
40MHz XTAL; RAM-only diagnostic followed by reset to preserved flash. No flash/SD
writes; no parser change or foreign runtime dependency introduced.

Previous native park/gate/VO3 cycle/access all pass. Native reference stage7,
mask10, preserved1/duplicateBUSY1/samples16:

- REF divider word58170503 -> unchanged: encoded23/divisor24.
- LP gate1FFCFFFD -> unchanged: XTAL40/SPLL480 gates set (not PLL-lock proof).
- Shared provider00000115 -> unchanged: provider bit8 already set.
- Source38862600 -> unchanged: selector0 already chosen, unmasked fields preserved.
- PHY configuration/reference word00000221 ->00000223: **one actual write**,
  enables only DSI reference gate1. No synthetic shared-provider disable/retune.
- Nominal frequency01312D00 =20000000Hz is the inherited SPLL software model,
  not measured output. Root divider, LP power/gates, root PLL configuration,
  SYS/reset and full VO3/eFuse words remain stable across16 observations.
- Host/PHY reset/bridge/FIFO0; DPI19D0(enable0), PHY_STATUS1528 observed only.
  PHY is still in shutdown/reset, not initialized/locked/ready for transmission.
  Prior VO3 off/on host windows30.017/26.329ms retain earlier observation-only limits.

Provider-enable and nonzero-mux gating/program/restore branches are **HOST-TESTED
only**, not exercised by this real default-state run. They are not claimed as
hardware-tested source switching. Unrelated register preservation/actual final
reference enable are hardware-tested. No failed hardware candidate in this unit.

Host CTest32/32 PASS; reference fixture ASan/UBSan PASS. Four compiled negative
mutants (skip shared-divider check, switch mux without config gating, enable DPI
clock, retry failed claim) abort as expected. Invalid reference-without-access
CMake profile rejects; public header strict C++98 PASS. Native ELF runtime gate
PASS (project objects+libgcc only; no unresolved/foreign runtime); RAM bounds and
entry validated before serial access. Known linker warnings: RWX LOAD segment and
removed empty flash segments, unchanged from previous diagnostic builds.

Scripts/logs/references/builds preserved under
`../maia-rtos-recovery/native-dsi-reference-20261113/`; sanitizer/mutation and strict
header commands are reproducible in `scripts/maia-dsi-reference-negatives.py`.
Pinned SDK reference files compared byte-for-byte with git revision before hashing.
TRM extraction used local PDFKit with module cache under/tmp (initial cache path
outside writable roots failed; retry succeeded, no permissions bypass).

Paced maia.py check passes shell echo, SciCalc `2+3*4=14`, module exit0 and desktop
left active. Installed functional desktop remains the preserved FreeRTOS baseline;
the native diagnostic is not full desktop replacement.

Implemented: bounded checked native nominal20MHz reference routing/reservation,
private test seam, host/device diagnostic and capture validation. Not implemented:
cold SPLL startup/calibration, physical frequency/rail measurement, PHY test-port/
PLL programming/analog and lane activation/lock/stop-state waits, panel commands,
video/PSRAM/framebuffer/full native desktop. Next small task: port the PHY PLL
configuration/test-port sequence with bounded failure policy, keeping transmission
and panel/video/DMA initialization separate.
