/* Minimal owner-compiler regression: no shell, imports or runtime library. */
unsigned int ordinary_divide(unsigned int value, unsigned int divisor)
{
    return value / divisor;
}
unsigned int compound_divide(unsigned int value, unsigned int divisor)
{
    value /= divisor;
    return value;
}
unsigned int compound_remainder(unsigned int value, unsigned int divisor)
{
    value %= divisor;
    return value;
}
unsigned int compound_shift(unsigned int value, unsigned int bits)
{
    value >>= bits;
    return value;
}
