#ifndef MAIA_SHELL_SCRIPT_UNIT_HPP
#define MAIA_SHELL_SCRIPT_UNIT_HPP
/* Physical framing only. Generated Input acceptance decides completion. */
struct ShellScriptUnit {
    unsigned int length, lines, startLine, collecting;
};
static void clearScriptUnit(ShellScriptUnit* unit)
{
    unit->length = 0; unit->lines = 0; unit->startLine = 0; unit->collecting = 0;
}
/* 0 snapshot end,1 next candidate,2 corrupt framing/state,3 bound exceeded.
   Check the entire append before touching the shared256-byte command buffer. */
static int appendScriptPhysicalLine(const unsigned char* bytes, unsigned int size,
    unsigned int* offset, unsigned int* physicalLine, char* output, ShellScriptUnit* unit)
{
    if (size > kScriptBytes || *offset > size || *physicalLine > kScriptLines
        || unit->collecting > 1 || unit->length > 255 || unit->lines > 8) return 2;
    if (unit->collecting && (!unit->lines || !unit->startLine
        || unit->startLine > *physicalLine
        || unit->lines - 1 != *physicalLine - unit->startLine)) return 2;
    if (*offset == size) return 0;
    if (!unit->collecting) {
        clearScriptUnit(unit); unit->startLine = *physicalLine + 1;
    }
    if (unit->lines == 8 || *physicalLine == kScriptLines) return 3;
    unsigned int end = *offset;
    while (end < size && bytes[end] != 10) {
        if (!bytes[end]) return 2;
        ++end;
    }
    const unsigned int separator = unit->collecting ? 1 : 0;
    const unsigned int count = end - *offset;
    if (count > 255 || unit->length + separator + count > 255) return 3;
    if (separator) output[unit->length++] = 10;
    for (unsigned int i = *offset; i < end; ++i)
        output[unit->length++] = static_cast<char>(bytes[i]);
    output[unit->length] = 0;
    *offset = end < size ? end + 1 : end;
    ++*physicalLine; ++unit->lines; unit->collecting = 1;
    return 1;
}
#endif
