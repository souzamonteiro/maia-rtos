extern i32 cycles();

i32 compute(i32 n) {
    i32 sum = 0;
    for (i32 i = 0; i < n; i++) {
        sum += i * 2;
    }
    return sum;
}

i32 main() {
    i32 t0 = cycles();
    i32 res = compute(20);
    i32 t1 = cycles();
    i32 diff = t1 - t0;
    if (diff > 0 && res == 380) {
        return 42;
    }
    return 0;
}
