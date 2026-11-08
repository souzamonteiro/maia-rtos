f32 compute(f32 a, f32 b) {
    f32 c = a * b + 2.5;
    return c;
}

i32 main() {
    f32 res = compute(4.0, 5.0);
    i32 truncated = res;
    return truncated + 20;
}

