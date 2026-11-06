/* MaiaGlob consumer: generated-pattern compilation and existing VFS transport. */
struct ShellGlobState {
    MaiaGlobInstruction code[257]; MaiaGlobRange ranges[256];
    MaiaGlobProgram program; MaiaGlobStorage storage; MaiaGlobLimits limits;
    MaiaGlobWalker walker; MaiaGlobFs filesystem;
};
static ShellGlobState* gGlob = 0;
static unsigned char gWordProtected[1152];
static char gExpandedWords[1152];
static char gGlobPath[1024];
static unsigned int gExpansionActive = 0;
static unsigned int gExpansionWalking = 0;
static unsigned int gExpansionInputCount = 0;
static unsigned int gExpansionIndex = 0;
static unsigned int gExpansionCount = 0;
static unsigned int gExpansionForHeader = 0;
static void finishForHeader(unsigned int count);
static void failForHeader(void);
static unsigned int gExpansionWordStart = 0;
static int expansionRedirection(void)
{
    for (unsigned int i = 0; i < gRedirectionCount; ++i)
        if (gRedirectionWords[i] == gExpansionIndex) return static_cast<int>(i);
    return -1;
}
static int globOpen(void*, const unsigned char* path, unsigned int length)
{
    if (length > 1023) return -1;
    for (unsigned int i = 0; i < length; ++i) gGlobPath[i] = static_cast<char>(path[i]);
    gGlobPath[length] = 0;
    if (!resolvePath(gGlobPath)) return -1;
    return maia_vfs_opendir(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)));
}
static int globRead(void*, int handle, unsigned char* name, unsigned int capacity, unsigned int* type)
{
    const int count = maia_vfs_readdir(handle, static_cast<int32_t>(reinterpret_cast<uintptr_t>(gDirectoryPacket)), 271);
    if (count <= 0) return count;
    const unsigned int length = directoryUint32(gDirectoryPacket + 8);
    const unsigned int kind = directoryUint32(gDirectoryPacket + 4);
    if (count < 17 || count > 271 || directoryUint32(gDirectoryPacket) != 1
        || directoryUint32(gDirectoryPacket + 12) || (kind != 1 && kind != 2)
        || !length || length > 255 || length >= capacity || length + 16 != static_cast<unsigned int>(count)) return -1;
    for (unsigned int i = 0; i < length; ++i) {
        const unsigned char value = gDirectoryPacket[16 + i];
        if (!value || value == '/') return -1;
        name[i] = value;
    }
    *type = kind; return static_cast<int>(length);
}
static int globClose(void*, int handle) { return maia_vfs_close(handle); }
static void initGlob(void)
{
    gGlob = static_cast<ShellGlobState*>(malloc(sizeof(ShellGlobState)));
    if (!gGlob) { gFatal = 1; return; }
    memset(gGlob, 0, sizeof(ShellGlobState));
    gGlob->storage.code = gGlob->code; gGlob->storage.code_capacity = 257;
    gGlob->storage.ranges = gGlob->ranges; gGlob->storage.range_capacity = 256;
    gGlob->storage.compiler_workspace_bytes = maia_glob_compile_workspace_size();
    gGlob->storage.compiler_workspace = malloc(gGlob->storage.compiler_workspace_bytes);
    if (!gGlob->storage.compiler_workspace) { gFatal = 1; return; }
    initShellPattern(gGlob->storage.compiler_workspace, gGlob->storage.compiler_workspace_bytes);
    maia_glob_default_limits(&gGlob->limits);
    gGlob->limits.max_pattern_bytes = 95;
    gGlob->filesystem.context = 0; gGlob->filesystem.open = globOpen;
    gGlob->filesystem.read = globRead; gGlob->filesystem.close = globClose;
}
static void endExpansion(int status)
{
    maia_glob_walk_cancel(&gGlob->walker);
    gExpansionActive = 0; gExpansionWalking = 0;
    if (status == -100) writeText(2, "shell: ambiguous redirection\n");
    else if (status == MAIA_GLOB_INVALID) writeText(2, "shell: invalid filename pattern\n");
    else if (status == MAIA_GLOB_LIMIT || status == MAIA_GLOB_SPACE) writeText(2, "shell: filename expansion limit exceeded\n");
    else writeText(2, "shell: filename expansion failed\n");
    gCommandStatus = status == -100 || status == MAIA_GLOB_INVALID
        || status == MAIA_GLOB_LIMIT || status == MAIA_GLOB_SPACE ? 2 : 1;
    if (gExpansionForHeader) failForHeader();
    completeCommand();
}
static int appendExpanded(const char* text, unsigned int length)
{
    if (gExpansionCount >= kMaxArgs || length >= kMaxWordBytes) return 0;
    char* output = gExpandedWords + gExpansionCount * kMaxWordBytes;
    for (unsigned int i = 0; i < length; ++i) output[i] = text[i];
    output[length] = 0; gExpandedStage[gExpansionCount]=gWordStage[gExpansionIndex]; ++gExpansionCount; return 1;
}
static int wordAfter(const char* left, const char* right)
{
    unsigned int i = 0;
    while (left[i] && right[i] && left[i] == right[i]) ++i;
    return static_cast<unsigned char>(left[i]) > static_cast<unsigned char>(right[i]);
}
static void sortExpandedWord(void)
{
    for (unsigned int i = gExpansionWordStart + 1; i < gExpansionCount; ++i) {
        unsigned int j = i;
        while (j > gExpansionWordStart && wordAfter(gExpandedWords + (j - 1) * kMaxWordBytes,
            gExpandedWords + j * kMaxWordBytes)) {
            for (unsigned int byte = 0; byte < kMaxWordBytes; ++byte) {
                const char temp = gExpandedWords[(j - 1) * kMaxWordBytes + byte];
                gExpandedWords[(j - 1) * kMaxWordBytes + byte] = gExpandedWords[j * kMaxWordBytes + byte];
                gExpandedWords[j * kMaxWordBytes + byte] = temp;
            }
            --j;
        }
    }
}
static void executeCommand(unsigned int wordCount);
/* The alias policy applies to an actual first argv field, after splitting.
   Redirection packets are not argv; stage ownership comes from the main AST. */
static int shellGlobCommandField(int redirection)
{
    if (gExpansionForHeader || redirection >= 0) return 0;
    for (unsigned int i = 0; i < gExpansionCount; ++i) {
        if (gExpandedStage[i] != gWordStage[gExpansionIndex]) continue;
        unsigned int target = 0;
        for (unsigned int j = 0; j < gRedirectionCount; ++j) {
            if (gRedirectionWords[j] < gExpansionIndex && gRedirectionExpanded[j] == i)
                target = 1;
        }
        if (!target) return 0;
    }
    return 1;
}
static void resumeExpansion(void)
{
    if (!gExpansionWalking) {
        if (gExpansionIndex == gExpansionInputCount) {
            if (gExpansionForHeader) {
                gExpansionActive = 0; finishForHeader(gExpansionCount); return;
            }
            for (unsigned int i = 0; i < 1152; ++i) gCommandWords[i] = gExpandedWords[i];
            gExpansionActive = 0;
            for (unsigned int i = 0; i < gRedirectionCount; ++i) {
                const unsigned int end = i + 1 < gRedirectionCount ? gRedirectionExpanded[i + 1] : gExpansionCount;
                if (end - gRedirectionExpanded[i] != 1) { endExpansion(-100); return; }
            }
            if(gPipelineStageCount>1){
                beginPipeline(gExpansionCount);return;
            }
            if(gExpansionCount && textEquals(gCommandWords,"wc")){pipelineError("shell: unsupported standalone wc\n");return;}
            const unsigned int count = gExpansionCount - gRedirectionCount;
            if (count && textEquals(gCommandWords, ".")) { beginScript(count); return; }
            for (unsigned int i = 0; i < gRedirectionCount; ++i) {
                const char* path = gCommandWords + gRedirectionExpanded[i] * kMaxWordBytes;
                const int opened = gRedirectionFlags[i] == 1 ? beginCommandInput(path) : beginCommandOutput(path);
                if (!opened) { completeCommand(); return; }
            }
            if (count) executeCommand(count);
            if (!gCatActive && !gLsActive) completeCommand();
            return;
        }
        const int redirection = expansionRedirection();
        if (redirection >= 0) gRedirectionExpanded[redirection] = gExpansionCount;
        const unsigned int offset = gExpansionIndex * kMaxWordBytes;
        const char* word = gCommandWords + offset;
        // Keep validation in MaiaGlob. Protect only the exact alias command byte
        // in a temporary mask; source quote/provenance storage is never changed.
        const unsigned char bracketCommandMask = 1;
        const unsigned char* protection = gWordProtected + offset;
        if (word[0] == '[' && word[1] == 0 && shellGlobCommandField(redirection))
            protection = &bracketCommandMask;
        const int compiled = maia_glob_compile_utf8(reinterpret_cast<const unsigned char*>(word), textLength(word),
            protection, &gGlob->storage, &gGlob->limits, &gGlob->program);
        if (compiled != MAIA_GLOB_MATCH) { endExpansion(compiled); return; }
        unsigned int wildcard = 0;
        for (unsigned int i = 0; i < gGlob->program.code_count; ++i) {
            const int op = gGlob->program.code[i].op;
            if (op == MAIA_GLOB_STAR || op == MAIA_GLOB_ANY || op == MAIA_GLOB_CLASS || op == MAIA_GLOB_NOT_CLASS) wildcard = 1;
        }
        if (!wildcard) {
            if (!appendExpanded(word, textLength(word))) { endExpansion(MAIA_GLOB_LIMIT); return; }
            ++gExpansionIndex; return;
        }
        gExpansionWordStart = gExpansionCount;
        const int started = maia_glob_walk_begin(&gGlob->walker, &gGlob->program, &gGlob->filesystem, &gGlob->limits);
        if (started != MAIA_GLOB_MATCH) { endExpansion(started); return; }
        gExpansionWalking = 1;
    }
    unsigned char path[1024]; unsigned int length = 0; unsigned int type = 0;
    const int result = maia_glob_walk_step(&gGlob->walker, path, 1024, &length, &type, 4);
    if (result == MAIA_GLOB_YIELD) return;
    if (result == MAIA_GLOB_ENTRY) {
        if (!appendExpanded(reinterpret_cast<const char*>(path), length)) { endExpansion(MAIA_GLOB_LIMIT); return; }
        const int redirection = expansionRedirection();
        if (redirection >= 0 && gExpansionCount - gRedirectionExpanded[redirection] > 1) endExpansion(-100);
        return;
    }
    if (result != MAIA_GLOB_DONE) { endExpansion(result); return; }
    if (gExpansionCount == gExpansionWordStart) {
        const char* word = gCommandWords + gExpansionIndex * kMaxWordBytes;
        if (!appendExpanded(word, textLength(word))) { endExpansion(MAIA_GLOB_LIMIT); return; }
    } else sortExpandedWord();
    gExpansionWalking = 0; ++gExpansionIndex;
}
