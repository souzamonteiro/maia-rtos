extern i32 host_kv_contains_text(string key);
extern i32 host_kv_set_text(string key, string value);
i32 main() {
    i32 prior=host_kv_contains_text("mkvp_221758");
    if(prior!=0) { return 2; }
    return host_kv_set_text("mkvp_221758","ok");
}
