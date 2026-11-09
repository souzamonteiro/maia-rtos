#pragma once

extern i32 host_fs_write_text(string path, string content);
extern string host_fs_read_text(string path);
extern i32 host_fs_remove(string path);
extern i32 host_fs_exists(string path);
extern i32 host_fs_list_count(string path);
extern string host_fs_list_item_name(string path, i32 idx);
extern i32 host_fs_list_item_is_dir(string path, i32 idx);

namespace fs {
    struct Entry {
        string name;
        i32 isDirectory;
    }

    Result<i32, i32> writeText(string path, string content) {
        i32 res = host_fs_write_text(path, content);
        if (res < 0) {
            return Err(-res);
        }
        return Ok(res);
    }

    Result<string, i32> readText(string path) {
        i32 ex = host_fs_exists(path);
        if (ex == 0) {
            return Err(1);
        }
        string content = host_fs_read_text(path);
        return Ok(content);
    }

    Result<i32, i32> remove(string path) {
        i32 res = host_fs_remove(path);
        if (res < 0) {
            return Err(-res);
        }
        return Ok(0);
    }

    i32 exists(string path) {
        return host_fs_exists(path);
    }
}

