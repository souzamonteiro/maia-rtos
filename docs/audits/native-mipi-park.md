# Native boot MIPI reset parking — 2026-11-12

**HARDWARE-TESTED, LIMITED: boot/reset-state digital parking.** Reset-state normalization only. Complete
firmware still uses FreeRTOS. No parser, default firmware or native display swap.

## Defects, primary sources and deliberate boot scope

The preceding active-SRAM VDMA test establishes one native controller-stop case,
not a live DSI/CSI teardown. ROM entry leaves MIPI bus clocks enabled; CSI host
was previously unmeasured because its bus clock was off. Clock bits alone cannot
prove shutdown. SDK object handles from the old image cannot be reused.

Primary silicon source: [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
38.7.2 p1974: CSI stop requires DMA completion, ISP reset and CSI host reset.
CSI bridge can be shared with ISP; no generic live-CSI shutdown is claimed here.
DSI chapter is still listed incomplete in this revision. Active video/command/
receive shutdown, physical lane-stop/settling and shared ISP ownership need a
separate verified contract. This unit addresses only boot/reset states already
holding hosts/analog PHYs down and DPI output disabled; live states reject.

Pinned ESP-IDF **f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0** local sources:

- `components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h` bridge `enable`;
  `mipi_csi_brg_ll.h` bridge `enable`: bit0 at DSI500A0804/CSI5009F804.
- `mipi_dsi_host_ll.h` power_on_off and `mipi_dsi_phy_ll.h` power_on_off/reset/
  enable_clock_lane/force_pll: host PWR500A0004 bit0; PHY500A00A0 bit0 active-high
  release of shutdown, bit1 digital reset release, bit2 clock lane, bit3 forcePLL.
- `mipi_csi_host_ll.h` enable_reset_output and `mipi_csi_phy_ll.h`
  enable_shutdown_input/enable_reset_output: CSI HOST_RESETN5009F008,
  PHY_SHUTDOWNZ5009F040, DPHY_RSTZ5009F044, each bit0 active-low reset/shutdown.
  `components/hal/mipi_csi_hal.c` init explicitly asserts all three low first.
- `components/esp_driver_cam/csi/src/esp_cam_ctlr_csi.c` s_ctlr_csi_stop disables
  bridge then channel, while delete only disables PHY config clock/free resources.
  `components/esp_lcd/dsi/esp_lcd_panel_dpi.c` delete and DSI bus delete disable
  different clocks; neither is a raw inherited-state shutdown proof.
- `mipi_csi_ll.h` host bus clock helper and `hp_sys_clkrst_reg.h` SYS+18 bit11;
  DSI/CSI bridge/host bits12/10/11, DMA SYS5/CPU13, PHY config bits0/4. Native
  register headers confirm all offsets/masks, DSI DPI+40 bit0, FIFO+14 bits13:0.
  VDMA CFG+10/CHEN+18/ABORT+1C/RESET+58 and four FIFO words are the previous
  pinned stop contract; global EN read0 means terminated activity per TRM p349.

Full source URLs/hashes are in the manifest under immutable revision above;
base https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/ .
Implementation is original MIT policy from register facts, with no vendor code
bodies/headers/runtime, locks, delay loops or stale SDK objects imported.

## API and failure/ownership contract

`maia_esp32p4_mipi_park_reset(report)` is exclusive pre-scheduler/core0/IRQs-off,
chip1.3, reservation until reset. Caller owns all MIPI/VDMA/clock/shared-bridge
access; all other cores/LP/external sources/producers quiescent. CSR checks cannot
prove that obligation. Native runtime/ISR/SMP use is not implemented.

Both VDMA bus clocks, three MIPI bus clocks and two PHY config clocks must
already be on before peripheral reads. Missing clocks reject with no peripheral
access. DMA must already be globally inactive, no enabled/aborting/resetting
channel and no FIFO residue. No implicit DMA stop or DMA writes occur. DSI host
and PHY shutdown controls, CSI host reset and PHY shutdown, DPI enable and DSI
FIFO occupancy must all indicate the defined inactive boot state.

Then stable snapshots and fresh preconditions precede every masked step:
DSI bridge disable, CSI bridge disable, clear DSI PHY digital reset release/
clock-lane/forcePLL bits1..3 (bit0 remains0), clear CSI PHY digital reset release.
No-op writes skipped; unrelated bits preserved; full readbacks and final state
rechecked. No clocks, analog shutdown/host control, power, DMA, test port, frame
buffer or IRQ-status register writes/reads (except documented readonly controls).
Four finite steps with responding-bus requirement; no microsecond/real-time claim.
Report phases: inspected1, bridges2, PHYs3, verified4; attempted_mask is conservative
for actual write attempts. Once inspected, errors leave sticky FAILED until reset;
no retries/rollback. Successful duplicate BUSY, both without further MMIO.
This proves parked control registers, not analog settling, live-stream shutdown,
ISP idle or unconditional clock/power gating permission.

## Opt-in device diagnostic and acceptance

Profile requires MIPI and excludes VDMA workloads. Runner sends `P` after RAM
load; probe has a finite host-ready wait before any MIPI preparation. This avoids
relying on the sometimes-truncated early boot banner. It enables **only** the
missing CSI host bus clock SYS11 for inspection, with full word verification;
source/PHY/DPI clocks/power controls remain unchanged. This deliberate bus-enable
write is outside the parking API; its resulting clock word is the preservation
baseline and remains enabled through reset. No blind clock gating is performed.

Probe verifies reset/shutdown state, DMA/FIFOs0, then arms both bridge EN bits,
DSI PHY digital RSTZ bit1 and CSI PHY digital RSTZ bit0. Analog shutdown/hosts/DPI
stay asserted/off; no transfer starts or live lane initialization. Native API must
make four actual masked writes and return all sampled stable control words to
baseline. Clocks, DMA/global/channel state and other bits compare unchanged;
PHY lane-status bits under reset are not a valid live stop handshake. Duplicate
BUSY and strict capture validation required. This tests digital reset parking,
not an active bridge/frame/analog PHY stop. Live/partial/failure paths are host
fixtures only until separately exercised on target.

```sh
cmake -S . -B /tmp/maia-native-mipi-park-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON
cmake --build /tmp/maia-native-mipi-park-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-mipi-park-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-mipi-park-20261112/maia_sched_probe_ram.bin --log /tmp/maia-mipi-park-hardware.log --mipi-park --capture-seconds 8
```

## Final verification and exact hardware scope

- **27/27 CTest PASS**, ASan/UBSan fixture PASS and public header C++98 PASS.
  Fixture checks every16 dirty/no-op combination, exact four-address write order,
  unrelated-field preservation, missing clock gates before any peripheral read,
  live host/PHY/DPI/VDMA/FIFO refusals, dropped writes at each step, DMA/clock
  change during programming and sticky failure/no later MMIO. Four compiled
  mutants reject: accepting live host, clobbering unrelated bits, ignoring
  immediate readback and losing sticky failure. Two invalid profiles reject.
- Standalone strict GCC14.2/RV32IMAC build/runtime gate PASS, project objects +
  libgcc only. No foreign headers/runtime, heap, ROM call or test seam on target.
  Image8208B confined to L2MEM_LOW, entry4FF00100. Existing linker RWX/removed
  empty flash-segment warnings remain unchanged.
- Actual Tab5 ESP32-P4 rev1.3/40MHz XTAL, `/dev/cu.usbmodem1301`, RAM-only PASS.
  SYS word0C0D5678 ->0C0D5E78 prepares only CSI host bus access (bit11). Thus CSI
  host/PHY shutdown bits are now inspected, not inferred from a skipped block.
  Arming produces DSI digital RSTZ2 and CSI digital RSTZ1 plus both bridge EN1;
  host/analog shutdown/DPI controls remain low and VDMA remains inactive. Native
  four attempted writes return all four reported control words0, stage4,
  preserved1,duplicate_busy1. Clocks compare to the prepared baseline (CSI host
  bus stays on); DMA/channel/config stable fields compare unchanged.
  DSI PHY bits2/3 clearing are host-tested, not physically armed here. No traffic
  or analog startup/shutdown cycle, stop-state handshake or waveform measurement.
- Automatic reset returns to preserved flash. Paced maia.py confirms shell,
  SciCalc `2+3*4=14`, module return0 and desktop active: PASS. No flash/SD or LDO/
  analog-power/control writes. Added target writes are the one CSI bus-enable,
  four digital arm writes, and four native park writes. Pending interrupts are
  neither acknowledged nor used. No active-stream/ISP/physical power-cycle claim.

[Actual target capture](native-mipi-park-hardware.txt),
[desktop recovery check](native-mipi-park-desktop.txt),
[manifest](native-mipi-park-manifest.json). Full source/builds, pinned SDK/TRM
snapshots, raw logs, sanitizer and mutant evidence are retained at
`../maia-rtos-recovery/native-mipi-park-20261112/`.

Changed files: public SoC contract, private native MMIO/context seam, parking
implementation, opt-in probe/CMake/capture checks, host fixtures and evidence/
status documentation. Default full firmware and canonical linker are untouched.
Native reset parking is implemented; generic live shutdown remains unimplemented.

Next: verify shared ISP/reset and boot-consumer ownership prerequisites, then
add checked clock gating for this proven reset state, with preserved unrelated
clock bits and failure reports. Live video/CSI stop requires its own ownership/
completion tests. VO3/panel-reset guards remain unchanged; cold display/PSRAM and
complete native desktop pending, POSIX DESIGN.
