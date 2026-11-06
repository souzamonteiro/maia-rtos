/* Semantic evaluation over the generated finite predicate tree. Argv framing
   copies data only; spelling, arity and operator ownership belong to MaiaCC. */
static char gShellTestInput[577];
static MaiaShellTest_MaiaCCParserToken gShellTestTokens[7];
static MaiaShellParseTreeNode gShellTestNodes[128];
static MaiaShellParseTreeCollector gShellTestTree;

static int parseShellTest(const char* input, unsigned int length)
{
    if (length > 576) return 0;
    MaiaShellTest_MaiaCCLexerState lexer;
    MaiaShellTest_maiaCCLexerStateInit(&lexer, input, length);
    if (!MaiaShellTest_maiaCCLexerTokenize(&lexer, gShellTestTokens, 7)) return 0;
    maiaShellParseTreeInit(&gShellTestTree, gShellTestNodes, 128);
    MaiaShellTest_MaiaCCParseEvents events;
    events.context = &gShellTestTree;
    events.checkpoint = maiaShellParseTreeCheckpoint;
    events.restore = maiaShellParseTreeRestore;
    events.startNonterminal = maiaShellParseTreeStart;
    events.terminal = maiaShellParseTreeTerminal;
    events.endNonterminal = maiaShellParseTreeEnd;
    events.abortNonterminal = maiaShellParseTreeAbort;
    events.matchesContextual = 0;
    MaiaShellTest_MaiaCCParserState parser;
    MaiaShellTest_maiaCCParserStateInit(&parser, gShellTestTokens, lexer.tokenCount, &events);
    const int accepted = MaiaShellTest_maiaCC_parse(&parser);
    return accepted && !parser.failed && !gShellTestTree.overflow
        && parser.position == lexer.tokenCount && gShellTestTree.activeNode < 0;
}

/* Input is a generated DECIMAL frame. Unsigned magnitude handles INT32_MIN
   without an overflowing signed operation or an out-of-range signed cast. */
static int shellTestMagnitude(const MaiaShellParseTreeNode* node,
    unsigned int* magnitude, unsigned int* negative)
{
    const char* bytes = node->value;
    const unsigned int length = node->length - 1;
    unsigned int index = 0;
    *negative = bytes[0] == '-' ? 1 : 0;
    if (bytes[0] == '-' || bytes[0] == '+') index = 1;
    const unsigned int limit = *negative ? 2147483648U : 2147483647U;
    *magnitude = 0;
    for (; index < length; ++index) {
        const unsigned int digit = static_cast<unsigned int>(bytes[index] - '0');
        if (*magnitude > (limit - digit) / 10U) return 0;
        *magnitude = *magnitude * 10U + digit;
    }
    if (*magnitude == 0) *negative = 0;
    return 1;
}

static int evaluateShellTest()
{
    const MaiaShellParseTreeNode* values[2];
    unsigned int count = 0, negated = 0, operation = 0;
    for (unsigned int i = 0; i < gShellTestTree.nodeCount; ++i) {
        const MaiaShellParseTreeNode* node = gShellTestNodes + i;
        if (node->kind != kShellParseTreeNonterminal) continue;
        const char* name = node->name;
        if (textEquals(name, "NegatedBinary") || textEquals(name, "NegatedUnary")
            || textEquals(name, "NegatedTruth") || textEquals(name, "BracketNegatedBinary")
            || textEquals(name, "BracketNegatedUnary") || textEquals(name, "BracketNegatedTruth")) negated = 1;
        if ((textEquals(name, "Empty") || textEquals(name, "BracketEmpty"))) operation = 1;
        if (textEquals(name, "Truth") || textEquals(name, "NegatedTruth")
            || textEquals(name, "BracketTruth") || textEquals(name, "BracketNegatedTruth")
            || textEquals(name, "NonemptyOperator")) operation = 2;
        if (textEquals(name, "EmptyOperator")) operation = 3;
        if (textEquals(name, "EqualOperator")) operation = 4;
        if (textEquals(name, "UnequalOperator")) operation = 5;
        if (textEquals(name, "NumericEqual")) operation = 6;
        if (textEquals(name, "NumericUnequal")) operation = 7;
        if (textEquals(name, "Less")) operation = 8;
        if (textEquals(name, "LessEqual")) operation = 9;
        if (textEquals(name, "Greater")) operation = 10;
        if (textEquals(name, "GreaterEqual")) operation = 11;
        if (textEquals(name, "Value") || textEquals(name, "Decimal")) {
            if (count == 2 || i + 1 >= gShellTestTree.nodeCount) return 2;
            const MaiaShellParseTreeNode* terminal = gShellTestNodes + i + 1;
            if (terminal->kind != kShellParseTreeTerminal || !terminal->length) return 2;
            values[count++] = terminal;
        }
    }
    int truth = 0;
    if (operation == 1 && count == 0) truth = 0;
    else if ((operation == 2 || operation == 3) && count == 1)
        truth = operation == 2 ? values[0]->length > 1 : values[0]->length == 1;
    else if ((operation == 4 || operation == 5) && count == 2) {
        truth = values[0]->length == values[1]->length;
        for (unsigned int i = 0; truth && i + 1 < values[0]->length; ++i)
            if (values[0]->value[i] != values[1]->value[i]) truth = 0;
        if (operation == 5) truth = !truth;
    } else if (operation >= 6 && operation <= 11 && count == 2) {
        unsigned int left = 0, right = 0, leftNegative = 0, rightNegative = 0;
        const int leftValid = shellTestMagnitude(values[0], &left, &leftNegative);
        const int rightValid = shellTestMagnitude(values[1], &right, &rightNegative);
        if (!leftValid || !rightValid) return 2;
        int order = 0;
        if (leftNegative != rightNegative) order = leftNegative ? -1 : 1;
        else if (left != right) {
            order = left < right ? -1 : 1;
            if (leftNegative) order = -order;
        }
        if (operation == 6) truth = order == 0;
        if (operation == 7) truth = order != 0;
        if (operation == 8) truth = order < 0;
        if (operation == 9) truth = order <= 0;
        if (operation == 10) truth = order > 0;
        if (operation == 11) truth = order >= 0;
    } else return 2;
    if (negated) truth = !truth;
    return truth ? 0 : 1;
}

static int runShellTest(const char* words, unsigned int count)
{
    if (!count || count > 6) return 2;
    unsigned int length = 0;
    for (unsigned int i = 0; i < count; ++i) {
        unsigned int size = 0;
        const char* value = words + i * 96;
        while (size < 96 && value[size]) ++size;
        if (size >= 96 || size + 1 > 576 - length) return 2;
        for (unsigned int j = 0; j <= size; ++j) gShellTestInput[length++] = value[j];
    }
    gShellTestInput[length] = 0;
    return parseShellTest(gShellTestInput, length) ? evaluateShellTest() : 2;
}
