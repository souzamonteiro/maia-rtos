#ifndef MAIA_DRIVERS_SERIAL_TX_H
#define MAIA_DRIVERS_SERIAL_TX_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_serial_tx_sink { MAIA_SERIAL_TX_USB=0, MAIA_SERIAL_TX_UART0=1 } maia_serial_tx_sink_t;
/* Bounded single-core raw TX, no display/heap/spin. Each try checks readiness
 * once; returns 0 accepted by FIFO, -1 busy, -2 wrong token, -3 invalid sink.
 * Accepted does not mean host receipt. USB commits at a software watermark of
 * 32 bytes; flush commits a shorter packet. UART flush is a no-op. No CR/LF map.
 * NULL is legacy owner until boot-only permanent claim by a stable non-null
 * token (0 success, -2 null, -3 already owned). Claim flushes pending legacy USB.
 * Caller ensures quiescence, token lifetime and a sole logical writer per token.
 * Operations preserve local IRQ masking; no SMP/security/isolation guarantee.
 * legacy_init resets software packet count only before claim; otherwise no-op.
 * may_write returns token equality without touching MMIO; flush returns 0/-2/-3.
 */
void maia_serial_tx_legacy_init(void);
int maia_serial_tx_claim(const void *owner);
int maia_serial_tx_may_write(const void *owner);
int maia_serial_tx_try_put(const void *owner, maia_serial_tx_sink_t sink, uint8_t byte);
int maia_serial_tx_flush(const void *owner, maia_serial_tx_sink_t sink);
#ifdef __cplusplus
}
#endif
#endif
