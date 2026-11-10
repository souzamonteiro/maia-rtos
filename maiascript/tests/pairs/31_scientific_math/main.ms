import math;

f32 saved = 1.25;
namespace sample {
    f32 identity(f32 value) { return value; }
    f32 nested(f32 value) { return identity(value) + 0.5; }
}

i32 main() {
    if (math.fabs(sample.nested(saved) - 1.75) > 0.0001) { return 1; }
    if (math.fabs(math.sin(0.0)) > 0.0001) { return 2; }
    if (math.fabs(math.cos(0.0) - 1.0) > 0.0001) { return 3; }
    if (math.fabs(math.sqrt(9.0) - 3.0) > 0.001) { return 4; }
    if (math.fabs(math.power(2.5, 2.0) - 6.25) > 0.001) { return 5; }
    if (math.fabs(math.power(9.0, 0.5) - 3.0) > 0.001) { return 6; }
    if (math.fabs(math.ln(math.exp(1.0)) - 1.0) > 0.002) { return 7; }
    if (math.power(0.0, 2.0) != 0.0) { return 8; }
    return 42;
}
