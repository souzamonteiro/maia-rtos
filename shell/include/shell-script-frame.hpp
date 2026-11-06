#ifndef MAIA_SHELL_SCRIPT_FRAME_HPP
#define MAIA_SHELL_SCRIPT_FRAME_HPP

/* Physical input bounds only. Shell syntax belongs to the generated frontend. */
static const unsigned int kScriptBytes = 16384;
static const unsigned int kScriptLines = 256;
static int validateScriptFrame(const unsigned char* bytes, unsigned int length, unsigned int* badLine)
{
    unsigned int lines = 0;
    unsigned int column = 0;
    *badLine = 1;
    if (length > kScriptBytes) return 0;
    for (unsigned int i = 0; i < length; ++i) {
        *badLine = lines + 1;
        if (lines == kScriptLines || bytes[i] == 0) return 0;
        if (bytes[i] == 10) { ++lines; column = 0; }
        else if (++column > 255) return 0;
    }
    return 1;
}

#endif
