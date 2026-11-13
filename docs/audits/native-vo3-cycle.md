# Guarded native VO3 disable/acquire diagnostic — 2026-11-12

**HARDWARE-TESTED, LIMITED.** Composes verified native MIPI park/gate with
existing VO3 calibration/acquire. No production/default firmware or public API
change, no FreeRTOS import, parser edit or native display completion claim.

## Prerequisites and primary references

The earlier [VO3 acquire](native-ldo3-acquire.md) could only adopt an already-on
rail on target because MIPI clocks remained enabled. Subsequent [native clock
gating](native-mipi-gate.md) verifies boot ISP input disconnection, inactive VDMA,
held shared resets and all guarded clocks0. This diagnostic consumes that actual
success before requesting VO3 off; no guard is bypassed or stale SDK handle used.

Pinned ESP-IDF **f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0**:
[esp_ldo_regulator.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/ldo/esp_ldo_regulator.c)
acquire/release configure calibrated codes/SW owner/ripple suppression then enable,
and release clears enable on last user. Neither sequence performs an explicit
voltage-ready/settling/discharge wait in this revision. Reference counts/locks are
not imported. Existing native acquire preserves Espressif Apache-2.0 attribution;
this unit leaves it unchanged and invokes its checked inactive-rail branch.

[ldo_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h)
unit mapping{0,3,1,4} maps logical unit2/VO3 to PMU slot1. Enable writes XPD;
power-delay and current-limit controls are separate, retained unchanged here.
Pinned pmu_reg.h VO3 CTRL501151C0 XPD bit8, source/owner/bypass mask4F80 and ANA
501151C4 dref/mul/VDET maskF7800000 remain from the preceding calibration audit.
Primary LCD MIPI example identifies channel3/2500mV for VDD_MIPI_DPHY; other
regulator slots are not modified. No undocumented ready flag/timing is invented.

Primary [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
38.7.2 p1974,34.7.5–7 pp1574–1575 and VDMA CFG0 p349 underlie the unchanged MIPI/
ISP/VDMA prerequisites. Exact register/source hashes are retained in manifest.

## Narrow diagnostic contract

New `MAIA_NATIVE_PROBE_VO3_CYCLE` opt-in extends only the MIPI gate RAM probe. It
requires the full native MIPI gate profile; old LDO standalone profiles remain
separate. Existing native APIs/public headers/production drivers unchanged.

After native gate success, read eFuse/PMU snapshot and calibrated2500mV plan.
Require supported chip/schema/no uncorrectable eFuse error, stable16 active
snapshots, SW-owned/source0/bypass0/enabled state and exact matching calibrated
codes/VDET. Snapshot ten other LDO control/analog words. Private helper rechecks
full CPU/RCC/ISP/reset/override words, zero MIPI clock masks, held resets, VDMA
completion/abort/reset/FIFO0, and exact fresh VO3 control/analog before clearing
**only XPD**. Read back complete off word. No public release API or lifetime
bypass: no native acquisition occurred before the off request.

Off snapshot must match initial eFuse/analog/all other control fields, with
XPD0. Probe emits OFF_READY and waits for host `O`, then checks16 stable off
samples and guards. Existing native acquire must reach verified5/adopted0/
changed1. It programs only fields that need changing; matching analog/source
steps may be no-ops. It must restore exactly the original full VO3 words. Probe
emits ON_READY, waits for host `N`, verifies16 stable on snapshots, all ten other
LDO words and RCC/VDMA guards, then duplicate acquire BUSY.

After gate success: no peripheral reads behind gated MIPI/ISP clocks, no MIPI re-enable/reset release,
DMA start, display/framebuffer/PSRAM access, calibration burning or LDO retune.
Failure stops with no retry/compensating power writes, possibly leaving rail off
until runner reset; existing acquire sticky-failure policy remains. Source/
current-limit/delay/other analog fields and every other rail are preserved.

## Observation windows and limits

Host acknowledges each READY only after measuring **at least20ms** via monotonic
clock. The native wait uses a finite10-million-read budget; no CPU-frequency-based
delay or undocumented regulator timing. This is a host-observed quiet window,
not an on-device elapsed measurement or vendor minimum settling/discharge spec.
Exclusive serial ownership is required; ACKs are diagnostic sequencing, not a
security/ownership primitive. No power command occurs before all native guards.

XPD readback proves a control request, not measured voltage, complete discharge,
regulator ramp, electrical settling or output under load. VDET is the retained
ripple-suppression setting, not treated as voltage-ready proof. No cold-board,
live-stream or functional display/analog voltage guarantee is claimed.

## Validation procedure

Host tests cover each RCC/DMA/reset guard, mismatched plan/source/owner/bypass/
control/analog, preservation of unrelated/current-limit fields, dropped off write
and no extra write after changed control. Existing native acquire tests cover
inactive programming/failures. Capture tests require both control transitions,
non-adoption, exact restored words, stage5, observation ACKs and complete records.

```sh
cmake -S . -B /tmp/maia-native-vo3-cycle-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON
cmake --build /tmp/maia-native-vo3-cycle-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-vo3-cycle-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-vo3-cycle-20261112/maia_sched_probe_ram.bin --log /tmp/maia-vo3-cycle-hardware.log --vo3-cycle --capture-seconds 10
```

Working firmware remains preserved on flash; runner resets to it in `finally`,
then maia.py checks shell/SciCalc/desktop. No flash or SD writes in this unit.

## CSI access preparation and failed candidates

The first composite run refused `bus-clocks` before VO3 writes. Measured SYS
0C0D5278/PHY2A3 differed from previous inherited boot states: CSI host/bridge and
PHY configuration clocks were unavailable. A first preparation candidate enabled
only the host bus and faulted reading CSI RESETN5009F008 (MTVAL5009F008). Both
clock and reset prerequisites had not been established; the evidence does not
isolate one cause. Neither candidate reached the VO3 disable operation. Genuine
[refusal](native-vo3-cycle-initial-refusal.txt) and
[access fault](native-vo3-cycle-access-fault.txt) captures are preserved.

Pinned SDK
[CSI controller constructor](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_driver_cam/csi/src/esp_cam_ctlr_csi.c)
`s_csi_claim_controller` enables host access/pulses reset; constructor enables PHY
configuration clock before HAL access. The corresponding
[mipi_csi_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_csi_ll.h)
and pinned RCC/CSI register definitions establish the guarded preparation.
Private boot-exclusive helper requires DSI/DMA clocks already on, DMA idle and
ISP disconnected or held reset; holds CSI host/bridge RCC resets, enables only
CSI SYS0C00/PHY10 clocks with readbacks, releases only CSI resets, then inspects
host/PHY shutdown defaults. No CSI register reads before those prerequisites.
It preserves unrelated reset bits, including inherited ISP reset29. This is a
controlled reset of boot-owned CSI, not a live-stream shutdown implementation.

VO3 profile now installs the existing native trap handler before MMIO. The failed
candidate had inherited the ROM/SDK trap vector, whose diagnostic printed a Guru
Meditation message. Native ELF dependency checks remain clean; no SDK/FreeRTOS
handler is linked. Other diagnostic profiles are unchanged. Warm reset/RAM entry
must not be assumed to restore cold clock/reset defaults.

## Reproducible host checks

Environment: macOS, AppleClang21, Python3.14.5, native GCC14.2 RV32IMAC/ILP32.
Executed host configure/build/CTest (30/30 PASS):

```sh
cmake -S . -B /tmp/maia-vo3-cycle-host-20261112 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-vo3-cycle-host-20261112 -j4
ctest --test-dir /tmp/maia-vo3-cycle-host-20261112 --output-on-failure
clang -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude -Itests/hardware tests/host/vo3_cycle_fixture_test.c -o /tmp/maia-vo3-cycle-sanitized
/tmp/maia-vo3-cycle-sanitized
clang -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude -Itests/hardware tests/host/vo3_csi_prepare_test.c -o /tmp/maia-vo3-csi-sanitized
/tmp/maia-vo3-csi-sanitized
```

Mutation/profile script and raw results retained under recovery `scripts/` and
`negative/`; desktop check script there too. These tests deliberately exercise
power guards and forbidden MMIO order; they do not substitute for target evidence.

## Actual validation — 2026-11-12

[Complete hardware capture](native-vo3-cycle-hardware.txt) passes on Tab5 ESP32-P4
rev1.3/40MHz via ROM RAM load; final image and reference hashes in
[manifest](native-vo3-cycle-manifest.json). No flash or SD writes.

- Preparation: SYS0C0D5278 ->0C0D5E78, PHY2A3 ->2B3,
  RESET38000040 ->20000040 (retains held ISP reset).
- Native park stage4/attemptF and native gate stage5/attempt7 pass;
  RESET ->3C000040, SYS ->0C0D4278, PHY ->220. ISP registers skipped because reset
  was already held; no fabricated zero measurement of ISP input registers.
- Calibration dref13/mul3/defaults0, ANA D5800000 unchanged.
  Full CTRL40200180 ->40200080 ->40200180. Actual inactive-rail native acquisition
  stage5/adopted0/changed1, duplicate BUSY; sixteen stable samples per phase,
  ten other LDO words and RCC/VDMA guards preserved.
- Host monotonic observation windows: off20.946ms/on21.124ms. These are not
  on-device timing or analog settling measurements. Analog/source fields already
  matched, so their native programming steps were no-ops; no hardware retune claim.
- Host CTest30/30 PASS; both new fixtures pass Clang ASan/UBSan. Four compiled
  mutants (clobbered control, ignored readback, skipped DMA enable guard, CSI read
  before clocks) abort as expected. VO3-without-gate CMake profile rejects.
  Capture/ACK tests reject incomplete/out-of-order/mismatched transitions and
  short host windows. Strict native runtime gate passes with project objects and
  libgcc only; image lies entirely in reserved L2_LOW RAM window.
- [Paced desktop check](native-vo3-cycle-desktop.txt) verifies preserved installed
  application, shell, SciCalc `2+3*4=14`, module exit0 and desktop left active.
  This installed desktop still uses FreeRTOS and is a recovery baseline.

Recovery: `../maia-rtos-recovery/native-vo3-cycle-20261112/` includes final
source/build/logs and both earlier candidates. Native public acquire/park/gate
APIs and their existing license attribution are unchanged. New code is an opt-in
private diagnostic plus host tests/runner; production firmware is unchanged.

Still unimplemented/unproven: measured rail voltage/discharge/ramp/load, cold
DSI/PHY/panel bring-up, PSRAM and full native desktop, live shared-ISP ownership
and stream shutdown. Next small task: document/port checked native DSI clock and
PHY initialization prerequisites, keeping the working firmware intact; do not
infer cold-display readiness from this supply-control test.
