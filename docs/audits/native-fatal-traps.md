# Native fatal traps — 2026-11-11

Unknown exceptions/breakpoints no longer advance PC and continue. Native traps
now retain cause/original PC/value/sp, emit bounded best-effort USB diagnostics
without display/heap/locks/scheduler, and halt until external reset. The working
flash desktop is preserved; all device tests load only into internal RAM.

## Before and after

Controlled illegal16 injection on `bda25ca` prints `FAULT_RESUMED`: the old
handler skipped 4 bytes, ignoring the fault and a following compressed NOP.
Capture: `native-probe-logs/fault-before.log` (expected-fatal test correctly FAILS).

The new independently authored policy only permits a valid interrupt-enabled
M-mode ECALL with non-overflowing, halfword-aligned PC; dispatch also requires
an active scheduler task. That known 32-bit instruction advances 4. All other
synchronous causes, including breakpoints, halt with the original PC. Unknown
IRQs halt rather than returning into a possible IRQ storm. No instruction is
read/decoded from a potentially invalid fault address.

Private pure helper: `include/maia/arch/riscv_trap_policy.h`.
SoC fatal sink: `soc/espressif/esp32p4/soc_panic.c` / corresponding header.
Dispatch: `soc/espressif/esp32p4/soc_timer.c`. Native CMake targets link the sink;
no foreign runtime/code or generated parser/lexer changes.

The fatal record is volatile RAM, valid marker published last after a fence;
no persistent storage write. Fixed output is 73 bytes; each byte has at most 4096
USB ready polls, with 32-byte packet commits. No latency bound is claimed from
this loop bound. A recursive software entry halts immediately, but hardware
stack/double-fault recovery is not implemented. Normal console globals and display
hooks are bypassed. USB output is best-effort; original record remains only
until reset. There is no automatic restart or per-task fault isolation.

## Test protocol

RAM diagnostic variants: `MAIA_NATIVE_PROBE_FAULT=illegal16|illegal32|breakpoint16`.
Faults occur at a halfword-aligned symbol `maia_probe_fault_instruction`.
The 32-bit case writes the read-only cycle CSR. Primary ISA/silicon definitions,
revisions and sections: hardware/EVIDENCE.md. These are explicit test-only
instructions in project-authored assembly, not imported OS implementation.

The host runner compares fatal cause and PC to the **actual symbol address in
the matching ELF**, supplied with `--expect-fatal-cause` and `--expect-fatal-pc`.
It rejects wrong cause/PC, duplicate fatal records, any FAULT_RESUMED or normal
PASS/FAIL marker. If an early FAULT_ARMED banner is captured it must match too;
it is optional because esptool/RAM boot may consume initial serial output.
No successful result is inferred solely from absence of resumed output.

An intermediate illegal32 capture showed the correct fatal cause/PC but lost
the early armed banner, and was correctly rejected by the original banner-only
validator. The final validator uses independent ELF metadata and retains strict
PC/cause requirements. That incomplete-capture FAIL is retained separately.

## Validation

ESP32-P4 rev v1.3, standalone GCC esp-14.2.0_20241119, RV32IMAC/ILP32 for
injection tests, esptool4.12, `/dev/cu.usbmodem1301`.

- Illegal16: raw cause 0x30000002, expected PC from ELF 0x4ff00a2e, no resume.
- Illegal32: raw cause 0x30000002, PC 0x4ff00a2e, value 0xc0001073, no resume.
- C.EBREAK: raw cause 0x30000003, exact ELF PC, no resume.
- Mixed FS/priority/yield/delay workload is rerun after the policy change to
  prove valid scheduler ecalls still resume and native timer preemption works.
- Host **9/9 PASS**. ASan/UBSan C policy + integer/FP kernel fixtures **3/3 PASS**;
  Python result-decoder fixtures pass separately. New headers compile as C++98.
- Native artifact runtime-symbol/archive gate passes for all diagnostic variants.

Raw results are captured in `native-probe-logs/fault-*-final.log` and
`mixed-after-fatal.log`. Exact final counters and symbol metadata are preserved
with recovery artifacts; see the appended run results below.

## Reproduce

```sh
cmake -S . -B /tmp/maia-native-fault-illegal16 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_FAULT=illegal16
cmake --build /tmp/maia-native-fault-illegal16 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-fault-illegal16/maia_sched_probe_ram.elf
```

Read `maia_probe_fault_instruction` with standalone `riscv32-esp-elf-nm` from
that ELF; pass its address to the runner with expected cause 2 (illegal) or 3
(breakpoint). Match the image/ELF, never reuse an address from a different build.
The runner validates RAM segments/PC before opening the port and resets in
finally. It never flashes the diagnostic. Recovery includes the exact orchestration
script and artifacts for all variants.

## Remaining limits and next small task

Single-core native SRAM validation, not native desktop/SMP completion. Unknown
IRQ halt and invalid-context yield rejection are implemented, but dedicated
hardware injections for those paths are not run in this unit. No memory isolation,
stack-overflow handling, alternate emergency stack or bounded fault-to-output
latency guarantee. Fault record RAM contents are not read back via a debugger in
this unit; actual cause/PC/value/SP are verified through emitted serial output.
Known native linker RWX warnings and incomplete full native shell integration
remain. No existing app/module ABI or legacy flash runtime is migrated here.

Next small task: address atomicity/interrupt-state contracts of scheduler task
operations and define/test idle/no-ready behavior before native driver integration.

Final actual results: all three controlled faults PASS against ELF PC 0x4ff00a2e,
with causes 2/2/3, no FAULT_RESUMED. After the fatal-policy change the mixed test
passes 250 IRQ/ticks, 18142 ecalls, 18267 switches, 124 sleeps, 125 priority checks,
374 FS-Off checks, reg_errors0 and MIL0. Host runner invalid cause/PC options
(missing PC, odd PC or PC outside image) are rejected before serial access.

After reset, maia.py verified the original flash desktop: shell echo, SciCalc
2+3*4=14, module exit 0 and desktop active. Capture `fatal-desktop-restored.log`.
No diagnostic was flashed or SD file written.
Final images/ELFs/maps, exact symbol PCs, SHA-256 manifest and orchestration script:
`../maia-rtos-recovery/native-fatal-traps-20261111/`.
