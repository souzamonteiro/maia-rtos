# SMP scheduler design

## First algorithm
Fixed priorities; per-CPU ready queues; one RUNNING task per CPU; idle task per CPU. Use affinity bitmask; a task is runnable on CPU if its bit is set. Equal-priority tasks rotate on timeslice expiration. Select next task under queue lock, avoiding global critical sections where feasible.

## Invariants
1. A task cannot be RUNNING on two cores simultaneously.
2. A READY task belongs to exactly one ready queue.
3. A BLOCKED task belongs to a wait queue, not a ready queue.
4. A wakeup is atomic with wait-state transition; no lost wakeups.
5. Reschedule requests across cores use verified IPI/interrupt routing.
6. No task migration while its saved context is owned by a running core.

## Context switch
Document all caller/callee-saved registers, stack alignment, trap frame layout, interrupt enable state, privilege state, and FPU/vector extension handling (only if present and enabled). Validate against exact CPU ISA/ABI. First implement cooperative context switch, then timer preemption, then SMP.

## Locking
Spinlocks only for short nonblocking critical sections; mutexes for thread context. Interrupt masking must be per-core and correctly nestable. Define memory barriers and atomics; avoid calling scheduler while holding incompatible locks. Priority inheritance requires documented transitive behavior or explicit limitation.

## Test matrix
Single core round-robin; priorities; sleep/wakeup; tick rollover; simultaneous wakeups; lock contention; migration/affinity; repeated create/destroy; ISR-to-thread wakeup; stress with both cores; deliberate race injection. Record jitter and worst observed interrupt-disabled time (not hard bounds).
