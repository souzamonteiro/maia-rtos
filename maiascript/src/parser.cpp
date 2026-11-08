#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "../../lib/maiaregex/maia_regex.h"
#include "maiascript_parser.inc"

namespace maiascript {

enum EventKind { EV_START, EV_END, EV_TERMINAL };

struct ParseEvent {
    EventKind kind;
    std::string name;
    unsigned int tokenType;
    unsigned int pos;
};

struct EventCollector {
    std::vector<ParseEvent> log;
};

static unsigned int cbCheckpoint(void* ctx) {
    return ((EventCollector*)ctx)->log.size();
}

static void cbRestore(void* ctx, unsigned int mark) {
    ((EventCollector*)ctx)->log.resize(mark);
}

static void cbStartNonterminal(void* ctx, const char* name, unsigned int pos) {
    ParseEvent ev;
    ev.kind = EV_START;
    ev.name = name;
    ev.tokenType = 0;
    ev.pos = pos;
    ((EventCollector*)ctx)->log.push_back(ev);
}

static void cbEndNonterminal(void* ctx, const char* name, unsigned int pos) {
    ParseEvent ev;
    ev.kind = EV_END;
    ev.name = name;
    ev.tokenType = 0;
    ev.pos = pos;
    ((EventCollector*)ctx)->log.push_back(ev);
}

static void cbAbortNonterminal(void* ctx, const char* name, unsigned int pos) {}

static void cbTerminal(void* ctx, unsigned int type, const char* val, unsigned int len, unsigned int pos) {
    ParseEvent ev;
    ev.kind = EV_TERMINAL;
    ev.name = std::string(val, len);
    ev.tokenType = type;
    ev.pos = pos;
    ((EventCollector*)ctx)->log.push_back(ev);
}

struct CSTNode {
    std::string name;
    std::string tokenVal;
    unsigned int tokenType;
    std::vector<std::shared_ptr<CSTNode>> children;

    CSTNode(const std::string& n = "") : name(n), tokenType(0) {}

    bool isToken() const { return name == "@token"; }

    std::shared_ptr<CSTNode> findFirst(const std::string& childName) const {
        for (const auto& ch : children) {
            if (ch->name == childName) return ch;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<CSTNode>> findAll(const std::string& childName) const {
        std::vector<std::shared_ptr<CSTNode>> res;
        for (const auto& ch : children) {
            if (ch->name == childName) res.push_back(ch);
        }
        return res;
    }
};

static std::shared_ptr<CSTNode> buildCST(const std::vector<ParseEvent>& log) {
    std::vector<std::shared_ptr<CSTNode>> stack;
    auto root = std::make_shared<CSTNode>("ROOT");
    stack.push_back(root);

    for (const auto& ev : log) {
        if (ev.kind == EV_START) {
            auto n = std::make_shared<CSTNode>(ev.name);
            stack.back()->children.push_back(n);
            stack.push_back(n);
        } else if (ev.kind == EV_TERMINAL) {
            auto n = std::make_shared<CSTNode>("@token");
            n->tokenVal = ev.name;
            n->tokenType = ev.tokenType;
            stack.back()->children.push_back(n);
        } else if (ev.kind == EV_END) {
            if (stack.size() > 1) {
                stack.pop_back();
            }
        }
    }
    return root;
}

// Forward declarations of AST builders
static Type convertType(const std::shared_ptr<CSTNode>& node);
static std::shared_ptr<Expr> convertExpr(const std::shared_ptr<CSTNode>& node);
static std::shared_ptr<Stmt> convertStmt(const std::shared_ptr<CSTNode>& node);
static std::shared_ptr<BlockStmt> convertBlock(const std::shared_ptr<CSTNode>& node);

static std::string collectAllTokens(const std::shared_ptr<CSTNode>& node) {
    if (node->isToken()) return node->tokenVal;
    std::string s;
    for (const auto& ch : node->children) {
        s += collectAllTokens(ch);
    }
    return s;
}

static Type convertType(const std::shared_ptr<CSTNode>& node) {
    if (!node) return Type(TypeKind::I32);
    std::string raw = collectAllTokens(node);
    return Type::fromString(raw);
}

static std::shared_ptr<Expr> convertPrimary(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    // Primary ::= Literal | NAME | '(' Expression ')' | ...
    for (const auto& ch : node->children) {
        if (ch->name == "Literal") {
            for (const auto& lch : ch->children) {
                if (lch->isToken()) {
                    const std::string& text = lch->tokenVal;
                    if (text == "true") return std::make_shared<LiteralExpr>("true", LiteralKind::Bool);
                    if (text == "false") return std::make_shared<LiteralExpr>("false", LiteralKind::Bool);
                    if (text == "null") return std::make_shared<LiteralExpr>("null", LiteralKind::Null);
                    if (!text.empty() && text.front() == '"') return std::make_shared<LiteralExpr>(text, LiteralKind::String);
                    if (!text.empty() && text.front() == '\'' && text.back() == '\'') {
                        char c = (text.size() > 1) ? text[1] : 0;
                        if (text.size() > 2 && text[1] == '\\') {
                            if (text[2] == 'n') c = '\n';
                            else if (text[2] == 't') c = '\t';
                            else if (text[2] == 'r') c = '\r';
                            else if (text[2] == '0') c = '\0';
                            else c = text[2];
                        }
                        return std::make_shared<LiteralExpr>((int64_t)(uint8_t)c);
                    }
                    if (text.find('.') != std::string::npos) {
                        return std::make_shared<LiteralExpr>(std::strtod(text.c_str(), nullptr));
                    }
                    int64_t val = 0;
                    if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
                        val = std::strtoll(text.c_str(), nullptr, 16);
                    } else {
                        val = std::strtoll(text.c_str(), nullptr, 10);
                    }
                    return std::make_shared<LiteralExpr>(val);
                }
            }
        }
        if (ch->isToken() && ch->tokenVal != "(" && ch->tokenVal != ")") {
            return std::make_shared<VarExpr>(ch->tokenVal);
        }
        if (ch->name == "Expression") {
            return convertExpr(ch);
        }
    }
    return nullptr;
}

static std::shared_ptr<Expr> convertPostfix(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto prim = node->findFirst("Primary");
    auto expr = convertPrimary(prim);

    auto tails = node->findAll("PostfixTail");
    for (const auto& tail : tails) {
        // PostfixTail can be: '(' ArgumentList? ')' | '.' NAME | '->' NAME | '++' | '--' | '[' ... ']'
        bool hasCall = false;
        bool hasArrow = false;
        bool hasDot = false;
        bool hasInc = false;
        bool hasDec = false;
        bool hasIndex = false;
        std::string memberName;

        for (size_t i = 0; i < tail->children.size(); ++i) {
            const auto& ch = tail->children[i];
            if (ch->isToken()) {
                if (ch->tokenVal == "(") hasCall = true;
                else if (ch->tokenVal == "[") hasIndex = true;
                else if (ch->tokenVal == "->") {
                    hasArrow = true;
                    if (i + 1 < tail->children.size() && tail->children[i + 1]->isToken()) {
                        memberName = tail->children[i + 1]->tokenVal;
                    }
                } else if (ch->tokenVal == ".") {
                    hasDot = true;
                    if (i + 1 < tail->children.size() && tail->children[i + 1]->isToken()) {
                        memberName = tail->children[i + 1]->tokenVal;
                    }
                } else if (ch->tokenVal == "++") hasInc = true;
                else if (ch->tokenVal == "--") hasDec = true;
            }
        }

        if (hasCall) {
            std::vector<std::shared_ptr<Expr>> args;
            auto argList = tail->findFirst("ArgumentList");
            if (argList) {
                for (const auto& a : argList->findAll("Expression")) {
                    args.push_back(convertExpr(a));
                }
            }
            std::string calleeName;
            if (auto v = ast_cast<VarExpr>(expr)) {
                calleeName = v->name;
            }
            expr = std::make_shared<CallExpr>(calleeName, args);
        } else if (hasIndex) {
            auto idxNode = tail->findFirst("Expression");
            if (idxNode) {
                expr = std::make_shared<IndexExpr>(expr, convertExpr(idxNode));
            }
        } else if (hasArrow || hasDot) {
            expr = std::make_shared<MemberExpr>(expr, memberName, hasArrow);
        } else if (hasInc) {
            expr = std::make_shared<UnaryExpr>("++", expr, false);
        } else if (hasDec) {
            expr = std::make_shared<UnaryExpr>("--", expr, false);
        }
    }
    return expr;
}

static std::shared_ptr<Expr> convertUnary(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    // Unary ::= ('!' | '~' | '+' | '-' | '*' | '&' | '++' | '--') Unary | Postfix
    auto post = node->findFirst("Postfix");
    if (post) return convertPostfix(post);

    std::string op;
    for (const auto& ch : node->children) {
        if (ch->isToken()) {
            op = ch->tokenVal;
            break;
        }
    }
    auto sub = node->findFirst("Unary");
    if (sub) {
        return std::make_shared<UnaryExpr>(op, convertUnary(sub), true);
    }
    return nullptr;
}

static std::shared_ptr<Expr> convertBinaryLevel(const std::shared_ptr<CSTNode>& node,
                                                const std::string& childRule) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subNodes;
    std::vector<std::string> ops;

    for (const auto& ch : node->children) {
        if (ch->name == childRule) {
            subNodes.push_back(ch);
        } else if (ch->isToken()) {
            ops.push_back(ch->tokenVal);
        }
    }

    if (subNodes.empty()) return nullptr;

    auto expr = (childRule == "Unary") ? convertUnary(subNodes[0]) : convertBinaryLevel(subNodes[0], "");
    for (size_t i = 0; i < ops.size() && i + 1 < subNodes.size(); ++i) {
        auto right = (childRule == "Unary") ? convertUnary(subNodes[i + 1]) : convertBinaryLevel(subNodes[i + 1], "");
        expr = std::make_shared<BinaryExpr>(ops[i], expr, right);
    }
    return expr;
}

static std::shared_ptr<Expr> convertMultiplicative(const std::shared_ptr<CSTNode>& node) {
    return convertBinaryLevel(node, "Unary");
}

static std::shared_ptr<Expr> convertAdditive(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("Multiplicative");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && (ch->tokenVal == "+" || ch->tokenVal == "-")) {
            ops.push_back(ch->tokenVal);
        }
    }
    if (subs.empty()) return nullptr;
    auto expr = convertMultiplicative(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        auto right = convertMultiplicative(subs[i + 1]);
        expr = std::make_shared<BinaryExpr>(ops[i], expr, right);
    }
    return expr;
}

static std::shared_ptr<Expr> convertShift(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("Additive");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && (ch->tokenVal == "<<" || ch->tokenVal == ">>")) ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertAdditive(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertAdditive(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertRelational(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("Shift");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && (ch->tokenVal == "<" || ch->tokenVal == ">" || ch->tokenVal == "<=" || ch->tokenVal == ">=")) {
            ops.push_back(ch->tokenVal);
        }
    }
    if (subs.empty()) return nullptr;
    auto expr = convertShift(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertShift(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertEquality(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("Relational");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && (ch->tokenVal == "==" || ch->tokenVal == "!=")) ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertRelational(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertRelational(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertBitwiseAnd(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("Equality");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal == "&") ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertEquality(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertEquality(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertBitwiseXor(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("BitwiseAnd");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal == "^") ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertBitwiseAnd(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertBitwiseAnd(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertBitwiseOr(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("BitwiseXor");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal == "|") ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertBitwiseXor(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertBitwiseXor(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertLogicalAnd(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("BitwiseOr");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal == "&&") ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertBitwiseOr(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertBitwiseOr(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertLogicalOr(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::vector<std::shared_ptr<CSTNode>> subs = node->findAll("LogicalAnd");
    std::vector<std::string> ops;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal == "||") ops.push_back(ch->tokenVal);
    }
    if (subs.empty()) return nullptr;
    auto expr = convertLogicalAnd(subs[0]);
    for (size_t i = 0; i < ops.size() && i + 1 < subs.size(); ++i) {
        expr = std::make_shared<BinaryExpr>(ops[i], expr, convertLogicalAnd(subs[i + 1]));
    }
    return expr;
}

static std::shared_ptr<Expr> convertConditional(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto logOr = node->findFirst("LogicalOr");
    return convertLogicalOr(logOr);
}

static std::shared_ptr<Expr> convertAssignment(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto cond = node->findFirst("Conditional");
    auto target = convertConditional(cond);

    auto assignOp = node->findFirst("AssignOp");
    auto nextAssign = node->findFirst("Assignment");
    if (assignOp && nextAssign) {
        std::string op = collectAllTokens(assignOp);
        auto val = convertAssignment(nextAssign);
        return std::make_shared<AssignExpr>(op, target, val);
    }
    return target;
}

static std::shared_ptr<Expr> convertExpr(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto assign = node->findFirst("Assignment");
    if (assign) return convertAssignment(assign);
    return nullptr;
}

static std::shared_ptr<Stmt> convertStmt(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    // Statement ::= Block | LocalDecl | IfStmt | WhileStmt | ForStmt | ReturnStmt | ExpressionStmt | ...
    for (const auto& ch : node->children) {
        if (ch->name == "Block") {
            return convertBlock(ch);
        }
        if (ch->name == "LocalDecl") {
            auto typeNode = ch->findFirst("Type");
            Type t = typeNode ? convertType(typeNode) : Type(TypeKind::I32);
            std::string varName;
            for (const auto& tok : ch->children) {
                if (tok->isToken() && tok->tokenVal != "const" && tok->tokenVal != "auto" && tok->tokenVal != "=" && tok->tokenVal != ";") {
                    varName = tok->tokenVal;
                    break;
                }
            }
            auto exprNode = ch->findFirst("Expression");
            std::shared_ptr<Expr> init = exprNode ? convertExpr(exprNode) : nullptr;
            return std::make_shared<VarDeclStmt>(t, varName, init);
        }
        if (ch->name == "IfStmt") {
            auto condNode = ch->findFirst("Expression");
            auto cond = convertExpr(condNode);
            auto blocks = ch->findAll("Block");
            auto thenBranch = blocks.size() > 0 ? convertBlock(blocks[0]) : nullptr;
            std::shared_ptr<Stmt> elseBranch = nullptr;
            if (blocks.size() > 1) {
                elseBranch = convertBlock(blocks[1]);
            } else {
                auto elseIf = ch->findFirst("IfStmt");
                if (elseIf) elseBranch = convertStmt(elseIf);
            }
            return std::make_shared<IfStmt>(cond, thenBranch, elseBranch);
        }
        if (ch->name == "WhileStmt") {
            auto condNode = ch->findFirst("Expression");
            auto cond = convertExpr(condNode);
            auto blockNode = ch->findFirst("Block");
            return std::make_shared<WhileStmt>(cond, convertBlock(blockNode));
        }
        if (ch->name == "ForStmt") {
            std::shared_ptr<Stmt> init = nullptr;
            auto forInit = ch->findFirst("ForInit");
            if (forInit) {
                auto localInit = forInit->findFirst("LocalInit");
                if (localInit) {
                    auto tNode = localInit->findFirst("Type");
                    Type t = tNode ? convertType(tNode) : Type(TypeKind::I32);
                    std::string vName;
                    for (const auto& tok : localInit->children) {
                        if (tok->isToken() && tok->tokenVal != "const" && tok->tokenVal != "auto" && tok->tokenVal != "=") {
                            vName = tok->tokenVal;
                            break;
                        }
                    }
                    auto eNode = localInit->findFirst("Expression");
                    init = std::make_shared<VarDeclStmt>(t, vName, eNode ? convertExpr(eNode) : nullptr);
                } else {
                    auto eNode = forInit->findFirst("Expression");
                    if (eNode) init = std::make_shared<ExprStmt>(convertExpr(eNode));
                }
            }
            auto exprs = ch->findAll("Expression");
            std::shared_ptr<Expr> cond = nullptr;
            std::shared_ptr<Expr> step = nullptr;
            if (exprs.size() == 2) {
                cond = convertExpr(exprs[0]);
                step = convertExpr(exprs[1]);
            } else if (exprs.size() == 1) {
                cond = convertExpr(exprs[0]);
            }
            auto blockNode = ch->findFirst("Block");
            return std::make_shared<ForStmt>(init, cond, step, convertBlock(blockNode));
        }
        if (ch->name == "ReturnStmt") {
            auto exprNode = ch->findFirst("Expression");
            return std::make_shared<ReturnStmt>(exprNode ? convertExpr(exprNode) : nullptr);
        }
        if (ch->name == "ExpressionStmt") {
            auto exprNode = ch->findFirst("Expression");
            return std::make_shared<ExprStmt>(convertExpr(exprNode));
        }
    }
    return nullptr;
}

static std::shared_ptr<BlockStmt> convertBlock(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto block = std::make_shared<BlockStmt>();
    for (const auto& ch : node->children) {
        if (ch->name == "Statement") {
            auto s = convertStmt(ch);
            if (s) block->statements.push_back(s);
        }
    }
    return block;
}

static std::shared_ptr<StructDecl> convertStructDecl(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::string name;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal != "struct" && ch->tokenVal != "{" && ch->tokenVal != "}" && ch->tokenVal != ";") {
            name = ch->tokenVal;
            break;
        }
    }

    std::vector<FieldDecl> fields;
    for (const auto& fNode : node->findAll("FieldDecl")) {
        auto typeNode = fNode->findFirst("Type");
        Type ft = typeNode ? convertType(typeNode) : Type(TypeKind::I32);
        std::string fName;
        for (const auto& tok : fNode->children) {
            if (tok->isToken() && tok->tokenVal != ";" && tok->tokenVal != "=" && tok->tokenVal != ft.name) {
                fName = tok->tokenVal;
            }
        }
        fields.emplace_back(ft, fName);
    }
    return std::make_shared<StructDecl>(name, fields);
}

static std::shared_ptr<FunctionDecl> convertFunctionDecl(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto typeNode = node->findFirst("Type");
    Type retType = typeNode ? convertType(typeNode) : Type(TypeKind::Void);

    std::string name;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal != "(" && ch->tokenVal != ")") {
            name = ch->tokenVal;
            break;
        }
    }

    std::vector<ParamDecl> params;
    auto pList = node->findFirst("ParameterList");
    if (pList) {
        for (const auto& pNode : pList->findAll("Parameter")) {
            auto pTypeNode = pNode->findFirst("Type");
            Type pt = pTypeNode ? convertType(pTypeNode) : Type(TypeKind::I32);
            std::string pName;
            for (const auto& tok : pNode->children) {
                if (tok->isToken()) {
                    pName = tok->tokenVal;
                    break;
                }
            }
            params.emplace_back(pt, pName);
        }
    }

    auto blockNode = node->findFirst("Block");
    auto body = convertBlock(blockNode);
    return std::make_shared<FunctionDecl>(retType, name, params, body);
}

static std::shared_ptr<FunctionDecl> convertExternDecl(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    auto typeNode = node->findFirst("Type");
    Type retType = typeNode ? convertType(typeNode) : Type(TypeKind::Void);

    std::string name;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal != "extern" && ch->tokenVal != "(" && ch->tokenVal != ")" && ch->tokenVal != ";") {
            name = ch->tokenVal;
            break;
        }
    }

    std::vector<ParamDecl> params;
    auto pList = node->findFirst("ParameterList");
    if (pList) {
        for (const auto& pNode : pList->findAll("Parameter")) {
            auto pTypeNode = pNode->findFirst("Type");
            Type pt = pTypeNode ? convertType(pTypeNode) : Type(TypeKind::I32);
            std::string pName;
            for (const auto& tok : pNode->children) {
                if (tok->isToken()) {
                    pName = tok->tokenVal;
                    break;
                }
            }
            params.emplace_back(pt, pName);
        }
    }

    return std::make_shared<FunctionDecl>(retType, name, params, nullptr, /*isExtern=*/true);
}

std::shared_ptr<Program> Parser::parseSource(const std::string& source, std::string& outError) {
    MaiaCCLexerState lexer;
    maiaCCLexerStateInit(&lexer, source.c_str(), source.length());
    size_t max_toks = (source.length() < 256) ? 512 : (source.length() * 2);
    if (max_toks < 1024) max_toks = 1024;
    if (max_toks > 16384) max_toks = 16384;
    std::vector<MaiaCCParserToken> tokens(max_toks);
    if (!maiaCCLexerTokenize(&lexer, tokens.data(), tokens.size())) {
        outError = "Lexical error";
        return nullptr;
    }

    EventCollector ec;
    MaiaCCParseEvents events;
    memset(&events, 0, sizeof(events));
    events.context = &ec;
    events.checkpoint = cbCheckpoint;
    events.restore = cbRestore;
    events.startNonterminal = cbStartNonterminal;
    events.endNonterminal = cbEndNonterminal;
    events.abortNonterminal = cbAbortNonterminal;
    events.terminal = cbTerminal;

    MaiaCCParserState parser;
    maiaCCParserStateInit(&parser, tokens.data(), lexer.tokenCount, &events);
    int res = maiaCC_parse(&parser);
    if (!res || parser.failed) {
        std::stringstream ss;
        ss << "Syntax error at token index " << parser.position << "/" << lexer.tokenCount;
        if (parser.position < lexer.tokenCount) {
            ss << " (token: '" << std::string(tokens[parser.position].value, tokens[parser.position].length) << "')";
        }
        outError = ss.str();
        return nullptr;
    }

    auto cst = buildCST(ec.log);
    auto programNode = cst->findFirst("Program");
    if (!programNode) {
        outError = "Failed to locate Program root in CST";
        return nullptr;
    }

    auto program = std::make_shared<Program>();
    for (const auto& top : programNode->findAll("TopLevel")) {
        auto st = top->findFirst("StructDecl");
        if (st) {
            program->decls.push_back(convertStructDecl(st));
        }
        auto fn = top->findFirst("FunctionDecl");
        if (fn) {
            program->decls.push_back(convertFunctionDecl(fn));
        }
        auto ext = top->findFirst("ExternDecl");
        if (ext) {
            program->decls.push_back(convertExternDecl(ext));
        }
    }

    return program;
}

std::shared_ptr<Program> Parser::parseFile(const std::string& path, std::string& outError) {
    std::ifstream f(path.c_str());
    if (!f.is_open()) {
        outError = "Could not open file: " + path;
        return nullptr;
    }
    std::stringstream buffer;
    buffer << f.rdbuf();
    return parseSource(buffer.str(), outError);
}

} // namespace maiascript
