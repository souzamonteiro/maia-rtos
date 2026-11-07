i32 main() { defer { asm("nop"); } unsafe { i32* p = null; } return 0; }
