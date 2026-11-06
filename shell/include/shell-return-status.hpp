/* Optional argv framing only. Decimal spelling comes from MaiaCC. */
static char gReturnInput[96];
static MaiaShellReturn_MaiaCCParserToken gReturnTokens[2];
static MaiaShellParseTreeNode gReturnNodes[8];
static MaiaShellParseTreeCollector gReturnTree;
static int shellReturnStatus(const char* operand, unsigned int previous, unsigned int* status)
{
    unsigned int length = 0;
    if (operand) {
        while (operand[length]) {
            if (length == 95) return 0;
            gReturnInput[length] = operand[length]; ++length;
        }
        gReturnInput[length++] = 0;
    }
    MaiaShellReturn_MaiaCCLexerState lexer;
    MaiaShellReturn_maiaCCLexerStateInit(&lexer, gReturnInput, length);
    if (!MaiaShellReturn_maiaCCLexerTokenize(&lexer, gReturnTokens, 2)) return 0;
    maiaShellParseTreeInit(&gReturnTree, gReturnNodes, 8);
    MaiaShellReturn_MaiaCCParseEvents events;
    events.context = &gReturnTree;
    events.checkpoint = maiaShellParseTreeCheckpoint; events.restore = maiaShellParseTreeRestore;
    events.startNonterminal = maiaShellParseTreeStart; events.terminal = maiaShellParseTreeTerminal;
    events.endNonterminal = maiaShellParseTreeEnd; events.abortNonterminal = maiaShellParseTreeAbort;
    events.matchesContextual = 0;
    MaiaShellReturn_MaiaCCParserState parser;
    MaiaShellReturn_maiaCCParserStateInit(&parser, gReturnTokens, lexer.tokenCount, &events);
    if (!MaiaShellReturn_maiaCC_parse(&parser) || parser.failed || gReturnTree.overflow
        || parser.position != lexer.tokenCount || gReturnTree.activeNode >= 0) return 0;
    if (!operand) { if (previous > 255) return 0; *status = previous; return 1; }
    for (unsigned int i = 0; i < gReturnTree.nodeCount; ++i) {
        const MaiaShellParseTreeNode* node = gReturnNodes + i;
        if (node->kind != kShellParseTreeNonterminal || !textEquals(node->name, "Status")) continue;
        if (i + 1 >= gReturnTree.nodeCount) return 0;
        const MaiaShellParseTreeNode* value = gReturnNodes + i + 1;
        unsigned int number = 0;
        for (unsigned int j = 0; j + 1 < value->length; ++j) {
            const unsigned int digit = static_cast<unsigned int>(value->value[j] - '0');
            if (number > (255U - digit) / 10U) return 0;
            number = number * 10U + digit;
        }
        *status = number; return 1;
    }
    return 0;
}
