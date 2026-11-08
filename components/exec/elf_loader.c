#include <maia/exec/elf_loader.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#include <esp_cache.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

// On ESP32-P4, instruction execution is strictly limited to sram_low (0x4FF00000 - 0x4FF3AFC0).
// sram_high (0x4FF40000 - 0x4FF80000) is rw-only and triggers an Illegal instruction abort on fetch.
// We maintain a dedicated 64 KB execution pool in .iram0.data with LIFO stack allocation
// to support nested execution (e.g. Desktop -> App -> Desktop) with zero fragmentation.
static uint8_t s_module_exec_pool[65536] __attribute__((aligned(64), section(".iram.data")));
static size_t s_module_pool_offset = 0;
#else
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#endif

typedef struct {
    char name[48];
    uintptr_t addr;
} maia_dyn_sym_t;

#define MAX_DYN_SYMS 64
static maia_dyn_sym_t s_dyn_syms[MAX_DYN_SYMS];
static int s_dyn_sym_count = 0;

int mnm_register_symbol(const char *name, uintptr_t address) {
    if (!name || !*name || !address) return -1;
    for (int i = 0; i < s_dyn_sym_count; ++i) {
        if (strcmp(s_dyn_syms[i].name, name) == 0) {
            s_dyn_syms[i].addr = address;
            return 0;
        }
    }
    if (s_dyn_sym_count >= MAX_DYN_SYMS) return -2;
    strncpy(s_dyn_syms[s_dyn_sym_count].name, name, sizeof(s_dyn_syms[s_dyn_sym_count].name) - 1);
    s_dyn_syms[s_dyn_sym_count].name[sizeof(s_dyn_syms[s_dyn_sym_count].name) - 1] = '\0';
    s_dyn_syms[s_dyn_sym_count].addr = address;
    s_dyn_sym_count++;
    return 0;
}

static const maia_host_api_v1 *s_active_host = NULL;

static void maia_host_print(const char *s) {
    if (!s) return;
    printf("%s", s);
    fflush(stdout);
}

static void maia_host_println(const char *s) {
    if (s) printf("%s", s);
    printf("\n");
    fflush(stdout);
}

static void maia_host_print_int(int32_t val) {
    printf("%d\n", (int)val);
    fflush(stdout);
}

static void maia_host_delay_ms(uint32_t ms) {
    if (s_active_host && s_active_host->sleep_ms) {
        s_active_host->sleep_ms(ms);
    } else {
#if defined(ESP_PLATFORM)
        vTaskDelay(pdMS_TO_TICKS(ms));
#else
        usleep(ms * 1000);
#endif
    }
}

static uint32_t maia_host_millis(void) {
    if (s_active_host && s_active_host->get_time_us) {
        return (uint32_t)(s_active_host->get_time_us() / 1000);
    }
#if defined(ESP_PLATFORM)
    return (uint32_t)(esp_timer_get_time() / 1000);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

static int32_t maia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
    if (s_active_host && s_active_host->display && s_active_host->display->fill_rect) {
        return s_active_host->display->fill_rect(x, y, w, h, color);
    }
    return 0;
}

static void maia_host_display_sync(void) {
    if (s_active_host && s_active_host->display && s_active_host->display->sync) {
        s_active_host->display->sync();
    }
}

static int32_t maia_host_play_beep(uint32_t freq_hz, uint32_t duration_ms) {
    if (s_active_host && s_active_host->play_beep) {
        return s_active_host->play_beep(freq_hz, duration_ms);
    }
    return 0;
}

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
    s_active_host = host;
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
#define MAX_ELF_SECTIONS 256
    size_t sec_offsets[MAX_ELF_SECTIONS] = {0};
    if (ehdr->e_shnum > MAX_ELF_SECTIONS) return MNM_ERR_OUT_OF_MEMORY;

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

    // 2. Allocate executable memory region in Internal L2 SRAM (strictly in sram_low < 0x4FF40000)
    uint8_t *module_base = NULL;
    bool using_static_pool = false;

#if defined(ESP_PLATFORM)
    size_t alloc_aligned = (total_alloc + 63) & ~63;
    if (s_module_pool_offset + alloc_aligned <= sizeof(s_module_exec_pool)) {
        module_base = s_module_exec_pool + s_module_pool_offset;
        s_module_pool_offset += alloc_aligned;
        using_static_pool = true;
    } else {
        // Fallback: dynamically allocate ensuring address is strictly within sram_low
        void *high_blocks[32] = {0};
        int high_count = 0;
        while (high_count < 32) {
            module_base = (uint8_t *)heap_caps_aligned_alloc(16, total_alloc, MALLOC_CAP_INTERNAL | MALLOC_CAP_EXEC);
            if (!module_base) {
                break;
            }
            if ((uintptr_t)module_base + total_alloc <= 0x4FF40000) {
                // Verified inside executable sram_low
                break;
            }
            high_blocks[high_count++] = module_base;
            module_base = NULL;
        }
        for (int i = 0; i < high_count; ++i) {
            free(high_blocks[i]);
        }
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
    uintptr_t sec_addrs[MAX_ELF_SECTIONS] = {0};
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
#if defined(ESP_PLATFORM)
        if (using_static_pool) {
            size_t alloc_aligned = (total_alloc + 63) & ~63;
            if (s_module_pool_offset >= alloc_aligned) {
                s_module_pool_offset -= alloc_aligned;
            } else {
                s_module_pool_offset = 0;
            }
        } else if (module_base) {
            free(module_base);
        }
#else
        if (module_base) free(module_base);
#endif
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
                    } else if (sym->st_shndx == 0 && strtab && sym->st_name != 0) {
                        const char *sname = strtab + sym->st_name;
                        if (sname[0] == '\0') {
                            // Empty symbol, do not report warning
                        } else {
                            extern int64_t __divdi3(int64_t, int64_t);
                            extern uint64_t __udivdi3(uint64_t, uint64_t);
                            extern uint64_t __umoddi3(uint64_t, uint64_t);
                            extern int64_t __moddi3(int64_t, int64_t);

                            extern double __adddf3(double, double);
                            extern double __subdf3(double, double);
                            extern double __muldf3(double, double);
                            extern double __divdf3(double, double);
                            extern int32_t __fixdfsi(double);
                            extern double __floatsidf(int32_t);
                            extern int __ltdf2(double, double);
                            extern int __gtdf2(double, double);
                            extern int __eqdf2(double, double);
                            extern int __nedf2(double, double);

                            if (strcmp(sname, "strlen") == 0) sym_val = (uintptr_t)strlen;
                            else if (strcmp(sname, "memcpy") == 0) sym_val = (uintptr_t)memcpy;
                            else if (strcmp(sname, "memset") == 0) sym_val = (uintptr_t)memset;
                            else if (strcmp(sname, "memmove") == 0) sym_val = (uintptr_t)memmove;
                            else if (strcmp(sname, "memcmp") == 0) sym_val = (uintptr_t)memcmp;
                            else if (strcmp(sname, "strcmp") == 0) sym_val = (uintptr_t)strcmp;
                            else if (strcmp(sname, "strncmp") == 0) sym_val = (uintptr_t)strncmp;
                            else if (strcmp(sname, "print") == 0) sym_val = (uintptr_t)maia_host_print;
                            else if (strcmp(sname, "println") == 0) sym_val = (uintptr_t)maia_host_println;
                            else if (strcmp(sname, "print_int") == 0) sym_val = (uintptr_t)maia_host_print_int;
                            else if (strcmp(sname, "delay_ms") == 0 || strcmp(sname, "sleep_ms") == 0) sym_val = (uintptr_t)maia_host_delay_ms;
                            else if (strcmp(sname, "millis") == 0) sym_val = (uintptr_t)maia_host_millis;
                            else if (strcmp(sname, "fill_rect") == 0) sym_val = (uintptr_t)maia_host_fill_rect;
                            else if (strcmp(sname, "display_sync") == 0) sym_val = (uintptr_t)maia_host_display_sync;
                            else if (strcmp(sname, "play_beep") == 0) sym_val = (uintptr_t)maia_host_play_beep;
                            else if (strcmp(sname, "puts") == 0) sym_val = (uintptr_t)puts;
                            else if (strcmp(sname, "printf") == 0) sym_val = (uintptr_t)printf;
                            else if (strcmp(sname, "malloc") == 0) sym_val = (uintptr_t)malloc;
                            else if (strcmp(sname, "free") == 0) sym_val = (uintptr_t)free;
                            else if (strcmp(sname, "__divdi3") == 0) sym_val = (uintptr_t)__divdi3;
                            else if (strcmp(sname, "__udivdi3") == 0) sym_val = (uintptr_t)__udivdi3;
                            else if (strcmp(sname, "__umoddi3") == 0) sym_val = (uintptr_t)__umoddi3;
                            else if (strcmp(sname, "__moddi3") == 0) sym_val = (uintptr_t)__moddi3;
                            else if (strcmp(sname, "__adddf3") == 0) sym_val = (uintptr_t)__adddf3;
                            else if (strcmp(sname, "__subdf3") == 0) sym_val = (uintptr_t)__subdf3;
                            else if (strcmp(sname, "__muldf3") == 0) sym_val = (uintptr_t)__muldf3;
                            else if (strcmp(sname, "__divdf3") == 0) sym_val = (uintptr_t)__divdf3;
                            else if (strcmp(sname, "__fixdfsi") == 0) sym_val = (uintptr_t)__fixdfsi;
                            else if (strcmp(sname, "__floatsidf") == 0) sym_val = (uintptr_t)__floatsidf;
                            else if (strcmp(sname, "__ltdf2") == 0) sym_val = (uintptr_t)__ltdf2;
                            else if (strcmp(sname, "__gtdf2") == 0) sym_val = (uintptr_t)__gtdf2;
                            else if (strcmp(sname, "__eqdf2") == 0) sym_val = (uintptr_t)__eqdf2;
                            else if (strcmp(sname, "__nedf2") == 0) sym_val = (uintptr_t)__nedf2;
                            else {
                                for (int d = 0; d < s_dyn_sym_count; ++d) {
                                    if (strcmp(sname, s_dyn_syms[d].name) == 0) {
                                        sym_val = s_dyn_syms[d].addr;
                                        break;
                                    }
                                }
                                if (sym_val == 0 && log_fn) {
                                    char err[128];
                                    snprintf(err, sizeof(err), "Loader Warning: Unresolved external symbol '%s'", sname);
                                    log_fn(err);
                                }
                            }
                        }
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

                    case R_RISCV_JAL: {
                        int32_t off = (int32_t)(val - (uintptr_t)loc);
                        uint32_t insn = get_u32(loc);
                        insn = (insn & ~0xFFFFF000) |
                               (((off >> 20) & 0x1) << 31) |
                               (((off >> 1) & 0x3FF) << 21) |
                               (((off >> 11) & 0x1) << 20) |
                               (((off >> 12) & 0xFF) << 12);
                        set_u32(loc, insn);
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

                    case R_RISCV_RVC_JUMP: {
                        int32_t off = (int32_t)(val - (uintptr_t)loc);
                        uint16_t insn = get_u16(loc);
                        insn = (insn & ~0x1FFC) |
                               (((off >> 11) & 0x1) << 12) |
                               (((off >> 4) & 0x1) << 11) |
                               (((off >> 8) & 0x3) << 9) |
                               (((off >> 10) & 0x1) << 8) |
                               (((off >> 6) & 0x1) << 7) |
                               (((off >> 7) & 0x1) << 6) |
                               (((off >> 1) & 0x7) << 3) |
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

    // 6. Synchronize instruction cache barrier and flush data cache to memory
#if defined(ESP_PLATFORM)
    uintptr_t sync_start = (uintptr_t)module_base & ~63;
    size_t sync_size = ((total_alloc + 63) & ~63) + ((uintptr_t)module_base & 63);
    esp_cache_msync((void *)sync_start, sync_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    esp_cache_msync((void *)sync_start, sync_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_INST);
#endif
#if defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
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

#if defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
    asm volatile("fence.i" ::: "memory");
#endif

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
#if defined(ESP_PLATFORM)
    if (using_static_pool) {
        size_t alloc_aligned = (total_alloc + 63) & ~63;
        if (s_module_pool_offset >= alloc_aligned) {
            s_module_pool_offset -= alloc_aligned;
        } else {
            s_module_pool_offset = 0;
        }
    } else if (module_base) {
        free(module_base);
    }
#else
    if (module_base) {
        free(module_base);
    }
#endif

    return MNM_LOAD_OK;
}
