/* Bounded command formatting, after generated shell parsing and expansion. */
static int appendPrintfByte(unsigned int* length, char value, int emit)
{
    if (*length >= 2048) return 0;
    if (emit && writeBytes(1, &value, 1) < 0) return 0;
    ++*length;
    return 1;
}
static int parsePrintfInteger(const char* text, int signedValue, uint32_t* magnitude, int* negative)
{
    unsigned int i = 0;
    *magnitude = 0; *negative = 0;
    if (!text[0]) return 1;
    if (text[i] == '+' || text[i] == '-') {
        *negative = text[i] == '-'; ++i;
        if (!text[i] || (*negative && !signedValue)) return 0;
    }
    unsigned int base = 10;
    if (text[i] == '0') {
        base = 8;
        if (text[i + 1] == 'x' || text[i + 1] == 'X') { base = 16; i += 2; }
    }
    if (!text[i]) return 0;
    const uint32_t limit = signedValue ? (*negative ? 2147483648u : 2147483647u) : 4294967295u;
    for (; text[i]; ++i) {
        unsigned int digit = 16;
        if (text[i] >= '0' && text[i] <= '9') digit = text[i] - '0';
        else if (text[i] >= 'a' && text[i] <= 'f') digit = text[i] - 'a' + 10;
        else if (text[i] >= 'A' && text[i] <= 'F') digit = text[i] - 'A' + 10;
        if (digit >= base || *magnitude > (limit - digit) / base) return 0;
        *magnitude = *magnitude * base + digit;
    }
    return 1;
}
static int appendPrintfPadding(unsigned int count, char byte, unsigned int* length, int emit)
{
    for (unsigned int i = 0; i < count; ++i)
        if (!appendPrintfByte(length, byte, emit)) return 0;
    return 1;
}
static int appendPrintfInteger(const char* text, char conversion, unsigned int width,
    unsigned int precision, int hasPrecision, int left, int zero,
    unsigned int* length, int emit)
{
    uint32_t magnitude = 0; int negative = 0;
    const int signedValue = conversion == 'd' || conversion == 'i';
    if (!parsePrintfInteger(text, signedValue, &magnitude, &negative)) return -2;
    const int sign = negative && magnitude != 0;
    const unsigned int base = conversion == 'o' ? 8 : (conversion == 'x' || conversion == 'X' ? 16 : 10);
    const char* digits = conversion == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
    char reversed[11]; unsigned int count = 0;
    if (magnitude || !hasPrecision || precision) {
        do {
            reversed[count++] = digits[magnitude % base];
            magnitude /= base;
        } while (magnitude);
    }
    unsigned int zeros = precision > count ? precision - count : 0;
    const unsigned int used = count + zeros + (sign ? 1 : 0);
    const unsigned int padding = width > used ? width - used : 0;
    if (!left && !(zero && !hasPrecision)
        && !appendPrintfPadding(padding, ' ', length, emit)) return -4;
    if (sign && !appendPrintfByte(length, '-', emit)) return -4;
    if (!left && zero && !hasPrecision) zeros += padding;
    if (!appendPrintfPadding(zeros, '0', length, emit)) return -4;
    while (count) if (!appendPrintfByte(length, reversed[--count], emit)) return -4;
    if (left && !appendPrintfPadding(padding, ' ', length, emit)) return -4;
    return 1;
}
// Format operands are already generated/expanded words. This scans only the
// standalone utility's field specification, never shell syntax or source.
static int parsePrintfFieldNumber(const char* format, unsigned int* index, unsigned int* value)
{
    *value = 0;
    while (format[*index] >= '0' && format[*index] <= '9') {
        const unsigned int digit = static_cast<unsigned char>(format[*index]) - '0';
        if (*value > (2048u - digit) / 10u) return 0;
        *value = *value * 10u + digit;
        ++*index;
    }
    return 1;
}
static int appendPrintfEscaped(const char* text, unsigned int* length, int emit)
{
    for (unsigned int i = 0; text[i]; ++i) {
        unsigned int value = static_cast<unsigned char>(text[i]);
        if (value == 92) {
            value = static_cast<unsigned char>(text[++i]);
            if (value == 'c') return 2;
            if (value == '0') {
                value = 0;
                for (unsigned int digits = 0; digits < 3 && text[i + 1] >= '0' && text[i + 1] <= '7'; ++digits)
                    value = value * 8 + static_cast<unsigned char>(text[++i]) - '0';
                if (value > 255) return 0;
            } else if (value == 'a') value = 7;
            else if (value == 'b') value = 8;
            else if (value == 'f') value = 12;
            else if (value == 'n') value = 10;
            else if (value == 'r') value = 13;
            else if (value == 't') value = 9;
            else if (value == 'v') value = 11;
            else if (value != 92) return 0;
        }
        if (!appendPrintfByte(length, static_cast<char>(value), emit)) return -4;
    }
    return 1;
}
static int formatPrintf(const char* format, unsigned int firstArgument,
    unsigned int wordCount, unsigned int* length, int emit)
{
    unsigned int argument = firstArgument;
    *length = 0;
    do {
        const unsigned int before = argument;
        for (unsigned int i = 0; format[i]; ++i) {
            unsigned int value = static_cast<unsigned char>(format[i]);
            if (value == '%') {
                ++i;
                const unsigned int fieldStart = i;
                int left = 0; int zero = 0;
                while (format[i] == '-' || format[i] == '0') {
                    if (format[i] == '-') left = 1;
                    else zero = 1;
                    ++i;
                }
                unsigned int width = 0; unsigned int precision = 0;
                if (!parsePrintfFieldNumber(format, &i, &width)) return -4;
                const int hasPrecision = format[i] == '.';
                if (hasPrecision) {
                    ++i;
                    if (!parsePrintfFieldNumber(format, &i, &precision)) return -4;
                }
                const int modified = i != fieldStart;
                if (format[i] == 's') {
                    if (zero) return 0;
                    const char* text = argument < wordCount ? gCommandWords + argument++ * kMaxWordBytes : "";
                    unsigned int count = 0;
                    while (text[count] && (!hasPrecision || count < precision)) ++count;
                    const unsigned int padding = width > count ? width - count : 0;
                    if (!left && !appendPrintfPadding(padding, ' ', length, emit)) return -4;
                    for (unsigned int j = 0; j < count; ++j)
                        if (!appendPrintfByte(length, text[j], emit)) return -4;
                    if (left && !appendPrintfPadding(padding, ' ', length, emit)) return -4;
                    continue;
                }
                if (format[i] == 'b' || format[i] == 'c') {
                    if (modified) return 0;
                    const char* text = argument < wordCount ? gCommandWords + argument++ * kMaxWordBytes : "";
                    if (format[i] == 'c') {
                        if (text[0] && !appendPrintfByte(length, text[0], emit)) return -4;
                    } else {
                        const int status = appendPrintfEscaped(text, length, emit);
                        if (status == 2) return 1;
                        if (status != 1) return status;
                    }
                    continue;
                }
                if (format[i] == 'd' || format[i] == 'i' || format[i] == 'u'
                    || format[i] == 'o' || format[i] == 'x' || format[i] == 'X') {
                    const char* text = argument < wordCount ? gCommandWords + argument++ * kMaxWordBytes : "";
                    const int status = appendPrintfInteger(text, format[i], width, precision,
                        hasPrecision, left, zero, length, emit);
                    if (status != 1) return status;
                    continue;
                }
                if (format[i] != '%' || modified) return 0;
            } else if (value == 92) {
                ++i;
                value = static_cast<unsigned char>(format[i]);
                if (value >= '0' && value <= '7') {
                    value -= '0';
                    for (unsigned int digits = 1; digits < 3 && format[i + 1] >= '0' && format[i + 1] <= '7'; ++digits)
                        value = value * 8 + static_cast<unsigned char>(format[++i]) - '0';
                    if (value > 255) return 0;
                } else if (value == 'a') value = 7;
                else if (value == 'b') value = 8;
                else if (value == 'f') value = 12;
                else if (value == 'n') value = 10;
                else if (value == 'r') value = 13;
                else if (value == 't') value = 9;
                else if (value == 'v') value = 11;
                else if (value != 92) return 0;
            }
            if (!appendPrintfByte(length, static_cast<char>(value), emit)) return -4;
        }
        // A literal-only format, or a pass with no remaining data, runs once.
        if (argument == before) break;
    } while (argument < wordCount);
    return 1;
}
static int runPrintf(unsigned int wordCount)
{
    unsigned int format = 1;
    if (wordCount > 1 && textEquals(gCommandWords + kMaxWordBytes, "--")) ++format;
    if (format >= wordCount) { writeText(2, "usage: printf [--] FORMAT [ARG ...]\n"); return 0; }
    unsigned int length = 0;
    const char* text = gCommandWords + format * kMaxWordBytes;
    const int status = formatPrintf(text, format + 1, wordCount, &length, 0);
    if (status == 0) writeText(2, "printf: unsupported or invalid format\n");
    else if (status == -2) writeText(2, "printf: invalid or out-of-range integer\n");
    else if (status < 0 || length > 4096 - gPendingLength) writeText(2, "printf: output limit exceeded\n");
    // Validated immutable words and capacity make the second pass infallible.
    // Reuse the output queue rather than allocating another result buffer.
    else { formatPrintf(text, format + 1, wordCount, &length, 1); return 1; }
    return 0;
}
