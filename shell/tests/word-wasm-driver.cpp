/* Test-only guest probes for generated word trees and semantic byte rendering.
   Production execution is independently exercised by shell.wasm regressions. */
#define MAIACPP_USE_C_ALLOCATOR
#include "../generated/shell-word-frontend.hpp"
#include "../generated/maia-shell-parser.hpp"
#include "../maia-shell-parse-tree-collector.hpp"
static int textEquals(const char* a, const char* b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
#include "../../../maiaglob/maia_glob.h"
#include "../shell-pattern.hpp"
#include "../shell-word.hpp"
#include "word-length-probe.hpp"
static char gProbeInput[256], gProbeOutput[96], gProbeName[32];
static unsigned char gProbeMasks[96];
static ShellWordFields gProbeFields;
static unsigned int gProbeScalarCount, gProbeBudgetUsed, gProbeRetained;
static void* gProbePatternWorkspace = 0;
static int initProbePattern() {
    if (!gProbePatternWorkspace) {
        gProbePatternWorkspace = malloc(maia_glob_compile_workspace_size());
        if (!gProbePatternWorkspace) return 0;
        initShellPattern(gProbePatternWorkspace, maia_glob_compile_workspace_size());
    }
    return 1;
}
extern "C" {
unsigned int probe_budget_used() { return gProbeBudgetUsed; }
unsigned int probe_retained() { return gProbeRetained; }
int probe_budget(unsigned int budget) {
    if (!initProbePattern()) return -1;
    const char* pattern = shellVariableValue("B", 1);
    unsigned int length = 0; while (pattern[length]) ++length;
    unsigned char masks[96];
    for (unsigned int i = 0; i < 96; ++i) masks[i] = 0;
    gProbeRetained = 999; gProbeBudgetUsed = 999;
    return shellSuffixValue(shellVariableValue("NAME", 4), pattern, length,
        masks, 0, budget, &gProbeRetained, &gProbeBudgetUsed);
}
int probe_prefix_budget(unsigned int budget) {
    if (!initProbePattern()) return -1;
    const char* pattern = shellVariableValue("B", 1);
    unsigned int length = 0; while (pattern[length]) ++length;
    unsigned char masks[96];
    for (unsigned int i = 0; i < 96; ++i) masks[i] = 0;
    gProbeRetained = 999; gProbeBudgetUsed = 999;
    return shellRemovalValue(shellVariableValue("NAME", 4), pattern, length,
        masks, 0, 1, budget, &gProbeRetained, &gProbeBudgetUsed);
}
const char* probe_source_literal() { return "ação \377 😀"; }
unsigned int probe_source_literal_size() { return sizeof("ação \377 😀"); }
int probe_count(unsigned int length) {
    if (length > 96) return -1;
    gProbeInput[length] = 0; gProbeScalarCount = 999;
    return shellParameterLength(gProbeInput, &gProbeScalarCount);
}
unsigned int probe_scalar_count() { return gProbeScalarCount; }
int probe_decode_guard(unsigned int length) {
    return length > 96 ? -1 : shellLengthDecodeGuard(gProbeInput, length);
}
char* probe_input() { return gProbeInput; }
char* probe_output() { return gProbeOutput; }
char* probe_masks() { return reinterpret_cast<char*>(gProbeMasks); }
char* probe_name() { return gProbeName; }
char* probe_split_flags() { return reinterpret_cast<char*>(gShellWordSplit); }
char* probe_anchors() { return reinterpret_cast<char*>(gShellWordAnchors); }
int probe_split() { unsigned int length = 0; while (length < 96 && gProbeOutput[length]) ++length; return splitShellWord(gProbeOutput, length, gShellWordSplit, gShellWordAnchors, &gProbeFields); }
unsigned int probe_field_count() { return gProbeFields.count; }
unsigned int probe_field_start(unsigned int i) { return gProbeFields.start[i]; }
unsigned int probe_field_length(unsigned int i) { return gProbeFields.length[i]; }
unsigned int probe_node_count() { return gShellWordTree.nodeCount; }
unsigned int probe_token_count() {
    unsigned int count = 0;
    do { ++count; } while (count < 256 && gShellWordTokens[count - 1].type != kShellWordEof);
    return count;
}
const char* probe_token_name(unsigned int i) { return MaiaShellWord_maiaCCParserTokenName(gShellWordTokens[i].type); }
const char* probe_token_value(unsigned int i) { return gShellWordTokens[i].value; }
unsigned int probe_token_length(unsigned int i) { return gShellWordTokens[i].length; }
unsigned int probe_token_start(unsigned int i) { return gShellWordTokens[i].start; }
unsigned int probe_token_end(unsigned int i) { return gShellWordTokens[i].end; }
unsigned int probe_node_kind(unsigned int i) { return gShellWordNodes[i].kind; }
int probe_node_parent(unsigned int i) { return gShellWordNodes[i].parent; }
const char* probe_node_name(unsigned int i) {
    return gShellWordNodes[i].kind == kShellParseTreeTerminal
        ? MaiaShellWord_maiaCCParserTokenName(gShellWordNodes[i].token) : gShellWordNodes[i].name;
}
const char* probe_node_value(unsigned int i) { return gShellWordNodes[i].value; }
unsigned int probe_node_length(unsigned int i) { return gShellWordNodes[i].length; }
int probe_run(unsigned int length, unsigned int status, int expand, int assignment) {
    if (length > 255 || !initProbePattern()) return -1;
    if (parseShellWord(gProbeInput, length) != 1) return -1;
    return renderShellWordMode(gProbeInput, length, status, expand,
        gProbeOutput, gProbeMasks, 96, assignment, gProbeName);
}
void probe_reset_fields() { unsetShellVariable("NAME"); unsetShellVariable("B"); unsetShellVariable("EMPTY"); }
void probe_seed_b(unsigned int length) {
    if (length > 95) return;
    gProbeInput[length] = 0; setShellVariable("B", gProbeInput);
}
void probe_seed(unsigned int length) {
    if (length > 95) return;
    gProbeInput[length] = 0; setShellVariable("NAME", gProbeInput);
}
}
