#ifndef MAIACC_GENERATED_CPP98_PARSER_H
#define MAIACC_GENERATED_CPP98_PARSER_H

#include <stdlib.h>
#include "maia_regex.h"

#define MAIACC_TOKEN_TOKEN_import 1
#define MAIACC_TOKEN_TOKEN_as 2
#define MAIACC_TOKEN_TOKEN__3B_ 3
#define MAIACC_TOKEN_TOKEN_export 4
#define MAIACC_TOKEN_TOKEN_namespace 5
#define MAIACC_TOKEN_TOKEN__7B_ 6
#define MAIACC_TOKEN_TOKEN__7D_ 7
#define MAIACC_TOKEN_TOKEN_struct 8
#define MAIACC_TOKEN_TOKEN__3D_ 9
#define MAIACC_TOKEN_TOKEN_class 10
#define MAIACC_TOKEN_TOKEN_public 11
#define MAIACC_TOKEN_TOKEN_private 12
#define MAIACC_TOKEN_TOKEN_protected 13
#define MAIACC_TOKEN_TOKEN__28_ 14
#define MAIACC_TOKEN_TOKEN__29_ 15
#define MAIACC_TOKEN_TOKEN_const 16
#define MAIACC_TOKEN_TOKEN_extern 17
#define MAIACC_TOKEN_TOKEN__2C_ 18
#define MAIACC_TOKEN_TOKEN__2E__2E__2E_ 19
#define MAIACC_TOKEN_TOKEN_static 20
#define MAIACC_TOKEN_TOKEN_Result 21
#define MAIACC_TOKEN_TOKEN__3C_ 22
#define MAIACC_TOKEN_TOKEN__3E_ 23
#define MAIACC_TOKEN_TOKEN_Option 24
#define MAIACC_TOKEN_TOKEN_void 25
#define MAIACC_TOKEN_TOKEN_bool 26
#define MAIACC_TOKEN_TOKEN_char 27
#define MAIACC_TOKEN_TOKEN_string 28
#define MAIACC_TOKEN_TOKEN_i8 29
#define MAIACC_TOKEN_TOKEN_i16 30
#define MAIACC_TOKEN_TOKEN_i32 31
#define MAIACC_TOKEN_TOKEN_i64 32
#define MAIACC_TOKEN_TOKEN_u8 33
#define MAIACC_TOKEN_TOKEN_u16 34
#define MAIACC_TOKEN_TOKEN_u32 35
#define MAIACC_TOKEN_TOKEN_u64 36
#define MAIACC_TOKEN_TOKEN_f32 37
#define MAIACC_TOKEN_TOKEN_f64 38
#define MAIACC_TOKEN_TOKEN__2A_ 39
#define MAIACC_TOKEN_TOKEN__5B_ 40
#define MAIACC_TOKEN_TOKEN__5D_ 41
#define MAIACC_TOKEN_TOKEN__3F_ 42
#define MAIACC_TOKEN_TOKEN__2E_ 43
#define MAIACC_TOKEN_TOKEN_auto 44
#define MAIACC_TOKEN_TOKEN_if 45
#define MAIACC_TOKEN_TOKEN_else 46
#define MAIACC_TOKEN_TOKEN_while 47
#define MAIACC_TOKEN_TOKEN_do 48
#define MAIACC_TOKEN_TOKEN_for 49
#define MAIACC_TOKEN_TOKEN_foreach 50
#define MAIACC_TOKEN_TOKEN__3A_ 51
#define MAIACC_TOKEN_TOKEN_switch 52
#define MAIACC_TOKEN_TOKEN_case 53
#define MAIACC_TOKEN_TOKEN_default 54
#define MAIACC_TOKEN_TOKEN_return 55
#define MAIACC_TOKEN_TOKEN_break 56
#define MAIACC_TOKEN_TOKEN_continue 57
#define MAIACC_TOKEN_TOKEN_defer 58
#define MAIACC_TOKEN_TOKEN_unsafe 59
#define MAIACC_TOKEN_TOKEN__2B__3D_ 60
#define MAIACC_TOKEN_TOKEN__2D__3D_ 61
#define MAIACC_TOKEN_TOKEN__2A__3D_ 62
#define MAIACC_TOKEN_TOKEN__2F__3D_ 63
#define MAIACC_TOKEN_TOKEN__25__3D_ 64
#define MAIACC_TOKEN_TOKEN__26__3D_ 65
#define MAIACC_TOKEN_TOKEN__7C__3D_ 66
#define MAIACC_TOKEN_TOKEN__5E__3D_ 67
#define MAIACC_TOKEN_TOKEN__3C__3C__3D_ 68
#define MAIACC_TOKEN_TOKEN__3E__3E__3D_ 69
#define MAIACC_TOKEN_TOKEN__7C__7C_ 70
#define MAIACC_TOKEN_TOKEN__26__26_ 71
#define MAIACC_TOKEN_TOKEN__7C_ 72
#define MAIACC_TOKEN_TOKEN__5E_ 73
#define MAIACC_TOKEN_TOKEN__26_ 74
#define MAIACC_TOKEN_TOKEN__3D__3D_ 75
#define MAIACC_TOKEN_TOKEN__21__3D_ 76
#define MAIACC_TOKEN_TOKEN__3C__3D_ 77
#define MAIACC_TOKEN_TOKEN__3E__3D_ 78
#define MAIACC_TOKEN_TOKEN__3C__3C_ 79
#define MAIACC_TOKEN_TOKEN__3E__3E_ 80
#define MAIACC_TOKEN_TOKEN__2B_ 81
#define MAIACC_TOKEN_TOKEN__2D_ 82
#define MAIACC_TOKEN_TOKEN__2F_ 83
#define MAIACC_TOKEN_TOKEN__25_ 84
#define MAIACC_TOKEN_TOKEN__21_ 85
#define MAIACC_TOKEN_TOKEN__7E_ 86
#define MAIACC_TOKEN_TOKEN__2B__2B_ 87
#define MAIACC_TOKEN_TOKEN__2D__2D_ 88
#define MAIACC_TOKEN_TOKEN__2D__3E_ 89
#define MAIACC_TOKEN_TOKEN_new 90
#define MAIACC_TOKEN_TOKEN_delete 91
#define MAIACC_TOKEN_TOKEN_sizeof 92
#define MAIACC_TOKEN_TOKEN_asm 93
#define MAIACC_TOKEN_TOKEN_true 94
#define MAIACC_TOKEN_TOKEN_false 95
#define MAIACC_TOKEN_TOKEN_null 96
#define MAIACC_TOKEN_EOF 97
#define MAIACC_TOKEN_NAME 98
#define MAIACC_TOKEN_Keyword 99
#define MAIACC_TOKEN_INTEGER 100
#define MAIACC_TOKEN_FLOAT 101
#define MAIACC_TOKEN_STRING 102
#define MAIACC_TOKEN_CHAR 103
#define MAIACC_TOKEN_Escape 104
#define MAIACC_TOKEN_LINE_COMMENT 105
#define MAIACC_TOKEN_BLOCK_COMMENT 106
#define MAIACC_TOKEN_WS 107
#define MAIACC_TOKEN_COUNT 108
#define MAIACC_RULE_COUNT 61

static const char* maiaCCParserTokenName(unsigned int type) {
  switch (type) {
    case MAIACC_TOKEN_TOKEN_import: return "TOKEN_import";
    case MAIACC_TOKEN_TOKEN_as: return "TOKEN_as";
    case MAIACC_TOKEN_TOKEN__3B_: return "TOKEN__3B_";
    case MAIACC_TOKEN_TOKEN_export: return "TOKEN_export";
    case MAIACC_TOKEN_TOKEN_namespace: return "TOKEN_namespace";
    case MAIACC_TOKEN_TOKEN__7B_: return "TOKEN__7B_";
    case MAIACC_TOKEN_TOKEN__7D_: return "TOKEN__7D_";
    case MAIACC_TOKEN_TOKEN_struct: return "TOKEN_struct";
    case MAIACC_TOKEN_TOKEN__3D_: return "TOKEN__3D_";
    case MAIACC_TOKEN_TOKEN_class: return "TOKEN_class";
    case MAIACC_TOKEN_TOKEN_public: return "TOKEN_public";
    case MAIACC_TOKEN_TOKEN_private: return "TOKEN_private";
    case MAIACC_TOKEN_TOKEN_protected: return "TOKEN_protected";
    case MAIACC_TOKEN_TOKEN__28_: return "TOKEN__28_";
    case MAIACC_TOKEN_TOKEN__29_: return "TOKEN__29_";
    case MAIACC_TOKEN_TOKEN_const: return "TOKEN_const";
    case MAIACC_TOKEN_TOKEN_extern: return "TOKEN_extern";
    case MAIACC_TOKEN_TOKEN__2C_: return "TOKEN__2C_";
    case MAIACC_TOKEN_TOKEN__2E__2E__2E_: return "TOKEN__2E__2E__2E_";
    case MAIACC_TOKEN_TOKEN_static: return "TOKEN_static";
    case MAIACC_TOKEN_TOKEN_Result: return "TOKEN_Result";
    case MAIACC_TOKEN_TOKEN__3C_: return "TOKEN__3C_";
    case MAIACC_TOKEN_TOKEN__3E_: return "TOKEN__3E_";
    case MAIACC_TOKEN_TOKEN_Option: return "TOKEN_Option";
    case MAIACC_TOKEN_TOKEN_void: return "TOKEN_void";
    case MAIACC_TOKEN_TOKEN_bool: return "TOKEN_bool";
    case MAIACC_TOKEN_TOKEN_char: return "TOKEN_char";
    case MAIACC_TOKEN_TOKEN_string: return "TOKEN_string";
    case MAIACC_TOKEN_TOKEN_i8: return "TOKEN_i8";
    case MAIACC_TOKEN_TOKEN_i16: return "TOKEN_i16";
    case MAIACC_TOKEN_TOKEN_i32: return "TOKEN_i32";
    case MAIACC_TOKEN_TOKEN_i64: return "TOKEN_i64";
    case MAIACC_TOKEN_TOKEN_u8: return "TOKEN_u8";
    case MAIACC_TOKEN_TOKEN_u16: return "TOKEN_u16";
    case MAIACC_TOKEN_TOKEN_u32: return "TOKEN_u32";
    case MAIACC_TOKEN_TOKEN_u64: return "TOKEN_u64";
    case MAIACC_TOKEN_TOKEN_f32: return "TOKEN_f32";
    case MAIACC_TOKEN_TOKEN_f64: return "TOKEN_f64";
    case MAIACC_TOKEN_TOKEN__2A_: return "TOKEN__2A_";
    case MAIACC_TOKEN_TOKEN__5B_: return "TOKEN__5B_";
    case MAIACC_TOKEN_TOKEN__5D_: return "TOKEN__5D_";
    case MAIACC_TOKEN_TOKEN__3F_: return "TOKEN__3F_";
    case MAIACC_TOKEN_TOKEN__2E_: return "TOKEN__2E_";
    case MAIACC_TOKEN_TOKEN_auto: return "TOKEN_auto";
    case MAIACC_TOKEN_TOKEN_if: return "TOKEN_if";
    case MAIACC_TOKEN_TOKEN_else: return "TOKEN_else";
    case MAIACC_TOKEN_TOKEN_while: return "TOKEN_while";
    case MAIACC_TOKEN_TOKEN_do: return "TOKEN_do";
    case MAIACC_TOKEN_TOKEN_for: return "TOKEN_for";
    case MAIACC_TOKEN_TOKEN_foreach: return "TOKEN_foreach";
    case MAIACC_TOKEN_TOKEN__3A_: return "TOKEN__3A_";
    case MAIACC_TOKEN_TOKEN_switch: return "TOKEN_switch";
    case MAIACC_TOKEN_TOKEN_case: return "TOKEN_case";
    case MAIACC_TOKEN_TOKEN_default: return "TOKEN_default";
    case MAIACC_TOKEN_TOKEN_return: return "TOKEN_return";
    case MAIACC_TOKEN_TOKEN_break: return "TOKEN_break";
    case MAIACC_TOKEN_TOKEN_continue: return "TOKEN_continue";
    case MAIACC_TOKEN_TOKEN_defer: return "TOKEN_defer";
    case MAIACC_TOKEN_TOKEN_unsafe: return "TOKEN_unsafe";
    case MAIACC_TOKEN_TOKEN__2B__3D_: return "TOKEN__2B__3D_";
    case MAIACC_TOKEN_TOKEN__2D__3D_: return "TOKEN__2D__3D_";
    case MAIACC_TOKEN_TOKEN__2A__3D_: return "TOKEN__2A__3D_";
    case MAIACC_TOKEN_TOKEN__2F__3D_: return "TOKEN__2F__3D_";
    case MAIACC_TOKEN_TOKEN__25__3D_: return "TOKEN__25__3D_";
    case MAIACC_TOKEN_TOKEN__26__3D_: return "TOKEN__26__3D_";
    case MAIACC_TOKEN_TOKEN__7C__3D_: return "TOKEN__7C__3D_";
    case MAIACC_TOKEN_TOKEN__5E__3D_: return "TOKEN__5E__3D_";
    case MAIACC_TOKEN_TOKEN__3C__3C__3D_: return "TOKEN__3C__3C__3D_";
    case MAIACC_TOKEN_TOKEN__3E__3E__3D_: return "TOKEN__3E__3E__3D_";
    case MAIACC_TOKEN_TOKEN__7C__7C_: return "TOKEN__7C__7C_";
    case MAIACC_TOKEN_TOKEN__26__26_: return "TOKEN__26__26_";
    case MAIACC_TOKEN_TOKEN__7C_: return "TOKEN__7C_";
    case MAIACC_TOKEN_TOKEN__5E_: return "TOKEN__5E_";
    case MAIACC_TOKEN_TOKEN__26_: return "TOKEN__26_";
    case MAIACC_TOKEN_TOKEN__3D__3D_: return "TOKEN__3D__3D_";
    case MAIACC_TOKEN_TOKEN__21__3D_: return "TOKEN__21__3D_";
    case MAIACC_TOKEN_TOKEN__3C__3D_: return "TOKEN__3C__3D_";
    case MAIACC_TOKEN_TOKEN__3E__3D_: return "TOKEN__3E__3D_";
    case MAIACC_TOKEN_TOKEN__3C__3C_: return "TOKEN__3C__3C_";
    case MAIACC_TOKEN_TOKEN__3E__3E_: return "TOKEN__3E__3E_";
    case MAIACC_TOKEN_TOKEN__2B_: return "TOKEN__2B_";
    case MAIACC_TOKEN_TOKEN__2D_: return "TOKEN__2D_";
    case MAIACC_TOKEN_TOKEN__2F_: return "TOKEN__2F_";
    case MAIACC_TOKEN_TOKEN__25_: return "TOKEN__25_";
    case MAIACC_TOKEN_TOKEN__21_: return "TOKEN__21_";
    case MAIACC_TOKEN_TOKEN__7E_: return "TOKEN__7E_";
    case MAIACC_TOKEN_TOKEN__2B__2B_: return "TOKEN__2B__2B_";
    case MAIACC_TOKEN_TOKEN__2D__2D_: return "TOKEN__2D__2D_";
    case MAIACC_TOKEN_TOKEN__2D__3E_: return "TOKEN__2D__3E_";
    case MAIACC_TOKEN_TOKEN_new: return "TOKEN_new";
    case MAIACC_TOKEN_TOKEN_delete: return "TOKEN_delete";
    case MAIACC_TOKEN_TOKEN_sizeof: return "TOKEN_sizeof";
    case MAIACC_TOKEN_TOKEN_asm: return "TOKEN_asm";
    case MAIACC_TOKEN_TOKEN_true: return "TOKEN_true";
    case MAIACC_TOKEN_TOKEN_false: return "TOKEN_false";
    case MAIACC_TOKEN_TOKEN_null: return "TOKEN_null";
    case MAIACC_TOKEN_EOF: return "EOF";
    case MAIACC_TOKEN_NAME: return "NAME";
    case MAIACC_TOKEN_Keyword: return "Keyword";
    case MAIACC_TOKEN_INTEGER: return "INTEGER";
    case MAIACC_TOKEN_FLOAT: return "FLOAT";
    case MAIACC_TOKEN_STRING: return "STRING";
    case MAIACC_TOKEN_CHAR: return "CHAR";
    case MAIACC_TOKEN_Escape: return "Escape";
    case MAIACC_TOKEN_LINE_COMMENT: return "LINE_COMMENT";
    case MAIACC_TOKEN_BLOCK_COMMENT: return "BLOCK_COMMENT";
    case MAIACC_TOKEN_WS: return "WS";
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
  { 1, 116, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 112, 0, 3 },
  { 1, 109, 0, 4 },
  { 1, 105, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges0[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode1[] = {
  { 0, 0, 0, 0 },
  { 1, 115, 0, 0 },
  { 1, 97, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges1[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode2[] = {
  { 0, 0, 0, 0 },
  { 1, 59, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges2[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode3[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 112, 0, 3 },
  { 1, 120, 0, 4 },
  { 1, 101, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges3[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode4[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 112, 0, 3 },
  { 1, 115, 0, 4 },
  { 1, 101, 0, 5 },
  { 1, 109, 0, 6 },
  { 1, 97, 0, 7 },
  { 1, 110, 0, 8 }
};
static const MaiaCCLexRange maiaCCLexRanges4[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode5[] = {
  { 0, 0, 0, 0 },
  { 1, 123, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges5[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode6[] = {
  { 0, 0, 0, 0 },
  { 1, 125, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges6[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode7[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 114, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges7[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode8[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges8[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode9[] = {
  { 0, 0, 0, 0 },
  { 1, 115, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 108, 0, 3 },
  { 1, 99, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges9[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode10[] = {
  { 0, 0, 0, 0 },
  { 1, 99, 0, 0 },
  { 1, 105, 0, 1 },
  { 1, 108, 0, 2 },
  { 1, 98, 0, 3 },
  { 1, 117, 0, 4 },
  { 1, 112, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges10[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode11[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 116, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 118, 0, 3 },
  { 1, 105, 0, 4 },
  { 1, 114, 0, 5 },
  { 1, 112, 0, 6 }
};
static const MaiaCCLexRange maiaCCLexRanges11[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode12[] = {
  { 0, 0, 0, 0 },
  { 1, 100, 0, 0 },
  { 1, 101, 0, 1 },
  { 1, 116, 0, 2 },
  { 1, 99, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 116, 0, 5 },
  { 1, 111, 0, 6 },
  { 1, 114, 0, 7 },
  { 1, 112, 0, 8 }
};
static const MaiaCCLexRange maiaCCLexRanges12[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode13[] = {
  { 0, 0, 0, 0 },
  { 1, 40, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges13[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode14[] = {
  { 0, 0, 0, 0 },
  { 1, 41, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges14[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode15[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 110, 0, 2 },
  { 1, 111, 0, 3 },
  { 1, 99, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges15[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode16[] = {
  { 0, 0, 0, 0 },
  { 1, 110, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 116, 0, 3 },
  { 1, 120, 0, 4 },
  { 1, 101, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges16[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode17[] = {
  { 0, 0, 0, 0 },
  { 1, 44, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges17[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode18[] = {
  { 0, 0, 0, 0 },
  { 1, 46, 0, 0 },
  { 1, 46, 0, 1 },
  { 1, 46, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges18[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode19[] = {
  { 0, 0, 0, 0 },
  { 1, 99, 0, 0 },
  { 1, 105, 0, 1 },
  { 1, 116, 0, 2 },
  { 1, 97, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges19[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode20[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 115, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 82, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges20[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode21[] = {
  { 0, 0, 0, 0 },
  { 1, 60, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges21[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode22[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges22[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode23[] = {
  { 0, 0, 0, 0 },
  { 1, 110, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 105, 0, 2 },
  { 1, 116, 0, 3 },
  { 1, 112, 0, 4 },
  { 1, 79, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges23[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode24[] = {
  { 0, 0, 0, 0 },
  { 1, 100, 0, 0 },
  { 1, 105, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 118, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges24[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode25[] = {
  { 0, 0, 0, 0 },
  { 1, 108, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 98, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges25[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode26[] = {
  { 0, 0, 0, 0 },
  { 1, 114, 0, 0 },
  { 1, 97, 0, 1 },
  { 1, 104, 0, 2 },
  { 1, 99, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges26[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode27[] = {
  { 0, 0, 0, 0 },
  { 1, 103, 0, 0 },
  { 1, 110, 0, 1 },
  { 1, 105, 0, 2 },
  { 1, 114, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges27[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode28[] = {
  { 0, 0, 0, 0 },
  { 1, 56, 0, 0 },
  { 1, 105, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges28[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode29[] = {
  { 0, 0, 0, 0 },
  { 1, 54, 0, 0 },
  { 1, 49, 0, 1 },
  { 1, 105, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges29[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode30[] = {
  { 0, 0, 0, 0 },
  { 1, 50, 0, 0 },
  { 1, 51, 0, 1 },
  { 1, 105, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges30[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode31[] = {
  { 0, 0, 0, 0 },
  { 1, 52, 0, 0 },
  { 1, 54, 0, 1 },
  { 1, 105, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges31[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode32[] = {
  { 0, 0, 0, 0 },
  { 1, 56, 0, 0 },
  { 1, 117, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges32[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode33[] = {
  { 0, 0, 0, 0 },
  { 1, 54, 0, 0 },
  { 1, 49, 0, 1 },
  { 1, 117, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges33[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode34[] = {
  { 0, 0, 0, 0 },
  { 1, 50, 0, 0 },
  { 1, 51, 0, 1 },
  { 1, 117, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges34[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode35[] = {
  { 0, 0, 0, 0 },
  { 1, 52, 0, 0 },
  { 1, 54, 0, 1 },
  { 1, 117, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges35[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode36[] = {
  { 0, 0, 0, 0 },
  { 1, 50, 0, 0 },
  { 1, 51, 0, 1 },
  { 1, 102, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges36[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode37[] = {
  { 0, 0, 0, 0 },
  { 1, 52, 0, 0 },
  { 1, 54, 0, 1 },
  { 1, 102, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges37[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode38[] = {
  { 0, 0, 0, 0 },
  { 1, 42, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges38[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode39[] = {
  { 0, 0, 0, 0 },
  { 1, 91, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges39[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode40[] = {
  { 0, 0, 0, 0 },
  { 1, 93, 0, 0 }
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
  { 1, 46, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges42[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode43[] = {
  { 0, 0, 0, 0 },
  { 1, 111, 0, 0 },
  { 1, 116, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 97, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges43[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode44[] = {
  { 0, 0, 0, 0 },
  { 1, 102, 0, 0 },
  { 1, 105, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges44[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode45[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 108, 0, 2 },
  { 1, 101, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges45[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode46[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 105, 0, 2 },
  { 1, 104, 0, 3 },
  { 1, 119, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges46[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode47[] = {
  { 0, 0, 0, 0 },
  { 1, 111, 0, 0 },
  { 1, 100, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges47[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode48[] = {
  { 0, 0, 0, 0 },
  { 1, 114, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 102, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges48[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode49[] = {
  { 0, 0, 0, 0 },
  { 1, 104, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 101, 0, 3 },
  { 1, 114, 0, 4 },
  { 1, 111, 0, 5 },
  { 1, 102, 0, 6 }
};
static const MaiaCCLexRange maiaCCLexRanges49[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode50[] = {
  { 0, 0, 0, 0 },
  { 1, 58, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges50[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode51[] = {
  { 0, 0, 0, 0 },
  { 1, 104, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 116, 0, 2 },
  { 1, 105, 0, 3 },
  { 1, 119, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges51[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode52[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 99, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges52[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode53[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 97, 0, 3 },
  { 1, 102, 0, 4 },
  { 1, 101, 0, 5 },
  { 1, 100, 0, 6 }
};
static const MaiaCCLexRange maiaCCLexRanges53[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode54[] = {
  { 0, 0, 0, 0 },
  { 1, 110, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 116, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 114, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges54[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode55[] = {
  { 0, 0, 0, 0 },
  { 1, 107, 0, 0 },
  { 1, 97, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 114, 0, 3 },
  { 1, 98, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges55[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode56[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 117, 0, 1 },
  { 1, 110, 0, 2 },
  { 1, 105, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 110, 0, 5 },
  { 1, 111, 0, 6 },
  { 1, 99, 0, 7 }
};
static const MaiaCCLexRange maiaCCLexRanges56[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode57[] = {
  { 0, 0, 0, 0 },
  { 1, 114, 0, 0 },
  { 1, 101, 0, 1 },
  { 1, 102, 0, 2 },
  { 1, 101, 0, 3 },
  { 1, 100, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges57[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode58[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 102, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 115, 0, 3 },
  { 1, 110, 0, 4 },
  { 1, 117, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges58[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode59[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 43, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges59[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode60[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges60[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode61[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 42, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges61[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode62[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 47, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges62[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode63[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 37, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges63[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode64[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 38, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges64[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode65[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 124, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges65[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode66[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 94, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges66[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode67[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 60, 0, 1 },
  { 1, 60, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges67[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode68[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 62, 0, 1 },
  { 1, 62, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges68[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode69[] = {
  { 0, 0, 0, 0 },
  { 1, 124, 0, 0 },
  { 1, 124, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges69[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode70[] = {
  { 0, 0, 0, 0 },
  { 1, 38, 0, 0 },
  { 1, 38, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges70[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode71[] = {
  { 0, 0, 0, 0 },
  { 1, 124, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges71[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode72[] = {
  { 0, 0, 0, 0 },
  { 1, 94, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges72[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode73[] = {
  { 0, 0, 0, 0 },
  { 1, 38, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges73[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode74[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 61, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges74[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode75[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 33, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges75[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode76[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 60, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges76[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode77[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 62, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges77[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode78[] = {
  { 0, 0, 0, 0 },
  { 1, 60, 0, 0 },
  { 1, 60, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges78[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode79[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 },
  { 1, 62, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges79[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode80[] = {
  { 0, 0, 0, 0 },
  { 1, 43, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges80[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode81[] = {
  { 0, 0, 0, 0 },
  { 1, 45, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges81[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode82[] = {
  { 0, 0, 0, 0 },
  { 1, 47, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges82[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode83[] = {
  { 0, 0, 0, 0 },
  { 1, 37, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges83[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode84[] = {
  { 0, 0, 0, 0 },
  { 1, 33, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges84[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode85[] = {
  { 0, 0, 0, 0 },
  { 1, 126, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges85[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode86[] = {
  { 0, 0, 0, 0 },
  { 1, 43, 0, 0 },
  { 1, 43, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges86[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode87[] = {
  { 0, 0, 0, 0 },
  { 1, 45, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges87[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode88[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges88[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode89[] = {
  { 0, 0, 0, 0 },
  { 1, 119, 0, 0 },
  { 1, 101, 0, 1 },
  { 1, 110, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges89[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode90[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 116, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 108, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 100, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges90[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode91[] = {
  { 0, 0, 0, 0 },
  { 1, 102, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 122, 0, 3 },
  { 1, 105, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges91[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode92[] = {
  { 0, 0, 0, 0 },
  { 1, 109, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 97, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges92[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode93[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 117, 0, 1 },
  { 1, 114, 0, 2 },
  { 1, 116, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges93[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode94[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 108, 0, 2 },
  { 1, 97, 0, 3 },
  { 1, 102, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges94[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode95[] = {
  { 0, 0, 0, 0 },
  { 1, 108, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 110, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges95[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode96[] = {
  { 0, 0, 0, 0 },
  { 2, 2, 0, 0 },
  { 3, 0, 4, 1 },
  { 3, 4, 3, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges96[] = {
  { 65, 90 },
  { 97, 122 },
  { 48, 57 },
  { 95, 95 },
  { 65, 90 },
  { 97, 122 },
  { 95, 95 }
};
static const MaiaCCLexInstruction maiaCCLexCode97[] = {
  { 0, 0, 0, 0 },
  { 1, 70, 0, 0 },
  { 1, 102, 0, 0 },
  { 2, 2, 1, 0 },
  { 2, 3, 0, 0 },
  { 2, 6, 4, 0 },
  { 3, 0, 1, 5 },
  { 1, 45, 0, 6 },
  { 1, 43, 0, 6 },
  { 2, 8, 7, 0 },
  { 2, 9, 6, 0 },
  { 1, 69, 0, 10 },
  { 1, 101, 0, 10 },
  { 2, 12, 11, 0 },
  { 2, 15, 13, 0 },
  { 3, 1, 1, 14 },
  { 1, 70, 0, 0 },
  { 1, 102, 0, 0 },
  { 2, 17, 16, 0 },
  { 2, 18, 0, 0 },
  { 2, 21, 19, 0 },
  { 3, 2, 1, 20 },
  { 1, 45, 0, 21 },
  { 1, 43, 0, 21 },
  { 2, 23, 22, 0 },
  { 2, 24, 21, 0 },
  { 1, 69, 0, 25 },
  { 1, 101, 0, 25 },
  { 2, 27, 26, 0 },
  { 2, 28, 19, 0 },
  { 2, 31, 29, 0 },
  { 3, 3, 1, 30 },
  { 1, 46, 0, 31 },
  { 2, 34, 32, 0 },
  { 3, 4, 1, 33 },
  { 2, 34, 15, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges97[] = {
  { 48, 57 },
  { 48, 57 },
  { 48, 57 },
  { 48, 57 },
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode98[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 2, 0 },
  { 3, 2, 2, 0 },
  { 2, 2, 1, 0 },
  { 2, 3, 0, 0 },
  { 2, 6, 4, 0 },
  { 3, 4, 1, 5 },
  { 2, 8, 4, 0 },
  { 3, 5, 2, 7 },
  { 1, 98, 0, 8 },
  { 1, 48, 0, 9 },
  { 2, 10, 6, 0 },
  { 2, 13, 4, 0 },
  { 3, 7, 3, 12 },
  { 1, 120, 0, 13 },
  { 1, 48, 0, 14 },
  { 2, 15, 11, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges98[] = {
  { 108, 108 },
  { 76, 76 },
  { 117, 117 },
  { 85, 85 },
  { 48, 57 },
  { 48, 48 },
  { 49, 49 },
  { 48, 57 },
  { 65, 70 },
  { 97, 102 }
};
static const MaiaCCLexInstruction maiaCCLexCode99[] = {
  { 0, 0, 0, 0 },
  { 1, 34, 0, 0 },
  { 2, 18, 1, 0 },
  { 4, 0, 4, 2 },
  { 1, 92, 0, 2 },
  { 1, 39, 0, 2 },
  { 2, 5, 4, 0 },
  { 1, 34, 0, 2 },
  { 2, 7, 6, 0 },
  { 1, 48, 0, 2 },
  { 2, 9, 8, 0 },
  { 1, 116, 0, 2 },
  { 2, 11, 10, 0 },
  { 1, 114, 0, 2 },
  { 2, 13, 12, 0 },
  { 1, 110, 0, 2 },
  { 2, 15, 14, 0 },
  { 1, 92, 0, 16 },
  { 2, 17, 3, 0 },
  { 1, 34, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges99[] = {
  { 34, 34 },
  { 92, 92 },
  { 10, 10 },
  { 13, 13 }
};
static const MaiaCCLexInstruction maiaCCLexCode100[] = {
  { 0, 0, 0, 0 },
  { 1, 39, 0, 0 },
  { 4, 0, 4, 1 },
  { 1, 92, 0, 1 },
  { 1, 39, 0, 1 },
  { 2, 4, 3, 0 },
  { 1, 34, 0, 1 },
  { 2, 6, 5, 0 },
  { 1, 48, 0, 1 },
  { 2, 8, 7, 0 },
  { 1, 116, 0, 1 },
  { 2, 10, 9, 0 },
  { 1, 114, 0, 1 },
  { 2, 12, 11, 0 },
  { 1, 110, 0, 1 },
  { 2, 14, 13, 0 },
  { 1, 92, 0, 15 },
  { 2, 16, 2, 0 },
  { 1, 39, 0, 17 }
};
static const MaiaCCLexRange maiaCCLexRanges100[] = {
  { 39, 39 },
  { 92, 92 },
  { 10, 10 },
  { 13, 13 }
};
static const MaiaCCLexInstruction maiaCCLexCode101[] = {
  { 0, 0, 0, 0 },
  { 2, 2, 0, 0 },
  { 4, 0, 2, 1 },
  { 1, 47, 0, 1 },
  { 1, 47, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges101[] = {
  { 10, 10 },
  { 13, 13 }
};
static const MaiaCCLexInstruction maiaCCLexCode102[] = {
  { 0, 0, 0, 0 },
  { 1, 47, 0, 0 },
  { 1, 42, 0, 1 },
  { 2, 7, 2, 0 },
  { 4, 0, 1, 3 },
  { 1, 42, 0, 4 },
  { 4, 1, 1, 3 },
  { 2, 6, 5, 0 },
  { 1, 42, 0, 3 },
  { 1, 47, 0, 8 }
};
static const MaiaCCLexRange maiaCCLexRanges102[] = {
  { 47, 47 },
  { 42, 42 }
};
static const MaiaCCLexInstruction maiaCCLexCode103[] = {
  { 0, 0, 0, 0 },
  { 2, 2, 0, 0 },
  { 3, 0, 4, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges103[] = {
  { 32, 32 },
  { 9, 9 },
  { 10, 10 },
  { 13, 13 }
};
static const MaiaCCLexInstruction maiaCCLexCode104[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 112, 0, 3 },
  { 1, 109, 0, 4 },
  { 1, 105, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges104[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode105[] = {
  { 0, 0, 0, 0 },
  { 1, 115, 0, 0 },
  { 1, 97, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges105[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode106[] = {
  { 0, 0, 0, 0 },
  { 1, 59, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges106[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode107[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 112, 0, 3 },
  { 1, 120, 0, 4 },
  { 1, 101, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges107[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode108[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 112, 0, 3 },
  { 1, 115, 0, 4 },
  { 1, 101, 0, 5 },
  { 1, 109, 0, 6 },
  { 1, 97, 0, 7 },
  { 1, 110, 0, 8 }
};
static const MaiaCCLexRange maiaCCLexRanges108[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode109[] = {
  { 0, 0, 0, 0 },
  { 1, 123, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges109[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode110[] = {
  { 0, 0, 0, 0 },
  { 1, 125, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges110[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode111[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 114, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges111[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode112[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges112[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode113[] = {
  { 0, 0, 0, 0 },
  { 1, 115, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 108, 0, 3 },
  { 1, 99, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges113[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode114[] = {
  { 0, 0, 0, 0 },
  { 1, 99, 0, 0 },
  { 1, 105, 0, 1 },
  { 1, 108, 0, 2 },
  { 1, 98, 0, 3 },
  { 1, 117, 0, 4 },
  { 1, 112, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges114[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode115[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 116, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 118, 0, 3 },
  { 1, 105, 0, 4 },
  { 1, 114, 0, 5 },
  { 1, 112, 0, 6 }
};
static const MaiaCCLexRange maiaCCLexRanges115[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode116[] = {
  { 0, 0, 0, 0 },
  { 1, 100, 0, 0 },
  { 1, 101, 0, 1 },
  { 1, 116, 0, 2 },
  { 1, 99, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 116, 0, 5 },
  { 1, 111, 0, 6 },
  { 1, 114, 0, 7 },
  { 1, 112, 0, 8 }
};
static const MaiaCCLexRange maiaCCLexRanges116[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode117[] = {
  { 0, 0, 0, 0 },
  { 1, 40, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges117[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode118[] = {
  { 0, 0, 0, 0 },
  { 1, 41, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges118[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode119[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 110, 0, 2 },
  { 1, 111, 0, 3 },
  { 1, 99, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges119[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode120[] = {
  { 0, 0, 0, 0 },
  { 1, 110, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 116, 0, 3 },
  { 1, 120, 0, 4 },
  { 1, 101, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges120[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode121[] = {
  { 0, 0, 0, 0 },
  { 1, 44, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges121[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode122[] = {
  { 0, 0, 0, 0 },
  { 1, 46, 0, 0 },
  { 1, 46, 0, 1 },
  { 1, 46, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges122[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode123[] = {
  { 0, 0, 0, 0 },
  { 1, 99, 0, 0 },
  { 1, 105, 0, 1 },
  { 1, 116, 0, 2 },
  { 1, 97, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges123[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode124[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 115, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 82, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges124[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode125[] = {
  { 0, 0, 0, 0 },
  { 1, 60, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges125[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode126[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges126[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode127[] = {
  { 0, 0, 0, 0 },
  { 1, 110, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 105, 0, 2 },
  { 1, 116, 0, 3 },
  { 1, 112, 0, 4 },
  { 1, 79, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges127[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode128[] = {
  { 0, 0, 0, 0 },
  { 1, 100, 0, 0 },
  { 1, 105, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 118, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges128[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode129[] = {
  { 0, 0, 0, 0 },
  { 1, 108, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 111, 0, 2 },
  { 1, 98, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges129[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode130[] = {
  { 0, 0, 0, 0 },
  { 1, 114, 0, 0 },
  { 1, 97, 0, 1 },
  { 1, 104, 0, 2 },
  { 1, 99, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges130[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode131[] = {
  { 0, 0, 0, 0 },
  { 1, 103, 0, 0 },
  { 1, 110, 0, 1 },
  { 1, 105, 0, 2 },
  { 1, 114, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges131[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode132[] = {
  { 0, 0, 0, 0 },
  { 1, 56, 0, 0 },
  { 1, 105, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges132[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode133[] = {
  { 0, 0, 0, 0 },
  { 1, 54, 0, 0 },
  { 1, 49, 0, 1 },
  { 1, 105, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges133[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode134[] = {
  { 0, 0, 0, 0 },
  { 1, 50, 0, 0 },
  { 1, 51, 0, 1 },
  { 1, 105, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges134[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode135[] = {
  { 0, 0, 0, 0 },
  { 1, 52, 0, 0 },
  { 1, 54, 0, 1 },
  { 1, 105, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges135[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode136[] = {
  { 0, 0, 0, 0 },
  { 1, 56, 0, 0 },
  { 1, 117, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges136[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode137[] = {
  { 0, 0, 0, 0 },
  { 1, 54, 0, 0 },
  { 1, 49, 0, 1 },
  { 1, 117, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges137[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode138[] = {
  { 0, 0, 0, 0 },
  { 1, 50, 0, 0 },
  { 1, 51, 0, 1 },
  { 1, 117, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges138[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode139[] = {
  { 0, 0, 0, 0 },
  { 1, 52, 0, 0 },
  { 1, 54, 0, 1 },
  { 1, 117, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges139[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode140[] = {
  { 0, 0, 0, 0 },
  { 1, 50, 0, 0 },
  { 1, 51, 0, 1 },
  { 1, 102, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges140[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode141[] = {
  { 0, 0, 0, 0 },
  { 1, 52, 0, 0 },
  { 1, 54, 0, 1 },
  { 1, 102, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges141[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode142[] = {
  { 0, 0, 0, 0 },
  { 1, 42, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges142[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode143[] = {
  { 0, 0, 0, 0 },
  { 1, 91, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges143[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode144[] = {
  { 0, 0, 0, 0 },
  { 1, 93, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges144[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode145[] = {
  { 0, 0, 0, 0 },
  { 1, 63, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges145[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode146[] = {
  { 0, 0, 0, 0 },
  { 1, 46, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges146[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode147[] = {
  { 0, 0, 0, 0 },
  { 1, 111, 0, 0 },
  { 1, 116, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 97, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges147[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode148[] = {
  { 0, 0, 0, 0 },
  { 1, 102, 0, 0 },
  { 1, 105, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges148[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode149[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 108, 0, 2 },
  { 1, 101, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges149[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode150[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 105, 0, 2 },
  { 1, 104, 0, 3 },
  { 1, 119, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges150[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode151[] = {
  { 0, 0, 0, 0 },
  { 1, 111, 0, 0 },
  { 1, 100, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges151[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode152[] = {
  { 0, 0, 0, 0 },
  { 1, 114, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 102, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges152[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode153[] = {
  { 0, 0, 0, 0 },
  { 1, 104, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 101, 0, 3 },
  { 1, 114, 0, 4 },
  { 1, 111, 0, 5 },
  { 1, 102, 0, 6 }
};
static const MaiaCCLexRange maiaCCLexRanges153[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode154[] = {
  { 0, 0, 0, 0 },
  { 1, 58, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges154[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode155[] = {
  { 0, 0, 0, 0 },
  { 1, 104, 0, 0 },
  { 1, 99, 0, 1 },
  { 1, 116, 0, 2 },
  { 1, 105, 0, 3 },
  { 1, 119, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges155[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode156[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 99, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges156[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode157[] = {
  { 0, 0, 0, 0 },
  { 1, 116, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 97, 0, 3 },
  { 1, 102, 0, 4 },
  { 1, 101, 0, 5 },
  { 1, 100, 0, 6 }
};
static const MaiaCCLexRange maiaCCLexRanges157[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode158[] = {
  { 0, 0, 0, 0 },
  { 1, 110, 0, 0 },
  { 1, 114, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 116, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 114, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges158[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode159[] = {
  { 0, 0, 0, 0 },
  { 1, 107, 0, 0 },
  { 1, 97, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 114, 0, 3 },
  { 1, 98, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges159[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode160[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 117, 0, 1 },
  { 1, 110, 0, 2 },
  { 1, 105, 0, 3 },
  { 1, 116, 0, 4 },
  { 1, 110, 0, 5 },
  { 1, 111, 0, 6 },
  { 1, 99, 0, 7 }
};
static const MaiaCCLexRange maiaCCLexRanges160[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode161[] = {
  { 0, 0, 0, 0 },
  { 1, 114, 0, 0 },
  { 1, 101, 0, 1 },
  { 1, 102, 0, 2 },
  { 1, 101, 0, 3 },
  { 1, 100, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges161[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode162[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 102, 0, 1 },
  { 1, 97, 0, 2 },
  { 1, 115, 0, 3 },
  { 1, 110, 0, 4 },
  { 1, 117, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges162[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode163[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 43, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges163[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode164[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges164[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode165[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 42, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges165[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode166[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 47, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges166[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode167[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 37, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges167[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode168[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 38, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges168[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode169[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 124, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges169[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode170[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 94, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges170[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode171[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 60, 0, 1 },
  { 1, 60, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges171[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode172[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 62, 0, 1 },
  { 1, 62, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges172[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode173[] = {
  { 0, 0, 0, 0 },
  { 1, 124, 0, 0 },
  { 1, 124, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges173[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode174[] = {
  { 0, 0, 0, 0 },
  { 1, 38, 0, 0 },
  { 1, 38, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges174[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode175[] = {
  { 0, 0, 0, 0 },
  { 1, 124, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges175[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode176[] = {
  { 0, 0, 0, 0 },
  { 1, 94, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges176[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode177[] = {
  { 0, 0, 0, 0 },
  { 1, 38, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges177[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode178[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 61, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges178[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode179[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 33, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges179[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode180[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 60, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges180[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode181[] = {
  { 0, 0, 0, 0 },
  { 1, 61, 0, 0 },
  { 1, 62, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges181[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode182[] = {
  { 0, 0, 0, 0 },
  { 1, 60, 0, 0 },
  { 1, 60, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges182[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode183[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 },
  { 1, 62, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges183[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode184[] = {
  { 0, 0, 0, 0 },
  { 1, 43, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges184[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode185[] = {
  { 0, 0, 0, 0 },
  { 1, 45, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges185[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode186[] = {
  { 0, 0, 0, 0 },
  { 1, 47, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges186[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode187[] = {
  { 0, 0, 0, 0 },
  { 1, 37, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges187[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode188[] = {
  { 0, 0, 0, 0 },
  { 1, 33, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges188[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode189[] = {
  { 0, 0, 0, 0 },
  { 1, 126, 0, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges189[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode190[] = {
  { 0, 0, 0, 0 },
  { 1, 43, 0, 0 },
  { 1, 43, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges190[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode191[] = {
  { 0, 0, 0, 0 },
  { 1, 45, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges191[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode192[] = {
  { 0, 0, 0, 0 },
  { 1, 62, 0, 0 },
  { 1, 45, 0, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges192[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode193[] = {
  { 0, 0, 0, 0 },
  { 1, 119, 0, 0 },
  { 1, 101, 0, 1 },
  { 1, 110, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges193[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode194[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 116, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 108, 0, 3 },
  { 1, 101, 0, 4 },
  { 1, 100, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges194[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode195[] = {
  { 0, 0, 0, 0 },
  { 1, 102, 0, 0 },
  { 1, 111, 0, 1 },
  { 1, 101, 0, 2 },
  { 1, 122, 0, 3 },
  { 1, 105, 0, 4 },
  { 1, 115, 0, 5 }
};
static const MaiaCCLexRange maiaCCLexRanges195[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode196[] = {
  { 0, 0, 0, 0 },
  { 1, 109, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 97, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges196[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode197[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 117, 0, 1 },
  { 1, 114, 0, 2 },
  { 1, 116, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges197[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode198[] = {
  { 0, 0, 0, 0 },
  { 1, 101, 0, 0 },
  { 1, 115, 0, 1 },
  { 1, 108, 0, 2 },
  { 1, 97, 0, 3 },
  { 1, 102, 0, 4 }
};
static const MaiaCCLexRange maiaCCLexRanges198[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode199[] = {
  { 0, 0, 0, 0 },
  { 1, 108, 0, 0 },
  { 1, 108, 0, 1 },
  { 1, 117, 0, 2 },
  { 1, 110, 0, 3 }
};
static const MaiaCCLexRange maiaCCLexRanges199[] = {
  { 0, 0 }
};
static const MaiaCCLexInstruction maiaCCLexCode200[] = {
  { 0, 0, 0, 0 },
  { 2, 2, 0, 0 },
  { 3, 0, 4, 1 },
  { 3, 4, 3, 1 }
};
static const MaiaCCLexRange maiaCCLexRanges200[] = {
  { 65, 90 },
  { 97, 122 },
  { 48, 57 },
  { 95, 95 },
  { 65, 90 },
  { 97, 122 },
  { 95, 95 }
};
static const MaiaCCLexInstruction maiaCCLexCode201[] = {
  { 0, 0, 0, 0 },
  { 1, 70, 0, 0 },
  { 1, 102, 0, 0 },
  { 2, 2, 1, 0 },
  { 2, 3, 0, 0 },
  { 2, 6, 4, 0 },
  { 3, 0, 1, 5 },
  { 1, 45, 0, 6 },
  { 1, 43, 0, 6 },
  { 2, 8, 7, 0 },
  { 2, 9, 6, 0 },
  { 1, 69, 0, 10 },
  { 1, 101, 0, 10 },
  { 2, 12, 11, 0 },
  { 2, 15, 13, 0 },
  { 3, 1, 1, 14 },
  { 1, 70, 0, 0 },
  { 1, 102, 0, 0 },
  { 2, 17, 16, 0 },
  { 2, 18, 0, 0 },
  { 2, 21, 19, 0 },
  { 3, 2, 1, 20 },
  { 1, 45, 0, 21 },
  { 1, 43, 0, 21 },
  { 2, 23, 22, 0 },
  { 2, 24, 21, 0 },
  { 1, 69, 0, 25 },
  { 1, 101, 0, 25 },
  { 2, 27, 26, 0 },
  { 2, 28, 19, 0 },
  { 2, 31, 29, 0 },
  { 3, 3, 1, 30 },
  { 1, 46, 0, 31 },
  { 2, 34, 32, 0 },
  { 3, 4, 1, 33 },
  { 2, 34, 15, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges201[] = {
  { 48, 57 },
  { 48, 57 },
  { 48, 57 },
  { 48, 57 },
  { 48, 57 }
};
static const MaiaCCLexInstruction maiaCCLexCode202[] = {
  { 0, 0, 0, 0 },
  { 3, 0, 2, 0 },
  { 3, 2, 2, 0 },
  { 2, 2, 1, 0 },
  { 2, 3, 0, 0 },
  { 2, 6, 4, 0 },
  { 3, 4, 1, 5 },
  { 2, 8, 4, 0 },
  { 3, 5, 2, 7 },
  { 1, 98, 0, 8 },
  { 1, 48, 0, 9 },
  { 2, 10, 6, 0 },
  { 2, 13, 4, 0 },
  { 3, 7, 3, 12 },
  { 1, 120, 0, 13 },
  { 1, 48, 0, 14 },
  { 2, 15, 11, 0 }
};
static const MaiaCCLexRange maiaCCLexRanges202[] = {
  { 108, 108 },
  { 76, 76 },
  { 117, 117 },
  { 85, 85 },
  { 48, 57 },
  { 48, 48 },
  { 49, 49 },
  { 48, 57 },
  { 65, 70 },
  { 97, 102 }
};
static const MaiaCCLexInstruction maiaCCLexCode203[] = {
  { 0, 0, 0, 0 },
  { 1, 34, 0, 0 },
  { 2, 18, 1, 0 },
  { 4, 0, 4, 2 },
  { 1, 92, 0, 2 },
  { 1, 39, 0, 2 },
  { 2, 5, 4, 0 },
  { 1, 34, 0, 2 },
  { 2, 7, 6, 0 },
  { 1, 48, 0, 2 },
  { 2, 9, 8, 0 },
  { 1, 116, 0, 2 },
  { 2, 11, 10, 0 },
  { 1, 114, 0, 2 },
  { 2, 13, 12, 0 },
  { 1, 110, 0, 2 },
  { 2, 15, 14, 0 },
  { 1, 92, 0, 16 },
  { 2, 17, 3, 0 },
  { 1, 34, 0, 2 }
};
static const MaiaCCLexRange maiaCCLexRanges203[] = {
  { 34, 34 },
  { 92, 92 },
  { 10, 10 },
  { 13, 13 }
};
static const MaiaCCLexInstruction maiaCCLexCode204[] = {
  { 0, 0, 0, 0 },
  { 1, 39, 0, 0 },
  { 4, 0, 4, 1 },
  { 1, 92, 0, 1 },
  { 1, 39, 0, 1 },
  { 2, 4, 3, 0 },
  { 1, 34, 0, 1 },
  { 2, 6, 5, 0 },
  { 1, 48, 0, 1 },
  { 2, 8, 7, 0 },
  { 1, 116, 0, 1 },
  { 2, 10, 9, 0 },
  { 1, 114, 0, 1 },
  { 2, 12, 11, 0 },
  { 1, 110, 0, 1 },
  { 2, 14, 13, 0 },
  { 1, 92, 0, 15 },
  { 2, 16, 2, 0 },
  { 1, 39, 0, 17 }
};
static const MaiaCCLexRange maiaCCLexRanges204[] = {
  { 39, 39 },
  { 92, 92 },
  { 10, 10 },
  { 13, 13 }
};
static const MaiaCCLexPattern maiaCCLexPatterns[] = {
  { maiaCCLexCode0, maiaCCLexRanges0, 7, 6, MAIACC_TOKEN_TOKEN_import, 0, 0, 0, 0, 0 },
  { maiaCCLexCode1, maiaCCLexRanges1, 3, 2, MAIACC_TOKEN_TOKEN_as, 0, 0, 0, 0, 0 },
  { maiaCCLexCode2, maiaCCLexRanges2, 2, 1, MAIACC_TOKEN_TOKEN__3B_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode3, maiaCCLexRanges3, 7, 6, MAIACC_TOKEN_TOKEN_export, 0, 0, 0, 0, 0 },
  { maiaCCLexCode4, maiaCCLexRanges4, 10, 9, MAIACC_TOKEN_TOKEN_namespace, 0, 0, 0, 0, 0 },
  { maiaCCLexCode5, maiaCCLexRanges5, 2, 1, MAIACC_TOKEN_TOKEN__7B_, 0, 0, 0, 4, 0 },
  { maiaCCLexCode6, maiaCCLexRanges6, 2, 1, MAIACC_TOKEN_TOKEN__7D_, 0, 0, 0, 5, 0 },
  { maiaCCLexCode7, maiaCCLexRanges7, 7, 6, MAIACC_TOKEN_TOKEN_struct, 0, 0, 0, 0, 0 },
  { maiaCCLexCode8, maiaCCLexRanges8, 2, 1, MAIACC_TOKEN_TOKEN__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode9, maiaCCLexRanges9, 6, 5, MAIACC_TOKEN_TOKEN_class, 0, 0, 0, 0, 0 },
  { maiaCCLexCode10, maiaCCLexRanges10, 7, 6, MAIACC_TOKEN_TOKEN_public, 0, 0, 0, 0, 0 },
  { maiaCCLexCode11, maiaCCLexRanges11, 8, 7, MAIACC_TOKEN_TOKEN_private, 0, 0, 0, 0, 0 },
  { maiaCCLexCode12, maiaCCLexRanges12, 10, 9, MAIACC_TOKEN_TOKEN_protected, 0, 0, 0, 0, 0 },
  { maiaCCLexCode13, maiaCCLexRanges13, 2, 1, MAIACC_TOKEN_TOKEN__28_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode14, maiaCCLexRanges14, 2, 1, MAIACC_TOKEN_TOKEN__29_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode15, maiaCCLexRanges15, 6, 5, MAIACC_TOKEN_TOKEN_const, 0, 0, 0, 0, 0 },
  { maiaCCLexCode16, maiaCCLexRanges16, 7, 6, MAIACC_TOKEN_TOKEN_extern, 0, 0, 0, 0, 0 },
  { maiaCCLexCode17, maiaCCLexRanges17, 2, 1, MAIACC_TOKEN_TOKEN__2C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode18, maiaCCLexRanges18, 4, 3, MAIACC_TOKEN_TOKEN__2E__2E__2E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode19, maiaCCLexRanges19, 7, 6, MAIACC_TOKEN_TOKEN_static, 0, 0, 0, 0, 0 },
  { maiaCCLexCode20, maiaCCLexRanges20, 7, 6, MAIACC_TOKEN_TOKEN_Result, 0, 0, 0, 0, 0 },
  { maiaCCLexCode21, maiaCCLexRanges21, 2, 1, MAIACC_TOKEN_TOKEN__3C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode22, maiaCCLexRanges22, 2, 1, MAIACC_TOKEN_TOKEN__3E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode23, maiaCCLexRanges23, 7, 6, MAIACC_TOKEN_TOKEN_Option, 0, 0, 0, 0, 0 },
  { maiaCCLexCode24, maiaCCLexRanges24, 5, 4, MAIACC_TOKEN_TOKEN_void, 0, 0, 0, 0, 0 },
  { maiaCCLexCode25, maiaCCLexRanges25, 5, 4, MAIACC_TOKEN_TOKEN_bool, 0, 0, 0, 0, 0 },
  { maiaCCLexCode26, maiaCCLexRanges26, 5, 4, MAIACC_TOKEN_TOKEN_char, 0, 0, 0, 0, 0 },
  { maiaCCLexCode27, maiaCCLexRanges27, 7, 6, MAIACC_TOKEN_TOKEN_string, 0, 0, 0, 0, 0 },
  { maiaCCLexCode28, maiaCCLexRanges28, 3, 2, MAIACC_TOKEN_TOKEN_i8, 0, 0, 0, 0, 0 },
  { maiaCCLexCode29, maiaCCLexRanges29, 4, 3, MAIACC_TOKEN_TOKEN_i16, 0, 0, 0, 0, 0 },
  { maiaCCLexCode30, maiaCCLexRanges30, 4, 3, MAIACC_TOKEN_TOKEN_i32, 0, 0, 0, 0, 0 },
  { maiaCCLexCode31, maiaCCLexRanges31, 4, 3, MAIACC_TOKEN_TOKEN_i64, 0, 0, 0, 0, 0 },
  { maiaCCLexCode32, maiaCCLexRanges32, 3, 2, MAIACC_TOKEN_TOKEN_u8, 0, 0, 0, 0, 0 },
  { maiaCCLexCode33, maiaCCLexRanges33, 4, 3, MAIACC_TOKEN_TOKEN_u16, 0, 0, 0, 0, 0 },
  { maiaCCLexCode34, maiaCCLexRanges34, 4, 3, MAIACC_TOKEN_TOKEN_u32, 0, 0, 0, 0, 0 },
  { maiaCCLexCode35, maiaCCLexRanges35, 4, 3, MAIACC_TOKEN_TOKEN_u64, 0, 0, 0, 0, 0 },
  { maiaCCLexCode36, maiaCCLexRanges36, 4, 3, MAIACC_TOKEN_TOKEN_f32, 0, 0, 0, 0, 0 },
  { maiaCCLexCode37, maiaCCLexRanges37, 4, 3, MAIACC_TOKEN_TOKEN_f64, 0, 0, 0, 0, 0 },
  { maiaCCLexCode38, maiaCCLexRanges38, 2, 1, MAIACC_TOKEN_TOKEN__2A_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode39, maiaCCLexRanges39, 2, 1, MAIACC_TOKEN_TOKEN__5B_, 0, 0, 0, 6, 0 },
  { maiaCCLexCode40, maiaCCLexRanges40, 2, 1, MAIACC_TOKEN_TOKEN__5D_, 0, 0, 1, 7, 0 },
  { maiaCCLexCode41, maiaCCLexRanges41, 2, 1, MAIACC_TOKEN_TOKEN__3F_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode42, maiaCCLexRanges42, 2, 1, MAIACC_TOKEN_TOKEN__2E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode43, maiaCCLexRanges43, 5, 4, MAIACC_TOKEN_TOKEN_auto, 0, 0, 0, 0, 0 },
  { maiaCCLexCode44, maiaCCLexRanges44, 3, 2, MAIACC_TOKEN_TOKEN_if, 0, 0, 0, 0, 0 },
  { maiaCCLexCode45, maiaCCLexRanges45, 5, 4, MAIACC_TOKEN_TOKEN_else, 0, 0, 0, 0, 0 },
  { maiaCCLexCode46, maiaCCLexRanges46, 6, 5, MAIACC_TOKEN_TOKEN_while, 0, 0, 0, 0, 0 },
  { maiaCCLexCode47, maiaCCLexRanges47, 3, 2, MAIACC_TOKEN_TOKEN_do, 0, 0, 0, 0, 0 },
  { maiaCCLexCode48, maiaCCLexRanges48, 4, 3, MAIACC_TOKEN_TOKEN_for, 0, 0, 0, 0, 0 },
  { maiaCCLexCode49, maiaCCLexRanges49, 8, 7, MAIACC_TOKEN_TOKEN_foreach, 0, 0, 0, 0, 0 },
  { maiaCCLexCode50, maiaCCLexRanges50, 2, 1, MAIACC_TOKEN_TOKEN__3A_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode51, maiaCCLexRanges51, 7, 6, MAIACC_TOKEN_TOKEN_switch, 0, 0, 0, 0, 0 },
  { maiaCCLexCode52, maiaCCLexRanges52, 5, 4, MAIACC_TOKEN_TOKEN_case, 0, 0, 0, 0, 0 },
  { maiaCCLexCode53, maiaCCLexRanges53, 8, 7, MAIACC_TOKEN_TOKEN_default, 0, 0, 0, 0, 0 },
  { maiaCCLexCode54, maiaCCLexRanges54, 7, 6, MAIACC_TOKEN_TOKEN_return, 0, 0, 0, 0, 0 },
  { maiaCCLexCode55, maiaCCLexRanges55, 6, 5, MAIACC_TOKEN_TOKEN_break, 0, 0, 0, 0, 0 },
  { maiaCCLexCode56, maiaCCLexRanges56, 9, 8, MAIACC_TOKEN_TOKEN_continue, 0, 0, 0, 0, 0 },
  { maiaCCLexCode57, maiaCCLexRanges57, 6, 5, MAIACC_TOKEN_TOKEN_defer, 0, 0, 0, 0, 0 },
  { maiaCCLexCode58, maiaCCLexRanges58, 7, 6, MAIACC_TOKEN_TOKEN_unsafe, 0, 0, 0, 0, 0 },
  { maiaCCLexCode59, maiaCCLexRanges59, 3, 2, MAIACC_TOKEN_TOKEN__2B__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode60, maiaCCLexRanges60, 3, 2, MAIACC_TOKEN_TOKEN__2D__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode61, maiaCCLexRanges61, 3, 2, MAIACC_TOKEN_TOKEN__2A__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode62, maiaCCLexRanges62, 3, 2, MAIACC_TOKEN_TOKEN__2F__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode63, maiaCCLexRanges63, 3, 2, MAIACC_TOKEN_TOKEN__25__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode64, maiaCCLexRanges64, 3, 2, MAIACC_TOKEN_TOKEN__26__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode65, maiaCCLexRanges65, 3, 2, MAIACC_TOKEN_TOKEN__7C__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode66, maiaCCLexRanges66, 3, 2, MAIACC_TOKEN_TOKEN__5E__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode67, maiaCCLexRanges67, 4, 3, MAIACC_TOKEN_TOKEN__3C__3C__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode68, maiaCCLexRanges68, 4, 3, MAIACC_TOKEN_TOKEN__3E__3E__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode69, maiaCCLexRanges69, 3, 2, MAIACC_TOKEN_TOKEN__7C__7C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode70, maiaCCLexRanges70, 3, 2, MAIACC_TOKEN_TOKEN__26__26_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode71, maiaCCLexRanges71, 2, 1, MAIACC_TOKEN_TOKEN__7C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode72, maiaCCLexRanges72, 2, 1, MAIACC_TOKEN_TOKEN__5E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode73, maiaCCLexRanges73, 2, 1, MAIACC_TOKEN_TOKEN__26_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode74, maiaCCLexRanges74, 3, 2, MAIACC_TOKEN_TOKEN__3D__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode75, maiaCCLexRanges75, 3, 2, MAIACC_TOKEN_TOKEN__21__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode76, maiaCCLexRanges76, 3, 2, MAIACC_TOKEN_TOKEN__3C__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode77, maiaCCLexRanges77, 3, 2, MAIACC_TOKEN_TOKEN__3E__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode78, maiaCCLexRanges78, 3, 2, MAIACC_TOKEN_TOKEN__3C__3C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode79, maiaCCLexRanges79, 3, 2, MAIACC_TOKEN_TOKEN__3E__3E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode80, maiaCCLexRanges80, 2, 1, MAIACC_TOKEN_TOKEN__2B_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode81, maiaCCLexRanges81, 2, 1, MAIACC_TOKEN_TOKEN__2D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode82, maiaCCLexRanges82, 2, 1, MAIACC_TOKEN_TOKEN__2F_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode83, maiaCCLexRanges83, 2, 1, MAIACC_TOKEN_TOKEN__25_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode84, maiaCCLexRanges84, 2, 1, MAIACC_TOKEN_TOKEN__21_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode85, maiaCCLexRanges85, 2, 1, MAIACC_TOKEN_TOKEN__7E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode86, maiaCCLexRanges86, 3, 2, MAIACC_TOKEN_TOKEN__2B__2B_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode87, maiaCCLexRanges87, 3, 2, MAIACC_TOKEN_TOKEN__2D__2D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode88, maiaCCLexRanges88, 3, 2, MAIACC_TOKEN_TOKEN__2D__3E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode89, maiaCCLexRanges89, 4, 3, MAIACC_TOKEN_TOKEN_new, 0, 0, 0, 0, 0 },
  { maiaCCLexCode90, maiaCCLexRanges90, 7, 6, MAIACC_TOKEN_TOKEN_delete, 0, 0, 0, 0, 0 },
  { maiaCCLexCode91, maiaCCLexRanges91, 7, 6, MAIACC_TOKEN_TOKEN_sizeof, 0, 0, 0, 0, 0 },
  { maiaCCLexCode92, maiaCCLexRanges92, 4, 3, MAIACC_TOKEN_TOKEN_asm, 0, 0, 0, 0, 0 },
  { maiaCCLexCode93, maiaCCLexRanges93, 5, 4, MAIACC_TOKEN_TOKEN_true, 0, 0, 0, 0, 0 },
  { maiaCCLexCode94, maiaCCLexRanges94, 6, 5, MAIACC_TOKEN_TOKEN_false, 0, 0, 0, 0, 0 },
  { maiaCCLexCode95, maiaCCLexRanges95, 5, 4, MAIACC_TOKEN_TOKEN_null, 0, 0, 0, 0, 0 },
  { maiaCCLexCode96, maiaCCLexRanges96, 4, 3, MAIACC_TOKEN_NAME, 0, 0, 0, 0, 7 },
  { maiaCCLexCode97, maiaCCLexRanges97, 36, 35, MAIACC_TOKEN_FLOAT, 0, 0, 0, 0, 5 },
  { maiaCCLexCode98, maiaCCLexRanges98, 17, 16, MAIACC_TOKEN_INTEGER, 0, 0, 0, 0, 10 },
  { maiaCCLexCode99, maiaCCLexRanges99, 20, 19, MAIACC_TOKEN_STRING, 0, 0, 0, 0, 4 },
  { maiaCCLexCode100, maiaCCLexRanges100, 19, 18, MAIACC_TOKEN_CHAR, 0, 0, 0, 0, 4 },
  { maiaCCLexCode101, maiaCCLexRanges101, 5, 4, MAIACC_TOKEN_LINE_COMMENT, 1, 0, 0, 0, 2 },
  { maiaCCLexCode102, maiaCCLexRanges102, 10, 9, MAIACC_TOKEN_BLOCK_COMMENT, 1, 0, 0, 0, 2 },
  { maiaCCLexCode103, maiaCCLexRanges103, 3, 2, MAIACC_TOKEN_WS, 1, 0, 0, 0, 4 },
  { maiaCCLexCode104, maiaCCLexRanges104, 7, 6, MAIACC_TOKEN_TOKEN_import, 0, 0, 0, 0, 0 },
  { maiaCCLexCode105, maiaCCLexRanges105, 3, 2, MAIACC_TOKEN_TOKEN_as, 0, 0, 0, 0, 0 },
  { maiaCCLexCode106, maiaCCLexRanges106, 2, 1, MAIACC_TOKEN_TOKEN__3B_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode107, maiaCCLexRanges107, 7, 6, MAIACC_TOKEN_TOKEN_export, 0, 0, 0, 0, 0 },
  { maiaCCLexCode108, maiaCCLexRanges108, 10, 9, MAIACC_TOKEN_TOKEN_namespace, 0, 0, 0, 0, 0 },
  { maiaCCLexCode109, maiaCCLexRanges109, 2, 1, MAIACC_TOKEN_TOKEN__7B_, 0, 0, 0, 4, 0 },
  { maiaCCLexCode110, maiaCCLexRanges110, 2, 1, MAIACC_TOKEN_TOKEN__7D_, 0, 0, 0, 5, 0 },
  { maiaCCLexCode111, maiaCCLexRanges111, 7, 6, MAIACC_TOKEN_TOKEN_struct, 0, 0, 0, 0, 0 },
  { maiaCCLexCode112, maiaCCLexRanges112, 2, 1, MAIACC_TOKEN_TOKEN__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode113, maiaCCLexRanges113, 6, 5, MAIACC_TOKEN_TOKEN_class, 0, 0, 0, 0, 0 },
  { maiaCCLexCode114, maiaCCLexRanges114, 7, 6, MAIACC_TOKEN_TOKEN_public, 0, 0, 0, 0, 0 },
  { maiaCCLexCode115, maiaCCLexRanges115, 8, 7, MAIACC_TOKEN_TOKEN_private, 0, 0, 0, 0, 0 },
  { maiaCCLexCode116, maiaCCLexRanges116, 10, 9, MAIACC_TOKEN_TOKEN_protected, 0, 0, 0, 0, 0 },
  { maiaCCLexCode117, maiaCCLexRanges117, 2, 1, MAIACC_TOKEN_TOKEN__28_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode118, maiaCCLexRanges118, 2, 1, MAIACC_TOKEN_TOKEN__29_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode119, maiaCCLexRanges119, 6, 5, MAIACC_TOKEN_TOKEN_const, 0, 0, 0, 0, 0 },
  { maiaCCLexCode120, maiaCCLexRanges120, 7, 6, MAIACC_TOKEN_TOKEN_extern, 0, 0, 0, 0, 0 },
  { maiaCCLexCode121, maiaCCLexRanges121, 2, 1, MAIACC_TOKEN_TOKEN__2C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode122, maiaCCLexRanges122, 4, 3, MAIACC_TOKEN_TOKEN__2E__2E__2E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode123, maiaCCLexRanges123, 7, 6, MAIACC_TOKEN_TOKEN_static, 0, 0, 0, 0, 0 },
  { maiaCCLexCode124, maiaCCLexRanges124, 7, 6, MAIACC_TOKEN_TOKEN_Result, 0, 0, 0, 0, 0 },
  { maiaCCLexCode125, maiaCCLexRanges125, 2, 1, MAIACC_TOKEN_TOKEN__3C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode126, maiaCCLexRanges126, 2, 1, MAIACC_TOKEN_TOKEN__3E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode127, maiaCCLexRanges127, 7, 6, MAIACC_TOKEN_TOKEN_Option, 0, 0, 0, 0, 0 },
  { maiaCCLexCode128, maiaCCLexRanges128, 5, 4, MAIACC_TOKEN_TOKEN_void, 0, 0, 0, 0, 0 },
  { maiaCCLexCode129, maiaCCLexRanges129, 5, 4, MAIACC_TOKEN_TOKEN_bool, 0, 0, 0, 0, 0 },
  { maiaCCLexCode130, maiaCCLexRanges130, 5, 4, MAIACC_TOKEN_TOKEN_char, 0, 0, 0, 0, 0 },
  { maiaCCLexCode131, maiaCCLexRanges131, 7, 6, MAIACC_TOKEN_TOKEN_string, 0, 0, 0, 0, 0 },
  { maiaCCLexCode132, maiaCCLexRanges132, 3, 2, MAIACC_TOKEN_TOKEN_i8, 0, 0, 0, 0, 0 },
  { maiaCCLexCode133, maiaCCLexRanges133, 4, 3, MAIACC_TOKEN_TOKEN_i16, 0, 0, 0, 0, 0 },
  { maiaCCLexCode134, maiaCCLexRanges134, 4, 3, MAIACC_TOKEN_TOKEN_i32, 0, 0, 0, 0, 0 },
  { maiaCCLexCode135, maiaCCLexRanges135, 4, 3, MAIACC_TOKEN_TOKEN_i64, 0, 0, 0, 0, 0 },
  { maiaCCLexCode136, maiaCCLexRanges136, 3, 2, MAIACC_TOKEN_TOKEN_u8, 0, 0, 0, 0, 0 },
  { maiaCCLexCode137, maiaCCLexRanges137, 4, 3, MAIACC_TOKEN_TOKEN_u16, 0, 0, 0, 0, 0 },
  { maiaCCLexCode138, maiaCCLexRanges138, 4, 3, MAIACC_TOKEN_TOKEN_u32, 0, 0, 0, 0, 0 },
  { maiaCCLexCode139, maiaCCLexRanges139, 4, 3, MAIACC_TOKEN_TOKEN_u64, 0, 0, 0, 0, 0 },
  { maiaCCLexCode140, maiaCCLexRanges140, 4, 3, MAIACC_TOKEN_TOKEN_f32, 0, 0, 0, 0, 0 },
  { maiaCCLexCode141, maiaCCLexRanges141, 4, 3, MAIACC_TOKEN_TOKEN_f64, 0, 0, 0, 0, 0 },
  { maiaCCLexCode142, maiaCCLexRanges142, 2, 1, MAIACC_TOKEN_TOKEN__2A_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode143, maiaCCLexRanges143, 2, 1, MAIACC_TOKEN_TOKEN__5B_, 0, 0, 0, 6, 0 },
  { maiaCCLexCode144, maiaCCLexRanges144, 2, 1, MAIACC_TOKEN_TOKEN__5D_, 0, 0, 1, 7, 0 },
  { maiaCCLexCode145, maiaCCLexRanges145, 2, 1, MAIACC_TOKEN_TOKEN__3F_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode146, maiaCCLexRanges146, 2, 1, MAIACC_TOKEN_TOKEN__2E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode147, maiaCCLexRanges147, 5, 4, MAIACC_TOKEN_TOKEN_auto, 0, 0, 0, 0, 0 },
  { maiaCCLexCode148, maiaCCLexRanges148, 3, 2, MAIACC_TOKEN_TOKEN_if, 0, 0, 0, 0, 0 },
  { maiaCCLexCode149, maiaCCLexRanges149, 5, 4, MAIACC_TOKEN_TOKEN_else, 0, 0, 0, 0, 0 },
  { maiaCCLexCode150, maiaCCLexRanges150, 6, 5, MAIACC_TOKEN_TOKEN_while, 0, 0, 0, 0, 0 },
  { maiaCCLexCode151, maiaCCLexRanges151, 3, 2, MAIACC_TOKEN_TOKEN_do, 0, 0, 0, 0, 0 },
  { maiaCCLexCode152, maiaCCLexRanges152, 4, 3, MAIACC_TOKEN_TOKEN_for, 0, 0, 0, 0, 0 },
  { maiaCCLexCode153, maiaCCLexRanges153, 8, 7, MAIACC_TOKEN_TOKEN_foreach, 0, 0, 0, 0, 0 },
  { maiaCCLexCode154, maiaCCLexRanges154, 2, 1, MAIACC_TOKEN_TOKEN__3A_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode155, maiaCCLexRanges155, 7, 6, MAIACC_TOKEN_TOKEN_switch, 0, 0, 0, 0, 0 },
  { maiaCCLexCode156, maiaCCLexRanges156, 5, 4, MAIACC_TOKEN_TOKEN_case, 0, 0, 0, 0, 0 },
  { maiaCCLexCode157, maiaCCLexRanges157, 8, 7, MAIACC_TOKEN_TOKEN_default, 0, 0, 0, 0, 0 },
  { maiaCCLexCode158, maiaCCLexRanges158, 7, 6, MAIACC_TOKEN_TOKEN_return, 0, 0, 0, 0, 0 },
  { maiaCCLexCode159, maiaCCLexRanges159, 6, 5, MAIACC_TOKEN_TOKEN_break, 0, 0, 0, 0, 0 },
  { maiaCCLexCode160, maiaCCLexRanges160, 9, 8, MAIACC_TOKEN_TOKEN_continue, 0, 0, 0, 0, 0 },
  { maiaCCLexCode161, maiaCCLexRanges161, 6, 5, MAIACC_TOKEN_TOKEN_defer, 0, 0, 0, 0, 0 },
  { maiaCCLexCode162, maiaCCLexRanges162, 7, 6, MAIACC_TOKEN_TOKEN_unsafe, 0, 0, 0, 0, 0 },
  { maiaCCLexCode163, maiaCCLexRanges163, 3, 2, MAIACC_TOKEN_TOKEN__2B__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode164, maiaCCLexRanges164, 3, 2, MAIACC_TOKEN_TOKEN__2D__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode165, maiaCCLexRanges165, 3, 2, MAIACC_TOKEN_TOKEN__2A__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode166, maiaCCLexRanges166, 3, 2, MAIACC_TOKEN_TOKEN__2F__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode167, maiaCCLexRanges167, 3, 2, MAIACC_TOKEN_TOKEN__25__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode168, maiaCCLexRanges168, 3, 2, MAIACC_TOKEN_TOKEN__26__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode169, maiaCCLexRanges169, 3, 2, MAIACC_TOKEN_TOKEN__7C__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode170, maiaCCLexRanges170, 3, 2, MAIACC_TOKEN_TOKEN__5E__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode171, maiaCCLexRanges171, 4, 3, MAIACC_TOKEN_TOKEN__3C__3C__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode172, maiaCCLexRanges172, 4, 3, MAIACC_TOKEN_TOKEN__3E__3E__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode173, maiaCCLexRanges173, 3, 2, MAIACC_TOKEN_TOKEN__7C__7C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode174, maiaCCLexRanges174, 3, 2, MAIACC_TOKEN_TOKEN__26__26_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode175, maiaCCLexRanges175, 2, 1, MAIACC_TOKEN_TOKEN__7C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode176, maiaCCLexRanges176, 2, 1, MAIACC_TOKEN_TOKEN__5E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode177, maiaCCLexRanges177, 2, 1, MAIACC_TOKEN_TOKEN__26_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode178, maiaCCLexRanges178, 3, 2, MAIACC_TOKEN_TOKEN__3D__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode179, maiaCCLexRanges179, 3, 2, MAIACC_TOKEN_TOKEN__21__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode180, maiaCCLexRanges180, 3, 2, MAIACC_TOKEN_TOKEN__3C__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode181, maiaCCLexRanges181, 3, 2, MAIACC_TOKEN_TOKEN__3E__3D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode182, maiaCCLexRanges182, 3, 2, MAIACC_TOKEN_TOKEN__3C__3C_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode183, maiaCCLexRanges183, 3, 2, MAIACC_TOKEN_TOKEN__3E__3E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode184, maiaCCLexRanges184, 2, 1, MAIACC_TOKEN_TOKEN__2B_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode185, maiaCCLexRanges185, 2, 1, MAIACC_TOKEN_TOKEN__2D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode186, maiaCCLexRanges186, 2, 1, MAIACC_TOKEN_TOKEN__2F_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode187, maiaCCLexRanges187, 2, 1, MAIACC_TOKEN_TOKEN__25_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode188, maiaCCLexRanges188, 2, 1, MAIACC_TOKEN_TOKEN__21_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode189, maiaCCLexRanges189, 2, 1, MAIACC_TOKEN_TOKEN__7E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode190, maiaCCLexRanges190, 3, 2, MAIACC_TOKEN_TOKEN__2B__2B_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode191, maiaCCLexRanges191, 3, 2, MAIACC_TOKEN_TOKEN__2D__2D_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode192, maiaCCLexRanges192, 3, 2, MAIACC_TOKEN_TOKEN__2D__3E_, 0, 0, 0, 0, 0 },
  { maiaCCLexCode193, maiaCCLexRanges193, 4, 3, MAIACC_TOKEN_TOKEN_new, 0, 0, 0, 0, 0 },
  { maiaCCLexCode194, maiaCCLexRanges194, 7, 6, MAIACC_TOKEN_TOKEN_delete, 0, 0, 0, 0, 0 },
  { maiaCCLexCode195, maiaCCLexRanges195, 7, 6, MAIACC_TOKEN_TOKEN_sizeof, 0, 0, 0, 0, 0 },
  { maiaCCLexCode196, maiaCCLexRanges196, 4, 3, MAIACC_TOKEN_TOKEN_asm, 0, 0, 0, 0, 0 },
  { maiaCCLexCode197, maiaCCLexRanges197, 5, 4, MAIACC_TOKEN_TOKEN_true, 0, 0, 0, 0, 0 },
  { maiaCCLexCode198, maiaCCLexRanges198, 6, 5, MAIACC_TOKEN_TOKEN_false, 0, 0, 0, 0, 0 },
  { maiaCCLexCode199, maiaCCLexRanges199, 5, 4, MAIACC_TOKEN_TOKEN_null, 0, 0, 0, 0, 0 },
  { maiaCCLexCode200, maiaCCLexRanges200, 4, 3, MAIACC_TOKEN_NAME, 0, 0, 0, 0, 7 },
  { maiaCCLexCode201, maiaCCLexRanges201, 36, 35, MAIACC_TOKEN_FLOAT, 0, 0, 0, 0, 5 },
  { maiaCCLexCode202, maiaCCLexRanges202, 17, 16, MAIACC_TOKEN_INTEGER, 0, 0, 0, 0, 10 },
  { maiaCCLexCode203, maiaCCLexRanges203, 20, 19, MAIACC_TOKEN_STRING, 0, 0, 0, 0, 4 },
  { maiaCCLexCode204, maiaCCLexRanges204, 19, 18, MAIACC_TOKEN_CHAR, 0, 0, 0, 0, 4 }
};
static const unsigned int maiaCCLexPatternCount = 104;
static const unsigned int maiaCCLexContextCount = 205;

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

static int maiaCC_parse_Program(MaiaCCParserState* self);
static int maiaCC_parse_TopLevel(MaiaCCParserState* self);
static int maiaCC_parse_ImportDecl(MaiaCCParserState* self);
static int maiaCC_parse_ExportDecl(MaiaCCParserState* self);
static int maiaCC_parse_NamespaceDecl(MaiaCCParserState* self);
static int maiaCC_parse_StructDecl(MaiaCCParserState* self);
static int maiaCC_parse_FieldDecl(MaiaCCParserState* self);
static int maiaCC_parse_ClassDecl(MaiaCCParserState* self);
static int maiaCC_parse_ClassMember(MaiaCCParserState* self);
static int maiaCC_parse_Visibility(MaiaCCParserState* self);
static int maiaCC_parse_MethodDecl(MaiaCCParserState* self);
static int maiaCC_parse_ConstructorDecl(MaiaCCParserState* self);
static int maiaCC_parse_FunctionDecl(MaiaCCParserState* self);
static int maiaCC_parse_ExternDecl(MaiaCCParserState* self);
static int maiaCC_parse_GlobalDecl(MaiaCCParserState* self);
static int maiaCC_parse_ParameterList(MaiaCCParserState* self);
static int maiaCC_parse_Parameter(MaiaCCParserState* self);
static int maiaCC_parse_Type(MaiaCCParserState* self);
static int maiaCC_parse_TypeAtom(MaiaCCParserState* self);
static int maiaCC_parse_PrimitiveType(MaiaCCParserState* self);
static int maiaCC_parse_TypeSuffix(MaiaCCParserState* self);
static int maiaCC_parse_QualifiedName(MaiaCCParserState* self);
static int maiaCC_parse_Block(MaiaCCParserState* self);
static int maiaCC_parse_Statement(MaiaCCParserState* self);
static int maiaCC_parse_LocalDecl(MaiaCCParserState* self);
static int maiaCC_parse_IfStmt(MaiaCCParserState* self);
static int maiaCC_parse_WhileStmt(MaiaCCParserState* self);
static int maiaCC_parse_DoStmt(MaiaCCParserState* self);
static int maiaCC_parse_ForStmt(MaiaCCParserState* self);
static int maiaCC_parse_ForInit(MaiaCCParserState* self);
static int maiaCC_parse_LocalInit(MaiaCCParserState* self);
static int maiaCC_parse_ForeachStmt(MaiaCCParserState* self);
static int maiaCC_parse_SwitchStmt(MaiaCCParserState* self);
static int maiaCC_parse_SwitchArm(MaiaCCParserState* self);
static int maiaCC_parse_DefaultArm(MaiaCCParserState* self);
static int maiaCC_parse_ReturnStmt(MaiaCCParserState* self);
static int maiaCC_parse_BreakStmt(MaiaCCParserState* self);
static int maiaCC_parse_ContinueStmt(MaiaCCParserState* self);
static int maiaCC_parse_DeferStmt(MaiaCCParserState* self);
static int maiaCC_parse_UnsafeStmt(MaiaCCParserState* self);
static int maiaCC_parse_ExpressionStmt(MaiaCCParserState* self);
static int maiaCC_parse_Expression(MaiaCCParserState* self);
static int maiaCC_parse_Assignment(MaiaCCParserState* self);
static int maiaCC_parse_AssignOp(MaiaCCParserState* self);
static int maiaCC_parse_Conditional(MaiaCCParserState* self);
static int maiaCC_parse_LogicalOr(MaiaCCParserState* self);
static int maiaCC_parse_LogicalAnd(MaiaCCParserState* self);
static int maiaCC_parse_BitwiseOr(MaiaCCParserState* self);
static int maiaCC_parse_BitwiseXor(MaiaCCParserState* self);
static int maiaCC_parse_BitwiseAnd(MaiaCCParserState* self);
static int maiaCC_parse_Equality(MaiaCCParserState* self);
static int maiaCC_parse_Relational(MaiaCCParserState* self);
static int maiaCC_parse_Shift(MaiaCCParserState* self);
static int maiaCC_parse_Additive(MaiaCCParserState* self);
static int maiaCC_parse_Multiplicative(MaiaCCParserState* self);
static int maiaCC_parse_Unary(MaiaCCParserState* self);
static int maiaCC_parse_Postfix(MaiaCCParserState* self);
static int isTernaryQuestion(MaiaCCParserState* self);
static int maiaCC_parse_PostfixTail(MaiaCCParserState* self);
static int maiaCC_parse_ArgumentList(MaiaCCParserState* self);
static int maiaCC_parse_Primary(MaiaCCParserState* self);
static int maiaCC_parse_Literal(MaiaCCParserState* self);


static int maiaCC_parse(MaiaCCParserState* self) {
  self->position = 0;
  self->failed = 0;
  self->resourceError = MAIACC_PARSE_RESOURCE_NONE;
  self->errorCount = 0;
  size_t columns = (size_t)self->tokenCount + 1;
  if (columns == 0 || MAIACC_RULE_COUNT > ((size_t)-1) / columns) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_LIMIT; return 0; }
  self->failureMemo = (unsigned char*)calloc(MAIACC_RULE_COUNT, columns);
  if (!self->failureMemo) { self->failed = 1; self->resourceError = MAIACC_PARSE_RESOURCE_NOMEM; return 0; }
  int accepted = maiaCC_parse_Program(self) && !self->failed;
  if (accepted) accepted = self->position == self->tokenCount
      || (self->position + 1 == self->tokenCount
          && maiaCC_peek(self)->type == MAIACC_TOKEN_EOF);
  free(self->failureMemo);
  self->failureMemo = 0;
  return accepted;
}


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

static int maiaCC_parse_Program(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)0 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Program");
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_TopLevel(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_EOF);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Program");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Program");
  return 0;
}

static int maiaCC_parse_TopLevel(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)1 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "TopLevel");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ImportDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ExportDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_NamespaceDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StructDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ClassDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_FunctionDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_GlobalDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ExternDecl(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "TopLevel");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "TopLevel");
  return 0;
}

static int maiaCC_parse_ImportDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)2 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ImportDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_import);
    if (!self->failed && !maiaCC_parse_QualifiedName(self)) self->failed = 1;
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_as);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ImportDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ImportDecl");
  return 0;
}

static int maiaCC_parse_ExportDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)3 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ExportDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_export);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_StructDecl(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_ClassDecl(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_FunctionDecl(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_GlobalDecl(self)) self->failed = 1;
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
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ExportDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ExportDecl");
  return 0;
}

static int maiaCC_parse_NamespaceDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)4 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "NamespaceDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_namespace);
    if (!self->failed && !maiaCC_parse_QualifiedName(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7B_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_TopLevel(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7D_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "NamespaceDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "NamespaceDecl");
  return 0;
}

static int maiaCC_parse_StructDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)5 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "StructDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_struct);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7B_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_FieldDecl(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7D_);
    maiaCC_matchToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "StructDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "StructDecl");
  return 0;
}

static int maiaCC_parse_FieldDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)6 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "FieldDecl");
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "FieldDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "FieldDecl");
  return 0;
}

static int maiaCC_parse_ClassDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)7 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ClassDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_class);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7B_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ClassMember(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7D_);
    maiaCC_matchToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ClassDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ClassDecl");
  return 0;
}

static int maiaCC_parse_ClassMember(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)8 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ClassMember");
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Visibility(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_FieldDecl(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_MethodDecl(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_ConstructorDecl(self)) self->failed = 1;
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
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ClassMember");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ClassMember");
  return 0;
}

static int maiaCC_parse_Visibility(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)9 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Visibility");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_public);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_private);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_protected);
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
    maiaCC_endNonterminal(self, "Visibility");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Visibility");
  return 0;
}

static int maiaCC_parse_MethodDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)10 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "MethodDecl");
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ParameterList(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    maiaCC_matchToken(self, MAIACC_TOKEN_TOKEN_const);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "MethodDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "MethodDecl");
  return 0;
}

static int maiaCC_parse_ConstructorDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)11 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ConstructorDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ParameterList(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ConstructorDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ConstructorDecl");
  return 0;
}

static int maiaCC_parse_FunctionDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)12 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "FunctionDecl");
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ParameterList(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "FunctionDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "FunctionDecl");
  return 0;
}

static int maiaCC_parse_ExternDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)13 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ExternDecl");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_extern);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ParameterList(self)) self->failed = 1;
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2C_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2E__2E__2E_);
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2E__2E__2E_);
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
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ExternDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ExternDecl");
  return 0;
}

static int maiaCC_parse_GlobalDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)14 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "GlobalDecl");
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_const);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_static);
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
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "GlobalDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "GlobalDecl");
  return 0;
}

static int maiaCC_parse_ParameterList(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)15 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ParameterList");
    if (!self->failed && !maiaCC_parse_Parameter(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2C_);
    if (!self->failed && !maiaCC_parse_Parameter(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ParameterList");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ParameterList");
  return 0;
}

static int maiaCC_parse_Parameter(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)16 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Parameter");
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Parameter");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Parameter");
  return 0;
}

static int maiaCC_parse_Type(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)17 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Type");
    if (!self->failed && !maiaCC_parse_TypeAtom(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_TypeSuffix(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Type");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Type");
  return 0;
}

static int maiaCC_parse_TypeAtom(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)18 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "TypeAtom");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_PrimitiveType(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_QualifiedName(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_Result);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3C_);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2C_);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3E_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_Option);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3C_);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3E_);
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
    maiaCC_endNonterminal(self, "TypeAtom");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "TypeAtom");
  return 0;
}

static int maiaCC_parse_PrimitiveType(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)19 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "PrimitiveType");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_void);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_bool);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_char);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_string);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_i8);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_i16);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_i32);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_i64);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_u8);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_u16);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_u32);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_u64);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_f32);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_f64);
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
    maiaCC_endNonterminal(self, "PrimitiveType");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "PrimitiveType");
  return 0;
}

static int maiaCC_parse_TypeSuffix(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)20 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "TypeSuffix");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2A_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__5B_);
    maiaCC_matchToken(self, MAIACC_TOKEN_INTEGER);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__5D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed && !isTernaryQuestion(self)) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3F_);
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
    maiaCC_endNonterminal(self, "TypeSuffix");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "TypeSuffix");
  return 0;
}

static int maiaCC_parse_QualifiedName(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)21 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "QualifiedName");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2E_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "QualifiedName");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "QualifiedName");
  return 0;
}

static int maiaCC_parse_Block(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)22 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Block");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7B_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Statement(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7D_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Block");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Block");
  return 0;
}

static int maiaCC_parse_Statement(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)23 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Statement");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_LocalDecl(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_IfStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_WhileStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DoStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ForStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ForeachStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_SwitchStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ReturnStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_BreakStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ContinueStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_DeferStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_UnsafeStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_ExpressionStmt(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
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
    maiaCC_endNonterminal(self, "Statement");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Statement");
  return 0;
}

static int maiaCC_parse_LocalDecl(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)24 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LocalDecl");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_const);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_auto);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
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
    maiaCC_endNonterminal(self, "LocalDecl");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LocalDecl");
  return 0;
}

static int maiaCC_parse_IfStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)25 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "IfStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_if);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_else);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_IfStmt(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
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
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "IfStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "IfStmt");
  return 0;
}

static int maiaCC_parse_WhileStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)26 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "WhileStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_while);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "WhileStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "WhileStmt");
  return 0;
}

static int maiaCC_parse_DoStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)27 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DoStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_do);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_while);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DoStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DoStmt");
  return 0;
}

static int maiaCC_parse_ForStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)28 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ForStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_for);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ForInit(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Expression(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Expression(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ForStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ForStmt");
  return 0;
}

static int maiaCC_parse_ForInit(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)29 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ForInit");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_LocalInit(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "ForInit");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ForInit");
  return 0;
}

static int maiaCC_parse_LocalInit(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)30 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LocalInit");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_const);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_auto);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
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
    maiaCC_endNonterminal(self, "LocalInit");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LocalInit");
  return 0;
}

static int maiaCC_parse_ForeachStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)31 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ForeachStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_foreach);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3A_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ForeachStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ForeachStmt");
  return 0;
}

static int maiaCC_parse_SwitchStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)32 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "SwitchStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_switch);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7B_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_SwitchArm(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_DefaultArm(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7D_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "SwitchStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "SwitchStmt");
  return 0;
}

static int maiaCC_parse_SwitchArm(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)33 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "SwitchArm");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_case);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3A_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Statement(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "SwitchArm");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "SwitchArm");
  return 0;
}

static int maiaCC_parse_DefaultArm(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)34 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DefaultArm");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_default);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3A_);
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Statement(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DefaultArm");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DefaultArm");
  return 0;
}

static int maiaCC_parse_ReturnStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)35 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ReturnStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_return);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Expression(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ReturnStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ReturnStmt");
  return 0;
}

static int maiaCC_parse_BreakStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)36 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BreakStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_break);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BreakStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BreakStmt");
  return 0;
}

static int maiaCC_parse_ContinueStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)37 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ContinueStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_continue);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ContinueStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ContinueStmt");
  return 0;
}

static int maiaCC_parse_DeferStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)38 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "DeferStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_defer);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
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
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
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
  if (!self->failed) {
    maiaCC_endNonterminal(self, "DeferStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "DeferStmt");
  return 0;
}

static int maiaCC_parse_UnsafeStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)39 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "UnsafeStmt");
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_unsafe);
    if (!self->failed && !maiaCC_parse_Block(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "UnsafeStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "UnsafeStmt");
  return 0;
}

static int maiaCC_parse_ExpressionStmt(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)40 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ExpressionStmt");
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3B_);
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ExpressionStmt");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ExpressionStmt");
  return 0;
}

static int maiaCC_parse_Expression(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)41 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Expression");
    if (!self->failed && !maiaCC_parse_Assignment(self)) self->failed = 1;
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Expression");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Expression");
  return 0;
}

static int maiaCC_parse_Assignment(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)42 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Assignment");
    if (!self->failed && !maiaCC_parse_Conditional(self)) self->failed = 1;
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_AssignOp(self)) self->failed = 1;
    if (!self->failed && !maiaCC_parse_Assignment(self)) self->failed = 1;
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Assignment");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Assignment");
  return 0;
}

static int maiaCC_parse_AssignOp(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)43 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "AssignOp");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2B__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2D__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2A__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2F__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__25__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__26__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7C__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__5E__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3C__3C__3D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3E__3E__3D_);
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
    maiaCC_endNonterminal(self, "AssignOp");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "AssignOp");
  return 0;
}

static int maiaCC_parse_Conditional(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)44 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Conditional");
    if (!self->failed && !maiaCC_parse_LogicalOr(self)) self->failed = 1;
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3F_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3A_);
    if (!self->failed && !maiaCC_parse_Conditional(self)) self->failed = 1;
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Conditional");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Conditional");
  return 0;
}

static int maiaCC_parse_LogicalOr(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)45 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LogicalOr");
    if (!self->failed && !maiaCC_parse_LogicalAnd(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7C__7C_);
    if (!self->failed && !maiaCC_parse_LogicalAnd(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "LogicalOr");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LogicalOr");
  return 0;
}

static int maiaCC_parse_LogicalAnd(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)46 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "LogicalAnd");
    if (!self->failed && !maiaCC_parse_BitwiseOr(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__26__26_);
    if (!self->failed && !maiaCC_parse_BitwiseOr(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "LogicalAnd");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "LogicalAnd");
  return 0;
}

static int maiaCC_parse_BitwiseOr(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)47 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BitwiseOr");
    if (!self->failed && !maiaCC_parse_BitwiseXor(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7C_);
    if (!self->failed && !maiaCC_parse_BitwiseXor(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BitwiseOr");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BitwiseOr");
  return 0;
}

static int maiaCC_parse_BitwiseXor(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)48 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BitwiseXor");
    if (!self->failed && !maiaCC_parse_BitwiseAnd(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__5E_);
    if (!self->failed && !maiaCC_parse_BitwiseAnd(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BitwiseXor");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BitwiseXor");
  return 0;
}

static int maiaCC_parse_BitwiseAnd(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)49 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "BitwiseAnd");
    if (!self->failed && !maiaCC_parse_Equality(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__26_);
    if (!self->failed && !maiaCC_parse_Equality(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "BitwiseAnd");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "BitwiseAnd");
  return 0;
}

static int maiaCC_parse_Equality(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)50 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Equality");
    if (!self->failed && !maiaCC_parse_Relational(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3D__3D_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__21__3D_);
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
    if (!self->failed && !maiaCC_parse_Relational(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Equality");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Equality");
  return 0;
}

static int maiaCC_parse_Relational(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)51 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Relational");
    if (!self->failed && !maiaCC_parse_Shift(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3C_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3E_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3C__3D_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3E__3D_);
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
    if (!self->failed && !maiaCC_parse_Shift(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Relational");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Relational");
  return 0;
}

static int maiaCC_parse_Shift(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)52 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Shift");
    if (!self->failed && !maiaCC_parse_Additive(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3C__3C_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3E__3E_);
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
    if (!self->failed && !maiaCC_parse_Additive(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Shift");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Shift");
  return 0;
}

static int maiaCC_parse_Additive(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)53 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Additive");
    if (!self->failed && !maiaCC_parse_Multiplicative(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2B_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2D_);
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
    if (!self->failed && !maiaCC_parse_Multiplicative(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Additive");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Additive");
  return 0;
}

static int maiaCC_parse_Multiplicative(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)54 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Multiplicative");
    if (!self->failed && !maiaCC_parse_Unary(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2A_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2F_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__25_);
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
    if (!self->failed && !maiaCC_parse_Unary(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Multiplicative");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Multiplicative");
  return 0;
}

static int maiaCC_parse_Unary(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)55 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Unary");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    {
      int _matchedAlternative = 0;
      if (!_matchedAlternative && !self->failed) {
        const unsigned int _alternativeStart = self->position;
        const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__21_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__7E_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2B_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2D_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2A_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__26_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2B__2B_);
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
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2D__2D_);
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
    if (!self->failed && !maiaCC_parse_Unary(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Postfix(self)) self->failed = 1;
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
    maiaCC_endNonterminal(self, "Unary");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Unary");
  return 0;
}

static int maiaCC_parse_Postfix(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)56 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Postfix");
    if (!self->failed && !maiaCC_parse_Primary(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_PostfixTail(self)) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "Postfix");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Postfix");
  return 0;
}

static int maiaCC_parse_PostfixTail(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)57 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "PostfixTail");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ArgumentList(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__5B_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3A_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_Expression(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
      if (self->failed) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__5D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2E_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2D__3E_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2B__2B_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2D__2D_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed && !isTernaryQuestion(self)) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__3F_);
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
    maiaCC_endNonterminal(self, "PostfixTail");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "PostfixTail");
  return 0;
}

static int maiaCC_parse_ArgumentList(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)58 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "ArgumentList");
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    while (!self->failed) {
      const unsigned int _iterationStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__2C_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
      if (self->failed) {
        self->position = _iterationStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
        break;
      }
      if (self->position == _iterationStart) break;
    }
  if (!self->failed) {
    maiaCC_endNonterminal(self, "ArgumentList");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "ArgumentList");
  return 0;
}

static int maiaCC_parse_Primary(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)59 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Primary");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed && !maiaCC_parse_Literal(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_NAME);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Expression(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_new);
    if (!self->failed && !maiaCC_parse_QualifiedName(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) {
      const unsigned int _optionalStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
      if (!maiaCC_parse_ArgumentList(self)) {
        self->position = _optionalStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_delete);
    if (!self->failed && !maiaCC_parse_Unary(self)) self->failed = 1;
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_sizeof);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed && !maiaCC_parse_Type(self)) self->failed = 1;
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_asm);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__28_);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_STRING);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN__29_);
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
    maiaCC_endNonterminal(self, "Primary");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Primary");
  return 0;
}

static int maiaCC_parse_Literal(MaiaCCParserState* self) {
  if (self->failed) return 0;
  const unsigned int __ruleStart = self->position;
  const size_t __memoSlot = (size_t)60 * ((size_t)self->tokenCount + 1) + __ruleStart;
  if (self->failureMemo && self->failureMemo[__memoSlot]) { self->failed = 1; return 0; }
  const unsigned int __eventMark = maiaCC_checkpointEvents(self);
  maiaCC_startNonterminal(self, "Literal");
    int _matched = 0;
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_INTEGER);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_FLOAT);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_STRING);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_CHAR);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_true);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_false);
      if (!self->failed) {
        _matched = 1;
      } else {
        self->position = _alternativeStart;
        maiaCC_restoreEvents(self, _eventMark);
        self->failed = 0;
      }
    }
    if (!_matched && !self->failed) {
      const unsigned int _alternativeStart = self->position;
      const unsigned int _eventMark = maiaCC_checkpointEvents(self);
    if (!self->failed) maiaCC_expectToken(self, MAIACC_TOKEN_TOKEN_null);
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
    maiaCC_endNonterminal(self, "Literal");
    return 1;
  }
  self->position = __ruleStart;
  if (self->failureMemo) self->failureMemo[__memoSlot] = 1;
  maiaCC_restoreEvents(self, __eventMark);
  maiaCC_abortNonterminal(self, "Literal");
  return 0;
}

#endif
