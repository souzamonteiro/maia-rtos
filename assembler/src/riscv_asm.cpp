#include "riscv_asm.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace maia {
namespace riscv {

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string to_lower(const std::string &s) {
    std::string res = s;
    for (size_t i = 0; i < res.size(); i++) {
        res[i] = (char)tolower((unsigned char)res[i]);
    }
    return res;
}

Assembler::Assembler() : m_active_section_idx(-1) {
    set_active_section(".text", SEC_TEXT, 4);
}

bool Assembler::has_errors() const {
    for (size_t i = 0; i < m_diagnostics.size(); i++) {
        if (m_diagnostics[i].is_error) return true;
    }
    return false;
}

void Assembler::add_error(int line, const std::string &msg) {
    AsmDiagnostic d;
    d.line = line;
    d.is_error = true;
    d.message = msg;
    m_diagnostics.push_back(d);
}

void Assembler::add_warning(int line, const std::string &msg) {
    AsmDiagnostic d;
    d.line = line;
    d.is_error = false;
    d.message = msg;
    m_diagnostics.push_back(d);
}

Section& Assembler::active_section() {
    if (m_active_section_idx < 0 || m_active_section_idx >= (int)m_sections.size()) {
        set_active_section(".text", SEC_TEXT, 4);
    }
    return m_sections[m_active_section_idx];
}

void Assembler::set_active_section(const std::string &name, SectionType type, uint32_t align) {
    for (size_t i = 0; i < m_sections.size(); i++) {
        if (m_sections[i].name == name) {
            m_active_section_idx = (int)i;
            return;
        }
    }
    m_sections.push_back(Section(name, type, align));
    m_active_section_idx = (int)m_sections.size() - 1;
}

const Section* Assembler::get_section(const std::string &name) const {
    for (size_t i = 0; i < m_sections.size(); i++) {
        if (m_sections[i].name == name) return &m_sections[i];
    }
    return NULL;
}

std::vector<uint8_t> Assembler::get_text_binary() const {
    const Section *sec = get_section(".text");
    if (sec) return sec->data;
    return std::vector<uint8_t>();
}

std::vector<uint8_t> Assembler::get_image_binary() const {
    std::vector<uint8_t> img;
    for (size_t i = 0; i < m_sections.size(); ++i) {
        const Section &sec = m_sections[i];
        if (sec.type == SEC_BSS) continue;
        uint32_t align = sec.align > 0 ? sec.align : 4;
        while (img.size() % align != 0) img.push_back(0);
        img.insert(img.end(), sec.data.begin(), sec.data.end());
    }
    return img;
}

void Assembler::add_relocation(const std::string &sec, uint32_t off, const std::string &sym, uint32_t type, int32_t addend) {
    m_relocations.emplace_back(sec, off, sym, type, addend);
}

std::vector<uint8_t> Assembler::emit_elf32() const {
    struct OutSection {
        std::string name;
        uint32_t name_offset;
        uint32_t type;
        uint32_t flags;
        uint32_t align;
        uint32_t link;
        uint32_t info;
        uint32_t entsize;
        std::vector<uint8_t> data;
    };

    std::vector<OutSection> out_secs;
    // Section 0: NULL
    {
        OutSection s0;
        s0.name = "";
        s0.name_offset = 0;
        s0.type = 0; // SHT_NULL
        s0.flags = 0;
        s0.align = 0;
        s0.link = 0;
        s0.info = 0;
        s0.entsize = 0;
        out_secs.push_back(s0);
    }

    // Add .text, .rodata, .data, .bss if present
    for (size_t i = 0; i < m_sections.size(); ++i) {
        const Section& sec = m_sections[i];
        if (sec.data.empty() && sec.type != SEC_BSS) continue;
        OutSection s;
        s.name = sec.name;
        s.name_offset = 0;
        s.align = sec.align > 0 ? sec.align : 4;
        s.link = 0;
        s.info = 0;
        s.entsize = 0;
        s.data = sec.data;
        if (sec.type == SEC_TEXT) {
            s.type = 1; // SHT_PROGBITS
            s.flags = 0x6; // SHF_ALLOC | SHF_EXECINSTR
        } else if (sec.type == SEC_RODATA) {
            s.type = 1; // SHT_PROGBITS
            s.flags = 0x2; // SHF_ALLOC
        } else if (sec.type == SEC_DATA) {
            s.type = 1; // SHT_PROGBITS
            s.flags = 0x3; // SHF_ALLOC | SHF_WRITE
        } else if (sec.type == SEC_BSS) {
            s.type = 8; // SHT_NOBITS
            s.flags = 0x3; // SHF_ALLOC | SHF_WRITE
        }
        out_secs.push_back(s);
    }

    // Build .strtab and symbols for .symtab
    std::vector<uint8_t> strtab;
    strtab.push_back(0); // initial null

    auto add_str = [&](const std::string& str) -> uint32_t {
        if (str.empty()) return 0;
        uint32_t off = (uint32_t)strtab.size();
        for (size_t c = 0; c < str.size(); ++c) strtab.push_back((uint8_t)str[c]);
        strtab.push_back(0);
        return off;
    };

    struct RawSym {
        uint32_t name_off;
        uint32_t value;
        uint32_t size;
        uint8_t info;
        uint8_t other;
        uint16_t shndx;
    };

    std::vector<RawSym> locals;
    std::vector<RawSym> globals;
    std::vector<std::string> local_names;
    std::vector<std::string> global_names;

    // Symbol 0 is STN_UNDEF
    RawSym s_undef = {0, 0, 0, 0, 0, 0};
    locals.push_back(s_undef);

    auto get_sec_ndx = [&](const std::string& sec_name) -> uint16_t {
        for (size_t i = 0; i < out_secs.size(); ++i) {
            if (out_secs[i].name == sec_name) return (uint16_t)i;
        }
        return 0; // SHN_UNDEF
    };

    for (std::map<std::string, Symbol>::const_iterator it = m_symbols.begin(); it != m_symbols.end(); ++it) {
        const Symbol& sym = it->second;
        RawSym r;
        r.name_off = add_str(sym.name);
        r.value = sym.offset;
        r.size = 0;
        r.other = 0;
        r.shndx = get_sec_ndx(sym.section);
        uint8_t bind = sym.is_global ? 1 : 0; // STB_GLOBAL (1) or STB_LOCAL (0)
        uint8_t type = (sym.section == ".text") ? 2 : (sym.section.empty() ? 0 : 1); // STT_FUNC (2), STT_OBJECT (1), STT_NOTYPE (0)
        r.info = (bind << 4) | (type & 0xf);

        if (sym.is_global) {
            globals.push_back(r);
            global_names.push_back(sym.name);
        } else {
            locals.push_back(r);
            local_names.push_back(sym.name);
        }
    }

    uint32_t first_global_idx = (uint32_t)locals.size();
    std::vector<RawSym> all_syms = locals;
    all_syms.insert(all_syms.end(), globals.begin(), globals.end());

    std::map<std::string, uint32_t> sym_to_idx;
    for (size_t i = 0; i < local_names.size(); ++i) {
        sym_to_idx[local_names[i]] = 1 + (uint32_t)i;
    }
    for (size_t i = 0; i < global_names.size(); ++i) {
        sym_to_idx[global_names[i]] = first_global_idx + (uint32_t)i;
    }

    std::vector<uint8_t> symtab_data;
    for (size_t i = 0; i < all_syms.size(); ++i) {
        const RawSym& s = all_syms[i];
        uint32_t name = s.name_off;
        uint32_t val = s.value;
        uint32_t sz = s.size;
        symtab_data.push_back(name & 0xff);
        symtab_data.push_back((name >> 8) & 0xff);
        symtab_data.push_back((name >> 16) & 0xff);
        symtab_data.push_back((name >> 24) & 0xff);

        symtab_data.push_back(val & 0xff);
        symtab_data.push_back((val >> 8) & 0xff);
        symtab_data.push_back((val >> 16) & 0xff);
        symtab_data.push_back((val >> 24) & 0xff);

        symtab_data.push_back(sz & 0xff);
        symtab_data.push_back((sz >> 8) & 0xff);
        symtab_data.push_back((sz >> 16) & 0xff);
        symtab_data.push_back((sz >> 24) & 0xff);

        symtab_data.push_back(s.info);
        symtab_data.push_back(s.other);
        symtab_data.push_back(s.shndx & 0xff);
        symtab_data.push_back((s.shndx >> 8) & 0xff);
    }

    uint32_t symtab_idx = (uint32_t)out_secs.size();
    uint32_t strtab_idx = symtab_idx + 1;

    {
        OutSection s_sym;
        s_sym.name = ".symtab";
        s_sym.name_offset = 0;
        s_sym.type = 2; // SHT_SYMTAB
        s_sym.flags = 0;
        s_sym.align = 4;
        s_sym.link = strtab_idx;
        s_sym.info = first_global_idx;
        s_sym.entsize = 16;
        s_sym.data = symtab_data;
        out_secs.push_back(s_sym);
    }

    {
        OutSection s_str;
        s_str.name = ".strtab";
        s_str.name_offset = 0;
        s_str.type = 3; // SHT_STRTAB
        s_str.flags = 0;
        s_str.align = 1;
        s_str.link = 0;
        s_str.info = 0;
        s_str.entsize = 0;
        s_str.data = strtab;
        out_secs.push_back(s_str);
    }

    // Add .rela.* sections if any relocations exist
    for (size_t sec_i = 1; sec_i < symtab_idx; ++sec_i) {
        const std::string& sec_name = out_secs[sec_i].name;
        std::vector<uint8_t> rela_data;
        for (const auto& r : m_relocations) {
            if (r.section == sec_name) {
                uint32_t sym_idx = 0;
                auto sit = sym_to_idx.find(r.symbol);
                if (sit != sym_to_idx.end()) {
                    sym_idx = sit->second;
                }
                // r_offset (uint32_t)
                rela_data.push_back(r.offset & 0xff);
                rela_data.push_back((r.offset >> 8) & 0xff);
                rela_data.push_back((r.offset >> 16) & 0xff);
                rela_data.push_back((r.offset >> 24) & 0xff);

                // r_info: (sym_idx << 8) | (type & 0xff)
                uint32_t r_info = (sym_idx << 8) | (r.type & 0xff);
                rela_data.push_back(r_info & 0xff);
                rela_data.push_back((r_info >> 8) & 0xff);
                rela_data.push_back((r_info >> 16) & 0xff);
                rela_data.push_back((r_info >> 24) & 0xff);

                // r_addend (int32_t)
                uint32_t r_addend = (uint32_t)r.addend;
                rela_data.push_back(r_addend & 0xff);
                rela_data.push_back((r_addend >> 8) & 0xff);
                rela_data.push_back((r_addend >> 16) & 0xff);
                rela_data.push_back((r_addend >> 24) & 0xff);
            }
        }
        if (!rela_data.empty()) {
            OutSection s_rela;
            s_rela.name = ".rela" + sec_name;
            s_rela.name_offset = 0;
            s_rela.type = 4; // SHT_RELA
            s_rela.flags = 0x40; // SHF_INFO_LINK
            s_rela.align = 4;
            s_rela.link = symtab_idx;
            s_rela.info = (uint32_t)sec_i;
            s_rela.entsize = 12;
            s_rela.data = rela_data;
            out_secs.push_back(s_rela);
        }
    }

    uint32_t shstrtab_idx = (uint32_t)out_secs.size();

    // Build .shstrtab
    std::vector<uint8_t> shstrtab;
    shstrtab.push_back(0);
    auto add_shstr = [&](const std::string& str) -> uint32_t {
        if (str.empty()) return 0;
        uint32_t off = (uint32_t)shstrtab.size();
        for (size_t c = 0; c < str.size(); ++c) shstrtab.push_back((uint8_t)str[c]);
        shstrtab.push_back(0);
        return off;
    };

    for (size_t i = 0; i < out_secs.size(); ++i) {
        out_secs[i].name_offset = add_shstr(out_secs[i].name);
    }
    uint32_t shstrtab_name_off = add_shstr(".shstrtab");

    {
        OutSection s_shstr;
        s_shstr.name = ".shstrtab";
        s_shstr.name_offset = shstrtab_name_off;
        s_shstr.type = 3; // SHT_STRTAB
        s_shstr.flags = 0;
        s_shstr.align = 1;
        s_shstr.link = 0;
        s_shstr.info = 0;
        s_shstr.entsize = 0;
        s_shstr.data = shstrtab;
        out_secs.push_back(s_shstr);
    }

    // Layout ELF file:
    // 1. Header (52 bytes)
    // 2. Sections data
    // 3. Section Header Table (out_secs.size() * 40 bytes)
    std::vector<uint8_t> elf(52, 0);

    // e_ident
    elf[0] = 0x7f; elf[1] = 'E'; elf[2] = 'L'; elf[3] = 'F';
    elf[4] = 1; // ELFCLASS32
    elf[5] = 1; // ELFDATA2LSB
    elf[6] = 1; // EV_CURRENT
    // bytes 7..15 remain 0

    // e_type = ET_REL (1)
    elf[16] = 1; elf[17] = 0;
    // e_machine = EM_RISCV (243 / 0xf3)
    elf[18] = 0xf3; elf[19] = 0;
    // e_version = 1
    elf[20] = 1; elf[21] = 0; elf[22] = 0; elf[23] = 0;
    // e_entry = 0 (24..27)
    // e_phoff = 0 (28..31)
    // e_shoff (will patch later at 32..35)
    // e_flags = 0x2 (EF_RISCV_FLOAT_ABI_SINGLE)
    elf[36] = 0x02; elf[37] = 0; elf[38] = 0; elf[39] = 0;
    // e_ehsize = 52
    elf[40] = 52; elf[41] = 0;
    // e_phentsize = 0 (42..43)
    // e_phnum = 0 (44..45)
    // e_shentsize = 40
    elf[46] = 40; elf[47] = 0;
    // e_shnum = out_secs.size()
    uint16_t shnum = (uint16_t)out_secs.size();
    elf[48] = shnum & 0xff; elf[49] = (shnum >> 8) & 0xff;
    // e_shstrndx = shstrtab_idx
    elf[50] = shstrtab_idx & 0xff; elf[51] = (shstrtab_idx >> 8) & 0xff;

    std::vector<uint32_t> sec_file_offsets(out_secs.size(), 0);
    std::vector<uint32_t> sec_file_sizes(out_secs.size(), 0);

    // Write sections data
    for (size_t i = 1; i < out_secs.size(); ++i) {
        if (out_secs[i].type == 8) { // SHT_NOBITS
            sec_file_offsets[i] = (uint32_t)elf.size();
            sec_file_sizes[i] = (uint32_t)out_secs[i].data.size();
            continue;
        }
        uint32_t align = out_secs[i].align > 0 ? out_secs[i].align : 1;
        while (elf.size() % align != 0) elf.push_back(0);
        sec_file_offsets[i] = (uint32_t)elf.size();
        sec_file_sizes[i] = (uint32_t)out_secs[i].data.size();
        elf.insert(elf.end(), out_secs[i].data.begin(), out_secs[i].data.end());
    }

    // Align for Section Header Table
    while (elf.size() % 4 != 0) elf.push_back(0);
    uint32_t shoff = (uint32_t)elf.size();

    // Patch e_shoff in header
    elf[32] = shoff & 0xff;
    elf[33] = (shoff >> 8) & 0xff;
    elf[34] = (shoff >> 16) & 0xff;
    elf[35] = (shoff >> 24) & 0xff;

    // Write Section Header Table
    for (size_t i = 0; i < out_secs.size(); ++i) {
        const OutSection& s = out_secs[i];
        uint32_t sh_name = s.name_offset;
        uint32_t sh_type = s.type;
        uint32_t sh_flags = s.flags;
        uint32_t sh_addr = 0;
        uint32_t sh_offset = sec_file_offsets[i];
        uint32_t sh_size = sec_file_sizes[i];
        uint32_t sh_link = s.link;
        uint32_t sh_info = s.info;
        uint32_t sh_addralign = s.align;
        uint32_t sh_entsize = s.entsize;

        auto append_u32 = [&](uint32_t v) {
            elf.push_back(v & 0xff);
            elf.push_back((v >> 8) & 0xff);
            elf.push_back((v >> 16) & 0xff);
            elf.push_back((v >> 24) & 0xff);
        };

        append_u32(sh_name);
        append_u32(sh_type);
        append_u32(sh_flags);
        append_u32(sh_addr);
        append_u32(sh_offset);
        append_u32(sh_size);
        append_u32(sh_link);
        append_u32(sh_info);
        append_u32(sh_addralign);
        append_u32(sh_entsize);
    }

    return elf;
}

int Assembler::parse_reg(const std::string &token) const {
    return parse_reg_name(trim(token).c_str());
}

int Assembler::parse_freg(const std::string &token) const {
    return parse_freg_name(trim(token).c_str());
}

static int parse_csr_name(const std::string& token) {
    std::string s = to_lower(trim(token));
    if (s == "cycle") return 0xc00;
    if (s == "time") return 0xc01;
    if (s == "instret") return 0xc02;
    if (s == "cycleh") return 0xc80;
    if (s == "timeh") return 0xc81;
    if (s == "instreth") return 0xc82;
    if (s == "mcycle") return 0xb00;
    if (s == "mtime") return 0x701;
    if (s == "minstret") return 0xb02;
    if (s == "mcycleh") return 0xb80;
    if (s == "minstreth") return 0xb82;
    if (s == "mstatus") return 0x300;
    if (s == "mie") return 0x304;
    if (s == "mtvec") return 0x305;
    if (s == "mepc") return 0x341;
    if (s == "mcause") return 0x342;
    if (s == "mtval") return 0x343;
    if (s == "fcsr") return 0x003;
    if (s == "fflags") return 0x001;
    if (s == "frm") return 0x002;

    char *endptr = NULL;
    long val = strtol(s.c_str(), &endptr, 0);
    if (endptr && *endptr == '\0' && val >= 0 && val <= 4095) {
        return (int)val;
    }
    return -1;
}

/* Avaliador simples de expressões aritméticas com suporte a constantes e símbolos */
bool Assembler::parse_expression(const std::string &expr_str, int32_t &result, int line) const {
    std::string s = trim(expr_str);
    if (s.empty()) return false;

    /* %hi(sym), %lo(sym), %pcrel_hi(sym), %pcrel_lo(sym) */
    if (s[0] == '%') {
        size_t lp = s.find('(');
        size_t rp = s.rfind(')');
        if (lp != std::string::npos && rp != std::string::npos && rp > lp) {
            std::string mod = to_lower(trim(s.substr(1, lp - 1)));
            std::string sym = trim(s.substr(lp + 1, rp - lp - 1));

            int32_t sym_val = 0;
            std::map<std::string, Symbol>::const_iterator it = m_symbols.find(sym);
            if (it != m_symbols.end()) {
                sym_val = it->second.is_constant ? it->second.constant_value : (int32_t)it->second.offset;
            }

            if (mod == "hi" || mod == "pcrel_hi") {
                result = ((sym_val + 0x800) >> 12) & 0xfffff;
                return true;
            } else if (mod == "lo" || mod == "pcrel_lo") {
                result = sym_val & 0xfff;
                if (result & 0x800) result |= 0xfffff000; /* Sign extend 12 */
                return true;
            }
        }
    }

    /* Hexadecimal 0x... */
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        char *endptr = NULL;
        result = (int32_t)strtoul(s.c_str(), &endptr, 16);
        return (endptr && *endptr == '\0');
    }

    /* Binário 0b... */
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        char *endptr = NULL;
        result = (int32_t)strtoul(s.c_str() + 2, &endptr, 2);
        return (endptr && *endptr == '\0');
    }

    /* Decimal direto (com sinal opcional) */
    char *endptr = NULL;
    long val = strtol(s.c_str(), &endptr, 10);
    if (endptr && *endptr == '\0') {
        result = (int32_t)val;
        return true;
    }

    /* Símbolo definido na tabela */
    std::map<std::string, Symbol>::const_iterator it = m_symbols.find(s);
    if (it != m_symbols.end()) {
        result = it->second.is_constant ? it->second.constant_value : (int32_t)it->second.offset;
        return true;
    }

    /* Operação aritmética binária simples (ex: "8 + 7", "64 + 32", "a * 4") */
    size_t op_pos = std::string::npos;
    char op = 0;
    const char *ops = "+-*/&|^";
    for (const char *o = ops; *o; ++o) {
        size_t p = s.find(*o);
        if (p != std::string::npos && p > 0) {
            op_pos = p;
            op = *o;
            break;
        }
    }

    if (op_pos != std::string::npos) {
        std::string lhs_s = trim(s.substr(0, op_pos));
        std::string rhs_s = trim(s.substr(op_pos + 1));
        int32_t lhs = 0, rhs = 0;
        if (parse_expression(lhs_s, lhs, line) && parse_expression(rhs_s, rhs, line)) {
            switch (op) {
                case '+': result = lhs + rhs; return true;
                case '-': result = lhs - rhs; return true;
                case '*': result = lhs * rhs; return true;
                case '/': if (rhs != 0) { result = lhs / rhs; return true; } break;
                case '&': result = lhs & rhs; return true;
                case '|': result = lhs | rhs; return true;
                case '^': result = lhs ^ rhs; return true;
            }
        }
    }

    (void)line;
    return false;
}

/* Parseia operandos de memória: "12(sp)", "0(a0)", "(a2)", "%lo(msg)(a0)" */
bool Assembler::parse_mem_operand(const std::string &op, int32_t &imm, int &base_reg, int line) {
    std::string s = trim(op);
    size_t lp = s.find('(');
    size_t rp = s.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return false;

    std::string imm_part = trim(s.substr(0, lp));
    std::string reg_part = trim(s.substr(lp + 1, rp - lp - 1));

    base_reg = parse_reg(reg_part);
    if (base_reg < 0) return false;

    if (imm_part.empty()) {
        imm = 0;
        return true;
    }

    return parse_expression(imm_part, imm, line);
}

/* =========================================================================
 * Montagem de Uma Instrução na Seção Ativa
 * ========================================================================= */

bool Assembler::assemble_instruction(const std::string &mnemonic,
                                    const std::vector<std::string> &operands,
                                    uint32_t current_pc,
                                    int line) {
    std::string mnem = to_lower(mnemonic);
    size_t argc = operands.size();

    /* ---------------- Pseudo-instruções ---------------- */

    /* nop -> addi x0, x0, 0 */
    if (mnem == "nop" && argc == 0) {
        active_section().append_word(encode_I(0, 0, 0, 0, 0x13));
        return true;
    }

    /* ret -> jalr x0, 0(ra) */
    if (mnem == "ret" && argc == 0) {
        active_section().append_word(encode_I(0, 1, 0, 0, 0x67));
        return true;
    }

    /* j target -> jal x0, target */
    if (mnem == "j" && argc == 1) {
        int32_t target_addr = 0;
        if (!parse_expression(operands[0], target_addr, line)) {
            add_error(line, "Unknown target symbol: " + operands[0]);
            return false;
        }
        int32_t offset = target_addr - (int32_t)current_pc;
        active_section().append_word(encode_J(offset, 0, 0x6f));
        return true;
    }

    /* jr rs -> jalr x0, 0(rs) */
    if (mnem == "jr" && argc == 1) {
        int rs = parse_reg(operands[0]);
        if (rs < 0) { add_error(line, "Invalid register: " + operands[0]); return false; }
        active_section().append_word(encode_I(0, rs, 0, 0, 0x67));
        return true;
    }

    /* mv rd, rs -> addi rd, rs, 0 */
    if (mnem == "mv" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in mv"); return false; }
        active_section().append_word(encode_I(0, rs, 0, rd, 0x13));
        return true;
    }

    /* not rd, rs -> xori rd, rs, -1 */
    if (mnem == "not" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in not"); return false; }
        active_section().append_word(encode_I(-1, rs, 4, rd, 0x13));
        return true;
    }

    /* neg rd, rs -> sub rd, x0, rs */
    if (mnem == "neg" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in neg"); return false; }
        active_section().append_word(encode_R(0x20, rs, 0, 0, rd, 0x33));
        return true;
    }

    /* snez rd, rs -> sltu rd, x0, rs */
    if (mnem == "snez" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in snez"); return false; }
        active_section().append_word(encode_R(0, rs, 0, 3, rd, 0x33));
        return true;
    }

    /* seqz rd, rs -> sltiu rd, rs, 1 */
    if (mnem == "seqz" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in seqz"); return false; }
        active_section().append_word(encode_I(1, rs, 3, rd, 0x13));
        return true;
    }

    /* sltz rd, rs -> slt rd, rs, x0 */
    if (mnem == "sltz" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in sltz"); return false; }
        active_section().append_word(encode_R(0, 0, rs, 2, rd, 0x33));
        return true;
    }

    /* sgtz rd, rs -> slt rd, x0, rs */
    if (mnem == "sgtz" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int rs = parse_reg(operands[1]);
        if (rd < 0 || rs < 0) { add_error(line, "Invalid registers in sgtz"); return false; }
        active_section().append_word(encode_R(0, rs, 0, 2, rd, 0x33));
        return true;
    }

    /* rdcycle rd -> csrrs rd, 0xc00, x0 */
    if (mnem == "rdcycle" && argc == 1) {
        int rd = parse_reg(operands[0]);
        if (rd < 0) { add_error(line, "Invalid register in rdcycle"); return false; }
        active_section().append_word(0xc0002073 | ((rd & 0x1f) << 7));
        return true;
    }

    /* rdcycleh rd -> csrrs rd, 0xc80, x0 */
    if (mnem == "rdcycleh" && argc == 1) {
        int rd = parse_reg(operands[0]);
        if (rd < 0) { add_error(line, "Invalid register in rdcycleh"); return false; }
        active_section().append_word(0xc8002073 | ((rd & 0x1f) << 7));
        return true;
    }

    /* rdtime rd -> csrrs rd, 0xc01, x0 */
    if (mnem == "rdtime" && argc == 1) {
        int rd = parse_reg(operands[0]);
        if (rd < 0) { add_error(line, "Invalid register in rdtime"); return false; }
        active_section().append_word(0xc0102073 | ((rd & 0x1f) << 7));
        return true;
    }

    /* rdinstret rd -> csrrs rd, 0xc02, x0 */
    if (mnem == "rdinstret" && argc == 1) {
        int rd = parse_reg(operands[0]);
        if (rd < 0) { add_error(line, "Invalid register in rdinstret"); return false; }
        active_section().append_word(0xc0202073 | ((rd & 0x1f) << 7));
        return true;
    }

    /* csrr rd, csr -> csrrs rd, csr, x0 */
    if (mnem == "csrr" && argc == 2) {
        int rd = parse_reg(operands[0]);
        int csr = parse_csr_name(operands[1]);
        if (rd < 0 || csr < 0) { add_error(line, "Invalid operands in csrr"); return false; }
        active_section().append_word(0x00002073 | ((csr & 0xfff) << 20) | ((rd & 0x1f) << 7));
        return true;
    }

    /* csrw csr, rs -> csrrw x0, csr, rs */
    if (mnem == "csrw" && argc == 2) {
        int csr = parse_csr_name(operands[0]);
        int rs = parse_reg(operands[1]);
        if (csr < 0 || rs < 0) { add_error(line, "Invalid operands in csrw"); return false; }
        active_section().append_word(0x00001073 | ((csr & 0xfff) << 20) | ((rs & 0x1f) << 15));
        return true;
    }

    /* csrs csr, rs -> csrrs x0, csr, rs */
    if (mnem == "csrs" && argc == 2) {
        int csr = parse_csr_name(operands[0]);
        int rs = parse_reg(operands[1]);
        if (csr < 0 || rs < 0) { add_error(line, "Invalid operands in csrs"); return false; }
        active_section().append_word(0x00002073 | ((csr & 0xfff) << 20) | ((rs & 0x1f) << 15));
        return true;
    }

    /* csrc csr, rs -> csrrc x0, csr, rs */
    if (mnem == "csrc" && argc == 2) {
        int csr = parse_csr_name(operands[0]);
        int rs = parse_reg(operands[1]);
        if (csr < 0 || rs < 0) { add_error(line, "Invalid operands in csrc"); return false; }
        active_section().append_word(0x00003073 | ((csr & 0xfff) << 20) | ((rs & 0x1f) << 15));
        return true;
    }

    /* li rd, imm */
    if (mnem == "li" && argc == 2) {
        int rd = parse_reg(operands[0]);
        if (rd < 0) { add_error(line, "Invalid destination register: " + operands[0]); return false; }
        int32_t val = 0;
        if (!parse_expression(operands[1], val, line)) {
            add_error(line, "Invalid immediate in li: " + operands[1]);
            return false;
        }

        if (val >= -2048 && val <= 2047) {
            /* Cabe em 12 bits assinado: addi rd, x0, val */
            active_section().append_word(encode_I(val, 0, 0, rd, 0x13));
        } else {
            /* 32 bits completo: lui rd, hi20 + addi rd, rd, lo12 */
            int32_t hi20 = ((val + 0x800) >> 12) & 0xfffff;
            int32_t lo12 = val - (hi20 << 12);
            active_section().append_word(encode_U(hi20, rd, 0x37));
            active_section().append_word(encode_I(lo12, rd, 0, rd, 0x13));
        }
        return true;
    }

    /* la rd, symbol */
    if (mnem == "la" && argc == 2) {
        int rd = parse_reg(operands[0]);
        if (rd < 0) { add_error(line, "Invalid destination register: " + operands[0]); return false; }
        std::string sym_name = trim(operands[1]);
        std::map<std::string, Symbol>::const_iterator it = m_symbols.find(sym_name);
        if (it == m_symbols.end()) {
            add_error(line, "Unknown symbol in la: " + sym_name);
            return false;
        }
        uint32_t sym_sec_base = 0;
        std::map<std::string, uint32_t>::const_iterator s_it = m_section_bases.find(it->second.section);
        if (s_it != m_section_bases.end()) sym_sec_base = s_it->second;

        uint32_t cur_sec_base = 0;
        std::map<std::string, uint32_t>::const_iterator c_it = m_section_bases.find(active_section().name);
        if (c_it != m_section_bases.end()) cur_sec_base = c_it->second;

        uint32_t sym_abs = sym_sec_base + it->second.offset;
        uint32_t pc_abs = cur_sec_base + current_pc;
        int32_t delta = (int32_t)sym_abs - (int32_t)pc_abs;

        int32_t hi20 = ((delta + 0x800) >> 12) & 0xfffff;
        int32_t lo12 = delta - (hi20 << 12);
        active_section().append_word(encode_U((uint32_t)hi20, rd, 0x17)); // auipc rd, hi20
        active_section().append_word(encode_I(lo12, rd, 0, rd, 0x13));    // addi rd, rd, lo12
        return true;
    }

    /* call symbol */
    if (mnem == "call" && argc == 1) {
        std::string sym_name = trim(operands[0]);
        std::map<std::string, Symbol>::const_iterator it = m_symbols.find(sym_name);
        if (it != m_symbols.end() && it->second.is_defined && !it->second.section.empty()) {
            uint32_t sym_sec_base = 0;
            std::map<std::string, uint32_t>::const_iterator s_it = m_section_bases.find(it->second.section);
            if (s_it != m_section_bases.end()) sym_sec_base = s_it->second;

            uint32_t cur_sec_base = 0;
            std::map<std::string, uint32_t>::const_iterator c_it = m_section_bases.find(active_section().name);
            if (c_it != m_section_bases.end()) cur_sec_base = c_it->second;

            uint32_t sym_abs = sym_sec_base + it->second.offset;
            uint32_t pc_abs = cur_sec_base + current_pc;
            int32_t delta = (int32_t)sym_abs - (int32_t)pc_abs;

            int32_t hi20 = ((delta + 0x800) >> 12) & 0xfffff;
            int32_t lo12 = delta - (hi20 << 12);
            active_section().append_word(encode_U((uint32_t)hi20, 1 /* ra */, 0x17)); // auipc ra, hi20
            active_section().append_word(encode_I(lo12, 1 /* ra */, 0, 1 /* ra */, 0x67)); // jalr ra, ra, lo12
            return true;
        } else {
            // External / undefined symbol: emit auipc ra, 0 + jalr ra, ra, 0 and record relocation
            add_relocation(active_section().name, current_pc, sym_name, 19 /* R_RISCV_CALL_PLT */, 0);

            if (it == m_symbols.end()) {
                Symbol sym;
                sym.name = sym_name;
                sym.section = "";
                sym.offset = 0;
                sym.is_global = true;
                sym.is_defined = false;
                m_symbols[sym_name] = sym;
            }

            active_section().append_word(0x00000097); // auipc ra, 0
            active_section().append_word(0x000080e7); // jalr ra, ra, 0
            return true;
        }
    }

    /* ---------------- Instruções Canônicas do Catálogo ---------------- */

    std::string base_mnem = mnem;
    bool has_aq = false;
    bool has_rl = false;

    if (base_mnem.size() > 4 && (base_mnem.substr(0, 3) == "amo" || base_mnem.substr(0, 2) == "lr" || base_mnem.substr(0, 2) == "sc")) {
        if (base_mnem.size() > 5 && base_mnem.substr(base_mnem.size() - 5) == ".aqrl") {
            has_aq = true; has_rl = true;
            base_mnem = base_mnem.substr(0, base_mnem.size() - 5);
        } else if (base_mnem.size() > 3 && base_mnem.substr(base_mnem.size() - 3) == ".aq") {
            has_aq = true;
            base_mnem = base_mnem.substr(0, base_mnem.size() - 3);
        } else if (base_mnem.size() > 3 && base_mnem.substr(base_mnem.size() - 3) == ".rl") {
            has_rl = true;
            base_mnem = base_mnem.substr(0, base_mnem.size() - 3);
        }
    }

    const InstructionDesc *desc = find_instruction_by_mnemonic(base_mnem.c_str());
    if (!desc) {
        add_error(line, "Unknown or unsupported instruction: " + mnem);
        return false;
    }

    switch (desc->format) {
        case FMT_R: {
            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rd, rs1, rs2"); return false; }
            int rd  = parse_reg(operands[0]);
            int rs1 = parse_reg(operands[1]);
            int rs2 = parse_reg(operands[2]);
            if (rd < 0 || rs1 < 0 || rs2 < 0) { add_error(line, "Invalid register in " + mnem); return false; }
            unsigned f7 = (desc->match >> 25) & 0x7f;
            unsigned f3 = (desc->match >> 12) & 0x7;
            unsigned op = desc->match & 0x7f;
            active_section().append_word(encode_R(f7, rs2, rs1, f3, rd, op));
            return true;
        }

        case FMT_I: {
            if (mnem == "fence") {
                /* fence pred, succ */
                unsigned pred = 0xf, succ = 0xf;
                if (argc == 2) {
                    std::string p_str = to_lower(trim(operands[0]));
                    std::string s_str = to_lower(trim(operands[1]));
                    pred = 0; succ = 0;
                    for (size_t c = 0; c < p_str.size(); c++) {
                        if (p_str[c] == 'i') pred |= 8;
                        if (p_str[c] == 'o') pred |= 4;
                        if (p_str[c] == 'r') pred |= 2;
                        if (p_str[c] == 'w') pred |= 1;
                    }
                    for (size_t c = 0; c < s_str.size(); c++) {
                        if (s_str[c] == 'i') succ |= 8;
                        if (s_str[c] == 'o') succ |= 4;
                        if (s_str[c] == 'r') succ |= 2;
                        if (s_str[c] == 'w') succ |= 1;
                    }
                }
                uint32_t inst = 0x0000000f | (pred << 24) | (succ << 20);
                active_section().append_word(inst);
                return true;
            }

            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rd, rs1, imm"); return false; }
            int rd  = parse_reg(operands[0]);
            int rs1 = parse_reg(operands[1]);
            int32_t imm = 0;
            if (rd < 0 || rs1 < 0 || !parse_expression(operands[2], imm, line)) {
                add_error(line, "Invalid operands in " + mnem);
                return false;
            }
            unsigned f3 = (desc->match >> 12) & 0x7;
            unsigned op = desc->match & 0x7f;
            active_section().append_word(encode_I(imm, rs1, f3, rd, op));
            return true;
        }

        case FMT_I_SHAMT: {
            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rd, rs1, shamt"); return false; }
            int rd  = parse_reg(operands[0]);
            int rs1 = parse_reg(operands[1]);
            int32_t shamt = 0;
            if (rd < 0 || rs1 < 0 || !parse_expression(operands[2], shamt, line)) {
                add_error(line, "Invalid operands in " + mnem);
                return false;
            }
            unsigned f7 = (desc->match >> 25) & 0x7f;
            unsigned f3 = (desc->match >> 12) & 0x7;
            unsigned op = desc->match & 0x7f;
            active_section().append_word(encode_R(f7, shamt & 0x1f, rs1, f3, rd, op));
            return true;
        }

        case FMT_I_LOAD: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: rd, imm(rs1)"); return false; }
            int rd = parse_reg(operands[0]);
            int32_t imm = 0;
            int rs1 = -1;
            if (rd < 0 || !parse_mem_operand(operands[1], imm, rs1, line)) {
                add_error(line, "Invalid memory operand in " + mnem + ": " + operands[1]);
                return false;
            }
            unsigned f3 = (desc->match >> 12) & 0x7;
            unsigned op = desc->match & 0x7f;
            active_section().append_word(encode_I(imm, rs1, f3, rd, op));
            return true;
        }

        case FMT_I_JALR: {
            int rd = 1; /* ra por padrão */
            int rs1 = -1;
            int32_t imm = 0;
            if (argc == 1) {
                if (!parse_mem_operand(operands[0], imm, rs1, line)) {
                    rs1 = parse_reg(operands[0]);
                }
            } else if (argc == 2) {
                rd = parse_reg(operands[0]);
                if (!parse_mem_operand(operands[1], imm, rs1, line)) {
                    rs1 = parse_reg(operands[1]);
                }
            }
            if (rd < 0 || rs1 < 0) { add_error(line, "Invalid operands in jalr"); return false; }
            active_section().append_word(encode_I(imm, rs1, 0, rd, 0x67));
            return true;
        }

        case FMT_S: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: rs2, imm(rs1)"); return false; }
            int rs2 = parse_reg(operands[0]);
            int32_t imm = 0;
            int rs1 = -1;
            if (rs2 < 0 || !parse_mem_operand(operands[1], imm, rs1, line)) {
                add_error(line, "Invalid store operand in " + mnem);
                return false;
            }
            unsigned f3 = (desc->match >> 12) & 0x7;
            unsigned op = desc->match & 0x7f;
            active_section().append_word(encode_S(imm, rs2, rs1, f3, op));
            return true;
        }

        case FMT_B: {
            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rs1, rs2, target"); return false; }
            int rs1 = parse_reg(operands[0]);
            int rs2 = parse_reg(operands[1]);
            int32_t target_addr = 0;
            if (rs1 < 0 || rs2 < 0 || !parse_expression(operands[2], target_addr, line)) {
                add_error(line, "Invalid branch operands in " + mnem);
                return false;
            }
            int32_t offset = target_addr - (int32_t)current_pc;
            unsigned f3 = (desc->match >> 12) & 0x7;
            unsigned op = desc->match & 0x7f;
            active_section().append_word(encode_B(offset, rs2, rs1, f3, op));
            return true;
        }

        case FMT_U: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: rd, imm20"); return false; }
            int rd = parse_reg(operands[0]);
            int32_t imm20 = 0;
            if (rd < 0 || !parse_expression(operands[1], imm20, line)) {
                add_error(line, "Invalid operands in " + mnem);
                return false;
            }
            active_section().append_word(encode_U((uint32_t)imm20, rd, desc->match & 0x7f));
            return true;
        }

        case FMT_J: {
            int rd = 1; /* ra por padrão em jal */
            int32_t target = 0;
            if (argc == 1) {
                if (!parse_expression(operands[0], target, line)) {
                    add_error(line, "Invalid target in jal: " + operands[0]);
                    return false;
                }
            } else if (argc == 2) {
                rd = parse_reg(operands[0]);
                if (rd < 0 || !parse_expression(operands[1], target, line)) {
                    add_error(line, "Invalid operands in jal");
                    return false;
                }
            }
            int32_t offset = target - (int32_t)current_pc;
            active_section().append_word(encode_J(offset, rd, 0x6f));
            return true;
        }

        case FMT_SYSTEM:
            active_section().append_word(desc->match);
            return true;

        case FMT_CSR: {
            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rd, csr, rs1"); return false; }
            int rd = parse_reg(operands[0]);
            int csr = parse_csr_name(operands[1]);
            int rs1 = parse_reg(operands[2]);
            if (rd < 0 || csr < 0 || rs1 < 0) {
                add_error(line, "Invalid operands in " + mnem);
                return false;
            }
            uint32_t inst = desc->match | ((csr & 0xfff) << 20) | ((rs1 & 0x1f) << 15) | ((rd & 0x1f) << 7);
            active_section().append_word(inst);
            return true;
        }

        case FMT_CSRI: {
            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rd, csr, uimm"); return false; }
            int rd = parse_reg(operands[0]);
            int csr = parse_csr_name(operands[1]);
            int32_t uimm = 0;
            if (!parse_expression(operands[2], uimm, line) || uimm < 0 || uimm > 31) {
                add_error(line, "Invalid immediate (0-31) in " + mnem);
                return false;
            }
            uint32_t inst = desc->match | ((csr & 0xfff) << 20) | ((uimm & 0x1f) << 15) | ((rd & 0x1f) << 7);
            active_section().append_word(inst);
            return true;
        }

        case FMT_ATOMIC: {
            /* amoadd.w[.aq][.rl] rd, rs2, (rs1) */
            if (argc != 3 && strcmp(desc->mnemonic, "lr.w") != 0) {
                add_error(line, mnem + " requires 3 operands: rd, rs2, (rs1)");
                return false;
            }
            int rd = parse_reg(operands[0]);
            int rs2 = 0;
            int rs1 = -1;
            int32_t imm = 0;
            if (strcmp(desc->mnemonic, "lr.w") == 0) {
                if (!parse_mem_operand(operands[1], imm, rs1, line)) {
                    add_error(line, "Invalid memory operand in lr.w");
                    return false;
                }
            } else {
                rs2 = parse_reg(operands[1]);
                if (!parse_mem_operand(operands[2], imm, rs1, line)) {
                    add_error(line, "Invalid memory operand in atomic op");
                    return false;
                }
            }

            uint32_t inst = desc->match | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((rd & 0x1f) << 7);
            if (has_aq) inst |= (1 << 26);
            if (has_rl) inst |= (1 << 25);
            active_section().append_word(inst);
            return true;
        }

        case FMT_F_LOAD: {
            if (argc != 2) { add_error(line, "flw requires 2 operands: fd, imm(rs1)"); return false; }
            int fd = parse_freg(operands[0]);
            int32_t imm = 0;
            int rs1 = -1;
            if (fd < 0 || !parse_mem_operand(operands[1], imm, rs1, line)) {
                add_error(line, "Invalid operands in flw");
                return false;
            }
            active_section().append_word(encode_I(imm, rs1, 2, fd, 0x07));
            return true;
        }

        case FMT_F_STORE: {
            if (argc != 2) { add_error(line, "fsw requires 2 operands: fs2, imm(rs1)"); return false; }
            int fs2 = parse_freg(operands[0]);
            int32_t imm = 0;
            int rs1 = -1;
            if (fs2 < 0 || !parse_mem_operand(operands[1], imm, rs1, line)) {
                add_error(line, "Invalid operands in fsw");
                return false;
            }
            active_section().append_word(encode_S(imm, fs2, rs1, 2, 0x27));
            return true;
        }

        case FMT_F_R: {
            if (argc != 3) { add_error(line, mnem + " requires 3 float operands: fd, fs1, fs2"); return false; }
            int fd  = parse_freg(operands[0]);
            int fs1 = parse_freg(operands[1]);
            int fs2 = parse_freg(operands[2]);
            if (fd < 0 || fs1 < 0 || fs2 < 0) { add_error(line, "Invalid float register in " + mnem); return false; }
            unsigned f7 = (desc->match >> 25) & 0x7f;
            unsigned f3 = (desc->match >> 12) & 0x7;
            if (f7 == 0x00 || f7 == 0x04 || f7 == 0x08 || f7 == 0x0c || f7 == 0x2c) {
                f3 = 7; /* Default rounding mode: DYN (dynamic/fcsr) */
            }
            active_section().append_word(encode_R(f7, fs2, fs1, f3, fd, 0x53));
            return true;
        }

        case FMT_F_CVT_F2I: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: rd, fs1"); return false; }
            int rd = parse_reg(operands[0]);
            int fs1 = parse_freg(operands[1]);
            if (rd < 0 || fs1 < 0) { add_error(line, "Invalid registers in " + mnem); return false; }
            active_section().append_word(encode_R(0x60, 0, fs1, 7, rd, 0x53));
            return true;
        }

        case FMT_F_CVT_I2F: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: fd, rs1"); return false; }
            int fd = parse_freg(operands[0]);
            int rs1 = parse_reg(operands[1]);
            if (fd < 0 || rs1 < 0) { add_error(line, "Invalid registers in " + mnem); return false; }
            active_section().append_word(encode_R(0x68, 0, rs1, 7, fd, 0x53));
            return true;
        }

        case FMT_F_MV_F2I: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: rd, fs1"); return false; }
            int rd = parse_reg(operands[0]);
            int fs1 = parse_freg(operands[1]);
            if (rd < 0 || fs1 < 0) { add_error(line, "Invalid registers in " + mnem); return false; }
            active_section().append_word(encode_R(0x70, 0, fs1, 0, rd, 0x53));
            return true;
        }

        case FMT_F_MV_I2F: {
            if (argc != 2) { add_error(line, mnem + " requires 2 operands: fd, rs1"); return false; }
            int fd = parse_freg(operands[0]);
            int rs1 = parse_reg(operands[1]);
            if (fd < 0 || rs1 < 0) { add_error(line, "Invalid registers in " + mnem); return false; }
            active_section().append_word(encode_R(0x78, 0, rs1, 0, fd, 0x53));
            return true;
        }

        case FMT_F_CMP: {
            if (argc != 3) { add_error(line, mnem + " requires 3 operands: rd, fs1, fs2"); return false; }
            int rd = parse_reg(operands[0]);
            int fs1 = parse_freg(operands[1]);
            int fs2 = parse_freg(operands[2]);
            if (rd < 0 || fs1 < 0 || fs2 < 0) { add_error(line, "Invalid registers in " + mnem); return false; }
            unsigned f3 = (desc->match >> 12) & 0x7;
            active_section().append_word(encode_R(0x50, fs2, fs1, f3, rd, 0x53));
            return true;
        }

        default:
            add_error(line, "Unhandled instruction format in " + mnem);
            return false;
    }
}

/* =========================================================================
 * Pipeline Principal de Montagem (Duas Passagens)
 * ========================================================================= */

struct ParsedLine {
    int line_num;
    std::string label;
    std::string directive;
    std::string instruction;
    std::vector<std::string> args;
    std::string section_at_entry;
    uint32_t pc_at_entry;
};

bool Assembler::assemble_string(const std::string &source) {
    m_diagnostics.clear();
    m_symbols.clear();
    m_relocations.clear();
    m_sections.clear();
    m_active_section_idx = -1;

    set_active_section(".text", SEC_TEXT, 4);

    std::vector<ParsedLine> parsed_lines;
    std::istringstream stream(source);
    std::string line_str;
    int line_num = 0;

    /* ---------------- Passagem 1: Lexing de Linhas e Coleta de Labels ---------------- */

    while (std::getline(stream, line_str)) {
        line_num++;
        std::string raw = line_str;

        /* Remove comentários (# e //) */
        size_t c_pos = raw.find('#');
        if (c_pos != std::string::npos) raw = raw.substr(0, c_pos);
        c_pos = raw.find("//");
        if (c_pos != std::string::npos) raw = raw.substr(0, c_pos);

        raw = trim(raw);
        if (raw.empty()) continue;

        ParsedLine pl;
        pl.line_num = line_num;
        pl.section_at_entry = active_section().name;
        pl.pc_at_entry = (uint32_t)active_section().data.size();

        /* Verifica se há Label ("label:") ignorando colons dentro de aspas */
        bool in_str = false;
        size_t colon = std::string::npos;
        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] == '\"') in_str = !in_str;
            else if (raw[i] == ':' && !in_str) {
                colon = i;
                break;
            }
        }
        if (colon != std::string::npos) {
            pl.label = trim(raw.substr(0, colon));
            raw = trim(raw.substr(colon + 1));

            /* Registra o símbolo com o offset da seção ativa */
            Symbol sym;
            sym.name = pl.label;
            sym.section = active_section().name;
            sym.offset = (uint32_t)active_section().data.size();
            sym.is_defined = true;
            m_symbols[pl.label] = sym;
        }

        if (raw.empty()) {
            parsed_lines.push_back(pl);
            continue;
        }

        /* Verifica se é Diretiva (".text", ".data", ".word", etc.) */
        if (raw[0] == '.') {
            size_t sp = raw.find_first_of(" \t");
            pl.directive = (sp == std::string::npos) ? raw : raw.substr(0, sp);
            std::string args_str = (sp == std::string::npos) ? "" : trim(raw.substr(sp + 1));

            /* Separa argumentos por vírgula mantendo strings com aspas */
            std::string cur_arg;
            bool in_quotes = false;
            for (size_t i = 0; i < args_str.size(); i++) {
                char ch = args_str[i];
                if (ch == '\"') in_quotes = !in_quotes;
                if (ch == ',' && !in_quotes) {
                    pl.args.push_back(trim(cur_arg));
                    cur_arg.clear();
                } else {
                    cur_arg += ch;
                }
            }
            if (!cur_arg.empty()) pl.args.push_back(trim(cur_arg));

            /* Trata diretivas estruturais na Passagem 1 para manter o PC correto */
            std::string d = to_lower(pl.directive);
            if (d == ".text") {
                set_active_section(".text", SEC_TEXT, 4);
            } else if (d == ".data") {
                set_active_section(".data", SEC_DATA, 4);
            } else if (d == ".rodata") {
                set_active_section(".rodata", SEC_RODATA, 4);
            } else if (d == ".bss") {
                set_active_section(".bss", SEC_BSS, 4);
            } else if (d == ".section" && !pl.args.empty()) {
                std::string sname = pl.args[0];
                SectionType stype = SEC_DATA;
                if (sname == ".text") stype = SEC_TEXT;
                else if (sname == ".rodata") stype = SEC_RODATA;
                else if (sname == ".bss") stype = SEC_BSS;
                set_active_section(sname, stype, 4);
            } else if (d == ".align" && !pl.args.empty()) {
                int32_t p = 0;
                parse_expression(pl.args[0], p, line_num);
                uint32_t a = (p <= 4) ? (1 << p) : p; /* pot de 2 se pequeno */
                active_section().align_to(a);
            } else if (d == ".word") {
                for (size_t a = 0; a < pl.args.size(); a++) active_section().append_word(0);
            } else if (d == ".half") {
                for (size_t a = 0; a < pl.args.size(); a++) active_section().append_half(0);
            } else if (d == ".byte") {
                for (size_t a = 0; a < pl.args.size(); a++) active_section().append_byte(0);
            } else if (d == ".asciz" || d == ".string") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    std::string str_val = pl.args[a];
                    if (str_val.size() >= 2 && str_val.front() == '\"' && str_val.back() == '\"') {
                        str_val = str_val.substr(1, str_val.size() - 2);
                    }
                    for (size_t c = 0; c < str_val.size(); c++) {
                        if (str_val[c] == '\\' && c + 1 < str_val.size()) {
                            c++;
                            if (str_val[c] == 'n') active_section().append_byte('\n');
                            else if (str_val[c] == 'r') active_section().append_byte('\r');
                            else if (str_val[c] == 't') active_section().append_byte('\t');
                            else if (str_val[c] == '0') active_section().append_byte('\0');
                            else active_section().append_byte((uint8_t)str_val[c]);
                        } else {
                            active_section().append_byte((uint8_t)str_val[c]);
                        }
                    }
                    active_section().append_byte(0);
                }
            } else if (d == ".ascii") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    std::string str_val = pl.args[a];
                    if (str_val.size() >= 2 && str_val.front() == '\"' && str_val.back() == '\"') {
                        str_val = str_val.substr(1, str_val.size() - 2);
                    }
                    for (size_t c = 0; c < str_val.size(); c++) {
                        if (str_val[c] == '\\' && c + 1 < str_val.size()) {
                            c++;
                            if (str_val[c] == 'n') active_section().append_byte('\n');
                            else if (str_val[c] == 'r') active_section().append_byte('\r');
                            else if (str_val[c] == 't') active_section().append_byte('\t');
                            else if (str_val[c] == '0') active_section().append_byte('\0');
                            else active_section().append_byte((uint8_t)str_val[c]);
                        } else {
                            active_section().append_byte((uint8_t)str_val[c]);
                        }
                    }
                }
            } else if (d == ".equ" || d == ".set") {
                if (pl.args.size() >= 2) {
                    int32_t cval = 0;
                    parse_expression(pl.args[1], cval, line_num);
                    Symbol sym;
                    sym.name = pl.args[0];
                    sym.is_constant = true;
                    sym.constant_value = cval;
                    m_symbols[pl.args[0]] = sym;
                }
            }
        } else {
            /* Instrução: identifica mnemônico e argumentos */
            size_t sp = raw.find_first_of(" \t");
            pl.instruction = (sp == std::string::npos) ? raw : raw.substr(0, sp);
            std::string args_str = (sp == std::string::npos) ? "" : trim(raw.substr(sp + 1));

            std::string cur_arg;
            for (size_t i = 0; i < args_str.size(); i++) {
                char ch = args_str[i];
                if (ch == ',') {
                    pl.args.push_back(trim(cur_arg));
                    cur_arg.clear();
                } else {
                    cur_arg += ch;
                }
            }
            if (!cur_arg.empty()) pl.args.push_back(trim(cur_arg));

            /* Reserva espaço no PC da seção (4 bytes por instrução, ou 8 bytes para li 32-bit / la) */
            active_section().append_word(0);
            if (to_lower(pl.instruction) == "li" && pl.args.size() == 2) {
                int32_t val = 0;
                if (parse_expression(pl.args[1], val, line_num) && (val < -2048 || val > 2047)) {
                    active_section().append_word(0); /* li 32-bit precisa de 8 bytes */
                }
            } else if (to_lower(pl.instruction) == "la" && pl.args.size() == 2) {
                active_section().append_word(0); /* la precisa de 8 bytes (auipc + addi) */
            } else if (to_lower(pl.instruction) == "call" && pl.args.size() == 1) {
                active_section().append_word(0); /* call precisa de 8 bytes (auipc + jalr) */
            }
        }

        parsed_lines.push_back(pl);
    }

    /* ---------------- Passagem 2: Emissão Real dos Bytes e Opcodes ---------------- */

    /* Calcula as bases acumuladas de cada seção com seus alinhamentos */
    m_section_bases.clear();
    uint32_t cumulative_offset = 0;
    for (size_t s = 0; s < m_sections.size(); s++) {
        uint32_t align = m_sections[s].align > 0 ? m_sections[s].align : 4;
        cumulative_offset = (cumulative_offset + align - 1) & ~(align - 1);
        m_section_bases[m_sections[s].name] = cumulative_offset;
        cumulative_offset += (uint32_t)m_sections[s].data.size();
    }

    /* Limpa os dados das seções para reconstruí-los com dados reais */
    for (size_t s = 0; s < m_sections.size(); s++) {
        m_sections[s].data.clear();
    }
    m_active_section_idx = -1;

    for (size_t i = 0; i < parsed_lines.size(); i++) {
        const ParsedLine &pl = parsed_lines[i];

        if (!pl.directive.empty()) {
            std::string d = to_lower(pl.directive);
            if (d == ".text") {
                set_active_section(".text", SEC_TEXT, 4);
            } else if (d == ".data") {
                set_active_section(".data", SEC_DATA, 4);
            } else if (d == ".rodata") {
                set_active_section(".rodata", SEC_RODATA, 4);
            } else if (d == ".bss") {
                set_active_section(".bss", SEC_BSS, 4);
            } else if (d == ".section" && !pl.args.empty()) {
                std::string sname = pl.args[0];
                SectionType stype = SEC_DATA;
                if (sname == ".text") stype = SEC_TEXT;
                else if (sname == ".rodata") stype = SEC_RODATA;
                else if (sname == ".bss") stype = SEC_BSS;
                set_active_section(sname, stype, 4);
            } else if (d == ".align" && !pl.args.empty()) {
                int32_t p = 0;
                parse_expression(pl.args[0], p, pl.line_num);
                uint32_t a = (p <= 4) ? (1 << p) : p;
                active_section().align_to(a);
            } else if (d == ".word") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    int32_t w = 0;
                    parse_expression(pl.args[a], w, pl.line_num);
                    active_section().append_word((uint32_t)w);
                }
            } else if (d == ".half") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    int32_t h = 0;
                    parse_expression(pl.args[a], h, pl.line_num);
                    active_section().append_half((uint16_t)h);
                }
            } else if (d == ".byte") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    int32_t b = 0;
                    parse_expression(pl.args[a], b, pl.line_num);
                    active_section().append_byte((uint8_t)b);
                }
            } else if (d == ".asciz" || d == ".string") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    std::string str_val = pl.args[a];
                    if (str_val.size() >= 2 && str_val.front() == '\"' && str_val.back() == '\"') {
                        str_val = str_val.substr(1, str_val.size() - 2);
                    }
                    /* Interpreta escapes comuns \n, \r, \t, \0 */
                    for (size_t c = 0; c < str_val.size(); c++) {
                        if (str_val[c] == '\\' && c + 1 < str_val.size()) {
                            c++;
                            if (str_val[c] == 'n') active_section().append_byte('\n');
                            else if (str_val[c] == 'r') active_section().append_byte('\r');
                            else if (str_val[c] == 't') active_section().append_byte('\t');
                            else if (str_val[c] == '0') active_section().append_byte('\0');
                            else active_section().append_byte((uint8_t)str_val[c]);
                        } else {
                            active_section().append_byte((uint8_t)str_val[c]);
                        }
                    }
                    active_section().append_byte(0);
                }
            } else if (d == ".ascii") {
                for (size_t a = 0; a < pl.args.size(); a++) {
                    std::string str_val = pl.args[a];
                    if (str_val.size() >= 2 && str_val.front() == '\"' && str_val.back() == '\"') {
                        str_val = str_val.substr(1, str_val.size() - 2);
                    }
                    for (size_t c = 0; c < str_val.size(); c++) active_section().append_byte((uint8_t)str_val[c]);
                }
            } else if (d == ".globl" || d == ".global") {
                if (!pl.args.empty()) {
                    m_symbols[pl.args[0]].name = pl.args[0];
                    m_symbols[pl.args[0]].is_global = true;
                }
            }
        } else if (!pl.instruction.empty()) {
            uint32_t cur_pc = (uint32_t)active_section().data.size();
            assemble_instruction(pl.instruction, pl.args, cur_pc, pl.line_num);
        }
    }

    return !has_errors();
}

} // namespace riscv
} // namespace maia
