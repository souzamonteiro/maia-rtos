/* Bounded planning over the generated preorder AST. All syntax ownership comes
   from MaiaCC; this neither scans source nor constructs substitute syntax. */
static const unsigned int kShellListCapacity = 8;
static const unsigned int kShellFlowCapacity = 32;
struct ShellListUnit {
    unsigned int first, end, assignment, negated, condition;
    unsigned int stages;
    unsigned int stageNodes[4];
    unsigned int pipeNodes[3];
    unsigned int prefixNode; /* Owned ! terminal, or nodeCount when absent. */
};
struct ShellFlowInstruction {
    unsigned int kind, argument, target;
};
/* EXEC, outer gate, failed-condition jump, jump, no-body zero, aggregate join. */
static ShellFlowInstruction gShellFlow[32];
static ShellListUnit gListUnits[8];
static unsigned int gListCount = 0, gFlowCount = 0, gFlowNext = 0;
static unsigned int gFlowStatusChanged = 0;
static unsigned int gListStatus = 0;
static unsigned int gCommandNegated = 0;
static MaiaShellParseTreeCollector gListTree;
/* One generated ForClause descriptor; no runtime values or source pointers. */
struct ShellForPlan {
    unsigned int root, name, wordCount, words[12];
    unsigned int entry, body, next, join;
};
static ShellForPlan gForPlan;
static unsigned int gForCount = 0;
struct ShellConditionalPlan {
    unsigned int root, kind, entry, condition, check, body, next, join;
};
static ShellConditionalPlan gConditionalPlan;
static unsigned int gConditionalCount = 0;
/* Certification of the one flat if region inside a loop; no runtime frame. */
struct ShellLoopIfPlan { unsigned int root, loop, gate, join, end, negated, prefix; };
static ShellLoopIfPlan gLoopIfPlan;
static unsigned int gLoopIfCount = 0;
/* Outcomes:8 done,9 for header,10 for next,11 yield,12-14 conditional control. */
static int shellConditionalClause(const char* name)
{
    return textEquals(name, "WhileClause") || textEquals(name, "UntilClause");
}
static int shellLoopClause(const char* name)
{
    return textEquals(name, "ForClause") || shellConditionalClause(name);
}


static int shellListSelected(unsigned int condition, unsigned int status)
{
    return condition == 0 || (condition == 1 && status == 0) || (condition == 2 && status != 0);
}
static unsigned int shellListEnd(const MaiaShellParseTreeCollector* tree, unsigned int root)
{
    unsigned int end = root + 1;
    while (end < tree->nodeCount && tree->nodes[end].parent >= static_cast<int>(root)) ++end;
    return end;
}
static int shellFlowEmit(unsigned int kind, unsigned int argument, unsigned int target)
{
    if (gFlowCount >= kShellFlowCapacity) return -1;
    const unsigned int index = gFlowCount++;
    gShellFlow[index].kind = kind;
    gShellFlow[index].argument = argument;
    gShellFlow[index].target = target;
    return static_cast<int>(index);
}
/* Certify stage identity and assignment-only restrictions before any rendering. */
static int shellListAppend(const MaiaShellParseTreeCollector* tree,
    unsigned int first, unsigned int end, unsigned int negated,
    unsigned int prefix, unsigned int condition)
{
    if (gListCount >= kShellListCapacity) return 3;
    ShellListUnit* unit = gListUnits + gListCount;
    unit->first = first; unit->end = end; unit->assignment = 0;
    unit->negated = negated; unit->condition = condition; unit->stages = 0;
    unit->prefixNode = prefix;
    unsigned int assignments = 0, commands = 0, redirects = 0, pipes = 0;
    for (unsigned int i = first; i < end; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->kind == kShellParseTreeNonterminal) {
            if (textEquals(node->name, "SimpleCommand")) {
                if (unit->stages == 4) return 2;
                unit->stageNodes[unit->stages++] = i;
            }
            if (textEquals(node->name, "Assignment")) { ++assignments; unit->assignment = i + 1; }
            if (textEquals(node->name, "CommandName")) ++commands;
            if (textEquals(node->name, "Redirection")) ++redirects;
        } else if (node->parent >= 0 && node->length == 1 && node->value[0] == '|') {
            const char* owner = tree->nodes[node->parent].name;
            if (textEquals(owner, "Pipeline") || textEquals(owner, "CompoundSimpleTail")) {
                if (pipes == 3) return 2;
                unit->pipeNodes[pipes++] = i;
            }
        }
    }
    if (!unit->stages || pipes + 1 != unit->stages) return 2;
    if (assignments && (assignments != 1 || commands || redirects || unit->stages != 1)) return 2;
    if (shellFlowEmit(1, gListCount, 0) < 0) return 3;
    ++gListCount;
    return 1;
}
/* The factored body grammar owns pipe tails and AND-OR/separators separately.
   End each unit at its last SimpleCommand subtree, before the next list tail. */
struct ShellBodyCursor {
    unsigned int position, end, first, last, stages, pipes, negated, prefix, condition;
};
static void shellBodyInit(const MaiaShellParseTreeCollector* tree, unsigned int root, ShellBodyCursor* cursor)
{
    const unsigned int end = shellListEnd(tree, root);
    cursor->position = root + 1; cursor->end = end;
    cursor->first = end; cursor->last = end; cursor->stages = 0; cursor->pipes = 0;
    cursor->negated = 0; cursor->prefix = tree->nodeCount; cursor->condition = 0;
}
/* 4 suspends construction at an owned compound. Flat callers reject it;
   loop-body callers emit its flat if blocks, then resume this same cursor. */
static int shellPlanBodySpan(const MaiaShellParseTreeCollector* tree, ShellBodyCursor* cursor,
    unsigned int* compound)
{
    for (unsigned int i = cursor->position; i < cursor->end; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->kind == kShellParseTreeNonterminal && textEquals(node->name, "SimpleCommand")) {
            if (cursor->stages && cursor->pipes != cursor->stages) return 2;
            if (!cursor->stages) cursor->first = i;
            ++cursor->stages; cursor->last = shellListEnd(tree, i); i = cursor->last - 1;
        } else if (node->kind == kShellParseTreeNonterminal && textEquals(node->name, "CompoundCommand")) {
            if (cursor->stages || i + 1 >= cursor->end
                || !textEquals(tree->nodes[i + 1].name, "IfClause")) return 2;
            *compound = i + 1; cursor->position = shellListEnd(tree, i);
            return 4;
        } else if (node->kind == kShellParseTreeTerminal) {
            if (node->parent < 0) return 2;
            const char* owner = tree->nodes[node->parent].name;
            if (textEquals(owner, "CompoundSequence") && node->length == 1 && node->value[0] == '!') {
                if (cursor->stages || cursor->negated) return 2;
                cursor->negated = 1; cursor->prefix = i;
            } else if (textEquals(owner, "CompoundSimpleTail") && node->length == 1 && node->value[0] == '|') {
                if (!cursor->stages || cursor->pipes + 1 != cursor->stages) return 2;
                ++cursor->pipes;
            } else if (textEquals(owner, "AndOrOperator") || textEquals(owner, "SeparatorOp")
                || (textEquals(owner, "NewlineList") && tree->nodes[node->parent].parent >= 0
                    && textEquals(tree->nodes[tree->nodes[node->parent].parent].name, "Separator"))) {
                if (cursor->stages) {
                    const int added = shellListAppend(tree, cursor->first, cursor->last,
                        cursor->negated, cursor->prefix, cursor->condition);
                    if (added != 1) return added;
                    cursor->stages = 0; cursor->pipes = 0; cursor->negated = 0; cursor->prefix = tree->nodeCount;
                }
                if (textEquals(owner, "AndOrOperator")) cursor->condition = node->value[0] == '&' ? 1 : 2;
                else cursor->condition = 0;
            } else if (node->token != MAIACC_TOKEN_NEWLINE) return 2;
        }
    }
    cursor->position = cursor->end;
    if (cursor->stages) return shellListAppend(tree, cursor->first, cursor->last,
        cursor->negated, cursor->prefix, cursor->condition);
    return 1;
}
static int shellPlanBody(const MaiaShellParseTreeCollector* tree, unsigned int root)
{
    ShellBodyCursor cursor; unsigned int compound = 0;
    shellBodyInit(tree, root, &cursor);
    const int result = shellPlanBodySpan(tree, &cursor, &compound);
    return result == 4 ? 2 : result;
}
static int shellPlanIf(const MaiaShellParseTreeCollector* tree, unsigned int root,
    unsigned int condition, unsigned int negated)
{
    unsigned int blocks[7], count = 0, hasElse = 0, elifs = 0;
    const unsigned int end = shellListEnd(tree, root);
    for (unsigned int i = root + 1; i < end; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->parent != static_cast<int>(root)) continue;
        if (node->kind == kShellParseTreeNonterminal && textEquals(node->name, "TerminatedCompoundList")) {
            if (count == 7) return 2;
            blocks[count++] = i;
        } else if (node->kind == kShellParseTreeTerminal) {
            if (node->length == 4 && node->value[0] == 'e' && node->value[1] == 'l') {
                if (node->value[2] == 's' && node->value[3] == 'e') hasElse = 1;
                if (node->value[2] == 'i' && node->value[3] == 'f') ++elifs;
            }
        }
    }
    if (elifs > 2 || count != 2 * (elifs + 1) + hasElse) return 2;
    const int gate = shellFlowEmit(2, condition, 0);
    if (gate < 0) return 3;
    unsigned int joins[4], joinCount = 0;
    for (unsigned int branch = 0; branch <= elifs; ++branch) {
        int result = shellPlanBody(tree, blocks[branch * 2]);
        if (result != 1) return result;
        const int test = shellFlowEmit(3, 0, 0);
        if (test < 0) return 3;
        result = shellPlanBody(tree, blocks[branch * 2 + 1]);
        if (result != 1) return result;
        const int jump = shellFlowEmit(4, 0, 0);
        if (jump < 0) return 3;
        joins[joinCount++] = static_cast<unsigned int>(jump);
        gShellFlow[test].target = gFlowCount;
    }
    if (hasElse) {
        const int result = shellPlanBody(tree, blocks[count - 1]);
        if (result != 1) return result;
    } else if (shellFlowEmit(5, 0, 0) < 0) return 3;
    const int join = shellFlowEmit(6, negated, 0);
    if (join < 0) return 3;
    for (unsigned int i = 0; i < joinCount; ++i) gShellFlow[joins[i]].target = static_cast<unsigned int>(join);
    gShellFlow[gate].target = gFlowCount;
    return 1;
}
static int shellPlanLoopBody(const MaiaShellParseTreeCollector* tree, unsigned int root, unsigned int loop)
{
    ShellBodyCursor cursor; unsigned int compound = 0;
    shellBodyInit(tree, root, &cursor);
    int result = shellPlanBodySpan(tree, &cursor, &compound);
    if (result != 4) return result;
    if (gLoopIfCount) return 2;
    gLoopIfPlan.root = compound; gLoopIfPlan.loop = loop; gLoopIfPlan.gate = gFlowCount;
    gLoopIfPlan.negated = cursor.negated; gLoopIfPlan.prefix = cursor.prefix;
    result = shellPlanIf(tree, compound, cursor.condition, cursor.negated);
    if (result != 1) return result;
    gLoopIfPlan.join = gFlowCount - 1; gLoopIfPlan.end = gFlowCount; gLoopIfCount = 1;
    cursor.condition = 0; cursor.negated = 0; cursor.prefix = tree->nodeCount;
    result = shellPlanBodySpan(tree, &cursor, &compound);
    return result == 4 ? 2 : result;
}
static int shellPlanFor(const MaiaShellParseTreeCollector* tree, unsigned int root,
    unsigned int condition, unsigned int negated)
{
    if (gForCount) return 2;
    const unsigned int end = shellListEnd(tree, root);
    unsigned int name = end, body = end, header = end, explicitIn = 0;
    for (unsigned int i = root + 1; i < end; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->parent != static_cast<int>(root)) continue;
        if (node->kind == kShellParseTreeNonterminal) {
            if (textEquals(node->name, "Name")) name = i;
            if (textEquals(node->name, "WordList")) header = i;
            if (textEquals(node->name, "DoGroup")) {
                const unsigned int groupEnd = shellListEnd(tree, i);
                for (unsigned int j = i + 1; j < groupEnd; ++j)
                    if (tree->nodes[j].parent == static_cast<int>(i)
                        && textEquals(tree->nodes[j].name, "TerminatedCompoundList")) body = j;
            }
        } else if (node->length == 2 && node->value[0] == 'i' && node->value[1] == 'n') explicitIn = 1;
    }
    if (!explicitIn || name == end || body == end) return 2;
    gForPlan.root = root; gForPlan.name = name; gForPlan.wordCount = 0;
    if (header != end) {
        const unsigned int headerEnd = shellListEnd(tree, header);
        for (unsigned int i = header + 1; i < headerEnd; ++i) {
            const MaiaShellParseTreeNode* node = tree->nodes + i;
            if (node->parent != static_cast<int>(header) || !textEquals(node->name, "Word")) continue;
            if (gForPlan.wordCount == 12) return 3;
            gForPlan.words[gForPlan.wordCount++] = i;
        }
    }
    const int gate = shellFlowEmit(2, condition, 0);
    const int entry = shellFlowEmit(7, 0, 0);
    if (gate < 0 || entry < 0) return 3;
    gForPlan.entry = static_cast<unsigned int>(entry); gForPlan.body = gFlowCount;
    const int result = shellPlanLoopBody(tree, body, root);
    if (result != 1) return result;
    const int next = shellFlowEmit(8, 0, gForPlan.body);
    const int join = shellFlowEmit(6, negated, 0);
    if (next < 0 || join < 0) return 3;
    gForPlan.next = static_cast<unsigned int>(next); gForPlan.join = static_cast<unsigned int>(join);
    gShellFlow[entry].target = gForPlan.join;
    gShellFlow[gate].target = gFlowCount;
    gForCount = 1;
    return 1;
}
static int shellPlanConditional(const MaiaShellParseTreeCollector* tree,
    unsigned int root, unsigned int condition, unsigned int negated)
{
    if (gConditionalCount || gForCount) return 2;
    const unsigned int end = shellListEnd(tree, root);
    unsigned int test = end, body = end;
    for (unsigned int i = root + 1; i < end; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->parent != static_cast<int>(root) || node->kind != kShellParseTreeNonterminal) continue;
        if (textEquals(node->name, "TerminatedCompoundList")) test = i;
        if (textEquals(node->name, "DoGroup")) {
            const unsigned int groupEnd = shellListEnd(tree, i);
            for (unsigned int j = i + 1; j < groupEnd; ++j)
                if (tree->nodes[j].parent == static_cast<int>(i)
                    && textEquals(tree->nodes[j].name, "TerminatedCompoundList")) body = j;
        }
    }
    if (test == end || body == end) return 2;
    gConditionalPlan.root = root;
    gConditionalPlan.kind = textEquals(tree->nodes[root].name, "WhileClause") ? 1 : 2;
    const int gate = shellFlowEmit(2, condition, 0);
    const int entry = shellFlowEmit(9, gConditionalPlan.kind, 0);
    if (gate < 0 || entry < 0) return 3;
    gConditionalPlan.entry = static_cast<unsigned int>(entry);
    gConditionalPlan.condition = gFlowCount;
    int result = shellPlanBody(tree, test);
    if (result != 1) return result;
    const int check = shellFlowEmit(10, 0, 0);
    if (check < 0) return 3;
    gConditionalPlan.check = static_cast<unsigned int>(check);
    gConditionalPlan.body = gFlowCount;
    result = shellPlanLoopBody(tree, body, root);
    if (result != 1) return result;
    const int next = shellFlowEmit(11, 0, gConditionalPlan.condition);
    const int join = shellFlowEmit(6, negated, 0);
    if (next < 0 || join < 0) return 3;
    gConditionalPlan.next = static_cast<unsigned int>(next); gConditionalPlan.join = static_cast<unsigned int>(join);
    gShellFlow[gate].target = gFlowCount;
    gShellFlow[entry].target = gConditionalPlan.condition;
    gShellFlow[check].target = gConditionalPlan.join;
    gConditionalCount = 1;
    return 1;
}
static int shellPlanFlow(const MaiaShellParseTreeCollector* tree)
{
    unsigned int ifs = 0, loops = 0, innerIfs = 0;
    /* Reject unsupported structures in every branch before constructing work. */
    for (unsigned int i = 0; i < tree->nodeCount; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->kind != kShellParseTreeNonterminal) continue;
        if (textEquals(node->name, "CompoundCommand")
            && (i + 1 == tree->nodeCount || (!textEquals(tree->nodes[i + 1].name, "IfClause")
                && !shellLoopClause(tree->nodes[i + 1].name)))) return 2;
        if (textEquals(node->name, "IfClause")) {
            if (++ifs > 4) return 3;
        }
        if (shellLoopClause(node->name) && ++loops > 1) return 2;
        if (shellLoopClause(node->name) || textEquals(node->name, "IfClause")) {
            unsigned int inBody = 0;
            for (int parent = node->parent; parent >= 0; parent = tree->nodes[parent].parent) {
                const char* ancestor = tree->nodes[parent].name;
                if (textEquals(ancestor, "DoGroup")) inBody = 1;
                if (textEquals(ancestor, "IfClause")) return 2;
                if (shellLoopClause(ancestor)) {
                    if (!textEquals(node->name, "IfClause") || !inBody || ++innerIfs > 1) return 2;
                }
            }
        }
        if (textEquals(node->name, "SeparatorOp")) {
            if (i + 1 >= tree->nodeCount) return 2;
            const MaiaShellParseTreeNode* terminal = tree->nodes + i + 1;
            if (terminal->kind != kShellParseTreeTerminal || terminal->length != 1 || terminal->value[0] != ';') return 2;
        }
    }
    unsigned int condition = 0;
    for (unsigned int i = 0; i < tree->nodeCount; ++i) {
        const MaiaShellParseTreeNode* node = tree->nodes + i;
        if (node->kind == kShellParseTreeNonterminal && textEquals(node->name, "Pipeline")) {
            const unsigned int end = shellListEnd(tree, i);
            unsigned int negated = 0, prefix = tree->nodeCount, ifRoot = end;
            for (unsigned int j = i + 1; j < end; ++j) {
                const MaiaShellParseTreeNode* child = tree->nodes + j;
                if (ifRoot == end && child->kind == kShellParseTreeNonterminal
                    && (textEquals(child->name, "IfClause") || shellLoopClause(child->name))) ifRoot = j;
                if (child->kind == kShellParseTreeTerminal && child->parent == static_cast<int>(i)
                    && child->length == 1 && child->value[0] == '!') { negated = 1; prefix = j; }
            }
            int result = 0;
            if (ifRoot != end) {
                const unsigned int ifEnd = shellListEnd(tree, ifRoot);
                for (unsigned int j = i + 1; j < end; ++j) {
                    if (j >= ifRoot && j < ifEnd) continue;
                    const MaiaShellParseTreeNode* child = tree->nodes + j;
                    if (child->kind == kShellParseTreeNonterminal
                        && (textEquals(child->name, "SimpleCommand") || textEquals(child->name, "Redirection") || textEquals(child->name, "IfClause"))) return 2;
                    if (child->kind == kShellParseTreeTerminal && j != prefix) return 2;
                }
                result = textEquals(tree->nodes[ifRoot].name, "ForClause")
                    ? shellPlanFor(tree, ifRoot, condition, negated)
                    : shellConditionalClause(tree->nodes[ifRoot].name) ? shellPlanConditional(tree, ifRoot, condition, negated)
                    : shellPlanIf(tree, ifRoot, condition, negated);
            } else result = shellListAppend(tree, i, end, negated, prefix, condition);
            if (result != 1) return result;
            condition = 0; i = end - 1;
        } else if (node->kind == kShellParseTreeTerminal) {
            if (node->token == MAIACC_TOKEN_EOF || node->token == MAIACC_TOKEN_NEWLINE) continue;
            if (node->parent < 0) return 2;
            const char* owner = tree->nodes[node->parent].name;
            if (textEquals(owner, "SeparatorOp")) condition = 0;
            else if (textEquals(owner, "AndOrOperator")) condition = node->value[0] == '&' ? 1 : 2;
            else return 2;
        }
    }
    return 1;
}
static int shellFlowValid()
{
    if (gForCount > 1 || gConditionalCount > 1 || gForCount + gConditionalCount > 1
        || gLoopIfCount > 1 || gFlowCount > 32 || gListCount > 8) return 0;
    unsigned int innerEnd = 0;
    if (gLoopIfCount) {
        const unsigned int body = gForCount ? gForPlan.body : gConditionalPlan.body;
        const unsigned int next = gForCount ? gForPlan.next : gConditionalPlan.next;
        const unsigned int root = gForCount ? gForPlan.root : gConditionalPlan.root;
        if (gForCount + gConditionalCount != 1 || gLoopIfPlan.loop != root
            || gLoopIfPlan.root >= gListTree.nodeCount
            || !textEquals(gListTree.nodes[gLoopIfPlan.root].name, "IfClause")
            || gLoopIfPlan.gate < body || gLoopIfPlan.gate >= gLoopIfPlan.join
            || gLoopIfPlan.join >= gFlowCount
            || gLoopIfPlan.end != gLoopIfPlan.join + 1 || gLoopIfPlan.end > next
            || gLoopIfPlan.end > gFlowCount
            || gShellFlow[gLoopIfPlan.gate].kind != 2
            || gShellFlow[gLoopIfPlan.gate].target != gLoopIfPlan.end
            || gShellFlow[gLoopIfPlan.join].kind != 6
            || gLoopIfPlan.negated > 1
            || gShellFlow[gLoopIfPlan.join].argument != gLoopIfPlan.negated) return 0;
        /* Only the generated sequence immediately owning this compound may
           supply its prefix. Reject stale, unit-owned or borrowed terminals. */
        const int command = gListTree.nodes[gLoopIfPlan.root].parent;
        if (command < 0 || !textEquals(gListTree.nodes[command].name, "CompoundCommand")) return 0;
        const int bodyCommand = gListTree.nodes[command].parent;
        if (bodyCommand < 0 || !textEquals(gListTree.nodes[bodyCommand].name, "CompoundBodyCommand")) return 0;
        const int sequence = gListTree.nodes[bodyCommand].parent;
        if (sequence < 0 || !textEquals(gListTree.nodes[sequence].name, "CompoundSequence")) return 0;
        unsigned int prefix = gListTree.nodeCount;
        for (unsigned int i = static_cast<unsigned int>(sequence) + 1; i < static_cast<unsigned int>(bodyCommand); ++i) {
            const MaiaShellParseTreeNode* node = gListTree.nodes + i;
            if (node->parent != sequence) continue;
            if (prefix != gListTree.nodeCount || node->kind != kShellParseTreeTerminal
                || node->length != 1 || node->value[0] != '!') return 0;
            prefix = i;
        }
        if (gLoopIfPlan.prefix != prefix
            || gLoopIfPlan.negated != (prefix != gListTree.nodeCount ? 1U : 0U)) return 0;
        unsigned int owner = gListTree.nodeCount, inBody = 0;
        for (int parent = gListTree.nodes[gLoopIfPlan.root].parent; parent >= 0;
            parent = gListTree.nodes[parent].parent) {
            const char* ancestor = gListTree.nodes[parent].name;
            if (textEquals(ancestor, "IfClause")) return 0;
            if (textEquals(ancestor, "DoGroup")) inBody = 1;
            if (shellLoopClause(ancestor)) {
                if (!inBody) return 0;
                owner = static_cast<unsigned int>(parent); break;
            }
        }
        if (owner != gLoopIfPlan.loop) return 0;
        innerEnd = shellListEnd(&gListTree, gLoopIfPlan.root);
    }
    unsigned int entries = 0, nexts = 0, conditionalEntries = 0, checks = 0, conditionalNexts = 0;
    for (unsigned int i = 0; i < gFlowCount; ++i) {
        const ShellFlowInstruction* op = gShellFlow + i;
        if (op->kind < 1 || op->kind > 11) return 0;
        if (op->kind == 1 && op->argument >= gListCount) return 0;
        if (gLoopIfCount && op->kind == 1) {
            const ShellListUnit* unit = gListUnits + op->argument;
            const int inSyntax = unit->first > gLoopIfPlan.root && unit->end <= innerEnd;
            const int inFlow = i > gLoopIfPlan.gate && i < gLoopIfPlan.join;
            if (inSyntax != inFlow) return 0;
        }
        if (op->kind >= 2 && op->kind <= 4 && (op->target <= i || op->target > gFlowCount)) return 0;
        if (gLoopIfCount && op->kind >= 2 && op->kind <= 4) {
            if (i >= gLoopIfPlan.gate && i <= gLoopIfPlan.join) {
                if (op->kind == 2 && i != gLoopIfPlan.gate) return 0;
                if (op->kind == 3 && op->target > gLoopIfPlan.join) return 0;
                if (op->kind == 4 && op->target != gLoopIfPlan.join) return 0;
            } else if (op->target > gLoopIfPlan.gate && op->target <= gLoopIfPlan.join) return 0;
        }
        if (op->kind == 2 && op->argument > 2) return 0;
        if (op->kind == 6 && op->argument > 1) return 0;
        if (op->kind == 7) {
            if (!gForCount || ++entries != 1 || op->argument || i != gForPlan.entry
                || op->target != gForPlan.join) return 0;
        }
        if (op->kind == 8) {
            if (!gForCount || ++nexts != 1 || op->argument || i != gForPlan.next
                || op->target != gForPlan.body || op->target >= i) return 0;
        }
        if (op->kind == 9 && (!gConditionalCount || ++conditionalEntries != 1
            || i != gConditionalPlan.entry || op->argument != gConditionalPlan.kind
            || op->target != gConditionalPlan.condition)) return 0;
        if (op->kind == 10 && (!gConditionalCount || ++checks != 1
            || i != gConditionalPlan.check || op->argument || op->target != gConditionalPlan.join)) return 0;
        if (op->kind == 11 && (!gConditionalCount || ++conditionalNexts != 1
            || i != gConditionalPlan.next || op->argument || op->target != gConditionalPlan.condition
            || op->target >= i)) return 0;
    }
    if (gForCount && (entries != 1 || nexts != 1 || gForPlan.wordCount > 12
        || gForPlan.body != gForPlan.entry + 1 || gForPlan.body >= gForPlan.next
        || gForPlan.join != gForPlan.next + 1 || gForPlan.join >= gFlowCount
        || gShellFlow[gForPlan.join].kind != 6)) return 0;
    if (gConditionalCount && (conditionalEntries != 1 || checks != 1 || conditionalNexts != 1
        || gConditionalPlan.kind < 1 || gConditionalPlan.kind > 2
        || gConditionalPlan.condition != gConditionalPlan.entry + 1
        || gConditionalPlan.check <= gConditionalPlan.condition
        || gConditionalPlan.body != gConditionalPlan.check + 1
        || gConditionalPlan.next <= gConditionalPlan.body
        || gConditionalPlan.join != gConditionalPlan.next + 1
        || gConditionalPlan.join >= gFlowCount || gShellFlow[gConditionalPlan.join].kind != 6)) return 0;
    return 1;
}
static int planShellList(const MaiaShellParseTreeCollector* tree)
{
    gListCount = 0; gFlowCount = 0; gFlowNext = 0; gForCount = 0; gConditionalCount = 0; gLoopIfCount = 0;
    int result = shellPlanFlow(tree);
    if (result == 1 && !shellFlowValid()) result = 2;
    if (result != 1) { gListCount = 0; gFlowCount = 0; gForCount = 0; gConditionalCount = 0; gLoopIfCount = 0; }
    return result;
}
/* Bounded dispatch. Only certified loop-next edges return to checked frames. */
static unsigned int nextShellListUnitBudget(unsigned int* remaining)
{
    gFlowStatusChanged = 0;
    while (*remaining && gFlowNext < gFlowCount) {
        --*remaining;
        const ShellFlowInstruction* op = gShellFlow + gFlowNext++;
        if (op->kind == 7) return 9;
        if (op->kind == 8) return 10;
        if (op->kind >= 9) return op->kind + 3;
        if (op->kind == 1) {
            const unsigned int index = op->argument;
            if (shellListSelected(gListUnits[index].condition, gListStatus)) return index;
        } else if ((op->kind == 2 && !shellListSelected(op->argument, gListStatus))
            || (op->kind == 3 && gListStatus != 0) || op->kind == 4) gFlowNext = op->target;
        else if (op->kind == 5 || op->kind == 6) {
            if (op->kind == 5) gListStatus = 0;
            else if (op->argument) gListStatus = gListStatus == 0 ? 1 : 0;
            gFlowStatusChanged = 1;
        }
    }
    return gFlowNext < gFlowCount ? 11 : kShellListCapacity;
}

static unsigned int nextShellListUnit()
{
    unsigned int remaining = kShellFlowCapacity;
    return nextShellListUnitBudget(&remaining);
}
