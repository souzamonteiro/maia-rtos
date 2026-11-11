#ifndef MAIA_DRIVERS_SERIAL_RX_H
#define MAIA_DRIVERS_SERIAL_RX_H
#ifdef __cplusplus
extern "C" {
#endif
/* Single-core bounded USB-Serial/JTAG then UART0 receive, no board/display/heap.
 * NULL identifies the legacy reader until a permanent claim. A non-null stable
 * token claims the port until reset; claim is boot-only with quiescent readers.
 * Caller retains token lifetime, passes it only to the one designated reader.
 * Returns 0 on claim, -2 on null token, -3 if already claimed. No security boundary.
 * read returns byte 0..255, -1 empty, -2 wrong token (without touching MMIO).
 * Reads serialize check+FIFO access with local IRQ exclusion, preserving nesting.
 * No SMP or fairness guarantee; sustained USB traffic may postpone UART reads.
 */
int maia_serial_rx_claim(const void *owner);
int maia_serial_rx_read(const void *owner);
#ifdef __cplusplus
}
#endif
#endif
