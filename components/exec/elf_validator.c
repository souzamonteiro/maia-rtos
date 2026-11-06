#include <maia/exec/elf_validator.h>
#include <string.h>
#include <stdio.h>

int elf_validate_relocatable(const void *data, size_t size, char *report, size_t report_max)
{
    if (!data) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: NULL buffer pointer");
        return ELF_ERR_NULL_PTR;
    }

    if (size < sizeof(Elf32_Ehdr)) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Buffer too small (%u < %u bytes)", (unsigned int)size, (unsigned int)sizeof(Elf32_Ehdr));
        return ELF_ERR_TOO_SMALL;
    }

    const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)data;

    // 1. Magic verification
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Bad magic byte (not 7f 45 4c 46)");
        return ELF_ERR_BAD_MAGIC;
    }

    // 2. Class: 32-bit only
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Not ELF32 (class=%d)", ehdr->e_ident[EI_CLASS]);
        return ELF_ERR_NOT_32BIT;
    }

    // 3. Data encoding: Little-Endian only
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Not 2's complement little-endian");
        return ELF_ERR_NOT_LITTLE_ENDIAN;
    }

    // 4. Object type: ET_REL (1) only
    if (ehdr->e_type != ET_REL) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Type %u is not ET_REL (1)", ehdr->e_type);
        return ELF_ERR_NOT_ET_REL;
    }

    // 5. Machine architecture: EM_RISCV (243)
    if (ehdr->e_machine != EM_RISCV) {
        if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Machine %u is not EM_RISCV (243)", ehdr->e_machine);
        return ELF_ERR_NOT_RISCV;
    }

    // 6. Section header table bounds checks
    if (ehdr->e_shnum > 0) {
        size_t sh_table_end = (size_t)ehdr->e_shoff + ((size_t)ehdr->e_shnum * (size_t)ehdr->e_shentsize);
        if (sh_table_end > size || ehdr->e_shentsize < sizeof(Elf32_Shdr)) {
            if (report && report_max > 0) snprintf(report, report_max, "ELF Error: Section header table extends beyond buffer (%u bytes)", (unsigned int)sh_table_end);
            return ELF_ERR_BAD_SHOFF;
        }

        if (ehdr->e_shstrndx >= ehdr->e_shnum) {
            if (report && report_max > 0) snprintf(report, report_max, "ELF Error: shstrndx (%u) >= shnum (%u)", ehdr->e_shstrndx, ehdr->e_shnum);
            return ELF_ERR_BAD_SHSTRNDX;
        }
    }

    // 7. Parse sections and collect statistics
    const Elf32_Shdr *shdrs = (const Elf32_Shdr *)((const uint8_t *)data + ehdr->e_shoff);
    const char *shstrtab = "";
    if (ehdr->e_shnum > 0 && ehdr->e_shstrndx < ehdr->e_shnum) {
        const Elf32_Shdr *strtab_hdr = &shdrs[ehdr->e_shstrndx];
        if (strtab_hdr->sh_offset + strtab_hdr->sh_size <= size) {
            shstrtab = (const char *)data + strtab_hdr->sh_offset;
        }
    }

    size_t text_sz = 0, rodata_sz = 0, data_sz = 0, bss_sz = 0;
    int section_count = 0;

    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const Elf32_Shdr *sh = &shdrs[i];
        const char *name = shstrtab + sh->sh_name;
        if (sh->sh_flags & SHF_ALLOC) {
            section_count++;
            if (strcmp(name, ".text") == 0) text_sz = sh->sh_size;
            else if (strcmp(name, ".rodata") == 0) rodata_sz = sh->sh_size;
            else if (strcmp(name, ".data") == 0) data_sz = sh->sh_size;
            else if (strcmp(name, ".bss") == 0) bss_sz = sh->sh_size;
        }
    }

    if (report && report_max > 0) {
        snprintf(report, report_max,
                 "ELF32 RISC-V Relocatable Object (ET_REL):\n"
                 "  Arch:    RISC-V (RV32) Little-Endian\n"
                 "  Sections: %u total (%d allocatable)\n"
                 "  Sizes:   .text=%u B, .rodata=%u B, .data=%u B, .bss=%u B\n"
                 "  Status:  VALID MNM-1 Object Candidate",
                 ehdr->e_shnum, section_count,
                 (unsigned int)text_sz, (unsigned int)rodata_sz,
                 (unsigned int)data_sz, (unsigned int)bss_sz);
    }

    return ELF_VALID_OK;
}
