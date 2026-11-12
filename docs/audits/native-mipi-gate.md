# Checked boot MIPI clock gating — 2026-11-12

**HARDWARE-TESTED, LIMITED: disconnected boot ISP and parked MIPI clock gating.** Follows native reset parking in the same
boot. No live ISP/DSI/CSI teardown, analog power cycle or production driver swap.

## Primary evidence and defects addressed

Previous [reset parking](native-mipi-park.md) deliberately left clocks on and
shared ISP handling open. The already verified VDMA global-disable completion
must precede reset/clock writes; gated peripheral MMIO must not be read afterward.
A shared ISP with enabled functional clock is not assumed idle from an EN bit or
old SDK object. Caller boot ownership remains explicit.

Primary [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
38.7.2 p1974: finish related VDMA channels, reset ISP, reset CSI host. This unit
uses the narrow already-shutdown boot state, additionally holding both bridges
in reset, without implementing active-stream stop. No claims about analog
settling/physical reset waveforms or internal ISP idle signals are introduced.

Pinned ESP-IDF **f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0** local primary sources:

- `components/hal/esp32p4/include/hal/isp_ll.h` `isp_ll_enable_module_clock`
  uses PERI_CLK_CTRL25.ISP_CLK_EN; reset_module_clock asserts/deasserts ISP reset.
  `components/esp_driver_isp/src/isp_core.c` claims/resets shared ISP and enables
  it separately, releases its module clock separately. That SDK object/lifecycle
  is not reused; native API inspects capture-input controls when that clock is enabled, rather
  than inferring activity/idle from the clock or global ISP_EN alone.
- `components/hal/esp32p4/include/hal/mipi_csi_ll.h` reset_host_clock/
  reset_brg_module_clock; `mipi_dsi_ll.h` reset_register, DPI/PHY reference/config
  clock helpers. `components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c` delete disables
  PHY reference/config before bus access. Native sequence holds resets rather
  than releasing a pulse; future native bring-up must own their release.
- `components/soc/esp32p4/register/soc/hp_sys_clkrst_reg.h`: base500E6000,
  PERI_CLK_CTRL25+A8.ISP_CLK_EN bit25; HP_RST_EN0+C0 bits26/27/28/29 DSI bridge/
  CSI host/CSI bridge/ISP (mask3C000000); HP_FORCE_NORST0+CC corresponding fields
  bits11..14 (mask7800), required0 and never written. SYS_CLK_CTRL1+18 MIPI bus
  mask1C00; PHY/DPI PERI_CLK_CTRL03+3C mask93, preserving divider/source/other bits.
  The SDK labels several generated RCC descriptions Reserved; use is cross-
  checked with its actual LL accessors. No unknown reset bit is guessed/cleared.
- `isp_reg.h` at ISP500A1000 pins CNTL+08: MIPI_DATA_EN bit0 and IN_SRC bits28:27
  (CSI0/CAM1/DMA2). CAM_CNTL+114 bits0/1/2 are enable/update/reset. TRM34.7.5–7
  pp1574–1575 separately enables input data/camera/DMA after selecting source.
  With clock on/reset released, require CSI source0 with MIPI_DATA_EN0 and DVP
  enable/update0/reset1. ISP_EN bit1 is not by itself a capture-input predicate;
  no write to ISP control words is performed. DMA-source trigger is WT and is not
  read as an idle signal; selecting DMA/CAM sources rejects.
- Previous pinned VDMA CFG0 global EN contract at TRM p349, CHEN/ABORT/RESET/FIFO
  guards, and MIPI host/shutdown/bridge/DPI controls remain unchanged. VDMA clocks
  CPU13/SYS5 stay on, allowing safe completion checks after MIPI clocks go off.

Source URL base (exact file hashes/HEAD equality in manifest):
https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/ .
Original native MIT implementation from register facts/policy. No vendor headers,
function bodies, locks, SDK handle, runtime or ROM call imported.

## Native API and failure contract

`maia_esp32p4_mipi_gate_clocks(report)` requires the internal successful park
claim in this boot; caller data cannot fabricate it. Same chip1.3 exclusive
pre-scheduler/core0/IRQs-off ownership of MIPI/ISP/VDMA/RCC, all other cores/LP/
external writers/producers quiescent. No runtime ISR/task/SMP or live client use.

Fresh RCC snapshot requires all MIPI and VDMA buses, both PHY config clocks,
reset override fields0. A clocked, reset-released ISP is read only at CNTL/CAM_CNTL
and accepted only in the disconnected CSI/DVP-reset state above. Clock-off or
already-reset ISP is not accessed; report marks those controls unmeasured. Fresh parked controls and DMA
completion/FIFO0 must still hold. Any enabled capture input/other input mode returns BUSY before writes. Input
words are rechecked stable immediately before reset; no presumed runtime idle.
It never enables an ISP clock or reads ISP through disabled clock/held reset.
Residual internal pixels may be discarded by reset; not a lossless-stop proof.

After stable guards, hold RESET mask3C000000 asserted preserving all other bits.
No release/pulse or presumed settling interval. Then clear PHY/config/reference/
DPI mask93 with full readback; then clear three MIPI bus clocks mask1C00. Between
steps recheck RCC words, unchanged ISP clock/overrides, and inactive still-clocked
VDMA. **After reset assertion, no MIPI internal register reads/writes**; only RCC
and VDMA. No voltage, LDO, pin reset, DMA command, frame-buffer or IRQ flag write.
Other clock/reset bits, CPU clock, DMA clocks and ISP clock configuration remain.

Stages: guarded1, reset2, PHY3, bus4, verified5. attempted_mask bits0/1/2 track
conservative reset/PHY/SYS write attempts, skipping no-ops. No retries/rollback.
Any failure after guards is sticky FAILED until hardware reset; success makes
later gate/park BUSY without MMIO. Failed operation may have partially applied.
Required report tracks before/observed words and last successful phase. Fixed
finite work with responding-bus precondition; no hard-real-time bound. Reset may clear peripheral status; no explicit IRQ acknowledgement. Native
IRQ owner handles routing/pending sources before enabling interrupts.

OK proves only this boot clock/reset state, not a live-stream shutdown, physical
power-cycle permission, electrical discharge/settling or generic ISP sharing.
Re-enable/release is a later native driver contract. Preserved flash remains the
complete FreeRTOS-based desktop; no default path changed.

## Acceptance and exact pending procedure

Host fixture checks native claim, wrong context/missing gates, ISP active/reset
overrides/DMA/live controls/FIFOs, exact reset->PHY->SYS order and masks,
unrelated bits/no-op reset, dropped writes at each stage, late ISP/DMA/override
changes, sticky failures and duplicate calls. Its MMIO whitelist asserts no MIPI
access after reset; raw ISP addresses are never allowed. Park regressions remain.

Opt-in `MAIA_NATIVE_PROBE_MIPI_GATE` follows the prior host-ready/park diagnostic,
reads RCC-only ISP snapshot, then calls gate. Post-gate validation reads only RCC
and VDMA configuration, preserving still-clocked DMA/CPU/other fields. Capture
validator requires park PASS, validated ISP source/input state or unreadable gated/reset state, override0, exact reset and clock masks,
complete final report and duplicate BUSY. Live active-stream behavior remains
NOT RUN; the diagnostic starts with shutdown hosts and inactive VDMA.

```sh
cmake -S . -B /tmp/maia-native-mipi-gate-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON
cmake --build /tmp/maia-native-mipi-gate-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-mipi-gate-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-mipi-gate-20261112/maia_sched_probe_ram.bin --log /tmp/maia-mipi-gate-hardware.log --mipi-gate --capture-seconds 8
```

## Actual findings and final validation

The first conservative policy required ISP functional clock off and refused
before any **gate** write. Target RCC word02E82000 has clock bit25 on. Additional
read-only inspection while its clock is on found CNTL40002442 and CAM_CNTL4:
MIPI input disabled, CSI source selected, DVP disabled/held reset. Global ISP_EN
is1, but the manual separates it from capture input enable. The final policy
checks these exact source/input predicates and stability, not a blanket waiver
of ISP ownership/active input. Both refusal/inspection logs and candidates remain
preserved. Native park/setup digital writes preceded that gate refusal.

- **28/28 CTest PASS**, ASan/UBSan fixture and C++98 public-header PASS. Includes
  clocked disconnected ISP acceptance, MIPI/CAM/DMA-source refusals, DVP/reset/
  update predicates, no raw ISP reads when gated/held-reset, all prior park
  regressions. Four compiled mutants rejected: accepting enabled input, missing
  ISP reset, peripheral reads after reset and lost sticky gate failure. Invalid
  gate-without-park CMake profile rejects. No fake target fault/timeout claim.
- Strict GCC14.2/RV32IMAC build/runtime gate PASS: project objects+libgcc only,
  no foreign runtime/header/ROM call or host hook. Image10416B in L2MEM_LOW,
  entry4FF00100. Final comment-only header change gives byte-identical tested
  image. Existing RWX/removed-empty-flash-segment warnings remain.
- Actual Tab5 ESP32-P4 rev1.3/40MHz XTAL, `/dev/cu.usbmodem1301`, RAM-only final
  PASS. Native park prerequisite PASS; ISP clock02E82000, control40002442,
  camera4, reset override0. Held reset word40 ->3C000040; PHY clock word2B3 ->220;
  MIPI SYS clock0C0D5E78 ->0C0D4278. Stage5/attempt7/preserved1/duplicate_busy1.
  Every guarded MIPI clock bit is0, all four shared reset requests1. CPU/VDMA and
  unrelated clock/reset fields compare preserved. ISP function clock stays on
  and its module stays held reset; it is **not** silently turned off. Stored ISP
  observations printed after gate are pre-reset snapshots, not post-reset reads.
- After asserting resets the implementation/diagnostic accesses only RCC and
  still-clocked VDMA, never gated MIPI/ISP registers. Hardware request/readback
  is tested, not electrical reset propagation, analog settling or live-link stop.
- Automatic reset returns to preserved flash; paced maia.py shell echo, SciCalc
  `2+3*4=14`, module return0 and desktop active PASS. No flash/SD, regulator/panel
  pin reset or analog power write. Resets can discard inactive internal state;
  no active camera/video/VDMA-peripheral-transfer shutdown was tested.

[Final target](native-mipi-gate-hardware.txt), [desktop](native-mipi-gate-desktop.txt),
[initial refusal](native-mipi-gate-initial-refusal.txt),
[ISP inspection](native-mipi-gate-isp-inspection.txt),
[manifest](native-mipi-gate-manifest.json). Full source/build/reference PDF/SDK,
raw logs, prior candidates, sanitizer and negative evidence:
`../maia-rtos-recovery/native-mipi-gate-20261112/`.

Changed files: existing native MIPI contract/seam/implementation, optional probe/
CMake/runner, new gate host fixture and evidence/status docs. No production/default
firmware or canonical linker change. Native clock gating implemented for this
boot state; live shared-ISP/stream teardown and native display remain incomplete.

Next: compose verified native park/gate with the guarded VO3 disable/acquire
probe, re-audit analog/discharge/stabilization assumptions and validate only its
actual scope. Existing voltage/reset guards remain in place. Complete desktop
still uses FreeRTOS; native cold display/PSRAM and optional POSIX remain pending.
