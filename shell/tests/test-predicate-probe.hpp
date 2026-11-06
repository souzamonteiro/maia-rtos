#define MAIACPP_USE_C_ALLOCATOR
#include "../generated/shell-test-frontend.hpp"
#define MaiaCCParseEvents MaiaShellTest_MaiaCCParseEvents
#include "../maia-shell-parse-tree-collector.hpp"
#undef MaiaCCParseEvents
static int textEquals(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; } return *a == *b;
}
#include "../shell-test.hpp"
static char gPredicateProbe[578];
extern "C" {
char* predicate_input() { return gPredicateProbe; }
int predicate_run(unsigned int length) {
    return parseShellTest(gPredicateProbe, length) ? evaluateShellTest() : 2;
}
unsigned int predicate_nodes() { return gShellTestTree.nodeCount; }
int predicate_parent(unsigned int i) { return gShellTestNodes[i].parent; }
unsigned int predicate_kind(unsigned int i) { return gShellTestNodes[i].kind; }
const char* predicate_name(unsigned int i) {
    return gShellTestNodes[i].kind == kShellParseTreeTerminal
        ? MaiaShellTest_maiaCCParserTokenName(gShellTestNodes[i].token) : gShellTestNodes[i].name;
}
const char* predicate_value(unsigned int i) { return gShellTestNodes[i].value; }
unsigned int predicate_length(unsigned int i) { return gShellTestNodes[i].length; }
unsigned int predicate_tokens() {
    unsigned int i = 0;
    do { ++i; } while (i < 7 && gShellTestTokens[i-1].type != kShellTestEof);
    return i;
}
const char* predicate_token_name(unsigned int i) { return MaiaShellTest_maiaCCParserTokenName(gShellTestTokens[i].type); }
const char* predicate_token_value(unsigned int i) { return gShellTestTokens[i].value; }
unsigned int predicate_token_length(unsigned int i) { return gShellTestTokens[i].length; }
unsigned int predicate_token_start(unsigned int i) { return gShellTestTokens[i].start; }
unsigned int predicate_token_end(unsigned int i) { return gShellTestTokens[i].end; }
int predicate_accepted(unsigned int length) { return parseShellTest(gPredicateProbe, length); }
/* Exercise collector exhaustion without altering production capacities. The
   first node beyond the temporary bound must retain its previous bytes. */
int predicate_tree_guard(unsigned int length) {
    if (!parseShellTest(gPredicateProbe, length)) return -1;
    gShellTestNodes[1].name = "predicate-guard";
    const char* guardName = gShellTestNodes[1].name;
    maiaShellParseTreeInit(&gShellTestTree, gShellTestNodes, 1);
    MaiaShellTest_MaiaCCParseEvents events;
    events.context = &gShellTestTree;
    events.checkpoint = maiaShellParseTreeCheckpoint;
    events.restore = maiaShellParseTreeRestore;
    events.startNonterminal = maiaShellParseTreeStart;
    events.terminal = maiaShellParseTreeTerminal;
    events.endNonterminal = maiaShellParseTreeEnd;
    events.abortNonterminal = maiaShellParseTreeAbort;
    events.matchesContextual = 0;
    MaiaShellTest_MaiaCCParserState parser;
    MaiaShellTest_maiaCCParserStateInit(&parser, gShellTestTokens, predicate_tokens(), &events);
    MaiaShellTest_maiaCC_parse(&parser);
    const int guarded = gShellTestTree.overflow && gShellTestTree.nodeCount <= 1
        && gShellTestNodes[1].name == guardName;
    return guarded && parseShellTest(gPredicateProbe, length) ? 1 : 0;
}
int predicate_slots(unsigned int count) { return runShellTest(gPredicateProbe, count); }
}
