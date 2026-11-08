#include "riscv_asm.hpp"
#include "riscv_disasm.hpp"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>

using namespace maia::riscv;

static void print_usage(const char *prog) {
    printf("MaiaASM RISC-V Assembler & Disassembler (RV32IMAF)\n");
    printf("Usage:\n");
    printf("  %s <input.s> [-o output.bin]\n", prog);
    printf("  %s <input.s> -o output.mobj (emits ELF32 ET_REL module)\n", prog);
    printf("  %s -d <input.bin|input.mobj>\n", prog);
    printf("  %s --disasm <input.bin>\n", prog);
}

static std::string read_file_string(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input_file;
    std::string output_file = "a.out";
    bool disasm_mode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disasm") == 0) {
            disasm_mode = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        }
    }

    if (input_file.empty()) {
        fprintf(stderr, "Error: No input file specified.\n");
        return 1;
    }

    if (disasm_mode) {
        FILE *f = fopen(input_file.c_str(), "rb");
        if (!f) {
            fprintf(stderr, "Error: Cannot open %s for reading.\n", input_file.c_str());
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::vector<uint8_t> bytes(sz);
        if (fread(&bytes[0], 1, sz, f) != (size_t)sz) {
            fclose(f);
            fprintf(stderr, "Error reading binary file.\n");
            return 1;
        }
        fclose(f);

        size_t code_start = 0;
        size_t code_size = bytes.size();

        // Check if ELF32
        if (bytes.size() >= 52 && bytes[0] == 0x7f && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') {
            uint32_t shoff = (uint32_t)bytes[32] | ((uint32_t)bytes[33] << 8) | ((uint32_t)bytes[34] << 16) | ((uint32_t)bytes[35] << 24);
            uint16_t shentsize = (uint16_t)bytes[46] | ((uint16_t)bytes[47] << 8);
            uint16_t shnum = (uint16_t)bytes[48] | ((uint16_t)bytes[49] << 8);
            uint16_t shstrndx = (uint16_t)bytes[50] | ((uint16_t)bytes[51] << 8);

            if (shstrndx < shnum && shoff + shstrndx * shentsize + 40 <= bytes.size()) {
                size_t strhdr_off = shoff + shstrndx * shentsize;
                uint32_t str_off = (uint32_t)bytes[strhdr_off + 16] | ((uint32_t)bytes[strhdr_off + 17] << 8) | ((uint32_t)bytes[strhdr_off + 18] << 16) | ((uint32_t)bytes[strhdr_off + 19] << 24);
                uint32_t str_sz = (uint32_t)bytes[strhdr_off + 20] | ((uint32_t)bytes[strhdr_off + 21] << 8) | ((uint32_t)bytes[strhdr_off + 22] << 16) | ((uint32_t)bytes[strhdr_off + 23] << 24);

                for (uint16_t i = 0; i < shnum; ++i) {
                    size_t h = shoff + i * shentsize;
                    uint32_t name_off = (uint32_t)bytes[h] | ((uint32_t)bytes[h + 1] << 8) | ((uint32_t)bytes[h + 2] << 16) | ((uint32_t)bytes[h + 3] << 24);
                    if (str_off + name_off < bytes.size()) {
                        const char *sname = (const char *)&bytes[str_off + name_off];
                        if (strcmp(sname, ".text") == 0) {
                            code_start = (uint32_t)bytes[h + 16] | ((uint32_t)bytes[h + 17] << 8) | ((uint32_t)bytes[h + 18] << 16) | ((uint32_t)bytes[h + 19] << 24);
                            code_size = (uint32_t)bytes[h + 20] | ((uint32_t)bytes[h + 21] << 8) | ((uint32_t)bytes[h + 22] << 16) | ((uint32_t)bytes[h + 23] << 24);
                            printf("; Found ELF .text section at file offset 0x%zx (%zu bytes)\n", code_start, code_size);
                            break;
                        }
                    }
                }
            }
        }

        Disassembler dis;
        dis.set_show_hex(true);
        printf("; Disassembly of %s (%zu bytes)\n", input_file.c_str(), code_size);
        for (size_t off = 0; off + 4 <= code_size; off += 4) {
            size_t file_pos = code_start + off;
            if (file_pos + 4 > bytes.size()) break;
            uint32_t w = (uint32_t)bytes[file_pos] |
                         ((uint32_t)bytes[file_pos + 1] << 8) |
                         ((uint32_t)bytes[file_pos + 2] << 16) |
                         ((uint32_t)bytes[file_pos + 3] << 24);
            DisasmResult res = dis.disassemble(w, (uint32_t)off);
            printf("%s\n", res.full_text.c_str());
        }
        return 0;
    }

    /* Modo Assembler */
    std::string src = read_file_string(input_file);
    if (src.empty()) {
        fprintf(stderr, "Error: Cannot read assembly source file %s.\n", input_file.c_str());
        return 1;
    }

    Assembler as;
    if (!as.assemble_string(src)) {
        fprintf(stderr, "Assembly failed with errors:\n");
        const std::vector<AsmDiagnostic> &diags = as.get_diagnostics();
        for (size_t i = 0; i < diags.size(); i++) {
            fprintf(stderr, "%s:%d: %s: %s\n",
                    input_file.c_str(),
                    diags[i].line,
                    diags[i].is_error ? "error" : "warning",
                    diags[i].message.c_str());
        }
        return 1;
    }

    bool is_elf = false;
    if (output_file.size() >= 5 && output_file.substr(output_file.size() - 5) == ".mobj") is_elf = true;
    else if (output_file.size() >= 2 && output_file.substr(output_file.size() - 2) == ".o") is_elf = true;

    std::vector<uint8_t> out_data = is_elf ? as.emit_elf32() : as.get_text_binary();
    FILE *fout = fopen(output_file.c_str(), "wb");
    if (!fout) {
        fprintf(stderr, "Error: Cannot open output file %s for writing.\n", output_file.c_str());
        return 1;
    }
    if (!out_data.empty()) {
        fwrite(&out_data[0], 1, out_data.size(), fout);
    }
    fclose(fout);

    printf("Successfully assembled %s -> %s (%zu bytes, format=%s)\n",
           input_file.c_str(), output_file.c_str(), out_data.size(), is_elf ? "ELF32 ET_REL" : "raw text binary");

    return 0;
}

