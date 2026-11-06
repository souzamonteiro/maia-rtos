#include "shell-variables.hpp"
#include "shell-fields.hpp"
/* Semantic rendering of generated word-part nodes. No source tokenization. */
static MaiaShellWord_MaiaCCParserToken gShellWordTokens[256];
static MaiaShellParseTreeNode gShellWordNodes[2048];
static MaiaShellParseTreeCollector gShellWordTree;
/* Metadata for the most recently rendered word, including its final boundary. */
static unsigned char gShellWordSplit[96], gShellWordAnchors[96];

static int parseShellWord(const char* input, unsigned int length)
{
    if (length > 255) return 3;
    MaiaShellWord_MaiaCCLexerState lexer;
    MaiaShellWord_maiaCCLexerStateInit(&lexer, input, length);
    if (!MaiaShellWord_maiaCCLexerTokenize(&lexer, gShellWordTokens, 256))
        return lexer.failed == 1 ? 2 : 3;
    maiaShellParseTreeInit(&gShellWordTree, gShellWordNodes, 2048);
    MaiaShellWord_MaiaCCParseEvents events;
    events.context = &gShellWordTree;
    events.checkpoint = maiaShellParseTreeCheckpoint;
    events.restore = maiaShellParseTreeRestore;
    events.startNonterminal = maiaShellParseTreeStart;
    events.terminal = maiaShellParseTreeTerminal;
    events.endNonterminal = maiaShellParseTreeEnd;
    events.abortNonterminal = maiaShellParseTreeAbort;
    events.matchesContextual = 0;
    MaiaShellWord_MaiaCCParserState parser;
    MaiaShellWord_maiaCCParserStateInit(&parser, gShellWordTokens, lexer.tokenCount, &events);
    const int accepted = MaiaShellWord_maiaCC_parse(&parser);
    if (gShellWordTree.overflow) return 3;
    return accepted && !parser.failed && parser.position == lexer.tokenCount
        && gShellWordTree.activeNode < 0 ? 1 : 2;
}

static unsigned int shellWordProtected(unsigned int index)
{
    const MaiaShellParseTreeNode* nodes = gShellWordNodes;
    int parent = static_cast<int>(index);
    while (parent >= 0) {
        const char* name = nodes[parent].name;
        if (textEquals(name, "SingleQuoted") || textEquals(name, "DoubleQuoted")
            || textEquals(name, "FallbackQuoted") || textEquals(name, "Escape") || textEquals(name, "DoubleEscape")) return 1;
        parent = nodes[parent].parent;
    }
    return 0;
}

/* Provenance follows generated fallback ownership, not source characters. */
static int shellWordInFallback(unsigned int index)
{
    int parent = static_cast<int>(index);
    while (parent >= 0) {
        const char* name = gShellWordNodes[parent].name;
        if (textEquals(name, "DefaultValue") || textEquals(name, "DoubleDefaultValue")) return 1;
        parent = gShellWordNodes[parent].parent;
    }
    return 0;
}
static unsigned int shellWordSubtreeEnd(unsigned int root)
{
    unsigned int end = root + 1;
    while (end < gShellWordTree.nodeCount && gShellWordNodes[end].parent >= static_cast<int>(root)) ++end;
    return end;
}

static int appendShellWord(const char* bytes, unsigned int count, unsigned int protection,
    char* output, unsigned char* masks, unsigned int capacity, unsigned int* length)
{
    if (*length >= capacity || count >= capacity - *length) return 0;
    for (unsigned int i = 0; i < count; ++i) {
        output[*length] = bytes[i]; masks[*length] = static_cast<unsigned char>(protection);
        ++*length;
    }
    return 1;
}

static int readShellWordName(unsigned int root, char* output)
{
    unsigned int length = 0;
    for (unsigned int i = root + 1; i < gShellWordTree.nodeCount
        && gShellWordNodes[i].parent >= static_cast<int>(root); ++i) {
        const MaiaShellParseTreeNode* node = gShellWordNodes + i;
        if (node->kind != kShellParseTreeTerminal) continue;
        if (node->length > 31 - length) return 0;
        for (unsigned int j = 0; j < node->length; ++j) output[length++] = node->value[j];
    }
    output[length] = 0;
    return length != 0;
}
/* Reuse generated UTF-8 validation/UTF-16 transport on stored value data.
   Distinct byte offsets count scalars without treating surrogate halves twice.
   Independent scratch preserves the active generated word tree and token spans. */
static int shellParameterLength(const char* value, unsigned int* result)
{
    unsigned int length = 0;
    while (length < 96 && value[length]) ++length;
    if (length > 95) return 3;
    unsigned int units[96], offsets[96], count = 0;
    if (!MaiaShellWord_maiaCCLexDecode(value, length, units, offsets, &count)) return 4;
    unsigned int scalars = 0;
    for (unsigned int i = 0; i < count; ++i)
        if (!i || offsets[i] != offsets[i - 1]) ++scalars;
    *result = scalars;
    return 1;
}
/* Pattern protection stops at PatternValue. An outer DoubleQuoted protects the
   retained result, while only quotes/escapes inside the pattern quote its syntax. */
static unsigned int shellPatternProtected(unsigned int index, unsigned int root)
{
    int parent = static_cast<int>(index);
    while (parent > static_cast<int>(root)) {
        const char* name = gShellWordNodes[parent].name;
        if (textEquals(name, "SingleQuoted") || textEquals(name, "FallbackQuoted")
            || textEquals(name, "Escape") || textEquals(name, "DoubleEscape")) return 1;
        parent = gShellWordNodes[parent].parent;
    }
    return 0;
}
static int renderShellPattern(unsigned int root, unsigned int status, int expand,
    char* output, unsigned char* masks, unsigned int* length)
{
    *length = 0;
    const unsigned int end = shellWordSubtreeEnd(root);
    for (unsigned int i = root + 1; i < end; ++i) {
        const MaiaShellParseTreeNode* node = gShellWordNodes + i;
        const unsigned int protection = shellPatternProtected(i, root);
        if (node->kind == kShellParseTreeNonterminal
            && (textEquals(node->name, "NamedParameter") || textEquals(node->name, "BracedNamedParameter")
                || textEquals(node->name, "ParameterLength"))) {
            unsigned int nameRoot = i + 1;
            const unsigned int next = shellWordSubtreeEnd(i);
            while (nameRoot < next && !textEquals(gShellWordNodes[nameRoot].name, "VariableName")) ++nameRoot;
            char name[32];
            if (nameRoot == next || !readShellWordName(nameRoot, name)) return 3;
            if (expand) {
                unsigned int size = 0; while (name[size]) ++size;
                const char* value = shellVariableValue(name, size);
                char digits[3]; unsigned int count = 0;
                if (textEquals(node->name, "ParameterLength")) {
                    unsigned int scalars = 0;
                    const int result = shellParameterLength(value, &scalars);
                    if (result != 1) return result;
                    if (scalars >= 10) digits[count++] = static_cast<char>('0' + scalars / 10);
                    digits[count++] = static_cast<char>('0' + scalars % 10);
                    value = digits;
                } else while (value[count]) ++count;
                if (!appendShellWord(value, count, protection, output, masks, 96, length)) return 3;
            }
            i = next - 1; continue;
        }
        if (node->kind == kShellParseTreeNonterminal
            && (textEquals(node->name, "StatusParameter") || textEquals(node->name, "BracedStatusParameter"))) {
            if (expand) {
                char digits[10]; unsigned int count = 0, value = status;
                do { digits[count++] = static_cast<char>('0' + value % 10); value /= 10; } while (value);
                while (count) if (!appendShellWord(digits + --count, 1, protection, output, masks, 96, length)) return 3;
            } else if (!appendShellWord("$?", 2, protection, output, masks, 96, length)) return 3;
            i = shellWordSubtreeEnd(i) - 1; continue;
        }
        if (node->kind != kShellParseTreeTerminal || node->token == kShellWordEof) continue;
        const char* owner = node->parent < 0 ? "" : gShellWordNodes[node->parent].name;
        if (textEquals(owner, "SingleQuoted") || textEquals(owner, "FallbackQuoted")
            || textEquals(owner, "EscapeMarker")) continue;
        if (!appendShellWord(node->value, node->length, protection, output, masks, 96, length)) return 3;
    }
    output[*length] = 0;
    return 1;
}
static int shellRemovalValue(const char* subject, const char* pattern,
    unsigned int patternLength, const unsigned char* masks, int longest, int prefix,
    unsigned int budget, unsigned int* retained, unsigned int* used)
{
    unsigned int length = 0;
    while (length < 96 && subject[length]) ++length;
    if (length > 95) return 3;
    unsigned int units[96], decodedOffsets[96], count = 0, offsets[96];
    if (!MaiaShellWord_maiaCCLexDecode(subject, length, units, decodedOffsets, &count)) return 4;
    unsigned int boundaries = 1; offsets[0] = 0;
    for (unsigned int i = 0; i < count; ++i)
        if (decodedOffsets[i] != offsets[boundaries - 1]) offsets[boundaries++] = decodedOffsets[i];
    if (length != offsets[boundaries - 1]) offsets[boundaries++] = length;
    unsigned int patternScalars = 0;
    const int valid = shellParameterLength(pattern, &patternScalars);
    if (valid != 1) return valid;
    return matchShellRemoval(subject, length, offsets, boundaries, pattern,
        patternLength, masks, longest, prefix, budget, retained, used);
}
/* Keep the existing suffix probe interface and its exact-budget regressions. */
static int shellSuffixValue(const char* subject, const char* pattern,
    unsigned int patternLength, const unsigned char* masks, int longest,
    unsigned int budget, unsigned int* retained, unsigned int* used)
{
    return shellRemovalValue(subject, pattern, patternLength, masks, longest, 0,
        budget, retained, used);
}
static int renderShellWordMode(const char* input, unsigned int inputLength,
    unsigned int status, int expandStatus, char* output, unsigned char* masks, unsigned int capacity,
    int assignment, char* assignmentName)
{
    if (capacity > 96 || !capacity) return 3;
    const int parsed = parseShellWord(input, inputLength);
    if (parsed != 1) return parsed;
    for (unsigned int byte = 0; byte < 96; ++byte) {
        gShellWordSplit[byte] = 0; gShellWordAnchors[byte] = 0;
    }
    const MaiaShellParseTreeNode* nodes = gShellWordNodes;
    // BareName is also the ordinary literal-word grammar alternative: its
    // 95-byte word bound applies, rather than the 31-byte semantic-name bound.
    for (unsigned int i = 0; i < gShellWordTree.nodeCount; ++i) {
        if (nodes[i].kind == kShellParseTreeNonterminal && textEquals(nodes[i].name, "VariableName")
            && nodes[i].parent >= 0 && !textEquals(nodes[nodes[i].parent].name, "BareName")) {
            char name[32];
            if (!readShellWordName(i, name)) return 3;
        }
    }
    unsigned int length = 0;
    unsigned int valueRoot = 0;
    if (assignment) {
        unsigned int found = 0;
        for (unsigned int i = 0; i < gShellWordTree.nodeCount; ++i) {
            const MaiaShellParseTreeNode* node = nodes + i;
            if (node->kind == kShellParseTreeNonterminal && textEquals(node->name, "AssignmentValue")) valueRoot = i;
            if (node->kind == kShellParseTreeNonterminal && textEquals(node->name, "VariableName") && !valueRoot && !found) {
                if (!readShellWordName(i, assignmentName)) return 3;
                found = 1;
            }
        }
        if (!found || !valueRoot) return 2;
    }
    for (unsigned int i = assignment ? valueRoot + 1 : 0; i < gShellWordTree.nodeCount; ++i) {
        const MaiaShellParseTreeNode* node = nodes + i;
        if (assignment && nodes[i].parent < static_cast<int>(valueRoot)) break;
        if (node->kind == kShellParseTreeNonterminal
            && (textEquals(node->name, "SuffixParameter") || textEquals(node->name, "PrefixParameter"))) {
            const int prefix = textEquals(node->name, "PrefixParameter");
            const unsigned int end = shellWordSubtreeEnd(i);
            unsigned int nameRoot = i + 1, patternRoot = i + 1;
            int longest = 0;
            while (nameRoot < end && !textEquals(nodes[nameRoot].name, "VariableName")) ++nameRoot;
            while (patternRoot < end && !textEquals(nodes[patternRoot].name, "PatternValue")) {
                if (textEquals(nodes[patternRoot].name, "LongestSuffixOperator")
                    || textEquals(nodes[patternRoot].name, "LongestPrefixOperator")) longest = 1;
                ++patternRoot;
            }
            char name[32], pattern[96]; unsigned char patternMasks[96]; unsigned int patternLength = 0;
            if (nameRoot == end || patternRoot == end || !readShellWordName(nameRoot, name)) return 2;
            const int rendered = renderShellPattern(patternRoot, status, expandStatus,
                pattern, patternMasks, &patternLength);
            if (rendered != 1) return rendered;
            if (expandStatus) {
                unsigned int size = 0; while (name[size]) ++size;
                const char* subject = shellVariableValue(name, size);
                unsigned int retained = 0, used = 0;
                const int matched = shellRemovalValue(subject, pattern, patternLength,
                    patternMasks, longest, prefix, 1048576, &retained, &used);
                if (matched != 1) return matched;
                const unsigned int start = length, protection = shellWordProtected(i);
                unsigned int subjectLength = 0; while (subject[subjectLength]) ++subjectLength;
                const char* result = prefix ? subject + retained : subject;
                const unsigned int resultLength = prefix ? subjectLength - retained : retained;
                if (!appendShellWord(result, resultLength, protection, output, masks, capacity, &length)) return 3;
                if (!assignment && !protection)
                    for (unsigned int byte = start; byte < length; ++byte) gShellWordSplit[byte] = 1;
            }
            i = end - 1; continue;
        }
        if (node->kind == kShellParseTreeNonterminal
            && (textEquals(node->name, "DefaultParameter") || textEquals(node->name, "DoubleDefaultParameter"))) {
            const unsigned int end = shellWordSubtreeEnd(i);
            unsigned int nameRoot = i + 1, fallback = i + 1;
            while (nameRoot < end && !textEquals(nodes[nameRoot].name, "VariableName")) ++nameRoot;
            while (fallback < end && !textEquals(nodes[fallback].name, "DefaultValue")
                && !textEquals(nodes[fallback].name, "DoubleDefaultValue")) ++fallback;
            char name[32];
            if (nameRoot == end || fallback == end || !readShellWordName(nameRoot, name)) return 2;
            int nullDefault = 0, alternate = 0;
            for (unsigned int child = nameRoot; child < fallback; ++child) {
                if (nodes[child].kind != kShellParseTreeNonterminal) continue;
                if (textEquals(nodes[child].name, "DefaultNullOperator")
                    || textEquals(nodes[child].name, "AlternateNonemptyOperator")) nullDefault = 1;
                if (textEquals(nodes[child].name, "AlternateSetOperator")
                    || textEquals(nodes[child].name, "AlternateNonemptyOperator")) alternate = 1;
            }
            // Static traversal visits fallback literals/names without reading the table.
            if (expandStatus) {
                unsigned int size = 0; while (name[size]) ++size;
                const int slot = shellVariableIndex(name, size);
                // Plus needs membership only; colon-plus tests the first byte.
                // Neither copies/decodes the subject or emits its stored value.
                const char* value = "";
                if (slot >= 0 && (!alternate || nullDefault)) value = shellVariableValue(name, size);
                const int hasValue = slot >= 0 && (!nullDefault || value[0]);
                if (alternate && !hasValue) { i = end - 1; continue; }
                if (!alternate && hasValue) {
                    unsigned int count = 0; while (value[count]) ++count;
                    const unsigned int start = length, protection = shellWordProtected(i);
                    if (!appendShellWord(value, count, protection, output, masks, capacity, &length)) return 3;
                    if (!assignment && !protection)
                        for (unsigned int byte = start; byte < length; ++byte) gShellWordSplit[byte] = 1;
                    i = end - 1;
                    continue;
                }
            }
            // No nested defaults: traverse this existing subtree with the same
            // fixed loop. The generated owner filters its closing delimiter.
            i = fallback;
            continue;
        }
        if (node->kind == kShellParseTreeNonterminal
            && (textEquals(node->name, "SingleQuoted") || textEquals(node->name, "DoubleQuoted") || textEquals(node->name, "FallbackQuoted")))
            gShellWordAnchors[length] = 1;
        if (node->kind == kShellParseTreeNonterminal && (textEquals(node->name, "NamedParameter") || textEquals(node->name, "BracedNamedParameter")
                || textEquals(node->name, "ParameterLength"))) {
            unsigned int nameRoot = i + 1;
            while (nameRoot < gShellWordTree.nodeCount && !textEquals(nodes[nameRoot].name, "VariableName")) ++nameRoot;
            char name[32];
            if (nameRoot == gShellWordTree.nodeCount || !readShellWordName(nameRoot, name)) return 3;
            // Static validation does not read bindings or speculate about values.
            if (expandStatus) {
                unsigned int size = 0; while (name[size]) ++size;
                const char* value = shellVariableValue(name, size);
                char digits[3];
                unsigned int count = 0;
                if (textEquals(node->name, "ParameterLength")) {
                    unsigned int scalars = 0;
                    const int counted = shellParameterLength(value, &scalars);
                    if (counted != 1) return counted;
                    if (scalars >= 10) digits[count++] = static_cast<char>('0' + scalars / 10);
                    digits[count++] = static_cast<char>('0' + scalars % 10);
                    value = digits;
                } else while (value[count]) ++count;
                const unsigned int start = length;
                const unsigned int protection = shellWordProtected(i);
                if (!appendShellWord(value, count, protection, output, masks, capacity, &length)) return 3;
                if (!assignment && !protection)
                    for (unsigned int byte = start; byte < length; ++byte) gShellWordSplit[byte] = 1;
            }
            const unsigned int root = i;
            while (i + 1 < gShellWordTree.nodeCount && nodes[i + 1].parent >= static_cast<int>(root)) ++i;
            continue;
        }
        if (node->kind == kShellParseTreeNonterminal && (textEquals(node->name, "StatusParameter") || textEquals(node->name, "BracedStatusParameter"))) {
            const unsigned int protection = shellWordProtected(i);
            if (expandStatus) {
                char digits[10]; unsigned int count = 0; unsigned int value = status;
                do { digits[count++] = static_cast<char>('0' + value % 10); value /= 10; } while (value);
                while (count) if (!appendShellWord(digits + --count, 1, protection, output, masks, capacity, &length)) return 3;
            } else if (!appendShellWord("$?", 2, protection, output, masks, capacity, &length)) return 3;
            const unsigned int root = i;
            while (i + 1 < gShellWordTree.nodeCount && nodes[i + 1].parent >= static_cast<int>(root)) ++i;
            continue;
        }
        if (node->kind != kShellParseTreeTerminal || node->token == kShellWordEof) continue;
        const char* owner = node->parent < 0 ? "" : nodes[node->parent].name;
        if (textEquals(owner, "SingleQuoted") || textEquals(owner, "DoubleQuoted")
            || textEquals(owner, "FallbackQuoted") || textEquals(owner, "EscapeMarker")
            || textEquals(owner, "DefaultParameter") || textEquals(owner, "DoubleDefaultParameter")) continue;
        const unsigned int start = length, protection = shellWordProtected(i);
        if (!appendShellWord(node->value, node->length, protection, output, masks, capacity, &length)) return 3;
        if (!assignment && !protection && shellWordInFallback(i))
            for (unsigned int byte = start; byte < length; ++byte) gShellWordSplit[byte] = 1;
    }
    if (length >= capacity) return 3;
    output[length] = 0;
    return 1;
}

static int renderShellWord(const char* input, unsigned int inputLength,
    unsigned int status, int expandStatus, char* output, unsigned char* masks, unsigned int capacity)
{
    return renderShellWordMode(input, inputLength, status, expandStatus, output, masks, capacity, 0, 0);
}
static int renderShellAssignment(const char* input, unsigned int inputLength,
    unsigned int status, int expandStatus, char* name, char* value)
{
    unsigned char masks[96];
    return renderShellWordMode(input, inputLength, status, expandStatus, value, masks, 96, 1, name);
}
static int validShellVariableName(const char* name)
{
    unsigned int length = 0; while (name[length]) ++length;
    if (!length || length > 31 || parseShellWord(name, length) != 1) return 0;
    for (unsigned int i = 0; i < gShellWordTree.nodeCount; ++i)
        if (gShellWordNodes[i].kind == kShellParseTreeNonterminal && textEquals(gShellWordNodes[i].name, "BareName")) return 1;
    return 0;
}
