/* One bounded script snapshot; all lexical/syntax work uses prepareShellList. */
#include "shell-script-frame.hpp"
#include "shell-script-unit.hpp"
static unsigned char gScriptBytes[16384];
static char gScriptCaller[256];
static unsigned int gScriptCallerLength = 0;
static unsigned int gScriptCallerNext = 0;
static unsigned int gScriptCallerNegated = 0;
static ShellForFrame gScriptCallerFor;
static ShellConditionalFrame gScriptCallerConditional;
static unsigned int gScriptLength = 0;
static unsigned int gScriptOffset = 0;
static unsigned int gScriptLine = 0;
static unsigned int gScriptStatus = 0;
static ShellScriptUnit gScriptUnit;
static void processLine(void);
static void processPreparedLine(int parsed);

static void scriptLineDiagnostic(unsigned int line, const char* message)
{
    char digits[10]; unsigned int count = 0;
    do { digits[count++] = static_cast<char>('0' + line % 10); line /= 10; } while (line);
    writeText(2, "source: line ");
    while (count) writeBytes(2, digits + --count, 1);
    writeText(2, message);
}

static void returnFromScript(unsigned int status)
{
    // Rebuild the immutable caller with the same generated frontend, without
    // executing its prefix or retaining a second parse tree in guest memory.
    for (unsigned int i = 0; i <= gScriptCallerLength; ++i) gLine[i] = gScriptCaller[i];
    gLineLength = gScriptCallerLength;
    clearScriptReturn(); clearScriptUnit(&gScriptUnit);
    gScriptPhase = 0;
    if (prepareShellList() != 1 || gScriptCallerNext > gFlowCount
        || !validForCaller(&gScriptCallerFor, gScriptCallerNext)
        || !validConditionalFrame(&gScriptCallerConditional, gScriptCallerNext)
        || (gScriptCallerFor.active && gScriptCallerConditional.active)) {
        gLineLength = 0; gFatal = 1; return;
    }
    gLineLength = 0;
    copyShellForFrame(&gForFrame, &gScriptCallerFor);
    copyConditionalFrame(&gConditionalFrame, &gScriptCallerConditional);
    gFlowNext = gScriptCallerNext;
    gCommandStatus = status;
    gCommandNegated = gScriptCallerNegated;
    completeCommand();
}

static void finishScriptLoad(unsigned int status, const char* message)
{
    if (message) writeText(2, message);
    if (gScriptHandle >= 0) {
        const int closed = maia_vfs_close(gScriptHandle); gScriptHandle = -1;
        if (closed < 0) { writeText(2, "source: close failed\n"); status = 1; }
    }
    if (status) { returnFromScript(status); return; }
    unsigned int badLine = 0;
    if (!validateScriptFrame(gScriptBytes, gScriptLength, &badLine)) {
        scriptLineDiagnostic(badLine, ": invalid framing\n");
        returnFromScript(2); return;
    }
    gScriptPhase = 2;
}

static void beginScript(unsigned int wordCount)
{
    if (wordCount != 2 || gRedirectionCount || gScriptPhase) {
        gCommandStatus = 2;
        writeText(2, "source: expected . path without redirects or nesting\n");
        completeCommand(); return;
    }
    if (!resolvePath(gCommandWords + kMaxWordBytes)) {
        gCommandStatus = 1; writeText(2, "source: invalid path\n"); completeCommand(); return;
    }
    gScriptHandle = maia_vfs_open(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)), kVfsRead);
    if (gScriptHandle <= 0) {
        gScriptHandle = -1; gCommandStatus = 1;
        writeText(2, "source: cannot open file\n"); completeCommand(); return;
    }
    gScriptCallerLength = textLength(gLine);
    for (unsigned int i = 0; i <= gScriptCallerLength; ++i) gScriptCaller[i] = gLine[i];
    gScriptCallerNext = gFlowNext;
    copyShellForFrame(&gScriptCallerFor, &gForFrame);
    copyConditionalFrame(&gScriptCallerConditional, &gConditionalFrame);
    gScriptCallerNegated = gCommandNegated;
    gCommandNegated = 0;
    clearScriptReturn(); clearScriptUnit(&gScriptUnit);
    gScriptLength = 0; gScriptOffset = 0; gScriptLine = 0; gScriptStatus = 0;
    gScriptPhase = 1;
}

static void resumeScript(void)
{
    if (gScriptPhase == 1) {
        const unsigned int remaining = kScriptBytes - gScriptLength;
        const unsigned int request = remaining == 0 ? 1 : remaining < kReadBufferBytes ? remaining : kReadBufferBytes;
        const int count = maia_vfs_read(gScriptHandle,
            static_cast<int32_t>(reinterpret_cast<uintptr_t>(gReadBuffer)), static_cast<int32_t>(request));
        if (count == -6) return;
        if (count < 0 || static_cast<unsigned int>(count) > request) {
            finishScriptLoad(1, "source: read failed\n"); return;
        }
        if (count == 0) { finishScriptLoad(0, 0); return; }
        if (static_cast<unsigned int>(count) > remaining) {
            finishScriptLoad(2, "source: file limit exceeded\n"); return;
        }
        for (int i = 0; i < count; ++i) gScriptBytes[gScriptLength++] = gReadBuffer[i];
        return;
    }
    const int appended = appendScriptPhysicalLine(gScriptBytes, gScriptLength,
        &gScriptOffset, &gScriptLine, gLine, &gScriptUnit);
    if (!appended) { returnFromScript(gScriptStatus); return; }
    if (appended == 2) { gFatal = 1; return; }
    if (appended == 3) { processPreparedLine(3); return; }
    gLineLength = gScriptUnit.length;
    // Quiet generated probe only: partial trees are never planned/executed.
    int parsed = tokenizeLine();
    if (parsed == 1) parsed = parseShellLine(&gListTree);
    if (!parsed && gScriptOffset < gScriptLength && gScriptUnit.lines < 8) return;
    if (!parsed && gScriptOffset < gScriptLength) parsed = 3;
    gScriptUnit.collecting = 0;
    if (parsed == 1) processLine(); // One bounded duplicate acceptance parse.
    else processPreparedLine(parsed);
    gLineLength = 0;
}
