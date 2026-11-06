#ifndef MAIACC_GENERATED_CPP98_PARSER_H
#define MAIACC_GENERATED_CPP98_PARSER_H

#include <stdlib.h>
#include "maia_regex.h"

#define MAIACC_TOKEN_EOF 1
#define MAIACC_TOKEN_CAPTURE_OPEN 2
#define MAIACC_TOKEN_CAPTURE_CLOSE 3
#define MAIACC_TOKEN_CAPTURE_SPACE 4
#define MAIACC_TOKEN_CAPTURE_CONTROL 5
#define MAIACC_TOKEN_LETTER 6
#define MAIACC_TOKEN_DIGIT 7
#define MAIACC_TOKEN_EQUAL 8
#define MAIACC_TOKEN_HASH 9
#define MAIACC_TOKEN_LBRACE 10
#define MAIACC_TOKEN_RBRACE 11
#define MAIACC_TOKEN_BRACED_DOLLAR 12
#define MAIACC_TOKEN_SUBSTITUTION_DOLLAR 13
#define MAIACC_TOKEN_DOLLAR 14
#define MAIACC_TOKEN_COLON 15
#define MAIACC_TOKEN_MINUS 16
#define MAIACC_TOKEN_PLUS 17
#define MAIACC_TOKEN_PERCENT 18
#define MAIACC_TOKEN_QUESTION 19
#define MAIACC_TOKEN_SQUOTE 20
#define MAIACC_TOKEN_DQUOTE 21
#define MAIACC_TOKEN_BACKSLASH 22
#define MAIACC_TOKEN_BACKTICK 23
#define MAIACC_TOKEN_LF 24
#define MAIACC_TOKEN_RAW 25
#define MAIACC_TOKEN_COUNT 26
#define MAIACC_RULE_COUNT 68

static const char* maiaCCParserTokenName(unsigned int type) {
  switch (type) {
    case MAIACC_TOKEN_EOF: return "EOF";
    case MAIACC_TOKEN_CAPTURE_OPEN: return "CAPTURE_OPEN";
    case MAIACC_TOKEN_CAPTURE_CLOSE: return "CAPTURE_CLOSE";
    case MAIACC_TOKEN_CAPTURE_SPACE: return "CAPTURE_SPACE";
    case MAIACC_TOKEN_CAPTURE_CONTROL: return "CAPTURE_CONTROL";
    case MAIACC_TOKEN_LETTER: return "LETTER";
    case MAIACC_TOKEN_DIGIT: return "DIGIT";
    case MAIACC_TOKEN_EQUAL: return "EQUAL";
    case MAIACC_TOKEN_HASH: return "HASH";
    case MAIACC_TOKEN_LBRACE: return "LBRACE";
    case MAIACC_TOKEN_RBRACE: return "RBRACE";
    case MAIACC_TOKEN_BRACED_DOLLAR: return "BRACED_DOLLAR";
    case MAIACC_TOKEN_SUBSTITUTION_DOLLAR: return "SUBSTITUTION_DOLLAR";
    case MAIACC_TOKEN_DOLLAR: return "DOLLAR";
    case MAIACC_TOKEN_COLON: return "COLON";
    case MAIACC_TOKEN_MINUS: return "MINUS";
    case MAIACC_TOKEN_PLUS: return "PLUS";
    case MAIACC_TOKEN_PERCENT: return "PERCENT";
    case MAIACC_TOKEN_QUESTION: return "QUESTION";
    case MAIACC_TOKEN_SQUOTE: return "SQUOTE";
    case MAIACC_TOKEN_DQUOTE: return "DQUOTE";
    case MAIACC_TOKEN_BACKSLASH: return "BACKSLASH";
    case MAIACC_TOKEN_BACKTICK: return "BACKTICK";
    case MAIACC_TOKEN_LF: return "LF";
    case MAIACC_TOKEN_RAW: return "RAW";
    default: return "UNKNOWN";
  }
}

struct MaiaCCParserToken {
  unsigned int type;
  const char* value;
  unsigned int length;
  unsigned int start;
  unsigned int end;
};


typedef MaiaRegexInstruction MaiaCCLexInstruction;
typedef MaiaRegexRange MaiaCCLexRange;
struct MaiaCCLexPattern {
  const MaiaCCLexInstruction* code;
  const MaiaCCLexRange* ranges;
  unsigned int count;
  unsigned int entry;
  unsigned int token;
  int skip;
  int generic;
  int preferred;
  int role;
  unsigned int rangeCount;
};
struct MaiaCCLexerState {
  const char* input;
  unsigned int length;
  unsigned int position;
  unsigned int bytePosition;
  unsigned int tokenCount;
  int failed;
};
static void maiaCCLexerStateInit(MaiaCCLexerState* self, const char* input, unsigned int length) {
  self->input = input;
  self->length = length;
  self->position = 0;
  self->bytePosition = 0;
  self->tokenCount = 0;
  self->failed = 0;
}
static const MaiaCCLexInstruction maiaCCLexCode0[] = {
  { 0, 0, 0, 0 },
  { 1, 40, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges0[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode1[] = {
  { 0, 0, 0, 0 },
  { 1, 41, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges1[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode2[] = {
  { 0, 0, 0, 0 },
  { 1, 9, 0, 0 },
  { 1, 32, 0, 0 },
  { 2, 2, 1, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges2[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode3[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 },
  { 1, 60, 0, 0 },
  { 2, 2, 1, 0 },
  { 1, 59, 0, 0 },
  { 2, 4, 3, 0 },
  { 1, 38, 0, 0 },
  { 2, 6, 5, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges3[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode4[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 3, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges4[] = {
  { 65, 90 },
  { 97, 122 },
  { 95, 95 }
};
static const MaiaCCLexInstruction maiaCCLexCode5[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 1, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges5[] = {
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode6[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges6[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode7[] = {
  { 0, 0, 0, 0 },
  { 1, 35, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges7[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode8[] = {
  { 0, 0, 0, 0 },
  { 1, 123, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges8[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode9[] = {
  { 0, 0, 0, 0 },
  { 1, 125, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges9[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode10[] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 1, 123, 0, 1 },
  { 7, 2, 0, 0 },
  { 1, 36, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges10[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode11[] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 1, 40, 0, 1 },
  { 7, 2, 0, 0 },
  { 1, 36, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges11[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode12[] = {
  { 0, 0, 0, 0 },
  { 1, 36, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges12[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode13[] = {
  { 0, 0, 0, 0 },
  { 1, 58, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges13[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode14[] = {
  { 0, 0, 0, 0 },
  { 1, 45, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges14[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode15[] = {
  { 0, 0, 0, 0 },
  { 1, 43, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges15[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode16[] = {
  { 0, 0, 0, 0 },
  { 1, 37, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges16[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode17[] = {
  { 0, 0, 0, 0 },
  { 1, 63, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges17[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode18[] = {
  { 0, 0, 0, 0 },
  { 1, 39, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges18[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode19[] = {
  { 0, 0, 0, 0 },
  { 1, 34, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges19[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode20[] = {
  { 0, 0, 0, 0 },
  { 1, 92, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges20[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode21[] = {
  { 0, 0, 0, 0 },
  { 1, 96, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges21[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode22[] = {
  { 0, 0, 0, 0 },
  { 1, 10, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges22[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode23[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 1, 0 },
  { 3, 1, 1, 1 },
  { 3, 2, 1, 0 },
  { 3, 3, 1, 3 },
  { 2, 4, 2, 0 },
  { 3, 4, 1, 0 },
  { 3, 5, 1, 6 },
  { 2, 7, 5, 0 },
  { 3, 6, 1, 0 },
  { 2, 9, 8, 0 },
  { 3, 7, 1, 0 },
  { 2, 11, 10, 0 },
  { 3, 8, 1, 0 },
  { 2, 13, 12, 0 },
  { 1, 91, 0, 0 },
  { 2, 15, 14, 0 },
  { 1, 64, 0, 0 },
  { 2, 17, 16, 0 },
  { 3, 9, 1, 0 },
  { 2, 19, 18, 0 },
  { 1, 44, 0, 0 },
  { 2, 21, 20, 0 },
  { 1, 42, 0, 0 },
  { 2, 23, 22, 0 },
  { 1, 33, 0, 0 },
  { 2, 25, 24, 0 },
  { 3, 10, 1, 0 },
  { 2, 27, 26, 0 },
  { 3, 11, 1, 0 },
  { 2, 29, 28, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges23[] = {
  { 56320, 57343 },
  { 56319, 56319 },
  { 56320, 57343 },
  { 55297, 56318 },
  { 56320, 57343 },
  { 55296, 55296 },
  { 57344, 65535 },
  { 126, 55295 },
  { 93, 94 },
  { 46, 47 },
  { 11, 31 },
  { 0, 8 }
};
static const MaiaCCLexInstruction maiaCCLexCode24[] = {
  { 0, 0, 0, 0 },
  { 1, 40, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges24[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode25[] = {
  { 0, 0, 0, 0 },
  { 1, 41, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges25[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode26[] = {
  { 0, 0, 0, 0 },
  { 1, 9, 0, 0 },
  { 1, 32, 0, 0 },
  { 2, 2, 1, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges26[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode27[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 },
  { 1, 60, 0, 0 },
  { 2, 2, 1, 0 },
  { 1, 59, 0, 0 },
  { 2, 4, 3, 0 },
  { 1, 38, 0, 0 },
  { 2, 6, 5, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges27[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode28[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 3, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges28[] = {
  { 65, 90 },
  { 97, 122 },
  { 95, 95 }
};
static const MaiaCCLexInstruction maiaCCLexCode29[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 1, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges29[] = {
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode30[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges30[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode31[] = {
  { 0, 0, 0, 0 },
  { 1, 35, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges31[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode32[] = {
  { 0, 0, 0, 0 },
  { 1, 123, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges32[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode33[] = {
  { 0, 0, 0, 0 },
  { 1, 125, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges33[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode34[] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 1, 123, 0, 1 },
  { 7, 2, 0, 0 },
  { 1, 36, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges34[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode35[] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 1, 40, 0, 1 },
  { 7, 2, 0, 0 },
  { 1, 36, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges35[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode36[] = {
  { 0, 0, 0, 0 },
  { 1, 36, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges36[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode37[] = {
  { 0, 0, 0, 0 },
  { 1, 58, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges37[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode38[] = {
  { 0, 0, 0, 0 },
  { 1, 45, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges38[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode39[] = {
  { 0, 0, 0, 0 },
  { 1, 43, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges39[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode40[] = {
  { 0, 0, 0, 0 },
  { 1, 37, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges40[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode41[] = {
  { 0, 0, 0, 0 },
  { 1, 63, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges41[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode42[] = {
  { 0, 0, 0, 0 },
  { 1, 39, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges42[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode43[] = {
  { 0, 0, 0, 0 },
  { 1, 34, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges43[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode44[] = {
  { 0, 0, 0, 0 },
  { 1, 92, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges44[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode45[] = {
  { 0, 0, 0, 0 },
  { 1, 96, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges45[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode46[] = {
  { 0, 0, 0, 0 },
  { 1, 10, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges46[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode47[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 1, 0 },
  { 3, 1, 1, 1 },
  { 3, 2, 1, 0 },
  { 3, 3, 1, 3 },
  { 2, 4, 2, 0 },
  { 3, 4, 1, 0 },
  { 3, 5, 1, 6 },
  { 2, 7, 5, 0 },
  { 3, 6, 1, 0 },
  { 2, 9, 8, 0 },
  { 3, 7, 1, 0 },
  { 2, 11, 10, 0 },
  { 3, 8, 1, 0 },
  { 2, 13, 12, 0 },
  { 1, 91, 0, 0 },
  { 2, 15, 14, 0 },
  { 1, 64, 0, 0 },
  { 2, 17, 16, 0 },
  { 3, 9, 1, 0 },
  { 2, 19, 18, 0 },
  { 1, 44, 0, 0 },
  { 2, 21, 20, 0 },
  { 1, 42, 0, 0 },
  { 2, 23, 22, 0 },
  { 1, 33, 0, 0 },
  { 2, 25, 24, 0 },
  { 3, 10, 1, 0 },
  { 2, 27, 26, 0 },
  { 3, 11, 1, 0 },
  { 2, 29, 28, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges47[] = {
  { 56320, 57343 },
  { 56319, 56319 },
  { 56320, 57343 },
  { 55297, 56318 },
  { 56320, 57343 },
  { 55296, 55296 },
  { 57344, 65535 },
  { 126, 55295 },
  { 93, 94 },
  { 46, 47 },
  { 11, 31 },
  { 0, 8 }
};
static const MaiaCCLexPattern maiaCCLexPatterns[] = {
  { maiaCCLexCode0, maiaCCLexRanges0, 2, 1, MAIACC_TOKEN_CAPTURE_OPEN, 0, 0, 0, 0, 0 },
  { maiaCCLexCode1, maiaCCLexRanges1, 2, 1, MAIACC_TOKEN_CAPTURE_CLOSE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode2, maiaCCLexRanges2, 4, 3, MAIACC_TOKEN_CAPTURE_SPACE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode3, maiaCCLexRanges3, 8, 7, MAIACC_TOKEN_CAPTURE_CONTROL, 0, 0, 0, 0, 0 },
  { maiaCCLexCode4, maiaCCLexRanges4, 2, 1, MAIACC_TOKEN_LETTER, 0, 0, 0, 0, 3 },
  { maiaCCLexCode5, maiaCCLexRanges5, 2, 1, MAIACC_TOKEN_DIGIT, 0, 0, 0, 0, 1 },
  { maiaCCLexCode6, maiaCCLexRanges6, 2, 1, MAIACC_TOKEN_EQUAL, 0, 0, 0, 0, 0 },
  { maiaCCLexCode7, maiaCCLexRanges7, 2, 1, MAIACC_TOKEN_HASH, 0, 0, 0, 0, 0 },
  { maiaCCLexCode8, maiaCCLexRanges8, 2, 1, MAIACC_TOKEN_LBRACE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode9, maiaCCLexRanges9, 2, 1, MAIACC_TOKEN_RBRACE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode10, maiaCCLexRanges10, 5, 4, MAIACC_TOKEN_BRACED_DOLLAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode11, maiaCCLexRanges11, 5, 4, MAIACC_TOKEN_SUBSTITUTION_DOLLAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode12, maiaCCLexRanges12, 2, 1, MAIACC_TOKEN_DOLLAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode13, maiaCCLexRanges13, 2, 1, MAIACC_TOKEN_COLON, 0, 0, 0, 0, 0 },
  { maiaCCLexCode14, maiaCCLexRanges14, 2, 1, MAIACC_TOKEN_MINUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode15, maiaCCLexRanges15, 2, 1, MAIACC_TOKEN_PLUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode16, maiaCCLexRanges16, 2, 1, MAIACC_TOKEN_PERCENT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode17, maiaCCLexRanges17, 2, 1, MAIACC_TOKEN_QUESTION, 0, 0, 0, 0, 0 },
  { maiaCCLexCode18, maiaCCLexRanges18, 2, 1, MAIACC_TOKEN_SQUOTE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode19, maiaCCLexRanges19, 2, 1, MAIACC_TOKEN_DQUOTE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode20, maiaCCLexRanges20, 2, 1, MAIACC_TOKEN_BACKSLASH, 0, 0, 0, 0, 0 },
  { maiaCCLexCode21, maiaCCLexRanges21, 2, 1, MAIACC_TOKEN_BACKTICK, 0, 0, 0, 0, 0 },
  { maiaCCLexCode22, maiaCCLexRanges22, 2, 1, MAIACC_TOKEN_LF, 0, 0, 0, 0, 0 },
  { maiaCCLexCode23, maiaCCLexRanges23, 31, 30, MAIACC_TOKEN_RAW, 0, 0, 0, 0, 12 },
  { maiaCCLexCode24, maiaCCLexRanges24, 2, 1, MAIACC_TOKEN_CAPTURE_OPEN, 0, 0, 0, 0, 0 },
  { maiaCCLexCode25, maiaCCLexRanges25, 2, 1, MAIACC_TOKEN_CAPTURE_CLOSE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode26, maiaCCLexRanges26, 4, 3, MAIACC_TOKEN_CAPTURE_SPACE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode27, maiaCCLexRanges27, 8, 7, MAIACC_TOKEN_CAPTURE_CONTROL, 0, 0, 0, 0, 0 },
  { maiaCCLexCode28, maiaCCLexRanges28, 2, 1, MAIACC_TOKEN_LETTER, 0, 0, 0, 0, 3 },
  { maiaCCLexCode29, maiaCCLexRanges29, 2, 1, MAIACC_TOKEN_DIGIT, 0, 0, 0, 0, 1 },
  { maiaCCLexCode30, maiaCCLexRanges30, 2, 1, MAIACC_TOKEN_EQUAL, 0, 0, 0, 0, 0 },
  { maiaCCLexCode31, maiaCCLexRanges31, 2, 1, MAIACC_TOKEN_HASH, 0, 0, 0, 0, 0 },
  { maiaCCLexCode32, maiaCCLexRanges32, 2, 1, MAIACC_TOKEN_LBRACE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode33, maiaCCLexRanges33, 2, 1, MAIACC_TOKEN_RBRACE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode34, maiaCCLexRanges34, 5, 4, MAIACC_TOKEN_BRACED_DOLLAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode35, maiaCCLexRanges35, 5, 4, MAIACC_TOKEN_SUBSTITUTION_DOLLAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode36, maiaCCLexRanges36, 2, 1, MAIACC_TOKEN_DOLLAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode37, maiaCCLexRanges37, 2, 1, MAIACC_TOKEN_COLON, 0, 0, 0, 0, 0 },
  { maiaCCLexCode38, maiaCCLexRanges38, 2, 1, MAIACC_TOKEN_MINUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode39, maiaCCLexRanges39, 2, 1, MAIACC_TOKEN_PLUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode40, maiaCCLexRanges40, 2, 1, MAIACC_TOKEN_PERCENT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode41, maiaCCLexRanges41, 2, 1, MAIACC_TOKEN_QUESTION, 0, 0, 0, 0, 0 },
  { maiaCCLexCode42, maiaCCLexRanges42, 2, 1, MAIACC_TOKEN_SQUOTE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode43, maiaCCLexRanges43, 2, 1, MAIACC_TOKEN_DQUOTE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode44, maiaCCLexRanges44, 2, 1, MAIACC_TOKEN_BACKSLASH, 0, 0, 0, 0, 0 },
  { maiaCCLexCode45, maiaCCLexRanges45, 2, 1, MAIACC_TOKEN_BACKTICK, 0, 0, 0, 0, 0 },
  { maiaCCLexCode46, maiaCCLexRanges46, 2, 1, MAIACC_TOKEN_LF, 0, 0, 0, 0, 0 },
  { maiaCCLexCode47, maiaCCLexRanges47, 31, 30, MAIACC_TOKEN_RAW, 0, 0, 0, 0, 12 }
};
static const unsigned int maiaCCLexPatternCount = 24;
static const unsigned int maiaCCLexContextCount = 48;

static int maiaCCLexWhitespace(unsigned int value) {
  return (value >= 9 && value <= 13) || value == 32 || value == 160 || value == 5760
      || (value >= 8192 && value <= 8202) || value == 8232 || value == 8233
      || value == 8239 || value == 8287 || value == 12288 || value == 65279;
}

static int maiaCCLexDecode(const char* input, unsigned int length,
    unsigned int* units, unsigned int* offsets, unsigned int* count) {
  unsigned int position = 0;
  *count = 0;
  while (position < length) {
    unsigned int start = position;
    unsigned int first = (unsigned char)input[position++];
    unsigned int value = first;
    unsigned int extra = 0;
    unsigned int minimum = 0;
    if (first >= 194 && first <= 223) { value = first & 31; extra = 1; minimum = 128; }
    else if (first >= 224 && first <= 239) { value = first & 15; extra = 2; minimum = 2048; }
    else if (first >= 240 && first <= 244) { value = first & 7; extra = 3; minimum = 65536; }
    else if (first >= 128) return 0;
    if (extra > length - position) return 0;
    while (extra > 0) {
      unsigned int byte = (unsigned char)input[position++];
      if (byte < 128 || byte > 191) return 0;
      value = (value << 6) | (byte & 63);
      --extra;
    }
    if (value < minimum || value > 1114111 || (value >= 55296 && value <= 57343)) return 0;
    offsets[*count] = start;
    if (value > 65535) {
      value -= 65536;
      units[(*count)++] = 55296 + (value >> 10);
      offsets[*count] = start;
      units[(*count)++] = 56320 + (value & 1023);
    } else units[(*count)++] = value;
  }
  offsets[*count] = length;
  return 1;
}

static int maiaCCLexMatch(const MaiaCCLexPattern* pattern, const unsigned int* units,
    unsigned int length, unsigned int position) {
  MaiaRegexProgram program = { pattern->code, pattern->ranges, pattern->count, pattern->rangeCount, pattern->entry };
  unsigned int end = 0;
  int result = maia_regex_exec(&program, units, length, position, 0, &end);
  if (result == MAIA_REGEX_MATCH) return (int)end;
  return result == MAIA_REGEX_NOMATCH ? -1 : -2;
}

static int maiaCCLexerTokenize(MaiaCCLexerState* self, MaiaCCParserToken* tokens, unsigned int capacity) {
  self->tokenCount = 0;
  self->position = 0;
  self->bytePosition = 0;
  self->failed = 0;
  if (!self->input || !tokens || !capacity || self->length > 2147483646U) { self->failed = 1; return 0; }
  size_t entries = (size_t)self->length + 1;
  if (entries > ((size_t)-1) / (3 * sizeof(unsigned int))) { self->failed = 2; return 0; }
  unsigned int* storage = (unsigned int*)malloc(entries * 3 * sizeof(unsigned int));
  if (!storage) { self->failed = 2; return 0; }
  unsigned int* units = storage;
  unsigned int* offsets = storage + entries;
  unsigned int* braces = storage + 2 * entries;
  unsigned int length = 0;
  unsigned int classDepth = 0;
  unsigned int templateDepth = 0;
  if (!maiaCCLexDecode(self->input, self->length, units, offsets, &length)) self->failed = 1;
  int middle = -1;
  int tail = -1;
  for (unsigned int index = 0; index < maiaCCLexPatternCount; ++index) {
    if (maiaCCLexPatterns[index].role == 1) middle = (int)index;
    if (maiaCCLexPatterns[index].role == 2) tail = (int)index;
  }
  while (!self->failed && self->position < length) {
    int best = -1;
    int bestEnd = -1;
    int preferred = -1;
    int preferredEnd = -1;
    int whitespace = -1;
    int whitespaceEnd = -1;
    for (unsigned int index = 0; index < maiaCCLexPatternCount; ++index) {
      int candidate = (int)index;
      const MaiaCCLexPattern* pattern = maiaCCLexPatterns + candidate;
      if (pattern->role == 1 || pattern->role == 2) continue;
      int end = maiaCCLexMatch(pattern, units, length, self->position);
      if (end == -2) { self->failed = 2; break; }
      if (end <= (int)self->position) continue;
      if (templateDepth && pattern->role == 5 && braces[templateDepth - 1] == 0) {
        unsigned int limit = self->position + 256;
        if (limit > length) limit = length;
        for (unsigned int scan = self->position + 1; scan < limit; ++scan) {
          if (units[scan] == 92) { ++scan; continue; }
          int span = -1;
          if (units[scan] == 36 && scan + 1 < length && units[scan + 1] == 123) span = middle;
          else if (units[scan] == 96) span = tail;
          else continue;
          if (span >= 0) {
            int spanEnd = maiaCCLexMatch(maiaCCLexPatterns + span, units, length, self->position);
            if (spanEnd == -2) self->failed = 2;
            if (spanEnd > (int)self->position) { candidate = span; end = spanEnd; pattern = maiaCCLexPatterns + span; }
          }
          break;
        }
      }
      if (best < 0 || end > bestEnd || (end == bestEnd && pattern->skip && !maiaCCLexPatterns[best].skip)
          || (end == bestEnd && maiaCCLexPatterns[best].generic && !pattern->generic)) { best = candidate; bestEnd = end; }
      if (classDepth && pattern->preferred && end > preferredEnd) { preferred = candidate; preferredEnd = end; }
      if (pattern->skip && end > whitespaceEnd) { whitespace = candidate; whitespaceEnd = end; }
    }
    if (preferred >= 0) { best = preferred; bestEnd = preferredEnd; }
    if (whitespace >= 0 && maiaCCLexWhitespace(units[self->position])) { best = whitespace; bestEnd = whitespaceEnd; }
    if (self->failed) break;
    if (best < 0) { self->failed = 1; break; }
    const MaiaCCLexPattern* pattern = maiaCCLexPatterns + best;
    if (!pattern->skip) {
      if (self->tokenCount + 1 >= capacity) { self->failed = 3; break; }
      MaiaCCParserToken* token = tokens + self->tokenCount++;
      token->type = pattern->token;
      token->value = self->input + offsets[self->position];
      token->length = offsets[bestEnd] - offsets[self->position];
      token->start = self->position;
      token->end = (unsigned int)bestEnd;
      if (pattern->role == 6) ++classDepth;
      else if (pattern->role == 7 && classDepth) --classDepth;
      else if (pattern->role == 4 && templateDepth) ++braces[templateDepth - 1];
      else if (pattern->role == 5 && templateDepth && braces[templateDepth - 1]) --braces[templateDepth - 1];
      else if (pattern->role == 3) braces[templateDepth++] = 0;
      else if (pattern->role == 2 && templateDepth) --templateDepth;
    }
    self->position = (unsigned int)bestEnd;
    self->bytePosition = offsets[self->position];
  }
  if (!self->failed) {
    MaiaCCParserToken* token = tokens + self->tokenCount++;
    token->type = MAIACC_TOKEN_EOF;
    token->value = self->input + self->length;
    token->length = 0;
    token->start = length;
    token->end = length;
  }
  free(storage);
  return !self->failed;
}

static int maiaCCLexMatchesContextual(unsigned int expected, const MaiaCCParserToken* token) {
  size_t entries = (size_t)token->length + 1;
  if (entries > ((size_t)-1) / (2 * sizeof(unsigned int))) return 0;
  unsigned int* units = (unsigned int*)malloc(entries * 2 * sizeof(unsigned int));
  if (!units) return 0;
  unsigned int length = 0;
  int accepted = 0;
  if (maiaCCLexDecode(token->value, token->length, units, units + entries, &length)) {
    for (unsigned int index = maiaCCLexPatternCount; index < maiaCCLexContextCount; ++index) {
      if (maiaCCLexPatterns[index].token == expected) {
        accepted = maiaCCLexMatch(maiaCCLexPatterns + index, units, length, 0) == (int)length;
        break;
      }
    }
  }
  free(units);
  return accepted;
}


struct MaiaCCParseEvents {
  void* context;
  unsigned int (*checkpoint)(void*);
  void (*restore)(void*, unsigned int);
  void (*startNonterminal)(void*, const char*, unsigned int);
  void (*terminal)(void*, unsigned int, const char*, unsigned int, unsigned int);
  void (*endNonterminal)(void*, const char*, unsigned int);
  void (*abortNonterminal)(void*, const char*, unsigned int);
  int (*matchesContextual)(void*, unsigned int, const MaiaCCParserToken*);
};

#define MAIACC_PARSE_RESOURCE_NONE 0
#define MAIACC_PARSE_RESOURCE_NOMEM 1
#define MAIACC_PARSE_RESOURCE_LIMIT 2

struct MaiaCCParserState {
  const MaiaCCParserToken* tokens;
  unsigned int tokenCount;
  unsigned int position;
  const MaiaCCParseEvents* events;
  int failed;
  int resourceError;
  unsigned int errorCount;
  unsigned int errorPosition;
  unsigned int expectedToken;
  unsigned int foundToken;
  unsigned char* failureMemo;
};

static void maiaCCParserStateInit(
    MaiaCCParserState* self,
    const MaiaCCParserToken* tokens,
    unsigned int tokenCount,
    const MaiaCCParseEvents* events) {
  self->tokens = tokens;
  self->tokenCount = tokenCount;
  self->position = 0;
  self->events = events;
  self->failed = 0;
  self->resourceError = MAIACC_PARSE_RESOURCE_NONE;
  self->errorCount = 0;
  self->errorPosition = 0;
  self->expectedToken = 0;
  self->foundToken = 0;
  self->failureMemo = 0;
}

static const MaiaCCParserToken* maiaCC_peek(const MaiaCCParserState* self) {
  return self->position < self->tokenCount ? self->tokens + self->position : 0;
}

static void maiaCC_recordError(MaiaCCParserState* self, unsigned int expected) {
  if (self->errorCount == 0) {
    const MaiaCCParserToken* token = maiaCC_peek(self);
    self->errorPosition = self->position;
    self->expectedToken = expected;
    self->foundToken = token ? token->type : MAIACC_TOKEN_EOF;
  }
  ++self->errorCount;
}

static unsigned int maiaCC_checkpointEvents(const MaiaCCParserState* self) {
  return self->events && self->events->checkpoint
      ? self->events->checkpoint(self->events->context)
      : 0;
}

static void maiaCC_restoreEvents(const MaiaCCParserState* self, unsigned int mark) {
  if (self->events && self->events->restore) {
    self->events->restore(self->events->context, mark);
  }
}

static void maiaCC_startNonterminal(const MaiaCCParserState* self, const char* name) {
  if (self->events && self->events->startNonterminal) {
    self->events->startNonterminal(self->events->context, name, self->position);
  }
}

static void maiaCC_endNonterminal(const MaiaCCParserState* self, const char* name) {
  if (self->events && self->events->endNonterminal) {
    self->events->endNonterminal(self->events->context, name, self->position);
  }
}

static void maiaCC_abortNonterminal(const MaiaCCParserState* self, const char* name) {
  if (self->events && self->events->abortNonterminal) {
    self->events->abortNonterminal(self->events->context, name, self->position);
  }
}

static void maiaCC_emitTerminal(
    const MaiaCCParserState* self,
    const MaiaCCParserToken* token) {
  if (self->events && self->events->terminal && token) {
    self->events->terminal(
        self->events->context,
        token->type,
        token->value,
        token->length,
        self->position);
  }
}

static int maiaCC_matchToken(MaiaCCParserState* self, unsigned int expected) {
  if (self->failed) return 0;
  const MaiaCCParserToken* token = maiaCC_peek(self);
  if (!token || token->type != expected) return 0;
  maiaCC_emitTerminal(self, token);
  ++self->position;
  return 1;
}

static int maiaCC_matchContextualToken(MaiaCCParserState* self, unsigned int expected) {
  if (self->failed) return 0;
  const MaiaCCParserToken* token = maiaCC_peek(self);
  if (!token) { maiaCC_recordError(self, expected); return 0; }
  const int accepted = token->type == expected
      || maiaCCLexMatchesContextual(expected, token)
      || (self->events && self->events->matchesContextual
          && self->events->matchesContextual(self->events->context, expected, token));
  if (!accepted) { maiaCC_recordError(self, expected); return 0; }
  maiaCC_emitTerminal(self, token);
  ++self->position;
  return 1;
}

static int maiaCC_expectToken(MaiaCCParserState* self, unsigned int expected) {
  if (maiaCC_matchToken(self, expected)) return 1;
  maiaCC_recordError(self, expected);
  self->failed = 1;
  return 0;
}

static int maiaCC_expectContextualToken(MaiaCCParserState* self, unsigned int expected) {
  if (maiaCC_matchContextualToken(self, expected)) return 1;
  self->failed = 1;
  return 0;
}

static int maiaCC_parse_Input(MaiaCCParserState* self);
static int maiaCC_parse_AssignmentForm(MaiaCCParserState* self);
static int maiaCC_parse_AssignmentValue(MaiaCCParserState* self);
static int maiaCC_parse_BareName(MaiaCCParserState* self);
static int maiaCC_parse_WordForm(MaiaCCParserState* self);
static int maiaCC_parse_VariableName(MaiaCCParserState* self);
static int maiaCC_parse_Part(MaiaCCParserState* self);
static int maiaCC_parse_CommandSubstitution(MaiaCCParserState* self);
static int maiaCC_parse_CaptureCommand(MaiaCCParserState* self);
static int maiaCC_parse_CaptureWord(MaiaCCParserState* self);
static int maiaCC_parse_CaptureInitialPart(MaiaCCParserState* self);
static int maiaCC_parse_CapturePart(MaiaCCParserState* self);
static int maiaCC_parse_CaptureInitialLiteral(MaiaCCParserState* self);
static int maiaCC_parse_CaptureLiteral(MaiaCCParserState* self);
static int maiaCC_parse_CaptureDoubleQuoted(MaiaCCParserState* self);
static int maiaCC_parse_CaptureDoublePiece(MaiaCCParserState* self);
static int maiaCC_parse_CaptureDoubleLiteral(MaiaCCParserState* self);
static int maiaCC_parse_StatusParameter(MaiaCCParserState* self);
static int maiaCC_parse_NamedParameter(MaiaCCParserState* self);
static int maiaCC_parse_BracedStatusParameter(MaiaCCParserState* self);
static int maiaCC_parse_BracedNamedParameter(MaiaCCParserState* self);
static int maiaCC_parse_ParameterLength(MaiaCCParserState* self);
static int maiaCC_parse_PrefixParameter(MaiaCCParserState* self);
static int maiaCC_parse_PrefixOperator(MaiaCCParserState* self);
static int maiaCC_parse_LongestPrefixOperator(MaiaCCParserState* self);
static int maiaCC_parse_ShortestPrefixOperator(MaiaCCParserState* self);
static int maiaCC_parse_SuffixParameter(MaiaCCParserState* self);
static int maiaCC_parse_SuffixOperator(MaiaCCParserState* self);
static int maiaCC_parse_LongestSuffixOperator(MaiaCCParserState* self);
static int maiaCC_parse_ShortestSuffixOperator(MaiaCCParserState* self);
static int maiaCC_parse_PatternValue(MaiaCCParserState* self);
static int maiaCC_parse_DefaultParameter(MaiaCCParserState* self);
static int maiaCC_parse_DoubleDefaultParameter(MaiaCCParserState* self);
static int maiaCC_parse_DefaultOperator(MaiaCCParserState* self);
static int maiaCC_parse_DefaultNullOperator(MaiaCCParserState* self);
static int maiaCC_parse_DefaultUnsetOperator(MaiaCCParserState* self);
static int maiaCC_parse_AlternateNonemptyOperator(MaiaCCParserState* self);
static int maiaCC_parse_AlternateSetOperator(MaiaCCParserState* self);
static int maiaCC_parse_DefaultValue(MaiaCCParserState* self);
static int maiaCC_parse_DefaultPart(MaiaCCParserState* self);
static int maiaCC_parse_DefaultLiteral(MaiaCCParserState* self);
static int maiaCC_parse_DoubleDefaultValue(MaiaCCParserState* self);
static int maiaCC_parse_DoubleDefaultPart(MaiaCCParserState* self);
static int maiaCC_parse_DoubleDefaultLiteral(MaiaCCParserState* self);
static int maiaCC_parse_DefaultBraceEscape(MaiaCCParserState* self);
static int maiaCC_parse_FallbackQuoted(MaiaCCParserState* self);
static int maiaCC_parse_FallbackDoublePart(MaiaCCParserState* self);
static int maiaCC_parse_FallbackDoubleLiteral(MaiaCCParserState* self);
static int maiaCC_parse_SingleQuoted(MaiaCCParserState* self);
static int maiaCC_parse_SinglePiece(MaiaCCParserState* self);
static int maiaCC_parse_DoubleQuoted(MaiaCCParserState* self);
static int maiaCC_parse_DoublePiece(MaiaCCParserState* self);
static int maiaCC_parse_DoubleEscape(MaiaCCParserState* self);
static int maiaCC_parse_DoubleEscapedCharacter(MaiaCCParserState* self);
static int maiaCC_parse_DoubleSlash(MaiaCCParserState* self);
static int maiaCC_parse_DoubleLiteral(MaiaCCParserState* self);
static int maiaCC_parse_OpaqueBraced(MaiaCCParserState* self);
static int maiaCC_parse_CompleteOpaqueBraced(MaiaCCParserState* self);
static int maiaCC_parse_CompleteBracedPiece(MaiaCCParserState* self);
static int maiaCC_parse_CompleteBracedNested(MaiaCCParserState* self);
static int maiaCC_parse_OpaqueQuoted(MaiaCCParserState* self);
static int maiaCC_parse_OpaqueQuotedPiece(MaiaCCParserState* self);
static int maiaCC_parse_BracedPiece(MaiaCCParserState* self);
static int maiaCC_parse_BracedNested(MaiaCCParserState* self);
static int maiaCC_parse_Escape(MaiaCCParserState* self);
static int maiaCC_parse_EscapeMarker(MaiaCCParserState* self);
static int maiaCC_parse_EscapedCharacter(MaiaCCParserState* self);
static int maiaCC_parse_Plain(MaiaCCParserState* self);


static int maiaCC_parse(MaiaCCParserState* self) {
  self->position = 0;
  self->failed = 0;
  self->resourceError = MAIACC_PARSE_RESOURCE_NONE;
  self->errorCount = 0;
  size_t columns = (size_t)self->tokenCount + 1;
  if (columns == 0 || MAIACC_RULE_COUNT > ((size_t)-1) / columns) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_LIMIT; return 0; }
  self->failureMemo = (unsigned char*)calloc(MAIACC_RULE_COUNT, columns);
  if (!self->failureMemo) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_NOMEM; return 0; }
  int accepted = maiaCC_parse_Input(self) && !self->failed;
  if (accepted) accepted = self->position == self->tokenCount
      || (self->position + 1 == self->tokenCount
          && maiaCC_peek(self)->type == MAIACC_TOKEN_EOF);
  free(self->failureMemo);
  self->failureMemo = 0;
  return accepted;
}

static int maiaCC_parse_Input(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)0 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Input");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AssignmentForm(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BareName(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_WordForm(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Input");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Input");
  return 0;
}

static int maiaCC_parse_AssignmentForm(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)1 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "AssignmentForm");
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
    if (!self->failed && !maiaCC_parse_AssignmentValue(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EOF);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "AssignmentForm");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "AssignmentForm");
  return 0;
}

static int maiaCC_parse_AssignmentValue(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)2 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "AssignmentValue");
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Part(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "AssignmentValue");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "AssignmentValue");
  return 0;
}

static int maiaCC_parse_BareName(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)3 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BareName");
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EOF);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BareName");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BareName");
  return 0;
}

static int maiaCC_parse_WordForm(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)4 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "WordForm");
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Part(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EOF);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "WordForm");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "WordForm");
  return 0;
}

static int maiaCC_parse_VariableName(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)5 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "VariableName");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "VariableName");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "VariableName");
  return 0;
}

static int maiaCC_parse_Part(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)6 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Part");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CommandSubstitution(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_PrefixParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SuffixParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DefaultParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OpaqueBraced(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SingleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Escape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Plain(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Part");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Part");
  return 0;
}

static int maiaCC_parse_CommandSubstitution(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)7 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CommandSubstitution");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
    if (!self->failed && !maiaCC_parse_CaptureCommand(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CommandSubstitution");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CommandSubstitution");
  return 0;
}

static int maiaCC_parse_CaptureCommand(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)8 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureCommand");
    while (maiaCC_matchToken(self, MAIACC_TOKEN_CAPTURE_SPACE)) {}
    if (!self->failed && !maiaCC_parse_CaptureWord(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
    while (maiaCC_matchToken(self, MAIACC_TOKEN_CAPTURE_SPACE)) {}
    if (!self->failed && !maiaCC_parse_CaptureWord(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    while (maiaCC_matchToken(self, MAIACC_TOKEN_CAPTURE_SPACE)) {}
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureCommand");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureCommand");
  return 0;
}

static int maiaCC_parse_CaptureWord(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)9 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureWord");
    if (!self->failed && !maiaCC_parse_CaptureInitialPart(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_CapturePart(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureWord");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureWord");
  return 0;
}

static int maiaCC_parse_CaptureInitialPart(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)10 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureInitialPart");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SingleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CaptureDoubleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Escape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CaptureInitialLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureInitialPart");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureInitialPart");
  return 0;
}

static int maiaCC_parse_CapturePart(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)11 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CapturePart");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SingleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CaptureDoubleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Escape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CaptureLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CapturePart");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CapturePart");
  return 0;
}

static int maiaCC_parse_CaptureInitialLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)12 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureInitialLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureInitialLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureInitialLiteral");
  return 0;
}

static int maiaCC_parse_CaptureLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)13 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureLiteral");
  return 0;
}

static int maiaCC_parse_CaptureDoubleQuoted(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)14 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureDoubleQuoted");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_CaptureDoublePiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureDoubleQuoted");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureDoubleQuoted");
  return 0;
}

static int maiaCC_parse_CaptureDoublePiece(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)15 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureDoublePiece");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CaptureDoubleLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureDoublePiece");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureDoublePiece");
  return 0;
}

static int maiaCC_parse_CaptureDoubleLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)16 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CaptureDoubleLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CaptureDoubleLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CaptureDoubleLiteral");
  return 0;
}

static int maiaCC_parse_StatusParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)17 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "StatusParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "StatusParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "StatusParameter");
  return 0;
}

static int maiaCC_parse_NamedParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)18 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "NamedParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "NamedParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "NamedParameter");
  return 0;
}

static int maiaCC_parse_BracedStatusParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)19 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BracedStatusParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BracedStatusParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BracedStatusParameter");
  return 0;
}

static int maiaCC_parse_BracedNamedParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)20 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BracedNamedParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BracedNamedParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BracedNamedParameter");
  return 0;
}

static int maiaCC_parse_ParameterLength(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)21 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ParameterLength");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ParameterLength");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ParameterLength");
  return 0;
}

static int maiaCC_parse_PrefixParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)22 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "PrefixParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_PrefixOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_PatternValue(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "PrefixParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "PrefixParameter");
  return 0;
}

static int maiaCC_parse_PrefixOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)23 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "PrefixOperator");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_LongestPrefixOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ShortestPrefixOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "PrefixOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "PrefixOperator");
  return 0;
}

static int maiaCC_parse_LongestPrefixOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)24 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LongestPrefixOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "LongestPrefixOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LongestPrefixOperator");
  return 0;
}

static int maiaCC_parse_ShortestPrefixOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)25 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ShortestPrefixOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ShortestPrefixOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ShortestPrefixOperator");
  return 0;
}

static int maiaCC_parse_SuffixParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)26 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "SuffixParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_SuffixOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_PatternValue(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "SuffixParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "SuffixParameter");
  return 0;
}

static int maiaCC_parse_SuffixOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)27 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "SuffixOperator");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_LongestSuffixOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ShortestSuffixOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "SuffixOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "SuffixOperator");
  return 0;
}

static int maiaCC_parse_LongestSuffixOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)28 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LongestSuffixOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "LongestSuffixOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LongestSuffixOperator");
  return 0;
}

static int maiaCC_parse_ShortestSuffixOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)29 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ShortestSuffixOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ShortestSuffixOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ShortestSuffixOperator");
  return 0;
}

static int maiaCC_parse_PatternValue(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)30 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "PatternValue");
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_DefaultPart(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "PatternValue");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "PatternValue");
  return 0;
}

static int maiaCC_parse_DefaultParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)31 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_DefaultOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_DefaultValue(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultParameter");
  return 0;
}

static int maiaCC_parse_DoubleDefaultParameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)32 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleDefaultParameter");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    if (!self->failed && !maiaCC_parse_VariableName(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_DefaultOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_DoubleDefaultValue(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleDefaultParameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleDefaultParameter");
  return 0;
}

static int maiaCC_parse_DefaultOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)33 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultOperator");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DefaultNullOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DefaultUnsetOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AlternateNonemptyOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AlternateSetOperator(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultOperator");
  return 0;
}

static int maiaCC_parse_DefaultNullOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)34 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultNullOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultNullOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultNullOperator");
  return 0;
}

static int maiaCC_parse_DefaultUnsetOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)35 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultUnsetOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultUnsetOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultUnsetOperator");
  return 0;
}

static int maiaCC_parse_AlternateNonemptyOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)36 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "AlternateNonemptyOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "AlternateNonemptyOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "AlternateNonemptyOperator");
  return 0;
}

static int maiaCC_parse_AlternateSetOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)37 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "AlternateSetOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "AlternateSetOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "AlternateSetOperator");
  return 0;
}

static int maiaCC_parse_DefaultValue(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)38 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultValue");
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_DefaultPart(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultValue");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultValue");
  return 0;
}

static int maiaCC_parse_DefaultPart(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)39 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultPart");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SingleQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_FallbackQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Escape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DefaultLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultPart");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultPart");
  return 0;
}

static int maiaCC_parse_DefaultLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)40 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultLiteral");
  return 0;
}

static int maiaCC_parse_DoubleDefaultValue(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)41 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleDefaultValue");
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_DoubleDefaultPart(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleDefaultValue");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleDefaultValue");
  return 0;
}

static int maiaCC_parse_DoubleDefaultPart(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)42 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleDefaultPart");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_FallbackQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DefaultBraceEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleDefaultLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleDefaultPart");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleDefaultPart");
  return 0;
}

static int maiaCC_parse_DoubleDefaultLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)43 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleDefaultLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleDefaultLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleDefaultLiteral");
  return 0;
}

static int maiaCC_parse_DefaultBraceEscape(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)44 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultBraceEscape");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKSLASH);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultBraceEscape");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultBraceEscape");
  return 0;
}

static int maiaCC_parse_FallbackQuoted(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)45 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "FallbackQuoted");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_FallbackDoublePart(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "FallbackQuoted");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "FallbackQuoted");
  return 0;
}

static int maiaCC_parse_FallbackDoublePart(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)46 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "FallbackDoublePart");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_FallbackDoubleLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "FallbackDoublePart");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "FallbackDoublePart");
  return 0;
}

static int maiaCC_parse_FallbackDoubleLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)47 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "FallbackDoubleLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "FallbackDoubleLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "FallbackDoubleLiteral");
  return 0;
}

static int maiaCC_parse_SingleQuoted(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)48 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "SingleQuoted");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_SinglePiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "SingleQuoted");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "SingleQuoted");
  return 0;
}

static int maiaCC_parse_SinglePiece(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)49 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "SinglePiece");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKSLASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "SinglePiece");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "SinglePiece");
  return 0;
}

static int maiaCC_parse_DoubleQuoted(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)50 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleQuoted");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_DoublePiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleQuoted");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleQuoted");
  return 0;
}

static int maiaCC_parse_DoublePiece(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)51 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoublePiece");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CommandSubstitution(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_PrefixParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SuffixParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleDefaultParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterLength(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedStatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StatusParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamedParameter(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OpaqueBraced(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleLiteral(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoublePiece");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoublePiece");
  return 0;
}

static int maiaCC_parse_DoubleEscape(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)52 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleEscape");
    if (!self->failed && !maiaCC_parse_EscapeMarker(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_DoubleEscapedCharacter(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleEscape");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleEscape");
  return 0;
}

static int maiaCC_parse_DoubleEscapedCharacter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)53 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleEscapedCharacter");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKSLASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleEscapedCharacter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleEscapedCharacter");
  return 0;
}

static int maiaCC_parse_DoubleSlash(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)54 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleSlash");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKSLASH);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleSlash");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleSlash");
  return 0;
}

static int maiaCC_parse_DoubleLiteral(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)55 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoubleLiteral");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoubleLiteral");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoubleLiteral");
  return 0;
}

static int maiaCC_parse_OpaqueBraced(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)56 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "OpaqueBraced");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CompleteOpaqueBraced(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_BracedPiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    maiaCC_matchToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "OpaqueBraced");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "OpaqueBraced");
  return 0;
}

static int maiaCC_parse_CompleteOpaqueBraced(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)57 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CompleteOpaqueBraced");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_CompleteBracedPiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CompleteOpaqueBraced");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CompleteOpaqueBraced");
  return 0;
}

static int maiaCC_parse_CompleteBracedPiece(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)58 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CompleteBracedPiece");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_CompleteBracedNested(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OpaqueQuoted(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CompleteBracedPiece");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CompleteBracedPiece");
  return 0;
}

static int maiaCC_parse_CompleteBracedNested(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)59 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "CompleteBracedNested");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_CompleteBracedPiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "CompleteBracedNested");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "CompleteBracedNested");
  return 0;
}

static int maiaCC_parse_OpaqueQuoted(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)60 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "OpaqueQuoted");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_OpaqueQuotedPiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "OpaqueQuoted");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "OpaqueQuoted");
  return 0;
}

static int maiaCC_parse_OpaqueQuotedPiece(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)61 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "OpaqueQuotedPiece");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "OpaqueQuotedPiece");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "OpaqueQuotedPiece");
  return 0;
}

static int maiaCC_parse_BracedPiece(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)62 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BracedPiece");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BracedNested(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleEscape(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoubleSlash(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BracedPiece");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BracedPiece");
  return 0;
}

static int maiaCC_parse_BracedNested(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)63 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BracedNested");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_BracedPiece(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    maiaCC_matchToken(self, MAIACC_TOKEN_RBRACE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BracedNested");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BracedNested");
  return 0;
}

static int maiaCC_parse_Escape(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)64 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Escape");
    if (!self->failed && !maiaCC_parse_EscapeMarker(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_EscapedCharacter(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Escape");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Escape");
  return 0;
}

static int maiaCC_parse_EscapeMarker(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)65 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "EscapeMarker");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKSLASH);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "EscapeMarker");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "EscapeMarker");
  return 0;
}

static int maiaCC_parse_EscapedCharacter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)66 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "EscapedCharacter");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SUBSTITUTION_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DQUOTE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKSLASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "EscapedCharacter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "EscapedCharacter");
  return 0;
}

static int maiaCC_parse_Plain(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)67 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Plain");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CONTROL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CAPTURE_SPACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BRACED_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_HASH);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LETTER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DIGIT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RAW);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQUAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_RBRACE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DOLLAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_QUESTION);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_BACKTICK);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LF);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Plain");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Plain");
  return 0;
}

#endif
