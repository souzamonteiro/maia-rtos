#ifndef MAIACC_GENERATED_CPP98_PARSER_H
#define MAIACC_GENERATED_CPP98_PARSER_H

#include <stdlib.h>
#include "maia_regex.h"

#define MAIACC_TOKEN_EOF 1
#define MAIACC_TOKEN_OPEN 2
#define MAIACC_TOKEN_CLOSE 3
#define MAIACC_TOKEN_OR 4
#define MAIACC_TOKEN_AND 5
#define MAIACC_TOKEN_EQ 6
#define MAIACC_TOKEN_NE 7
#define MAIACC_TOKEN_LE 8
#define MAIACC_TOKEN_GE 9
#define MAIACC_TOKEN_LT 10
#define MAIACC_TOKEN_GT 11
#define MAIACC_TOKEN_PLUS 12
#define MAIACC_TOKEN_MINUS 13
#define MAIACC_TOKEN_STAR 14
#define MAIACC_TOKEN_SLASH 15
#define MAIACC_TOKEN_PERCENT 16
#define MAIACC_TOKEN_COLON 17
#define MAIACC_TOKEN_DECIMAL 18
#define MAIACC_TOKEN_VALUE 19
#define MAIACC_TOKEN_COUNT 20
#define MAIACC_RULE_COUNT 58

static const char* maiaCCParserTokenName(unsigned int type) {
  switch (type) {
    case MAIACC_TOKEN_EOF: return "EOF";
    case MAIACC_TOKEN_OPEN: return "OPEN";
    case MAIACC_TOKEN_CLOSE: return "CLOSE";
    case MAIACC_TOKEN_OR: return "OR";
    case MAIACC_TOKEN_AND: return "AND";
    case MAIACC_TOKEN_EQ: return "EQ";
    case MAIACC_TOKEN_NE: return "NE";
    case MAIACC_TOKEN_LE: return "LE";
    case MAIACC_TOKEN_GE: return "GE";
    case MAIACC_TOKEN_LT: return "LT";
    case MAIACC_TOKEN_GT: return "GT";
    case MAIACC_TOKEN_PLUS: return "PLUS";
    case MAIACC_TOKEN_MINUS: return "MINUS";
    case MAIACC_TOKEN_STAR: return "STAR";
    case MAIACC_TOKEN_SLASH: return "SLASH";
    case MAIACC_TOKEN_PERCENT: return "PERCENT";
    case MAIACC_TOKEN_COLON: return "COLON";
    case MAIACC_TOKEN_DECIMAL: return "DECIMAL";
    case MAIACC_TOKEN_VALUE: return "VALUE";
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
  { 1, 0, 0, 0 },
  { 1, 40, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges0[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode1[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 41, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges1[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode2[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 124, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges2[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode3[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 38, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges3[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode4[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges4[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode5[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 },
  { 1, 33, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges5[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode6[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 },
  { 1, 60, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges6[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode7[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 },
  { 1, 62, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges7[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode8[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 60, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges8[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode9[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 62, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges9[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode10[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 43, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges10[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode11[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges11[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode12[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 42, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges12[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode13[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 47, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges13[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode14[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 37, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges14[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode15[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 58, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges15[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode16[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 2, 3, 1, 0 },
  { 3, 0, 1, 2 },
  { 1, 45, 0, 3 },
  { 2, 4, 3, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges16[] = {
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode17[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 2, 3, 1, 0 },
  { 4, 0, 1, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges17[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode18[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 40, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges18[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode19[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 41, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges19[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode20[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 124, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges20[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode21[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 38, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges21[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode22[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges22[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode23[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 },
  { 1, 33, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges23[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode24[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 },
  { 1, 60, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges24[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode25[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 61, 0, 1 },
  { 1, 62, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges25[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode26[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 60, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges26[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode27[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 62, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges27[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode28[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 43, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges28[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode29[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges29[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode30[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 42, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges30[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode31[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 47, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges31[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode32[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 37, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges32[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode33[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 1, 58, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges33[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode34[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 2, 3, 1, 0 },
  { 3, 0, 1, 2 },
  { 1, 45, 0, 3 },
  { 2, 4, 3, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges34[] = {
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode35[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 2, 3, 1, 0 },
  { 4, 0, 1, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges35[] = {
  { 0, 0 }
};
static const MaiaCCLexPattern maiaCCLexPatterns[] = {
  { maiaCCLexCode0, maiaCCLexRanges0, 3, 2, MAIACC_TOKEN_OPEN, 0, 0, 0, 0, 0 },
  { maiaCCLexCode1, maiaCCLexRanges1, 3, 2, MAIACC_TOKEN_CLOSE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode2, maiaCCLexRanges2, 3, 2, MAIACC_TOKEN_OR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode3, maiaCCLexRanges3, 3, 2, MAIACC_TOKEN_AND, 0, 0, 0, 0, 0 },
  { maiaCCLexCode4, maiaCCLexRanges4, 3, 2, MAIACC_TOKEN_EQ, 0, 0, 0, 0, 0 },
  { maiaCCLexCode5, maiaCCLexRanges5, 4, 3, MAIACC_TOKEN_NE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode6, maiaCCLexRanges6, 4, 3, MAIACC_TOKEN_LE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode7, maiaCCLexRanges7, 4, 3, MAIACC_TOKEN_GE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode8, maiaCCLexRanges8, 3, 2, MAIACC_TOKEN_LT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode9, maiaCCLexRanges9, 3, 2, MAIACC_TOKEN_GT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode10, maiaCCLexRanges10, 3, 2, MAIACC_TOKEN_PLUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode11, maiaCCLexRanges11, 3, 2, MAIACC_TOKEN_MINUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode12, maiaCCLexRanges12, 3, 2, MAIACC_TOKEN_STAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode13, maiaCCLexRanges13, 3, 2, MAIACC_TOKEN_SLASH, 0, 0, 0, 0, 0 },
  { maiaCCLexCode14, maiaCCLexRanges14, 3, 2, MAIACC_TOKEN_PERCENT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode15, maiaCCLexRanges15, 3, 2, MAIACC_TOKEN_COLON, 0, 0, 0, 0, 0 },
  { maiaCCLexCode16, maiaCCLexRanges16, 6, 5, MAIACC_TOKEN_DECIMAL, 0, 0, 0, 0, 1 },
  { maiaCCLexCode17, maiaCCLexRanges17, 4, 2, MAIACC_TOKEN_VALUE, 0, 0, 0, 0, 1 },
  { maiaCCLexCode18, maiaCCLexRanges18, 3, 2, MAIACC_TOKEN_OPEN, 0, 0, 0, 0, 0 },
  { maiaCCLexCode19, maiaCCLexRanges19, 3, 2, MAIACC_TOKEN_CLOSE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode20, maiaCCLexRanges20, 3, 2, MAIACC_TOKEN_OR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode21, maiaCCLexRanges21, 3, 2, MAIACC_TOKEN_AND, 0, 0, 0, 0, 0 },
  { maiaCCLexCode22, maiaCCLexRanges22, 3, 2, MAIACC_TOKEN_EQ, 0, 0, 0, 0, 0 },
  { maiaCCLexCode23, maiaCCLexRanges23, 4, 3, MAIACC_TOKEN_NE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode24, maiaCCLexRanges24, 4, 3, MAIACC_TOKEN_LE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode25, maiaCCLexRanges25, 4, 3, MAIACC_TOKEN_GE, 0, 0, 0, 0, 0 },
  { maiaCCLexCode26, maiaCCLexRanges26, 3, 2, MAIACC_TOKEN_LT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode27, maiaCCLexRanges27, 3, 2, MAIACC_TOKEN_GT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode28, maiaCCLexRanges28, 3, 2, MAIACC_TOKEN_PLUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode29, maiaCCLexRanges29, 3, 2, MAIACC_TOKEN_MINUS, 0, 0, 0, 0, 0 },
  { maiaCCLexCode30, maiaCCLexRanges30, 3, 2, MAIACC_TOKEN_STAR, 0, 0, 0, 0, 0 },
  { maiaCCLexCode31, maiaCCLexRanges31, 3, 2, MAIACC_TOKEN_SLASH, 0, 0, 0, 0, 0 },
  { maiaCCLexCode32, maiaCCLexRanges32, 3, 2, MAIACC_TOKEN_PERCENT, 0, 0, 0, 0, 0 },
  { maiaCCLexCode33, maiaCCLexRanges33, 3, 2, MAIACC_TOKEN_COLON, 0, 0, 0, 0, 0 },
  { maiaCCLexCode34, maiaCCLexRanges34, 6, 5, MAIACC_TOKEN_DECIMAL, 0, 0, 0, 0, 1 },
  { maiaCCLexCode35, maiaCCLexRanges35, 4, 2, MAIACC_TOKEN_VALUE, 0, 0, 0, 0, 1 }
};
static const unsigned int maiaCCLexPatternCount = 18;
static const unsigned int maiaCCLexContextCount = 36;

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
static int maiaCC_parse_Expression0(MaiaCCParserState* self);
static int maiaCC_parse_Or0(MaiaCCParserState* self);
static int maiaCC_parse_And0(MaiaCCParserState* self);
static int maiaCC_parse_Relation0(MaiaCCParserState* self);
static int maiaCC_parse_Sum0(MaiaCCParserState* self);
static int maiaCC_parse_Product0(MaiaCCParserState* self);
static int maiaCC_parse_Match0(MaiaCCParserState* self);
static int maiaCC_parse_Primary0(MaiaCCParserState* self);
static int maiaCC_parse_Expression1(MaiaCCParserState* self);
static int maiaCC_parse_Or1(MaiaCCParserState* self);
static int maiaCC_parse_And1(MaiaCCParserState* self);
static int maiaCC_parse_Relation1(MaiaCCParserState* self);
static int maiaCC_parse_Sum1(MaiaCCParserState* self);
static int maiaCC_parse_Product1(MaiaCCParserState* self);
static int maiaCC_parse_Match1(MaiaCCParserState* self);
static int maiaCC_parse_Primary1(MaiaCCParserState* self);
static int maiaCC_parse_Expression2(MaiaCCParserState* self);
static int maiaCC_parse_Or2(MaiaCCParserState* self);
static int maiaCC_parse_And2(MaiaCCParserState* self);
static int maiaCC_parse_Relation2(MaiaCCParserState* self);
static int maiaCC_parse_Sum2(MaiaCCParserState* self);
static int maiaCC_parse_Product2(MaiaCCParserState* self);
static int maiaCC_parse_Match2(MaiaCCParserState* self);
static int maiaCC_parse_Primary2(MaiaCCParserState* self);
static int maiaCC_parse_Expression3(MaiaCCParserState* self);
static int maiaCC_parse_Or3(MaiaCCParserState* self);
static int maiaCC_parse_And3(MaiaCCParserState* self);
static int maiaCC_parse_Relation3(MaiaCCParserState* self);
static int maiaCC_parse_Sum3(MaiaCCParserState* self);
static int maiaCC_parse_Product3(MaiaCCParserState* self);
static int maiaCC_parse_Match3(MaiaCCParserState* self);
static int maiaCC_parse_Primary3(MaiaCCParserState* self);
static int maiaCC_parse_Expression4(MaiaCCParserState* self);
static int maiaCC_parse_Or4(MaiaCCParserState* self);
static int maiaCC_parse_And4(MaiaCCParserState* self);
static int maiaCC_parse_Relation4(MaiaCCParserState* self);
static int maiaCC_parse_Sum4(MaiaCCParserState* self);
static int maiaCC_parse_Product4(MaiaCCParserState* self);
static int maiaCC_parse_Match4(MaiaCCParserState* self);
static int maiaCC_parse_Primary4(MaiaCCParserState* self);
static int maiaCC_parse_DepthOverflow(MaiaCCParserState* self);
static int maiaCC_parse_OrOperator(MaiaCCParserState* self);
static int maiaCC_parse_AndOperator(MaiaCCParserState* self);
static int maiaCC_parse_Equal(MaiaCCParserState* self);
static int maiaCC_parse_Unequal(MaiaCCParserState* self);
static int maiaCC_parse_Less(MaiaCCParserState* self);
static int maiaCC_parse_LessEqual(MaiaCCParserState* self);
static int maiaCC_parse_Greater(MaiaCCParserState* self);
static int maiaCC_parse_GreaterEqual(MaiaCCParserState* self);
static int maiaCC_parse_Add(MaiaCCParserState* self);
static int maiaCC_parse_Subtract(MaiaCCParserState* self);
static int maiaCC_parse_Multiply(MaiaCCParserState* self);
static int maiaCC_parse_Divide(MaiaCCParserState* self);
static int maiaCC_parse_Remainder(MaiaCCParserState* self);
static int maiaCC_parse_MatchOperator(MaiaCCParserState* self);
static int maiaCC_parse_Value(MaiaCCParserState* self);
static int maiaCC_parse_Pattern(MaiaCCParserState* self);


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
    if (!self->failed && !maiaCC_parse_Expression0(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EOF);
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

static int maiaCC_parse_Expression0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)1 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Expression0");
    if (!self->failed && !maiaCC_parse_Or0(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Expression0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Expression0");
  return 0;
}

static int maiaCC_parse_Or0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)2 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Or0");
    if (!self->failed && !maiaCC_parse_And0(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OrOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_And0(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Or0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Or0");
  return 0;
}

static int maiaCC_parse_And0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)3 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "And0");
    if (!self->failed && !maiaCC_parse_Relation0(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AndOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Relation0(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "And0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "And0");
  return 0;
}

static int maiaCC_parse_Relation0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)4 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Relation0");
    if (!self->failed && !maiaCC_parse_Sum0(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Equal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Unequal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Less(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_LessEqual(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Greater(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_GreaterEqual(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Sum0(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Relation0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Relation0");
  return 0;
}

static int maiaCC_parse_Sum0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)5 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Sum0");
    if (!self->failed && !maiaCC_parse_Product0(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Add(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Subtract(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Product0(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Sum0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Sum0");
  return 0;
}

static int maiaCC_parse_Product0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)6 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Product0");
    if (!self->failed && !maiaCC_parse_Match0(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Multiply(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Divide(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Remainder(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Match0(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Product0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Product0");
  return 0;
}

static int maiaCC_parse_Match0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)7 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Match0");
    if (!self->failed && !maiaCC_parse_Primary0(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_MatchOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Pattern(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Match0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Match0");
  return 0;
}

static int maiaCC_parse_Primary0(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)8 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Primary0");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OPEN);
    if (!self->failed && !maiaCC_parse_Expression1(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Value(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "Primary0");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Primary0");
  return 0;
}

static int maiaCC_parse_Expression1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)9 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Expression1");
    if (!self->failed && !maiaCC_parse_Or1(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Expression1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Expression1");
  return 0;
}

static int maiaCC_parse_Or1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)10 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Or1");
    if (!self->failed && !maiaCC_parse_And1(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OrOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_And1(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Or1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Or1");
  return 0;
}

static int maiaCC_parse_And1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)11 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "And1");
    if (!self->failed && !maiaCC_parse_Relation1(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AndOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Relation1(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "And1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "And1");
  return 0;
}

static int maiaCC_parse_Relation1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)12 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Relation1");
    if (!self->failed && !maiaCC_parse_Sum1(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Equal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Unequal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Less(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_LessEqual(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Greater(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_GreaterEqual(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Sum1(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Relation1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Relation1");
  return 0;
}

static int maiaCC_parse_Sum1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)13 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Sum1");
    if (!self->failed && !maiaCC_parse_Product1(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Add(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Subtract(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Product1(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Sum1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Sum1");
  return 0;
}

static int maiaCC_parse_Product1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)14 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Product1");
    if (!self->failed && !maiaCC_parse_Match1(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Multiply(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Divide(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Remainder(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Match1(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Product1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Product1");
  return 0;
}

static int maiaCC_parse_Match1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)15 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Match1");
    if (!self->failed && !maiaCC_parse_Primary1(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_MatchOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Pattern(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Match1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Match1");
  return 0;
}

static int maiaCC_parse_Primary1(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)16 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Primary1");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OPEN);
    if (!self->failed && !maiaCC_parse_Expression2(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Value(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "Primary1");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Primary1");
  return 0;
}

static int maiaCC_parse_Expression2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)17 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Expression2");
    if (!self->failed && !maiaCC_parse_Or2(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Expression2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Expression2");
  return 0;
}

static int maiaCC_parse_Or2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)18 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Or2");
    if (!self->failed && !maiaCC_parse_And2(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OrOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_And2(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Or2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Or2");
  return 0;
}

static int maiaCC_parse_And2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)19 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "And2");
    if (!self->failed && !maiaCC_parse_Relation2(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AndOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Relation2(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "And2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "And2");
  return 0;
}

static int maiaCC_parse_Relation2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)20 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Relation2");
    if (!self->failed && !maiaCC_parse_Sum2(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Equal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Unequal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Less(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_LessEqual(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Greater(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_GreaterEqual(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Sum2(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Relation2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Relation2");
  return 0;
}

static int maiaCC_parse_Sum2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)21 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Sum2");
    if (!self->failed && !maiaCC_parse_Product2(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Add(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Subtract(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Product2(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Sum2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Sum2");
  return 0;
}

static int maiaCC_parse_Product2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)22 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Product2");
    if (!self->failed && !maiaCC_parse_Match2(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Multiply(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Divide(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Remainder(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Match2(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Product2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Product2");
  return 0;
}

static int maiaCC_parse_Match2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)23 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Match2");
    if (!self->failed && !maiaCC_parse_Primary2(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_MatchOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Pattern(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Match2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Match2");
  return 0;
}

static int maiaCC_parse_Primary2(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)24 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Primary2");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OPEN);
    if (!self->failed && !maiaCC_parse_Expression3(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Value(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "Primary2");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Primary2");
  return 0;
}

static int maiaCC_parse_Expression3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)25 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Expression3");
    if (!self->failed && !maiaCC_parse_Or3(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Expression3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Expression3");
  return 0;
}

static int maiaCC_parse_Or3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)26 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Or3");
    if (!self->failed && !maiaCC_parse_And3(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OrOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_And3(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Or3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Or3");
  return 0;
}

static int maiaCC_parse_And3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)27 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "And3");
    if (!self->failed && !maiaCC_parse_Relation3(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AndOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Relation3(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "And3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "And3");
  return 0;
}

static int maiaCC_parse_Relation3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)28 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Relation3");
    if (!self->failed && !maiaCC_parse_Sum3(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Equal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Unequal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Less(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_LessEqual(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Greater(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_GreaterEqual(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Sum3(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Relation3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Relation3");
  return 0;
}

static int maiaCC_parse_Sum3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)29 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Sum3");
    if (!self->failed && !maiaCC_parse_Product3(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Add(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Subtract(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Product3(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Sum3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Sum3");
  return 0;
}

static int maiaCC_parse_Product3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)30 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Product3");
    if (!self->failed && !maiaCC_parse_Match3(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Multiply(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Divide(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Remainder(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Match3(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Product3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Product3");
  return 0;
}

static int maiaCC_parse_Match3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)31 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Match3");
    if (!self->failed && !maiaCC_parse_Primary3(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_MatchOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Pattern(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Match3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Match3");
  return 0;
}

static int maiaCC_parse_Primary3(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)32 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Primary3");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OPEN);
    if (!self->failed && !maiaCC_parse_Expression4(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Value(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "Primary3");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Primary3");
  return 0;
}

static int maiaCC_parse_Expression4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)33 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Expression4");
    if (!self->failed && !maiaCC_parse_Or4(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Expression4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Expression4");
  return 0;
}

static int maiaCC_parse_Or4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)34 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Or4");
    if (!self->failed && !maiaCC_parse_And4(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_OrOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_And4(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Or4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Or4");
  return 0;
}

static int maiaCC_parse_And4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)35 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "And4");
    if (!self->failed && !maiaCC_parse_Relation4(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AndOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Relation4(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "And4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "And4");
  return 0;
}

static int maiaCC_parse_Relation4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)36 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Relation4");
    if (!self->failed && !maiaCC_parse_Sum4(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Equal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Unequal(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Less(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_LessEqual(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Greater(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_GreaterEqual(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Sum4(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Relation4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Relation4");
  return 0;
}

static int maiaCC_parse_Sum4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)37 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Sum4");
    if (!self->failed && !maiaCC_parse_Product4(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Add(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Subtract(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Product4(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Sum4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Sum4");
  return 0;
}

static int maiaCC_parse_Product4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)38 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Product4");
    if (!self->failed && !maiaCC_parse_Match4(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Multiply(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Divide(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Remainder(self)) self->failed = 1;
        if (!self->failed) {
          _matchedAlternative = 1;
        } else {
          self->position = _alternativeStart;
          maiaCC_restoreEvents(self, _eventMark);
          self->failed = 0;
        }
      }
      if (!_matchedAlternative) self->failed = 1;
    }
    if (!self->failed && !maiaCC_parse_Match4(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Product4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Product4");
  return 0;
}

static int maiaCC_parse_Match4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)39 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Match4");
    if (!self->failed && !maiaCC_parse_Primary4(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_MatchOperator(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Pattern(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Match4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Match4");
  return 0;
}

static int maiaCC_parse_Primary4(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)40 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Primary4");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Value(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DepthOverflow(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "Primary4");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Primary4");
  return 0;
}

static int maiaCC_parse_DepthOverflow(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)41 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DepthOverflow");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OPEN);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DepthOverflow");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DepthOverflow");
  return 0;
}

static int maiaCC_parse_OrOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)42 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "OrOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OR);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "OrOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "OrOperator");
  return 0;
}

static int maiaCC_parse_AndOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)43 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "AndOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_AND);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "AndOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "AndOperator");
  return 0;
}

static int maiaCC_parse_Equal(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)44 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Equal");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQ);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Equal");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Equal");
  return 0;
}

static int maiaCC_parse_Unequal(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)45 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Unequal");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Unequal");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Unequal");
  return 0;
}

static int maiaCC_parse_Less(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)46 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Less");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LT);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Less");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Less");
  return 0;
}

static int maiaCC_parse_LessEqual(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)47 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LessEqual");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "LessEqual");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LessEqual");
  return 0;
}

static int maiaCC_parse_Greater(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)48 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Greater");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_GT);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Greater");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Greater");
  return 0;
}

static int maiaCC_parse_GreaterEqual(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)49 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "GreaterEqual");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_GE);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "GreaterEqual");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "GreaterEqual");
  return 0;
}

static int maiaCC_parse_Add(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)50 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Add");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PLUS);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Add");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Add");
  return 0;
}

static int maiaCC_parse_Subtract(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)51 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Subtract");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_MINUS);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Subtract");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Subtract");
  return 0;
}

static int maiaCC_parse_Multiply(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)52 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Multiply");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_STAR);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Multiply");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Multiply");
  return 0;
}

static int maiaCC_parse_Divide(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)53 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Divide");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SLASH);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Divide");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Divide");
  return 0;
}

static int maiaCC_parse_Remainder(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)54 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Remainder");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_PERCENT);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Remainder");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Remainder");
  return 0;
}

static int maiaCC_parse_MatchOperator(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)55 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "MatchOperator");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_COLON);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "MatchOperator");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "MatchOperator");
  return 0;
}

static int maiaCC_parse_Value(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)56 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Value");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DECIMAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_VALUE);
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
    maiaCC_endNonterminal(self, "Value");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Value");
  return 0;
}

static int maiaCC_parse_Pattern(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)57 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Pattern");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OPEN);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CLOSE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_OR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_AND);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EQ);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_LE);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_GT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_GE);
      if (!self->failed) {
        _matched = 1;
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_STAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_SLASH);
      if (!self->failed) {
        _matched = 1;
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DECIMAL);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_VALUE);
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
    maiaCC_endNonterminal(self, "Pattern");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Pattern");
  return 0;
}

#endif
