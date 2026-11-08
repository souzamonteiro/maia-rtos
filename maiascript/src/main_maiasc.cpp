#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ast.hpp"
#include "parser.hpp"
#include "codegen_riscv.hpp"
#include "emulator_rv32.hpp"
#include "../../assembler/include/riscv_asm.hpp"
#include "../../assembler/include/riscv_disasm.hpp"

static void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " <input.ms> [options]\n\n"
              << "Options:\n"
              << "  -o <file>      Output file (.s for assembly, .bin for binary, .mobj for ELF)\n"
              << "  -S             Emit RISC-V assembly to stdout or -o file (default)\n"
              << "  -c             Compile and assemble to machine code binary\n"
              << "  --run          Compile, assemble, and execute in RV32 emulator\n"
              << "  --disasm       Disassemble compiled instructions to stdout\n"
              << "  --profile      Show detailed instruction and cycle metrics\n"
              << "  -v, --verbose  Verbose diagnostics\n"
              << "  -h, --help     Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;
    bool emitAsm = false;
    bool emitBin = false;
    bool runEmu = false;
    bool showDisasm = false;
    bool showProfile = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "-S") {
            emitAsm = true;
        } else if (arg == "-c") {
            emitBin = true;
        } else if (arg == "--run") {
            runEmu = true;
        } else if (arg == "--disasm") {
            showDisasm = true;
        } else if (arg == "--profile") {
            showProfile = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg[0] != '-') {
            inputPath = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "Error: No input file specified.\n";
        return 1;
    }

    // 1. Parse MaiaScript
    std::string parseErr;
    auto prog = maiascript::Parser::parseFile(inputPath, parseErr);
    if (!prog) {
        std::cerr << "Error parsing " << inputPath << ":\n  " << parseErr << "\n";
        return 1;
    }

    if (verbose) {
        std::cout << "Parsed " << prog->getFunctions().size() << " function(s) successfully.\n";
    }

    // 2. Generate RISC-V assembly
    maiascript::CodegenRISCV codegen;
    std::string asmCode = codegen.generateAssembly(prog);

    if (outputPath.empty() && !runEmu && !emitBin && !showDisasm) {
        std::cout << asmCode;
        return 0;
    }

    // Determine output mode from extension if not explicit
    bool emitElf = false;
    if (!outputPath.empty()) {
        if (outputPath.size() >= 5 && outputPath.substr(outputPath.size() - 5) == ".mobj") {
            emitElf = true;
            emitBin = true;
        } else if (outputPath.size() >= 2 && outputPath.substr(outputPath.size() - 2) == ".o") {
            emitElf = true;
            emitBin = true;
        } else if (outputPath.size() >= 4 && outputPath.substr(outputPath.size() - 4) == ".bin") {
            emitBin = true;
        } else if (outputPath.size() >= 2 && (outputPath.substr(outputPath.size() - 2) == ".s" || outputPath.substr(outputPath.size() - 2) == ".S")) {
            emitAsm = true;
        }
    }

    if (emitAsm && !emitBin && !showDisasm && !runEmu) {
        std::ofstream out(outputPath.c_str());
        if (!out.is_open()) {
            std::cerr << "Error: cannot open output file: " << outputPath << "\n";
            return 1;
        }
        out << asmCode;
        if (verbose) std::cout << "Wrote assembly to " << outputPath << "\n";
        return 0;
    }

    // 3. Assemble with MaiaASM
    maia::riscv::Assembler assembler;
    if (!assembler.assemble_string(asmCode)) {
        std::cerr << "Assembly failed:\n";
        for (const auto& diag : assembler.get_diagnostics()) {
            std::cerr << "  Line " << diag.line << ": " << diag.message << "\n";
        }
        return 1;
    }

    std::vector<uint8_t> textBin = assembler.get_text_binary();
    std::vector<uint8_t> binary = emitElf ? assembler.emit_elf32() : textBin;

    if (emitBin && !outputPath.empty()) {
        std::ofstream out(outputPath.c_str(), std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Error: cannot open output file: " << outputPath << "\n";
            return 1;
        }
        out.write((const char*)binary.data(), binary.size());
        if (verbose) {
            std::cout << "Wrote " << binary.size() << " bytes to " << outputPath
                      << " (" << (emitElf ? "ELF32 ET_REL .mobj" : "binary") << ")\n";
        }
    }

    // 4. Disassemble if requested
    if (showDisasm) {
        std::cout << "\n--- RISC-V DISASSEMBLY (" << textBin.size() << " bytes, " 
                  << textBin.size() / 4 << " instructions) ---\n";
        maia::riscv::Disassembler disasm;
        for (size_t offset = 0; offset + 4 <= textBin.size(); offset += 4) {
            uint32_t word = (uint32_t)textBin[offset] |
                            ((uint32_t)textBin[offset + 1] << 8) |
                            ((uint32_t)textBin[offset + 2] << 16) |
                            ((uint32_t)textBin[offset + 3] << 24);
            auto res = disasm.disassemble(word, (uint32_t)offset);
            std::cout << res.full_text << "\n";
        }
        std::cout << "--------------------------------------------------------\n\n";
    }

    // 5. Run emulator if requested
    if (runEmu) {
        uint32_t entry = 0;
        const auto& syms = assembler.get_symbols();
        auto it = syms.find("main");
        if (it != syms.end()) {
            entry = it->second.offset;
        }

        maiascript::EmulatorRV32 emu;
        std::vector<uint8_t> imgBin = assembler.get_image_binary();
        emu.loadCode(imgBin.data(), imgBin.size(), 0);

        // Register standard host runtime functions
        emu.registerHostFunction("cycles", [](maiascript::EmulatorRV32& e) -> int32_t {
            return (int32_t)e.getCycleCount();
        });
        emu.registerHostFunction("rdcycle", [](maiascript::EmulatorRV32& e) -> int32_t {
            return (int32_t)e.getCycleCount();
        });
        emu.registerHostFunction("millis", [](maiascript::EmulatorRV32&) -> int32_t {
            return 0;
        });
        emu.registerHostFunction("print", [](maiascript::EmulatorRV32& e) -> int32_t {
            uint32_t ptr = e.regs[10];
            while (ptr < e.memory.size() && e.memory[ptr] != 0) {
                std::cout << (char)e.memory[ptr++];
            }
            std::cout << std::flush;
            return 0;
        });
        emu.registerHostFunction("println", [](maiascript::EmulatorRV32& e) -> int32_t {
            uint32_t ptr = e.regs[10];
            while (ptr < e.memory.size() && e.memory[ptr] != 0) {
                std::cout << (char)e.memory[ptr++];
            }
            std::cout << "\n";
            return 0;
        });
        emu.registerHostFunction("print_int", [](maiascript::EmulatorRV32& e) -> int32_t {
            std::cout << (int32_t)e.regs[10] << "\n";
            return 0;
        });

        // Patch external calls from relocations
        for (const auto& r : assembler.get_relocations()) {
            if (r.type == 19 || r.type == 18) { // R_RISCV_CALL / R_RISCV_CALL_PLT
                emu.patchCall(r.offset, r.symbol);
            }
        }

        int32_t ret = 0;
        try {
            ret = emu.run(entry, 2000000);
        } catch (const std::exception& ex) {
            std::cerr << "Emulator runtime error: " << ex.what() << "\n";
            return 1;
        }

        if (showProfile || verbose) {
            std::cout << "Program exited with a0 = " << ret
                      << " (" << emu.instructionCount << " instructions executed)\n";
        } else {
            std::cout << ret << "\n";
        }
        return ret;
    }

    return 0;
}

