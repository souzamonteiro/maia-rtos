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
    Counter counter = Counter();
    counter.increment();
    counter.increment();
    return counter.getValue();
}

