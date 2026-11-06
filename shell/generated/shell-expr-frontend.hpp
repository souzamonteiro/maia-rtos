#ifndef MAIA_SHELL_EXPR_FRONTEND_HPP
#define MAIA_SHELL_EXPR_FRONTEND_HPP
#include "shell-expr-prefix.hpp"
#include "shell-expr-parser.hpp"
static const unsigned int kShellExprTokenEOF = MAIACC_TOKEN_EOF;
static const unsigned int kShellExprTokenOR = MAIACC_TOKEN_OR;
static const unsigned int kShellExprTokenAND = MAIACC_TOKEN_AND;
static const unsigned int kShellExprTokenEQ = MAIACC_TOKEN_EQ;
static const unsigned int kShellExprTokenNE = MAIACC_TOKEN_NE;
static const unsigned int kShellExprTokenLT = MAIACC_TOKEN_LT;
static const unsigned int kShellExprTokenLE = MAIACC_TOKEN_LE;
static const unsigned int kShellExprTokenGT = MAIACC_TOKEN_GT;
static const unsigned int kShellExprTokenGE = MAIACC_TOKEN_GE;
static const unsigned int kShellExprTokenPLUS = MAIACC_TOKEN_PLUS;
static const unsigned int kShellExprTokenMINUS = MAIACC_TOKEN_MINUS;
static const unsigned int kShellExprTokenSTAR = MAIACC_TOKEN_STAR;
static const unsigned int kShellExprTokenSLASH = MAIACC_TOKEN_SLASH;
static const unsigned int kShellExprTokenPERCENT = MAIACC_TOKEN_PERCENT;
static const unsigned int kShellExprTokenCOLON = MAIACC_TOKEN_COLON;
static const int kShellExprParseNoMemory = MAIACC_PARSE_RESOURCE_NOMEM;
static const int kShellExprParseLimit = MAIACC_PARSE_RESOURCE_LIMIT;
#include "shell-expr-unprefix.hpp"
#endif
