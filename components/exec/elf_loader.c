#include <maia/exec/elf_loader.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#if defined(MAIA_NATIVE_LCD_DISPLAY)
#include <maia/services/native_display.h>
#include <maia/exec/legacy_host_compat.h>
#endif

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
#if defined(MAIA_EXEC_ESP32P4_CACHE_SYNC)
#include <maia/soc/esp32p4_cache.h>
#endif
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

int mnm_get_registered_symbols(const char **names, uintptr_t *addrs, int max) {
    int count = s_dyn_sym_count < max ? s_dyn_sym_count : max;
    for (int i = 0; i < count; ++i) {
        if (names) names[i] = s_dyn_syms[i].name;
        if (addrs) addrs[i] = s_dyn_syms[i].addr;
    }
    return count;
}

#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS && !defined(ESP_PLATFORM)
static _Thread_local const maia_host_api_v1 *s_active_host = NULL;
#else
static const maia_host_api_v1 *s_active_host = NULL;
#endif

#if defined(MAIA_NATIVE_LCD_DISPLAY)
static int __attribute__((noinline)) mnm_invoke_legacy_entry(
    maia_module_entry_fn entry, const maia_module_context_t *ctx) {
    maia_legacy_host_scope_t legacy_scope;
    if (maia_legacy_host_scope_begin(&legacy_scope, ctx->host) != 0) {
        return -1;
    }
    int result = entry(ctx);
    if (maia_legacy_host_scope_end(&legacy_scope) != 0 && result == 0) {
        result = -1;
    }
    return result;
}
#endif

static int mnm_invoke_entry(maia_module_entry_fn entry,
                            const maia_module_context_t *ctx,
                            int needs_legacy_scope) {
    const maia_host_api_v1 *previous_host = s_active_host;
    s_active_host = ctx->host;
#if defined(MAIA_NATIVE_LCD_DISPLAY)
    if (maia_native_display_scope_begin() != 0) {
        s_active_host = previous_host;
        return -1;
    }
    int result = needs_legacy_scope
        ? mnm_invoke_legacy_entry(entry, ctx)
        : entry(ctx);
#else
    (void)needs_legacy_scope;
    int result = entry(ctx);
#endif
#if defined(MAIA_NATIVE_LCD_DISPLAY)
    if (maia_native_display_scope_end() != 0 && result == 0) result = -1;
#endif
    s_active_host = previous_host;
    return result;
}

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

static uint32_t maia_host_cycles(void) {
#if defined(__riscv)
    uint32_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
#elif defined(ESP_PLATFORM)
    return (uint32_t)esp_timer_get_time();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
#endif
}

#if !defined(MAIA_NATIVE_LCD_DISPLAY)
static int32_t maia_host_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
    if (s_active_host && s_active_host->display && s_active_host->display->fill_rect) {
        return s_active_host->display->fill_rect(x, y, w, h, color);
    }
    return -1;
}

static void maia_host_display_sync(void) {
    if (s_active_host && s_active_host->display && s_active_host->display->sync) {
        s_active_host->display->sync();
    }
}
#endif

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

#define MAX_ELF_SECTIONS 256

static int source_read_exact(const elf_reader_t *reader, size_t offset,
                             void *buffer, size_t length) {
    if (reader == NULL || reader->read_at == NULL ||
        (length > 0U && buffer == NULL) ||
        offset > reader->size || length > reader->size - offset) {
        return MNM_ERR_VALIDATION_FAILED;
    }
    if (length == 0U) return MNM_LOAD_OK;
    return reader->read_at(reader->context, offset, buffer, length) == 0
        ? MNM_LOAD_OK : MNM_ERR_IO;
}

static int source_read_section(const elf_reader_t *reader,
                               const Elf32_Ehdr *ehdr, uint16_t index,
                               Elf32_Shdr *section) {
    if (index >= ehdr->e_shnum ||
        ehdr->e_shentsize < sizeof(Elf32_Shdr)) {
        return MNM_ERR_VALIDATION_FAILED;
    }
    uint64_t offset = (uint64_t)ehdr->e_shoff +
                      (uint64_t)index * ehdr->e_shentsize;
    if (offset > SIZE_MAX) return MNM_ERR_VALIDATION_FAILED;
    return source_read_exact(reader, (size_t)offset, section, sizeof(*section));
}

static int source_read_symbol(const elf_reader_t *reader,
                              const Elf32_Shdr *symbol_table,
                              size_t symbol_count, size_t index,
                              Elf32_Sym *symbol) {
    if (index >= symbol_count) return MNM_ERR_VALIDATION_FAILED;
    uint64_t offset = (uint64_t)symbol_table->sh_offset +
                      (uint64_t)index * sizeof(*symbol);
    if (offset > SIZE_MAX) return MNM_ERR_VALIDATION_FAILED;
    return source_read_exact(reader, (size_t)offset, symbol, sizeof(*symbol));
}

static int source_read_string(const elf_reader_t *reader,
                              const Elf32_Shdr *string_table,
                              uint32_t name_offset,
                              char *output, size_t output_size) {
    if (output == NULL || output_size == 0U ||
        name_offset >= string_table->sh_size) {
        return MNM_ERR_VALIDATION_FAILED;
    }

    size_t cursor = name_offset;
    size_t copied = 0U;
    int truncated = 0;
    char chunk[32];
    output[0] = '\0';

    while (cursor < string_table->sh_size) {
        size_t remaining = (size_t)string_table->sh_size - cursor;
        size_t length = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        size_t offset = (size_t)string_table->sh_offset + cursor;
        int result = source_read_exact(reader, offset, chunk, length);
        if (result != MNM_LOAD_OK) return result;

        char *terminator = (char *)memchr(chunk, '\0', length);
        size_t part = terminator != NULL
            ? (size_t)(terminator - chunk) : length;
        size_t room = output_size - 1U - copied;
        size_t copy = part < room ? part : room;
        if (copy > 0U) {
            memcpy(output + copied, chunk, copy);
            copied += copy;
            output[copied] = '\0';
        }
        if (copy < part) truncated = 1;
        if (terminator != NULL) {
            return truncated ? 1 : MNM_LOAD_OK;
        }
        cursor += length;
    }
    return MNM_ERR_VALIDATION_FAILED;
}

static size_t relocation_width(uint32_t type) {
    switch (type) {
        case R_RISCV_32:
        case R_RISCV_HI20:
        case R_RISCV_LO12_I:
        case R_RISCV_LO12_S:
        case R_RISCV_JAL:
        case R_RISCV_BRANCH:
            return 4U;
        case R_RISCV_CALL:
        case R_RISCV_CALL_PLT:
            return 8U;
        case R_RISCV_RVC_BRANCH:
        case R_RISCV_RVC_JUMP:
            return 2U;
        default:
            return 0U;
    }
}

static void release_module_memory(uint8_t *module_base, size_t total_alloc,
                                 bool using_static_pool) {
#if defined(ESP_PLATFORM)
    if (using_static_pool) {
        size_t alloc_aligned = (total_alloc + 63U) & ~(size_t)63U;
        if (s_module_pool_offset >= alloc_aligned) {
            s_module_pool_offset -= alloc_aligned;
        } else {
            s_module_pool_offset = 0U;
        }
    } else if (module_base != NULL) {
        free(module_base);
    }
#else
    (void)total_alloc;
    (void)using_static_pool;
    free(module_base);
#endif
}

static int memory_source_read(void *context, size_t offset,
                              void *buffer, size_t length) {
    if (context == NULL || (length > 0U && buffer == NULL)) return -1;
    memcpy(buffer, (const uint8_t *)context + offset, length);
    return 0;
}

int mnm_load_and_run(const void *elf_data, size_t elf_size,
                     int argc, const char *const *argv,
                     const maia_host_api_v1 *host,
                     int *exit_code,
                     void (*log_fn)(const char *msg)) {
    if (elf_data == NULL) {
        if (log_fn) log_fn("ELF Error: NULL buffer pointer");
        return MNM_ERR_VALIDATION_FAILED;
    }
    elf_reader_t reader = {
        .read_at = memory_source_read,
        .context = (void *)elf_data,
        .size = elf_size
    };
    return mnm_load_and_run_reader(&reader, argc, argv, host, exit_code, log_fn);
}

int mnm_load_and_run_reader(const elf_reader_t *reader,
                            int argc, const char *const *argv,
                            const maia_host_api_v1 *host,
                            int *exit_code,
                            void (*log_fn)(const char *msg)) {
#if defined(MAIA_DIAGNOSTIC_MODULE_TIMING)
    uint64_t validation_started_us =
        host != NULL && host->get_time_us != NULL ? host->get_time_us() : 0U;
#endif
    char report[256];
    int validation = elf_validate_relocatable_reader(reader, report, sizeof(report));
#if defined(MAIA_DIAGNOSTIC_MODULE_TIMING)
    if (host != NULL && host->get_time_us != NULL && log_fn != NULL) {
        uint64_t validation_finished_us = host->get_time_us();
        uint64_t elapsed_us = validation_finished_us >= validation_started_us
            ? validation_finished_us - validation_started_us : 0U;
        char message[96];
        snprintf(message, sizeof(message),
                 "Loader timing: validation=%llu us",
                 (unsigned long long)elapsed_us);
        log_fn(message);
    }
#endif
    if (validation != ELF_VALID_OK) {
        if (log_fn) log_fn(report);
        return validation == ELF_ERR_IO ? MNM_ERR_IO : MNM_ERR_VALIDATION_FAILED;
    }
#if defined(MAIA_DIAGNOSTIC_MODULE_TIMING)
    uint64_t load_started_us =
        host != NULL && host->get_time_us != NULL ? host->get_time_us() : 0U;
#endif

    Elf32_Ehdr ehdr;
    int result = source_read_exact(reader, 0U, &ehdr, sizeof(ehdr));
    if (result != MNM_LOAD_OK) return result;
    if (ehdr.e_shnum > MAX_ELF_SECTIONS) return MNM_ERR_VALIDATION_FAILED;

    size_t total_alloc = 0U;
    uintptr_t section_addresses[MAX_ELF_SECTIONS] = {0};
    int text_section_index = -1;
    bool needs_legacy_scope = false;

    for (uint16_t i = 0U; i < ehdr.e_shnum; ++i) {
        Elf32_Shdr section;
        result = source_read_section(reader, &ehdr, i, &section);
        if (result != MNM_LOAD_OK) return result;
        if ((section.sh_flags & SHF_ALLOC) == 0U) continue;

        size_t alignment = section.sh_addralign > 0U
            ? section.sh_addralign : 4U;
        if ((alignment & (alignment - 1U)) != 0U ||
            total_alloc > SIZE_MAX - (alignment - 1U)) {
            return MNM_ERR_VALIDATION_FAILED;
        }
        total_alloc = (total_alloc + alignment - 1U) & ~(alignment - 1U);
#if SIZE_MAX > UINTPTR_MAX
        if (total_alloc > UINTPTR_MAX) {
            return MNM_ERR_VALIDATION_FAILED;
        }
#endif
        section_addresses[i] = (uintptr_t)total_alloc;
        if (section.sh_size > SIZE_MAX - total_alloc) {
            return MNM_ERR_VALIDATION_FAILED;
        }
        total_alloc += section.sh_size;
        if ((section.sh_flags & SHF_EXECINSTR) != 0U) {
            text_section_index = (int)i;
        }
    }

    if (text_section_index < 0) {
        if (log_fn) log_fn("Loader Error: No executable .text section found");
        return MNM_ERR_NO_TEXT_SECTION;
    }
    if (total_alloc > SIZE_MAX - 63U) return MNM_ERR_OUT_OF_MEMORY;
    total_alloc = (total_alloc + 15U) & ~(size_t)15U;
#if SIZE_MAX > UINTPTR_MAX
    if (total_alloc > UINTPTR_MAX) {
        return MNM_ERR_OUT_OF_MEMORY;
    }
#endif

    uint8_t *module_base = NULL;
    bool using_static_pool = false;
#if defined(ESP_PLATFORM)
    size_t alloc_aligned = (total_alloc + 63U) & ~(size_t)63U;
    if (s_module_pool_offset <= sizeof(s_module_exec_pool) &&
        alloc_aligned <= sizeof(s_module_exec_pool) - s_module_pool_offset) {
        module_base = s_module_exec_pool + s_module_pool_offset;
        s_module_pool_offset += alloc_aligned;
        using_static_pool = true;
    } else {
        void *high_blocks[32] = {0};
        int high_count = 0;
        while (high_count < 32) {
            module_base = (uint8_t *)heap_caps_aligned_alloc(
                16U, total_alloc, MALLOC_CAP_INTERNAL | MALLOC_CAP_EXEC);
            if (module_base == NULL) break;
            if ((uintptr_t)module_base + total_alloc <= 0x4FF40000U) break;
            high_blocks[high_count++] = module_base;
            module_base = NULL;
        }
        for (int i = 0; i < high_count; ++i) free(high_blocks[i]);
    }
#else
    module_base = (uint8_t *)malloc(total_alloc);
#endif
    if (module_base == NULL) {
        if (log_fn) log_fn("Loader Error: Out of executable memory");
        return MNM_ERR_OUT_OF_MEMORY;
    }
    memset(module_base, 0, total_alloc);

    if (log_fn) {
        char message[128];
        snprintf(message, sizeof(message), "Loader: Allocated %u bytes at %p",
                 (unsigned int)total_alloc, module_base);
        log_fn(message);
    }

    for (uint16_t i = 0U; i < ehdr.e_shnum; ++i) {
        Elf32_Shdr section;
        result = source_read_section(reader, &ehdr, i, &section);
        if (result != MNM_LOAD_OK) goto cleanup;
        if ((section.sh_flags & SHF_ALLOC) == 0U) continue;

        size_t section_offset = (size_t)section_addresses[i];
        if ((uintptr_t)section_offset != section_addresses[i]) {
            result = MNM_ERR_VALIDATION_FAILED;
            goto cleanup;
        }
        section_addresses[i] = (uintptr_t)(module_base + section_offset);
        if (section.sh_type != SHT_NOBITS && section.sh_size > 0U) {
            result = source_read_exact(reader, section.sh_offset,
                                       (void *)section_addresses[i],
                                       section.sh_size);
            if (result != MNM_LOAD_OK) goto cleanup;
        }
    }

    Elf32_Shdr symbol_table = {0};
    Elf32_Shdr string_table = {0};
    size_t symbol_count = 0U;
    int has_symbol_table = 0;
    for (uint16_t i = 0U; i < ehdr.e_shnum; ++i) {
        Elf32_Shdr section;
        result = source_read_section(reader, &ehdr, i, &section);
        if (result != MNM_LOAD_OK) goto cleanup;
        if (section.sh_type != SHT_SYMTAB) continue;
        if (section.sh_link >= ehdr.e_shnum) {
            result = MNM_ERR_VALIDATION_FAILED;
            goto cleanup;
        }
        result = source_read_section(reader, &ehdr, (uint16_t)section.sh_link,
                                     &string_table);
        if (result != MNM_LOAD_OK) goto cleanup;
        if (string_table.sh_type != SHT_STRTAB) {
            result = MNM_ERR_VALIDATION_FAILED;
            goto cleanup;
        }
        symbol_table = section;
        symbol_count = section.sh_size / sizeof(Elf32_Sym);
        has_symbol_table = 1;
        break;
    }

    maia_module_entry_fn entry_point = NULL;
    if (has_symbol_table) {
        for (size_t i = 0U; i < symbol_count; ++i) {
            Elf32_Sym symbol;
            result = source_read_symbol(reader, &symbol_table, symbol_count,
                                        i, &symbol);
            if (result != MNM_LOAD_OK) goto cleanup;
            if (symbol.st_shndx == 0U ||
                symbol.st_shndx >= ehdr.e_shnum) continue;

            char name[32];
            result = source_read_string(reader, &string_table,
                                        symbol.st_name, name, sizeof(name));
            if (result == MNM_ERR_IO) goto cleanup;
            if (result == MNM_ERR_VALIDATION_FAILED) goto cleanup;
            if (result == 0 && strcmp(name, "maia_module_main") == 0) {
                Elf32_Shdr entry_section;
                result = source_read_section(reader, &ehdr,
                                             symbol.st_shndx,
                                             &entry_section);
                if (result != MNM_LOAD_OK) goto cleanup;
                if ((entry_section.sh_flags & SHF_EXECINSTR) == 0U ||
                    symbol.st_value >= entry_section.sh_size) {
                    result = MNM_ERR_VALIDATION_FAILED;
                    goto cleanup;
                }
                entry_point = (maia_module_entry_fn)(
                    section_addresses[symbol.st_shndx] + symbol.st_value);
                break;
            }
        }
    }

    if (entry_point == NULL) {
        if (log_fn) log_fn("Loader Error: Symbol 'maia_module_main' not found");
        result = MNM_ERR_NO_ENTRY_POINT;
        goto cleanup;
    }

    // Apply relocations using bounded reads from the ELF source.
    for (uint16_t i = 0U; i < ehdr.e_shnum; ++i) {
        Elf32_Shdr relocation_section;
        result = source_read_section(reader, &ehdr, i, &relocation_section);
        if (result != MNM_LOAD_OK) goto cleanup;
        if (relocation_section.sh_type != SHT_RELA) continue;
        if (relocation_section.sh_info >= ehdr.e_shnum ||
            relocation_section.sh_size % sizeof(Elf32_Rela) != 0U) {
            result = MNM_ERR_VALIDATION_FAILED;
            goto cleanup;
        }

        uint16_t target_section_index = (uint16_t)relocation_section.sh_info;
        if (section_addresses[target_section_index] == 0U) continue;
        Elf32_Shdr target_section;
        result = source_read_section(reader, &ehdr, target_section_index,
                                     &target_section);
        if (result != MNM_LOAD_OK) goto cleanup;
        uint8_t *target = (uint8_t *)section_addresses[target_section_index];
        size_t relocation_count =
            relocation_section.sh_size / sizeof(Elf32_Rela);

        for (size_t r = 0U; r < relocation_count; ++r) {
            uint64_t relocation_offset =
                (uint64_t)relocation_section.sh_offset +
                (uint64_t)r * sizeof(Elf32_Rela);
            if (relocation_offset > SIZE_MAX) {
                result = MNM_ERR_VALIDATION_FAILED;
                goto cleanup;
            }
            Elf32_Rela relocation;
            result = source_read_exact(reader, (size_t)relocation_offset,
                                       &relocation, sizeof(relocation));
            if (result != MNM_LOAD_OK) goto cleanup;

            uint32_t r_offset = relocation.r_offset;
            uint32_t r_sym = ELF32_R_SYM(relocation.r_info);
            uint32_t r_type = ELF32_R_TYPE(relocation.r_info);
            int32_t r_addend = relocation.r_addend;
            size_t width = relocation_width(r_type);
            if (width > 0U &&
                ((size_t)r_offset > target_section.sh_size ||
                 width > (size_t)target_section.sh_size - r_offset)) {
                if (log_fn) log_fn("Loader Error: Relocation offset out of range");
                result = MNM_ERR_VALIDATION_FAILED;
                goto cleanup;
            }

            uintptr_t sym_val = 0U;
            if (has_symbol_table && r_sym < symbol_count) {
                Elf32_Sym symbol;
                result = source_read_symbol(reader, &symbol_table,
                                            symbol_count, r_sym, &symbol);
                if (result != MNM_LOAD_OK) goto cleanup;
                if (symbol.st_shndx > 0U &&
                    symbol.st_shndx < ehdr.e_shnum) {
                    Elf32_Shdr definition_section;
                    result = source_read_section(reader, &ehdr,
                                                 symbol.st_shndx,
                                                 &definition_section);
                    if (result != MNM_LOAD_OK) goto cleanup;
                    if ((definition_section.sh_flags & SHF_ALLOC) == 0U ||
                        symbol.st_value > definition_section.sh_size ||
                        section_addresses[symbol.st_shndx] >
                            UINTPTR_MAX - symbol.st_value) {
                        result = MNM_ERR_VALIDATION_FAILED;
                        goto cleanup;
                    }
                    sym_val = section_addresses[symbol.st_shndx] +
                              symbol.st_value;
                } else if (symbol.st_shndx == 0U && symbol.st_name != 0U) {
                    char symbol_name[128];
                    result = source_read_string(reader, &string_table,
                                                symbol.st_name, symbol_name,
                                                sizeof(symbol_name));
                    if (result < 0) goto cleanup;
                    if (symbol_name[0] == '\0') {
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

                            if (strcmp(symbol_name, "strlen") == 0) sym_val = (uintptr_t)strlen;
                            else if (strcmp(symbol_name, "memcpy") == 0) sym_val = (uintptr_t)memcpy;
                            else if (strcmp(symbol_name, "memset") == 0) sym_val = (uintptr_t)memset;
                            else if (strcmp(symbol_name, "memmove") == 0) sym_val = (uintptr_t)memmove;
                            else if (strcmp(symbol_name, "memcmp") == 0) sym_val = (uintptr_t)memcmp;
                            else if (strcmp(symbol_name, "strcmp") == 0) sym_val = (uintptr_t)strcmp;
                            else if (strcmp(symbol_name, "strncmp") == 0) sym_val = (uintptr_t)strncmp;
                            else if (strcmp(symbol_name, "strncpy") == 0) sym_val = (uintptr_t)strncpy;
                            else if (strcmp(symbol_name, "print") == 0) sym_val = (uintptr_t)maia_host_print;
                            else if (strcmp(symbol_name, "println") == 0) sym_val = (uintptr_t)maia_host_println;
                            else if (strcmp(symbol_name, "print_int") == 0) sym_val = (uintptr_t)maia_host_print_int;
                            else if (strcmp(symbol_name, "delay_ms") == 0 || strcmp(symbol_name, "sleep_ms") == 0) sym_val = (uintptr_t)maia_host_delay_ms;
                            else if (strcmp(symbol_name, "millis") == 0) sym_val = (uintptr_t)maia_host_millis;
                            else if (strcmp(symbol_name, "cycles") == 0 || strcmp(symbol_name, "rdcycle") == 0) sym_val = (uintptr_t)maia_host_cycles;
                            else if (strcmp(symbol_name, "fill_rect") == 0) {
#if defined(MAIA_NATIVE_LCD_DISPLAY)
                                sym_val = maia_legacy_host_resolve_symbol(symbol_name);
                                if (sym_val != 0U) needs_legacy_scope = true;
#else
                                sym_val = (uintptr_t)maia_host_fill_rect;
#endif
                            }
                            else if (strcmp(symbol_name, "display_sync") == 0) {
#if defined(MAIA_NATIVE_LCD_DISPLAY)
                                sym_val = maia_legacy_host_resolve_symbol(symbol_name);
                                if (sym_val != 0U) needs_legacy_scope = true;
#else
                                sym_val = (uintptr_t)maia_host_display_sync;
#endif
                            }
                            else if (strcmp(symbol_name, "play_beep") == 0) sym_val = (uintptr_t)maia_host_play_beep;
                            else if (strcmp(symbol_name, "puts") == 0) sym_val = (uintptr_t)puts;
                            else if (strcmp(symbol_name, "printf") == 0) sym_val = (uintptr_t)printf;
                            else if (strcmp(symbol_name, "malloc") == 0) sym_val = (uintptr_t)malloc;
                            else if (strcmp(symbol_name, "free") == 0) sym_val = (uintptr_t)free;
                            else if (strcmp(symbol_name, "__divdi3") == 0) sym_val = (uintptr_t)__divdi3;
                            else if (strcmp(symbol_name, "__udivdi3") == 0) sym_val = (uintptr_t)__udivdi3;
                            else if (strcmp(symbol_name, "__umoddi3") == 0) sym_val = (uintptr_t)__umoddi3;
                            else if (strcmp(symbol_name, "__moddi3") == 0) sym_val = (uintptr_t)__moddi3;
                            else if (strcmp(symbol_name, "__adddf3") == 0) sym_val = (uintptr_t)__adddf3;
                            else if (strcmp(symbol_name, "__subdf3") == 0) sym_val = (uintptr_t)__subdf3;
                            else if (strcmp(symbol_name, "__muldf3") == 0) sym_val = (uintptr_t)__muldf3;
                            else if (strcmp(symbol_name, "__divdf3") == 0) sym_val = (uintptr_t)__divdf3;
                            else if (strcmp(symbol_name, "__fixdfsi") == 0) sym_val = (uintptr_t)__fixdfsi;
                            else if (strcmp(symbol_name, "__floatsidf") == 0) sym_val = (uintptr_t)__floatsidf;
                            else if (strcmp(symbol_name, "__ltdf2") == 0) sym_val = (uintptr_t)__ltdf2;
                            else if (strcmp(symbol_name, "__gtdf2") == 0) sym_val = (uintptr_t)__gtdf2;
                            else if (strcmp(symbol_name, "__eqdf2") == 0) sym_val = (uintptr_t)__eqdf2;
                            else if (strcmp(symbol_name, "__nedf2") == 0) sym_val = (uintptr_t)__nedf2;
                            else {
                                for (int d = 0; d < s_dyn_sym_count; ++d) {
                                    if (strcmp(symbol_name, s_dyn_syms[d].name) == 0) {
                                        sym_val = s_dyn_syms[d].addr;
                                        break;
                                    }
                                }
#if defined(MAIA_NATIVE_LCD_DISPLAY)
                                if (sym_val == 0U) {
                                    sym_val = maia_legacy_host_resolve_symbol(symbol_name);
                                    if (sym_val != 0U) needs_legacy_scope = true;
                                }
#endif
                                if (sym_val == 0 && log_fn) {
                                    char err[128];
                                    snprintf(err, sizeof(err), "Loader Warning: Unresolved external symbol '%s'", symbol_name);
                                    log_fn(err);
                                }
                            }
                        }
                    }
                }

                if (r_type == R_RISCV_NONE || r_type == R_RISCV_RELAX) {
                    continue;
                }
                if (width == 0U) {
                    if (log_fn) {
                        char warn[80];
                        snprintf(warn, sizeof(warn),
                                 "Loader Warning: Ignored relocation %u",
                                 (unsigned int)r_type);
                        log_fn(warn);
                    }
                    continue;
                }

                uint8_t *loc = target + r_offset;
                uintptr_t val = sym_val + r_addend;

                switch (r_type) {
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
                        break;
                }
            }
        }

    // 6. Synchronize instruction cache barrier and flush data cache to memory
#if defined(ESP_PLATFORM)
    uintptr_t sync_start = (uintptr_t)module_base & ~63;
    size_t sync_size = ((total_alloc + 63) & ~63) + ((uintptr_t)module_base & 63);
    esp_cache_msync((void *)sync_start, sync_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    esp_cache_msync((void *)sync_start, sync_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_INST);
#elif defined(MAIA_EXEC_ESP32P4_CACHE_SYNC)
    if (maia_esp32p4_cache_sync_code((uint32_t)(uintptr_t)module_base, (uint32_t)total_alloc) != 0) {
        if (log_fn) {
            log_fn("Loader Error: Failed to synchronize instruction cache.");
        }
        result = MNM_ERR_EXECUTION_FAULT;
        goto cleanup;
    }
#endif
#if defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
    asm volatile("fence.i" ::: "memory");
#endif

#if defined(MAIA_DIAGNOSTIC_MODULE_TIMING)
    if (host != NULL && host->get_time_us != NULL && log_fn != NULL) {
        uint64_t load_finished_us = host->get_time_us();
        uint64_t elapsed_us = load_finished_us >= load_started_us
            ? load_finished_us - load_started_us : 0U;
        char message[96];
        snprintf(message, sizeof(message),
                 "Loader timing: load-relocate=%llu us",
                 (unsigned long long)elapsed_us);
        log_fn(message);
    }
#endif

    if (log_fn) {
        log_fn("Loader: Relocations applied & fence.i synchronized.");
    }
    if (reader->release != NULL) {
        reader->release(reader->context);
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
        char gdb_msg[160];
        snprintf(gdb_msg, sizeof(gdb_msg), "GDB: add-symbol-file <module.mobj> 0x%lx", (unsigned long)section_addresses[text_section_index]);
        log_fn(gdb_msg);
    }

#if defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
    asm volatile("fence.i" ::: "memory");
#endif

    int ret = mnm_invoke_entry(entry_point, &ctx, needs_legacy_scope);

    if (log_fn) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Loader: Module returned %d (0x%X)", ret, ret);
        log_fn(msg);
    }

    if (exit_code) {
        *exit_code = ret;
    }

    result = MNM_LOAD_OK;

cleanup:
    release_module_memory(module_base, total_alloc, using_static_pool);
    return result;
}
