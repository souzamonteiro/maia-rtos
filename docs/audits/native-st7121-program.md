# Native selected ST7121 command program — 2026-11-14

**HARDWARE-TESTED, LIMITED:** two final RAM loads complete31 command packets,
461 command+parameter bytes,1150ms minimum requested waits; four stable error-free
RDDID response words afterwards. Commands include actual multiword packets up to
61 bytes. This proves bounded host transport completion and continued response,
not vendor-register readback, sleep/display state, pixels or complete native video.

## Port source, license and contract

[Preserved driver](https://github.com/souzamonteiro/maia-rtos/blob/883e9a6/platforms/tab5/components/esp_lcd_st7121/esp_lcd_st7121.c)
revision883e9a6, SHA256
f6e5454fdcca02bf5a71cf67c6ae16e74f4abb95093c5ab98fcfd8500a04a6fa:
`panel_st7121_reset` (reset_gpio=-1), `vendor_specific_init_default` and
`panel_st7121_init` are the complete local reference. Copyright2024 Espressif,
Apache-2.0 retained in `st7121_program.h`; full license in LICENSES/Apache-2.0.txt.
Native executor is original MIT. The owner authorized adapted open-source ports.
No vendor headers/types, task/delay/heap/lock/DPI/runtime implementation imported.

The unchanged [working BSP](https://github.com/souzamonteiro/maia-rtos/blob/883e9a6/platforms/tab5/components/m5stack_tab5/m5stack_tab5.c)
selects ST7121 for touch firmware1; only that value is accepted here. Firmware3
selects ST7123 and is rejected by this program. This selection is BSP policy, not
independent silicon identification; raw DSI00FBA080 is not used to guess a model.

New board-layer API takes immutable write/wait callbacks; it has no SoC MMIO or
kernel/foreign runtime dependency. Caller supplies fresh verified board reset,
stable supply/DSI, native boot CPU0/IRQs-off and exclusive ownership. One lifetime
claim until SoC reboot: successful duplicate BUSY, any callback failure sticky
FAILED. Validate inputs/variant before callbacks; no retry, rollback, compensating
software reset or continued command after a partial write/wait failure. Attempt
counter precedes callback; completed counter follows its successful wait. Report
retains callback error and last command. No generic runtime concurrency service.

Exact pinned program: software reset01/wait120ms, then30 vendor table entries,
including original unlock/page sequence, sleep-out11/80ms, display-on29/800ms and
TE35/00. Each table entry retains the source's additional5ms wait (including those
with declared0ms). All literal counts match explicit data_bytes; no guessed tails
or silently corrected vendor values. Max60 parameters+command=61 bytes. Native
fixed data table uses explicit lengths; no parser/grammar/generator modification.
An independent source extraction/comparison and fixed test stream hash F644B840
(length+bytes+little-endian wait for each entry) protect exact order/data/delays.

The vendor driver's call to DPI init and BSP's later duplicate DISPON29 are not
included: no video/framebuffer driver is available yet. This API deliberately
claims a command program, not replacement of the complete vendor panel object.

## FIFO refusal found and corrected

First RAM run stopped at A3/40bytes before header publication, stage3/10000 polls.
Diagnostic rerun recorded FIFO90015, PHY15BD, two words/eight bytes staged, no
header. Native long transport correctly poisoned itself and the program stopped;
no partial WC header or retry was attempted. Initial images/source/logs preserved.

The internal buffer full bit19 was incorrectly used to deny front-interface
space. Primary pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
`mipi_dsi_host_ll_gen_is_write_fifo_full`, uses GEN_PLD_W_FULL/bit3 only;
[HAL long/DCS path](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
checks it before each GEN_PLD write. Internal GEN_BUFF_PLD_FULL/bit19 describes
another level and can coexist with writable generic FIFO.90015 has bit3 clear.
Native space now follows that interface contract; no capacity was invented.

Initial/final F007F=50015 checks still require both interface and internal buffers
empty/not-full; root/supply/PHY/error guards, byte-safe tail packing, exact WC,
shared aggregate budget, staged-before-header cap64 and sticky failure unchanged.
No error is masked/ignored. Host regression recreates internal-full/front-empty
with a61-byte packet; old space predicate mutant fails. Full front FIFO continues
to time out before header, including partial staging. This supersedes only the
old long audit's space-mask80008 policy, not its stronger completion requirement.
Observed61-byte success is not a general FIFO-depth or arbitrary streaming proof.

## Actual verification

Host39/39 CTest PASS. New board-program fixture covers pinned full stream/hash,
unsupported variants/invalid inputs, duplicate success and all31 write+31 wait
failure positions, no following callback/retry. Existing long fixture adds90015
room regression and keeps full-FIFO/partial timeout/internal-drain negative tests.
Capture evaluator requires all31 exact command/length rows, stage/poll/error
checks, completed waits and four stable restored post-program responses in order.

ASan/UBSan PASS for program and actual long transport; C++98 public header PASS.
Five compiled runtime mutants rejected: wrong unlock, shortened reset wait,
ignored write error, retry failed program, old internal-full space predicate.
Invalid native option combination rejected. Table byte/length/wait comparison
against883e9a6 PASS. Native GCC14.2 RV32IMAC/ILP32/runtime gate PASS: project/libgcc
only, no unresolved/foreign symbols. Existing RWX/empty removed-segment warnings.

Two final image loads, Tab5 chip1.3/40MHz/cu.usbmodem1301:

- Proven native gate/VO3/reset/touchFW1/DSI setup, prior16short/4long/4read pass.
- All31 program packets pass: attempted/completed1F, waited_ms47E, last35,
  callback0. A7/61bytes long reaches stage6/23 polls/errors0.
- Subsequent four RDDID words each00FBA080, stage10/7polls/headers3/oneword,
  PHY15BD/FIFO50015/errors0; modeB0000 restored, restoration mask5.
- Both runner exits0/PASS. Original shell/SciCalc `2+3*4=14`/module exit0/desktop
  recovered. No flash/SD writes. Panel startup changes volatile state only.

Software wait backend uses existing native pause>=5us,200 bounded calls per ms;
it detects stalled timebase through that callback. Total requested1150ms is a
minimum, not WCET; wall time includes MMIO/serial/checks. No physical clock,
voltage, waveform or hard-real-time guarantee. No pixel/state-readback claim.

## Reproduction, recovery and next task

Full prior DSI_RESET configuration plus `-DMAIA_NATIVE_PROBE_ST7121=ON`, build
`/tmp/maia-native-st7121-20261114`. All new dependencies are project-native.

```sh
cmake --build /tmp/maia-native-st7121-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-st7121-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-st7121-20261114/maia_sched_probe_ram.bin --log /tmp/maia-st7121-final-hardware.log --st7121 --capture-seconds 15
python3 /tmp/maia-st7121-checks.py
python3 /tmp/maia-st7121-verify-table.py
```

[Final capture](native-st7121-hardware.txt), [repeat](native-st7121-repeat-hardware.txt),
[desktop](native-st7121-desktop.txt), [manifest](native-st7121-manifest.json).
Recovery `../maia-rtos-recovery/native-st7121-20261114/` includes tested source/bin,
initial refusal candidates, complete raw captures, source table extraction/checks,
pinned vendor references and preserved original firmware hashes. Changed board
API/program/executor, SoC space predicate/comment, native adapter/profile/runner,
host/capture fixtures and evidence. No parser/EBNF/default production changes.

Next: bounded panel-state readback (sleep-out/display-on and relevant configuration)
to distinguish command drain from applied panel settings, before native DPI video
and framebuffer/PSRAM. The complete installed desktop still uses FreeRTOS; no
native desktop equivalence or POSIX implementation is claimed.
