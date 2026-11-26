import maia.kv;

i32 main() {
    Result<i32, i32> absent = kv.contains("theme");
    if (absent.isError() || absent.value() != 0) {
        return 1;
    }

    Result<i32, i32> saved = kv.set("theme", "dark");
    if (saved.isError()) {
        return 2;
    }

    Result<string, i32> loaded = kv.get("theme");
    if (loaded.isError()) {
        return 3;
    }
    string value = loaded.value();
    i32 matches = value[0] == 100 && value[1] == 97 &&
                  value[2] == 114 && value[3] == 107;
    kv.freeText(value);
    if (matches == 0) {
        return 4;
    }

    Result<i32, i32> present = kv.contains("theme");
    if (present.isError() || present.value() != 1) {
        return 5;
    }

    Result<i32, i32> removed = kv.remove("theme");
    if (removed.isError()) {
        return 6;
    }

    Result<string, i32> missing = kv.get("theme");
    if (missing.isOk() || missing.error() != 3) {
        return 7;
    }
    return 42;
}
