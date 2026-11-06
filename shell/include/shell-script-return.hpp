/* A return belongs only to the selected unit of the active script snapshot.
   Requested status is independent of raw command/cleanup success. */
struct ShellScriptReturn {
    unsigned int active, status, pc, unit, line, offset, length;
};
static ShellScriptReturn gScriptReturn;
static void clearScriptReturn(void) { gScriptReturn.active = 0; }
static int scriptReturnOwner(void)
{
    if (gScriptPhase != 2) return 0;
    if (gScriptHandle >= 0 || !gScriptLine || gScriptLine > kScriptLines
        || gScriptLength > kScriptBytes || gScriptOffset > gScriptLength
        || !shellFlowValid() || !gFlowNext || gFlowNext > gFlowCount
        || gShellFlow[gFlowNext - 1].kind != 1
        || gShellFlow[gFlowNext - 1].argument >= gListCount
        || !validForCaller(&gForFrame, gFlowNext)
        || !validConditionalFrame(&gConditionalFrame, gFlowNext)
        || (gForFrame.active && gConditionalFrame.active)) return -1;
    return 1;
}
static int armScriptReturn(unsigned int status)
{
    if (gScriptReturn.active || gLoopControl.action || status > 255) return -1;
    const int owner = scriptReturnOwner();
    if (owner != 1) return owner;
    gScriptReturn.active = 1; gScriptReturn.status = status;
    gScriptReturn.pc = gFlowNext; gScriptReturn.unit = gShellFlow[gFlowNext - 1].argument;
    gScriptReturn.line = gScriptLine; gScriptReturn.offset = gScriptOffset;
    gScriptReturn.length = gScriptLength;
    return 1;
}
/* -1 corrupt origin,0 ordinary completion/canceled,1 script return consumed. */
static int consumeScriptReturn(unsigned int rawStatus, unsigned int* status)
{
    if (!gScriptReturn.active) return 0;
    const unsigned int active = gScriptReturn.active;
    clearScriptReturn();
    if (rawStatus) return 0;
    if (active != 1 || gLoopControl.action || gScriptReturn.status > 255
        || scriptReturnOwner() != 1 || gScriptReturn.pc != gFlowNext
        || gScriptReturn.unit != gShellFlow[gFlowNext - 1].argument
        || gScriptReturn.line != gScriptLine || gScriptReturn.offset != gScriptOffset
        || gScriptReturn.length != gScriptLength) return -1;
    *status = gScriptReturn.status;
    return 1;
}
