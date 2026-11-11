# RV32F/FCSR context — 2026-11-11

The native RAM diagnostic now preserves single-precision FP registers and FCSR
across real timer preemption. This is a separate checkpoint after integer/tp fix
`b028d91`; no FreeRTOS code/runtime, parser changes or flash writes.

## Independently authored implementation

- Private frame header shared by C and assembly: 128 bytes for integer builds,
  272 bytes with RV32F. Original core offsets remain fixed; appended slots hold
  all f0–f31 and FCSR, plus alignment padding. Unsupported FLEN is rejected.
- Enabled FP states are saved/restored eagerly around the C trap handler.
  FS-Off frames skip FP instructions/CSRs; mixed Off/on task testing is pending.
  Fresh frames enable FS and zero all FP state. No write to read-only MIL.
- Stack construction uses the selected layout; task creation rejects too-small
  stacks and pointer+length overflow before memory writes. Host fixtures cover
  integer and simulated FP frame initialization/bounds.
- `MAIA_NATIVE_PROBE_FP32=ON` opts only the diagnostic into RV32IMAFC/ILP32F.
  C and assembly flags now match. The toolchain previously left ASM flags at
  compiler defaults; the new layout guard exposed its FLEN mismatch. Added
  `CMAKE_ASM_FLAGS_INIT`; use **fresh build directories** to avoid cached old flags.
- Assembly sentinel workload additionally checks all32 FP registers, FCSR,
  and zero initial FP state. A separate C translation unit executes
  `maia_probe_fp_add(2.0f,3.0f)` across actual float argument/result ABI boundaries.
  Disassembly confirms `fadd.s fa0,fa0,fa1`; ELF flags identify single-float ABI.
  ABI check counters are per-task, avoiding lost increments under preemption.

Primary silicon/ABI references and exact revision/page numbers are in
[hardware evidence](../hardware/EVIDENCE.md). Full frame contract and limitations:
[RISC-V context](../architecture/RISCV_CONTEXT.md).

## Hardware evidence

Connected ESP32-P4 rev v1.3; GCC esp-14.2.0_20241119, esptool4.12. RAM only;
runner resets in finally to original flash. All logs are in `native-probe-logs/`.

| Capture | IRQ / switches | WorkA / WorkB | Result |
|---|---|---|---|
| fp-before.log | 3 /4 | 451 /450 | FAIL, f0 contaminated by other task |
| fp-fixed-1.log (intermediate counter) | 250 /251 | 61084 /61575 | PASS |
| fcsr-negative.log | 3 /4 | 448 /447 | FAIL, FCSR expected82 actual0 |
| fp-final-1.log | 250 /251 | 56182 /56634 | PASS |
| fp-final-2.log | 250 /251 | 13977 /14085 | PASS |
| integer-final.log | 250 /251 | 25847 /26048 | PASS |

Final passes: ticks250, reg_errors0, ecalls0, synchronous exceptions0, MIL0.
Both final FP runs check fresh zero state and all live FP registers/FCSR, plus
**112816** and **28062** C float ABI calls respectively. Integer sentinels and
sp/gp/canary checks remain active. Iterations are not performance measurements;
ROM clock state and interrupt phase vary. No yield/sleep/synthetic ticks are used.
The intermediate shared ABI counter could lose increments, so its recorded count
is not used as an exact total; final counters are separate per-task.

Negative FCSR test was an isolated temporary copy of the project's trap assembly,
changing only `csrw fcsr,t3` to `csrw fcsr,zero`, linked with otherwise identical
fixed objects. Production source was never changed for that mutant. The probe
rejects it with `reg=512 expected=82 actual=0` after three real IRQs. This proves
FCSR checks do not pass merely because FP registers survive. Mutation builder
and binary are preserved with recovery artifacts.

Host suite **7/7 PASS**, including both context-size fixtures and invalid stack
sizes/overflow. Kernel integer + FP frame fixtures under ASan/UBSan **2/2 PASS**.
Both final native builds pass the runtime-symbol/archive guard. Linker RWX and
removed empty flash-section warnings remain; no protection guarantees are made.

## Reproduce

```sh
cmake -S . -B /tmp/maia-native-fp-context-final -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_FP32=ON
cmake --build /tmp/maia-native-fp-context-final --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-fp-context-final/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-fp-context-final/maia_sched_probe_ram.bin --log /tmp/maia-native-fp.log
```

For the integer regression use another fresh directory and omit the FP option.
Do not run full native `maia_m0.elf` as if it were an equivalent desktop port.

## Remaining work

Limited single-core SRAM context evidence, not complete M2/native desktop
acceptance. No RV32D/double hardware, mixed float ABI modules, TLS runtime, SMP,
nested interrupt, latency bound or driver equivalence claim. FS-Off/on mixing,
yield/delay/priority stress, fatal-trap policy, stack bounds and cache/DMA still
need independent validation. FP save/restore increases ISR work and stack use;
no latency/performance guarantee is inferred from these runs.

Next small task: test mixed enabled/disabled FP tasks and scheduling operations,
then replace unsafe unknown-fault skipping with bounded fatal diagnostics before
integrating driver services. Preserve working desktop throughout migration.

Final images, ELFs/maps, mutant and builder are preserved with SHA-256 manifest
in `../maia-rtos-recovery/native-fp-context-20261111/`. Final FP image SHA-256:
`2fa6baf2cba5bee77398528a11950705529818b4ad0584b46846a5edcddd9f93`.

After the final RAM reset, `maia.py` verified the preserved original flash app:
shell sentinel echo, SciCalc `2+3*4 = 14`, module exit zero and desktop left active.
Capture: `native-probe-logs/fp-desktop-restored.log`. The native fix was never
flashed over the desktop. Final rebuilds produce byte-identical RAM images to
the final hardware-tested binaries.
