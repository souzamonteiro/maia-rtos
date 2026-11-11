    .section .text.stack_probe, "ax"
    .option push
    .option norvc
    .global maia_probe_stack_entry
    .global maia_probe_fault_instruction
    .type maia_probe_stack_entry, @function
maia_probe_stack_entry:
#if MAIA_PROBE_STACK_KIND == 1 || MAIA_PROBE_STACK_KIND == 5 || MAIA_PROBE_STACK_KIND == 6
    mv sp, a0 /* No memory access after installing the deliberately invalid SP. */
#endif
maia_probe_fault_instruction:
    ecall
    tail maia_probe_stack_resumed
    .size maia_probe_stack_entry, .-maia_probe_stack_entry
    .option pop
