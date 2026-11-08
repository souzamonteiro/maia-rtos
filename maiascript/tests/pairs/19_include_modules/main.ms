import math;
import sys;
import str;
#include "helper.ms"

i32 main() {
    i32 p = math.pow(2, 3);
    i32 c = math.clamp(150, 0, 100);
    i32 slen = str.len("hello");
    i32 h = helper.addSeven(10);

    sys.print("Test 19: modules and standard library working!\n");

    return p + c - slen - h;
}

