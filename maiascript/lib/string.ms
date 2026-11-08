namespace strings {
    i32 len(string s) {
        if (s == 0) {
            return 0;
        }
        i32 i = 0;
        while (s[i] != 0) {
            i = i + 1;
        }
        return i;
    }

    i32 charAt(string s, i32 idx) {
        if (s == 0) {
            return 0;
        }
        return s[idx];
    }

    i32 cmp(string a, string b) {
        if (a == 0 && b == 0) {
            return 0;
        }
        if (a == 0) {
            return -1;
        }
        if (b == 0) {
            return 1;
        }
        i32 i = 0;
        while (a[i] != 0 && b[i] != 0) {
            if (a[i] < b[i]) {
                return -1;
            }
            if (a[i] > b[i]) {
                return 1;
            }
            i = i + 1;
        }
        if (a[i] == 0 && b[i] == 0) {
            return 0;
        }
        if (a[i] == 0) {
            return -1;
        }
        return 1;
    }

    i32 eq(string a, string b) {
        return cmp(a, b) == 0;
    }

    i32 indexOf(string s, i32 ch) {
        if (s == 0) {
            return -1;
        }
        i32 i = 0;
        while (s[i] != 0) {
            if (s[i] == ch) {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    i32 startsWith(string s, string prefix) {
        if (s == 0 || prefix == 0) {
            return 0;
        }
        i32 i = 0;
        while (prefix[i] != 0) {
            if (s[i] == 0 || s[i] != prefix[i]) {
                return 0;
            }
            i = i + 1;
        }
        return 1;
    }

    i32 copy(string dst, string src, i32 maxLen) {
        if (dst == 0 || src == 0 || maxLen <= 0) {
            return 0;
        }
        i32 i = 0;
        while (i < maxLen - 1 && src[i] != 0) {
            dst[i] = src[i];
            i = i + 1;
        }
        dst[i] = 0;
        return i;
    }
}

