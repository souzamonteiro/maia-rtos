extern i32 printf(string fmt, ...);
extern i32 puts(string s);
extern u32 cycles();
extern u32 millis();
extern i32* malloc(i32 size);
extern void free(i32* ptr);

namespace sys {
    i32 uptimeSeconds() {
        return millis() / 1000;
    }

    void print(string s) {
        printf("%s", s);
    }

    void println(string s) {
        printf("%s\n", s);
    }

    void printInt(i32 n) {
        printf("%d\n", n);
    }

    void printFloat(f32 val) {
        printf("%f\n", val);
    }

    u32 getCycles() {
        return cycles();
    }

    u32 getMillis() {
        return millis();
    }
}

