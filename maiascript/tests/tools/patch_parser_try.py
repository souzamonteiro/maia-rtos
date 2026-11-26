#!/usr/bin/env python3
import sys

def replace_required(content, old, new):
    if new in content:
        return content
    if old in content:
        return content.replace(old, new)
    raise RuntimeError("generated parser layout changed; expected fragment not found")

def main():
    if len(sys.argv) < 2:
        return
    path = sys.argv[1]
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    ternary_func = '''
static int isTernaryQuestion(MaiaCCParserState* self) {
    if (self->position >= self->tokenCount) return 0;
    if (self->tokens[self->position].type != MAIACC_TOKEN_TOKEN__3F_) return 0;
    int depth = 0;
    for (unsigned int i = self->position + 1; i < self->tokenCount; ++i) {
        unsigned int t = self->tokens[i].type;
        if (t == MAIACC_TOKEN_TOKEN__28_ || t == MAIACC_TOKEN_TOKEN__7B_ || t == MAIACC_TOKEN_TOKEN__5B_) depth++;
        else if (t == MAIACC_TOKEN_TOKEN__29_ || t == MAIACC_TOKEN_TOKEN__7D_ || t == MAIACC_TOKEN_TOKEN__5D_) {
            if (depth == 0) break;
            depth--;
        } else if (depth == 0) {
            if (t == MAIACC_TOKEN_TOKEN__3A_) return 1;
            if (t == MAIACC_TOKEN_TOKEN__3B_ || t == MAIACC_TOKEN_TOKEN__2C_ || t == MAIACC_TOKEN_EOF) break;
        }
    }
    return 0;
}
'''
    if 'isTernaryQuestion' not in content and 'MAIACC_TOKEN_TOKEN__3F_' in content:
        content = content.replace('static int maiaCC_parse_PostfixTail(MaiaCCParserState* self);',
                                  'static int isTernaryQuestion(MaiaCCParserState* self);\nstatic int maiaCC_parse_PostfixTail(MaiaCCParserState* self);')
        content = content.replace('static int maiaCC_parse_Program(MaiaCCParserState* self) {',
                                  ternary_func + '\nstatic int maiaCC_parse_Program(MaiaCCParserState* self) {')
        old_match = '''    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3F_);'''
        new_match = '''    if (!_matched && !self->failed && !isTernaryQuestion(self)) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3F_);'''
        content = content.replace(old_match, new_match)
        with open(path, "w", encoding="utf-8") as f:
            f.write(content)

    content = replace_required(
        content,
        '''struct MaiaCCParserToken {
  unsigned int type;
  const char* value;
  unsigned int length;
  unsigned int start;
  unsigned int end;
};''',
        '''struct MaiaCCParserToken {
  unsigned int type;
  const char* value;
  unsigned int length;
};''')
    content = replace_required(
        content,
        '''  if (!self->input || !tokens || !capacity || self->length > 2147483646U) { self->failed = 1; return 0; }
  size_t entries = (size_t)self->length + 1;
  if (entries > ((size_t)-1) / (3 * sizeof(unsigned int))) { self->failed = 2; return 0; }
  unsigned int* storage = (unsigned int*)malloc(entries * 3 * sizeof(unsigned int));
  if (!storage) { self->failed = 2; return 0; }
  unsigned int* units = storage;
  unsigned int* offsets = storage + entries;
  unsigned int* braces = storage + 2 * entries;
  unsigned int length = 0;
  unsigned int classDepth = 0;
  unsigned int templateDepth = 0;''',
        '''  if (!self->input || (tokens && !capacity) || (!tokens && capacity)
      || self->length > 2147483646U) { self->failed = 1; return 0; }
  size_t entries = (size_t)self->length + 1;
  if (entries > ((size_t)-1) / (2 * sizeof(unsigned int))) { self->failed = 2; return 0; }
  unsigned int* storage = (unsigned int*)malloc(entries * 2 * sizeof(unsigned int));
  if (!storage) { self->failed = 2; return 0; }
  unsigned int* units = storage;
  unsigned int* offsets = storage + entries;
  unsigned int* braces = 0;
  unsigned int braceCapacity = 0;
  unsigned int length = 0;
  unsigned int classDepth = 0;
  unsigned int templateDepth = 0;''')
    content = replace_required(
        content,
        '''    if (!pattern->skip) {
      if (self->tokenCount + 1 >= capacity) { self->failed = 3; break; }
      MaiaCCParserToken* token = tokens + self->tokenCount++;
      token->type = pattern->token;
      token->value = self->input + offsets[self->position];
      token->length = offsets[bestEnd] - offsets[self->position];
      token->start = self->position;
      token->end = (unsigned int)bestEnd;
      if (pattern->role == 6) ++classDepth;''',
        '''    if (!pattern->skip) {
      if (tokens) {
        if (self->tokenCount + 1 >= capacity) { self->failed = 3; break; }
        MaiaCCParserToken* token = tokens + self->tokenCount;
        token->type = pattern->token;
        token->value = self->input + offsets[self->position];
        token->length = offsets[bestEnd] - offsets[self->position];
      }
      ++self->tokenCount;
      if (pattern->role == 3 && templateDepth == braceCapacity) {
        unsigned int newCapacity = braceCapacity ? braceCapacity * 2 : 8;
        if (newCapacity > entries) newCapacity = (unsigned int)entries;
        if (newCapacity <= braceCapacity
            || newCapacity > ((size_t)-1) / sizeof(unsigned int)) {
          self->failed = 2;
          break;
        }
        unsigned int* grown = (unsigned int*)realloc(
            braces, (size_t)newCapacity * sizeof(unsigned int));
        if (!grown) { self->failed = 2; break; }
        braces = grown;
        braceCapacity = newCapacity;
      }
      if (pattern->role == 6) ++classDepth;''')
    content = replace_required(
        content,
        '''  if (!self->failed) {
    MaiaCCParserToken* token = tokens + self->tokenCount++;
    token->type = MAIACC_TOKEN_EOF;
    token->value = self->input + self->length;
    token->length = 0;
    token->start = length;
    token->end = length;
  }
  free(storage);
  return !self->failed;''',
        '''  if (!self->failed) {
    if (tokens) {
      if (self->tokenCount >= capacity) {
        self->failed = 3;
      } else {
        MaiaCCParserToken* token = tokens + self->tokenCount;
        token->type = MAIACC_TOKEN_EOF;
        token->value = self->input + self->length;
        token->length = 0;
      }
    }
    if (!self->failed) ++self->tokenCount;
  }
  free(braces);
  free(storage);
  return !self->failed;''')
    content = replace_required(
        content,
        '''static void maiaCCParserStateInit(
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
}''',
        '''static void maiaCCParserStateInit(
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

static int maiaCC_failureMemoContains(const MaiaCCParserState* self, size_t slot) {
  return self->failureMemo
      && (self->failureMemo[slot >> 3] & (unsigned char)(1u << (slot & 7)));
}

static void maiaCC_failureMemoInsert(MaiaCCParserState* self, size_t slot) {
  if (self->failureMemo) {
    self->failureMemo[slot >> 3] |= (unsigned char)(1u << (slot & 7));
  }
}''')
    content = replace_required(
        content,
        '''static void maiaCC_recordError(MaiaCCParserState* self, unsigned int expected) {
  if (self->errorCount == 0) {
    const MaiaCCParserToken* token = maiaCC_peek(self);
    self->errorPosition = self->position;
    self->expectedToken = expected;
    self->foundToken = token ? token->type : MAIACC_TOKEN_EOF;
  }
  ++self->errorCount;
}''',
        '''static void maiaCC_recordError(MaiaCCParserState* self, unsigned int expected) {
  if (self->errorCount == 0 || self->position >= self->errorPosition) {
    const MaiaCCParserToken* token = maiaCC_peek(self);
    self->errorPosition = self->position;
    self->expectedToken = expected;
    self->foundToken = token ? token->type : MAIACC_TOKEN_EOF;
  }
  ++self->errorCount;
}''')
    content = replace_required(
        content,
        '''  size_t columns = (size_t)self->tokenCount + 1;
  if (columns == 0 || MAIACC_RULE_COUNT > ((size_t)-1) / columns) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_LIMIT; return 0; }
  self->failureMemo = (unsigned char*)calloc(MAIACC_RULE_COUNT, columns);
  if (!self->failureMemo) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_NOMEM; return 0; }''',
        '''  size_t columns = (size_t)self->tokenCount + 1;
  if (columns == 0 || MAIACC_RULE_COUNT > ((size_t)-1) / columns) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_LIMIT; return 0; }
  size_t memoBits = (size_t)MAIACC_RULE_COUNT * columns;
  size_t memoBytes = memoBits / 8 + (memoBits % 8 != 0);
  self->failureMemo = (unsigned char*)calloc(memoBytes, 1);
  if (!self->failureMemo) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_NOMEM; return 0; }''')
    content = replace_required(
        content,
        'self->failureMemo[__memoSlot] = 1;',
        'maiaCC_failureMemoInsert(self, __memoSlot);')
    content = replace_required(
        content,
        'self->failureMemo[__memoSlot]',
        'maiaCC_failureMemoContains(self, __memoSlot)')
    content = replace_required(
        content,
        '''static int maiaCCLexWhitespace(unsigned int value) {
  return (value >= 9 && value <= 13) || value == 32 || value == 160 || value == 5760
      || (value >= 8192 && value <= 8202) || value == 8232 || value == 8233
      || value == 8239 || value == 8287 || value == 12288 || value == 65279;
}''',
        '''static int maiaCCLexWhitespace(unsigned int value) {
  return (value >= 9 && value <= 13) || value == 32 || value == 160 || value == 5760
      || (value >= 8192 && value <= 8202) || value == 8232 || value == 8233
      || value == 8239 || value == 8287 || value == 12288 || value == 65279;
}

static int maiaCCLexSizeBytes(size_t count, size_t itemSize, size_t* bytes) {
  if (itemSize && count > ((size_t)-1) / itemSize) return 0;
  *bytes = count * itemSize;
  return 1;
}''')
    content = replace_required(
        content,
        '''  while (!self->failed && self->position < length) {
    int best = -1;''',
        '''  while (!self->failed && self->position < length) {
    if (self->position + 1 < length && units[self->position] == '/' &&
        units[self->position + 1] == '/') {
      self->position += 2;
      while (self->position < length && units[self->position] != 10 &&
             units[self->position] != 13) {
        ++self->position;
      }
      self->bytePosition = offsets[self->position];
      continue;
    }
    if (self->position + 1 < length && units[self->position] == '/' &&
        units[self->position + 1] == '*') {
      self->position += 2;
      while (self->position + 1 < length &&
             !(units[self->position] == '*' && units[self->position + 1] == '/')) {
        ++self->position;
      }
      if (self->position + 1 >= length) { self->failed = 1; break; }
      self->position += 2;
      self->bytePosition = offsets[self->position];
      continue;
    }
    int best = -1;''')
    content = replace_required(
        content,
        '''        if (newCapacity <= braceCapacity
            || newCapacity > ((size_t)-1) / sizeof(unsigned int)) {
          self->failed = 2;
          break;
        }
        unsigned int* grown = (unsigned int*)realloc(
            braces, (size_t)newCapacity * sizeof(unsigned int));''',
        '''        size_t braceBytes = 0;
        if (newCapacity <= braceCapacity
            || !maiaCCLexSizeBytes(newCapacity, sizeof(unsigned int), &braceBytes)) {
          self->failed = 2;
          break;
        }
        unsigned int* grown = (unsigned int*)realloc(braces, braceBytes);''')

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)

if __name__ == '__main__':
    main()
