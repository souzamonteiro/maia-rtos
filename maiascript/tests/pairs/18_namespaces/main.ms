namespace mathutil {
    i32 clamp(i32 v, i32 lo, i32 hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    i32 add(i32 a, i32 b) {
        return a + b;
    }
}

i32 main() {
    i32 c = mathutil.clamp(120, 0, 100);
    i32 a = mathutil.add(10, 20);
    return c - a;
}

