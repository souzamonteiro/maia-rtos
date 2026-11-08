#include "riscv_disasm.hpp"
#include <stdio.h>
#include <assert.h>

using namespace maia::riscv;

struct TestCase {
    uint32_t word;
    uint32_t pc;
    const char *expected_mnemonic;
    const char *expected_operands;
};

int main() {
    printf("====================================================\n");
    printf("   MaiaASM RISC-V Disassembler Test Suite (RV32)   \n");
    printf("====================================================\n\n");

    Disassembler disasm;
    disasm.set_show_hex(false);

    /* Vetores do manual de referência (Seção 12) e instruções canônicas */
    TestCase cases[] = {
        /* Vetor normativo Seção 12: */
        { 0x00a00513, 0x0000, "addi", "a0, zero, 10" },
        { 0x01400593, 0x0004, "addi", "a1, zero, 20" },
        { 0x00b50533, 0x0008, "add",  "a0, a0, a1" },
        { 0x00008067, 0x000c, "ret",  "" },

        /* Pseudo-instruções canônicas: */
        { 0x00000013, 0x0010, "nop",  "" },
        { 0x00058513, 0x0014, "mv",   "a0, a1" },
        { 0x40b00533, 0x0018, "neg",  "a0, a1" },
        { 0xfff5c513, 0x001c, "not",  "a0, a1" },

        /* Carga e Armazenamento (Load/Store): */
        { 0x00c12083, 0x0020, "lw",   "ra, 12(sp)" },
        { 0x00112623, 0x0024, "sw",   "ra, 12(sp)" },
        { 0xff010113, 0x0028, "addi", "sp, sp, -16" },

        /* Saltos e Branches: */
        { 0x0000006f, 0x002c, "j",    "0x2c" },

        /* Extensão M (Multiplicação): */
        { 0x02c58533, 0x0030, "mul",  "a0, a1, a2" },
        { 0x02c5c533, 0x0034, "div",  "a0, a1, a2" },

        /* Extensão A (Atômico): */
        { 0x04b6252f, 0x0038, "amoadd.w.aq", "a0, a1, (a2)" },

        /* Extensão F (Ponto Flutuante SP): */
        { 0x00052507, 0x003c, "flw",    "fa0, 0(a0)" },
        { 0x00a52027, 0x0040, "fsw",    "fa0, 0(a0)" },
        { 0x00a575d3, 0x0044, "fadd.s", "fa1, fa0, fa0" },

        /* Sync / Fence: */
        { 0x0ff0000f, 0x0048, "fence",  "iorw, iorw" },
        { 0x00000073, 0x004c, "ecall",  "" },
        { 0x00100073, 0x0050, "ebreak", "" }
    };

    size_t total = sizeof(cases) / sizeof(cases[0]);
    size_t passed = 0;

    for (size_t i = 0; i < total; i++) {
        DisasmResult res = disasm.disassemble(cases[i].word, cases[i].pc);
        bool ok = (res.mnemonic == cases[i].expected_mnemonic) &&
                  (res.operands == cases[i].expected_operands);

        printf("[%s] Word: 0x%08x -> %-14s (Expected: %s %s)\n",
               ok ? "PASS" : "FAIL",
               cases[i].word,
               res.full_text.c_str(),
               cases[i].expected_mnemonic,
               cases[i].expected_operands);

        if (ok) passed++;
    }

    printf("\n----------------------------------------------------\n");
    printf("Result: %zu / %zu tests passed (%.1f%%)\n", passed, total, (passed * 100.0) / total);
    printf("----------------------------------------------------\n");

    return (passed == total) ? 0 : 1;
}
