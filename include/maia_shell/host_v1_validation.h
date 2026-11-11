#ifndef MAIA_SHELL_HOST_V1_VALIDATION_H
#define MAIA_SHELL_HOST_V1_VALIDATION_H

#include <maia_shell/host_v1.h>

/* Validate the existing v1 prefix without changing its layout or error values.
 * Returns MSH_OK or MSH_ERR_INVALID. A newer minor version/larger struct may
 * append fields; an unknown major version cannot be interpreted here.
 * Pure/nonblocking; thread/ISR-safe while the caller keeps host immutable.
 * The caller must supply readable storage for its declared struct_size.
 */
static inline int32_t msh_host_v1_validate(const msh_host_v1 *host) {
    if (host == NULL || host->struct_size < (uint32_t)sizeof(msh_host_v1)) {
        return MSH_ERR_INVALID;
    }
    if ((host->version >> 16U) != (MSH_HOST_VERSION_1 >> 16U)) {
        return MSH_ERR_INVALID;
    }
    if (host->read == NULL || host->write == NULL || host->open == NULL ||
        host->close == NULL || host->pipe_create == NULL || host->fd_dup == NULL ||
        host->spawn == NULL || host->wait == NULL || host->cwd_get == NULL ||
        host->cwd_set == NULL || host->glob == NULL) {
        return MSH_ERR_INVALID;
    }
    return MSH_OK;
}

#endif
