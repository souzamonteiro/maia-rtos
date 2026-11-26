#include <maia/exec/elf_validator.h>
#include <stdio.h>
#include <string.h>

enum {
    SOURCE_READ_OK = 0,
    SOURCE_READ_RANGE = -1,
    SOURCE_READ_IO = -2
};

static void set_report(char *report, size_t report_max, const char *message) {
    if (report != NULL && report_max > 0U) {
        snprintf(report, report_max, "%s", message);
    }
}

static int source_read(const elf_reader_t *reader, size_t offset,
                       void *buffer, size_t length) {
    if (reader == NULL || reader->read_at == NULL ||
        (length > 0U && buffer == NULL) ||
        offset > reader->size || length > reader->size - offset) {
        return SOURCE_READ_RANGE;
    }
    if (length == 0U) {
        return SOURCE_READ_OK;
    }
    return reader->read_at(reader->context, offset, buffer, length) == 0
        ? SOURCE_READ_OK : SOURCE_READ_IO;
}

static int read_section(const elf_reader_t *reader, const Elf32_Ehdr *ehdr,
                        uint16_t index, Elf32_Shdr *section) {
    if (index >= ehdr->e_shnum ||
        ehdr->e_shentsize < sizeof(Elf32_Shdr)) {
        return SOURCE_READ_RANGE;
    }
    uint64_t offset = (uint64_t)ehdr->e_shoff +
                      (uint64_t)index * ehdr->e_shentsize;
    if (offset > SIZE_MAX) {
        return SOURCE_READ_RANGE;
    }
    return source_read(reader, (size_t)offset, section, sizeof(*section));
}

static int string_terminated(const elf_reader_t *reader,
                             const Elf32_Shdr *string_table,
                             uint32_t name_offset) {
    if (name_offset >= string_table->sh_size) {
        return 0;
    }

    size_t cursor = name_offset;
    char chunk[32];
    while (cursor < string_table->sh_size) {
        size_t remaining = (size_t)string_table->sh_size - cursor;
        size_t length = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        size_t source_offset = (size_t)string_table->sh_offset + cursor;
        int result = source_read(reader, source_offset, chunk, length);
        if (result != SOURCE_READ_OK) {
            return result == SOURCE_READ_IO ? -1 : 0;
        }
        if (memchr(chunk, '\0', length) != NULL) {
            return 1;
        }
        cursor += length;
    }
    return 0;
}

static int string_equals(const elf_reader_t *reader,
                         const Elf32_Shdr *string_table,
                         uint32_t name_offset, const char *expected) {
    size_t expected_length = strlen(expected);
    size_t available;
    char actual[32];

    if (name_offset >= string_table->sh_size ||
        expected_length + 1U > sizeof(actual)) {
        return 0;
    }
    available = (size_t)string_table->sh_size - name_offset;
    if (expected_length + 1U > available) {
        return 0;
    }

    size_t source_offset = (size_t)string_table->sh_offset + name_offset;
    int result = source_read(reader, source_offset, actual,
                             expected_length + 1U);
    if (result == SOURCE_READ_IO) {
        return -1;
    }
    if (result != SOURCE_READ_OK) {
        return 0;
    }
    return memcmp(actual, expected, expected_length + 1U) == 0;
}

static int memory_read_at(void *context, size_t offset,
                          void *buffer, size_t length) {
    memcpy(buffer, (const uint8_t *)context + offset, length);
    return 0;
}

int elf_validate_relocatable_reader(const elf_reader_t *reader,
                                    char *report, size_t report_max) {
    if (reader == NULL || reader->read_at == NULL) {
        set_report(report, report_max, "ELF Error: NULL reader");
        return ELF_ERR_NULL_PTR;
    }
    if (reader->size < sizeof(Elf32_Ehdr)) {
        if (report != NULL && report_max > 0U) {
            snprintf(report, report_max,
                     "ELF Error: Buffer too small (%u < %u bytes)",
                     (unsigned int)reader->size,
                     (unsigned int)sizeof(Elf32_Ehdr));
        }
        return ELF_ERR_TOO_SMALL;
    }

    Elf32_Ehdr ehdr;
    int read_result = source_read(reader, 0U, &ehdr, sizeof(ehdr));
    if (read_result != SOURCE_READ_OK) {
        set_report(report, report_max, "ELF Error: Unable to read ELF header");
        return read_result == SOURCE_READ_IO ? ELF_ERR_IO : ELF_ERR_TOO_SMALL;
    }

    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        set_report(report, report_max,
                   "ELF Error: Bad magic byte (not 7f 45 4c 46)");
        return ELF_ERR_BAD_MAGIC;
    }
    if (ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
        if (report != NULL && report_max > 0U) {
            snprintf(report, report_max, "ELF Error: Not ELF32 (class=%d)",
                     ehdr.e_ident[EI_CLASS]);
        }
        return ELF_ERR_NOT_32BIT;
    }
    if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB) {
        set_report(report, report_max,
                   "ELF Error: Not 2's complement little-endian");
        return ELF_ERR_NOT_LITTLE_ENDIAN;
    }
    if (ehdr.e_type != ET_REL) {
        if (report != NULL && report_max > 0U) {
            snprintf(report, report_max,
                     "ELF Error: Type %u is not ET_REL (1)", ehdr.e_type);
        }
        return ELF_ERR_NOT_ET_REL;
    }
    if (ehdr.e_machine != EM_RISCV) {
        if (report != NULL && report_max > 0U) {
            snprintf(report, report_max,
                     "ELF Error: Machine %u is not EM_RISCV (243)",
                     ehdr.e_machine);
        }
        return ELF_ERR_NOT_RISCV;
    }

    Elf32_Shdr string_table = {0};
    if (ehdr.e_shnum > 0U) {
        if (ehdr.e_shentsize < sizeof(Elf32_Shdr)) {
            set_report(report, report_max,
                       "ELF Error: Invalid section header entry size");
            return ELF_ERR_BAD_SHOFF;
        }
        uint64_t table_bytes = (uint64_t)ehdr.e_shnum * ehdr.e_shentsize;
        if ((uint64_t)ehdr.e_shoff > reader->size ||
            table_bytes > (uint64_t)reader->size - ehdr.e_shoff) {
            if (report != NULL && report_max > 0U) {
                snprintf(report, report_max,
                         "ELF Error: Section header table extends beyond buffer");
            }
            return ELF_ERR_BAD_SHOFF;
        }
        if (ehdr.e_shstrndx >= ehdr.e_shnum) {
            if (report != NULL && report_max > 0U) {
                snprintf(report, report_max,
                         "ELF Error: shstrndx (%u) >= shnum (%u)",
                         ehdr.e_shstrndx, ehdr.e_shnum);
            }
            return ELF_ERR_BAD_SHSTRNDX;
        }
        read_result = read_section(reader, &ehdr, ehdr.e_shstrndx,
                                   &string_table);
        if (read_result != SOURCE_READ_OK) {
            set_report(report, report_max,
                       "ELF Error: Unable to read section-name table header");
            return read_result == SOURCE_READ_IO ? ELF_ERR_IO : ELF_ERR_BAD_SHOFF;
        }
        if (string_table.sh_type != SHT_STRTAB ||
            (uint64_t)string_table.sh_offset > reader->size ||
            string_table.sh_size >
                (uint64_t)reader->size - string_table.sh_offset) {
            set_report(report, report_max,
                       "ELF Error: Invalid section-name string table");
            return ELF_ERR_BAD_SHOFF;
        }
    }

    size_t text_size = 0U;
    size_t rodata_size = 0U;
    size_t data_size = 0U;
    size_t bss_size = 0U;
    int allocatable_sections = 0;

    for (uint16_t i = 0U; i < ehdr.e_shnum; ++i) {
        Elf32_Shdr section;
        read_result = read_section(reader, &ehdr, i, &section);
        if (read_result != SOURCE_READ_OK) {
            set_report(report, report_max,
                       "ELF Error: Unable to read section header");
            return read_result == SOURCE_READ_IO ? ELF_ERR_IO : ELF_ERR_BAD_SHOFF;
        }

        if (section.sh_type != SHT_NOBITS && section.sh_size > 0U &&
            ((uint64_t)section.sh_offset > reader->size ||
             section.sh_size >
                 (uint64_t)reader->size - section.sh_offset)) {
            if (report != NULL && report_max > 0U) {
                snprintf(report, report_max,
                         "ELF Error: Section %u extends beyond buffer", i);
            }
            return ELF_ERR_BAD_SHOFF;
        }

        int name_valid = string_terminated(reader, &string_table,
                                           section.sh_name);
        if (name_valid <= 0) {
            set_report(report, report_max,
                       name_valid < 0
                           ? "ELF Error: Unable to read section name"
                           : "ELF Error: Invalid section name");
            return name_valid < 0 ? ELF_ERR_IO : ELF_ERR_BAD_SHOFF;
        }

        if (section.sh_flags & SHF_ALLOC) {
            ++allocatable_sections;
            int matches = string_equals(reader, &string_table,
                                        section.sh_name, ".text");
            if (matches < 0) {
                set_report(report, report_max,
                           "ELF Error: Unable to read section name");
                return ELF_ERR_IO;
            }
            if (matches) {
                text_size = section.sh_size;
                continue;
            }
            matches = string_equals(reader, &string_table,
                                    section.sh_name, ".rodata");
            if (matches < 0) {
                set_report(report, report_max,
                           "ELF Error: Unable to read section name");
                return ELF_ERR_IO;
            }
            if (matches) {
                rodata_size = section.sh_size;
                continue;
            }
            matches = string_equals(reader, &string_table,
                                    section.sh_name, ".data");
            if (matches < 0) {
                set_report(report, report_max,
                           "ELF Error: Unable to read section name");
                return ELF_ERR_IO;
            }
            if (matches) {
                data_size = section.sh_size;
                continue;
            }
            matches = string_equals(reader, &string_table,
                                    section.sh_name, ".bss");
            if (matches < 0) {
                set_report(report, report_max,
                           "ELF Error: Unable to read section name");
                return ELF_ERR_IO;
            }
            if (matches) {
                bss_size = section.sh_size;
            }
        }
    }

    if (report != NULL && report_max > 0U) {
        snprintf(report, report_max,
                 "ELF32 RISC-V Relocatable Object (ET_REL):\n"
                 "  Arch:    RISC-V (RV32) Little-Endian\n"
                 "  Sections: %u total (%d allocatable)\n"
                 "  Sizes:   .text=%u B, .rodata=%u B, .data=%u B, .bss=%u B\n"
                 "  Status:  VALID MNM-1 Object Candidate",
                 ehdr.e_shnum, allocatable_sections,
                 (unsigned int)text_size, (unsigned int)rodata_size,
                 (unsigned int)data_size, (unsigned int)bss_size);
    }
    return ELF_VALID_OK;
}

int elf_validate_relocatable(const void *data, size_t size,
                             char *report, size_t report_max) {
    if (data == NULL) {
        set_report(report, report_max, "ELF Error: NULL buffer pointer");
        return ELF_ERR_NULL_PTR;
    }
    elf_reader_t reader = {
        .read_at = memory_read_at,
        .context = (void *)data,
        .size = size
    };
    return elf_validate_relocatable_reader(&reader, report, report_max);
}
