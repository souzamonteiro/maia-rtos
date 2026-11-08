extern i32 add_host(i32 a, i32 b);
extern i32 multiply_host(i32 x, i32 y);

i32 main() {
    i32 sum = add_host(10, 20);
    i32 prod = multiply_host(sum, 3);
    return prod + 10; // 100
}

