#include <stddef.h>
#include <string.h>

void *memcpy(void *restrict destination, const void *restrict source, size_t size) {
    unsigned char *out = destination;
    const unsigned char *in = source;

    while (size-- != 0U) {
        *out++ = *in++;
    }

    return destination;
}
