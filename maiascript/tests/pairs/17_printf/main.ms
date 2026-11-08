extern i32 printf(string fmt);

i32 main() {
    printf("Format test: int=%d, hex=%x, char=%c, str=%s\n", 42, 255, 65, "PASS");
    return 42;
}

