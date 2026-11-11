# Integer context preservation — 2026-11-11

Added an independently authored assembly workload to the existing RAM-only
scheduler diagnostic. It holds task/register-specific patterns in **x1, x4 and
x5–x31**, including caller-saved registers, through an interruptible busy window.
It checks those values before calling C. sp and gp retain their normal ABI values
and are checked against stack snapshots; a stack canary is also checked. Pattern
initialization/checking can itself be interrupted. No yield, synthetic ticks,
foreign code or runtime imports. Primary ABI reference: hardware/EVIDENCE.md.

## Reproduced defect and fix

With unchanged `3f3bd80` trap context, the test failed after three timer IRQs:
`REG_FAIL task=1 reg=4 expected=2986344452 actual=2701131784`.
The missing register is **tp/x4**. Switching tasks without preserving it lets a
task observe another task's sentinel, possibly interrupted mid-materialization.

The independently authored fix saves/restores tp in the **previously reserved
word at byte 124** of the existing 128-byte frame. Frame size, mepc/mstatus/mcause
offsets and public task API stay unchanged. Fresh frames already zero all words;
the host regression now asserts the deterministic zero initial tp. This is not
TLS allocation or C/C++ thread-local runtime support. gp remains a shared global
pointer under the current static single-image ABI; changing gp per task is not
supported by this test or kernel.

## Validation

- Native GCC esp-14.2.0_20241119, RV32IMAC/ILP32; esptool 4.12; connected
  ESP32-P4 rev v1.3. Explicit `maia_sched_probe_ram` target builds and its
  native runtime-symbol/archive gate passes.
- Before fix: hardware FAIL with tp corruption, captured in
  `native-probe-logs/registers-before.log`.
- After fix: 250 real IRQs/ticks, 251 task switches, both tasks repeatedly
  validate registers, **reg_errors=0**, zero ecalls/synchronous exceptions,
  MIE=1 and MIL=0. Captures: `registers-fixed-1.log`, `registers-fixed-2.log`.
- Host suite **6/6 PASS**; kernel under ASan/UBSan **PASS**.
- Reproduce using the commands from `native-scheduler-probe.md`; the same target
  now links `tests/hardware/native_register_probe.S`. The assembly checks actual
  live register values, rather than relying on the compiler's incidental usage.

## Files and limits

Production fix: `arch/riscv/trap_context.S` (tp save/restore) and the frame comment
in `kernel/ksched.c`. Diagnostic/host assertions: CMakeLists.txt,
`tests/hardware/native_register_probe.S`, `native_scheduler_probe.c` and
`tests/host/kernel_mvp_test.c`. Evidence/status/handoff updated.

Single-core internal-SRAM integer context only. No FP context/FCSR, SMP,
latency bounds, TLS allocation, PSRAM/cache/DMA, driver or desktop equivalence.
The existing trap policy forces MPIE/MPP and skips unknown faults; review those
separately. Timer-driven tests exercise different instruction points but do not
exhaust all interleavings. Existing native linker RWX warnings remain.

Next small task: establish RV32F/FCSR preservation and consistent frame/ABI
initialization, with a failing-before/passing-after hardware test, separately.
