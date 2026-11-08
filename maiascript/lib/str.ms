#include "string.ms"

namespace str {
    i32 len(string s) {
        return strings.len(s);
    }

    i32 cmp(string a, string b) {
        return strings.cmp(a, b);
    }

    i32 eq(string a, string b) {
        return strings.eq(a, b);
    }

    i32 charAt(string s, i32 idx) {
        return strings.charAt(s, idx);
    }

    i32 indexOf(string s, i32 ch) {
        return strings.indexOf(s, ch);
    }
}

