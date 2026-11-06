#ifndef MAIACC_GENERATED_CPP98_PARSER_H
#define MAIACC_GENERATED_CPP98_PARSER_H

#include <stdlib.h>
#include "maia_regex.h"

#define MAIACC_TOKEN_EOF 1
#define MAIACC_TOKEN_DECIMAL 2
#define MAIACC_TOKEN_COUNT 3
#define MAIACC_RULE_COUNT 2

static const char* maiaCCParserTokenName(unsigned int type) {
  switch (type) {
    case MAIACC_TOKEN_EOF: return "EOF";
    case MAIACC_TOKEN_DECIMAL: return "DECIMAL";
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
  { 2, 3, 1, 0 },
  { 3, 0, 1, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges0[] = {
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode1[] = {
  { 0, 0, 0, 0 },
  { 1, 0, 0, 0 },
  { 2, 3, 1, 0 },
  { 3, 0, 1, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges1[] = {
  { 48, 57 }
};
static const MaiaCCLexPattern maiaCCLexPatterns[] = {
  { maiaCCLexCode0, maiaCCLexRanges0, 4, 3, MAIACC_TOKEN_DECIMAL, 0, 0, 0, 0, 1 },
  { maiaCCLexCode1, maiaCCLexRanges1, 4, 3, MAIACC_TOKEN_DECIMAL, 0, 0, 0, 0, 1 }
};
static const unsigned int maiaCCLexPatternCount = 1;
static const unsigned int maiaCCLexContextCount = 2;

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
static int maiaCC_parse_Status(MaiaCCParserState* self);


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
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Status(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
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

static int maiaCC_parse_Status(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)1 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Status");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_DECIMAL);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Status");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Status");
  return 0;
}

#endif
