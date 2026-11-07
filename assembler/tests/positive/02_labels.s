.text
start: addi t0, zero, 1
loop:
    addi t0, t0, 1
    blt t0, a0, loop
    jal zero, done
done: ret
