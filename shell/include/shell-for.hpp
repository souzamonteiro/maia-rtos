/* Frozen, fixed-size loop state. Syntax and names remain generated ownership;
   this adapter reuses word rendering/field provenance and the ordinary walker. */
#include "shell-for-frame.hpp"
static unsigned int gFlowReady = 0;
static void clearShellFor()
{
    gForFrame.active = 0; gForFrame.count = 0; gForFrame.index = 0;
    gExpansionForHeader = 0; gFlowReady = 0;
}
static int prepareForHeader(unsigned int* wordCount, int expand)
{
    *wordCount = 0;
    if (!gForCount) return 1;
    const MaiaShellParseTreeNode* name = gListTree.nodes + gForPlan.name + 1;
    if (name->kind != kShellParseTreeTerminal || name->length > 31) return 3;
    char checkedName[32];
    for (unsigned int i = 0; i < name->length; ++i) checkedName[i] = name->value[i];
    checkedName[name->length] = 0;
    if (!validShellVariableName(checkedName)) return 2;
    if (expand) for (unsigned int i = 0; i <= name->length; ++i) gForFrame.name[i] = checkedName[i];
    const unsigned int incoming = expand ? gLastStatus : 0;
    for (unsigned int i = 0; i < gForPlan.wordCount; ++i) {
        const MaiaShellParseTreeNode* node = gListTree.nodes + gForPlan.words[i] + 1;
        if (node->kind != kShellParseTreeTerminal) return 2;
        char value[96]; unsigned char masks[96];
        const int rendered = renderShellWord(node->value, node->length, incoming, expand, value, masks, 96);
        if (rendered != 1) return rendered;
        ShellWordFields fields;
        if (!expand) {
            fields.count = 1; fields.start[0] = 0; fields.length[0] = textLength(value);
        } else if (splitShellWord(value, textLength(value), gShellWordSplit, gShellWordAnchors, &fields) != 1) return 3;
        if (fields.count > 12 - *wordCount) return 3;
        for (unsigned int field = 0; field < fields.count; ++field) {
            const unsigned int offset = *wordCount * 96;
            for (unsigned int j = 0; j < fields.length[field]; ++j) {
                gCommandWords[offset + j] = value[fields.start[field] + j];
                gWordProtected[offset + j] = masks[fields.start[field] + j];
            }
            gCommandWords[offset + fields.length[field]] = 0;
            gWordStage[(*wordCount)++] = 0;
        }
    }
    return 1;
}
static void failForHeader()
{
    gExpansionForHeader = 0; gForFrame.active = 0; gFlowReady = 0;
    gFlowNext = gForPlan.join; gCommandNegated = 0;
}
static int bindForValue()
{
    if (setShellVariable(gForFrame.name, gForFrame.values + gForFrame.index * 96)) return 1;
    gCommandStatus = 2; writeText(2, "shell: variable table limit exceeded\n");
    failForHeader(); completeCommand(); return 0;
}
static void finishForHeader(unsigned int count)
{
    gExpansionForHeader = 0;
    if (count > 12) { gFatal = 1; return; }
    gForFrame.count = count; gForFrame.index = 0; gForFrame.active = count ? 1 : 0;
    for (unsigned int i = 0; i < count * 96; ++i) gForFrame.values[i] = gExpandedWords[i];
    if (!count) { gListStatus = 0; gLastStatus = 0; gFlowNext = gForPlan.join; }
    else {
        if (!bindForValue()) return;
        gFlowNext = gForPlan.body;
    }
    gFlowReady = 1;
}
static void beginForHeader()
{
    gForFrame.root = gForPlan.root; gForFrame.entry = gForPlan.entry;
    gForFrame.body = gForPlan.body; gForFrame.next = gForPlan.next; gForFrame.join = gForPlan.join;
    gCommandNegated = 0; gCommandStatus = 0;
    gRedirectionCount = 0; gRedirectFlags = 0; gPipelineStageCount = 0;
    unsigned int count = 0;
    const int result = prepareForHeader(&count, 1);
    if (result != 1) {
        gCommandStatus = 2;
        writeText(2, result == 4 ? "shell: invalid UTF-8 in parameter\n"
            : result == 5 ? "shell: invalid parameter pattern\n" : "shell: word expansion limit exceeded\n");
        failForHeader(); completeCommand(); return;
    }
    gExpansionForHeader = 1; gExpansionActive = 1; gExpansionWalking = 0;
    gExpansionInputCount = count; gExpansionIndex = 0; gExpansionCount = 0;
}
static int advanceShellFor()
{
    if (!gForFrame.active || !gForFrame.count || gForFrame.count > 12
        || gForFrame.index >= gForFrame.count) { gFatal = 1; return 0; }
    ++gForFrame.index;
    if (gForFrame.index == gForFrame.count) {
        gForFrame.active = 0; gFlowNext = gForPlan.join; return 1;
    }
    if (!bindForValue()) return 0;
    gFlowNext = gForPlan.body;
    return 1;
}
