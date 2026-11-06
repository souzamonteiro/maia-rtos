/* Pure bounded control over generated descriptors. No I/O, parsing, source
   replay or recursive execution; condition results are completed-unit statuses. */
struct ShellConditionalFrame {
    unsigned int active, root, kind, condition, check, body, next, join;
    unsigned int phase, entries, checks, completed, bodyStatus;
};
static ShellConditionalFrame gConditionalFrame;
static void clearConditionalLoop() { gConditionalFrame.active = 0; }
static void copyConditionalFrame(ShellConditionalFrame* to, const ShellConditionalFrame* from)
{
    to->active = from->active; to->root = from->root; to->kind = from->kind;
    to->condition = from->condition; to->check = from->check; to->body = from->body;
    to->next = from->next; to->join = from->join; to->phase = from->phase;
    to->entries = from->entries; to->checks = from->checks;
    to->completed = from->completed; to->bodyStatus = from->bodyStatus;
}
static int validConditionalFrame(const ShellConditionalFrame* frame, unsigned int pc)
{
    if (!frame->active) return 1;
    if (frame->active != 1 || gConditionalCount != 1 || frame->root != gConditionalPlan.root
        || frame->kind != gConditionalPlan.kind || frame->condition != gConditionalPlan.condition
        || frame->check != gConditionalPlan.check || frame->body != gConditionalPlan.body
        || frame->next != gConditionalPlan.next || frame->join != gConditionalPlan.join
        || frame->entries > 12 || frame->checks != frame->entries || frame->completed > 1) return 0;
    if (frame->phase == 1)
        return pc >= frame->condition && pc <= frame->check
            && frame->completed == (frame->entries ? 1U : 0U);
    return frame->phase == 2 && frame->entries && pc >= frame->body && pc <= frame->next
        && frame->completed == (frame->entries > 1 ? 1U : 0U);
}
static int enterConditionalLoop()
{
    if (!gConditionalCount || gConditionalFrame.active) return 0;
    gConditionalFrame.active = 1; gConditionalFrame.root = gConditionalPlan.root;
    gConditionalFrame.kind = gConditionalPlan.kind; gConditionalFrame.condition = gConditionalPlan.condition;
    gConditionalFrame.check = gConditionalPlan.check; gConditionalFrame.body = gConditionalPlan.body;
    gConditionalFrame.next = gConditionalPlan.next; gConditionalFrame.join = gConditionalPlan.join;
    gConditionalFrame.phase = 1; gConditionalFrame.entries = 0; gConditionalFrame.checks = 0;
    gConditionalFrame.completed = 0; gConditionalFrame.bodyStatus = 0;
    return validConditionalFrame(&gConditionalFrame, gFlowNext);
}
/* -1 invariant failure,0 normal end,1 body selected,2 budget exhausted. */
static int checkConditionalLoop()
{
    if (!gConditionalFrame.active || gConditionalFrame.phase != 1
        || !validConditionalFrame(&gConditionalFrame, gConditionalPlan.check)) return -1;
    ++gConditionalFrame.checks;
    const int selected = gConditionalFrame.kind == 1 ? gListStatus == 0 : gListStatus != 0;
    if (!selected || gConditionalFrame.entries == 12) {
        gListStatus = selected ? 2 : gConditionalFrame.completed ? gConditionalFrame.bodyStatus : 0;
        gConditionalFrame.active = 0; gFlowNext = gConditionalPlan.join;
        return selected ? 2 : 0;
    }
    ++gConditionalFrame.entries; gConditionalFrame.phase = 2;
    return 1;
}
static int nextConditionalIteration()
{
    if (!gConditionalFrame.active || gConditionalFrame.phase != 2
        || !validConditionalFrame(&gConditionalFrame, gConditionalPlan.next)) return 0;
    gConditionalFrame.bodyStatus = gListStatus; gConditionalFrame.completed = 1;
    gConditionalFrame.phase = 1; gFlowNext = gConditionalPlan.condition;
    return 1;
}
