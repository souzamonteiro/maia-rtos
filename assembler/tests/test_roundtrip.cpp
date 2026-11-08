#include "riscv_asm.hpp"
#include "riscv_disasm.hpp"
#include <stdio.h>
#include <fstream>
#include <sstream>

using namespace maia::riscv;

static std::string read_file(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char **argv) {
    printf("====================================================\n");
    printf("   MaiaASM RISC-V Assembler & Round-Trip Tests     \n");
    printf("====================================================\n\n");

    std::vector<std::string> test_files;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) test_files.push_back(argv[i]);
    } else {
        test_files.push_back("tests/positive/01_basic.s");
        test_files.push_back("tests/positive/02_labels.s");
        test_files.push_back("tests/positive/03_data.s");
        test_files.push_back("tests/positive/04_reloc.s");
        test_files.push_back("tests/positive/05_expression.s");
        test_files.push_back("tests/positive/06_fpu_atomic.s");
    }

    size_t passed = 0;
    Disassembler disasm;
    disasm.set_show_hex(true);

    for (size_t i = 0; i < test_files.size(); i++) {
        const std::string &filepath = test_files[i];
        std::string src = read_file(filepath);
        if (src.empty()) {
            printf("[FAIL] Could not open %s\n", filepath.c_str());
            continue;
        }

        Assembler as;
        bool ok = as.assemble_string(src);

        if (!ok || as.has_errors()) {
            printf("[FAIL] Assembly error in %s:\n", filepath.c_str());
            const std::vector<AsmDiagnostic> &diags = as.get_diagnostics();
            for (size_t d = 0; d < diags.size(); d++) {
                printf("  Line %d: %s\n", diags[d].line, diags[d].message.c_str());
            }
            continue;
        }

        printf("----------------------------------------------------\n");
        printf("[PASS] Successfully assembled: %s\n", filepath.c_str());

        /* Disassembly da seção .text para conferência */
        const Section *text_sec = as.get_section(".text");
        if (text_sec && !text_sec->data.empty()) {
            printf("--- Disassembly of .text (%zu bytes) ---\n", text_sec->data.size());
            for (size_t offset = 0; offset + 4 <= text_sec->data.size(); offset += 4) {
                uint32_t w = (uint32_t)text_sec->data[offset] |
                             ((uint32_t)text_sec->data[offset + 1] << 8) |
                             ((uint32_t)text_sec->data[offset + 2] << 16) |
                             ((uint32_t)text_sec->data[offset + 3] << 24);
                DisasmResult dres = disasm.disassemble(w, (uint32_t)offset);
                printf("%s\n", dres.full_text.c_str());
            }
        }

        /* Verificação de seções de dados (.data / .rodata) */
        const Section *data_sec = as.get_section(".data");
        if (data_sec && !data_sec->data.empty()) {
            printf("--- Section .data (%zu bytes) ---\n", data_sec->data.size());
        }
        const Section *rodata_sec = as.get_section(".rodata");
        if (rodata_sec && !rodata_sec->data.empty()) {
            printf("--- Section .rodata (%zu bytes) ---\n", rodata_sec->data.size());
        }

        passed++;
    }

    printf("\n====================================================\n");
    printf("Result: %zu / %zu test files assembled successfully (%.1f%%)\n",
           passed, test_files.size(), (passed * 100.0) / test_files.size());
    printf("====================================================\n");

    return (passed == test_files.size()) ? 0 : 1;
}

