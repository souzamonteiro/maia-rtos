# Native scheduler RAM probe — partial hardware result

This change establishes an isolated acceptance diagnostic and fixes two verified
native SYSTIMER constants. It does **not** complete preemption or replace the
working desktop. No parser/lexer/EBNF changes; no vendor runtime imports.

## Implemented

- Explicit `maia_sched_probe_ram` target, project startup/trap/kernel/SoC objects
  plus compiler `libgcc`; no shell, display initialization or vendor libraries.
- Two equal-priority integer busy tasks with no yield, sleep or synthetic ticks.
  PASS requires 250 real timer IRQs/ticks, both tasks progressing, task switches,
  zero synchronous exceptions and zero ecalls. A cycle budget reports failure
  rather than silently accepting one task running forever.
- Link-artifact regression gate rejects known foreign symbols, unresolved
  references and archives other than libgcc. Positive/negative host fixtures
  exercise the gate. It cannot prove provenance of renamed/copied code.
- RAM loader validates every image segment and entry point against the existing
  L2MEM_LOW linker window before opening a port, captures output and resets in
  `finally` to the preserved flash image. No flash-write command is used.
- SYSTIMER base corrected from `0x500C6000` (I2S0) to `0x500E2000`; target-0
  routing corrected from source 54/offset d8 to source 53/offset d4. Primary
  revision/sections are in [hardware evidence](../hardware/EVIDENCE.md).
- Removed mandatory absolute neighboring-checkout dependency at native CMake
  configuration: optional RGB fixture now uses `MAIA_EXTERNAL_RGB_TEST_DIR`.

## Actual results

Connected USB-Serial/JTAG ESP32-P4 rev v1.3; standalone GCC esp-14.2.0_20241119,
RV32IMAC/ILP32; esptool 4.12.0. Full captured logs are in
[`native-probe-logs`](native-probe-logs/).

| Configuration | Real IRQ / ticks | Switches | workA / workB | Result |
|---|---|---|---|---|
| Original base and routing | 0 / 0 | 1 | 11962368 / 0 | FAIL |
| Routing corrected, old base | 0 / 0 | 1 | 11962368 / 0 | FAIL |
| Base and routing corrected | 1 / 1 | 2 | 0 / 11961344 | FAIL |

All runs report zero ecalls and synchronous exceptions. The final register dump
has `conf=3238002689 raw=1 ena=1 st=1 map=16`, showing a pending enabled alarm.
The first real interrupt arrives and selects task B before task A makes progress;
subsequent interrupts do not arrive. This does not establish the cause of the
remaining failure. Investigate CLIC priority/threshold restoration and delivery
next, with measured CSR state and primary references before changing context code.

Host build/CTest: **6/6 PASS**, including register and dependency negative tests.
Native target builds and runtime-symbol/archive gate passes. RAM image validation
passes; feeding the preserved desktop image with a nonexistent port fails before
serial access, as intended. Existing linker RWX and removed-empty-flash-segment
warnings remain; this diagnostic makes no memory-protection claim.

## Reproduce

```sh
cmake -S . -B /tmp/maia-native-preempt-probe -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake
cmake --build /tmp/maia-native-preempt-probe --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-preempt-probe/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-preempt-probe/maia_sched_probe_ram.bin --log /tmp/maia-native-probe.log
cmake -S . -B /tmp/maia-host-contracts-20261110
cmake --build /tmp/maia-host-contracts-20261110 -j4
ctest --test-dir /tmp/maia-host-contracts-20261110 --output-on-failure
```

After the final RAM test/reset, the preserved flash application was verified
through `maia.py`: shell echo succeeds, SciCalc returns `2+3*4 = 14`, the module
returns zero and the desktop is left active. Capture: `native-probe-logs/desktop-restored.log`.
The verification helper's old `CST_BASELINE_RESTORED` echo is just its existing
sentinel, not a claim that the patched CST image is installed.

## Limits and next step

M1/M2 gates remain open. No FP context, SMP, graphics/PSRAM/driver equivalence,
latency bound or native desktop is established. Full `maia_m0.elf` shell integration
remains incomplete; build the explicit probe target above. Working Tab5 firmware
continues using FreeRTOS; this probe isolates native migration work without
modifying flash, its bootloader, partitions, SD or application sources.

Next small task: capture CLIC CSR state across the first interrupt/context return,
then fix repeated timer delivery with a separate hardware-tested commit.
