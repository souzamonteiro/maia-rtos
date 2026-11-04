# Boot, traps and interrupt architecture

## Sequence (design)
ROM -> verified image loader / MaiaBoot -> initial stack and sections -> clocks and memory -> trap vector -> UART -> timer -> kernel init -> secondary-core bring-up -> SMP scheduler -> application.

## Required hardware research before implementation
ROM boot format; flash image header; memory map; reset vector; privileged modes; interrupt controller; timer source; IPI mechanism; cache and PSRAM initialization; second-core release; watchdog behavior. Record primary documentation and revision in hardware/EVIDENCE.md.

## Trap handler contract
Capture cause, EPC, status, fault address where applicable, core ID, stack pointer; distinguish interrupt vs exception; avoid unsafe recursive logging; provide panic path. Verify ABI and assembler directives with toolchain.

## Stage gates
M0 UART proof; M1 trap and timer; M2 one-core preemption; M3 secondary-core startup and SMP. Do not combine all four in one change.
