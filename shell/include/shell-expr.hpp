#ifndef MAIA_SHELL_EXPR_HPP
#define MAIA_SHELL_EXPR_HPP
/* Bounded semantic evaluation of the generated final-argv tree.
   No shell lookup, I/O, host evaluation or expression tokenization lives here. */
#include <string.h>
#include "../../maiaregex/maia_bre.h"
#include "generated/shell-expr-frontend.hpp"
#ifndef MAIA_SHELL_PARSE_TREE_COLLECTOR_HPP
#define MaiaCCParseEvents MaiaShellExpr_MaiaCCParseEvents
#include "maia-shell-parse-tree-collector.hpp"
#undef MaiaCCParseEvents
#endif

struct MaiaShellExprValue { char text[96]; unsigned int length; };
struct MaiaShellExprContext {
    char input[1056];
    char result[96];
    MaiaShellExpr_MaiaCCParserToken tokens[12];
    MaiaShellParseTreeNode nodes[512];
    MaiaShellParseTreeCollector tree;
    unsigned int ends[512], bases[512], pending[512];
    MaiaShellExprValue values[12];
    unsigned int tokenCount, valueCount, steps, stepLimit, regexCalls;
    int status, accepted, depthOverflow;
};
static MaiaShellExprContext gShellExpr;

static int shellExprStep(MaiaShellExprContext* s, unsigned int count)
{
    if (s->steps > s->stepLimit || count > s->stepLimit - s->steps) {
        s->status = 3; return 0;
    }
    s->steps += count; return 1;
}
static int shellExprNamed(const MaiaShellParseTreeNode* node, const char* name)
{
    return node->kind == kShellParseTreeNonterminal && strcmp(node->name, name) == 0;
}
static int shellExprLayer(const MaiaShellParseTreeNode* node)
{
    if (node->kind != kShellParseTreeNonterminal) return 0;
    const char* name = node->name;
    unsigned int prefix = 0;
    if (strncmp(name, "Expression", 10) == 0) prefix = 10;
    else if (strncmp(name, "Or", 2) == 0) prefix = 2;
    else if (strncmp(name, "And", 3) == 0) prefix = 3;
    else if (strncmp(name, "Relation", 8) == 0) prefix = 8;
    else if (strncmp(name, "Sum", 3) == 0) prefix = 3;
    else if (strncmp(name, "Product", 7) == 0) prefix = 7;
    else if (strncmp(name, "Match", 5) == 0) prefix = 5;
    else if (strncmp(name, "Primary", 7) == 0) prefix = 7;
    return prefix && name[prefix] >= '0' && name[prefix] <= '4' && name[prefix + 1] == 0;
}
static int shellExprOperator(const MaiaShellParseTreeNode* node)
{
    return shellExprNamed(node, "OrOperator") || shellExprNamed(node, "AndOperator")
        || shellExprNamed(node, "Equal") || shellExprNamed(node, "Unequal")
        || shellExprNamed(node, "Less") || shellExprNamed(node, "LessEqual")
        || shellExprNamed(node, "Greater") || shellExprNamed(node, "GreaterEqual")
        || shellExprNamed(node, "Add") || shellExprNamed(node, "Subtract")
        || shellExprNamed(node, "Multiply") || shellExprNamed(node, "Divide")
        || shellExprNamed(node, "Remainder") || shellExprNamed(node, "MatchOperator");
}
static void shellExprEnd(void* context, const char* name, unsigned int position)
{
    maiaShellParseTreeEnd(context, name, position);
    if (strcmp(name, "DepthOverflow") == 0) gShellExpr.depthOverflow = 1;
}
static int parseShellExprWithCapacity(const char* input, unsigned int length, unsigned int capacity)
{
    MaiaShellExprContext* s = &gShellExpr;
    s->result[0] = 0; s->status = 0; s->accepted = 0; s->depthOverflow = 0;
    s->tokenCount = 0; s->valueCount = 0; s->steps = 0; s->stepLimit = 4096; s->regexCalls = 0;
    maiaShellParseTreeInit(&s->tree, s->nodes, capacity <= 512 ? capacity : 512);
    if (!input) { s->status = 2; return 2; }
    if (length > 1056 || capacity > 512) { s->status = 3; return 3; }
    MaiaShellExpr_MaiaCCLexerState lexer;
    MaiaShellExpr_maiaCCLexerStateInit(&lexer, input, length);
    if (!MaiaShellExpr_maiaCCLexerTokenize(&lexer, s->tokens, 12)) {
        s->status = lexer.failed == 2 || lexer.failed == 3 ? 3 : 2; return s->status;
    }
    s->tokenCount = lexer.tokenCount;
    for (unsigned int i = 0; i < lexer.tokenCount; ++i) {
        if (s->tokens[i].length > 96) { s->status = 3; return 3; }
    }
    MaiaShellExpr_MaiaCCParseEvents events;
    events.context = &s->tree;
    events.checkpoint = maiaShellParseTreeCheckpoint;
    events.restore = maiaShellParseTreeRestore;
    events.startNonterminal = maiaShellParseTreeStart;
    events.terminal = maiaShellParseTreeTerminal;
    events.endNonterminal = shellExprEnd;
    events.abortNonterminal = maiaShellParseTreeAbort;
    events.matchesContextual = 0;
    MaiaShellExpr_MaiaCCParserState parser;
    MaiaShellExpr_maiaCCParserStateInit(&parser, s->tokens, lexer.tokenCount, &events);
    const int accepted = MaiaShellExpr_maiaCC_parse(&parser);
    if (s->tree.overflow || s->depthOverflow || parser.resourceError == kShellExprParseNoMemory
        || parser.resourceError == kShellExprParseLimit) s->status = 3;
    else if (!accepted || parser.failed || parser.position != lexer.tokenCount
        || s->tree.activeNode >= 0 || !s->tree.nodeCount) s->status = 2;
    else s->accepted = 1;
    return s->status;
}
static int parseShellExpr(const char* input, unsigned int length)
{
    return parseShellExprWithCapacity(input, length, 512);
}
/* Decimal conversion is semantic and checked. Continue after a magnitude
   overflow so an otherwise nonnumeric string remains string comparison data. */
static int shellExprNumber(MaiaShellExprContext* s, const MaiaShellExprValue* value,
    unsigned int* magnitude, unsigned int* negative)
{
    unsigned int i = 0, overflow = 0;
    *magnitude = 0; *negative = value->length && value->text[0] == '-' ? 1 : 0;
    if (*negative) i = 1;
    if (i == value->length) return 0;
    const unsigned int limit = *negative ? 2147483648U : 2147483647U;
    for (; i < value->length; ++i) {
        if (!shellExprStep(s, 1)) return 0;
        const unsigned char byte = static_cast<unsigned char>(value->text[i]);
        if (byte < '0' || byte > '9') return 0;
        const unsigned int digit = byte - '0';
        if (!overflow) {
            if (*magnitude > (limit - digit) / 10U) overflow = 1;
            else *magnitude = *magnitude * 10U + digit;
        }
    }
    if (!overflow && *magnitude == 0) *negative = 0;
    return overflow ? 2 : 1;
}
static int shellExprFalse(MaiaShellExprContext* s, const MaiaShellExprValue* value)
{
    if (!value->length) return 1;
    unsigned int i = value->text[0] == '-' ? 1 : 0;
    if (i == value->length) return 0;
    for (; i < value->length; ++i) {
        if (!shellExprStep(s, 1)) return 0;
        if (value->text[i] != '0') return 0;
    }
    return 1;
}
static int shellExprFormat(MaiaShellExprContext* s, MaiaShellExprValue* result,
    unsigned int magnitude, unsigned int negative)
{
    char digits[10]; unsigned int count = 0, used = 0;
    if (!magnitude) negative = 0;
    do {
        if (!shellExprStep(s, 1)) return 0;
        digits[count++] = static_cast<char>('0' + magnitude % 10U);
        magnitude /= 10U;
    } while (magnitude);
    if (negative) result->text[used++] = '-';
    while (count) result->text[used++] = digits[--count];
    result->text[used] = 0; result->length = used; return 1;
}
static int shellExprCompare(MaiaShellExprContext* s, const MaiaShellExprValue* left,
    const MaiaShellExprValue* right, int* comparison)
{
    unsigned int a = 0, b = 0, na = 0, nb = 0;
    const int ca = shellExprNumber(s, left, &a, &na);
    const int cb = shellExprNumber(s, right, &b, &nb);
    if (s->status) return 0;
    *comparison = 0;
    if (ca && cb) {
        if (ca == 2 || cb == 2) { s->status = 2; return 0; }
        if (na != nb) *comparison = na ? -1 : 1;
        else if (a != b) *comparison = (a < b ? -1 : 1) * (na ? -1 : 1);
        return 1;
    }
    unsigned int i = 0;
    while (i < left->length && i < right->length) {
        if (!shellExprStep(s, 1)) return 0;
        const unsigned char x = static_cast<unsigned char>(left->text[i]);
        const unsigned char y = static_cast<unsigned char>(right->text[i]);
        if (x != y) { *comparison = x < y ? -1 : 1; return 1; }
        ++i;
    }
    if (left->length != right->length) *comparison = left->length < right->length ? -1 : 1;
    return 1;
}
static int shellExprBreStatus(int status)
{
    if (status == MAIA_BRE_BADPAT || status == MAIA_BRE_ENCODING
        || status == MAIA_BRE_ARGUMENT || status == MAIA_BRE_UNSUPPORTED) return 2;
    return 3;
}
static int shellExprMatch(MaiaShellExprContext* s, const MaiaShellExprValue* left,
    const MaiaShellExprValue* right, MaiaShellExprValue* result)
{
    if (++s->regexCalls > 5) { s->status = 3; return 0; }
    MaiaBreLimits limits; maia_bre_default_limits(&limits);
    limits.max_pattern_bytes = 95; limits.max_input_bytes = 95;
    limits.max_instructions = 384; limits.max_ranges = 384;
    limits.max_captures = 9; limits.max_parse_depth = 9; limits.max_repeat = 255;
    // Compilation retains the generated frontend context; matching has its
    // separate, smaller workspace quota. Programs are released before returning.
    limits.max_frames = 128; limits.max_steps = 10000; limits.max_workspace_bytes = 393216;
    MaiaBreProgram* program = 0;
    int status = maia_bre_compile_utf8(reinterpret_cast<const unsigned char*>(right->text),
        right->length, &limits, &program);
    if (status) { s->status = shellExprBreStatus(status); return 0; }
    MaiaBreInfo info;
    status = maia_bre_info(program, &info);
    if (status) { maia_bre_free(program); s->status = shellExprBreStatus(status); return 0; }
    MaiaBreCapture captures[2];
    limits.max_workspace_bytes = 65536;
    status = maia_bre_exec_utf8(program, reinterpret_cast<const unsigned char*>(left->text),
        left->length, 0, MAIA_BRE_ANCHORED, &limits, captures, 2);
    maia_bre_free(program);
    if (status != MAIA_BRE_OK && status != MAIA_BRE_NOMATCH) {
        s->status = shellExprBreStatus(status); return 0;
    }
    if (info.capture_count) {
        unsigned int begin = 0, size = 0;
        if (status == MAIA_BRE_OK && captures[1].matched) {
            begin = captures[1].start_byte;
            if (captures[1].end_byte < begin || captures[1].end_byte > left->length) {
                s->status = 3; return 0;
            }
            size = captures[1].end_byte - begin;
        }
        if (!shellExprStep(s, size + 1)) return 0;
        memcpy(result->text, left->text + begin, size);
        result->length = size; result->text[size] = 0; return 1;
    }
    return shellExprFormat(s, result,
        status == MAIA_BRE_OK ? captures[0].end_scalar - captures[0].start_scalar : 0, 0);
}
static int shellExprApply(MaiaShellExprContext* s, unsigned int operation)
{
    if (s->valueCount < 2 || !shellExprStep(s, 1)) { s->status = 3; return 0; }
    MaiaShellExprValue* left = s->values + s->valueCount - 2;
    const MaiaShellExprValue* right = s->values + s->valueCount - 1;
    MaiaShellExprValue result;
    if (operation == kShellExprTokenOR || operation == kShellExprTokenAND) {
        const int a = shellExprFalse(s, left);
        if (s->status) return 0;
        if (operation == kShellExprTokenOR) {
            if (a) result = *right; else result = *left;
        } else {
            const int b = shellExprFalse(s, right);
            if (s->status) return 0;
            if (a || b) { if (!shellExprFormat(s, &result, 0, 0)) return 0; }
            else result = *left;
        }
    } else if (operation == kShellExprTokenCOLON) {
        if (!shellExprMatch(s, left, right, &result)) return 0;
    } else if (operation == kShellExprTokenEQ || operation == kShellExprTokenNE
        || operation == kShellExprTokenLT || operation == kShellExprTokenLE
        || operation == kShellExprTokenGT || operation == kShellExprTokenGE) {
        int comparison = 0;
        if (!shellExprCompare(s, left, right, &comparison)) return 0;
        const unsigned int truth = (operation == kShellExprTokenEQ && comparison == 0)
            || (operation == kShellExprTokenNE && comparison != 0)
            || (operation == kShellExprTokenLT && comparison < 0)
            || (operation == kShellExprTokenLE && comparison <= 0)
            || (operation == kShellExprTokenGT && comparison > 0)
            || (operation == kShellExprTokenGE && comparison >= 0);
        if (!shellExprFormat(s, &result, truth, 0)) return 0;
    } else {
        unsigned int a = 0, b = 0, na = 0, nb = 0, magnitude = 0, negative = 0;
        const int ca = shellExprNumber(s, left, &a, &na);
        const int cb = shellExprNumber(s, right, &b, &nb);
        if (s->status) return 0;
        if (ca != 1 || cb != 1) { s->status = 2; return 0; }
        if (operation == kShellExprTokenPLUS || operation == kShellExprTokenMINUS) {
            if (operation == kShellExprTokenMINUS) nb = !nb;
            if (na == nb) {
                const unsigned int limit = na ? 2147483648U : 2147483647U;
                if (b > limit || a > limit - b) { s->status = 3; return 0; }
                magnitude = a + b; negative = na;
            } else if (a >= b) { magnitude = a - b; negative = na; }
            else { magnitude = b - a; negative = nb; }
        } else if (operation == kShellExprTokenSTAR) {
            negative = na != nb;
            const unsigned int limit = negative ? 2147483648U : 2147483647U;
            if (b && a > limit / b) { s->status = 3; return 0; }
            magnitude = a * b;
        } else if (operation == kShellExprTokenSLASH || operation == kShellExprTokenPERCENT) {
            if (!b) { s->status = 2; return 0; }
            if (operation == kShellExprTokenSLASH) {
                negative = na != nb; magnitude = a / b;
                if (!negative && magnitude > 2147483647U) { s->status = 3; return 0; }
            } else { magnitude = a % b; negative = na; }
        } else { s->status = 3; return 0; }
        if (!shellExprFormat(s, &result, magnitude, negative)) return 0;
    }
    *left = result; --s->valueCount; return 1;
}
/* One forward walk over generated nodes. Each layer retains an accumulator;
   reducing when its next operand exits preserves left association and eager,
   left-to-right evaluation. Parent/end indices replace recursive evaluation. */
static int shellExprFinish(MaiaShellExprContext* s, unsigned int index)
{
    if (!shellExprStep(s, 1)) return 0;
    const MaiaShellParseTreeNode* node = s->nodes + index;
    if (shellExprOperator(node)) {
        if (node->parent < 0 || index + 1 >= s->ends[index]
            || s->nodes[index + 1].kind != kShellParseTreeTerminal) { s->status = 3; return 0; }
        s->pending[node->parent] = s->nodes[index + 1].token;
        return 1;
    }
    const int value = shellExprNamed(node, "Value") || shellExprNamed(node, "Pattern");
    if (value) {
        if (s->valueCount == 12 || index + 1 >= s->ends[index]) { s->status = 3; return 0; }
        const MaiaShellParseTreeNode* token = s->nodes + index + 1;
        if (token->kind != kShellParseTreeTerminal || !token->length || token->length > 96) { s->status = 3; return 0; }
        const unsigned int length = token->length - 1;
        if (!shellExprStep(s, length + 1)) return 0;
        MaiaShellExprValue* output = s->values + s->valueCount++;
        memcpy(output->text, token->value, length); output->text[length] = 0; output->length = length;
    }
    if (value || shellExprLayer(node)) {
        if (shellExprLayer(node) && (s->pending[index] || s->valueCount != s->bases[index] + 1)) { s->status = 3; return 0; }
        if (node->parent >= 0 && s->pending[node->parent]) {
            const unsigned int parent = static_cast<unsigned int>(node->parent);
            if (s->valueCount != s->bases[parent] + 2) { s->status = 3; return 0; }
            if (!shellExprApply(s, s->pending[parent])) return 0;
            s->pending[parent] = 0;
        }
    }
    return 1;
}
static int evaluateShellExpr()
{
    MaiaShellExprContext* s = &gShellExpr;
    s->result[0] = 0;
    if (!s->accepted || s->status) return s->status ? s->status : 2;
    const unsigned int count = s->tree.nodeCount;
    if (!count || count > 512) { s->status = 3; return 3; }
    s->valueCount = 0; s->steps = 0; s->regexCalls = 0;
    for (unsigned int i = 0; i < count; ++i) {
        if (!shellExprStep(s, 1)) return s->status;
        s->ends[i] = i + 1; s->bases[i] = 0; s->pending[i] = 0;
    }
    for (unsigned int reverse = count; reverse; --reverse) {
        const unsigned int i = reverse - 1;
        const int parent = s->nodes[i].parent;
        if (parent >= 0) {
            if (static_cast<unsigned int>(parent) >= i) { s->status = 3; return 3; }
            if (s->ends[parent] < s->ends[i]) s->ends[parent] = s->ends[i];
        }
    }
    int active = -1;
    for (unsigned int i = 0; i < count; ++i) {
        if (!shellExprStep(s, 1)) return s->status;
        while (active >= 0 && s->ends[active] <= i) {
            if (!shellExprFinish(s, static_cast<unsigned int>(active))) return s->status;
            active = s->nodes[active].parent;
        }
        if (s->nodes[i].parent != active) { s->status = 3; return 3; }
        if (s->nodes[i].kind == kShellParseTreeNonterminal) {
            s->bases[i] = s->valueCount; active = static_cast<int>(i);
        }
    }
    while (active >= 0) {
        if (!shellExprFinish(s, static_cast<unsigned int>(active))) return s->status;
        active = s->nodes[active].parent;
    }
    if (s->valueCount != 1) { s->status = 3; return 3; }
    const int zero = shellExprFalse(s, s->values);
    if (s->status) return s->status;
    memcpy(s->result, s->values[0].text, s->values[0].length + 1);
    return zero ? 1 : 0;
}
static int runShellExpr(const char* words, unsigned int wordCount)
{
    gShellExpr.result[0] = 0;
    gShellExpr.accepted = 0;
    if (!words || !wordCount) return 2;
    if (wordCount > 12) return 3;
    unsigned int used = 0;
    for (unsigned int i = 1; i < wordCount; ++i) {
        const char* word = words + i * 96;
        unsigned int length = 0;
        while (length < 96 && word[length]) ++length;
        if (length == 96 || length + 1 > 1056 - used) return 3;
        memcpy(gShellExpr.input + used, word, length);
        used += length; gShellExpr.input[used++] = 0;
    }
    const int status = parseShellExpr(gShellExpr.input, used);
    return status ? status : evaluateShellExpr();
}
#endif
