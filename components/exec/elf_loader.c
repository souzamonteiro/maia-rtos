#include <maia/exec/elf_loader.h>
#include <string.h>
#include <stdio.h>

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#else
#include <stdlib.h>
#endif

static inline uint32_t get_u32(const void *p) {
    uint32_t val;
    memcpy(&val, p, 4);
    return val;
}

static inline void set_u32(void *p, uint32_t val) {
    memcpy(p, &val, 4);
}

static inline uint16_t get_u16(const void *p) {
    uint16_t val;
    memcpy(&val, p, 2);
    return val;
}

static inline void set_u16(void *p, uint16_t val) {
    memcpy(p, &val, 2);
}

int mnm_load_and_run(const void *elf_data, size_t elf_size,
                     int argc, const char *const *argv,
                     const maia_host_api_v1 *host,
                     int *exit_code,
                     void (*log_fn)(const char *msg))
{
    char rep[256];
    int vres = elf_validate_relocatable(elf_data, elf_size, rep, sizeof(rep));
    if (vres != ELF_VALID_OK) {
        if (log_fn) log_fn(rep);
        return MNM_ERR_VALIDATION_FAILED;
    }

    const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)elf_data;
    const Elf32_Shdr *shdrs = (const Elf32_Shdr *)((const uint8_t *)elf_data + ehdr->e_shoff);

    // 1. Calculate size and offsets for all allocatable sections
    size_t total_alloc = 0;
    size_t sec_offsets[64] = {0};
    if (ehdr->e_shnum > 64) return MNM_ERR_OUT_OF_MEMORY;

    int text_sec_idx = -1;

    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const Elf32_Shdr *sh = &shdrs[i];
        if (sh->sh_flags & SHF_ALLOC) {
            size_t align = sh->sh_addralign > 0 ? sh->sh_addralign : 4;
            total_alloc = (total_alloc + align - 1) & ~(align - 1);
            sec_offsets[i] = total_alloc;
            total_alloc += sh->sh_size;

            if (sh->sh_flags & SHF_EXECINSTR) {
                text_sec_idx = (int)i;
            }
        }
    }

    if (text_sec_idx < 0) {
        if (log_fn) log_fn("Loader Error: No executable .text section found");
        return MNM_ERR_NO_TEXT_SECTION;
    }

    // Add extra padding to guarantee safety
    total_alloc = (total_alloc + 15) & ~15;

    // 2. Allocate executable memory region in Internal L2 SRAM
    uint8_t *module_base = NULL;
#if defined(ESP_PLATFORM)
    module_base = (uint8_t *)heap_caps_aligned_alloc(16, total_alloc, MALLOC_CAP_INTERNAL | MALLOC_CAP_EXEC);
    if (!module_base) {
        module_base = (uint8_t *)heap_caps_aligned_alloc(16, total_alloc, MALLOC_CAP_INTERNAL);
    }
#else
    module_base = (uint8_t *)malloc(total_alloc);
#endif

    if (!module_base) {
        if (log_fn) log_fn("Loader Error: Out of executable memory");
        return MNM_ERR_OUT_OF_MEMORY;
    }

    memset(module_base, 0, total_alloc);

    if (log_fn) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Loader: Allocated %u bytes at %p", (unsigned int)total_alloc, module_base);
        log_fn(msg);
    }

    // 3. Copy section data into loaded image
    uintptr_t sec_addrs[64] = {0};
    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const Elf32_Shdr *sh = &shdrs[i];
        if (sh->sh_flags & SHF_ALLOC) {
            sec_addrs[i] = (uintptr_t)(module_base + sec_offsets[i]);
            if (sh->sh_type != SHT_NOBITS && sh->sh_size > 0) {
                memcpy((void *)sec_addrs[i], (const uint8_t *)elf_data + sh->sh_offset, sh->sh_size);
            }
        }
    }

    // 4. Locate symbol table and find entry point
    const Elf32_Sym *symtab = NULL;
    size_t sym_count = 0;
    const char *strtab = NULL;

    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const Elf32_Shdr *sh = &shdrs[i];
        if (sh->sh_type == SHT_SYMTAB) {
            symtab = (const Elf32_Sym *)((const uint8_t *)elf_data + sh->sh_offset);
            sym_count = sh->sh_size / sizeof(Elf32_Sym);
            if (sh->sh_link < ehdr->e_shnum) {
                strtab = (const char *)elf_data + shdrs[sh->sh_link].sh_offset;
            }
            break;
        }
    }

    maia_module_entry_fn entry_point = NULL;

    if (symtab && strtab) {
        for (size_t s = 0; s < sym_count; ++s) {
            const Elf32_Sym *sym = &symtab[s];
            const char *name = strtab + sym->st_name;
            if (sym->st_shndx > 0 && sym->st_shndx < ehdr->e_shnum) {
                if (strcmp(name, "maia_module_main") == 0) {
                    entry_point = (maia_module_entry_fn)(sec_addrs[sym->st_shndx] + sym->st_value);
                }
            }
        }
    }

    if (!entry_point) {
        if (log_fn) log_fn("Loader Error: Symbol 'maia_module_main' not found");
        free(module_base);
        return MNM_ERR_NO_ENTRY_POINT;
    }

    // 5. Apply relocations with unaligned-safe helpers
    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const Elf32_Shdr *sh = &shdrs[i];
        if (sh->sh_type == SHT_RELA) {
            uint16_t target_sec_idx = (uint16_t)sh->sh_info;
            if (target_sec_idx >= ehdr->e_shnum || sec_addrs[target_sec_idx] == 0) continue;

            uint8_t *target_sec = (uint8_t *)sec_addrs[target_sec_idx];
            const Elf32_Rela *relas = (const Elf32_Rela *)((const uint8_t *)elf_data + sh->sh_offset);
            size_t num_relas = sh->sh_size / sizeof(Elf32_Rela);

            for (size_t r = 0; r < num_relas; ++r) {
                uint32_t r_offset = relas[r].r_offset;
                uint32_t r_sym = ELF32_R_SYM(relas[r].r_info);
                uint32_t r_type = ELF32_R_TYPE(relas[r].r_info);
                int32_t r_addend = relas[r].r_addend;

                uintptr_t sym_val = 0;
                if (r_sym < sym_count && symtab) {
                    const Elf32_Sym *sym = &symtab[r_sym];
                    if (sym->st_shndx > 0 && sym->st_shndx < ehdr->e_shnum) {
                        sym_val = sec_addrs[sym->st_shndx] + sym->st_value;
                    }
                }

                uint8_t *loc = target_sec + r_offset;
                uintptr_t val = sym_val + r_addend;

                switch (r_type) {
                    case R_RISCV_NONE:
                    case R_RISCV_RELAX:
                        break;

                    case R_RISCV_32: {
                        set_u32(loc, (uint32_t)val);
                        break;
                    }

                    case R_RISCV_HI20: {
                        uint32_t imm = (uint32_t)((val + 0x800) >> 12);
                        uint32_t insn = get_u32(loc);
                        insn = (insn & 0x00000FFF) | (imm << 12);
                        set_u32(loc, insn);
                        break;
                    }

                    case R_RISCV_LO12_I: {
                        uint32_t imm = (uint32_t)(val & 0xFFF);
                        uint32_t insn = get_u32(loc);
                        insn = (insn & 0x000FFFFF) | (imm << 20);
                        set_u32(loc, insn);
                        break;
                    }

                    case R_RISCV_LO12_S: {
                        uint32_t imm = (uint32_t)(val & 0xFFF);
                        uint32_t insn = get_u32(loc);
                        insn = (insn & 0x01FFF07F) | ((imm & 0xFE0) << 20) | ((imm & 0x01F) << 7);
                        set_u32(loc, insn);
                        break;
                    }

                    case R_RISCV_CALL:
                    case R_RISCV_CALL_PLT: {
                        int32_t off = (int32_t)(val - (uintptr_t)loc);
                        uint32_t hi = (uint32_t)((off + 0x800) >> 12);
                        uint32_t lo = (uint32_t)(off & 0xFFF);
                        uint32_t insn0 = get_u32(loc);
                        uint32_t insn1 = get_u32(loc + 4);
                        insn0 = (insn0 & 0x00000FFF) | (hi << 12);
                        insn1 = (insn1 & 0x000FFFFF) | (lo << 20);
                        set_u32(loc, insn0);
                        set_u32(loc + 4, insn1);
                        break;
                    }

                    case R_RISCV_BRANCH: {
                        int32_t off = (int32_t)(val - (uintptr_t)loc);
                        uint32_t insn = get_u32(loc);
                        insn = (insn & ~0xFE000F80) |
                               (((off >> 12) & 0x1) << 31) |
                               (((off >> 5) & 0x3F) << 25) |
                               (((off >> 1) & 0xF) << 8) |
                               (((off >> 11) & 0x1) << 7);
                        set_u32(loc, insn);
                        break;
                    }

                    case R_RISCV_RVC_BRANCH: {
                        int32_t off = (int32_t)(val - (uintptr_t)loc);
                        uint16_t insn = get_u16(loc);
                        insn = (insn & ~0x1C7C) |
                               (((off >> 8) & 0x1) << 12) |
                               (((off >> 3) & 0x3) << 10) |
                               (((off >> 6) & 0x3) << 5) |
                               (((off >> 1) & 0x3) << 3) |
                               (((off >> 5) & 0x1) << 2);
                        set_u16(loc, insn);
                        break;
                    }

                    default:
                        if (log_fn) {
                            char warn[80];
                            snprintf(warn, sizeof(warn), "Loader Warning: Ignored relocation %u", (unsigned int)r_type);
                            log_fn(warn);
                        }
                        break;
                }
            }
        }
    }

    // 6. Synchronize instruction cache barrier
#if defined(__riscv)
    asm volatile("fence.i" ::: "memory");
#endif

    if (log_fn) {
        log_fn("Loader: Relocations applied & fence.i synchronized.");
    }

    // 7. Prepare module context and execute
    maia_module_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.struct_size = sizeof(ctx);
    ctx.argc = (uint32_t)argc;
    ctx.argv = argv;
    ctx.host = host;
    ctx.stdin_fd = 0;
    ctx.stdout_fd = 1;
    ctx.stderr_fd = 2;

    if (log_fn) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Loader: Executing maia_module_main at %p...", (void *)entry_point);
        log_fn(msg);
    }

    int ret = entry_point(&ctx);

    if (log_fn) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Loader: Module returned %d (0x%X)", ret, ret);
        log_fn(msg);
    }

    if (exit_code) {
        *exit_code = ret;
    }

    // 8. Cleanup and free loaded executable memory
    free(module_base);

    return MNM_LOAD_OK;
}
