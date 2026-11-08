#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>

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
static std::shared_ptr<Expr> convertUnary(const std::shared_ptr<CSTNode>& node);
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

    bool hasNew = false;
    bool hasDelete = false;
    bool hasSizeof = false;

    for (const auto& ch : node->children) {
        if (ch->isToken()) {
            if (ch->tokenVal == "new") hasNew = true;
            else if (ch->tokenVal == "delete") hasDelete = true;
            else if (ch->tokenVal == "sizeof") hasSizeof = true;
        }
    }

    if (hasNew) {
        auto qnameNode = node->findFirst("QualifiedName");
        std::string className = qnameNode ? collectAllTokens(qnameNode) : "";
        std::vector<std::shared_ptr<Expr>> args;
        auto argList = node->findFirst("ArgumentList");
        if (argList) {
            for (const auto& a : argList->findAll("Expression")) {
                args.push_back(convertExpr(a));
            }
        }
        return std::make_shared<NewExpr>(className, args);
    }

    if (hasDelete) {
        auto uNode = node->findFirst("Unary");
        if (uNode) {
            return std::make_shared<DeleteExpr>(convertUnary(uNode));
        }
    }

    if (hasSizeof) {
        auto tNode = node->findFirst("Type");
        Type t = tNode ? convertType(tNode) : Type(TypeKind::I32);
        return std::make_shared<SizeofExpr>(t);
    }

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
            if (auto mem = ast_cast<MemberExpr>(expr)) {
                auto mcall = std::make_shared<MethodCallExpr>(mem->target, mem->member, args);
                mcall->isArrow = mem->isArrow;
                expr = mcall;
            } else {
                std::string calleeName;
                if (auto v = ast_cast<VarExpr>(expr)) {
                    calleeName = v->name;
                }
                expr = std::make_shared<CallExpr>(calleeName, args);
            }
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
                break;
            }
        }
        std::shared_ptr<Expr> dInit = nullptr;
        auto exprNode = fNode->findFirst("Expression");
        if (exprNode) {
            dInit = convertExpr(exprNode);
        }
        fields.emplace_back(ft, fName, dInit);
    }
    return std::make_shared<StructDecl>(name, fields);
}

static std::shared_ptr<ClassDecl> convertClassDecl(const std::shared_ptr<CSTNode>& node) {
    if (!node) return nullptr;
    std::string name;
    for (const auto& ch : node->children) {
        if (ch->isToken() && ch->tokenVal != "class" && ch->tokenVal != "{" && ch->tokenVal != "}" && ch->tokenVal != ";") {
            name = ch->tokenVal;
            break;
        }
    }

    std::vector<FieldDecl> fields;
    std::vector<Visibility> fieldVisibilities;
    std::vector<std::shared_ptr<MethodDecl>> methods;

    for (const auto& mNode : node->findAll("ClassMember")) {
        Visibility vis = Visibility::Public;
        auto visNode = mNode->findFirst("Visibility");
        if (visNode) {
            for (const auto& ch : visNode->children) {
                if (ch->isToken()) {
                    if (ch->tokenVal == "private") vis = Visibility::Private;
                    else if (ch->tokenVal == "protected") vis = Visibility::Protected;
                    else if (ch->tokenVal == "public") vis = Visibility::Public;
                    break;
                }
            }
        }

        auto fNode = mNode->findFirst("FieldDecl");
        if (fNode) {
            auto typeNode = fNode->findFirst("Type");
            Type ft = typeNode ? convertType(typeNode) : Type(TypeKind::I32);
            std::string fName;
            for (const auto& tok : fNode->children) {
                if (tok->isToken() && tok->tokenVal != ";" && tok->tokenVal != "=" && tok->tokenVal != ft.name) {
                    fName = tok->tokenVal;
                    break;
                }
            }
            std::shared_ptr<Expr> dInit = nullptr;
            auto exprNode = fNode->findFirst("Expression");
            if (exprNode) {
                dInit = convertExpr(exprNode);
            }
            fields.emplace_back(ft, fName, dInit);
            fieldVisibilities.push_back(vis);
        }

        auto methodNode = mNode->findFirst("MethodDecl");
        if (methodNode) {
            auto typeNode = methodNode->findFirst("Type");
            Type retType = typeNode ? convertType(typeNode) : Type(TypeKind::Void);
            std::string mName;
            bool isConst = false;
            for (const auto& ch : methodNode->children) {
                if (ch->isToken()) {
                    if (ch->tokenVal == "const") isConst = true;
                    else if (ch->tokenVal != "(" && ch->tokenVal != ")" && mName.empty() && ch->tokenVal != retType.name) {
                        mName = ch->tokenVal;
                    }
                }
            }
            std::vector<ParamDecl> params;
            auto pList = methodNode->findFirst("ParameterList");
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
            auto body = convertBlock(methodNode->findFirst("Block"));
            methods.push_back(std::make_shared<MethodDecl>(vis, retType, mName, params, body, isConst, false));
        }

        auto ctorNode = mNode->findFirst("ConstructorDecl");
        if (ctorNode) {
            std::string ctorName;
            for (const auto& ch : ctorNode->children) {
                if (ch->isToken() && ch->tokenVal != "(" && ch->tokenVal != ")") {
                    ctorName = ch->tokenVal;
                    break;
                }
            }
            std::vector<ParamDecl> params;
            auto pList = ctorNode->findFirst("ParameterList");
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
            auto body = convertBlock(ctorNode->findFirst("Block"));
            methods.push_back(std::make_shared<MethodDecl>(vis, Type(TypeKind::Void), ctorName, params, body, false, true));
        }
    }

    return std::make_shared<ClassDecl>(name, fields, fieldVisibilities, methods);
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

static void processTopLevel(const std::shared_ptr<CSTNode>& top,
                            const std::shared_ptr<Program>& program,
                            const std::string& currentNamespace) {
    auto ns = top->findFirst("NamespaceDecl");
    if (ns) {
        auto qnameNode = ns->findFirst("QualifiedName");
        std::string nsName = qnameNode ? collectAllTokens(qnameNode) : "";
        std::string fullNs = currentNamespace.empty() ? nsName : (currentNamespace + "_" + nsName);
        program->namespaces.insert(fullNs);

        for (const auto& subTop : ns->findAll("TopLevel")) {
            processTopLevel(subTop, program, fullNs);
        }
        return;
    }

    auto st = top->findFirst("StructDecl");
    if (st) {
        auto sdecl = convertStructDecl(st);
        if (!currentNamespace.empty() && sdecl) {
            sdecl->name = currentNamespace + "_" + sdecl->name;
        }
        program->decls.push_back(sdecl);
    }
    auto cl = top->findFirst("ClassDecl");
    if (cl) {
        auto cdecl = convertClassDecl(cl);
        if (!currentNamespace.empty() && cdecl) {
            cdecl->name = currentNamespace + "_" + cdecl->name;
        }
        program->decls.push_back(cdecl);
    }
    auto fn = top->findFirst("FunctionDecl");
    if (fn) {
        auto fdecl = convertFunctionDecl(fn);
        if (!currentNamespace.empty() && fdecl) {
            fdecl->name = currentNamespace + "_" + fdecl->name;
        }
        program->decls.push_back(fdecl);
    }
    auto ext = top->findFirst("ExternDecl");
    if (ext) {
        program->decls.push_back(convertExternDecl(ext));
    }
}

namespace fs = std::filesystem;

static std::string trimString(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string resolveIncludePath(const std::string& requested,
                                      const std::string& currentPath,
                                      const std::vector<std::string>& searchPaths) {
    std::vector<std::string> nameCandidates = { requested };
    if (requested.size() < 3 || requested.substr(requested.size() - 3) != ".ms") {
        nameCandidates.push_back(requested + ".ms");
    }

    // Se o target for absoluto e existir
    for (const auto& cand : nameCandidates) {
        std::error_code ec;
        fs::path p(cand);
        if (p.is_absolute() && fs::exists(p, ec)) {
            return fs::canonical(p, ec).string();
        }
    }

    // Diretório base
    fs::path baseDir = ".";
    if (!currentPath.empty()) {
        std::error_code ec;
        fs::path cur(currentPath);
        if (fs::is_directory(cur, ec)) {
            baseDir = cur;
        } else if (cur.has_parent_path()) {
            baseDir = cur.parent_path();
        }
    }

    for (const auto& cand : nameCandidates) {
        std::error_code ec;
        fs::path p = baseDir / cand;
        if (fs::exists(p, ec)) {
            return fs::canonical(p, ec).string();
        }
    }

    // Search paths do usuário
    for (const auto& sp : searchPaths) {
        for (const auto& cand : nameCandidates) {
            std::error_code ec;
            fs::path p = fs::path(sp) / cand;
            if (fs::exists(p, ec)) {
                return fs::canonical(p, ec).string();
            }
        }
    }

    // Caminhos padrão do sistema
    static const std::vector<std::string> defaultDirs = {
        "lib",
        "maiascript/lib",
        "../maiascript/lib",
        "../../maiascript/lib",
        "/Volumes/External_SSD/Documentos/Projects/maia-rtos/maiascript/lib",
        "/sd/lib",
        "/lib"
    };

    for (const auto& dd : defaultDirs) {
        for (const auto& cand : nameCandidates) {
            std::error_code ec;
            fs::path p = fs::path(dd) / cand;
            if (fs::exists(p, ec)) {
                return fs::canonical(p, ec).string();
            }
        }
    }

    return "";
}

std::string Parser::expandIncludes(const std::string& source, const std::string& currentPath,
                                  const std::vector<std::string>& searchPaths,
                                  std::unordered_set<std::string>& includedFiles,
                                  std::string& outError) {
    std::stringstream in(source);
    std::string line;
    std::string output;
    bool inBlockComment = false;

    while (std::getline(in, line)) {
        std::string trimmed = trimString(line);

        if (inBlockComment) {
            output += line + "\n";
            if (trimmed.find("*/") != std::string::npos) {
                inBlockComment = false;
            }
            continue;
        }

        if (trimmed.rfind("/*", 0) == 0) {
            output += line + "\n";
            if (trimmed.find("*/") == std::string::npos) {
                inBlockComment = true;
            }
            continue;
        }

        if (trimmed.rfind("//", 0) == 0) {
            output += line + "\n";
            continue;
        }

        std::string target;
        bool isInclude = false;

        // 1. #include "path" ou #include <path>
        if (trimmed.rfind("#include", 0) == 0) {
            size_t q1 = trimmed.find_first_of("\"<");
            size_t q2 = (q1 != std::string::npos) ? trimmed.find_first_of("\">", q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                target = trimmed.substr(q1 + 1, q2 - q1 - 1);
                isInclude = true;
            }
        }
        // 2. include("path"); ou include "path";
        else if (trimmed.rfind("include", 0) == 0 && trimmed.size() > 7 &&
                 (trimmed[7] == ' ' || trimmed[7] == '(' || trimmed[7] == '\t' || trimmed[7] == '"')) {
            size_t q1 = trimmed.find('"');
            size_t q2 = (q1 != std::string::npos) ? trimmed.find('"', q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                target = trimmed.substr(q1 + 1, q2 - q1 - 1);
                isInclude = true;
            }
        }
        // 3. import ...;
        else if (trimmed.rfind("import", 0) == 0 && trimmed.size() > 6 &&
                 (trimmed[6] == ' ' || trimmed[6] == '\t' || trimmed[6] == '"')) {
            size_t q1 = trimmed.find('"');
            if (q1 != std::string::npos) {
                size_t q2 = trimmed.find('"', q1 + 1);
                if (q2 != std::string::npos && q2 > q1) {
                    target = trimmed.substr(q1 + 1, q2 - q1 - 1);
                    isInclude = true;
                }
            } else {
                std::string rest = trimString(trimmed.substr(6));
                size_t semi = rest.find(';');
                if (semi != std::string::npos) {
                    rest = rest.substr(0, semi);
                }
                size_t asPos = rest.find(" as ");
                if (asPos != std::string::npos) {
                    rest = rest.substr(0, asPos);
                }
                rest = trimString(rest);
                if (!rest.empty()) {
                    std::string slashRest = rest;
                    for (char& c : slashRest) {
                        if (c == '.') c = '/';
                    }
                    target = slashRest;
                    isInclude = true;
                }
            }
        }

        if (isInclude) {
            std::string resolved = resolveIncludePath(target, currentPath, searchPaths);
            if (resolved.empty()) {
                outError = "Include error: could not locate '" + target + "' from '" + currentPath + "'";
                return "";
            }

            if (includedFiles.find(resolved) != includedFiles.end()) {
                output += "// [already included: " + target + "]\n";
            } else {
                includedFiles.insert(resolved);
                std::ifstream f(resolved.c_str());
                if (!f.is_open()) {
                    outError = "Include error: cannot open '" + resolved + "'";
                    return "";
                }
                std::stringstream buf;
                buf << f.rdbuf();
                std::string subSource = buf.str();
                std::string expanded = expandIncludes(subSource, resolved, searchPaths, includedFiles, outError);
                if (!outError.empty()) {
                    return "";
                }
                output += "// [begin: " + target + "]\n" + expanded + "\n// [end: " + target + "]\n";
            }
        } else {
            output += line + "\n";
        }
    }

    return output;
}

std::shared_ptr<Program> Parser::parseSource(const std::string& source, std::string& outError,
                                            const std::string& currentPath,
                                            const std::vector<std::string>& searchPaths) {
    std::string finalSource = source;
    if (source.find("#include") != std::string::npos ||
        source.find("include") != std::string::npos ||
        source.find("import") != std::string::npos) {
        std::unordered_set<std::string> includedFiles;
        if (!currentPath.empty()) {
            includedFiles.insert(currentPath);
        }
        std::string exp = expandIncludes(source, currentPath.empty() ? "." : currentPath, searchPaths, includedFiles, outError);
        if (!outError.empty()) return nullptr;
        finalSource = exp;
    }

    MaiaCCLexerState lexer;
    maiaCCLexerStateInit(&lexer, finalSource.c_str(), finalSource.length());
    size_t max_toks = (finalSource.length() < 256) ? 512 : (finalSource.length() * 2);
    if (max_toks < 1024) max_toks = 1024;
    if (max_toks > 32768) max_toks = 32768;
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
        processTopLevel(top, program, "");
    }

    return program;
}

std::shared_ptr<Program> Parser::parseFile(const std::string& path, std::string& outError,
                                          const std::vector<std::string>& searchPaths) {
    std::error_code ec;
    std::string canonicalPath = path;
    if (fs::exists(path, ec)) {
        canonicalPath = fs::canonical(path, ec).string();
    }

    std::ifstream f(canonicalPath.c_str());
    if (!f.is_open()) {
        outError = "Could not open file: " + path;
        return nullptr;
    }
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string rawSource = buffer.str();

    std::unordered_set<std::string> includedFiles;
    includedFiles.insert(canonicalPath);

    std::string expanded = expandIncludes(rawSource, canonicalPath, searchPaths, includedFiles, outError);
    if (!outError.empty()) {
        return nullptr;
    }

    return parseSource(expanded, outError, canonicalPath, searchPaths);
}

} // namespace maiascript
