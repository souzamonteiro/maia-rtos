# Native VO3 acquisition — 2026-11-12

**HARDWARE-TESTED, LIMITED: write-free adoption. HOST-TESTED: off-rail programming.**
Checked boot-only fixed2500mV reservation following the verified calibration
planner. No DSI initialization; final validation/status and limits are below.

## Primary sources and scope

ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0,
[esp_ldo_regulator.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/ldo/esp_ldo_regulator.c)
acquire/release and [ldo_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h)
VO3(unit2 ->PMU slot1), adjust/source selection/VDET/enable are port references.
Native code preserves Apache-2.0 attribution/license; FreeRTOS critical sections,
reference counts, dynamic adjustment and SDK headers/runtime are not imported.

Pinned pmu_reg.h P0_0P2A control/analog: CTRL mask4F80 covers force-SW, XPD,
source select, bypass; ANA maskF7800000 covers dref/mul/VDET. Current-limit bit27
and all unrelated fields are preserved. Pinned hp_sys_clkrst_reg.h SOC_CLK_CTRL1
at+18 bits10/11/12 (CSI bridge/host,DSI), PERI_CLK_CTRL03 at+3C bits0/1/4/7
(DSI PHY config/reference,CSI PHY config,DPI) provide clock-enable guards.
SDK LCD example documents VO3 supplying VDD_MIPI_DPHY; all guarded MIPI clocks
must be inactive before off-rail programming or a diagnostic rail change.
Matching active-state adoption is read-only and can preserve running consumers.
The already pinned calibration/PMU/eFuse facts remain in
[the calibration report](native-ldo3-calibration.md).

## API, lifetime and failure contract

Changed files: acquire implementation/public header, private context/MMIO seam
and snapshot refactor, register constants, opt-in probe/CMake/capture checks,
host fixtures and evidence/status documents (exact list/hashes in manifest).

New acquire2500 API reserves fixed VO3 until reset. No release, retune, reference
count, heap or implicit kernel task. Caller must exclusively own PMU access at
boot/core0 with IRQs off and stable supplies. CSR checks enforce hart/MIE, not
every possible ISR/task misuse; pre-scheduler use remains a caller obligation.
This is not SMP synchronization or protection against arbitrary MMIO writers.

Acquisition snapshots/calculates calibration and rechecks a stable snapshot.
A matching active rail is adopted without power writes, even with running MIPI
consumers. Any active source/bypass/voltage/VDET conflict is BUSY, write-free.
An inactive rail requires all seven guarded MIPI clock-enable bits clear before
programming. Then analog fields, software-source routing (still off), and enable
are masked/checked in order, skipping no-op writes. A final full snapshot checks
calibration and register words. Current-limit, delay targets/flags and all other
fields are preserved. No eFuse programming or clocks initialization occurs.

Report stages: NONE0,PLANNED1,ANALOG2,ROUTED3,ENABLED4,VERIFIED5. It records
adoption, conservative may_have_changed (set before an actual write attempt),
selected codes and snapshot/programming-readback words. A failed guard may observe
something different; these fields are not a promise of current hardware state.

Once programming begins, any mismatch/failure leaves the reservation FAULTED;
further calls return FAILED until reset, without peripheral I/O. There are no
retries, release/rollback writes or assumption that failed writes did not apply.
Pre-program rejection/stability failures remain unclaimed. A successful claim
makes later acquire BUSY, also without peripheral I/O. This replaces the SDK's
critical-section/reference-count policy with a smaller explicit boot contract.
OK proves checked registers/reservation, not actual voltage or analog settling.

## Actual target finding and justified scope

The first target run used an overly broad guard that also rejected write-free
adoption. It stopped before any power write because **MIPI clocks remain enabled
after ROM RAM entry**: system clock word202200696 (0x0C0D5678), PHY word691
(0x2B3). Relevant guarded bits were active. This is inherited peripheral state;
ROM load_ram must not be treated as a complete peripheral shutdown/cold boot.
The earlier panel-reset probe had not measured that precondition. Its report is
qualified, and its test now rejects active MIPI clocks before any reset pulse;
that guarded probe was compiled, not rerun physically in this unit.

The final policy permits matching write-free adoption but keeps the inactive
programming/diagnostic-cycle clock guards. This does not bypass protection for
power writes. Final Tab5 run: stage5,adopted1,changed0,preserved1,duplicate_busy1,
with sys_active1/phy_active1. Full eFuse/VO3 snapshots, both clock words and ten
other external-LDO-slot register words compare unchanged. Calibration13/3 and
sixteen stable samples also pass. This tests adoption, **not cold power enable**.

The opt-in controlled disable/enable candidate compiles and preflights, but its
physical run is **NOT RUN** because the observed clock precondition fails. The
programming branch is **HOST-TESTED** only. Before testing it on hardware, port a
bounded, ordered quiesce of the actual DSI/CSI/DMA consumers; do not simply clear
clock bits to pass the guard. SDK esp_lcd_del_dsi_bus disables PHY reference/config
and bus clocks, while panel/DMA teardown is separate; this needs a joint audit,
not reuse of invalid SDK object handles from the previously running image.

## Verification and evidence

- macOS C11/Clang build and **23/23 CTest PASS**. New fixture compiles the actual
  acquire/snapshot/planner over explicit context/MMIO hooks (host-only). It
  verifies exact23-read/3-write off sequence, adoption with consumers active,
  every controlled active conflict, seven live clock guards, skipped unchanged
  writes, unrelated-bit preservation, dropped writes at all three stages,
  unstable snapshots/readbacks/final observations, sticky failures/no later I/O,
  invalid context/revision/eFuse errors and reservation lifetime.
- **ASan/UBSan fixture PASS**, public header **C++98 PASS**. Six compiled mutants
  rejected: accepting conflicts, ignoring readback, clobbering other analog bits,
  early enable, losing sticky failure and ignoring live system clocks. Capture
  checks reject impossible initial states, missing/duplicate records, incorrect
  ownership result and a cycle claim with active MIPI clocks.
- Standalone strict GCC14.2.0 build and runtime-symbol/archive gate PASS for
  adoption and cycle candidate (project objects + libgcc only). Host hooks/reset
  entry do not exist in the target. Adoption image9408B lies entirely in L2MEM_LOW;
  existing linker RWX/empty removed flash-segment warnings remain. Final adoption
  image is byte-identical to the hardware-tested image after comment/format edits.
- Actual Tab5 ESP32-P4 rev1.3/40MHz XTAL on `/dev/cu.usbmodem1301`, adoption run
  as above: PASS. Initial guard refusal is retained. No voltage change or
  enable/disable request was made in either actual RAM run. No physical voltage,
  settling/discharge, load behavior or cold-boot display claim.
- Runner reset returns to preserved flash. Paced maia.py check verifies shell,
  SciCalc `2+3*4=14`, module exit0 and desktop active: PASS. No flash/SD writes.

[Initial guard refusal](native-ldo3-acquire-initial-guard.txt),
[final target capture](native-ldo3-acquire-hardware.txt),
[desktop verification](native-ldo3-acquire-desktop.txt),
[manifest](native-ldo3-acquire-manifest.json). Full source/build/reference snapshots,
logs, sanitizer/mutant scripts and the unrun cycle candidate are preserved in
`../maia-rtos-recovery/native-ldo3-acquire-20261112/`.

```sh
cmake -S . -B /tmp/maia-native-ldo3-acquire-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_LDO3=ON -DMAIA_NATIVE_PROBE_LDO3_ACQUIRE=ON
cmake --build /tmp/maia-native-ldo3-acquire-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-ldo3-acquire-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-ldo3-acquire-20261112/maia_sched_probe_ram.bin --log /tmp/maia-ldo3-acquire-hardware-final.log --ldo3-acquire
cmake -S . -B /tmp/maia-ldo3-acquire-host-20261112
cmake --build /tmp/maia-ldo3-acquire-host-20261112 -j4
ctest --test-dir /tmp/maia-ldo3-acquire-host-20261112 --output-on-failure
```

## Checkpoint and next small task

**HARDWARE-TESTED, LIMITED: matching active-state adoption. HOST-TESTED: inactive
configuration/enable and failure handling.** No production/default display path
is switched; old native fixed-LDO/display code is not made safe by this separate
API. Complete desktop still uses FreeRTOS; native cold display/PSRAM/DMA remain
pending, POSIX DESIGN. Next: audit/port orderly MIPI/DMA quiesce, then validate
native off->on programming under its actual preconditions before DSI bring-up.
