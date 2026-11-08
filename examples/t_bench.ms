extern i32 println(string s);
extern i32 print_int(i32 v);
extern i32 print(string s);
extern i32 cycles();

i32 fib(i32 n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

i32 sum_loop(i32 limit) {
    i32 s = 0;
    for (i32 i = 0; i < limit; i++) {
        s += i * 3;
    }
    return s;
}

i32 bitwise_test(i32 count) {
    i32 val = 4660; // 0x1234
    for (i32 i = 0; i < count; i++) {
        val = (val ^ (i << 2)) & 65535;
    }
    return val;
}

i32 main() {
    println("=== MAIASCRIPT RISC-V PROFILING BENCHMARK ===");
    
    // Benchmark 1: Loop & Arithmetic
    i32 t0 = cycles();
    i32 sum = sum_loop(500);
    i32 t1 = cycles();
    i32 d_loop = t1 - t0;
    print("1. Arithmetic Loop (500 iters): cycles = ");
    print_int(d_loop);

    // Benchmark 2: Recursive Fibonacci(12)
    i32 t2 = cycles();
    i32 f = fib(12);
    i32 t3 = cycles();
    i32 d_fib = t3 - t2;
    print("2. Fibonacci(12) = ");
    print_int(f);
    print("   Fibonacci cycles = ");
    print_int(d_fib);

    // Benchmark 3: Bitwise Operations
    i32 t4 = cycles();
    i32 bw = bitwise_test(500);
    i32 t5 = cycles();
    i32 d_bw = t5 - t4;
    print("3. Bitwise ops (500 iters): cycles = ");
    print_int(d_bw);

    i32 total = d_loop + d_fib + d_bw;
    print("Total Benchmark Cycles = ");
    print_int(total);
    println("===============================================");

    return 42;
}

