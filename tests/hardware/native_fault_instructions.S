/* Inject at a halfword PC, preserving the exact fault address in the report. */
    .section .text.fault_probe, "ax"
    .option push
    .option norvc
    .balign 4
    .global maia_probe_fault_entry
    .global maia_probe_fault_instruction
    .type maia_probe_fault_entry, @function
maia_probe_fault_entry:
    .2byte 0x0001 /* C.NOP makes the fault PC only halfword aligned. */
maia_probe_fault_instruction:
#if MAIA_PROBE_FAULT_KIND == 1
    .2byte 0x0000 /* Defined illegal compressed instruction. */
    .2byte 0x0001 /* Old PC+4 skipping incorrectly resumes after this NOP. */
#elif MAIA_PROBE_FAULT_KIND == 2
    .4byte 0xc0001073 /* CSRRW x0,cycle,x0: write to read-only CSR. */
#elif MAIA_PROBE_FAULT_KIND == 3
    .2byte 0x9002 /* C.EBREAK. */
    .2byte 0x0001
#else
#error "Select a defined fault kind"
#endif
    tail maia_probe_fault_resumed
    .size maia_probe_fault_entry, .-maia_probe_fault_entry
    .option pop
