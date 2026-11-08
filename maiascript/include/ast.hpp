#ifndef MAIASCRIPT_AST_HPP
#define MAIASCRIPT_AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace maiascript {

enum class TypeKind {
    Void,
    Bool,
    Char,
    I8,
    U8,
    I16,
    U16,
    I32,
    U32,
    I64,
    U64,
    F32,
    F64,
    String,
    Custom
};

struct Type {
    TypeKind kind;
    std::string name;
    int pointerLevel;
    int arraySize;

    Type(TypeKind k = TypeKind::I32, const std::string& n = "", int ptr = 0, int arr = 0)
        : kind(k), name(n), pointerLevel(ptr), arraySize(arr) {}

    bool isVoid() const { return kind == TypeKind::Void && pointerLevel == 0 && arraySize == 0; }
    bool isPointer() const { return pointerLevel > 0; }
    bool isArray() const { return arraySize > 0; }
    bool isFloat() const { return pointerLevel == 0 && arraySize == 0 && (kind == TypeKind::F32 || kind == TypeKind::F64); }
    bool isInteger() const {
        if (pointerLevel > 0 || arraySize > 0) return false;
        return kind == TypeKind::I8 || kind == TypeKind::U8 ||
               kind == TypeKind::I16 || kind == TypeKind::U16 ||
               kind == TypeKind::I32 || kind == TypeKind::U32 ||
               kind == TypeKind::I64 || kind == TypeKind::U64 ||
               kind == TypeKind::Bool || kind == TypeKind::Char;
    }

    uint32_t size() const {
        if (pointerLevel > 0) return 4; // 32-bit RV32
        uint32_t baseSz = 4;
        switch (kind) {
            case TypeKind::Void: baseSz = 0; break;
            case TypeKind::Bool:
            case TypeKind::Char:
            case TypeKind::I8:
            case TypeKind::U8: baseSz = 1; break;
            case TypeKind::I16:
            case TypeKind::U16: baseSz = 2; break;
            case TypeKind::I32:
            case TypeKind::U32:
            case TypeKind::F32: baseSz = 4; break;
            case TypeKind::I64:
            case TypeKind::U64:
            case TypeKind::F64: baseSz = 8; break;
            default: baseSz = 4; break;
        }
        if (arraySize > 0) return baseSz * arraySize;
        return baseSz;
    }

    static Type fromString(const std::string& str) {
        std::string s = str;
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
        int arr = 0;
        if (!s.empty() && s.back() == ']') {
            s.pop_back();
            std::string num;
            while (!s.empty() && s.back() >= '0' && s.back() <= '9') {
                num = s.back() + num;
                s.pop_back();
            }
            if (!s.empty() && s.back() == '[') s.pop_back();
            arr = num.empty() ? -1 : std::stoi(num);
        }
        int ptr = 0;
        while (!s.empty() && s.back() == '*') {
            ptr++;
            s.pop_back();
        }
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();

        if (s == "void") return Type(TypeKind::Void, s, ptr, arr);
        if (s == "bool") return Type(TypeKind::Bool, s, ptr, arr);
        if (s == "char") return Type(TypeKind::Char, s, ptr, arr);
        if (s == "i8") return Type(TypeKind::I8, s, ptr, arr);
        if (s == "u8") return Type(TypeKind::U8, s, ptr, arr);
        if (s == "i16") return Type(TypeKind::I16, s, ptr, arr);
        if (s == "u16") return Type(TypeKind::U16, s, ptr, arr);
        if (s == "i32") return Type(TypeKind::I32, s, ptr, arr);
        if (s == "u32") return Type(TypeKind::U32, s, ptr, arr);
        if (s == "i64") return Type(TypeKind::I64, s, ptr, arr);
        if (s == "u64") return Type(TypeKind::U64, s, ptr, arr);
        if (s == "f32") return Type(TypeKind::F32, s, ptr, arr);
        if (s == "f64") return Type(TypeKind::F64, s, ptr, arr);
        if (s == "string") return Type(TypeKind::String, s, ptr, arr);
        return Type(TypeKind::Custom, s, ptr, arr);
    }
};

enum class ASTKind {
    Unknown,
    Program,
    FunctionDecl,
    StructDecl,
    ExprStmt,
    VarDeclStmt,
    BlockStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    ReturnStmt,
    BreakStmt,
    ContinueStmt,
    LiteralExpr,
    VarExpr,
    BinaryExpr,
    UnaryExpr,
    AssignExpr,
    CallExpr,
    MemberExpr,
    IndexExpr
};

class ASTNode {
public:
    ASTKind kind;
    ASTNode(ASTKind k = ASTKind::Unknown) : kind(k) {}
    virtual ~ASTNode() = default;
};

template<typename T, typename U>
inline std::shared_ptr<T> ast_cast(const std::shared_ptr<U>& node) {
    if (node && node->kind == T::KIND) {
        return std::static_pointer_cast<T>(node);
    }
    return nullptr;
}

class Expr : public ASTNode {
public:
    Expr(ASTKind k) : ASTNode(k) {}
};

enum class LiteralKind {
    Int,
    Float,
    String,
    Bool,
    Null
};

class LiteralExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::LiteralExpr;
    LiteralKind litKind;
    int64_t intVal;
    double floatVal;
    std::string strVal;

    LiteralExpr(int64_t val) : Expr(KIND), litKind(LiteralKind::Int), intVal(val), floatVal(0) {}
    LiteralExpr(double val) : Expr(KIND), litKind(LiteralKind::Float), intVal(0), floatVal(val) {}
    LiteralExpr(const std::string& val, LiteralKind k = LiteralKind::String)
        : Expr(KIND), litKind(k), intVal(0), floatVal(0), strVal(val) {}
};

class VarExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::VarExpr;
    std::string name;
    VarExpr(const std::string& n) : Expr(KIND), name(n) {}
};

class BinaryExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::BinaryExpr;
    std::string op;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;

    BinaryExpr(const std::string& o, std::shared_ptr<Expr> l, std::shared_ptr<Expr> r)
        : Expr(KIND), op(o), left(l), right(r) {}
};

class UnaryExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::UnaryExpr;
    std::string op;
    std::shared_ptr<Expr> operand;
    bool isPrefix;

    UnaryExpr(const std::string& o, std::shared_ptr<Expr> expr, bool prefix = true)
        : Expr(KIND), op(o), operand(expr), isPrefix(prefix) {}
};

class AssignExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::AssignExpr;
    std::string op; // "=", "+=", "-=", etc.
    std::shared_ptr<Expr> target;
    std::shared_ptr<Expr> value;

    AssignExpr(const std::string& o, std::shared_ptr<Expr> tgt, std::shared_ptr<Expr> val)
        : Expr(KIND), op(o), target(tgt), value(val) {}
};

class CallExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::CallExpr;
    std::string callee;
    std::vector<std::shared_ptr<Expr>> args;

    CallExpr(const std::string& fn, const std::vector<std::shared_ptr<Expr>>& a)
        : Expr(KIND), callee(fn), args(a) {}
};

class MemberExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::MemberExpr;
    std::shared_ptr<Expr> target;
    std::string member;
    bool isArrow;

    MemberExpr(std::shared_ptr<Expr> tgt, const std::string& mem, bool arrow)
        : Expr(KIND), target(tgt), member(mem), isArrow(arrow) {}
};

class IndexExpr : public Expr {
public:
    static constexpr ASTKind KIND = ASTKind::IndexExpr;
    std::shared_ptr<Expr> target;
    std::shared_ptr<Expr> index;

    IndexExpr(std::shared_ptr<Expr> tgt, std::shared_ptr<Expr> idx)
        : Expr(KIND), target(tgt), index(idx) {}
};

class Stmt : public ASTNode {
public:
    Stmt(ASTKind k) : ASTNode(k) {}
};

class ExprStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::ExprStmt;
    std::shared_ptr<Expr> expr;
    ExprStmt(std::shared_ptr<Expr> e) : Stmt(KIND), expr(e) {}
};

class VarDeclStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::VarDeclStmt;
    Type type;
    std::string name;
    std::shared_ptr<Expr> initExpr;

    VarDeclStmt(const Type& t, const std::string& n, std::shared_ptr<Expr> init = nullptr)
        : Stmt(KIND), type(t), name(n), initExpr(init) {}
};

class BlockStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::BlockStmt;
    std::vector<std::shared_ptr<Stmt>> statements;
    BlockStmt(const std::vector<std::shared_ptr<Stmt>>& stmts = {}) : Stmt(KIND), statements(stmts) {}
};

class IfStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::IfStmt;
    std::shared_ptr<Expr> cond;
    std::shared_ptr<Stmt> thenBranch;
    std::shared_ptr<Stmt> elseBranch;

    IfStmt(std::shared_ptr<Expr> c, std::shared_ptr<Stmt> th, std::shared_ptr<Stmt> el = nullptr)
        : Stmt(KIND), cond(c), thenBranch(th), elseBranch(el) {}
};

class WhileStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::WhileStmt;
    std::shared_ptr<Expr> cond;
    std::shared_ptr<Stmt> body;

    WhileStmt(std::shared_ptr<Expr> c, std::shared_ptr<Stmt> b)
        : Stmt(KIND), cond(c), body(b) {}
};

class ForStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::ForStmt;
    std::shared_ptr<Stmt> init;
    std::shared_ptr<Expr> cond;
    std::shared_ptr<Expr> step;
    std::shared_ptr<Stmt> body;

    ForStmt(std::shared_ptr<Stmt> i, std::shared_ptr<Expr> c, std::shared_ptr<Expr> s, std::shared_ptr<Stmt> b)
        : Stmt(KIND), init(i), cond(c), step(s), body(b) {}
};

class ReturnStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::ReturnStmt;
    std::shared_ptr<Expr> expr;
    ReturnStmt(std::shared_ptr<Expr> e = nullptr) : Stmt(KIND), expr(e) {}
};

class BreakStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::BreakStmt;
    BreakStmt() : Stmt(KIND) {}
};

class ContinueStmt : public Stmt {
public:
    static constexpr ASTKind KIND = ASTKind::ContinueStmt;
    ContinueStmt() : Stmt(KIND) {}
};

struct ParamDecl {
    Type type;
    std::string name;
    ParamDecl(const Type& t, const std::string& n) : type(t), name(n) {}
};

class FunctionDecl : public ASTNode {
public:
    static constexpr ASTKind KIND = ASTKind::FunctionDecl;
    Type returnType;
    std::string name;
    std::vector<ParamDecl> params;
    std::shared_ptr<BlockStmt> body;
    bool isExtern = false;

    FunctionDecl(const Type& ret, const std::string& n, const std::vector<ParamDecl>& p, std::shared_ptr<BlockStmt> b, bool ext = false)
        : ASTNode(KIND), returnType(ret), name(n), params(p), body(b), isExtern(ext) {}
};

struct FieldDecl {
    Type type;
    std::string name;
    FieldDecl(const Type& t, const std::string& n) : type(t), name(n) {}
};

class StructDecl : public ASTNode {
public:
    static constexpr ASTKind KIND = ASTKind::StructDecl;
    std::string name;
    std::vector<FieldDecl> fields;
    StructDecl(const std::string& n, const std::vector<FieldDecl>& f) : ASTNode(KIND), name(n), fields(f) {}
};

class Program : public ASTNode {
public:
    static constexpr ASTKind KIND = ASTKind::Program;
    std::vector<std::shared_ptr<ASTNode>> decls;

    Program() : ASTNode(KIND) {}

    std::vector<std::shared_ptr<FunctionDecl>> getFunctions() const {
        std::vector<std::shared_ptr<FunctionDecl>> funcs;
        for (const auto& d : decls) {
            if (auto f = ast_cast<FunctionDecl>(d)) {
                funcs.push_back(f);
            }
        }
        return funcs;
    }

    std::vector<std::shared_ptr<StructDecl>> getStructs() const {
        std::vector<std::shared_ptr<StructDecl>> structs;
        for (const auto& d : decls) {
            if (auto s = ast_cast<StructDecl>(d)) {
                structs.push_back(s);
            }
        }
        return structs;
    }
};

} // namespace maiascript

#endif // MAIASCRIPT_AST_HPP
