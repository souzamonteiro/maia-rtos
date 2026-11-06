/* A request belongs to one completed body unit. No parsing or resource access. */
struct ShellLoopControl {
    unsigned int action, kind, root, pc, unit, target;
};
static ShellLoopControl gLoopControl;
static void clearLoopControl() { gLoopControl.action = 0; }
/* 0: no current body, -1: corrupt plan/frame, 1: certified current body. */
static int loopControlOwner(unsigned int* kind, unsigned int* root,
    unsigned int* next, unsigned int* join)
{
    if (!shellFlowValid() || (gForFrame.active && gConditionalFrame.active)) return -1;
    if (gForFrame.active) {
        if (!validForCaller(&gForFrame, gFlowNext)) return -1;
        *kind = 1; *root = gForFrame.root; *next = gForFrame.next; *join = gForFrame.join;
        if (gFlowNext <= gForFrame.body) return -1;
    } else if (gConditionalFrame.active) {
        if (!validConditionalFrame(&gConditionalFrame, gFlowNext)) return -1;
        if (gConditionalFrame.phase != 2) return 0;
        *kind = gConditionalFrame.kind + 1; *root = gConditionalFrame.root;
        *next = gConditionalFrame.next; *join = gConditionalFrame.join;
        if (gFlowNext <= gConditionalFrame.body) return -1;
    } else return 0;
    if (!gFlowNext || gFlowNext > gFlowCount
        || gShellFlow[gFlowNext - 1].kind != 1
        || gShellFlow[gFlowNext - 1].argument >= gListCount) return -1;
    return 1;
}
static int armLoopControl(unsigned int action)
{
    unsigned int kind = 0, root = 0, next = 0, join = 0;
    if (gLoopControl.action || action < 1 || action > 2) return -1;
    const int owner = loopControlOwner(&kind, &root, &next, &join);
    if (owner != 1) return owner;
    gLoopControl.action = action; gLoopControl.kind = kind; gLoopControl.root = root;
    gLoopControl.pc = gFlowNext; gLoopControl.unit = gShellFlow[gFlowNext - 1].argument;
    gLoopControl.target = action == 1 ? join : next;
    return 1;
}
/* Called once after close/drain and status publication; raw failure cancels. */
static int finishLoopControl(unsigned int rawStatus)
{
    if (!gLoopControl.action) return 1;
    const unsigned int action = gLoopControl.action;
    clearLoopControl();
    if (rawStatus) return 1;
    unsigned int kind = 0, root = 0, next = 0, join = 0;
    if (action > 2 || loopControlOwner(&kind, &root, &next, &join) != 1
        || gLoopControl.kind != kind || gLoopControl.root != root
        || gLoopControl.pc != gFlowNext
        || gLoopControl.unit != gShellFlow[gFlowNext - 1].argument
        || gLoopControl.target != (action == 1 ? join : next)) return 0;
    /* Origin is still certified here. The transfer bypasses the inner join;
       consume its aggregate negation once, after raw cleanup/unit negation. */
    if (gLoopIfCount && gLoopIfPlan.negated
        && gLoopControl.pc - 1 > gLoopIfPlan.gate
        && gLoopControl.pc - 1 < gLoopIfPlan.join)
        gListStatus = gListStatus == 0 ? 1 : 0;
    gFlowNext = gLoopControl.target;
    if (action == 1) {
        if (kind == 1) gForFrame.active = 0;
        else gConditionalFrame.active = 0;
    }
    return 1;
}
