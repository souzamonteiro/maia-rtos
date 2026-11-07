i32 main() { i32 s = 0; for (i32 i = 0; i < 10; i++) { if (i == 3) { continue; } s += i; if (s > 30) { break; } } return s; }
