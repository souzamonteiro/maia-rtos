extern u32 millis();
extern void delay_ms(i32 x);

i32 main() {
    u32 t = millis();
    delay_ms(300);
    return millis() - t;
}
