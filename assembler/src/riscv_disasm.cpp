#include "riscv_disasm.hpp"
#include <stdio.h>
#include <string.h>

namespace maia {
namespace riscv {

DisasmResult Disassembler::disassemble(uint32_t word, uint32_t pc) const {
    DisasmResult res;
    res.success = false;
    res.raw_word = word;
    res.desc = decode_instruction(word);

    char buf[128] = {0};
    char op_buf[96] = {0};

    /* 1. Checa Pseudo-instruções canônicas de alta prioridade */

    /* nop: addi x0, x0, 0 (0x00000013) */
    if (word == 0x00000013) {
        res.success = true;
        res.mnemonic = "nop";
        res.operands = "";
        if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   nop", (unsigned int)pc, (unsigned int)word);
        else snprintf(buf, sizeof(buf), "nop");
        res.full_text = buf;
        return res;
    }

    /* ret: jalr x0, 0(ra) (0x00008067) */
    if (word == 0x00008067) {
        res.success = true;
        res.mnemonic = "ret";
        res.operands = "";
        if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   ret", (unsigned int)pc, (unsigned int)word);
        else snprintf(buf, sizeof(buf), "ret");
        res.full_text = buf;
        return res;
    }

    /* mv rd, rs: addi rd, rs, 0 (com imm=0, rs!=x0, rd!=x0) */
    if ((word & 0x0000707f) == 0x00000013 && get_imm_I(word) == 0) {
        unsigned rd = get_rd(word);
        unsigned rs1 = get_rs1(word);
        if (rd != 0 && rs1 != 0) {
            res.success = true;
            res.mnemonic = "mv";
            const char *r_rd = m_use_abi_names ? get_reg_name(rd) : get_reg_name(rd);
            const char *r_rs = m_use_abi_names ? get_reg_name(rs1) : get_reg_name(rs1);
            snprintf(op_buf, sizeof(op_buf), "%s, %s", r_rd, r_rs);
            res.operands = op_buf;
            if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   mv %s", (unsigned int)pc, (unsigned int)word, op_buf);
            else snprintf(buf, sizeof(buf), "mv %s", op_buf);
            res.full_text = buf;
            return res;
        }
    }

    /* j offset: jal x0, offset */
    if ((word & 0x0000007f) == 0x0000006f && get_rd(word) == 0) {
        res.success = true;
        res.mnemonic = "j";
        int32_t imm = get_imm_J(word);
        uint32_t target = pc + imm;
        snprintf(op_buf, sizeof(op_buf), "0x%x", (unsigned int)target);
        res.operands = op_buf;
        if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   j %s", (unsigned int)pc, (unsigned int)word, op_buf);
        else snprintf(buf, sizeof(buf), "j %s", op_buf);
        res.full_text = buf;
        return res;
    }

    /* not rd, rs: xori rd, rs, -1 */
    if ((word & 0x0000707f) == 0x00004013 && get_imm_I(word) == -1) {
        res.success = true;
        res.mnemonic = "not";
        snprintf(op_buf, sizeof(op_buf), "%s, %s", get_reg_name(get_rd(word)), get_reg_name(get_rs1(word)));
        res.operands = op_buf;
        if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   not %s", pc, word, op_buf);
        else snprintf(buf, sizeof(buf), "not %s", op_buf);
        res.full_text = buf;
        return res;
    }

    /* neg rd, rs: sub rd, x0, rs */
    if ((word & 0xfe00707f) == 0x40000033 && get_rs1(word) == 0) {
        res.success = true;
        res.mnemonic = "neg";
        snprintf(op_buf, sizeof(op_buf), "%s, %s", get_reg_name(get_rd(word)), get_reg_name(get_rs2(word)));
        res.operands = op_buf;
        if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   neg %s", pc, word, op_buf);
        else snprintf(buf, sizeof(buf), "neg %s", op_buf);
        res.full_text = buf;
        return res;
    }

    /* 2. Decodificação Canônica com Base no Descritor de Tabela */

    if (!res.desc) {
        res.mnemonic = ".word";
        snprintf(op_buf, sizeof(op_buf), "0x%08x", word);
        res.operands = op_buf;
        if (m_show_hex) snprintf(buf, sizeof(buf), "%08x:   %08x   .word %s", pc, word, op_buf);
        else snprintf(buf, sizeof(buf), ".word %s", op_buf);
        res.full_text = buf;
        return res;
    }

    res.success = true;
    res.mnemonic = res.desc->mnemonic;

    unsigned rd  = get_rd(word);
    unsigned rs1 = get_rs1(word);
    unsigned rs2 = get_rs2(word);

    const char *r_d  = get_reg_name(rd);
    const char *r_s1 = get_reg_name(rs1);
    const char *r_s2 = get_reg_name(rs2);

    const char *f_d  = get_freg_name(rd);
    const char *f_s1 = get_freg_name(rs1);
    const char *f_s2 = get_freg_name(rs2);

    switch (res.desc->format) {
        case FMT_R:
            snprintf(op_buf, sizeof(op_buf), "%s, %s, %s", r_d, r_s1, r_s2);
            break;

        case FMT_I:
            if (strcmp(res.desc->mnemonic, "fence") == 0) {
                unsigned pred = (word >> 24) & 0xf;
                unsigned succ = (word >> 20) & 0xf;
                char pred_s[5] = {0}, succ_s[5] = {0};
                int p = 0;
                if (pred & 8) pred_s[p++] = 'i';
                if (pred & 4) pred_s[p++] = 'o';
                if (pred & 2) pred_s[p++] = 'r';
                if (pred & 1) pred_s[p++] = 'w';
                if (!p) pred_s[p++] = '0';
                p = 0;
                if (succ & 8) succ_s[p++] = 'i';
                if (succ & 4) succ_s[p++] = 'o';
                if (succ & 2) succ_s[p++] = 'r';
                if (succ & 1) succ_s[p++] = 'w';
                if (!p) succ_s[p++] = '0';
                snprintf(op_buf, sizeof(op_buf), "%s, %s", pred_s, succ_s);
            } else {
                snprintf(op_buf, sizeof(op_buf), "%s, %s, %d", r_d, r_s1, (int)get_imm_I(word));
            }
            break;

        case FMT_I_SHAMT:
            snprintf(op_buf, sizeof(op_buf), "%s, %s, %u", r_d, r_s1, (unsigned)(word >> 20) & 0x1f);
            break;

        case FMT_I_LOAD:
        case FMT_I_JALR:
            snprintf(op_buf, sizeof(op_buf), "%s, %d(%s)", r_d, (int)get_imm_I(word), r_s1);
            break;

        case FMT_S:
            snprintf(op_buf, sizeof(op_buf), "%s, %d(%s)", r_s2, (int)get_imm_S(word), r_s1);
            break;

        case FMT_B: {
            int32_t imm = get_imm_B(word);
            uint32_t target = pc + imm;
            snprintf(op_buf, sizeof(op_buf), "%s, %s, 0x%x", r_s1, r_s2, target);
            break;
        }

        case FMT_U:
            snprintf(op_buf, sizeof(op_buf), "%s, 0x%x", r_d, (unsigned)get_imm_U(word));
            break;

        case FMT_J: {
            int32_t imm = get_imm_J(word);
            uint32_t target = pc + imm;
            snprintf(op_buf, sizeof(op_buf), "%s, 0x%x", r_d, target);
            break;
        }

        case FMT_SYSTEM:
            /* ecall, ebreak sem operandos */
            op_buf[0] = '\0';
            break;

        case FMT_CSR:
            snprintf(op_buf, sizeof(op_buf), "%s, 0x%x, %s", r_d, get_csr(word), r_s1);
            break;

        case FMT_CSRI:
            snprintf(op_buf, sizeof(op_buf), "%s, 0x%x, %u", r_d, get_csr(word), rs1);
            break;

        case FMT_ATOMIC: {
            bool aq = (word >> 26) & 1;
            bool rl = (word >> 25) & 1;
            std::string mnem = res.desc->mnemonic;
            if (aq && rl) mnem += ".aqrl";
            else if (aq)  mnem += ".aq";
            else if (rl)  mnem += ".rl";
            res.mnemonic = mnem;

            if (strcmp(res.desc->mnemonic, "lr.w") == 0) {
                snprintf(op_buf, sizeof(op_buf), "%s, (%s)", r_d, r_s1);
            } else {
                snprintf(op_buf, sizeof(op_buf), "%s, %s, (%s)", r_d, r_s2, r_s1);
            }
            break;
        }

        case FMT_F_LOAD:
            snprintf(op_buf, sizeof(op_buf), "%s, %d(%s)", f_d, (int)get_imm_I(word), r_s1);
            break;

        case FMT_F_STORE:
            snprintf(op_buf, sizeof(op_buf), "%s, %d(%s)", f_s2, (int)get_imm_S(word), r_s1);
            break;

        case FMT_F_R:
            snprintf(op_buf, sizeof(op_buf), "%s, %s, %s", f_d, f_s1, f_s2);
            break;

        case FMT_F_CVT_F2I:
        case FMT_F_MV_F2I:
        case FMT_F_CLASS:
            snprintf(op_buf, sizeof(op_buf), "%s, %s", r_d, f_s1);
            break;

        case FMT_F_CVT_I2F:
        case FMT_F_MV_I2F:
            snprintf(op_buf, sizeof(op_buf), "%s, %s", f_d, r_s1);
            break;

        case FMT_F_CMP:
            snprintf(op_buf, sizeof(op_buf), "%s, %s, %s", r_d, f_s1, f_s2);
            break;

        default:
            op_buf[0] = '\0';
            break;
    }

    res.operands = op_buf;
    if (m_show_hex) {
        if (op_buf[0]) snprintf(buf, sizeof(buf), "%08x:   %08x   %-7s %s", pc, word, res.mnemonic.c_str(), op_buf);
        else snprintf(buf, sizeof(buf), "%08x:   %08x   %-7s", pc, word, res.mnemonic.c_str());
    } else {
        if (op_buf[0]) snprintf(buf, sizeof(buf), "%-7s %s", res.mnemonic.c_str(), op_buf);
        else snprintf(buf, sizeof(buf), "%-7s", res.mnemonic.c_str());
    }
    res.full_text = buf;
    return res;
}

} // namespace riscv
} // namespace maia

