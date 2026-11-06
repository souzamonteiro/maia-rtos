#define MAIACPP_USE_C_ALLOCATOR
#include "../../../maiaregex/maia_bre_compile.cpp"
#include "../shell-expr.hpp"
static char gExprProbeInput[1201];
extern "C" {
char* expr_input() { return gExprProbeInput; }
const char* expr_result() { return gShellExpr.result; }
int expr_run(unsigned int length) {
    const int status = parseShellExpr(gExprProbeInput, length);
    return status ? status : evaluateShellExpr();
}
int expr_accepted() { return gShellExpr.accepted; }
unsigned int expr_token_count() { return gShellExpr.tokenCount; }
const char* expr_token_name(unsigned int i) { return MaiaShellExpr_maiaCCParserTokenName(gShellExpr.tokens[i].type); }
const char* expr_token_value(unsigned int i) { return gShellExpr.tokens[i].value; }
unsigned int expr_token_length(unsigned int i) { return gShellExpr.tokens[i].length; }
unsigned int expr_token_start(unsigned int i) { return gShellExpr.tokens[i].start; }
unsigned int expr_token_end(unsigned int i) { return gShellExpr.tokens[i].end; }
unsigned int expr_node_count() { return gShellExpr.tree.nodeCount; }
int expr_node_parent(unsigned int i) { return gShellExpr.nodes[i].parent; }
unsigned int expr_node_kind(unsigned int i) { return gShellExpr.nodes[i].kind; }
const char* expr_node_name(unsigned int i) {
    return gShellExpr.nodes[i].kind == kShellParseTreeTerminal
        ? MaiaShellExpr_maiaCCParserTokenName(gShellExpr.nodes[i].token) : gShellExpr.nodes[i].name;
}
const char* expr_node_value(unsigned int i) { return gShellExpr.nodes[i].value; }
unsigned int expr_node_length(unsigned int i) { return gShellExpr.nodes[i].length; }
unsigned int expr_steps() { return gShellExpr.steps; }
int expr_slots(unsigned int count) { return runShellExpr(gExprProbeInput, count); }
int expr_budget(unsigned int length, unsigned int budget) {
    const int status = parseShellExpr(gExprProbeInput, length);
    if (status) return status;
    gShellExpr.stepLimit = budget; return evaluateShellExpr();
}
int expr_tree_guard(unsigned int length) {
    if (parseShellExpr(gExprProbeInput, length)) return 0;
    gShellExpr.nodes[1].name = "expr-guard";
    const char* guard = gShellExpr.nodes[1].name;
    if (parseShellExprWithCapacity(gExprProbeInput, length, 1) != 3) return 0;
    if (gShellExpr.nodes[1].name != guard || gShellExpr.tree.nodeCount > 1) return 0;
    return expr_run(length) <= 1 ? 1 : 0;
}
}
