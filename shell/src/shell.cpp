#define MAIACPP_USE_C_ALLOCATOR
#include <stdint.h>
#include "../../maiaglob/maia_glob.h"
#include "shell-pattern.hpp"
#include "generated/shell-word-frontend.hpp"
#include "generated/shell-test-frontend.hpp"
#include "generated/shell-expr-frontend.hpp"
#include "generated/shell-return-frontend.hpp"
#include "generated/maia-shell-parser.hpp"
#include "maia-shell-parse-tree-collector.hpp"
#define MAIA_IMPORT(module_name, field_name) __attribute__((import_module(module_name), import_name(field_name)))
extern "C" {
MAIA_IMPORT("maia.stream", "read") int32_t maia_stream_read(int32_t handle, int32_t pointer, int32_t count);
MAIA_IMPORT("maia.stream", "write") int32_t maia_stream_write(int32_t handle, int32_t pointer, int32_t count);
MAIA_IMPORT("maia.stream", "close") int32_t maia_stream_close(int32_t handle);
MAIA_IMPORT("maia.vfs", "open") int32_t maia_vfs_open(int32_t pointer, int32_t length, int32_t flags);
MAIA_IMPORT("maia.vfs", "write") int32_t maia_vfs_write(int32_t handle, int32_t pointer, int32_t count);
MAIA_IMPORT("maia.vfs", "read") int32_t maia_vfs_read(int32_t handle, int32_t pointer, int32_t count);
MAIA_IMPORT("maia.vfs", "opendir") int32_t maia_vfs_opendir(int32_t pointer, int32_t length);
MAIA_IMPORT("maia.vfs", "readdir") int32_t maia_vfs_readdir(int32_t handle, int32_t pointer, int32_t capacity);
MAIA_IMPORT("maia.vfs", "close") int32_t maia_vfs_close(int32_t handle);
MAIA_IMPORT("maia.process", "pipeline_start") int32_t maia_pipeline_start(int32_t pointer,int32_t length);
MAIA_IMPORT("maia.process", "pipeline_cancel") int32_t maia_pipeline_cancel(int32_t group);
MAIA_IMPORT("maia.process", "pipeline_wait") int32_t maia_pipeline_wait(int32_t group,int32_t pointer,int32_t capacity);
MAIA_IMPORT("maia.process", "exit") void maia_process_exit(int32_t status);
}
static const int32_t kVfsRead = 1;
static const unsigned int kMaxTokens = 32;
static const unsigned int kMaxWordBytes = 96;
static const unsigned int kMaxArgs = 12;
static const unsigned int kReadBufferBytes = 128;
static const unsigned int kOutputBufferBytes = 263;
static const char kPromptText[] = "maia> ";
static char gLine[256];
static MaiaCCParserToken gTokens[32];
static MaiaShellParseTreeNode gParseNodes[1024];
static char gCommandWords[1152];
static unsigned int gWordStage[12],gExpandedStage[12],gPipelineNodeIds[4];
static unsigned int gPipelineStageCount=0;
static int gPipelineGroup=0;
static unsigned int gPipelineOutput=0,gPipelineDrained=0;
static unsigned int gPipelineInput=0,gPipelineFed=0,gPipelineFeedOffset=0,gPipelineFeedCount=0;
static uint8_t gPipelineFeed[128];
static uint8_t gPipelinePlan[1536],gPipelineResult[48];
static uint8_t gReadBuffer[128];
static char gOutputBuffer[263];
static uint8_t gInputBuffer[256];
static unsigned int gInputOffset = 0;
static unsigned int gInputCount = 0;
static unsigned int gLineLength = 0;
static unsigned int gTokenCount = 0;
static int gInputOverflow = 0;
static int gInputEOF = 0;
static int gRedirectHandle = -1;
static unsigned int gRedirectFlags = 0;
static int gInputRedirectHandle = -1;
static unsigned int gRedirectionCount = 0;
static unsigned int gRedirectionNodes[2], gRedirectionFlags[2], gRedirectionStage[2];
static unsigned int gRedirectionWords[2], gRedirectionExpanded[2];
static unsigned int gCommandFinishing = 0;
static unsigned int gCommandStatus = 0;
static unsigned int gLastStatus = 0;
static void clearScriptReturn(void);
static unsigned int gScriptPhase = 0; /* 0: terminal; 1: loading; 2: body. */
static int gScriptHandle = -1;
static void beginScript(unsigned int wordCount);
static int gCatHandle = -1;
static unsigned int gCatActive = 0;
static unsigned int gCatCount = 0;
static unsigned int gCatIndex = 0;
static int gDirectoryHandle = -1;
static unsigned int gLsActive = 0;
static unsigned int gLsCount = 0;
static unsigned int gLsIndex = 0;
static unsigned int gLsLookup = 0;
static unsigned int gLsHidden = 0;
static unsigned int gLsDots = 0;
static unsigned int gLsMultiple = 0;
static const char* gLsOperand = 0;
static char gLsBase[256];
static char gCwd[1024] = "/";
// Scratch also holds a maximal cwd plus one bounded operand before normalization.
static char gResolvedPath[1152];
static uint8_t gDirectoryPacket[271];
static char gDirectoryOutput[1024];
static int gExitRequested = 0;
static int gExited = 0;
static int gFatal = 0;
static char gPendingOutput[4096];
static uint8_t gPendingStream[4096];
static unsigned int gPendingOffset = 0;
static unsigned int gPendingLength = 0;
static char charAt(const char* text, unsigned int index)
{
    return *(text + index);
}

static void putChar(char* text, unsigned int index, char value)
{
    *(text + index) = value;
}

static unsigned int textLength(const char* text)
{
    unsigned int length = 0;
    while (text && charAt(text, length)) ++length;
    return length;
}

static int textEquals(const char* left, const char* right)
{
    unsigned int index = 0;
    if (!left || !right) return 0;
    while (charAt(left, index) && charAt(right, index)) {
        if (charAt(left, index) != charAt(right, index)) return 0;
        ++index;
    }
    return charAt(left, index) == charAt(right, index);
}


static int writeBytes(int32_t stream, const char* bytes, unsigned int length)
{
    if ((stream != 1 && stream != 2) || length > 4096 - gPendingLength) {
        gFatal = 1; return -4;
    }
    for (unsigned int i = 0; i < length; ++i) {
        gPendingOutput[gPendingLength] = bytes[i];
        gPendingStream[gPendingLength] = static_cast<uint8_t>(stream);
        ++gPendingLength;
    }
    return static_cast<int>(length);
}
static int writeText(int32_t stream, const char* text) { return writeBytes(stream, text, textLength(text)); }
static void abortCommandOutput(void);
static void completeCommand(void) { gCommandFinishing = 1; }
static void flushOutput(void)
{
    for (unsigned int attempt = 0; attempt < 16 && gPendingOffset < gPendingLength; ++attempt) {
        unsigned int end = gPendingOffset + 1;
        const unsigned int stream = gPendingStream[gPendingOffset];
        while (end < gPendingLength && gPendingStream[end] == stream) ++end;
        const int redirected = stream == 1 && gRedirectHandle >= 0;
        const int32_t result = redirected ? maia_vfs_write(gRedirectHandle,
            static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPendingOutput + gPendingOffset)),
            static_cast<int32_t>(end - gPendingOffset)) : maia_stream_write(static_cast<int32_t>(stream),
            static_cast<int32_t>(reinterpret_cast<uintptr_t>(gPendingOutput + gPendingOffset)),
            static_cast<int32_t>(end - gPendingOffset));
        if (result == -6) return;
        if (result <= 0 || static_cast<unsigned int>(result) > end - gPendingOffset) {
            if (redirected) abortCommandOutput(); else gFatal = 1;
            return;
        }
        gPendingOffset += static_cast<unsigned int>(result);
    }
    if (gPendingOffset == gPendingLength) { gPendingOffset = 0; gPendingLength = 0; }
}
static void resumeCat(void)
{
    const int32_t count = maia_vfs_read(gCatHandle,
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(gReadBuffer)), kReadBufferBytes);
    if (count == -6) return;
    if (count > 0) { writeBytes(1, reinterpret_cast<const char*>(gReadBuffer), static_cast<unsigned int>(count)); return; }
    const int status = maia_vfs_close(gCatHandle); gCatHandle = -1;
    if (count < 0) { gCommandStatus = 1; writeText(2, "cat: read failed\n"); }
    else if (status < 0) { gCommandStatus = 1; writeText(2, "cat: close failed\n"); }
}
static int resolvePath(const char* path)
{
    if (!path || !path[0]) return 0;
    unsigned int length = 1;
    gResolvedPath[0] = '/';
    if (path[0] != '/') {
        length = textLength(gCwd);
        for (unsigned int i = 0; i < length; ++i) gResolvedPath[i] = gCwd[i];
    }
    unsigned int offset = 0;
    while (path[offset]) {
        while (path[offset] == '/') ++offset;
        const unsigned int start = offset;
        while (path[offset] && path[offset] != '/') ++offset;
        const unsigned int count = offset - start;
        if (count == 0 || (count == 1 && path[start] == '.')) continue;
        if (count == 2 && path[start] == '.' && path[start + 1] == '.') {
            while (length > 1 && gResolvedPath[length - 1] != '/') --length;
            if (length > 1) --length;
            continue;
        }
        const unsigned int separator = length > 1 ? 1 : 0;
        if (length + separator + count >= 1152) return 0;
        if (separator) gResolvedPath[length++] = '/';
        for (unsigned int i = 0; i < count; ++i) gResolvedPath[length++] = path[start + i];
    }
    if (length >= 1024) return 0;
    gResolvedPath[length] = 0;
    return 1;
}
static void runCat(const char* path)
{
    if (!resolvePath(path)) { gCommandStatus = 1; writeText(2, "cat: invalid path\n"); return; }
    gCatHandle = maia_vfs_open(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)), kVfsRead);
    if (gCatHandle <= 0) {
        gCommandStatus = 1;
        if (gCatHandle == -3) writeText(2, "cat: permission denied\n");
        else if (gCatHandle == -4) writeText(2, "cat: open-file limit exceeded\n");
        else if (gCatHandle == -1) writeText(2, "cat: invalid path\n");
        else writeText(2, "cat: cannot open file\n");
        gCatHandle = -1;
    }
}
static void runCatOperands(unsigned int wordCount)
{
    if (wordCount == 1 && gInputRedirectHandle >= 0) {
        // Transfer ownership; terminal command input remains on stream 0.
        gCatHandle = gInputRedirectHandle; gInputRedirectHandle = -1;
        gCatCount = 1; gCatIndex = 1; gCatActive = 1; return;
    }
    if (wordCount < 2) { gCommandStatus = 2; writeText(2, "usage: cat <file> [file ...]\n"); return; }
    for (unsigned int i = 1; i < wordCount; ++i) {
        if (gCommandWords[i * kMaxWordBytes] == '-') {
            gCommandStatus = 2; writeText(2, "usage: cat <file> [file ...]\n"); return;
        }
    }
    gCatCount = wordCount; gCatIndex = 1; gCatActive = 1;
}
static void resumeCatOperands(void)
{
    if (gCatIndex >= gCatCount) {
        gCatActive = 0;
        completeCommand();
        return;
    }
    const char* path = gCommandWords + gCatIndex * kMaxWordBytes;
    ++gCatIndex;
    runCat(path);
}
static void directoryError(const char* command, int status)
{
    gCommandStatus = 1;
    writeText(2, command); writeText(2, ": ");
    if (status == -1) writeText(2, "invalid path or directory entry\n");
    else if (status == -2) writeText(2, "no such directory\n");
    else if (status == -3) writeText(2, "permission denied\n");
    else if (status == -4) writeText(2, "directory limit exceeded\n");
    else if (status == -9) writeText(2, "not a directory\n");
    else writeText(2, "directory operation failed\n");
}
static void runDirectoryCommand(const char* command, unsigned int wordCount)
{
    const int change = textEquals(command, "cd");
    const char* path = wordCount == 1 ? (change ? "/home/org.maiaplatform.shell" : gCwd) : gCommandWords + kMaxWordBytes;
    if (wordCount > 2 || path[0] == '-') {
        gCommandStatus = 2; writeText(2, change ? "usage: cd [path]\n" : "usage: ls [path]\n"); return;
    }
    if (!resolvePath(path)) { directoryError(command, -1); return; }
    const int32_t handle = maia_vfs_opendir(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)));
    if (handle < 0) { directoryError(command, handle); return; }
    if (change) {
        const int32_t status = maia_vfs_close(handle);
        if (status < 0) { directoryError(command, status); return; }
        const unsigned int length = textLength(gResolvedPath);
        for (unsigned int i = 0; i <= length; ++i) gCwd[i] = gResolvedPath[i];
    } else gDirectoryHandle = handle;
}
static uint32_t directoryUint32(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8)
        | (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}
static void writeEscaped(const char* text, unsigned int length, int newline)
{
    unsigned int output = 0; const char* digits = "0123456789ABCDEF";
    for (unsigned int i = 0; i < length; ++i) {
        const unsigned int value = static_cast<unsigned char>(text[i]);
        if (value == 92) { gDirectoryOutput[output++] = 92; gDirectoryOutput[output++] = 92; }
        else if (value < 32 || value == 127) {
            gDirectoryOutput[output++] = 92; gDirectoryOutput[output++] = 'x';
            gDirectoryOutput[output++] = digits[value >> 4]; gDirectoryOutput[output++] = digits[value & 15];
        } else gDirectoryOutput[output++] = static_cast<char>(value);
    }
    if (newline) gDirectoryOutput[output++] = '\n';
    writeBytes(1, gDirectoryOutput, output);
}
static void resumeLs(void)
{
    if (gLsIndex >= gLsCount) {
        gLsActive = 0; completeCommand(); return;
    }
    const unsigned int index = gLsIndex++;
    const char* operand = index == 0 ? gCwd : gCommandWords + index * kMaxWordBytes;
    gLsOperand = operand; gLsLookup = 0;
    if (!resolvePath(operand)) { directoryError("ls", -1); return; }
    int handle = maia_vfs_opendir(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)));
    if (handle == -9 && operand[textLength(operand) - 1] != '/') {
        unsigned int slash = textLength(gResolvedPath);
        while (slash && gResolvedPath[slash - 1] != '/') --slash;
        const unsigned int length = textLength(gResolvedPath + slash);
        if (length > 255) { directoryError("ls", -1); return; }
        for (unsigned int i = 0; i <= length; ++i) gLsBase[i] = gResolvedPath[slash + i];
        gResolvedPath[slash > 1 ? slash - 1 : 1] = 0;
        handle = maia_vfs_opendir(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
            static_cast<int32_t>(textLength(gResolvedPath)));
        if (handle > 0) gLsLookup = 1;
    }
    if (handle <= 0) { directoryError("ls", handle); return; }
    if (!gLsLookup && gLsMultiple) {
        writeEscaped(operand, textLength(operand), 0); writeText(1, ":\n");
    }
    gDirectoryHandle = handle;
    if (!gLsLookup && gLsDots) writeText(1, "./\n../\n");
}
static void runLs(unsigned int wordCount)
{
    gLsHidden = 0; gLsDots = 0; gLsMultiple = 0;
    unsigned int first = 1;
    for (; first < wordCount; ++first) {
        const char* option = gCommandWords + first * kMaxWordBytes;
        if (textEquals(option, "--")) { ++first; break; }
        if (option[0] != '-' || !option[1]) break;
        for (unsigned int i = 1; option[i]; ++i) {
            if (option[i] == 'a') { gLsHidden = 1; gLsDots = 1; }
            else if (option[i] == 'A') gLsHidden = 1;
            else { gCommandStatus = 2; writeText(2, "usage: ls [-aA] [--] [path ...]\n"); return; }
        }
    }
    for (unsigned int i = first; i < wordCount; ++i) {
        const char* operand = gCommandWords + i * kMaxWordBytes;
        if (!operand[0]) { directoryError("ls", -1); return; }
    }
    gLsMultiple = wordCount - first > 1;
    gLsCount = first == wordCount ? 1 : wordCount;
    gLsIndex = first == wordCount ? 0 : first; gLsActive = 1;
}
static int beginCommandOutput(const char* path)
{
    if (!resolvePath(path)) { gCommandStatus = 1; writeText(2, "shell: invalid redirection path\n"); return 0; }
    gRedirectHandle = maia_vfs_open(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)), static_cast<int32_t>(gRedirectFlags));
    if (gRedirectHandle <= 0) {
        directoryError("shell: redirection", gRedirectHandle);
        gRedirectHandle = -1; return 0;
    }
    return 1;
}
static int beginCommandInput(const char* path)
{
    if (!resolvePath(path)) { gCommandStatus = 1; writeText(2, "shell: invalid redirection path\n"); return 0; }
    gInputRedirectHandle = maia_vfs_open(static_cast<int32_t>(reinterpret_cast<uintptr_t>(gResolvedPath)),
        static_cast<int32_t>(textLength(gResolvedPath)), kVfsRead);
    if (gInputRedirectHandle <= 0) {
        directoryError("shell: input redirection", gInputRedirectHandle);
        gInputRedirectHandle = -1; return 0;
    }
    return 1;
}
#include "shell-pipeline.hpp"
#include "shell-glob.hpp"

static void abortCommandOutput(void)
{
    gCommandStatus = 1;
    // Preserve queued stderr; discard only stdout that the VFS did not accept.
    unsigned int kept = 0;
    for (unsigned int i = gPendingOffset; i < gPendingLength; ++i) {
        if (gPendingStream[i] == 2) {
            gPendingOutput[kept] = gPendingOutput[i]; gPendingStream[kept++] = 2;
        }
    }
    gPendingOffset = 0; gPendingLength = kept;
    if (gCatHandle >= 0) { maia_vfs_close(gCatHandle); gCatHandle = -1; }
    if (gInputRedirectHandle >= 0) { maia_vfs_close(gInputRedirectHandle); gInputRedirectHandle = -1; }
    if (gDirectoryHandle >= 0) { maia_vfs_close(gDirectoryHandle); gDirectoryHandle = -1; }
    if (gRedirectHandle >= 0) { maia_vfs_close(gRedirectHandle); gRedirectHandle = -1; }
    gCatActive = 0; gLsActive = 0;
    writeText(2, "shell: redirection write failed\n");
    if(gPipelineGroup>0){stopPipelineInput();maia_stream_close(3);gPipelineDrained=1;maia_pipeline_cancel(gPipelineGroup);}
    else completeCommand();
}

static void endListing(int status)
{
    const int closed = maia_vfs_close(gDirectoryHandle); gDirectoryHandle = -1;
    if (status == 0 && closed < 0) status = closed;
    if (status < 0) directoryError("ls", status);
}
static void resumeListing(void)
{
    const int32_t count = maia_vfs_readdir(gDirectoryHandle,
        static_cast<int32_t>(reinterpret_cast<uintptr_t>(gDirectoryPacket)), 271);
    if (count <= 0) { endListing(count == 0 && gLsLookup ? -2 : count); return; }
    if (count < 17 || count > 271 || directoryUint32(gDirectoryPacket) != 1
        || directoryUint32(gDirectoryPacket + 12) != 0) { endListing(-1); return; }
    const unsigned int type = directoryUint32(gDirectoryPacket + 4);
    const unsigned int length = directoryUint32(gDirectoryPacket + 8);
    if ((type != 1 && type != 2) || length == 0 || length > 255
        || length + 16 != static_cast<unsigned int>(count)) { endListing(-1); return; }
    if (gLsLookup) {
        unsigned int same = length == textLength(gLsBase) ? 1 : 0;
        for (unsigned int i = 0; same && i < length; ++i)
            if (gDirectoryPacket[16 + i] != static_cast<unsigned char>(gLsBase[i])) same = 0;
        if (!same) return;
        if (type != 1) { endListing(-9); return; }
        writeEscaped(gLsOperand, textLength(gLsOperand), 1);
        endListing(0); return;
    }
    unsigned int output = 0;
    const char* digits = "0123456789ABCDEF";
    for (unsigned int i = 0; i < length; ++i) {
        const unsigned int value = gDirectoryPacket[16 + i];
        if (value == 0 || value == '/') { endListing(-1); return; }
        if (value == 92) { gDirectoryOutput[output++] = 92; gDirectoryOutput[output++] = 92; }
        else if (value < 32 || value == 127) {
            gDirectoryOutput[output++] = 92; gDirectoryOutput[output++] = 'x';
            gDirectoryOutput[output++] = digits[value >> 4]; gDirectoryOutput[output++] = digits[value & 15];
        } else gDirectoryOutput[output++] = static_cast<char>(value);
    }
    if (type == 2) gDirectoryOutput[output++] = '/';
    gDirectoryOutput[output++] = '\n';
    if (!gLsHidden && gDirectoryPacket[16] == '.') return;
    writeBytes(1, gDirectoryOutput, output);
}
static int tokenizeLine(void)
{
    MaiaCCLexerState lexer;
    gTokenCount = 0;
    maiaCCLexerStateInit(&lexer, gLine, gLineLength);
    if (!maiaCCLexerTokenize(&lexer, gTokens, kMaxTokens)) {
        return lexer.failed == 1 ? 0 : 3;
    }
    gTokenCount = lexer.tokenCount;
    return 1;
}

static int parseShellLine(MaiaShellParseTreeCollector* tree)
{
    MaiaCCParserState parser;
    MaiaCCParseEvents events;
    maiaShellParseTreeInit(
        tree,
        gParseNodes,
        kShellParseTreeCapacity);
    maiaShellParseTreeSetEvents(tree, &events);
    maiaCCParserStateInit(&parser, gTokens, gTokenCount, &events);
    const int accepted = maiaCC_parse(&parser);
    if (tree->overflow) return 3;
    return accepted && !parser.failed && parser.position == gTokenCount
        && tree->activeNode < 0;
}

#include "shell-list.hpp"
#include "shell-word.hpp"
static unsigned int gAssignmentUnit = 0;
static char gAssignmentName[32], gAssignmentValue[96];

static int buildCommandWords(const MaiaShellParseTreeCollector* tree, const ShellListUnit* unit, unsigned int* wordCount, int expandStatus)
{
    const unsigned int first = unit->first, end = unit->end;
    const unsigned int incomingStatus = expandStatus ? gLastStatus : 0;
    *wordCount = 0;
    gAssignmentUnit = 0;
    unsigned int simpleCommands = 0;
    unsigned int redirections = 0;
    unsigned int sourceWords = 0;
    gPipelineStageCount = 0;
    gRedirectFlags = 0;
    gRedirectionCount = 0;
    for (unsigned int index = first; index < end; ++index) {
        const MaiaShellParseTreeNode* node = tree->nodes + index;
        if (node->kind != kShellParseTreeNonterminal) continue;
        if (textEquals(node->name, "SimpleCommand")) {
            if (simpleCommands >= unit->stages || unit->stageNodes[simpleCommands] != index) return 2;
            if(simpleCommands<4)gPipelineNodeIds[simpleCommands]=index;
            ++simpleCommands;
        }
        if (textEquals(node->name, "Assignment")) {
            // The list planner already certified one assignment-only unit.
            const MaiaShellParseTreeNode* terminal = tree->nodes + index + 1;
            gAssignmentUnit = 1;
            return renderShellAssignment(terminal->value, terminal->length, incomingStatus,
                expandStatus, gAssignmentName, gAssignmentValue);
        }
        if (textEquals(node->name, "Redirection")) {
            if (redirections >= 2) return 2;
            gRedirectionNodes[redirections] = index;
            gRedirectionFlags[redirections] = 0;
            gRedirectionWords[redirections] = kMaxArgs;
            ++redirections;
        }
    }
    if (simpleCommands > 4 || simpleCommands != unit->stages) return 2;
    gPipelineStageCount=simpleCommands;
    gRedirectionCount = redirections;
    // Decode argv first, then targets in AST source order.
    for (unsigned int pass = 0; pass < 2; ++pass) {
    for (unsigned int index = first; index < end; ++index) {
        const MaiaShellParseTreeNode* node = tree->nodes + index;
        if (node->kind != kShellParseTreeTerminal) continue;
        int parent = node->parent;
        unsigned int redirected = 0;
        unsigned int redirection = 0;
        while (parent >= 0) {
            const MaiaShellParseTreeNode* ancestor = tree->nodes + parent;
            if (textEquals(ancestor->name, "Redirection")) {
                redirected = 1;
                while (redirection < redirections && gRedirectionNodes[redirection] != static_cast<unsigned int>(parent)) ++redirection;
                if (redirection >= redirections) return 2;
            }
            parent = ancestor->parent;
        }
        if (index == unit->prefixNode) continue;
        unsigned int pipe = 0;
        while (pipe + 1 < unit->stages && unit->pipeNodes[pipe] != index) ++pipe;
        if (pipe + 1 < unit->stages) continue;
        if ((pass == 0 && redirected) || (pass == 1 && !redirected)) continue;
        if (redirected && node->token != MAIACC_TOKEN_WORD) {
            if (node->parent < 0 || !textEquals(tree->nodes[node->parent].name, "RedirectionOperator")) return 2;
            if (node->length == 1 && node->value[0] == '>') gRedirectionFlags[redirection] = 14;
            else if (node->length == 2 && node->value[0] == '>' && node->value[1] == '>') gRedirectionFlags[redirection] = 22;
            else if (node->length == 1 && node->value[0] == '<') gRedirectionFlags[redirection] = 1;
            else return 2;
            continue;
        }
        if (node->token == MAIACC_TOKEN_EOF || node->token == MAIACC_TOKEN_NEWLINE) continue;
        if (node->token != MAIACC_TOKEN_WORD
            && node->token != MAIACC_TOKEN_ASSIGNMENT_WORD
            && node->token != MAIACC_TOKEN_IO_NUMBER) return 2;
        if (sourceWords++ >= kMaxArgs) return 3;
        int commandParent=node->parent;
        while(commandParent>=0 && !textEquals(tree->nodes[commandParent].name,"SimpleCommand"))commandParent=tree->nodes[commandParent].parent;
        unsigned int stage=0;
        while(stage<simpleCommands && gPipelineNodeIds[stage]!=static_cast<unsigned int>(commandParent))++stage;
        if(stage>=simpleCommands)return 2;
        char renderedWord[96]; unsigned char renderedProtection[96];
        const int rendered = renderShellWord(node->value, node->length, incomingStatus,
            expandStatus, renderedWord, renderedProtection, kMaxWordBytes);
        if (rendered != 1) return rendered;
        ShellWordFields fields;
        // Preflight reserves original slots without looking up values. Redirect
        // targets always remain one word; only selected ordinary argv split.
        if (!expandStatus || redirected) {
            fields.count = 1; fields.start[0] = 0; fields.length[0] = textLength(renderedWord);
        } else if (splitShellWord(renderedWord, textLength(renderedWord),
            gShellWordSplit, gShellWordAnchors, &fields) != 1) return 3;
        if (fields.count > kMaxArgs - *wordCount) return 3;
        if (redirected) {
            gRedirectionWords[redirection] = *wordCount;
            gRedirectionStage[redirection] = stage;
        }
        for (unsigned int field = 0; field < fields.count; ++field) {
            const unsigned int offset = *wordCount * kMaxWordBytes;
            for (unsigned int byte = 0; byte < fields.length[field]; ++byte) {
                gCommandWords[offset + byte] = renderedWord[fields.start[field] + byte];
                gWordProtected[offset + byte] = renderedProtection[fields.start[field] + byte];
            }
            gCommandWords[offset + fields.length[field]] = 0;
            gWordStage[*wordCount] = stage; ++*wordCount;
        }
    }
    }
    unsigned int input = 0;
    for (unsigned int i = 0; i < redirections; ++i) {
        if (!gRedirectionFlags[i] || gRedirectionWords[i] >= *wordCount) return 2;
        if (gRedirectionFlags[i] == 1) {
            if (input || (simpleCommands > 1 && gRedirectionStage[i] != 0)) return 2;
            input = 1;
        } else {
            if (gRedirectFlags || (simpleCommands > 1 && gRedirectionStage[i] + 1 != simpleCommands)) return 2;
            gRedirectFlags = gRedirectionFlags[i];
        }
    }
    if (!expandStatus && redirections && *wordCount <= redirections) return 2;
    return 1;
}

#include "shell-for.hpp"
#include "shell-conditional-loop.hpp"
#include "shell-loop-control.hpp"

static int prepareShellList(void)
{
    clearShellFor(); clearConditionalLoop(); clearLoopControl(); clearScriptReturn();
    const int lexical = tokenizeLine();
    if (lexical != 1) return lexical;
    const int parsed = parseShellLine(&gListTree);
    if (parsed != 1) return parsed;
    if (gScriptPhase == 2) {
        // This profile excludes multiline source words. Ownership/spans come
        // from the generated lexer; expanded values keep their existing rules.
        for (unsigned int i = 0; i < gTokenCount; ++i) {
            const MaiaCCParserToken* token = gTokens + i;
            if (token->type != MAIACC_TOKEN_WORD && token->type != MAIACC_TOKEN_ASSIGNMENT_WORD) continue;
            for (unsigned int j = 0; j < token->length; ++j)
                if (token->value[j] == 10 || token->value[j] == 13) return 2;
        }
    }
    const int planned = planShellList(&gListTree);
    if (planned != 1) return planned;
    // Static validation is whole-line and has no VFS/stream/process effects.
    for (unsigned int i = 0; i < gListCount; ++i) {
        unsigned int count = 0;
        const int status = buildCommandWords(&gListTree, gListUnits + i, &count, 0);
        if (status != 1) return status;
    }
    unsigned int headerCount = 0;
    return prepareForHeader(&headerCount, 0);
}

static int beginNextListUnit(void)
{
    unsigned int budget = kShellFlowCapacity;
    while (budget) {
    const unsigned int index = nextShellListUnitBudget(&budget);
    if (gFlowStatusChanged) gLastStatus = gListStatus;
    if (index == 9) { beginForHeader(); return 1; }
    if (index == 10) { if (!advanceShellFor()) return 1; continue; }
    if (index == 11) { gFlowReady = 1; return 1; }
    if (index == 12) {
        if (!enterConditionalLoop()) { gFatal = 1; return 1; }
        continue;
    }
    if (index == 13) {
        const int result = checkConditionalLoop();
        if (result < 0) { gFatal = 1; return 1; }
        if (result == 2) {
            gCommandStatus = 2; gCommandNegated = 0;
            writeText(2, "shell: loop iteration limit exceeded\n");
            completeCommand(); return 1;
        }
        if (result == 0) gLastStatus = gListStatus;
        continue;
    }
    if (index == 14) {
        if (!nextConditionalIteration()) { gFatal = 1; return 1; }
        continue;
    }
    if (index < gListCount) {
        clearLoopControl(); clearScriptReturn();
        gCommandNegated = gListUnits[index].negated;
        unsigned int wordCount = 0;
        const int status = buildCommandWords(&gListTree, gListUnits + index, &wordCount, 1);
        // Decimal substitution can exceed a statically valid literal word.
        if (status == 3) {
            gCommandStatus = 2; writeText(2, "shell: word expansion limit exceeded\n");
            completeCommand(); return 1;
        }
        if (status == 4) {
            gCommandStatus = 2; writeText(2, "shell: invalid UTF-8 in parameter\n");
            completeCommand(); return 1;
        }
        if (status == 5) {
            gCommandStatus = 2; writeText(2, "shell: invalid parameter pattern\n");
            completeCommand(); return 1;
        }
        if (status != 1) { gFatal = 1; return 0; }
        gCommandStatus = 0;
        if (gAssignmentUnit) {
            if (!setShellVariable(gAssignmentName, gAssignmentValue)) {
                gCommandStatus = 2; writeText(2, "shell: variable table limit exceeded\n");
            }
            completeCommand(); return 1;
        }
        gExpansionActive = 1; gExpansionWalking = 0;
        gExpansionInputCount = wordCount; gExpansionIndex = 0; gExpansionCount = 0;
        return 1;
    }
    gListCount = 0; gFlowCount = 0; gFlowNext = 0; gForCount = 0; gConditionalCount = 0; gLoopIfCount = 0;
    clearShellFor(); clearConditionalLoop(); clearLoopControl(); clearScriptReturn(); return 0;
    }
    gFlowReady = 1; return 1;
}

#include "shell-script.hpp"
#include "shell-return-status.hpp"
#include "shell-script-return.hpp"
#include "shell-printf.hpp"
#include "shell-test.hpp"
#include "shell-expr.hpp"
#include "shell-path.hpp"

static void printHelp(void)
{
    writeText(1, "Commands: help, echo, printf, expr, basename, dirname, unset, test/[ ... ], return [n], true, false, :, pwd, ls [path ...], cd [path], cat <file> [file ...], clear, exit\n");
    writeText(1, "Pipelines: cat <file> [file ...] | cat; optional final wc (2-4 stages), first cat < input, final > or >>\n");
    writeText(1, "Variables: NAME=value; $NAME splits space/tab/LF unless quoted; unset [--] [name ...] (32 locals, 31/95 name/value bytes)\n");
    writeText(1, "Words: $? expands the previous status, except single-quoted or escaped dollars\n");
    writeText(1, "Scripts: . path (16 KiB, 256 physical lines; no nesting or invocation redirects)\n");
    writeText(1, "Lists: ; runs next, && on success, || on failure; ! negates status (up to 8 units)\n");
    writeText(1, "Simple commands: < input, > output or >> output; cat < input copies bytes\n");
    writeText(1, "Loops: break/continue [1] in the current body; no control across dot scripts\n");
    writeText(1, "ls: -A includes hidden entries; -a also shows . and ..; -- ends options\n");
    writeText(1, "printf FORMAT [ARG ...]: %s/%b/%c, %d/%i/%u/%o/%x/%X, %%, byte escapes; no automatic newline\n");
}

static void executeCommand(unsigned int wordCount)
{
    const char* command = gCommandWords;
    char* outputBuffer = gOutputBuffer;
    if (textEquals(command, "help")) {
        printHelp();
    } else if (textEquals(command, "echo")) {
        for (unsigned int index = 1; index < wordCount; ++index) {
            if (index > 1) writeText(1, " ");
            writeText(1, gCommandWords + (index * kMaxWordBytes));
        }
        writeText(1, "\n");
    } else if (textEquals(command, "test") || textEquals(command, "[")) {
        gCommandStatus = runShellTest(gCommandWords, wordCount);
        if (gCommandStatus == 2) writeText(2, textEquals(command, "[")
            ? "[: invalid expression, integer or closing ]\n" : "test: invalid expression or integer\n");
    } else if (textEquals(command, "printf")) {
        if (!runPrintf(wordCount)) gCommandStatus = 2;
    } else if (textEquals(command, "expr")) {
        gCommandStatus = runShellExpr(gCommandWords, wordCount);
        if (gCommandStatus <= 1) {
            writeText(1, gShellExpr.result); writeText(1, "\n");
        } else {
            writeText(2, gCommandStatus == 3 ? "expr: overflow or resource limit\n"
                : "expr: invalid expression, number or pattern\n");
        }
    } else if (textEquals(command, "basename") || textEquals(command, "dirname")) {
        if (!runPathUtility(wordCount, textEquals(command, "basename"))) gCommandStatus = 2;
    } else if (textEquals(command, "unset")) {
        unsigned int first = 1;
        if (wordCount > first && textEquals(gCommandWords + first * kMaxWordBytes, "--")) ++first;
        for (unsigned int i = first; i < wordCount; ++i) {
            if (!validShellVariableName(gCommandWords + i * kMaxWordBytes)) {
                gCommandStatus = 2; writeText(2, "unset: invalid variable name\n"); return;
            }
        }
        for (unsigned int i = first; i < wordCount; ++i) unsetShellVariable(gCommandWords + i * kMaxWordBytes);
    } else if (textEquals(command, "return")) {
        unsigned int requested = 0;
        if (wordCount > 2 || !shellReturnStatus(wordCount == 2 ? gCommandWords + kMaxWordBytes : 0, gLastStatus, &requested)) {
            gCommandStatus = 2; writeText(2, "return: expected optional decimal status 0..255\n"); return;
        }
        const int result = armScriptReturn(requested);
        if (result < 0) { gFatal = 1; return; }
        if (!result) { gCommandStatus = 2; writeText(2, "return: requires an active dot script\n"); }
    } else if (textEquals(command, "break") || textEquals(command, "continue")) {
        if (wordCount > 2 || (wordCount == 2 && !textEquals(gCommandWords + kMaxWordBytes, "1"))) {
            gCommandStatus = 2; writeText(2, "shell: loop control expects no operand or 1\n"); return;
        }
        const int result = armLoopControl(textEquals(command, "break") ? 1 : 2);
        if (result < 0) { gFatal = 1; return; }
        if (!result) { gCommandStatus = 2; writeText(2, "shell: loop control requires an active body\n"); }
    } else if (textEquals(command, "true") || textEquals(command, ":")) {
        // Arguments and redirects already passed the ordinary expansion path.
        // Preserve completion/close errors instead of publishing status early.
    } else if (textEquals(command, "false")) {
        gCommandStatus = 1;
    } else if (textEquals(command, "pwd") && wordCount == 1) {
        writeText(1, gCwd); writeText(1, "\n");
    } else if (textEquals(command, "ls")) {
        runLs(wordCount);
    } else if (textEquals(command, "cd")) {
        runDirectoryCommand(command, wordCount);
    } else if (textEquals(command, "cat")) {
        runCatOperands(wordCount);
    } else if (textEquals(command, "clear") && wordCount == 1) {
        writeText(1, "\033[2J\033[H");
    } else if (textEquals(command, "exit") && wordCount == 1) {
        gExitRequested = 1;
    } else {
        gCommandStatus = textEquals(command, "pwd") || textEquals(command, "clear") || textEquals(command, "exit") ? 2 : 127;
        putChar(outputBuffer, 0, 0);
        const char* prefix = "command not found: ";
        unsigned int offset = 0;
        while (charAt(prefix, offset) && offset + 1 < kOutputBufferBytes) {
            putChar(outputBuffer, offset, charAt(prefix, offset));
            ++offset;
        }
        unsigned int character = 0;
        while (charAt(command, character) && offset + 2 < kOutputBufferBytes) {
            putChar(outputBuffer, offset, charAt(command, character));
            ++offset;
            ++character;
        }
        putChar(outputBuffer, offset, 10);
        ++offset;
        putChar(outputBuffer, offset, 0);
        writeText(2, outputBuffer);
    }
}

static void processPreparedLine(int parsed)
{
    if (parsed != 1) {
        gLastStatus = 2;
        if (parsed == 0) writeText(2, "syntax error\n");
        else if (parsed == 3) writeText(2, "shell input limit exceeded\n");
        else writeText(2, "unsupported syntax: simple commands only\n");
        if (gScriptPhase == 2) {
            scriptLineDiagnostic(gScriptUnit.startLine, ": rejected input\n");
            returnFromScript(2);
        } else if (!gInputEOF) writeText(1, kPromptText);
        return;
    }
    gListStatus = 0;
    if (!beginNextListUnit() && !gInputEOF && !gScriptPhase) writeText(1, kPromptText);
}


static void processLine(void) { processPreparedLine(prepareShellList()); }

static void finish(void)
{
    clearShellFor(); clearConditionalLoop(); clearLoopControl(); clearScriptReturn();
    if (gScriptHandle >= 0) { maia_vfs_close(gScriptHandle); gScriptHandle = -1; }
    gScriptPhase = 0; clearScriptUnit(&gScriptUnit);
    if (gGlob) {
        maia_glob_walk_cancel(&gGlob->walker);
        free(gGlob->storage.compiler_workspace); free(gGlob); gGlob = 0;
    }
    if (gCatHandle >= 0) { maia_vfs_close(gCatHandle); gCatHandle = -1; }
    if (gInputRedirectHandle >= 0) { maia_vfs_close(gInputRedirectHandle); gInputRedirectHandle = -1; }
    if (gDirectoryHandle >= 0) { maia_vfs_close(gDirectoryHandle); gDirectoryHandle = -1; }
    if (gRedirectHandle >= 0) { maia_vfs_close(gRedirectHandle); gRedirectHandle = -1; }
    for (int32_t handle = 0; handle < 3; ++handle) maia_stream_close(handle);
    gExited = 1; maia_process_exit(gFatal ? 1 : 0);
}
static void completeLine(void)
{
    gLine[gLineLength] = 0;
    if (gInputOverflow) {
        gLastStatus = 2;
        writeText(2, "shell input limit exceeded\n");
        if (!gInputEOF) writeText(1, kPromptText);
    } else processLine();
    gLineLength = 0; gInputOverflow = 0;
}
extern "C" __attribute__((visibility("default"))) void maia_app_start(void)
{
    initGlob();
    writeText(1, "MaiaOS Shell MVP. Type help to begin.\n");
    writeText(1, kPromptText); flushOutput();
}
extern "C" __attribute__((visibility("default"))) void maiaos_step(void)
{
    if (gExited) return;
    for (unsigned int budget = 0; budget < 16; ++budget) {
        flushOutput();
        if (gFatal) { finish(); return; }
        if (gPendingLength) return;
        if (gCommandFinishing) {
            if (gInputRedirectHandle >= 0) {
                const int status = maia_vfs_close(gInputRedirectHandle); gInputRedirectHandle = -1;
                if (status < 0) { gCommandStatus = 1; writeText(2, "shell: input redirection close failed\n"); }
            }
            if (gRedirectHandle >= 0) {
                const int status = maia_vfs_close(gRedirectHandle); gRedirectHandle = -1;
                if (status < 0) { gCommandStatus = 1; writeText(2, "shell: redirection close failed\n"); }
            }
            // Close diagnostics must drain before another unit acquires stdout.
            if (gPendingLength) continue;
            gCommandFinishing = 0;
            const unsigned int rawStatus = gCommandStatus;
            unsigned int returnedStatus = 0;
            const int returned = consumeScriptReturn(rawStatus, &returnedStatus);
            if (returned < 0) { gFatal = 1; finish(); return; }
            if (returned) {
                gCommandNegated = 0; clearLoopControl();
                returnFromScript(returnedStatus); continue;
            }
            if (gCommandNegated) gCommandStatus = gCommandStatus == 0 ? 1 : 0;
            gCommandNegated = 0;
            gLastStatus = gCommandStatus;
            gListStatus = gCommandStatus;
            if (!finishLoopControl(rawStatus)) { gFatal = 1; finish(); return; }
            gCommandStatus = gListStatus; gLastStatus = gListStatus;
            if (gExitRequested) { gListCount = 0; gFlowCount = 0; gFlowNext = 0; }
            else if (!beginNextListUnit()) {
                if (gScriptPhase == 2) gScriptStatus = gListStatus;
                else if (!gInputEOF) writeText(1, kPromptText);
            }
            continue;
        }
        if (gExitRequested) { finish(); return; }
        if (gFlowReady) {
            gFlowReady = 0;
            if (!beginNextListUnit()) {
                if (gScriptPhase == 2) gScriptStatus = gListStatus;
                else if (!gInputEOF) writeText(1, kPromptText);
            }
            continue;
        }
        if (gPipelineGroup>0) { resumePipeline(); if(gPipelineGroup>0)return; continue; }
        if (gExpansionActive) { resumeExpansion(); if(gPipelineGroup>0)return; continue; }
        if (gCatHandle >= 0) { resumeCat(); continue; }
        if (gCatActive) { resumeCatOperands(); continue; }
        if (gDirectoryHandle >= 0) { resumeListing(); continue; }
        if (gLsActive) { resumeLs(); continue; }
        if (gScriptPhase) { resumeScript(); if (gScriptPhase == 1 || gScriptUnit.collecting) return; continue; }
        if (gInputEOF) { gExitRequested = 1; continue; }
        if (gInputOffset == gInputCount) {
            const int32_t count = maia_stream_read(0,
                static_cast<int32_t>(reinterpret_cast<uintptr_t>(gInputBuffer)), 256);
            if (count == -6) return;
            if (count < 0) { gFatal = 1; continue; }
            if (count == 0) {
                gInputEOF = 1;
                if (gLineLength || gInputOverflow) completeLine();
                continue;
            }
            gInputOffset = 0; gInputCount = static_cast<unsigned int>(count);
        }
        for (unsigned int i = 0; i < 256 && gInputOffset < gInputCount; ++i) {
            const char value = static_cast<char>(gInputBuffer[gInputOffset++]);
            if (value == '\n') { completeLine(); break; }
            if (gLineLength < 255) gLine[gLineLength++] = value;
            else gInputOverflow = 1;
        }
    }
}
