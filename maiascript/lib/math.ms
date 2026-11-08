namespace math {
    i32 abs(i32 v) {
        if (v < 0) {
            return -v;
        }
        return v;
    }

    f32 fabs(f32 v) {
        if (v < 0.0) {
            return -v;
        }
        return v;
    }

    i32 min(i32 a, i32 b) {
        if (a < b) {
            return a;
        }
        return b;
    }

    i32 max(i32 a, i32 b) {
        if (a > b) {
            return a;
        }
        return b;
    }

    i32 clamp(i32 v, i32 lo, i32 hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    f32 fclamp(f32 v, f32 lo, f32 hi) {
        if (v < lo) {
            return lo;
        }
        if (v > hi) {
            return hi;
        }
        return v;
    }

    f32 sqrt(f32 x) {
        if (x <= 0.0) {
            return 0.0;
        }
        f32 guess = x / 2.0;
        for (i32 i = 0; i < 10; i++) {
            guess = (guess + x / guess) / 2.0;
        }
        return guess;
    }

    i32 pow(i32 base, i32 exp) {
        i32 res = 1;
        for (i32 i = 0; i < exp; i++) {
            res = res * base;
        }
        return res;
    }
}

