class Counter {
    private i32 value = 0;

    void increment() {
        value += 1;
    }

    i32 getValue() const {
        return value;
    }
}

i32 main() {
    i32 sz = sizeof(Counter); // 4 bytes
    Counter* c = new Counter();
    c->increment();
    c->increment();
    c->increment();
    i32 val = c->getValue(); // 3
    delete c;
    return sz * 10 + val; // 4 * 10 + 3 = 43
}

