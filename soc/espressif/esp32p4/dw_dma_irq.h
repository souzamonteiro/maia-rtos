/* Original MaiaRTOS MIT. Core0 DW-GDMA IRQ ownership, no allocator/foreign OS.
 * Boot/quiescent IRQs-off claim/release; matrix source24 -> free CLIC17..47.
 * Exact128-source scan excludes another producer on the chosen line; source/controller idle,
 * line disabled/nonpending required. Short bounded callback on native IRQ stack,
 * no printing/blocking/cache maintenance there. Callback must mask its source.
 * Any post-store guard failure sticky until reset; reports are not ownership. */
#ifndef MAIA_DW_DMA_IRQ_H
#define MAIA_DW_DMA_IRQ_H
#include <stdint.h>
#define MAIA_DW_IRQ_SOURCE 24U
#define MAIA_DW_IRQ_FIRST 17U
#define MAIA_DW_IRQ_LIMIT 48U
typedef void (*maia_dw_irq_callback_t)(void *);
int maia_dw_irq_claim(maia_dw_irq_callback_t,void *);
int maia_dw_irq_release(void);
uint32_t maia_dw_irq_stage(void);
uint32_t maia_dw_irq_line(void);
uint32_t maia_dw_irq_saved_ctrl(void);
int maia_soc_external_irq_dispatch(uint32_t id);
#endif
