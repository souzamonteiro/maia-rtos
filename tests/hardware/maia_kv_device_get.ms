extern string host_kv_get_text(string key);
extern void host_kv_free_text(string value);
i32 main() {
    string value=host_kv_get_text("mkvp_221758");
    if(value==0) { return 1; }
    i32 matches=value[0]==111 && value[1]==107;
    host_kv_free_text(value);
    if(matches==0) { return 2; }
    return 42;
}
