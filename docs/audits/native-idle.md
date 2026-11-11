# Private idle and no-ready behavior — 2026-11-11

The scheduler no longer revives a sleeping/blocked/exited application when no
application is READY. A private static idle context owns a 2048-byte stack,
consumes no public task slot, starts with MIE enabled/FS Off and waits with WFI.
Its saved sp is retained across interrupts. Ready applications always outrank
idle, including priority 0 applications. `current_id=-1` denotes no active app.

Idle tick/entry diagnostics are coherent single-core snapshots. User slot/count
semantics stay unchanged: 16 application slots, occupied count includes EXITED,
private idle excluded. Empty scheduler startup now starts idle instead of returning.
No heap, vendor code/runtime, parser or flash change. WFI may return spuriously;
there is no power-saving or hard timing claim.

## Validation

Host suite **11/11 PASS**; atomicity/idle/integer/FP fixtures under ASan/UBSan
**4/4 PASS**. Tests cover empty selection, all sleeping, two different deadlines,
higher-priority wakeup, all exited, unchanged terminal states, idle counts and
invalid attempts to suspend/exit the internal idle. An isolated project-source
mutant reinstating task revival is rejected at line 26 of the host idle fixture.
No pre-fix hardware timing reproduction is claimed for this unit.

Hardware: ESP32-P4 rev v1.3, GCC esp-14.2.0_20241119, RV32IMAFC/ILP32F,
esptool4.12; RAM-only images. The diagnostic overrides the architecture wait
hook to observe a short coherent snapshot before the same WFI instruction.
It does not change scheduling decisions or inject ticks/wakes. This is an
instrumented port build, not evidence of measured idle power consumption.

- Sleep/exit workload: two tasks sleep 3/5 ticks for 20 cycles then return. PASS:
  150 real IRQs, 149 ticks charged to idle, 37 idle entries, 20 completed waits per
  task, 2 normal returns, 60 all-sleeping observations, 50 all-exited observations,
  42 ecalls and0 errors. Both FP tasks still execute a real float-ABI sum.
- Empty boot: PASS, 150 real IRQs/idle ticks, 1 idle entry, 0 apps/returns/ecalls and
  0 errors. Idle MIE enabled and FS off are checked directly in both variants.
- Lifecycle/mixed workload is rebuilt and rerun to check that internal idle
  does not change the verified live-register/create/exit/priority behavior.
- Native artifact runtime-symbol/archive gate PASS; linker RWX/empty stripped
  flash-segment warnings remain. No native desktop/SMP or stack-protection claim.

Captured logs: `native-probe-logs/idle-sleep.log`, `idle-empty.log` and
`idle-lifecycle-regression.log`. No diagnostic is flashed; runner resets to
preserved flash after every test.

## Reproduce

```sh
cmake -S . -B /tmp/maia-native-idle-sleep -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_IDLE=ON -DMAIA_NATIVE_PROBE_FP32=ON
cmake --build /tmp/maia-native-idle-sleep --target maia_sched_probe_ram -j4
```

For the empty variant use another fresh directory and add
`-DMAIA_NATIVE_PROBE_EMPTY=ON`. Use established elf2image/RAM runner procedure.
Production files: scheduler, public idle snapshot contract and private idle port
hook. CMake/host/hardware workloads and evidence/status/handoff are supporting
changes. Current idle stack is a fixed diagnostic size, not a proven stack bound.

Next in the owner's authorized order: stack bounds/guards and fault reporting
before driver services; SMP remains separate. Neither idle nor local IRQ masking
establishes multiprocessor correctness.

Lifecycle regression PASS after idle: 250 IRQ/ticks, 125 creates/returns/critical
checks, 4 registered slots, 67229 ecalls, reg_errors0/MIL0.
Artifacts/checksums: `../maia-rtos-recovery/native-idle-20261111/`.

After the final RAM reset, maia.py verified preserved flash shell/SciCalc
2+3*4=14, module exit0 and desktop active. Capture `idle-desktop-restored.log`.
No flash/SD write. Stack guards remain the next ordered implementation.
