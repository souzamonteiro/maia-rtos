#include "riscv_isa.hpp"
#include <string.h>
#include <strings.h>

namespace maia {
namespace riscv {

/* =========================================================================
 * 1. Nomes dos Registradores Inteiros e Flutuantes (ABI e Canônico)
 * ========================================================================= */

static const char *s_abi_regs[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static const char *s_x_regs[32] = {
    "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
    "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
    "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"
};

static const char *s_abi_fregs[32] = {
    "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6",  "ft7",
    "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
    "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6",  "fs7",
    "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

static const char *s_f_regs[32] = {
    "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
    "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
    "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
    "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
};

const char* get_reg_name(unsigned reg_idx) {
    if (reg_idx < 32) return s_abi_regs[reg_idx];
    return "unknown";
}

const char* get_freg_name(unsigned freg_idx) {
    if (freg_idx < 32) return s_abi_fregs[freg_idx];
    return "unknown";
}

int parse_reg_name(const char *name) {
    if (!name || !*name) return -1;
    if (strcasecmp(name, "fp") == 0) return 8; /* s0 alias */

    for (int i = 0; i < 32; i++) {
        if (strcasecmp(name, s_abi_regs[i]) == 0) return i;
        if (strcasecmp(name, s_x_regs[i]) == 0) return i;
    }
    return -1;
}

int parse_freg_name(const char *name) {
    if (!name || !*name) return -1;
    for (int i = 0; i < 32; i++) {
        if (strcasecmp(name, s_abi_fregs[i]) == 0) return i;
        if (strcasecmp(name, s_f_regs[i]) == 0) return i;
    }
    return -1;
}

/* =========================================================================
 * 2. Catálogo de Instruções com Máscaras e Matches
 * ========================================================================= */

#define OP_LUI      0x37
#define OP_AUIPC    0x17
#define OP_JAL      0x6f
#define OP_JALR     0x67
#define OP_BRANCH   0x63
#define OP_LOAD     0x03
#define OP_STORE    0x23
#define OP_IMM      0x13
#define OP_REG      0x33
#define OP_FENCE    0x0f
#define OP_SYSTEM   0x73
#define OP_AMO      0x2f
#define OP_LOAD_FP  0x07
#define OP_STORE_FP 0x27
#define OP_OP_FP    0x53
#define OP_MADD     0x43
#define OP_MSUB     0x47
#define OP_NMSUB    0x4b
#define OP_NMADD    0x4f

const InstructionDesc g_instruction_table[] = {
    /* ---------------- RV32I ---------------- */
    { "lui",      0x00000037, 0x0000007f, FMT_U,       EXT_I },
    { "auipc",    0x00000017, 0x0000007f, FMT_U,       EXT_I },
    { "jal",      0x0000006f, 0x0000007f, FMT_J,       EXT_I },
    { "jalr",     0x00000067, 0x0000707f, FMT_I_JALR,  EXT_I },

    /* Branch */
    { "beq",      0x00000063, 0x0000707f, FMT_B,       EXT_I },
    { "bne",      0x00001063, 0x0000707f, FMT_B,       EXT_I },
    { "blt",      0x00004063, 0x0000707f, FMT_B,       EXT_I },
    { "bge",      0x00005063, 0x0000707f, FMT_B,       EXT_I },
    { "bltu",     0x00006063, 0x0000707f, FMT_B,       EXT_I },
    { "bgeu",     0x00007063, 0x0000707f, FMT_B,       EXT_I },

    /* Load */
    { "lb",       0x00000003, 0x0000707f, FMT_I_LOAD,  EXT_I },
    { "lh",       0x00001003, 0x0000707f, FMT_I_LOAD,  EXT_I },
    { "lw",       0x00002003, 0x0000707f, FMT_I_LOAD,  EXT_I },
    { "lbu",      0x00004003, 0x0000707f, FMT_I_LOAD,  EXT_I },
    { "lhu",      0x00005003, 0x0000707f, FMT_I_LOAD,  EXT_I },

    /* Store */
    { "sb",       0x00000023, 0x0000707f, FMT_S,       EXT_I },
    { "sh",       0x00001023, 0x0000707f, FMT_S,       EXT_I },
    { "sw",       0x00002023, 0x0000707f, FMT_S,       EXT_I },

    /* ALU Immediate */
    { "addi",     0x00000013, 0x0000707f, FMT_I,       EXT_I },
    { "slti",     0x00002013, 0x0000707f, FMT_I,       EXT_I },
    { "sltiu",    0x00003013, 0x0000707f, FMT_I,       EXT_I },
    { "xori",     0x00004013, 0x0000707f, FMT_I,       EXT_I },
    { "ori",      0x00006013, 0x0000707f, FMT_I,       EXT_I },
    { "andi",     0x00007013, 0x0000707f, FMT_I,       EXT_I },
    { "slli",     0x00001013, 0xfe00707f, FMT_I_SHAMT, EXT_I },
    { "srli",     0x00005013, 0xfe00707f, FMT_I_SHAMT, EXT_I },
    { "srai",     0x40005013, 0xfe00707f, FMT_I_SHAMT, EXT_I },

    /* ALU Register */
    { "add",      0x00000033, 0xfe00707f, FMT_R,       EXT_I },
    { "sub",      0x40000033, 0xfe00707f, FMT_R,       EXT_I },
    { "sll",      0x00001033, 0xfe00707f, FMT_R,       EXT_I },
    { "slt",      0x00002033, 0xfe00707f, FMT_R,       EXT_I },
    { "sltu",     0x00003033, 0xfe00707f, FMT_R,       EXT_I },
    { "xor",      0x00004033, 0xfe00707f, FMT_R,       EXT_I },
    { "srl",      0x00005033, 0xfe00707f, FMT_R,       EXT_I },
    { "sra",      0x40005033, 0xfe00707f, FMT_R,       EXT_I },
    { "or",       0x00006033, 0xfe00707f, FMT_R,       EXT_I },
    { "and",      0x00007033, 0xfe00707f, FMT_R,       EXT_I },

    /* System / Sync */
    { "fence",    0x0000000f, 0x0000707f, FMT_I,       EXT_I },
    { "ecall",    0x00000073, 0xffffffff, FMT_SYSTEM,  EXT_I },
    { "ebreak",   0x00100073, 0xffffffff, FMT_SYSTEM,  EXT_I },

    /* ---------------- RV32M Extension ---------------- */
    { "mul",      0x02000033, 0xfe00707f, FMT_R,       EXT_M },
    { "mulh",     0x02001033, 0xfe00707f, FMT_R,       EXT_M },
    { "mulhsu",   0x02002033, 0xfe00707f, FMT_R,       EXT_M },
    { "mulhu",    0x02003033, 0xfe00707f, FMT_R,       EXT_M },
    { "div",      0x02004033, 0xfe00707f, FMT_R,       EXT_M },
    { "divu",     0x02005033, 0xfe00707f, FMT_R,       EXT_M },
    { "rem",      0x02006033, 0xfe00707f, FMT_R,       EXT_M },
    { "remu",     0x02007033, 0xfe00707f, FMT_R,       EXT_M },

    /* ---------------- RV32A Extension (Atomic) ---------------- */
    { "lr.w",     0x1000202f, 0xf9f0707f, FMT_ATOMIC,  EXT_A },
    { "sc.w",     0x1800202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amoswap.w",0x0800202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amoadd.w", 0x0000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amoxor.w", 0x2000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amoand.w", 0x6000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amoor.w",  0x4000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amomin.w", 0x8000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amomax.w", 0xa000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amominu.w",0xc000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },
    { "amomaxu.w",0xe000202f, 0xf800707f, FMT_ATOMIC,  EXT_A },

    /* ---------------- Zicsr / Zifencei ---------------- */
    { "csrrw",    0x00001073, 0x0000707f, FMT_CSR,     EXT_ZICSR },
    { "csrrs",    0x00002073, 0x0000707f, FMT_CSR,     EXT_ZICSR },
    { "csrrc",    0x00003073, 0x0000707f, FMT_CSR,     EXT_ZICSR },
    { "csrrwi",   0x00005073, 0x0000707f, FMT_CSRI,    EXT_ZICSR },
    { "csrrsi",   0x00006073, 0x0000707f, FMT_CSRI,    EXT_ZICSR },
    { "csrrci",   0x00007073, 0x0000707f, FMT_CSRI,    EXT_ZICSR },
    { "fence.i",  0x0000100f, 0xffffffff, FMT_I,       EXT_ZIFENCEI },

    /* ---------------- RV32F Extension (Single Precision Float) ---------------- */
    { "flw",      0x00002007, 0x0000707f, FMT_F_LOAD,    EXT_F },
    { "fsw",      0x00002027, 0x0000707f, FMT_F_STORE,   EXT_F },
    { "fadd.s",   0x00000053, 0xfe00007f, FMT_F_R,       EXT_F },
    { "fsub.s",   0x08000053, 0xfe00007f, FMT_F_R,       EXT_F },
    { "fmul.s",   0x10000053, 0xfe00007f, FMT_F_R,       EXT_F },
    { "fdiv.s",   0x18000053, 0xfe00007f, FMT_F_R,       EXT_F },
    { "fsqrt.s",  0x58000053, 0xfff0007f, FMT_F_R,       EXT_F },
    { "fsgnj.s",  0x20000053, 0xfe00707f, FMT_F_R,       EXT_F },
    { "fsgnjn.s", 0x20001053, 0xfe00707f, FMT_F_R,       EXT_F },
    { "fsgnjx.s", 0x20002053, 0xfe00707f, FMT_F_R,       EXT_F },
    { "fmin.s",   0x28000053, 0xfe00707f, FMT_F_R,       EXT_F },
    { "fmax.s",   0x28001053, 0xfe00707f, FMT_F_R,       EXT_F },
    { "fcvt.w.s", 0xc0000053, 0xfff0007f, FMT_F_CVT_F2I, EXT_F },
    { "fcvt.wu.s",0xc0100053, 0xfff0007f, FMT_F_CVT_F2I, EXT_F },
    { "fmv.x.w",  0xe0000053, 0xfff0707f, FMT_F_MV_F2I,  EXT_F },
    { "feq.s",    0xa0002053, 0xfe00707f, FMT_F_CMP,     EXT_F },
    { "flt.s",    0xa0001053, 0xfe00707f, FMT_F_CMP,     EXT_F },
    { "fle.s",    0xa0000053, 0xfe00707f, FMT_F_CMP,     EXT_F },
    { "fclass.s", 0xe0001053, 0xfff0707f, FMT_F_CLASS,   EXT_F },
    { "fcvt.s.w", 0xd0000053, 0xfff0007f, FMT_F_CVT_I2F, EXT_F },
    { "fcvt.s.wu",0xd0100053, 0xfff0007f, FMT_F_CVT_I2F, EXT_F },
    { "fmv.w.x",  0xf0000053, 0xfff0707f, FMT_F_MV_I2F,  EXT_F }
};

const size_t g_instruction_table_count = sizeof(g_instruction_table) / sizeof(g_instruction_table[0]);

const InstructionDesc* find_instruction_by_mnemonic(const char *mnemonic) {
    if (!mnemonic || !*mnemonic) return NULL;
    for (size_t i = 0; i < g_instruction_table_count; i++) {
        if (strcasecmp(mnemonic, g_instruction_table[i].mnemonic) == 0) {
            return &g_instruction_table[i];
        }
    }
    return NULL;
}

const InstructionDesc* decode_instruction(uint32_t word) {
    for (size_t i = 0; i < g_instruction_table_count; i++) {
        const InstructionDesc *desc = &g_instruction_table[i];
        if ((word & desc->mask) == desc->match) {
            return desc;
        }
    }
    return NULL;
}

} // namespace riscv
} // namespace maia
