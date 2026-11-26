extern i32 host_kv_delete_text(string key);
extern i32 host_kv_contains_text(string key);
extern string host_kv_get_text(string key);
extern i32 host_kv_last_get_error();
extern void host_kv_free_text(string value);
i32 main() {
    i32 r=host_kv_delete_text("mkvp_221758");
    if(r!=0) { return 5; }
    if(host_kv_contains_text("mkvp_221758")!=0) { return 6; }
    string value=host_kv_get_text("mkvp_221758");
    if(value!=0) { host_kv_free_text(value); return 7; }
    if(host_kv_last_get_error()!=-3) { return 8; }
    return 42;
}
