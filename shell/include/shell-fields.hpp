/* Bounded field boundaries over rendered data, never over shell source.
   Split eligibility and zero-length quoted anchors come from the generated AST. */
struct ShellWordFields {
    unsigned int start[12];
    unsigned int length[12];
    unsigned int count;
};
static int shellFieldDelimiter(unsigned char byte)
{
    return byte == 32 || byte == 9 || byte == 10;
}
static int splitShellWord(const char* text, unsigned int length,
    const unsigned char* eligible, const unsigned char* anchors, ShellWordFields* fields)
{
    fields->count = 0;
    if (length > 95) return 3;
    unsigned int active = 0, start = 0;
    for (unsigned int i = 0; i <= length; ++i) {
        if (anchors[i] && !active) { start = i; active = 1; }
        const int boundary = i == length || (eligible[i]
            && shellFieldDelimiter(static_cast<unsigned char>(text[i])));
        if (boundary) {
            if (active) {
                if (fields->count == 12) return 3;
                fields->start[fields->count] = start;
                fields->length[fields->count++] = i - start;
                active = 0;
            }
        } else if (!active) { start = i; active = 1; }
    }
    return 1;
}
