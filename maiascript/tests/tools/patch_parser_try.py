#!/usr/bin/env python3
import sys

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

if __name__ == '__main__':
    main()

