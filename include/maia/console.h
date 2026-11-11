#ifndef MAIA_CONSOLE_H
#define MAIA_CONSOLE_H

/*
 * MaiaRTOS Bare-Metal Diagnostic Console Interface
 *
 * Concurrency & Context Contract:
 * - Legacy boot/thread console: serialize callers, including TX and display.
 * - Fatal traps use a separate bounded sink; this API is not ISR/panic-safe.
 * - All polling loops are strictly bounded to prevent infinite hardware hangs.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize early SoC watchdogs and console peripheral. */
void maia_soc_early_init(void);

/** Initialize the diagnostic console. */
void maia_console_init(void);

/** Write a single byte to the console (bounded wait). */
void maia_console_putc(char c);

/** Write `len` bytes from `buf` to the console and flush. */
void maia_console_write(const char *buf, size_t len);

/** Write a NUL-terminated string to the console and flush. */
void maia_console_puts(const char *str);

/** Flush pending console FIFO data to the host. */
void maia_console_flush(void);

/**
 * Non-blocking character read from the console.
 * Returns >= 0 byte value (0..255) if available, or -1 if no byte is ready.
 * After the optional serial input service claims RX, returns -1 without
 * consuming serial/keyboard data or calling display; use the service read API.
 */
int maia_console_getc_nonblocking(void);

/** Entry point invoked by `arch/riscv/start.S` on primary core after `.bss` init. */
void maia_boot_main(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIA_CONSOLE_H */

