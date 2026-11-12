# Native VO3 calibration planning — 2026-11-12

**HARDWARE-TESTED, LIMITED:** read-only prerequisite to native MIPI-power acquisition. No PMU programming,
LDO ownership, output enable, eFuse commands/burning or DSI clocks in this unit.

## Primary source ledger

- ESP-IDF revisionf5c3654a1c2d2a01f7f67def7a0dc48e691f63c0,
  [ldo_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h),
  ldo_ll_voltage_to_dref_mul: VO3 uses unit2 and eFuse K/Vos/C; signed-magnitude
  coefficients with distinct zero defaults, fixed-point model,128 candidates,
  strict first-minimum tie order. Unit-to-PMU index array maps VO3 to ext_ldo[1].
  Native pure-C port retains 2023–2025 Espressif copyright/Apache-2.0 and adds
  explicit validation, int64 arithmetic, model/default diagnostics. Full license
  in LICENSES/Apache-2.0.txt; no runtime header/lock or SDK linkage imported.
- Same SDK [efuse_reg.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/efuse_reg.h),
  RD_MAC_SYS_2 offset4C revision/schema flags, RD_MAC_SYS_3 offset50 VO3 K[13:6],
  Vos[19:14],C[25:20], RD_RS_ERR0 offset1C0 block1 uncorrectable bit3.
  reg_base.h maps eFuse base5012D000; efuse_hal.c defines version major*100+minor.
  pmu_reg.h P0_0P2A offsets1C0/1C4 defines VO3 control/analog fields. These pinned
  headers are the source for the exact fields; no claim the older TRM documents
  the complete factory-calibration model.
- SDK esp_hw_support/ldo/esp_ldo_regulator.c acquire path: convert, configure
  voltage/owner/ripple suppression, enable; locking/reference lifecycle is a
  separate dependency to replace. Local M5GFX Bus_DSI.inl delegates acquisition
  to that SDK service. Baseline Tab5 BSP requests channel3/2500mV.
- Native old soc_display.c instead writes fixed D5800000 and enable/owner bits;
  this audit does not treat its “eFuse calibrated” comment as an actual eFuse
  read. Its display path is not invoked or changed in this unit.

## Scope and validation policy

The work began2026-11-11 and target validation completed2026-11-12; temporary
build directories retain the start-date suffix. Native code is a pure calibration
planner plus read-only snapshot, **not** a regulator driver yet. Existing native
soc_display.c and the default complete firmware are not modified or switched.

Snapshot is boot/core0/IRQs-off only and assumes readable ROM-populated shadows.
It reads two eFuse data words, block1 error status and two existing VO3 PMU words.
No eFuse read/program command, clock change, PMU write or rail toggle is emitted.
Planner is pure/reentrant with bounded128-candidate integer arithmetic, no heap,
FP or foreign headers. It accepts chip1.3, schema major0/minor1..7 and enabled
revision flags; unknown revision/schema flags fail closed. An uncorrectable
block1 flag rejects; a corrected-error count alone is not treated as failure.
This deliberately narrows the SDK's broader compatibility/fallback policy.
Zero trim components use the SDK's documented-in-code defaults and report a
bitmask, avoiding a silent assumption that every coefficient is programmed.
Output commits only on success. The calculated voltage/error is a model estimate,
never a measured supply or a bound on analog accuracy.

Ownership, reference counts, arbitration, hardware-source selection, enable,
settling and failure after a write are **not implemented** by this API. In
particular, PMU's software-selection flag does not prove an exclusive native
resource reservation. A later acquire path must not retune an already-active
mismatching rail or overwrite unrelated register fields.

## Actual tests and device observations

- Host macOS C11/Clang build and **22/22 CTest PASS**. New fixture covers114,752
  trim sets, signed-magnitude extrema/negative-zero/zero defaults, every128
  candidate's global-optimum/tie-order property, exact nominal2500mV case,
  output preservation on error, unsupported revisions/schema and eFuse failure.
  Same fixture **ASan/UBSan PASS**; public header **C++98 PASS**.
- A separate host differential harness compiles the exact conversion function
  extracted from the pinned SDK file with mock calibration fields only. All
  **1,048,576 K/Vos/C combinations** match dref/mul for VO3/2500mV, with no rail
  bypass. The harness/reference remain in recovery, not the target image. This
  verifies the port's arithmetic, not hardware voltage.
- Six compiled negative mutants rejected: wrong K sign, wrong zero default,
  last-minimum tie selection, missing multiplier candidates, ignored eFuse
  failure and accepted unknown schema. Native capture checks independently
  recompute coefficients/grid selection from logged raw trims; invalid mixed
  CMake profile rejects. Existing host regressions pass.
- Standalone strict GCC14.2.0 native build and symbol/archive gate PASS (project
  objects + libgcc only). esptool4.12 image preflight confines the7984-byte segment
  and entry to L2MEM_LOW. Existing RWX/empty removed flash-segment warnings
  remain; no memory-isolation claim.
- Actual ESP32-P4 rev1.3 /40MHz XTAL on `/dev/cu.usbmodem1301`: block schema0.3,
  version-disable flags0, block1 uncorrectable0/corrected0. Raw trims **K55,
  Vos33,C33** decode to **K1000=1030,Vos1000=-4,C1000=989**, default_mask0.
  Selected **dref13/mul3**, modeled output2504637uV, modeled error4637uV.
  This matches the old fixed code on this particular chip, but the old native
  source does not perform per-chip calibration reads or calculation.
- PMU already reports enabled1/software-selection1/bypass0/source0,
  dref13/mul3/VDET1. **That is inherited hardware state, not power initialized
  by MaiaRTOS in this probe.** Sixteen snapshot/plan samples keep both eFuse
  words, error status and PMU words unchanged, errors0. No physical voltage,
  cold enable, settling or powered-load behavior was measured.
- Initial unpaced3s desktop capture did not obtain the calculator result and
  ended inside audio logs. A repeat with paced keys and8s final capture passed;
  both logs are retained, without a firmware change or a claimed diagnosis of
  the first capture failure.
- Runner resets to preserved flash; maia.py verifies shell echo,
  SciCalc `2+3*4=14`, module exit0 and desktop active: PASS. No flash/SD write.

[Hardware capture](native-ldo3-hardware.txt),
[desktop verification](native-ldo3-desktop.txt),
[initial desktop capture failure](native-ldo3-desktop-initial-fail.txt),
[manifest](native-ldo3-manifest.json). Exact source/artifact/reference snapshots,
licenses, host/sanitizer/mutant/differential logs and scripts are preserved in
`../maia-rtos-recovery/native-ldo3-calibration-20261112/`.

```sh
cmake -S . -B /tmp/maia-native-ldo3-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_LDO3=ON
cmake --build /tmp/maia-native-ldo3-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-ldo3-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-ldo3-20261111/maia_sched_probe_ram.bin --log /tmp/maia-ldo3-hardware.log --ldo3
cmake -S . -B /tmp/maia-ldo3-host-20261111
cmake --build /tmp/maia-ldo3-host-20261111 -j4
ctest --test-dir /tmp/maia-ldo3-host-20261111 --output-on-failure
```

## Checkpoint and next small unit

**HARDWARE-TESTED, LIMITED:** reading calibration/current state and calculating a
native plan, with exact SDK arithmetic compatibility. No FreeRTOS/ESP-IDF runtime
imports, parser changes, eFuse commands, regulator ownership or power writes.
The full desktop remains FreeRTOS-dependent; native display/DSI/DMA are pending.

Next: native boot-exclusive VO3 acquire with explicit lifetime and error state,
adopting an already-active matching configuration without retuning it; reject
active conflicts. For an inactive rail, masked program/verify of voltage,
software-source selection and enable must preserve unrelated fields. Test
partial-write failures before a controlled hardware enable, and do not infer
physical voltage or cold-boot display parity from register readback alone.
