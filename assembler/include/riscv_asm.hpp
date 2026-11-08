#ifndef MAIA_RISCV_ASM_HPP
#define MAIA_RISCV_ASM_HPP

#include "riscv_isa.hpp"
#include <string>
#include <vector>
#include <map>

namespace maia {
namespace riscv {

enum SectionType {
    SEC_TEXT,
    SEC_DATA,
    SEC_RODATA,
    SEC_BSS
};

struct Section {
    std::string name;
    SectionType type;
    std::vector<uint8_t> data;
    uint32_t align;

    Section(const std::string &n, SectionType t, uint32_t a = 4)
        : name(n), type(t), align(a) {}

    void append_byte(uint8_t b) { data.push_back(b); }
    void append_half(uint16_t h) {
        data.push_back(h & 0xff);
        data.push_back((h >> 8) & 0xff);
    }
    void append_word(uint32_t w) {
        data.push_back(w & 0xff);
        data.push_back((w >> 8) & 0xff);
        data.push_back((w >> 16) & 0xff);
        data.push_back((w >> 24) & 0xff);
    }
    void align_to(uint32_t a) {
        while (data.size() % a != 0) {
            data.push_back(0);
        }
    }
};

struct Symbol {
    std::string name;
    std::string section;
    uint32_t offset;
    bool is_global;
    bool is_defined;
    int32_t constant_value;
    bool is_constant;

    Symbol() : offset(0), is_global(false), is_defined(false),
               constant_value(0), is_constant(false) {}
};

struct Relocation {
    std::string section;
    uint32_t offset;
    std::string symbol;
    uint32_t type;
    int32_t addend;

    Relocation(const std::string &sec = "", uint32_t off = 0, const std::string &sym = "", uint32_t t = 0, int32_t add = 0)
        : section(sec), offset(off), symbol(sym), type(t), addend(add) {}
};

struct AsmDiagnostic {
    int line;
    bool is_error;
    std::string message;
};

class Assembler {
public:
    Assembler();

    /* Monta um script assembly textual RISC-V */
    bool assemble_string(const std::string &source);

    /* Relatórios e Diagnósticos */
    const std::vector<AsmDiagnostic>& get_diagnostics() const { return m_diagnostics; }
    bool has_errors() const;

    /* Acesso às seções emitidas */
    const Section* get_section(const std::string &name) const;
    const std::vector<Section>& get_sections() const { return m_sections; }
    const std::map<std::string, Symbol>& get_symbols() const { return m_symbols; }
    const std::vector<Relocation>& get_relocations() const { return m_relocations; }

    /* Emite binário linear da seção .text */
    std::vector<uint8_t> get_text_binary() const;

    /* Emite binário contíguo de todas as seções alocáveis (.text + .rodata + .data) */
    std::vector<uint8_t> get_image_binary() const;

    /* Emite arquivo ELF32 relocável (.o / .mobj) */
    std::vector<uint8_t> emit_elf32() const;

private:
    std::vector<Section> m_sections;
    std::map<std::string, Symbol> m_symbols;
    std::vector<Relocation> m_relocations;
    std::map<std::string, uint32_t> m_section_bases;
    std::vector<AsmDiagnostic> m_diagnostics;
    int m_active_section_idx;

    void add_relocation(const std::string &sec, uint32_t off, const std::string &sym, uint32_t type, int32_t addend);

    void add_error(int line, const std::string &msg);
    void add_warning(int line, const std::string &msg);

    Section& active_section();
    void set_active_section(const std::string &name, SectionType type, uint32_t align = 4);

    bool parse_expression(const std::string &expr_str, int32_t &result, int line) const;
    int parse_reg(const std::string &token) const;
    int parse_freg(const std::string &token) const;

    bool parse_mem_operand(const std::string &op, int32_t &imm, int &base_reg, int line);

    bool assemble_instruction(const std::string &mnemonic,
                              const std::vector<std::string> &operands,
                              uint32_t current_pc,
                              int line);
};

} // namespace riscv
} // namespace maia

#endif // MAIA_RISCV_ASM_HPP

