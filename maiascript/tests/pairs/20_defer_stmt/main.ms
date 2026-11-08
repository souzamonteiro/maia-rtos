extern i32 printf(string fmt, ...);

class Resource {
    i32 id = 100;
    i32 active = 1;

    void cleanup() {
        active = 0;
    }

    i32 isActive() const {
        return active;
    }
}

i32 compute() {
    Resource* r = new Resource();
    defer delete r;

    i32 val = r->id;
    return val + 5;
}

i32 testOrder() {
    return compute();
}

i32 main() {
    i32 res = testOrder();
    printf("Test 20 defer executed: compute=%d\n", res);
    return res - 63;
}

