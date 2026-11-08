#ifndef MAIASCRIPT_CODEGEN_RISCV_HPP
#define MAIASCRIPT_CODEGEN_RISCV_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "ast.hpp"

namespace maiascript {

struct CodegenOptions {
    bool rv32f = true;
    bool optimize = false;
    bool debugComments = true;
};

struct StructFieldInfo {
    std::string name;
    Type type;
    int offset;
};

struct StructInfo {
    std::string name;
    int totalSize;
    std::unordered_map<std::string, StructFieldInfo> fields;
};

class CodegenRISCV {
public:
    explicit CodegenRISCV(const CodegenOptions& opts = CodegenOptions());
    std::string generateAssembly(const std::shared_ptr<Program>& program);

private:
    CodegenOptions options;
    int labelCounter;
    int currentFrameSize;
    std::string currentFunc;

    std::vector<std::string> stringLiterals;
    std::unordered_map<std::string, std::string> stringLabels;
    std::unordered_map<std::string, StructInfo> structs;
    std::unordered_map<std::string, Type> currentVarTypes;
    std::unordered_map<std::string, Type> functionReturnTypes;
    Type currentFuncReturnType;

    std::string getOrCreateString(const std::string& str);
    int getTypeSize(const Type& t);
    int getElemSize(const Type& t);
    bool isFloatExpr(const std::shared_ptr<Expr>& expr);

    std::string newLabel(const std::string& prefix = "L");
    void emitFunction(const std::shared_ptr<FunctionDecl>& fn, std::stringstream& out);
    void emitStmt(const std::shared_ptr<Stmt>& stmt, std::stringstream& out,
                  std::unordered_map<std::string, int>& varOffsets);
    void emitExpr(const std::shared_ptr<Expr>& expr, const std::string& targetReg,
                  std::stringstream& out, std::unordered_map<std::string, int>& varOffsets,
                  int depth = 0);

    void scanLocals(const std::shared_ptr<Stmt>& stmt, std::vector<std::pair<std::string, Type>>& locals);
};

} // namespace maiascript

#endif // MAIASCRIPT_CODEGEN_RISCV_HPP
