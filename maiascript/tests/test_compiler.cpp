#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>

#include "ast.hpp"
#include "parser.hpp"
#include "codegen_riscv.hpp"
#include "emulator_rv32.hpp"

#include "../../assembler/include/riscv_asm.hpp"
#include "../../assembler/include/riscv_disasm.hpp"

int main() {
    std::vector<std::pair<std::string, int32_t>> testCases = {
        {"01_return", 42},
        {"02_arithmetic", 38},
        {"03_branch", 9},
        {"04_loop", 15},
        {"05_function_call", 36},
        {"06_memory", 9},
        {"07_bitwise", 53},
        {"08_recursion", 120},
        {"09_strings", 77},
        {"10_struct", 42},
        {"11_arrays", 150},
        {"12_float", 42},
        {"13_extern", 100},
        {"14_profiling", 42},
        {"15_classes", 2},
        {"16_heap_objects", 43},
        {"17_printf", 42},
        {"18_namespaces", 70},
        {"19_include_modules", 86}
    };

    std::string basePath = "/Volumes/External_SSD/Documentos/Projects/maia-rtos/maiascript/tests/pairs/";
    int passed = 0;

    std::cout << "=================================================================\n";
    std::cout << "MAIASCRIPT -> MAIAASM -> EMULATOR END-TO-END VERIFICATION SUITE\n";
    std::cout << "=================================================================\n";

    maiascript::CodegenRISCV codegen;

    for (const auto& tc : testCases) {
        const std::string& name = tc.first;
        int32_t expectedA0 = tc.second;
        std::string msPath = basePath + name + "/main.ms";

        std::cout << "TEST [" << name << "]: ";

        // 1. Parse MaiaScript source
        std::string parseErr;
        auto prog = maiascript::Parser::parseFile(msPath, parseErr);
        if (!prog) {
            std::cout << "FAIL (Parse error: " << parseErr << ")\n";
            continue;
        }

        // 2. Generate RISC-V assembly
        std::string asmCode = codegen.generateAssembly(prog);
        if (asmCode.empty()) {
            std::cout << "FAIL (Codegen produced empty assembly)\n";
            continue;
        }

        // 3. Assemble with MaiaASM
        maia::riscv::Assembler assembler;
        if (!assembler.assemble_string(asmCode)) {
            std::cout << "FAIL (MaiaASM assembly error):\n";
            for (const auto& diag : assembler.get_diagnostics()) {
                std::cout << "  " << diag.message << " (line " << diag.line << ")\n";
            }
            continue;
        }

        std::vector<uint8_t> machineCode = assembler.get_image_binary();

        // 4. Find entry point for "main"
        uint32_t mainOffset = 0;
        const auto& symbols = assembler.get_symbols();
        auto it = symbols.find("main");
        if (it != symbols.end()) {
            mainOffset = it->second.offset;
        }

        // 5. Execute in RV32 emulator
        maiascript::EmulatorRV32 emu;
        emu.loadCode(machineCode.data(), machineCode.size(), 0);

        // Register host test mock functions
        emu.registerHostFunction("add_host", [](maiascript::EmulatorRV32& e) -> int32_t {
            int32_t a = (int32_t)e.regs[10];
            int32_t b = (int32_t)e.regs[11];
            return a + b;
        });
        emu.registerHostFunction("multiply_host", [](maiascript::EmulatorRV32& e) -> int32_t {
            int32_t a = (int32_t)e.regs[10];
            int32_t b = (int32_t)e.regs[11];
            return a * b;
        });
        emu.registerHostFunction("cycles", [](maiascript::EmulatorRV32& e) -> int32_t {
            return (int32_t)e.getCycleCount();
        });
        static uint32_t s_test_heap = 0x4000;
        emu.registerHostFunction("malloc", [](maiascript::EmulatorRV32& e) -> int32_t {
            uint32_t sz = e.regs[10];
            sz = (sz + 7) & ~7;
            uint32_t a = s_test_heap;
            s_test_heap += sz;
            return (int32_t)a;
        });
        emu.registerHostFunction("free", [](maiascript::EmulatorRV32&) -> int32_t {
            return 0;
        });
        emu.registerHostFunction("printf", [](maiascript::EmulatorRV32& e) -> int32_t {
            uint32_t fmt_ptr = e.regs[10];
            int argIdx = 11;
            int fArgIdx = 0;
            int count = 0;
            while (fmt_ptr < e.memory.size() && e.memory[fmt_ptr] != 0) {
                char ch = (char)e.memory[fmt_ptr++];
                if (ch == '%' && fmt_ptr < e.memory.size()) {
                    char spec = (char)e.memory[fmt_ptr++];
                    if (spec == 'd' || spec == 'i') {
                        if (argIdx <= 17) argIdx++;
                    } else if (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'c' || spec == 's') {
                        if (argIdx <= 17) argIdx++;
                    } else if (spec == 'f') {
                        if (fArgIdx < 8) fArgIdx++;
                    }
                }
                count++;
            }
            return count;
        });

        // Patch external calls from relocations
        for (const auto& r : assembler.get_relocations()) {
            if (r.type == 19 || r.type == 18) {
                emu.patchCall(r.offset, r.symbol);
            }
        }

        int32_t actualA0 = 0;
        try {
            actualA0 = emu.run(mainOffset, 200000);
        } catch (const std::exception& ex) {
            std::cout << "FAIL (Emulator runtime exception: " << ex.what() << ")\n";
            continue;
        }

        // 6. Verify result
        if (actualA0 == expectedA0) {
            std::cout << "PASS (return a0 = " << actualA0
                      << ", " << emu.instructionCount << " instructions, "
                      << machineCode.size() << " bytes)\n";
            passed++;
        } else {
            std::cout << "FAIL (expected a0 = " << expectedA0
                      << ", got " << actualA0 << ")\n";
        }
    }

    std::cout << "=================================================================\n";
    std::cout << "RESULT: " << passed << "/" << testCases.size() << " tests passed.\n";
    std::cout << "=================================================================\n";

    return (passed == (int)testCases.size()) ? 0 : 1;
}
