#ifndef MAIA_RISCV_DISASM_HPP
#define MAIA_RISCV_DISASM_HPP

#include "riscv_isa.hpp"
#include <string>

namespace maia {
namespace riscv {

struct DisasmResult {
    bool success;
    uint32_t raw_word;
    std::string mnemonic;
    std::string operands;
    std::string full_text;
    const InstructionDesc *desc;
};

class Disassembler {
public:
    Disassembler() : m_show_hex(true), m_use_abi_names(true) {}

    void set_show_hex(bool show) { m_show_hex = show; }
    void set_use_abi_names(bool abi) { m_use_abi_names = abi; }

    /* Desmonta uma única instrução de 32 bits em pc especificado */
    DisasmResult disassemble(uint32_t word, uint32_t pc = 0) const;

private:
    bool m_show_hex;
    bool m_use_abi_names;
};

} // namespace riscv
} // namespace maia

#endif // MAIA_RISCV_DISASM_HPP

