i32 sum(i32* ptr, i32 n) {
    i32 s = 0;
    for (i32 i = 0; i < n; i++) {
        s += ptr[i];
    }
    return s;
}

i32 main() {
    i32[5] arr;
    for (i32 i = 0; i < 5; i++) {
        arr[i] = (i + 1) * 10;
    }
    return sum(arr, 5);
}

