# Fresh task CLIC return fix — 2026-11-11

The native single-core integer busy-task test now passes repeated real timer
preemption on the connected ESP32-P4 rev v1.3. This fixes the one-IRQ stall from
checkpoint `5b08561`; it does not port the desktop or remove its legacy runtime.

## Investigation and independently authored fix

The owner-provided `/Users/roberto/esp-idf/FreeRTOS/FreeRTOS/Source` checkout
identifies FreeRTOS Kernel V11.3.3, Git `89592801dd8b73329a7583719b7e3567b50910e5`.
Read-only study of `portable/GCC/RISC-V/portASM.S` and `portContext.h` established
the separation between common register context and chip-specific context hooks,
and the need to keep interrupts disabled during restore. The standard CLINT
port is not an ESP32-P4 CLIC implementation. No source, macros, headers,
algorithms or libraries were copied into MaiaRTOS from it.

The local ESP-IDF v5.4.2 CLIC definitions and generic vector return were then
consulted as chip references. The silicon-specific manual is **ESP32-P4 Chip
Revision v1.3 TRM, Pre-release v0.4**. The newer unqualified ESP32-P4 manual uses
different CSR definitions, so it must not silently replace the rev-v1.3 reference.
URLs, document sections/pages and downloaded SHA are in
[hardware evidence](../hardware/EVIDENCE.md).

Before the fix, the RAM test measured one IRQ, an enabled pending timer source,
`mstatus=0x88`, and **mintstatus.MIL=0x3f** while a normal task was running.
`last_mcause=0xb8000010`; the fresh task frame used `mcause=0x38000000`.
Thus another same-level IRQ could not preempt it. In the controlled experiment,
changing only the fresh frame's **INT bit 31** to one (`0xb8000000`) allowed
`mret` to restore MPIL=0, and repeated interrupts resumed. This is measured
ESP32-P4 rev-v1.3 behavior; no universal statement about other CLIC cores is made.

The production change is confined to fresh-frame initialization in
`kernel/ksched.c`. Actual saved trap contexts and assembly return code are
unchanged. The initial context is a constructed return frame, not a timer event:
IRQ/tick counters are still incremented only by the actual trap/ISR paths.

Invariants for this single-core integer test:

- A task may be selected for its first run from a timer ISR.
- Fresh frame has MPIL=0 and interrupt-return INT=1; MPP=M, MPIE=1.
- MIE remains zero during frame restore; only mret enables it.
- Timer ISR saves outgoing context, clears its source, advances one real tick
  and selects the next ready equal-priority task. No nested-ISR feature is added.

## Tests and captured results

Using standalone GCC esp-14.2.0_20241119, RV32IMAC/ILP32, esptool 4.12.0 and
`/dev/cu.usbmodem1301`. All runs load into the existing internal RAM window;
no flash-write, erase, bootloader/partition or SD change.

| Capture in `native-probe-logs/` | IRQ / ticks | Switches | workA / workB | Result |
|---|---|---|---|---|
| clic-before.log | 1 / 1 | 2 | 0 / 11961344 | FAIL |
| clic-fixed-1.log | 250 / 250 | 251 | 4443136 / 4477506 | PASS |
| clic-fixed-2.log | 250 / 250 | 251 | 4443136 / 4477525 | PASS |
| clic-fixed-3.log | 250 / 250 | 251 | 1105920 / 1113566 | PASS |
| clic-fixed-final.log | 250 / 250 | 251 | 4443136 / 4477526 | PASS |

All corrected runs have **zero ecalls and synchronous exceptions**, MIE enabled,
and MIL=0 at task completion. No cooperative yields, sleep, synthetic ticks,
manual priority reset or interrupt-level writes occur in the workload. Work
counts are iteration counts, not a benchmark; clock state after ROM entry varies.
The final probe strengthens PASS to require at least 250 switches as well as
250 IRQs/ticks, both tasks progressing, MIE=1 and MIL=0.
Final ELF/map/RAM image artifacts are preserved in the sibling directory
`../maia-rtos-recovery/native-clic-return-20261111/` with a checksum manifest.
Final RAM image SHA-256:
`bc8e768078e05895b093241528ccf05889bba412338727b75add515c394da2c6`.

- Host suite **6/6 PASS**; native artifact runtime-symbol/archive gate **PASS**.
- Kernel test under ASan/UBSan **PASS**. New frame checks cover stack alignment,
  initial task argument, INT, MPIL and MIE/MPIE state.
- Negative regression: compile the new host test with pre-fix `HEAD:kernel/ksched.c`
  (checkpoint `5b08561`) and the same `kmem.c`, `ksync.c`, `maia_shell_mvp.c`;
  it exits **52** at missing INT, as required. Captured pre-fix hardware run also
  reproduces the single-IRQ failure with the added CSR diagnostics.
- Reproduce build/RAM execution using the commands in
  [probe report](native-scheduler-probe.md). The runner resets in `finally`.

After the last RAM test, `maia.py` verified the preserved flash application's
shell echo (`NATIVE_CLIC_BASELINE_RESTORED`), SciCalc `2+3*4 = 14`, module exit
zero and return to desktop. Capture: `native-probe-logs/clic-desktop-restored.log`.
The preserved original app artifact SHA remains
`21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd`.
No claim that the current flash application contains this native-kernel fix.

## Scope, remaining risks and next small task

Hardware-tested: repeated single-core timer-driven switching of two C integer
busy tasks in internal SRAM. This is limited M1/M2 evidence; **not acceptance of
the complete context ABI, native desktop, SMP or hard real-time guarantees**.
The older report's zero-/one-IRQ captures remain valid historical evidence,
superseded only for the corrected fresh frame.

No FP context, full integer-register sentinel stress, mixed yield/delay/priority
stress, fault policy, PSRAM/cache/DMA or driver equivalence is established here.
Existing trap code forces MPIE/MPP and skips unknown faults; those behaviors
need separate review before integrating general services. Generic kernel code
still contains the existing architecture-specific frame construction; moving
that ownership is separate work. Known native linker RWX warnings remain.

Next small task: strengthen context-preservation hardware tests (live integer
registers, then FP registers/FCSR and ABI) before integrating driver services.
Keep working desktop on preserved flash until feature equivalence exists.
