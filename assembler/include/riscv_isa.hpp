#ifndef MAIA_RISCV_ISA_HPP
#define MAIA_RISCV_ISA_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

namespace maia {
namespace riscv {

/* =========================================================================
 * 1. Formatos e Encodings de Instrução (RV32 - 32 bits Little Endian)
 * ========================================================================= */

enum InstructionFormat {
    FMT_UNKNOWN = 0,
    FMT_R,          /* rd, rs1, rs2, funct7, funct3, opcode */
    FMT_I,          /* rd, rs1, imm12, funct3, opcode */
    FMT_I_SHAMT,    /* rd, rs1, shamt[4:0], funct7, funct3, opcode */
    FMT_I_LOAD,     /* rd, imm12(rs1), funct3, opcode */
    FMT_I_JALR,     /* rd, imm12(rs1), funct3, opcode */
    FMT_S,          /* rs2, imm12(rs1), funct3, opcode */
    FMT_B,          /* rs1, rs2, imm13_b, funct3, opcode */
    FMT_U,          /* rd, imm20_u, opcode */
    FMT_J,          /* rd, imm21_j, opcode */
    FMT_SYSTEM,     /* ecall, ebreak */
    FMT_CSR,        /* rd, csr12, rs1, funct3, opcode */
    FMT_CSRI,       /* rd, csr12, uimm5, funct3, opcode */
    FMT_F_LOAD,     /* fd, imm12(rs1) */
    FMT_F_STORE,    /* fs2, imm12(rs1) */
    FMT_F_R,        /* fd, fs1, fs2, rm, funct7 */
    FMT_F_R4,       /* fd, fs1, fs2, fs3, rm, funct2 */
    FMT_F_CVT_F2I,  /* rd, fs1, rm */
    FMT_F_CVT_I2F,  /* fd, rs1, rm */
    FMT_F_MV_F2I,   /* rd, fs1 */
    FMT_F_MV_I2F,   /* fd, rs1 */
    FMT_F_CMP,      /* rd, fs1, fs2 */
    FMT_F_CLASS,    /* rd, fs1 */
    FMT_ATOMIC      /* rd, rs2, (rs1), funct5, aq, rl */
};

/* =========================================================================
 * 2. Construtores de Palavra de Instrução (32-bit Words)
 * ========================================================================= */

static inline uint32_t encode_R(unsigned funct7, unsigned rs2, unsigned rs1, unsigned funct3, unsigned rd, unsigned opcode) {
    return ((funct7 & 0x7f) << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) |
           ((funct3 & 0x7) << 12)  | ((rd & 0x1f) << 7)   | (opcode & 0x7f);
}

static inline uint32_t encode_I(int32_t imm, unsigned rs1, unsigned funct3, unsigned rd, unsigned opcode) {
    return ((uint32_t(imm) & 0xfff) << 20) | ((rs1 & 0x1f) << 15) |
           ((funct3 & 0x7) << 12)          | ((rd & 0x1f) << 7)   | (opcode & 0x7f);
}

static inline uint32_t encode_S(int32_t imm, unsigned rs2, unsigned rs1, unsigned funct3, unsigned opcode) {
    uint32_t u = uint32_t(imm) & 0xfff;
    return (((u >> 5) & 0x7f) << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) |
           ((funct3 & 0x7) << 12)    | ((u & 0x1f) << 7)    | (opcode & 0x7f);
}

static inline uint32_t encode_B(int32_t imm, unsigned rs2, unsigned rs1, unsigned funct3, unsigned opcode) {
    uint32_t u = uint32_t(imm) & 0x1fff;
    return (((u >> 12) & 0x1) << 31) | (((u >> 5) & 0x3f) << 25) |
           ((rs2 & 0x1f) << 20)      | ((rs1 & 0x1f) << 15)       |
           ((funct3 & 0x7) << 12)    | (((u >> 1) & 0xf) << 8)    |
           (((u >> 11) & 0x1) << 7)  | (opcode & 0x7f);
}

static inline uint32_t encode_U(uint32_t imm20, unsigned rd, unsigned opcode) {
    return ((imm20 & 0xfffff) << 12) | ((rd & 0x1f) << 7) | (opcode & 0x7f);
}

static inline uint32_t encode_J(int32_t imm, unsigned rd, unsigned opcode) {
    uint32_t u = uint32_t(imm) & 0x1fffff;
    return (((u >> 20) & 0x1) << 31)   | (((u >> 1) & 0x3ff) << 21) |
           (((u >> 11) & 0x1) << 20)   | (((u >> 12) & 0xff) << 12) |
           ((rd & 0x1f) << 7)          | (opcode & 0x7f);
}

/* =========================================================================
 * 3. Extratores de Bits para Decodificação / Disassembly
 * ========================================================================= */

static inline unsigned get_opcode(uint32_t inst) { return inst & 0x7f; }
static inline unsigned get_rd(uint32_t inst)     { return (inst >> 7) & 0x1f; }
static inline unsigned get_funct3(uint32_t inst) { return (inst >> 12) & 0x7; }
static inline unsigned get_rs1(uint32_t inst)    { return (inst >> 15) & 0x1f; }
static inline unsigned get_rs2(uint32_t inst)    { return (inst >> 20) & 0x1f; }
static inline unsigned get_funct7(uint32_t inst) { return (inst >> 25) & 0x7f; }
static inline unsigned get_rs3(uint32_t inst)    { return (inst >> 27) & 0x1f; }

static inline int32_t get_imm_I(uint32_t inst) {
    return (int32_t)inst >> 20; /* Sign extended */
}

static inline int32_t get_imm_S(uint32_t inst) {
    uint32_t val = ((inst >> 25) << 5) | ((inst >> 7) & 0x1f);
    if (val & 0x800) val |= 0xfffff000;
    return (int32_t)val;
}

static inline int32_t get_imm_B(uint32_t inst) {
    uint32_t b12 = (inst >> 31) & 0x1;
    uint32_t b11 = (inst >> 7)  & 0x1;
    uint32_t b10_5 = (inst >> 25) & 0x3f;
    uint32_t b4_1  = (inst >> 8)  & 0xf;
    uint32_t val = (b12 << 12) | (b11 << 11) | (b10_5 << 5) | (b4_1 << 1);
    if (b12) val |= 0xffffe000;
    return (int32_t)val;
}

static inline uint32_t get_imm_U(uint32_t inst) {
    return (inst >> 12) & 0xfffff;
}

static inline int32_t get_imm_J(uint32_t inst) {
    uint32_t b20 = (inst >> 31) & 0x1;
    uint32_t b19_12 = (inst >> 12) & 0xff;
    uint32_t b11    = (inst >> 20) & 0x1;
    uint32_t b10_1  = (inst >> 21) & 0x3ff;
    uint32_t val = (b20 << 20) | (b19_12 << 12) | (b11 << 11) | (b10_1 << 1);
    if (b20) val |= 0xfff00000;
    return (int32_t)val;
}

static inline unsigned get_csr(uint32_t inst) {
    return (inst >> 20) & 0xfff;
}

/* =========================================================================
 * 4. Tabela de Nomes e Aliases de Registradores ABI
 * ========================================================================= */

const char* get_reg_name(unsigned reg_idx);
const char* get_freg_name(unsigned freg_idx);
int parse_reg_name(const char *name);
int parse_freg_name(const char *name);

/* =========================================================================
 * 5. Tabela de Descritores de Instrução
 * ========================================================================= */

enum IsaExtension {
    EXT_I      = (1 << 0),
    EXT_M      = (1 << 1),
    EXT_A      = (1 << 2),
    EXT_F      = (1 << 3),
    EXT_ZICSR  = (1 << 4),
    EXT_ZIFENCEI = (1 << 5)
};

struct InstructionDesc {
    const char *mnemonic;
    uint32_t match;
    uint32_t mask;
    InstructionFormat format;
    uint32_t extension;
};

extern const InstructionDesc g_instruction_table[];
extern const size_t g_instruction_table_count;

const InstructionDesc* find_instruction_by_mnemonic(const char *mnemonic);
const InstructionDesc* decode_instruction(uint32_t word);

} // namespace riscv
} // namespace maia

#endif // MAIA_RISCV_ISA_HPP

