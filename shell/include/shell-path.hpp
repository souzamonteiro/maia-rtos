/* Literal pathname utilities after generated shell parsing and expansion.
   These byte transformations never resolve paths or access the VFS. */
static int runPathUtility(unsigned int wordCount, int basename)
{
    unsigned int first = 1;
    if (wordCount > first && textEquals(gCommandWords + first * kMaxWordBytes, "--")) ++first;
    const unsigned int operands = wordCount - first;
    if (operands < 1 || operands > (basename ? 2u : 1u)) {
        writeText(2, basename ? "usage: basename [--] path [suffix]\n" : "usage: dirname [--] path\n");
        return 0;
    }
    const char* text = gCommandWords + first * kMaxWordBytes;
    unsigned int end = textLength(text);
    while (end && text[end - 1] == '/') --end;
    if (!end) {
        if (text[0] == '/') writeText(1, "/");
        else if (!basename) writeText(1, ".");
    } else if (basename) {
        unsigned int begin = end;
        while (begin && text[begin - 1] != '/') --begin;
        if (operands == 2) {
            const char* suffix = gCommandWords + (first + 1) * kMaxWordBytes;
            const unsigned int length = textLength(suffix);
            if (length && length < end - begin) {
                int matches = 1;
                for (unsigned int i = 0; i < length; ++i)
                    if (text[end - length + i] != suffix[i]) matches = 0;
                if (matches) end -= length;
            }
        }
        writeBytes(1, text + begin, end - begin);
    } else {
        while (end && text[end - 1] != '/') --end;
        while (end && text[end - 1] == '/') --end;
        if (end) writeBytes(1, text, end);
        else writeText(1, text[0] == '/' ? "/" : ".");
    }
    writeText(1, "\n");
    return 1;
}
