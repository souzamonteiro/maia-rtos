#pragma once

#include <stdint.h>
#include <stddef.h>
#include "mnm.h"
#include "elf_validator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MNM_LOAD_OK               0
#define MNM_ERR_VALIDATION_FAILED -1
#define MNM_ERR_NO_TEXT_SECTION   -2
#define MNM_ERR_NO_ENTRY_POINT    -3
#define MNM_ERR_UNSUPPORTED_RELOC -4
#define MNM_ERR_OUT_OF_MEMORY     -5
#define MNM_ERR_EXECUTION_FAULT   -6

/**
 * @brief Load, relocate and execute an in-memory RISC-V ELF32 ET_REL module.
 *
 * @param elf_data Pointer to the ELF32 binary buffer.
 * @param elf_size Size of the buffer in bytes.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param host Host API dispatch table.
 * @param exit_code Out parameter receiving the return code of maia_module_main.
 * @param log_fn Optional logging callback for verbose loader diagnostics.
 * @return 0 on success, negative error code on loader failure.
 */
int mnm_load_and_run(const void *elf_data, size_t elf_size,
                     int argc, const char *const *argv,
                     const maia_host_api_v1 *host,
                     int *exit_code,
                     void (*log_fn)(const char *msg));

#ifdef __cplusplus
}
#endif

