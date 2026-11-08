class Tracker {
    i32 cleanups = 0;

    void cleanup() {
        cleanups += 7;
    }
}

Result<i32, i32> divide(i32 a, i32 b) {
    if (b == 0) {
        return Err(10);
    }
    return Ok(a / b);
}

Result<i32, i32> calculate(i32 a, i32 b, i32 c) {
    i32 d = divide(a, b)?;
    i32 e = divide(d, c)?;
    return Ok(e + 5);
}

Result<i32, i32> try_with_defer(Tracker* t, i32 b) {
    defer t->cleanup();
    i32 v = divide(100, b)?;
    return Ok(v);
}

i32 main() {
    i32 sum = 0;

    // Test 1: Successful chained calculation (100 / 2 = 50; 50 / 5 = 10; 10 + 5 = 15)
    Result<i32, i32> r1 = calculate(100, 2, 5);
    if (r1.isOk()) {
        sum += r1.value(); // +15 -> 15
    }

    // Test 2: Early return on first divide by zero (Err(10))
    Result<i32, i32> r2 = calculate(100, 0, 5);
    if (r2.isError()) {
        sum += r2.error(); // +10 -> 25
    }

    // Test 3: Early return on second divide by zero (Err(10))
    Result<i32, i32> r3 = calculate(100, 2, 0);
    if (r3.isError()) {
        sum += r3.error(); // +10 -> 35
    }

    // Test 4: Early return with defer execution (t->cleanup() -> cleanups += 7)
    Tracker* tracker = new Tracker();
    Result<i32, i32> r4 = try_with_defer(tracker, 0);
    if (r4.isError()) {
        sum += tracker->cleanups; // +7 -> 42
    }
    delete tracker;

    return sum; // 42
}
