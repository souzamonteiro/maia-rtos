.text
main:
    flw fa0, 0(a0)
    fadd.s fa1, fa0, fa0
    amoadd.w.aq a0, a1, (a2)
    fence iorw, iorw
    ret
