#pragma once

// Available only in the native Tab5 product profile, not the ESP-IDF compatibility app.
extern i32 host_kv_set_text(string key, string value);
extern string host_kv_get_text(string key);
extern i32 host_kv_last_get_error();
extern void host_kv_free_text(string value);
extern i32 host_kv_delete_text(string key);
extern i32 host_kv_contains_text(string key);

namespace kv {
    Result<i32, i32> set(string key, string value) {
        i32 result = host_kv_set_text(key, value);
        if (result < 0) {
            return Err(-result);
        }
        return Ok(0);
    }

    Result<string, i32> get(string key) {
        string value = host_kv_get_text(key);
        if (value == 0) {
            return Err(-host_kv_last_get_error());
        }
        return Ok(value);
    }

    Result<i32, i32> remove(string key) {
        i32 result = host_kv_delete_text(key);
        if (result < 0) {
            return Err(-result);
        }
        return Ok(0);
    }

    Result<i32, i32> contains(string key) {
        i32 result = host_kv_contains_text(key);
        if (result < 0) {
            return Err(-result);
        }
        return Ok(result);
    }

    void freeText(string value) {
        host_kv_free_text(value);
    }
}
