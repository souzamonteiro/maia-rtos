#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EI_NIDENT 16

/* Standard ELF Identification indices */
#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_ABIVERSION 8

/* Expected values for MNM-1 */
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EV_CURRENT 1

#define ET_REL 1
#define EM_RISCV 243

/* Section header types */
#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_NOBITS   8
#define SHT_REL      9

/* Section header flags */
#define SHF_WRITE     0x1
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4

typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} Elf32_Shdr;

typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} Elf32_Sym;

/* Validation result codes */
#define ELF_VALID_OK               0
#define ELF_ERR_NULL_PTR          -1
#define ELF_ERR_TOO_SMALL         -2
#define ELF_ERR_BAD_MAGIC         -3
#define ELF_ERR_NOT_32BIT         -4
#define ELF_ERR_NOT_LITTLE_ENDIAN -5
#define ELF_ERR_BAD_VERSION       -6
#define ELF_ERR_NOT_ET_REL        -7
#define ELF_ERR_NOT_RISCV         -8
#define ELF_ERR_BAD_SHOFF         -9
#define ELF_ERR_BAD_SHSTRNDX      -10

/**
 * @brief Strictly validate an in-memory buffer as a trusted RISC-V ELF32 ET_REL object.
 *
 * @param data Pointer to buffer containing ELF binary.
 * @param size Buffer size in bytes.
 * @param report Optional buffer where human-readable inspection report is written.
 * @param report_max Size of report buffer.
 * @return 0 on success, negative error code on failure.
 */
int elf_validate_relocatable(const void *data, size_t size, char *report, size_t report_max);

#ifdef __cplusplus
}
#endif

