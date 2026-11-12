# Native PI4IOE5V6408 configuration port — 2026-11-11

**HARDWARE-TESTED, LIMITED:** single-core boot transport, configuration reads and
same-value output-latch writes on the two Tab5 expanders. Changed-bit RMW and
readback error handling are HOST-TESTED. No panel reset/power sequence yet.

## Owner authorization and source

The owner explicitly permits adapting open-source M5Stack/ESP-IDF/owner code and
portable drivers found in FreeRTOS, preserving the requirement of no FreeRTOS
underneath MaiaRTOS. Root AGENTS.md now records this clarification; the former
blanket prohibition on copied implementations no longer overrides that direction.
No FreeRTOS scheduler, headers, APIs or runtime dependency was introduced here.

This C port follows M5Unified's PI4IOE5V6408 configuration and bit-update model:
`../M5Stack/M5Unified/src/utility/PI4IOE5V6408_Class.inl`, `I2C_Class.inl`, and
`IOExpander_Base.hpp`, local revision44952cb8e4ac88bf04ad23b9dfd6f4b05c788348.
Inspected files match that local HEAD. [M5Unified upstream](https://github.com/m5stack/M5Unified)
is the project reference; the local revision/hashes in the manifest identify the
actual sources studied, without assuming a public upstream commit exists.
M5Stack copyright/MIT license is retained in source and
[LICENSES/M5Stack-MIT.txt](../../LICENSES/M5Stack-MIT.txt).
No C++ M5 runtime or underlying platform I2C backend is imported/linked.

Maia adds typed bus errors, a restricted register set, no-op elimination and
full-byte readback verification. This unit touches the native transport/driver,
public headers, isolated probe/CMake, host tests/runner, owner instruction and
status/evidence documents; it does not replace the legacy board driver.

## Hardware references

Primary [Diodes datasheet](https://www.diodes.com/datasheet/download/PI4IOE5V6408.pdf),
DS40583 Rev3-5, September2022, pp8–10 register map/configuration and p13
Figures6/7: single-byte pointer and data, repeated START for read; no burst-read
assumption. Figure page rendered and visually inspected; file hashed in recovery.
Allowed config registers are03 direction,05 output latch,07 high impedance,
0B pull enable,0D pull selection. ID/control and interrupt/status accesses are
excluded because of side effects. OUT05 readback is latch state, **not a voltage
measurement**. Existing [Tab5 audit](display-baseline-audit.md), user Tab5.pdf
update2026-11-05 pp9/12–13 and M5Unified board power code identify board wiring.

## Contracts and implementation

`drivers/i2c/bitbang.c` gains read_register8 (repeated START) and write_register8
(one data byte). Existing read16 touch semantics remain STOP/START. Both retain
bounded clock waits, NACK/error cleanup and line release. Read output commits only
after successful STOP. A write error can follow device application of the byte;
there is no transactional rollback guarantee, automatic retry or recovery pulse.

`drivers/gpio/pi4ioe5v6408.c` exposes read_config/update_config, only address43/44
and the five config registers. Values containing bits outside the supplied mask
are INVALID before I/O. Update reads the actual byte, preserves unmasked bits,
skips unchanged writes, otherwise writes and rereads the whole byte. Any readback
mismatch returns the added MAIA_I2C_VERIFY=-5; existing enum values are unchanged.
A bus failure propagates without further access. On failure hardware may already
have changed: caller must resynchronize, not assume the old byte is intact.

The complete read/modify/write/verify must be exclusively owned by the caller.
No locks, ISR calls, concurrent task/SMP or external-master synchronization are
provided. The GPIO backend remains pre-scheduler/core0/IRQs-off only, with a
stationary context and exclusive pin/SYSTIMER capture ownership until reset.
No board reset/power policy or multi-register atomic sequence is hidden here.

## Validation

- macOS host C11/Clang build and **20/20 CTest PASS**. Wire fixture validates
  read8 address/pointer/read-address, repeated START (two STARTs/one STOP), write8
  address/register/data, and every byte-NACK stage. Fault injection covers all
  **119 read8 /88 write8 pauses**, preserving read output and releasing lines.
  Existing149-position read16/touch tests still pass.
- Driver fixture checks **65,536 old-byte/mask combinations**, unchanged-byte
  no-op, failures at each RMW phase (including applied-but-failed write), all
  eight readback-corruption bits and register/address restrictions. Errors stop
  subsequent accesses; full-byte verification includes unmasked bits.
- Both C fixtures **ASan/UBSan PASS**, public header **C++98 PASS**. Five compiled
  negative mutants rejected: clobbering other bits, ignoring verify mismatch,
  unnecessary write, invalid mask/value acceptance, ignored write failure.
  Runner positive/negative capture checks pass.
- Standalone target GCC14.2.0 strict build and runtime-symbol/archive gate PASS:
  project objects + libgcc, no unresolved or foreign runtime symbols. esptool4.12
  image preflight confines the9440-byte segment and entry to L2MEM_LOW. Existing
  RWX/empty removed flash-segment linker warnings remain, without isolation claim.
- Actual ESP32-P4 rev1.3/40MHz XTAL on `/dev/cu.usbmodem1301`: both expanders
  snapshot five config bytes, write the **identical existing OUT05 byte**, exercise
  the masked no-op path, then compare all five bytes for16 rounds.
  Bank43: DIR111 (0x6F), OUT102 (0x66); bank44: DIR185 (0xB9), OUT9 (0x09).
  Both report mirror1/unchanged1. This requests no output transitions; an
  oscilloscope was not used to establish absence of electrical glitches.
- Actual0x56 address-write NACK aborts before any register/data write. Touch
  regression reads status4/firmware1 sixteen times plus final recovery read,
  errors0; sampled-SCL injection TIMEOUT remains a **software input injection**.
  No physical short, cold power-up, DMA, PSRAM or display-reset test.
- Runner resets to the preserved flash app. maia.py verifies shell echo,
  SciCalc `2+3*4=14`, module exit0 and desktop active: PASS. No flash or SD write.

[Native capture](native-expander-hardware.txt),
[desktop check](native-expander-desktop.txt),
[manifest](native-expander-manifest.json). Complete source/artifact snapshots,
M5 reference/license, datasheet and raw host/device logs are in
`../maia-rtos-recovery/native-expander-port-20261111/`.

```sh
cmake -S . -B /tmp/maia-native-expander-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_TOUCH=ON -DMAIA_NATIVE_PROBE_EXPANDER=ON
cmake --build /tmp/maia-native-expander-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-expander-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-expander-20261111/maia_sched_probe_ram.bin --log /tmp/maia-expander-hardware.log --expander
cmake -S . -B /tmp/maia-expander-host-20261111
cmake --build /tmp/maia-expander-host-20261111 -j4
ctest --test-dir /tmp/maia-expander-host-20261111 --output-on-failure
```

## Next small task and remaining risks

Port the board's LCD/touch reset sequence over this checked transport, preserving
unrelated bits, reviewing intermediate states and failures, and releasing LCD
reset through input/pull-up as required. First test bounded delay and safe state
transitions separately from LDO/DSI/PSRAM/DMA. Actual changed reset/output bits
have not been tested in this unit; a wrong board mask can still affect power.
Legacy desktop remains FreeRTOS-dependent; this port is not the complete removal
or a functioning native display. Future POSIX remains DESIGN only.
