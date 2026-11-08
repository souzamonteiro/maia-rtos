#include "codegen_riscv.hpp"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace maiascript {

CodegenRISCV::CodegenRISCV(const CodegenOptions& opts)
    : options(opts), labelCounter(0), currentFrameSize(0) {}

std::string CodegenRISCV::newLabel(const std::string& prefix) {
    std::stringstream ss;
    ss << ".L" << prefix << "_" << (++labelCounter);
    return ss.str();
}

std::string CodegenRISCV::resolveClassName(const std::string& name) const {
    std::string s = name;
    for (char& c : s) {
        if (c == '.') c = '_';
    }
    if (classes.find(s) != classes.end() || structs.find(s) != structs.end()) {
        return s;
    }
    for (const auto& ns : namespaces) {
        std::string cand = ns + "_" + s;
        if (classes.find(cand) != classes.end() || structs.find(cand) != structs.end()) {
            return cand;
        }
    }
    return s;
}

int CodegenRISCV::getTypeSize(const Type& t) {
    if (t.isPointer()) return 4;
    if (t.kind == TypeKind::Custom) {
        std::string sname = resolveClassName(t.name);
        auto it = structs.find(sname);
        if (it != structs.end()) {
            int s = it->second.totalSize;
            if (t.isArray()) s *= t.arraySize;
            return s;
        }
        return t.isArray() ? 4 * t.arraySize : 4;
    }
    if (t.kind == TypeKind::String) return 4;
    if (t.kind == TypeKind::Result || t.kind == TypeKind::Option) return 4;
    uint32_t sz = t.size();
    return sz > 0 ? (int)sz : 4;
}

int CodegenRISCV::getElemSize(const Type& t) {
    if (t.kind == TypeKind::String) return 1;
    if (t.kind == TypeKind::Char || t.kind == TypeKind::I8 || t.kind == TypeKind::U8) return 1;
    if (t.kind == TypeKind::I16 || t.kind == TypeKind::U16) return 2;
    if (t.kind == TypeKind::I64 || t.kind == TypeKind::U64 || t.kind == TypeKind::F64) return 8;
    return 4;
}

std::string CodegenRISCV::getOrCreateString(const std::string& str) {
    auto it = stringLabels.find(str);
    if (it != stringLabels.end()) {
        return it->second;
    }
    std::stringstream ss;
    ss << ".Lstr_" << stringLiterals.size();
    std::string lbl = ss.str();
    stringLiterals.push_back(str);
    stringLabels[str] = lbl;
    return lbl;
}

bool CodegenRISCV::isFloatExpr(const std::shared_ptr<Expr>& expr) {
    if (!expr) return false;
    if (auto lit = ast_cast<LiteralExpr>(expr)) {
        return lit->litKind == LiteralKind::Float;
    }
    if (auto v = ast_cast<VarExpr>(expr)) {
        auto it = currentVarTypes.find(v->name);
        if (it != currentVarTypes.end()) {
            return it->second.isFloat();
        }
        if (!currentClass.empty()) {
            auto sit = structs.find(currentClass);
            if (sit != structs.end()) {
                auto fit = sit->second.fields.find(v->name);
                if (fit != sit->second.fields.end()) {
                    return fit->second.type.isFloat();
                }
            }
        }
    }
    if (auto mem = ast_cast<MemberExpr>(expr)) {
        if (auto targetVar = ast_cast<VarExpr>(mem->target)) {
            std::string tName;
            if (targetVar->name == "this") {
                tName = currentClass;
            } else {
                auto vtIt = currentVarTypes.find(targetVar->name);
                if (vtIt != currentVarTypes.end()) tName = vtIt->second.name;
            }
            if (!tName.empty()) {
                auto sit = structs.find(tName);
                if (sit != structs.end()) {
                    auto fit = sit->second.fields.find(mem->member);
                    if (fit != sit->second.fields.end()) {
                        return fit->second.type.isFloat();
                    }
                }
            }
        }
    }
    if (auto bin = ast_cast<BinaryExpr>(expr)) {
        if (bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=" || bin->op == "==" || bin->op == "!=") {
            return false;
        }
        return isFloatExpr(bin->left) || isFloatExpr(bin->right);
    }
    if (auto u = ast_cast<UnaryExpr>(expr)) {
        if (u->op == "-") return isFloatExpr(u->operand);
    }
    if (auto call = ast_cast<CallExpr>(expr)) {
        auto fit = functionReturnTypes.find(call->callee);
        if (fit != functionReturnTypes.end()) {
            return fit->second.isFloat();
        }
    }
    if (auto mcall = ast_cast<MethodCallExpr>(expr)) {
        if (mcall->method == "value") {
            if (auto targetVar = ast_cast<VarExpr>(mcall->object)) {
                auto vtIt = currentVarTypes.find(targetVar->name);
                if (vtIt != currentVarTypes.end()) {
                    if (vtIt->second.name.find("<f32") != std::string::npos ||
                        vtIt->second.name.find("<f64") != std::string::npos) {
                        return true;
                    }
                }
            }
        }
        if (auto targetVar = ast_cast<VarExpr>(mcall->object)) {
            if (namespaces.count(targetVar->name)) {
                auto fit = functionReturnTypes.find(targetVar->name + "_" + mcall->method);
                if (fit != functionReturnTypes.end()) {
                    return fit->second.isFloat();
                }
            }
        }
        std::string cName;
        if (auto targetVar = ast_cast<VarExpr>(mcall->object)) {
            if (targetVar->name == "this") {
                cName = currentClass;
            } else {
                auto vtIt = currentVarTypes.find(targetVar->name);
                if (vtIt != currentVarTypes.end()) cName = vtIt->second.name;
            }
        }
        if (!cName.empty()) {
            auto fit = functionReturnTypes.find(cName + "_" + mcall->method);
            if (fit != functionReturnTypes.end()) {
                return fit->second.isFloat();
            }
        }
    }
    return false;
}

void CodegenRISCV::scanLocals(const std::shared_ptr<Stmt>& stmt, std::vector<std::pair<std::string, Type>>& locals) {
    if (!stmt) return;
    if (auto v = ast_cast<VarDeclStmt>(stmt)) {
        bool found = false;
        for (const auto& l : locals) {
            if (l.first == v->name) { found = true; break; }
        }
        if (!found) {
            locals.emplace_back(v->name, v->type);
        }
    } else if (auto b = ast_cast<BlockStmt>(stmt)) {
        for (const auto& s : b->statements) {
            scanLocals(s, locals);
        }
    } else if (auto ifs = ast_cast<IfStmt>(stmt)) {
        scanLocals(ifs->thenBranch, locals);
        scanLocals(ifs->elseBranch, locals);
    } else if (auto wh = ast_cast<WhileStmt>(stmt)) {
        scanLocals(wh->body, locals);
    } else if (auto fr = ast_cast<ForStmt>(stmt)) {
        scanLocals(fr->init, locals);
        scanLocals(fr->body, locals);
    } else if (auto df = ast_cast<DeferStmt>(stmt)) {
        scanLocals(df->stmt, locals);
    }
}

std::string CodegenRISCV::generateAssembly(const std::shared_ptr<Program>& program) {
    std::stringstream out;
    out << ".text\n";
    out << ".align 2\n\n";

    if (!program) return out.str();

    stringLiterals.clear();
    stringLabels.clear();
    structs.clear();
    classes.clear();
    namespaces = program->namespaces;
    currentClass.clear();
    functionReturnTypes.clear();
    globalVars.clear();
    for (const auto& g : program->getGlobals()) {
        globalVars[g->name] = g;
    }

    // 1. Pre-scan struct definitions
    for (const auto& s : program->getStructs()) {
        StructInfo sinfo;
        sinfo.name = s->name;
        int offset = 0;
        for (const auto& f : s->fields) {
            int fsize = f.type.size();
            if (fsize == 0) fsize = 4;
            int align = (fsize >= 4) ? 4 : (fsize == 2 ? 2 : 1);
            offset = (offset + align - 1) & ~(align - 1);
            sinfo.fields[f.name] = {f.name, f.type, offset};
            offset += fsize;
        }
        sinfo.totalSize = (offset + 3) & ~3;
        structs[s->name] = sinfo;
    }

    // 1b. Pre-scan class definitions
    for (const auto& c : program->getClasses()) {
        classes[c->name] = c;
        StructInfo sinfo;
        sinfo.name = c->name;
        int offset = 0;
        for (const auto& f : c->fields) {
            int fsize = f.type.size();
            if (fsize == 0) fsize = 4;
            int align = (fsize >= 4) ? 4 : (fsize == 2 ? 2 : 1);
            offset = (offset + align - 1) & ~(align - 1);
            sinfo.fields[f.name] = {f.name, f.type, offset};
            offset += fsize;
        }
        sinfo.totalSize = (offset + 3) & ~3;
        if (sinfo.totalSize == 0) sinfo.totalSize = 4;
        structs[c->name] = sinfo;

        bool hasCtor = false;
        for (const auto& m : c->methods) {
            std::string mName = m->isConstructor ? c->name : m->name;
            if (m->isConstructor) hasCtor = true;
            functionReturnTypes[c->name + "_" + mName] = m->returnType;
        }
        if (!hasCtor) {
            functionReturnTypes[c->name + "_" + c->name] = Type(TypeKind::Void);
        }
    }

    // 2. Pre-scan function signatures
    for (const auto& fn : program->getFunctions()) {
        functionReturnTypes[fn->name] = fn->returnType;
    }

    // 3. Emit functions
    for (const auto& fn : program->getFunctions()) {
        emitFunction(fn, out);
    }

    // 3b. Emit class methods
    for (const auto& c : program->getClasses()) {
        bool hasCtor = false;
        for (const auto& m : c->methods) {
            if (m->isConstructor) hasCtor = true;
            emitMethod(c, m, out);
        }
        if (!hasCtor) {
            std::string ctorName = c->name + "_" + c->name;
            out << ".globl " << ctorName << "\n";
            out << ctorName << ":\n";
            const auto& sinfo = structs[c->name];
            for (const auto& f : c->fields) {
                int fOffset = sinfo.fields.at(f.name).offset;
                if (f.defaultInit) {
                    if (auto lit = ast_cast<LiteralExpr>(f.defaultInit)) {
                        if (lit->litKind == LiteralKind::Int) {
                            out << "    li t0, " << lit->intVal << "\n";
                            out << "    sw t0, " << fOffset << "(a0)\n";
                        } else if (lit->litKind == LiteralKind::Float) {
                            float fl = (float)lit->floatVal;
                            uint32_t bits = 0;
                            std::memcpy(&bits, &fl, 4);
                            out << "    li t0, " << (int32_t)bits << "\n";
                            out << "    sw t0, " << fOffset << "(a0)\n";
                        } else {
                            out << "    sw zero, " << fOffset << "(a0)\n";
                        }
                    } else {
                        out << "    sw zero, " << fOffset << "(a0)\n";
                    }
                } else {
                    out << "    sw zero, " << fOffset << "(a0)\n";
                }
            }
            out << "    ret\n\n";
        }
    }

    // 4. Emit global variables in .data
    if (!globalVars.empty()) {
        out << ".data\n";
        out << ".align 2\n";
        for (const auto& pair : globalVars) {
            const auto& g = pair.second;
            out << ".globl " << g->name << "\n";
            out << g->name << ":\n";
            if (g->init) {
                if (auto lit = ast_cast<LiteralExpr>(g->init)) {
                    if (lit->litKind == LiteralKind::Int) {
                        out << "    .word " << lit->intVal << "\n";
                    } else if (lit->litKind == LiteralKind::String) {
                        std::string lbl = getOrCreateString(lit->strVal);
                        out << "    .word " << lbl << "\n";
                    } else if (lit->litKind == LiteralKind::Float) {
                        float fl = (float)lit->floatVal;
                        uint32_t bits = 0;
                        std::memcpy(&bits, &fl, 4);
                        out << "    .word " << (int32_t)bits << "\n";
                    } else {
                        out << "    .word 0\n";
                    }
                } else {
                    out << "    .word 0\n";
                }
            } else {
                out << "    .word 0\n";
            }
        }
        out << "\n";
    }

    // 5. Emit string literals in .rodata
    if (!stringLiterals.empty()) {
        out << ".section .rodata\n";
        out << ".align 2\n";
        for (const auto& s : stringLiterals) {
            std::string lbl = stringLabels[s];
            out << lbl << ":\n";
            if (!s.empty() && s.front() == '"' && s.back() == '"') {
                out << "    .string " << s << "\n";
            } else {
                out << "    .string \"" << s << "\"\n";
            }
        }
        out << "\n";
    }

    return optimizeAssembly(out.str());
}

std::string CodegenRISCV::optimizeAssembly(const std::string& input) {
    std::vector<std::string> lines;
    std::stringstream in(input);
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }

    auto trim_str = [](const std::string& s) -> std::string {
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return "";
        size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e - b + 1);
    };

    bool changed = true;
    int passes = 0;
    while (changed && passes < 10) {
        changed = false;
        passes++;
        std::vector<std::string> next_lines;
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string cur = lines[i];
            std::string cur_t = trim_str(cur);

            // 1. Remove NOP: addi r, r, 0 or mv r, r
            if (cur_t.rfind("addi ", 0) == 0) {
                std::string rest = cur_t.substr(5);
                size_t c1 = rest.find(',');
                size_t c2 = rest.rfind(',');
                if (c1 != std::string::npos && c2 != std::string::npos && c1 < c2) {
                    std::string r1 = trim_str(rest.substr(0, c1));
                    std::string r2 = trim_str(rest.substr(c1 + 1, c2 - c1 - 1));
                    std::string imm = trim_str(rest.substr(c2 + 1));
                    if (r1 == r2 && imm == "0") {
                        changed = true;
                        continue;
                    }
                }
            }
            if (cur_t.rfind("mv ", 0) == 0) {
                std::string rest = cur_t.substr(3);
                size_t c = rest.find(',');
                if (c != std::string::npos) {
                    std::string r1 = trim_str(rest.substr(0, c));
                    std::string r2 = trim_str(rest.substr(c + 1));
                    if (r1 == r2) {
                        changed = true;
                        continue;
                    }
                }
            }

            // 2. Lookahead optimizations
            if (i + 1 < lines.size()) {
                std::string next = lines[i + 1];
                std::string next_t = trim_str(next);

                // 2a. sw r, off(sp) followed immediately by lw r, off(sp)
                if (cur_t.rfind("sw ", 0) == 0 && next_t.rfind("lw ", 0) == 0) {
                    std::string sw_rest = cur_t.substr(3);
                    std::string lw_rest = next_t.substr(3);
                    if (sw_rest == lw_rest) {
                        next_lines.push_back(cur);
                        i++; // skip lw
                        changed = true;
                        continue;
                    }
                }

                // 2b. Jump to immediate next label: j .L or jal zero, .L followed by .L:
                if (cur_t.rfind("jal zero, ", 0) == 0 || cur_t.rfind("j ", 0) == 0) {
                    std::string tgt = (cur_t.rfind("jal zero, ", 0) == 0) ? cur_t.substr(10) : cur_t.substr(2);
                    tgt = trim_str(tgt);
                    if (next_t == tgt + ":") {
                        changed = true;
                        continue; // drop jump to next line
                    }
                }
            }

            next_lines.push_back(cur);
        }
        lines = next_lines;
    }

    std::stringstream out;
    for (const auto& l : lines) {
        out << l << "\n";
    }
    return out.str();
}

void CodegenRISCV::emitFunction(const std::shared_ptr<FunctionDecl>& fn, std::stringstream& out) {
    if (fn->isExtern) {
        out << ".globl " << fn->name << "\n";
        return;
    }

    currentFunc = fn->name;
    currentFuncReturnType = fn->returnType;
    currentVarTypes.clear();
    currentDefers.clear();

    out << ".globl " << fn->name << "\n";
    if (fn->name == "main") {
        out << ".globl maia_module_main\n";
    }
    out << fn->name << ":\n";
    if (fn->name == "main") {
        out << "maia_module_main:\n";
    }

    // 1. Collect params and locals
    std::vector<std::pair<std::string, Type>> locals;
    for (const auto& p : fn->params) {
        locals.emplace_back(p.name, p.type);
    }
    scanLocals(fn->body, locals);

    int scratchWords = 16;
    int runningOffset = scratchWords * 4;
    std::unordered_map<std::string, int> varOffsets;

    for (const auto& loc : locals) {
        int varSize = getTypeSize(loc.second);
        varSize = (varSize + 3) & ~3; // 4-byte align
        varOffsets[loc.first] = runningOffset;
        currentVarTypes[loc.first] = loc.second;
        runningOffset += varSize;
    }

    int totalBytes = runningOffset + 4; // for saved ra
    currentFrameSize = (totalBytes + 15) & ~15;
    int raOffset = currentFrameSize - 4;

    // Prologue
    out << "    addi sp, sp, -" << currentFrameSize << "\n";
    out << "    sw ra, " << raOffset << "(sp)\n";

    // Save incoming arguments to their stack slots
    int intArgIdx = 0;
    int floatArgIdx = 0;
    for (size_t i = 0; i < fn->params.size(); ++i) {
        int off = varOffsets[fn->params[i].name];
        if (fn->params[i].type.isFloat()) {
            if (floatArgIdx < 8) {
                out << "    fsw fa" << floatArgIdx++ << ", " << off << "(sp)\n";
            }
        } else {
            if (intArgIdx < 8) {
                out << "    sw a" << intArgIdx++ << ", " << off << "(sp)\n";
            }
        }
    }

    // Body
    if (fn->body) {
        for (const auto& s : fn->body->statements) {
            emitStmt(s, out, varOffsets);
        }
    }

    // Execute defers if fell through without explicit return
    if (!currentDefers.empty()) {
        for (auto it = currentDefers.rbegin(); it != currentDefers.rend(); ++it) {
            emitStmt(*it, out, varOffsets);
        }
    }

    // Default return 0 if reached end of function
    if (currentFuncReturnType.isFloat()) {
        out << "    fmv.w.x fa0, zero\n";
    } else {
        out << "    addi a0, zero, 0\n";
    }

    // Epilogue
    out << ".Lret_" << fn->name << ":\n";
    out << "    lw ra, " << raOffset << "(sp)\n";
    out << "    addi sp, sp, " << currentFrameSize << "\n";
    out << "    ret\n\n";
}

void CodegenRISCV::emitMethod(const std::shared_ptr<ClassDecl>& c,
                              const std::shared_ptr<MethodDecl>& m,
                              std::stringstream& out) {
    std::string mName = m->isConstructor ? c->name : m->name;
    std::string mangledName = c->name + "_" + mName;
    currentFunc = mangledName;
    currentClass = c->name;
    currentFuncReturnType = m->returnType;
    currentVarTypes.clear();
    currentDefers.clear();

    out << ".globl " << mangledName << "\n";
    out << mangledName << ":\n";

    // 1. Collect params and locals
    // Param 0 is implicit "this"
    std::vector<std::pair<std::string, Type>> locals;
    Type thisType(TypeKind::Custom, c->name, 1, 0); // ClassName*
    locals.emplace_back("this", thisType);

    for (const auto& p : m->params) {
        locals.emplace_back(p.name, p.type);
    }
    scanLocals(m->body, locals);

    int scratchWords = 16;
    int runningOffset = scratchWords * 4;
    std::unordered_map<std::string, int> varOffsets;

    for (const auto& loc : locals) {
        int varSize = getTypeSize(loc.second);
        varSize = (varSize + 3) & ~3; // 4-byte align
        varOffsets[loc.first] = runningOffset;
        currentVarTypes[loc.first] = loc.second;
        runningOffset += varSize;
    }

    int totalBytes = runningOffset + 4; // for saved ra
    currentFrameSize = (totalBytes + 15) & ~15;
    int raOffset = currentFrameSize - 4;

    // Prologue
    out << "    addi sp, sp, -" << currentFrameSize << "\n";
    out << "    sw ra, " << raOffset << "(sp)\n";

    // Save incoming "this" from a0 to its stack slot
    out << "    sw a0, " << varOffsets["this"] << "(sp)\n";

    // Save incoming arguments (starting at a1 / fa0)
    int intArgIdx = 1; // a1 onwards
    int floatArgIdx = 0;
    for (size_t i = 0; i < m->params.size(); ++i) {
        int off = varOffsets[m->params[i].name];
        if (m->params[i].type.isFloat()) {
            if (floatArgIdx < 8) {
                out << "    fsw fa" << floatArgIdx++ << ", " << off << "(sp)\n";
            }
        } else {
            if (intArgIdx < 8) {
                out << "    sw a" << intArgIdx++ << ", " << off << "(sp)\n";
            }
        }
    }

    // Body
    if (m->body) {
        for (const auto& s : m->body->statements) {
            emitStmt(s, out, varOffsets);
        }
    }

    // Execute defers if fell through without explicit return
    if (!currentDefers.empty()) {
        for (auto it = currentDefers.rbegin(); it != currentDefers.rend(); ++it) {
            emitStmt(*it, out, varOffsets);
        }
    }

    // Default return 0 or this (for constructor)
    if (m->isConstructor) {
        out << "    lw a0, " << varOffsets["this"] << "(sp)\n";
    } else if (currentFuncReturnType.isFloat()) {
        out << "    fmv.w.x fa0, zero\n";
    } else {
        out << "    addi a0, zero, 0\n";
    }

    // Epilogue
    out << ".Lret_" << mangledName << ":\n";
    out << "    lw ra, " << raOffset << "(sp)\n";
    out << "    addi sp, sp, " << currentFrameSize << "\n";
    out << "    ret\n\n";

    currentClass.clear();
}

void CodegenRISCV::emitStmt(const std::shared_ptr<Stmt>& stmt, std::stringstream& out,
                            std::unordered_map<std::string, int>& varOffsets) {
    if (!stmt) return;

    if (auto v = ast_cast<VarDeclStmt>(stmt)) {
        int off = varOffsets[v->name];
        std::string clsName = resolveClassName(v->type.name);
        if (classes.find(clsName) != classes.end() && !v->type.isPointer()) {
            auto call = ast_cast<CallExpr>(v->initExpr);
            if (call && call->callee == v->type.name) {
                // Constructor call with args
                for (size_t i = 0; i < call->args.size() && i < 7; ++i) {
                    int argSlot = (i + 1) * 4;
                    if (isFloatExpr(call->args[i])) {
                        emitExpr(call->args[i], "ft0", out, varOffsets, i + 1);
                        out << "    fsw ft0, " << argSlot << "(sp)\n";
                    } else {
                        emitExpr(call->args[i], "t0", out, varOffsets, i + 1);
                        out << "    sw t0, " << argSlot << "(sp)\n";
                    }
                }
                out << "    addi a0, sp, " << off << "\n";
                int intArgIdx = 1;
                int floatArgIdx = 0;
                for (size_t i = 0; i < call->args.size() && i < 7; ++i) {
                    int argSlot = (i + 1) * 4;
                    if (isFloatExpr(call->args[i])) {
                        if (floatArgIdx < 8) {
                            out << "    flw fa" << floatArgIdx++ << ", " << argSlot << "(sp)\n";
                        }
                    } else {
                        if (intArgIdx < 8) {
                            out << "    lw a" << intArgIdx++ << ", " << argSlot << "(sp)\n";
                        }
                    }
                }
                out << "    call " << clsName << "_" << clsName << "\n";
            } else if (!v->initExpr && functionReturnTypes.count(clsName + "_" + clsName)) {
                // Default constructor call
                out << "    addi a0, sp, " << off << "\n";
                out << "    call " << clsName << "_" << clsName << "\n";
            } else if (v->initExpr) {
                // Other initialization
                emitExpr(v->initExpr, "t0", out, varOffsets);
                out << "    sw t0, " << off << "(sp)\n";
            }
        } else if (v->initExpr) {
            if (v->type.isFloat()) {
                if (isFloatExpr(v->initExpr)) {
                    emitExpr(v->initExpr, "ft0", out, varOffsets);
                } else {
                    emitExpr(v->initExpr, "t0", out, varOffsets);
                    out << "    fcvt.s.w ft0, t0\n";
                }
                out << "    fsw ft0, " << off << "(sp)\n";
            } else {
                if (isFloatExpr(v->initExpr)) {
                    emitExpr(v->initExpr, "ft0", out, varOffsets);
                    out << "    fcvt.w.s t0, ft0\n";
                } else {
                    emitExpr(v->initExpr, "t0", out, varOffsets);
                }
                out << "    sw t0, " << off << "(sp)\n";
            }
        }
    } else if (auto e = ast_cast<ExprStmt>(stmt)) {
        if (isFloatExpr(e->expr)) {
            emitExpr(e->expr, "ft0", out, varOffsets);
        } else {
            emitExpr(e->expr, "t0", out, varOffsets);
        }
    } else if (auto r = ast_cast<ReturnStmt>(stmt)) {
        bool hasVal = (r->expr != nullptr);
        if (hasVal) {
            if (currentFuncReturnType.isFloat()) {
                if (isFloatExpr(r->expr)) {
                    emitExpr(r->expr, "fa0", out, varOffsets);
                } else {
                    emitExpr(r->expr, "t0", out, varOffsets);
                    out << "    fcvt.s.w fa0, t0\n";
                }
            } else {
                if (isFloatExpr(r->expr)) {
                    emitExpr(r->expr, "ft0", out, varOffsets);
                    out << "    fcvt.w.s a0, ft0\n";
                } else {
                    emitExpr(r->expr, "a0", out, varOffsets);
                }
            }
        }
        if (!currentDefers.empty()) {
            if (hasVal) {
                if (currentFuncReturnType.isFloat()) {
                    out << "    fsw fa0, 0(sp)\n";
                } else {
                    out << "    sw a0, 0(sp)\n";
                }
            }
            for (auto it = currentDefers.rbegin(); it != currentDefers.rend(); ++it) {
                emitStmt(*it, out, varOffsets);
            }
            if (hasVal) {
                if (currentFuncReturnType.isFloat()) {
                    out << "    flw fa0, 0(sp)\n";
                } else {
                    out << "    lw a0, 0(sp)\n";
                }
            }
        }
        out << "    jal zero, .Lret_" << currentFunc << "\n";
    } else if (auto def = ast_cast<DeferStmt>(stmt)) {
        currentDefers.push_back(def->stmt);
    } else if (auto b = ast_cast<BlockStmt>(stmt)) {
        for (const auto& s : b->statements) {
            emitStmt(s, out, varOffsets);
        }
    } else if (auto ifs = ast_cast<IfStmt>(stmt)) {
        std::string lblElse = newLabel("else");
        std::string lblEnd = newLabel("ifend");

        emitExpr(ifs->cond, "t0", out, varOffsets);
        out << "    beq t0, zero, " << lblElse << "\n";
        emitStmt(ifs->thenBranch, out, varOffsets);
        out << "    jal zero, " << lblEnd << "\n";
        out << lblElse << ":\n";
        if (ifs->elseBranch) {
            emitStmt(ifs->elseBranch, out, varOffsets);
        }
        out << lblEnd << ":\n";
    } else if (auto wh = ast_cast<WhileStmt>(stmt)) {
        std::string lblStart = newLabel("while_start");
        std::string lblEnd = newLabel("while_end");

        out << lblStart << ":\n";
        emitExpr(wh->cond, "t0", out, varOffsets);
        out << "    beq t0, zero, " << lblEnd << "\n";
        emitStmt(wh->body, out, varOffsets);
        out << "    jal zero, " << lblStart << "\n";
        out << lblEnd << ":\n";
    } else if (auto fr = ast_cast<ForStmt>(stmt)) {
        std::string lblStart = newLabel("for_start");
        std::string lblEnd = newLabel("for_end");

        if (fr->init) emitStmt(fr->init, out, varOffsets);
        out << lblStart << ":\n";
        if (fr->cond) {
            emitExpr(fr->cond, "t0", out, varOffsets);
            out << "    beq t0, zero, " << lblEnd << "\n";
        }
        emitStmt(fr->body, out, varOffsets);
        if (fr->step) {
            emitExpr(fr->step, "t0", out, varOffsets);
        }
        out << "    jal zero, " << lblStart << "\n";
        out << lblEnd << ":\n";
    }
}

void CodegenRISCV::emitExpr(const std::shared_ptr<Expr>& expr, const std::string& targetReg,
                            std::stringstream& out, std::unordered_map<std::string, int>& varOffsets,
                            int depth) {
    if (!expr) return;

    if (auto lit = ast_cast<LiteralExpr>(expr)) {
        if (lit->litKind == LiteralKind::Int) {
            if (targetReg[0] == 'f') {
                out << "    li t0, " << lit->intVal << "\n";
                out << "    fcvt.s.w " << targetReg << ", t0\n";
            } else {
                if (lit->intVal >= -2048 && lit->intVal <= 2047) {
                    out << "    addi " << targetReg << ", zero, " << lit->intVal << "\n";
                } else {
                    out << "    li " << targetReg << ", " << lit->intVal << "\n";
                }
            }
        } else if (lit->litKind == LiteralKind::Float) {
            float f = (float)lit->floatVal;
            uint32_t bits = 0;
            std::memcpy(&bits, &f, 4);
            out << "    li t0, " << (int32_t)bits << "\n";
            if (targetReg[0] == 'f') {
                out << "    fmv.w.x " << targetReg << ", t0\n";
            } else {
                out << "    addi " << targetReg << ", t0, 0\n";
            }
        } else if (lit->litKind == LiteralKind::Bool) {
            int val = (lit->strVal == "true") ? 1 : 0;
            out << "    addi " << targetReg << ", zero, " << val << "\n";
        } else if (lit->litKind == LiteralKind::Null) {
            out << "    addi " << targetReg << ", zero, 0\n";
        } else if (lit->litKind == LiteralKind::String) {
            std::string lbl = getOrCreateString(lit->strVal);
            out << "    la " << targetReg << ", " << lbl << "\n";
        }
    } else if (auto v = ast_cast<VarExpr>(expr)) {
        auto it = varOffsets.find(v->name);
        if (it != varOffsets.end()) {
            auto vtIt = currentVarTypes.find(v->name);
            if (vtIt != currentVarTypes.end()) {
                if (vtIt->second.isFloat()) {
                    if (targetReg[0] == 'f') {
                        out << "    flw " << targetReg << ", " << it->second << "(sp)\n";
                    } else {
                        out << "    flw ft0, " << it->second << "(sp)\n";
                        out << "    fcvt.w.s " << targetReg << ", ft0\n";
                    }
                    return;
                }
                if ((vtIt->second.kind == TypeKind::Custom && !vtIt->second.isPointer()) || vtIt->second.isArray()) {
                    out << "    addi " << targetReg << ", sp, " << it->second << "\n";
                    return;
                }
            }
            if (targetReg[0] == 'f') {
                out << "    lw t0, " << it->second << "(sp)\n";
                out << "    fcvt.s.w " << targetReg << ", t0\n";
            } else {
                out << "    lw " << targetReg << ", " << it->second << "(sp)\n";
            }
        } else if (functionReturnTypes.count(v->name)) {
            out << "    la " << targetReg << ", " << v->name << "\n";
            return;
        } else if (globalVars.count(v->name)) {
            bool isFloatVar = globalVars[v->name]->type.isFloat();
            out << "    la t1, " << v->name << "\n";
            if (isFloatVar && targetReg[0] == 'f') {
                out << "    flw " << targetReg << ", 0(t1)\n";
            } else {
                out << "    lw " << targetReg << ", 0(t1)\n";
            }
            return;
        } else if (!currentClass.empty()) {
            auto sit = structs.find(currentClass);
            if (sit != structs.end()) {
                auto fit = sit->second.fields.find(v->name);
                if (fit != sit->second.fields.end()) {
                    int mOffset = fit->second.offset;
                    bool isFieldFloat = fit->second.type.isFloat();
                    out << "    lw t1, " << varOffsets["this"] << "(sp)\n";
                    if (isFieldFloat && targetReg[0] == 'f') {
                        out << "    flw " << targetReg << ", " << mOffset << "(t1)\n";
                    } else {
                        out << "    lw " << targetReg << ", " << mOffset << "(t1)\n";
                    }
                    return;
                }
            }
            out << "    # unknown variable " << v->name << "\n";
            out << "    addi " << targetReg << ", zero, 0\n";
        } else {
            out << "    # unknown variable " << v->name << "\n";
            out << "    addi " << targetReg << ", zero, 0\n";
        }
    } else if (auto mem = ast_cast<MemberExpr>(expr)) {
        if (mem->isArrow) {
            emitExpr(mem->target, "t0", out, varOffsets, depth);
            int mOffset = 0;
            bool isFieldFloat = false;
            if (auto targetVar = ast_cast<VarExpr>(mem->target)) {
                if (targetVar->name == "this") {
                    auto sit = structs.find(currentClass);
                    if (sit != structs.end()) {
                        auto fit = sit->second.fields.find(mem->member);
                        if (fit != sit->second.fields.end()) {
                            mOffset = fit->second.offset;
                            isFieldFloat = fit->second.type.isFloat();
                        }
                    }
                } else {
                    auto vtIt = currentVarTypes.find(targetVar->name);
                    if (vtIt != currentVarTypes.end()) {
                        auto sit = structs.find(resolveClassName(vtIt->second.name));
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) {
                                mOffset = fit->second.offset;
                                isFieldFloat = fit->second.type.isFloat();
                            }
                        }
                    } else if (globalVars.count(targetVar->name)) {
                        auto sit = structs.find(resolveClassName(globalVars[targetVar->name]->type.name));
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) {
                                mOffset = fit->second.offset;
                                isFieldFloat = fit->second.type.isFloat();
                            }
                        }
                    }
                }
            }
            if (isFieldFloat && targetReg[0] == 'f') {
                out << "    flw " << targetReg << ", " << mOffset << "(t0)\n";
            } else {
                out << "    lw " << targetReg << ", " << mOffset << "(t0)\n";
            }
        } else {
            if (auto targetVar = ast_cast<VarExpr>(mem->target)) {
                if (targetVar->name == "this") {
                    auto sit = structs.find(currentClass);
                    int mOffset = 0;
                    bool isFieldFloat = false;
                    if (sit != structs.end()) {
                        auto fit = sit->second.fields.find(mem->member);
                        if (fit != sit->second.fields.end()) {
                            mOffset = fit->second.offset;
                            isFieldFloat = fit->second.type.isFloat();
                        }
                    }
                    out << "    lw t0, " << varOffsets["this"] << "(sp)\n";
                    if (isFieldFloat && targetReg[0] == 'f') {
                        out << "    flw " << targetReg << ", " << mOffset << "(t0)\n";
                    } else {
                        out << "    lw " << targetReg << ", " << mOffset << "(t0)\n";
                    }
                } else {
                    auto vtIt = currentVarTypes.find(targetVar->name);
                    int mOffset = 0;
                    bool isFieldFloat = false;
                    if (vtIt != currentVarTypes.end()) {
                        auto sit = structs.find(resolveClassName(vtIt->second.name));
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) {
                                mOffset = fit->second.offset;
                                isFieldFloat = fit->second.type.isFloat();
                            }
                        }
                    } else if (globalVars.count(targetVar->name)) {
                        auto sit = structs.find(resolveClassName(globalVars[targetVar->name]->type.name));
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) {
                                mOffset = fit->second.offset;
                                isFieldFloat = fit->second.type.isFloat();
                            }
                        }
                    }
                    int totalOff = varOffsets[targetVar->name] + mOffset;
                    if (isFieldFloat && targetReg[0] == 'f') {
                        out << "    flw " << targetReg << ", " << totalOff << "(sp)\n";
                    } else {
                        out << "    lw " << targetReg << ", " << totalOff << "(sp)\n";
                    }
                }
            } else {
                emitExpr(mem->target, "t0", out, varOffsets, depth);
                out << "    lw " << targetReg << ", 0(t0)\n";
            }
        }
    } else if (auto idx = ast_cast<IndexExpr>(expr)) {
        int scratchOffset = depth * 4;
        int esize = 4;
        if (auto targetVar = ast_cast<VarExpr>(idx->target)) {
            auto vtIt = currentVarTypes.find(targetVar->name);
            if (vtIt != currentVarTypes.end()) {
                esize = getElemSize(vtIt->second);
                if ((vtIt->second.kind == TypeKind::Custom && !vtIt->second.isPointer()) || vtIt->second.isArray()) {
                    int off = varOffsets[targetVar->name];
                    out << "    addi t0, sp, " << off << "\n";
                } else {
                    emitExpr(idx->target, "t0", out, varOffsets, depth);
                }
            } else {
                emitExpr(idx->target, "t0", out, varOffsets, depth);
            }
        } else {
            emitExpr(idx->target, "t0", out, varOffsets, depth);
        }

        out << "    sw t0, " << scratchOffset << "(sp)\n";
        emitExpr(idx->index, "t1", out, varOffsets, depth + 1);
        out << "    lw t0, " << scratchOffset << "(sp)\n";

        if (esize == 1) {
            out << "    add t0, t0, t1\n";
            out << "    lbu " << targetReg << ", 0(t0)\n";
        } else if (esize == 2) {
            out << "    slli t1, t1, 1\n";
            out << "    add t0, t0, t1\n";
            out << "    lh " << targetReg << ", 0(t0)\n";
        } else {
            out << "    slli t1, t1, 2\n";
            out << "    add t0, t0, t1\n";
            if (targetReg[0] == 'f') {
                out << "    flw " << targetReg << ", 0(t0)\n";
            } else {
                out << "    lw " << targetReg << ", 0(t0)\n";
            }
        }
    } else if (auto u = ast_cast<UnaryExpr>(expr)) {
        if (u->op == "&") {
            if (auto targetVar = ast_cast<VarExpr>(u->operand)) {
                int off = varOffsets[targetVar->name];
                out << "    addi " << targetReg << ", sp, " << off << "\n";
            } else if (auto mem = ast_cast<MemberExpr>(u->operand)) {
                if (auto targetVar = ast_cast<VarExpr>(mem->target)) {
                    auto vtIt = currentVarTypes.find(targetVar->name);
                    int mOffset = 0;
                    if (vtIt != currentVarTypes.end()) {
                        auto sit = structs.find(vtIt->second.name);
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) mOffset = fit->second.offset;
                        }
                    }
                    int totalOff = varOffsets[targetVar->name] + mOffset;
                    out << "    addi " << targetReg << ", sp, " << totalOff << "\n";
                }
            }
        } else if (u->op == "*") {
            emitExpr(u->operand, targetReg, out, varOffsets, depth);
            out << "    lw " << targetReg << ", 0(" << targetReg << ")\n";
        } else if (u->op == "-") {
            if (isFloatExpr(u->operand)) {
                emitExpr(u->operand, "ft0", out, varOffsets, depth);
                out << "    fsgnjn.s " << targetReg << ", ft0, ft0\n";
            } else {
                emitExpr(u->operand, targetReg, out, varOffsets, depth);
                out << "    sub " << targetReg << ", zero, " << targetReg << "\n";
            }
        } else if (u->op == "!") {
            emitExpr(u->operand, targetReg, out, varOffsets, depth);
            out << "    seqz " << targetReg << ", " << targetReg << "\n";
        } else if (u->op == "~") {
            emitExpr(u->operand, targetReg, out, varOffsets, depth);
            out << "    not " << targetReg << ", " << targetReg << "\n";
        } else if (u->op == "++") {
            if (auto targetVar = ast_cast<VarExpr>(u->operand)) {
                int off = varOffsets[targetVar->name];
                out << "    lw " << targetReg << ", " << off << "(sp)\n";
                out << "    addi t2, " << targetReg << ", 1\n";
                out << "    sw t2, " << off << "(sp)\n";
                if (u->isPrefix) {
                    out << "    addi " << targetReg << ", " << targetReg << ", 1\n";
                }
            }
        } else if (u->op == "--") {
            if (auto targetVar = ast_cast<VarExpr>(u->operand)) {
                int off = varOffsets[targetVar->name];
                out << "    lw " << targetReg << ", " << off << "(sp)\n";
                out << "    addi t2, " << targetReg << ", -1\n";
                out << "    sw t2, " << off << "(sp)\n";
                if (u->isPrefix) {
                    out << "    addi " << targetReg << ", " << targetReg << ", -1\n";
                }
            }
        }
    } else if (auto bin = ast_cast<BinaryExpr>(expr)) {
        if (isFloatExpr(bin->left) || isFloatExpr(bin->right)) {
            int scratchOffset = depth * 4;
            emitExpr(bin->left, "ft0", out, varOffsets, depth);
            out << "    fsw ft0, " << scratchOffset << "(sp)\n";
            emitExpr(bin->right, "ft1", out, varOffsets, depth + 1);
            out << "    flw ft0, " << scratchOffset << "(sp)\n";

            const std::string& op = bin->op;
            if (op == "+") {
                out << "    fadd.s " << targetReg << ", ft0, ft1\n";
            } else if (op == "-") {
                out << "    fsub.s " << targetReg << ", ft0, ft1\n";
            } else if (op == "*") {
                out << "    fmul.s " << targetReg << ", ft0, ft1\n";
            } else if (op == "/") {
                out << "    fdiv.s " << targetReg << ", ft0, ft1\n";
            } else if (op == "<") {
                out << "    flt.s " << targetReg << ", ft0, ft1\n";
            } else if (op == "<=") {
                out << "    fle.s " << targetReg << ", ft0, ft1\n";
            } else if (op == ">") {
                out << "    flt.s " << targetReg << ", ft1, ft0\n";
            } else if (op == ">=") {
                out << "    fle.s " << targetReg << ", ft1, ft0\n";
            } else if (op == "==") {
                out << "    feq.s " << targetReg << ", ft0, ft1\n";
            } else if (op == "!=") {
                out << "    feq.s t2, ft0, ft1\n";
                out << "    xori " << targetReg << ", t2, 1\n";
            }
            return;
        }

        // 1. Constant Folding: Both operands are integer literals
        auto litL = ast_cast<LiteralExpr>(bin->left);
        auto litR = ast_cast<LiteralExpr>(bin->right);
        if (litL && litR && litL->litKind == LiteralKind::Int && litR->litKind == LiteralKind::Int) {
            int64_t l = litL->intVal;
            int64_t r = litR->intVal;
            int64_t res = 0;
            const std::string& op = bin->op;
            if (op == "+") res = l + r;
            else if (op == "-") res = l - r;
            else if (op == "*") res = l * r;
            else if (op == "/" && r != 0) res = l / r;
            else if (op == "%" && r != 0) res = l % r;
            else if (op == "&") res = l & r;
            else if (op == "|") res = l | r;
            else if (op == "^") res = l ^ r;
            else if (op == "<<") res = l << r;
            else if (op == ">>") res = l >> r;
            else if (op == "<") res = (l < r) ? 1 : 0;
            else if (op == ">") res = (l > r) ? 1 : 0;
            else if (op == "<=") res = (l <= r) ? 1 : 0;
            else if (op == ">=") res = (l >= r) ? 1 : 0;
            else if (op == "==") res = (l == r) ? 1 : 0;
            else if (op == "!=") res = (l != r) ? 1 : 0;
            if (res >= -2048 && res <= 2047) {
                out << "    addi " << targetReg << ", zero, " << res << "\n";
            } else {
                out << "    li " << targetReg << ", " << res << "\n";
            }
            return;
        }

        // 2. Immediate Optimization: Right operand is integer literal
        if (litR && litR->litKind == LiteralKind::Int) {
            int64_t val = litR->intVal;
            const std::string& op = bin->op;
            if (op == "+" && val >= -2048 && val <= 2047) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    addi " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            } else if (op == "-" && (-val) >= -2048 && (-val) <= 2047) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    addi " << targetReg << ", " << targetReg << ", " << (-val) << "\n";
                return;
            } else if (op == "<<" && val >= 0 && val <= 31) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    slli " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            } else if (op == ">>" && val >= 0 && val <= 31) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    srai " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            } else if (op == "&" && val >= -2048 && val <= 2047) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    andi " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            } else if (op == "|" && val >= -2048 && val <= 2047) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    ori " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            } else if (op == "^" && val >= -2048 && val <= 2047) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    xori " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            } else if (op == "<" && val >= -2048 && val <= 2047) {
                emitExpr(bin->left, targetReg, out, varOffsets, depth);
                out << "    slti " << targetReg << ", " << targetReg << ", " << val << "\n";
                return;
            }
        }

        int scratchOffset = depth * 4;
        emitExpr(bin->left, "t0", out, varOffsets, depth);
        out << "    sw t0, " << scratchOffset << "(sp)\n";
        emitExpr(bin->right, "t1", out, varOffsets, depth + 1);
        out << "    lw t0, " << scratchOffset << "(sp)\n";

        const std::string& op = bin->op;
        if (op == "+") {
            out << "    add " << targetReg << ", t0, t1\n";
        } else if (op == "-") {
            out << "    sub " << targetReg << ", t0, t1\n";
        } else if (op == "*") {
            out << "    mul " << targetReg << ", t0, t1\n";
        } else if (op == "/") {
            out << "    div " << targetReg << ", t0, t1\n";
        } else if (op == "%") {
            out << "    rem " << targetReg << ", t0, t1\n";
        } else if (op == "&") {
            out << "    and " << targetReg << ", t0, t1\n";
        } else if (op == "|") {
            out << "    or " << targetReg << ", t0, t1\n";
        } else if (op == "^") {
            out << "    xor " << targetReg << ", t0, t1\n";
        } else if (op == "<<") {
            out << "    sll " << targetReg << ", t0, t1\n";
        } else if (op == ">>") {
            out << "    sra " << targetReg << ", t0, t1\n";
        } else if (op == "<") {
            out << "    slt " << targetReg << ", t0, t1\n";
        } else if (op == ">") {
            out << "    slt " << targetReg << ", t1, t0\n";
        } else if (op == "<=") {
            out << "    slt t2, t1, t0\n";
            out << "    xori " << targetReg << ", t2, 1\n";
        } else if (op == ">=") {
            out << "    slt t2, t0, t1\n";
            out << "    xori " << targetReg << ", t2, 1\n";
        } else if (op == "==") {
            out << "    sub t2, t0, t1\n";
            out << "    seqz " << targetReg << ", t2\n";
        } else if (op == "!=") {
            out << "    sub t2, t0, t1\n";
            out << "    snez " << targetReg << ", t2\n";
        }
    } else if (auto asgn = ast_cast<AssignExpr>(expr)) {
        if (auto v = ast_cast<VarExpr>(asgn->target)) {
            auto it = varOffsets.find(v->name);
            if (it == varOffsets.end() && !currentClass.empty()) {
                auto sit = structs.find(currentClass);
                if (sit != structs.end()) {
                    auto fit = sit->second.fields.find(v->name);
                    if (fit != sit->second.fields.end()) {
                        int mOffset = fit->second.offset;
                        bool isFieldFloat = fit->second.type.isFloat();
                        if (isFieldFloat) {
                            if (isFloatExpr(asgn->value)) {
                                emitExpr(asgn->value, "ft0", out, varOffsets, depth);
                            } else {
                                emitExpr(asgn->value, "t0", out, varOffsets, depth);
                                out << "    fcvt.s.w ft0, t0\n";
                            }
                            out << "    lw t1, " << varOffsets["this"] << "(sp)\n";
                            if (asgn->op == "+=") {
                                out << "    flw ft1, " << mOffset << "(t1)\n";
                                out << "    fadd.s ft0, ft1, ft0\n";
                            } else if (asgn->op == "-=") {
                                out << "    flw ft1, " << mOffset << "(t1)\n";
                                out << "    fsub.s ft0, ft1, ft0\n";
                            } else if (asgn->op == "*=") {
                                out << "    flw ft1, " << mOffset << "(t1)\n";
                                out << "    fmul.s ft0, ft1, ft0\n";
                            } else if (asgn->op == "/=") {
                                out << "    flw ft1, " << mOffset << "(t1)\n";
                                out << "    fdiv.s ft0, ft1, ft0\n";
                            }
                            out << "    fsw ft0, " << mOffset << "(t1)\n";
                            if (targetReg != "ft0" && targetReg[0] == 'f') {
                                out << "    fsgnj.s " << targetReg << ", ft0, ft0\n";
                            }
                            return;
                        }

                        emitExpr(asgn->value, "t0", out, varOffsets, depth);
                        out << "    lw t1, " << varOffsets["this"] << "(sp)\n";
                        if (asgn->op == "+=") {
                            out << "    lw t2, " << mOffset << "(t1)\n";
                            out << "    add t0, t2, t0\n";
                        } else if (asgn->op == "-=") {
                            out << "    lw t2, " << mOffset << "(t1)\n";
                            out << "    sub t0, t2, t0\n";
                        } else if (asgn->op == "*=") {
                            out << "    lw t2, " << mOffset << "(t1)\n";
                            out << "    mul t0, t2, t0\n";
                        }
                        out << "    sw t0, " << mOffset << "(t1)\n";
                        if (targetReg != "t0") {
                            out << "    addi " << targetReg << ", t0, 0\n";
                        }
                        return;
                    }
                }
            }

            if (globalVars.count(v->name)) {
                bool isFloatVar = globalVars[v->name]->type.isFloat();
                if (isFloatVar) {
                    if (isFloatExpr(asgn->value)) {
                        emitExpr(asgn->value, "ft0", out, varOffsets, depth);
                    } else {
                        emitExpr(asgn->value, "t0", out, varOffsets, depth);
                        out << "    fcvt.s.w ft0, t0\n";
                    }
                    out << "    la t1, " << v->name << "\n";
                    out << "    fsw ft0, 0(t1)\n";
                    if (targetReg != "ft0" && targetReg[0] == 'f') {
                        out << "    fsgnj.s " << targetReg << ", ft0, ft0\n";
                    }
                    return;
                }
                emitExpr(asgn->value, "t0", out, varOffsets, depth);
                out << "    la t1, " << v->name << "\n";
                if (asgn->op == "+=") {
                    out << "    lw t2, 0(t1)\n";
                    out << "    add t0, t2, t0\n";
                } else if (asgn->op == "-=") {
                    out << "    lw t2, 0(t1)\n";
                    out << "    sub t0, t2, t0\n";
                } else if (asgn->op == "*=") {
                    out << "    lw t2, 0(t1)\n";
                    out << "    mul t0, t2, t0\n";
                }
                out << "    sw t0, 0(t1)\n";
                if (targetReg != "t0") {
                    out << "    addi " << targetReg << ", t0, 0\n";
                }
                return;
            }

            int off = varOffsets[v->name];
            auto vtIt = currentVarTypes.find(v->name);
            bool isTargetFloat = (vtIt != currentVarTypes.end() && vtIt->second.isFloat());
            if (isTargetFloat) {
                if (isFloatExpr(asgn->value)) {
                    emitExpr(asgn->value, "ft0", out, varOffsets, depth);
                } else {
                    emitExpr(asgn->value, "t0", out, varOffsets, depth);
                    out << "    fcvt.s.w ft0, t0\n";
                }
                if (asgn->op == "+=") {
                    out << "    flw ft1, " << off << "(sp)\n";
                    out << "    fadd.s ft0, ft1, ft0\n";
                } else if (asgn->op == "-=") {
                    out << "    flw ft1, " << off << "(sp)\n";
                    out << "    fsub.s ft0, ft1, ft0\n";
                } else if (asgn->op == "*=") {
                    out << "    flw ft1, " << off << "(sp)\n";
                    out << "    fmul.s ft0, ft1, ft0\n";
                } else if (asgn->op == "/=") {
                    out << "    flw ft1, " << off << "(sp)\n";
                    out << "    fdiv.s ft0, ft1, ft0\n";
                }
                out << "    fsw ft0, " << off << "(sp)\n";
                if (targetReg != "ft0" && targetReg[0] == 'f') {
                    out << "    fsgnj.s " << targetReg << ", ft0, ft0\n";
                }
                return;
            }

            emitExpr(asgn->value, "t0", out, varOffsets, depth);
            if (asgn->op == "+=") {
                out << "    lw t1, " << off << "(sp)\n";
                out << "    add t0, t1, t0\n";
            } else if (asgn->op == "-=") {
                out << "    lw t1, " << off << "(sp)\n";
                out << "    sub t0, t1, t0\n";
            } else if (asgn->op == "*=") {
                out << "    lw t1, " << off << "(sp)\n";
                out << "    mul t0, t1, t0\n";
            }
            out << "    sw t0, " << off << "(sp)\n";
            if (targetReg != "t0") {
                out << "    addi " << targetReg << ", t0, 0\n";
            }
        } else if (auto mem = ast_cast<MemberExpr>(asgn->target)) {
            int scratchOffset = depth * 4;
            emitExpr(asgn->value, "t0", out, varOffsets, depth);
            out << "    sw t0, " << scratchOffset << "(sp)\n";

            if (mem->isArrow) {
                emitExpr(mem->target, "t1", out, varOffsets, depth + 1);
                int mOffset = 0;
                if (auto targetVar = ast_cast<VarExpr>(mem->target)) {
                    if (targetVar->name == "this") {
                        auto sit = structs.find(currentClass);
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) mOffset = fit->second.offset;
                        }
                    } else {
                        auto vtIt = currentVarTypes.find(targetVar->name);
                        if (vtIt != currentVarTypes.end()) {
                            auto sit = structs.find(resolveClassName(vtIt->second.name));
                            if (sit != structs.end()) {
                                auto fit = sit->second.fields.find(mem->member);
                                if (fit != sit->second.fields.end()) mOffset = fit->second.offset;
                            }
                        }
                    }
                }
                out << "    lw t0, " << scratchOffset << "(sp)\n";
                if (asgn->op == "+=") {
                    out << "    lw t2, " << mOffset << "(t1)\n";
                    out << "    add t0, t2, t0\n";
                } else if (asgn->op == "-=") {
                    out << "    lw t2, " << mOffset << "(t1)\n";
                    out << "    sub t0, t2, t0\n";
                } else if (asgn->op == "*=") {
                    out << "    lw t2, " << mOffset << "(t1)\n";
                    out << "    mul t0, t2, t0\n";
                }
                out << "    sw t0, " << mOffset << "(t1)\n";
            } else {
                if (auto targetVar = ast_cast<VarExpr>(mem->target)) {
                    if (targetVar->name == "this") {
                        auto sit = structs.find(currentClass);
                        int mOffset = 0;
                        if (sit != structs.end()) {
                            auto fit = sit->second.fields.find(mem->member);
                            if (fit != sit->second.fields.end()) mOffset = fit->second.offset;
                        }
                        out << "    lw t1, " << varOffsets["this"] << "(sp)\n";
                        out << "    lw t0, " << scratchOffset << "(sp)\n";
                        if (asgn->op == "+=") {
                            out << "    lw t2, " << mOffset << "(t1)\n";
                            out << "    add t0, t2, t0\n";
                        } else if (asgn->op == "-=") {
                            out << "    lw t2, " << mOffset << "(t1)\n";
                            out << "    sub t0, t2, t0\n";
                        } else if (asgn->op == "*=") {
                            out << "    lw t2, " << mOffset << "(t1)\n";
                            out << "    mul t0, t2, t0\n";
                        }
                        out << "    sw t0, " << mOffset << "(t1)\n";
                    } else {
                        auto vtIt = currentVarTypes.find(targetVar->name);
                        int mOffset = 0;
                        if (vtIt != currentVarTypes.end()) {
                            auto sit = structs.find(resolveClassName(vtIt->second.name));
                            if (sit != structs.end()) {
                                auto fit = sit->second.fields.find(mem->member);
                                if (fit != sit->second.fields.end()) mOffset = fit->second.offset;
                            }
                        }
                        int totalOff = varOffsets[targetVar->name] + mOffset;
                        out << "    lw t0, " << scratchOffset << "(sp)\n";
                        if (asgn->op == "+=") {
                            out << "    lw t2, " << totalOff << "(sp)\n";
                            out << "    add t0, t2, t0\n";
                        } else if (asgn->op == "-=") {
                            out << "    lw t2, " << totalOff << "(sp)\n";
                            out << "    sub t0, t2, t0\n";
                        } else if (asgn->op == "*=") {
                            out << "    lw t2, " << totalOff << "(sp)\n";
                            out << "    mul t0, t2, t0\n";
                        }
                        out << "    sw t0, " << totalOff << "(sp)\n";
                    }
                }
            }
            if (targetReg != "t0") {
                out << "    addi " << targetReg << ", t0, 0\n";
            }
        } else if (auto idx = ast_cast<IndexExpr>(asgn->target)) {
            int scratchVal = depth * 4;
            int scratchTarget = (depth + 1) * 4;

            emitExpr(asgn->value, "t0", out, varOffsets, depth);
            out << "    sw t0, " << scratchVal << "(sp)\n";

            int esize = 4;
            if (auto targetVar = ast_cast<VarExpr>(idx->target)) {
                auto vtIt = currentVarTypes.find(targetVar->name);
                if (vtIt != currentVarTypes.end()) {
                    esize = getElemSize(vtIt->second);
                    if ((vtIt->second.kind == TypeKind::Custom && !vtIt->second.isPointer()) || vtIt->second.isArray()) {
                        int off = varOffsets[targetVar->name];
                        out << "    addi t1, sp, " << off << "\n";
                    } else {
                        emitExpr(idx->target, "t1", out, varOffsets, depth + 1);
                    }
                } else {
                    emitExpr(idx->target, "t1", out, varOffsets, depth + 1);
                }
            } else {
                emitExpr(idx->target, "t1", out, varOffsets, depth + 1);
            }
            out << "    sw t1, " << scratchTarget << "(sp)\n";

            emitExpr(idx->index, "t2", out, varOffsets, depth + 2);
            out << "    lw t1, " << scratchTarget << "(sp)\n";
            out << "    lw t0, " << scratchVal << "(sp)\n";

            if (esize == 1) {
                out << "    add t1, t1, t2\n";
                out << "    sb t0, 0(t1)\n";
            } else if (esize == 2) {
                out << "    slli t2, t2, 1\n";
                out << "    add t1, t1, t2\n";
                out << "    sh t0, 0(t1)\n";
            } else {
                out << "    slli t2, t2, 2\n";
                out << "    add t1, t1, t2\n";
                out << "    sw t0, 0(t1)\n";
            }
            if (targetReg != "t0") {
                out << "    addi " << targetReg << ", t0, 0\n";
            }
        } else if (auto u = ast_cast<UnaryExpr>(asgn->target)) {
            if (u->op == "*") {
                int scratchOffset = depth * 4;
                emitExpr(asgn->value, "t0", out, varOffsets, depth);
                out << "    sw t0, " << scratchOffset << "(sp)\n";
                emitExpr(u->operand, "t1", out, varOffsets, depth + 1);
                out << "    lw t0, " << scratchOffset << "(sp)\n";
                out << "    sw t0, 0(t1)\n";
                if (targetReg != "t0") {
                    out << "    addi " << targetReg << ", t0, 0\n";
                }
            }
        }
    } else if (auto call = ast_cast<CallExpr>(expr)) {
        if (call->callee == "Ok") {
            if (!call->args.empty()) {
                int argSlot = (depth + 1) * 4;
                if (isFloatExpr(call->args[0])) {
                    emitExpr(call->args[0], "ft0", out, varOffsets, depth + 1);
                    out << "    fsw ft0, " << argSlot << "(sp)\n";
                } else {
                    emitExpr(call->args[0], "t0", out, varOffsets, depth + 1);
                    out << "    sw t0, " << argSlot << "(sp)\n";
                }
            }
            out << "    li a0, 12\n";
            out << "    call malloc\n";
            out << "    sw zero, 0(a0)\n";
            if (!call->args.empty()) {
                int argSlot = (depth + 1) * 4;
                out << "    lw t1, " << argSlot << "(sp)\n";
                out << "    sw t1, 4(a0)\n";
            } else {
                out << "    sw zero, 4(a0)\n";
            }
            out << "    sw zero, 8(a0)\n";
            if (targetReg != "a0") {
                out << "    addi " << targetReg << ", a0, 0\n";
            }
            return;
        } else if (call->callee == "Err") {
            if (!call->args.empty()) {
                int argSlot = (depth + 1) * 4;
                emitExpr(call->args[0], "t0", out, varOffsets, depth + 1);
                out << "    sw t0, " << argSlot << "(sp)\n";
            }
            out << "    li a0, 12\n";
            out << "    call malloc\n";
            out << "    li t1, 1\n";
            out << "    sw t1, 0(a0)\n";
            out << "    sw zero, 4(a0)\n";
            if (!call->args.empty()) {
                int argSlot = (depth + 1) * 4;
                out << "    lw t1, " << argSlot << "(sp)\n";
                out << "    sw t1, 8(a0)\n";
            } else {
                out << "    sw zero, 8(a0)\n";
            }
            if (targetReg != "a0") {
                out << "    addi " << targetReg << ", a0, 0\n";
            }
            return;
        }

        std::string targetCallee = call->callee;
        bool isMethodOnThis = false;
        if (!currentClass.empty() && functionReturnTypes.count(currentClass + "_" + call->callee)) {
            targetCallee = currentClass + "_" + call->callee;
            isMethodOnThis = true;
        }

        int intArgIdx = isMethodOnThis ? 1 : 0;
        int floatArgIdx = 0;
        for (size_t i = 0; i < call->args.size() && i < 8; ++i) {
            int argSlot = (depth + 1 + i) * 4;
            if (isFloatExpr(call->args[i])) {
                emitExpr(call->args[i], "ft0", out, varOffsets, depth + 1 + i);
                out << "    fsw ft0, " << argSlot << "(sp)\n";
            } else {
                emitExpr(call->args[i], "t0", out, varOffsets, depth + 1 + i);
                out << "    sw t0, " << argSlot << "(sp)\n";
            }
        }
        if (isMethodOnThis) {
            out << "    lw a0, " << varOffsets["this"] << "(sp)\n";
        }
        for (size_t i = 0; i < call->args.size() && i < 8; ++i) {
            int argSlot = (depth + 1 + i) * 4;
            if (isFloatExpr(call->args[i])) {
                if (floatArgIdx < 8) {
                    out << "    flw fa" << floatArgIdx++ << ", " << argSlot << "(sp)\n";
                }
            } else {
                if (intArgIdx < 8) {
                    out << "    lw a" << intArgIdx++ << ", " << argSlot << "(sp)\n";
                }
            }
        }
        auto vit = varOffsets.find(targetCallee);
        if (vit != varOffsets.end()) {
            out << "    lw t2, " << vit->second << "(sp)\n";
            out << "    jalr ra, 0(t2)\n";
        } else {
            out << "    call " << targetCallee << "\n";
        }

        bool isFloatRet = false;
        auto fit = functionReturnTypes.find(targetCallee);
        if (fit != functionReturnTypes.end() && fit->second.isFloat()) isFloatRet = true;

        if (isFloatRet) {
            if (targetReg != "fa0" && targetReg[0] == 'f') {
                out << "    fsgnj.s " << targetReg << ", fa0, fa0\n";
            }
        } else {
            if (targetReg != "a0") {
                out << "    addi " << targetReg << ", a0, 0\n";
            }
        }
    } else if (auto mcall = ast_cast<MethodCallExpr>(expr)) {
        if (mcall->method == "isError" || mcall->method == "isOk" ||
            mcall->method == "value" || mcall->method == "error" ||
            mcall->method == "orElse") {
            emitExpr(mcall->object, "t0", out, varOffsets, depth);
            if (mcall->method == "isError") {
                out << "    lw " << targetReg << ", 0(t0)\n";
            } else if (mcall->method == "isOk") {
                out << "    lw t1, 0(t0)\n";
                out << "    seqz " << targetReg << ", t1\n";
            } else if (mcall->method == "value") {
                if (targetReg[0] == 'f') {
                    out << "    flw " << targetReg << ", 4(t0)\n";
                } else {
                    out << "    lw " << targetReg << ", 4(t0)\n";
                }
            } else if (mcall->method == "error") {
                out << "    lw " << targetReg << ", 8(t0)\n";
            } else if (mcall->method == "orElse") {
                int resSlot = depth * 4;
                out << "    sw t0, " << resSlot << "(sp)\n";
                std::string lblElse = newLabel("orelse_def");
                std::string lblEnd = newLabel("orelse_end");
                out << "    lw t1, 0(t0)\n";
                out << "    bne t1, zero, " << lblElse << "\n";
                if (targetReg[0] == 'f') {
                    out << "    flw " << targetReg << ", 4(t0)\n";
                } else {
                    out << "    lw " << targetReg << ", 4(t0)\n";
                }
                out << "    j " << lblEnd << "\n";
                out << lblElse << ":\n";
                if (!mcall->args.empty()) {
                    emitExpr(mcall->args[0], targetReg, out, varOffsets, depth + 1);
                } else {
                    out << "    li " << targetReg << ", 0\n";
                }
                out << lblEnd << ":\n";
            }
            return;
        }

        // Check if object is a namespace
        if (auto targetVar = ast_cast<VarExpr>(mcall->object)) {
            if (namespaces.count(targetVar->name)) {
                std::string mangledName = targetVar->name + "_" + mcall->method;
                int intArgIdx = 0;
                int floatArgIdx = 0;
                for (size_t i = 0; i < mcall->args.size() && i < 8; ++i) {
                    int argSlot = (depth + i) * 4;
                    if (isFloatExpr(mcall->args[i])) {
                        emitExpr(mcall->args[i], "ft0", out, varOffsets, depth + i);
                        out << "    fsw ft0, " << argSlot << "(sp)\n";
                    } else {
                        emitExpr(mcall->args[i], "t0", out, varOffsets, depth + i);
                        out << "    sw t0, " << argSlot << "(sp)\n";
                    }
                }
                for (size_t i = 0; i < mcall->args.size() && i < 8; ++i) {
                    int argSlot = (depth + i) * 4;
                    if (isFloatExpr(mcall->args[i])) {
                        if (floatArgIdx < 8) {
                            out << "    flw fa" << floatArgIdx++ << ", " << argSlot << "(sp)\n";
                        }
                    } else {
                        if (intArgIdx < 8) {
                            out << "    lw a" << intArgIdx++ << ", " << argSlot << "(sp)\n";
                        }
                    }
                }
                out << "    call " << mangledName << "\n";

                bool isFloatRet = false;
                auto fit = functionReturnTypes.find(mangledName);
                if (fit != functionReturnTypes.end() && fit->second.isFloat()) isFloatRet = true;

                if (isFloatRet) {
                    if (targetReg != "fa0" && targetReg[0] == 'f') {
                        out << "    fsgnj.s " << targetReg << ", fa0, fa0\n";
                    }
                } else {
                    if (targetReg != "a0") {
                        out << "    addi " << targetReg << ", a0, 0\n";
                    }
                }
                return;
            }
        }

        // 1. Determine target class name
        std::string className;
        bool isPointerObj = false;
        if (auto targetVar = ast_cast<VarExpr>(mcall->object)) {
            if (targetVar->name == "this") {
                className = currentClass;
                isPointerObj = true;
            } else {
                auto vtIt = currentVarTypes.find(targetVar->name);
                if (vtIt != currentVarTypes.end()) {
                    className = vtIt->second.name;
                    isPointerObj = vtIt->second.isPointer();
                } else if (globalVars.count(targetVar->name)) {
                    className = globalVars[targetVar->name]->type.name;
                    isPointerObj = globalVars[targetVar->name]->type.isPointer();
                }
            }
        }
        if (className.empty()) {
            className = currentClass; // fallback
        }
        className = resolveClassName(className);

        // 2. Evaluate object address and store in scratch slot (depth * 4)
        int objSlot = depth * 4;
        if (auto targetVar = ast_cast<VarExpr>(mcall->object)) {
            if (targetVar->name == "this") {
                out << "    lw t0, " << varOffsets["this"] << "(sp)\n";
            } else if (globalVars.count(targetVar->name)) {
                out << "    la t1, " << targetVar->name << "\n";
                out << "    lw t0, 0(t1)\n";
            } else if (isPointerObj) {
                out << "    lw t0, " << varOffsets[targetVar->name] << "(sp)\n";
            } else {
                // Stack object: pass address &obj
                out << "    addi t0, sp, " << varOffsets[targetVar->name] << "\n";
            }
        } else {
            emitExpr(mcall->object, "t0", out, varOffsets, depth);
        }
        out << "    sw t0, " << objSlot << "(sp)\n";

        // 3. Evaluate arguments into scratch slots (depth + 1 + i) * 4
        for (size_t i = 0; i < mcall->args.size() && i < 7; ++i) {
            int argSlot = (depth + 1 + i) * 4;
            if (isFloatExpr(mcall->args[i])) {
                emitExpr(mcall->args[i], "ft0", out, varOffsets, depth + 1 + i);
                out << "    fsw ft0, " << argSlot << "(sp)\n";
            } else {
                emitExpr(mcall->args[i], "t0", out, varOffsets, depth + 1 + i);
                out << "    sw t0, " << argSlot << "(sp)\n";
            }
        }

        // 4. Load object address into a0
        out << "    lw a0, " << objSlot << "(sp)\n";

        // 5. Load arguments into a1..a7 / fa0..fa7
        int intArgIdx = 1; // start at a1
        int floatArgIdx = 0;
        for (size_t i = 0; i < mcall->args.size() && i < 7; ++i) {
            int argSlot = (depth + 1 + i) * 4;
            if (isFloatExpr(mcall->args[i])) {
                if (floatArgIdx < 8) {
                    out << "    flw fa" << floatArgIdx++ << ", " << argSlot << "(sp)\n";
                }
            } else {
                if (intArgIdx < 8) {
                    out << "    lw a" << intArgIdx++ << ", " << argSlot << "(sp)\n";
                }
            }
        }

        // 6. Call ClassName_methodName
        std::string mangledName = className + "_" + mcall->method;
        out << "    call " << mangledName << "\n";

        // 7. Handle return value
        bool isFloatRet = false;
        auto fit = functionReturnTypes.find(mangledName);
        if (fit != functionReturnTypes.end() && fit->second.isFloat()) isFloatRet = true;

        if (isFloatRet) {
            if (targetReg != "fa0" && targetReg[0] == 'f') {
                out << "    fsgnj.s " << targetReg << ", fa0, fa0\n";
            }
        } else {
            if (targetReg != "a0") {
                out << "    addi " << targetReg << ", a0, 0\n";
            }
        }
    } else if (auto sz = ast_cast<SizeofExpr>(expr)) {
        int s = getTypeSize(sz->targetType);
        if (s <= 0) s = 4;
        if (targetReg[0] == 'f') {
            out << "    li t0, " << s << "\n";
            out << "    fcvt.s.w " << targetReg << ", t0\n";
        } else {
            out << "    li " << targetReg << ", " << s << "\n";
        }
    } else if (auto del = ast_cast<DeleteExpr>(expr)) {
        emitExpr(del->target, "a0", out, varOffsets, depth);
        out << "    call free\n";
        if (targetReg != "a0") {
            out << "    addi " << targetReg << ", zero, 0\n";
        }
    } else if (auto nexpr = ast_cast<NewExpr>(expr)) {
        // 1. Get class size
        int clsSize = 4;
        std::string clsName = resolveClassName(nexpr->className);
        auto sit = structs.find(clsName);
        if (sit != structs.end()) {
            clsSize = sit->second.totalSize;
        }
        if (clsSize <= 0) clsSize = 4;

        // 2. Evaluate constructor arguments and store in scratch slots (depth + 1 + i) * 4
        for (size_t i = 0; i < nexpr->args.size() && i < 7; ++i) {
            int argSlot = (depth + 1 + i) * 4;
            if (isFloatExpr(nexpr->args[i])) {
                emitExpr(nexpr->args[i], "ft0", out, varOffsets, depth + 1 + i);
                out << "    fsw ft0, " << argSlot << "(sp)\n";
            } else {
                emitExpr(nexpr->args[i], "t0", out, varOffsets, depth + 1 + i);
                out << "    sw t0, " << argSlot << "(sp)\n";
            }
        }

        // 3. Allocate heap memory: a0 = clsSize; call malloc
        out << "    li a0, " << clsSize << "\n";
        out << "    call malloc\n";

        // 4. Save allocated pointer in scratch slot depth * 4
        int ptrSlot = depth * 4;
        out << "    sw a0, " << ptrSlot << "(sp)\n";

        // 5. Load constructor arguments into a1..a7 / fa0..fa7
        int intArgIdx = 1; // starts at a1
        int floatArgIdx = 0;
        for (size_t i = 0; i < nexpr->args.size() && i < 7; ++i) {
            int argSlot = (depth + 1 + i) * 4;
            if (isFloatExpr(nexpr->args[i])) {
                if (floatArgIdx < 8) {
                    out << "    flw fa" << floatArgIdx++ << ", " << argSlot << "(sp)\n";
                }
            } else {
                if (intArgIdx < 8) {
                    out << "    lw a" << intArgIdx++ << ", " << argSlot << "(sp)\n";
                }
            }
        }

        // 6. Reload allocated pointer to a0
        out << "    lw a0, " << ptrSlot << "(sp)\n";

        // 7. Call constructor ClassName_ClassName
        std::string ctorName = clsName + "_" + clsName;
        out << "    call " << ctorName << "\n";

        // 8. Result is the allocated pointer (saved in ptrSlot)
        out << "    lw " << targetReg << ", " << ptrSlot << "(sp)\n";
    } else if (auto tryExp = ast_cast<TryExpr>(expr)) {
        // Evaluate the inner expression (produces a Result*) into t0
        emitExpr(tryExp->expr, "t0", out, varOffsets, depth);
        // Result layout:
        // offset 0: is_err (i32)
        // offset 4: val (i32)
        // offset 8: err (i32)
        std::string lblOk = newLabel("try_ok");
        out << "    lw t1, 0(t0)\n";
        out << "    beq t1, zero, " << lblOk << "\n";
        // It's an error: propagate Result* by loading into a0, run defers, and return immediately!
        out << "    addi a0, t0, 0\n";
        if (!currentDefers.empty()) {
            out << "    sw a0, 0(sp)\n";
            for (auto it = currentDefers.rbegin(); it != currentDefers.rend(); ++it) {
                emitStmt(*it, out, varOffsets);
            }
            out << "    lw a0, 0(sp)\n";
        }
        out << "    jal zero, .Lret_" << currentFunc << "\n";
        out << lblOk << ":\n";
        // Success: extract the unwrapped value from 4(t0) into targetReg
        if (targetReg[0] == 'f') {
            out << "    flw " << targetReg << ", 4(t0)\n";
        } else {
            out << "    lw " << targetReg << ", 4(t0)\n";
        }
    }
}

} // namespace maiascript
