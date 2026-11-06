/* Test-only guards around the same generated decoder used by length rendering. */
static int shellLengthDecodeGuard(const char* value, unsigned int length)
{
    if (length > 95) return 2;
    struct Frame {
        unsigned int before;
        unsigned int units[96];
        unsigned int middle;
        unsigned int offsets[96];
        unsigned int after;
    } frame;
    frame.before = 1234567; frame.middle = 7654321; frame.after = 2468135;
    unsigned int count = 0;
    const int accepted = MaiaShellWord_maiaCCLexDecode(value, length, frame.units, frame.offsets, &count);
    if (frame.before != 1234567 || frame.middle != 7654321 || frame.after != 2468135 || count > 95) return -1;
    return accepted;
}
