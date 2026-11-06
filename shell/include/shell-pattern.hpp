/* Private string-pattern storage. Compilation borrows the idle glob compiler
   workspace; programs and NFA states never borrow filename-walk storage. */
static MaiaGlobInstruction gShellPatternCode[96];
static MaiaGlobRange gShellPatternRanges[96];
static unsigned char gShellPatternFirst[96], gShellPatternSecond[96];
static MaiaGlobStorage gShellPatternStorage;
static void initShellPattern(void* workspace, size_t bytes)
{
    gShellPatternStorage.code = gShellPatternCode;
    gShellPatternStorage.code_capacity = 96;
    gShellPatternStorage.ranges = gShellPatternRanges;
    gShellPatternStorage.range_capacity = 96;
    gShellPatternStorage.compiler_workspace = workspace;
    gShellPatternStorage.compiler_workspace_bytes = bytes;
}
/* Offsets include zero/end. Suffix candidates retain [0, at); prefix candidates
   retain [at, length). Ascending boundaries try longest suffix / shortest prefix.
   Each whole-match invocation resets steps; debit one aggregate budget. */
static int matchShellRemoval(const char* subject, unsigned int length,
    const unsigned int* offsets, unsigned int boundaries, const char* pattern,
    unsigned int patternLength, const unsigned char* masks, int longest, int prefix,
    unsigned int budget, unsigned int* retained, unsigned int* used)
{
    MaiaGlobLimits limits;
    maia_glob_default_limits(&limits);
    limits.max_pattern_bytes = 95; limits.max_input_bytes = 95;
    MaiaGlobProgram program;
    const int compiled = maia_glob_compile_utf8(
        reinterpret_cast<const unsigned char*>(pattern), patternLength, masks,
        &gShellPatternStorage, &limits, &program);
    *used = 0;
    if (compiled != MAIA_GLOB_MATCH)
        return compiled == MAIA_GLOB_INVALID ? 5 : 3;
    program.flags = 0;
    MaiaGlobWorkspace work;
    work.first = gShellPatternFirst; work.second = gShellPatternSecond;
    work.capacity = 96;
    for (unsigned int candidate = 0; candidate < boundaries; ++candidate) {
        if (*used >= budget) return 3;
        const unsigned int at = offsets[(longest != prefix) ? candidate : boundaries - 1 - candidate];
        limits.max_steps = budget - *used;
        work.steps = 0;
        const int result = maia_glob_match_utf8(&program,
            reinterpret_cast<const unsigned char*>(subject + (prefix ? 0 : at)), prefix ? at : length - at,
            &work, &limits);
        *used += work.steps;
        if (result == MAIA_GLOB_MATCH) { *retained = at; return 1; }
        if (result != MAIA_GLOB_NOMATCH)
            return result == MAIA_GLOB_INVALID ? 5 : 3;
    }
    *retained = prefix ? 0 : length;
    return 1;
}
