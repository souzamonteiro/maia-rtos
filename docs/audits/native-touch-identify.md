# Native boot-only touch identification — 2026-11-11

**HARDWARE-TESTED, LIMITED.** Read the validation and limits below.
Scope: bounded standalone GPIO I2C register reader and Tab5 identity diagnostic.
No expander/reset/LDO/display/DMA writes or legacy driver replacements.

## Source facts and implementation prerequisites

User-supplied Tab5.pdf, update2026-11-05, PDF page12 PinMap: touch SDA31,
SCL32, ST7121/ST7123 address0x55. PDF pages8–10 include block diagram and raster
schematics (visually inspected), not searchable register-level documentation.
Sitronix ST7123 TDDI Interface Protocol A v01.11, 2023-12-25, PDF page9
§5.3.1–2 defines address0x55 and register-address high/low bytes, STOP then
START for a register read. Page10 §5.3.4 specifies I2C timing; page15 §6.1–2
identifies firmware0x0000 and status0x0001, with Init requiring a wait before accessing other registers. The bounded
read-only identity policy accepts Normal(0) and Idle(4), rejecting all other
status bytes (including error bits); Idle access requires the device test below. ST7121 shares this access in preserved BSP/device evidence;
this is not a claim that every ST7123 electrical specification applies to ST7121.

Primary documents:
- [Tab5 PDF](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Tab5.pdf), local `docs/hardware/Tab5.pdf`, update above.
- [Sitronix protocol PDF](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123-TDDI-Interface-Protocol-V01.11.pdf.pdf), local datasheets copy, revision/pages above.
- [ESP32-P4 rev1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf), chapter9 GPIO/IO_MUX (IO_MUX summary p552), chapter15 System Timer §15.8 pp1041/1044 UNIT0 snapshot and valid bit. Existing timer clock-map evidence in EVIDENCE.md applies.
- [Pinned GPIO register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/gpio_reg.h), GPIO_FUNC31/32_OUT_SEL_CFG offsets0x5d4/0x5d8, OEN_SEL bit10 selects GPIO_ENABLE; GPIO_IN/IN1 and output-enable W1TS/W1TC. Companion io_mux_reg.h GPIO31/32 offsets0x80/0x84, HP GPIO function1, input enable/pull-up. These are register facts, not imported runtime code.

New output drives low only: zero latch + open drain, output-enable clear releases
the line. Function matrix explicitly selects software output enable. Native delay
uses verified 16MHz SYSTIMER snapshots, never presumed CPU frequency. Snapshot and
progress loops have finite iteration caps; failure returns TIMEOUT. This is a
boot-exclusive mechanism, not a general concurrent task driver.

## Contract and changed files

`drivers/i2c/bitbang.c` and its C/C++ header provide address probe and a single-byte
16-bit-register read. Addresses outside0x08..0x77 and missing callbacks/output
are rejected before I/O. No unrestricted register write API was added.
Register pointer bytes are high then low; pointer write ends with STOP, followed
by START/address-read. Master sends NACK for its final received byte. Results:
OK, INVALID, NACK, TIMEOUT or BUSY. Output is committed only after successful
final STOP. NACK cleanup failure supersedes NACK. All valid-call exits release
both lines; no automatic recovery pulses or retries hide failures.

Each released-SCL wait has at most40 polls, each with a5us pause; after observing
high, another complete5us high phase is held. This prevents stretching that ends
just before a sample from shortening the high pulse. Bounds are loop/policy
bounds, not a measured maximum wall-clock duration or a hard-real-time guarantee.

`soc_boot_i2c.c` supplies HP-GPIO input/low/release callbacks and bounded timer
capture/progress waits (256 snapshot polls,1024 progress samples). No CPU-cycle
frequency assumption or fallback is used. Prepare requires core0/IRQs already
off; the caller must additionally ensure pre-scheduler ownership. It does **not**
detect all ISR/task misuse or arbitrate peripheral/other-core access. Context
storage is stationary until reset; no teardown. Only GPIO31/32 were target-tested;
other accepted HP pins16..56 have not been exercised.

`boards/m5stack/tab5/touch_identify.c` reads status then firmware with no guessed
panel fallback. Normal0/Idle4 permit read-only identification; all other byte
status values are rejected as BUSY. Firmware remains raw, including unknown
values. Mapping1/3 to ST7121/ST7123 is the baseline BSP's inference, not LCD-ID
verification. This policy must not be generalized to display initialization.

CMake adds an opt-in `MAIA_NATIVE_PROBE_TOUCH` profile, rejecting combinations
with other probe workloads. The existing runner adds `--touch-identify`, requiring
complete/stable identity and both negative probes; its existing image-memory
preflight and finally-reset remain in force. No production console, board-init,
display, scheduler, grammar or legacy platform call path is switched to this API.

## Actual tests and hardware evidence

Environment: macOS host, host C11/Clang, target Espressif GCC14.2.0 standalone,
esptool4.12.0, ESP32-P4 rev1.3/40MHz XTAL over `/dev/cu.usbmodem1301`.

- Host build and CTest **19/19 PASS**. New virtual-slave test checks actual
  observed wire bytes/pointer order, master NACK, complete high phase after
  stretch, all149 pause-failure positions, four slave-NACK stages, bounded held
  SCL, busy SDA, short/long stretching, failed STOP, NACK cleanup priority,
  invalid inputs and status policy for all256 possible values.
- Same C fixture **ASan/UBSan PASS**, public headers **C++98 PASS**.
- Six compiled negative mutants rejected: ignoring stretch timeout, ignoring
  slave NACK, publishing before STOP succeeds, ACKing final byte, wrong pointer
  byte order and missing full high hold after stretch. Mutants are temporary
  test artifacts outside repository sources. Invalid combined CMake profile
  rejects configuration as expected. Runner result positive/negative tests pass.
- Native strict build and runtime-symbol/archive gate **PASS** (project objects
  and libgcc only). RAM image/entry-point preflight **PASS** within L2MEM_LOW.
  Existing linker RWX/empty flash-segment warnings remain; no memory-isolation
  claim. Final image has a7936-byte loaded segment; reserved display bands absent.
- First target run failed identification. Diagnostic follow-up successfully
  read raw status4 (Idle); the initial status==Normal-only policy was too narrow
  for read-only ID. Both failures are retained, not rewritten as hardware I2C
  faults. Final source includes Normal/Idle policy and full high-hold correction.
- Final target capture: status result0/raw4, **16 reads firmware1**, recovery read
  firmware1, errors0; actual address0x56 NACK, output sentinel preserved; sampled
  SCL-low injection returns TIMEOUT and actual GPIO inputs are released high.
  **The timeout injection changes sampled input, not the electrical wire.**
  A physical stuck bus and a real frozen/missing timer were not injected.
- Runner reset to preserved flash. maia.py then verifies shell echo, SciCalc
  `2+3*4=14`, module exit0 and desktop active: **PASS**. No flash or SD writes.
  This is serial behavior evidence, not a physical pixel test.

Tracked captures: [initial failure](native-touch-initial-fail.txt),
[Idle diagnosis](native-touch-idle-diagnosis.txt),
[final hardware](native-touch-hardware.txt),
[desktop verification](native-touch-desktop.txt).
[Manifest](native-touch-identify-manifest.json) records document/source/artifact
hashes. Full build artifacts, source copies, host/mutant logs and scripts are in
`../maia-rtos-recovery/native-touch-identify-20261111/`.

```sh
cmake -S . -B /tmp/maia-native-touch-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_TOUCH=ON
cmake --build /tmp/maia-native-touch-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-touch-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-touch-20261111/maia_sched_probe_ram.bin --log /tmp/maia-touch-hardware-final-verified.log --touch-identify
cmake -S . -B /tmp/maia-touch-host-20261111
cmake --build /tmp/maia-touch-host-20261111 -j4
ctest --test-dir /tmp/maia-touch-host-20261111 --output-on-failure
```

## Limits and next small task

**HARDWARE-TESTED, LIMITED: warm-reset native read-only I2C identification.**
Board expanders/panel retain prior supply/reset state from the functional image;
this does not establish cold panel bring-up, reset ownership, LDO, PSRAM/cache/DMA,
SMP, multi-master, concurrent task I2C, arbitrary sensors or a native desktop.
Legacy board-I/O helpers still have their original problems; they are not silently
fixed by adding an isolated transport. No FreeRTOS component was removed from
the working complete firmware in this unit; native diagnostic imports none.

Next: independently specify a bounded checked expander-register access and
preserve other output/direction bits before a separate native reset/power probe.
Use the supplied schematics and safe LCD reset-release rule, review write/error
contracts first, and preserve recovery. Do not couple this work to DMA/PSRAM or
replace the working firmware until equivalent functionality is demonstrated.
