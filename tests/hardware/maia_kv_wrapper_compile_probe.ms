import maia.kv;
i32 main() {
    Result<i32,i32> r=kv.contains("copilot_probe");
    if(r.isError()) { return 1; }
    return 42+r.value();
}
