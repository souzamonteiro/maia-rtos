#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS
#include <pthread.h>
#endif
#include <stdint.h>
#include <string.h>

#include "../../components/exec/elf_loader.c"

static unsigned outer_delays;
static unsigned inner_delays;
static unsigned fill_calls;
static int32_t fill_result;

static int32_t test_fill_rect(int32_t x, int32_t y, int32_t width,
                              int32_t height, uint16_t color) {
    assert(x == 1 && y == 2 && width == 3 && height == 4 && color == 0x1234U);
    ++fill_calls;
    return fill_result;
}

static int missing_display_entry(const maia_module_context_t *ctx) {
    assert(s_active_host == ctx->host);
    assert(maia_host_fill_rect(1, 2, 3, 4, 0x1234U) < 0);
    return 0;
}

static int available_display_entry(const maia_module_context_t *ctx) {
    assert(s_active_host == ctx->host);
    fill_result = 37;
    assert(maia_host_fill_rect(1, 2, 3, 4, 0x1234U) == 37);
    fill_result = -17;
    assert(maia_host_fill_rect(1, 2, 3, 4, 0x1234U) == -17);
    return 0;
}

static void check_display_forwarding(void) {
    const maia_display_api_t unavailable_display = {0};
    const maia_display_api_t available_display = {.fill_rect = test_fill_rect};
    const maia_host_api_v1 no_display_host = {0};
    const maia_host_api_v1 missing_callback_host = {.display = &unavailable_display};
    const maia_host_api_v1 available_host = {.display = &available_display};
    const maia_module_context_t no_display = {.host = &no_display_host};
    const maia_module_context_t missing_callback = {.host = &missing_callback_host};
    const maia_module_context_t available = {.host = &available_host};

    assert(maia_host_fill_rect(1, 2, 3, 4, 0x1234U) < 0);
    assert(mnm_invoke_entry(missing_display_entry, &no_display, 0) == 0);
    assert(mnm_invoke_entry(missing_display_entry, &missing_callback, 0) == 0);
    assert(mnm_invoke_entry(available_display_entry, &available, 0) == 0);
    assert(s_active_host == NULL && fill_calls == 2U);
}

static uint64_t outer_time(void) { return 111000U; }
static uint64_t inner_time(void) { return 222000U; }

static int32_t outer_sleep(uint32_t ms) {
    assert(ms == 11U);
    ++outer_delays;
    return 0;
}

static int32_t inner_sleep(uint32_t ms) {
    assert(ms == 7U);
    ++inner_delays;
    return 0;
}

static const maia_host_api_v1 outer_host = {
    .sleep_ms = outer_sleep,
    .get_time_us = outer_time
};
static const maia_host_api_v1 inner_host = {
    .sleep_ms = inner_sleep,
    .get_time_us = inner_time
};
static const maia_host_api_v1 rejected_host = {0};

typedef struct test_elf_source {
    const uint8_t *bytes;
    size_t size;
    unsigned reads;
    int fail_reads;
    int fail_at_offset;
    size_t failed_offset;
} test_elf_source_t;

static int test_elf_read_at(void *context, size_t offset,
                            void *buffer, size_t length) {
    test_elf_source_t *source = (test_elf_source_t *)context;
    if (source == NULL || source->fail_reads ||
        (source->fail_at_offset && offset == source->failed_offset) ||
        offset > source->size || length > source->size - offset) {
        return -1;
    }
    ++source->reads;
    memcpy(buffer, source->bytes + offset, length);
    return 0;
}

static size_t make_no_entry_elf(uint8_t *buffer, size_t capacity) {
    static const char section_names[] = "\0.text\0.shstrtab";
    const size_t text_offset = sizeof(Elf32_Ehdr);
    const size_t names_offset = text_offset + 4U;
    const size_t section_table_offset =
        (names_offset + sizeof(section_names) + 3U) & ~(size_t)3U;
    const size_t total_size =
        section_table_offset + 3U * sizeof(Elf32_Shdr);
    assert(capacity >= total_size);
    memset(buffer, 0, total_size);

    Elf32_Ehdr header = {0};
    header.e_ident[EI_MAG0] = ELFMAG0;
    header.e_ident[EI_MAG1] = ELFMAG1;
    header.e_ident[EI_MAG2] = ELFMAG2;
    header.e_ident[EI_MAG3] = ELFMAG3;
    header.e_ident[EI_CLASS] = ELFCLASS32;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_type = ET_REL;
    header.e_machine = EM_RISCV;
    header.e_version = EV_CURRENT;
    header.e_ehsize = sizeof(Elf32_Ehdr);
    header.e_shoff = (uint32_t)section_table_offset;
    header.e_shentsize = sizeof(Elf32_Shdr);
    header.e_shnum = 3U;
    header.e_shstrndx = 2U;

    Elf32_Shdr sections[3] = {0};
    sections[1].sh_name = 1U;
    sections[1].sh_type = SHT_PROGBITS;
    sections[1].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    sections[1].sh_offset = (uint32_t)text_offset;
    sections[1].sh_size = 4U;
    sections[1].sh_addralign = 4U;
    sections[2].sh_name = 7U;
    sections[2].sh_type = SHT_STRTAB;
    sections[2].sh_offset = (uint32_t)names_offset;
    sections[2].sh_size = sizeof(section_names);
    sections[2].sh_addralign = 1U;

    const uint8_t text[4] = {0x13U, 0x00U, 0x00U, 0x00U};
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + text_offset, text, sizeof(text));
    memcpy(buffer + names_offset, section_names, sizeof(section_names));
    memcpy(buffer + section_table_offset, sections, sizeof(sections));
    return total_size;
}

static void check_streaming_loader(void) {
    uint8_t bytes[256];
    size_t size = make_no_entry_elf(bytes, sizeof(bytes));
    test_elf_source_t source = {
        .bytes = bytes,
        .size = size
    };
    elf_reader_t reader = {
        .read_at = test_elf_read_at,
        .context = &source,
        .size = size
    };
    char report[256];

    assert(elf_validate_relocatable_reader(&reader, report, sizeof(report)) ==
           ELF_VALID_OK);
    assert(source.reads > 0U);
    source.reads = 0U;
    assert(mnm_load_and_run_reader(&reader, 0, NULL, NULL, NULL, NULL) ==
           MNM_ERR_NO_ENTRY_POINT);
    assert(source.reads > 0U);
    assert(mnm_load_and_run(bytes, size, 0, NULL, NULL, NULL, NULL) ==
           MNM_ERR_NO_ENTRY_POINT);

    source.fail_at_offset = 1;
    source.failed_offset = sizeof(Elf32_Ehdr);
    assert(mnm_load_and_run_reader(&reader, 0, NULL, NULL, NULL, NULL) ==
           MNM_ERR_IO);
    source.fail_at_offset = 0;
    source.fail_reads = 1;
    assert(mnm_load_and_run_reader(&reader, 0, NULL, NULL, NULL, NULL) ==
           MNM_ERR_IO);
}

static void check_invalid_load_preserves_host(const maia_host_api_v1 *expected) {
    const uint8_t invalid_elf[] = {0};
    assert(mnm_load_and_run(invalid_elf, sizeof(invalid_elf), 0, NULL,
                            &rejected_host, NULL, NULL) == MNM_ERR_VALIDATION_FAILED);
    assert(s_active_host == expected);
}

static int inner_entry(const maia_module_context_t *ctx) {
    assert(s_active_host == ctx->host && ctx->host == &inner_host);
    assert(maia_host_millis() == 222U);
    maia_host_delay_ms(7U);
    check_invalid_load_preserves_host(&inner_host);
    return 17;
}

static int outer_entry(const maia_module_context_t *ctx) {
    assert(s_active_host == ctx->host && ctx->host == &outer_host);
    assert(maia_host_millis() == 111U);
    check_invalid_load_preserves_host(&outer_host);
    const maia_module_context_t inner = {.host = &inner_host};
    assert(mnm_invoke_entry(inner_entry, &inner, 0) == 17);
    assert(s_active_host == &outer_host);
    assert(maia_host_millis() == 111U);
    maia_host_delay_ms(11U);
    return 42;
}

#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static unsigned entered;
static unsigned checked;

static void rendezvous(unsigned *count) {
    assert(pthread_mutex_lock(&lock) == 0);
    ++*count;
    assert(pthread_cond_broadcast(&cond) == 0);
    while (*count < 2U) assert(pthread_cond_wait(&cond, &lock) == 0);
    assert(pthread_mutex_unlock(&lock) == 0);
}

static int concurrent_entry(const maia_module_context_t *ctx) {
    rendezvous(&entered);
    assert(s_active_host == ctx->host);
    uint32_t expected = ctx->host == &outer_host ? 111U : 222U;
    assert(maia_host_millis() == expected);
    rendezvous(&checked);
    return (int)expected;
}

struct thread_arg {
    const maia_host_api_v1 *host;
};

static void *run_thread(void *arg) {
    const struct thread_arg *thread = arg;
    const maia_host_api_v1 *host = thread->host;
    const maia_module_context_t ctx = {.host = host};
    assert(s_active_host == NULL);
    assert(mnm_invoke_entry(concurrent_entry, &ctx, 0) ==
           (host == &outer_host ? 111 : 222));
    assert(s_active_host == NULL);
    return NULL;
}
#endif

int main(void) {
    check_display_forwarding();
    check_streaming_loader();
    check_invalid_load_preserves_host(NULL);
    const maia_module_context_t outer = {.host = &outer_host};
    assert(mnm_invoke_entry(outer_entry, &outer, 0) == 42);
    assert(s_active_host == NULL);
    assert(outer_delays == 1U && inner_delays == 1U);

#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS
    struct thread_arg first_arg = {&outer_host};
    struct thread_arg second_arg = {&inner_host};
    pthread_t first;
    pthread_t second;
    assert(pthread_create(&first, NULL, run_thread, &first_arg) == 0);
    assert(pthread_create(&second, NULL, run_thread, &second_arg) == 0);
    assert(pthread_join(first, NULL) == 0);
    assert(pthread_join(second, NULL) == 0);
#endif
    return 0;
}
