#include <maia/kernel.h>
#include <maia/soc/esp32p4_regs.h>

extern void _maia_trap_entry(void);
extern void maia_display_refresh_dma_if_needed(void);

uint64_t maia_soc_time_us(void) {
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) = MAIA_ESP32P4_SYSTIMER_UNIT0_UPDATE;
    for (uint32_t spin = 0U; spin < 10000U; ++spin) {
        if ((MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) & MAIA_ESP32P4_SYSTIMER_UNIT0_VALID) != 0UL) {
            break;
        }
    }
    uint32_t hi1 = MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_HI_REG) & 0x000FFFFFUL;
    uint32_t lo  = MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_LO_REG);
    uint64_t ticks = ((uint64_t)hi1 << 32U) | (uint64_t)lo;
    uint64_t sys_us = ticks / (uint64_t)MAIA_ESP32P4_SYSTIMER_TICKS_PER_US;
    if (sys_us != 0ULL) {
        return sys_us;
    }
    uint32_t mcyc_hi = 0U;
    uint32_t mcyc_lo = 0U;
    __asm__ volatile("csrr %0, mcycleh\n\tcsrr %1, mcycle" : "=r"(mcyc_hi), "=r"(mcyc_lo));
    return (((uint64_t)mcyc_hi << 32U) | (uint64_t)mcyc_lo) / 40ULL;
}

void maia_soc_trap_init(void);
void maia_soc_trap_init(void) {
    uintptr_t vec = ((uintptr_t)&_maia_trap_entry) | 3UL;
    __asm__ volatile("csrw mtvec, %0" :: "r"(vec) : "memory");
}

void maia_soc_systimer_init(uint32_t tick_hz);
void maia_soc_systimer_init(uint32_t tick_hz) {
    if (tick_hz == 0U) {
        tick_hz = 1000U;
    }
    uint32_t period_ticks = (uint32_t)(MAIA_ESP32P4_SYSTIMER_HZ / tick_hz);
    if (period_ticks > 0x03FFFFFFUL) {
        period_ticks = 0x03FFFFFFUL;
    }

    maia_soc_trap_init();

    /* Ensure SYSTIMER APB bus clock and XTAL function clock are enabled */
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG)  |= MAIA_ESP32P4_HP_CLKRST_SYSTIMER_APB_EN;
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG) =
        (MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG) & ~MAIA_ESP32P4_HP_CLKRST_SYSTIMER_SRC_RC) |
        MAIA_ESP32P4_HP_CLKRST_SYSTIMER_CLK_EN;

    /* Configure CLIC: NLBITS = 3, CPU interrupt threshold = level 0 */
    MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_CONFIG_REG) = (3UL << 1U);
    MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_THRESH_REG) = (0x1FUL << 24U);
    (void)MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_THRESH_REG);

    /* Route ETS_SYSTIMER_TARGET0_INTR_SOURCE (53) to CLIC interrupt ID 16 on Core 0 */
    MAIA_MMIO32(MAIA_ESP32P4_INTR_CORE0_SYSTIMER0_MAP) = MAIA_ESP32P4_CLIC_SYSTIMER_INTR_ID;

    /* Configure CLIC interrupt 16: prio=1 (0x3F<<24), mode=M (0x3<<22), SHV=0, TRIG=level, IE=1 */
    MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_CTRL_REG(MAIA_ESP32P4_CLIC_SYSTIMER_INTR_ID)) =
        (0x3FUL << 24U) | (0x3UL << 22U) | MAIA_ESP32P4_CLIC_INT_IE;

    /* Configure SYSTIMER TARGET0 in periodic mode on UNIT0, clearing CPU stall bits (Core 1 is stalled!) */
    uint32_t conf = MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_CONF_REG);
    conf &= ~(MAIA_ESP32P4_SYSTIMER_CPU_STALL_MASK | MAIA_ESP32P4_SYSTIMER_TARGET0_WORK_EN);
    conf |= (MAIA_ESP32P4_SYSTIMER_CLK_EN | MAIA_ESP32P4_SYSTIMER_UNIT0_WORK_EN | 1UL);
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_CONF_REG) = conf;

    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_TARGET0_CONF_REG) =
        MAIA_ESP32P4_SYSTIMER_TARGET0_PERIOD_MODE | (period_ticks & 0x03FFFFFFUL);
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_COMP0_LOAD_REG) = 1UL;
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_INT_CLR_REG)    = MAIA_ESP32P4_SYSTIMER_TARGET0_INT_BIT;
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_INT_ENA_REG)   |= MAIA_ESP32P4_SYSTIMER_TARGET0_INT_BIT;
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_CONF_REG)      |= MAIA_ESP32P4_SYSTIMER_TARGET0_WORK_EN;
}

uintptr_t maia_riscv_trap_handler(uintptr_t sp, uint32_t mcause, uint32_t mepc, uint32_t mtval);
uintptr_t maia_riscv_trap_handler(uintptr_t sp, uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    uint32_t *frame = (uint32_t *)sp;
    maia_trap_record_event(mcause, mepc, mtval);

    if ((mcause & 0x80000000UL) != 0UL) {
        uint32_t irq_id = mcause & 0xFFFUL;
        if (irq_id == MAIA_ESP32P4_CLIC_SYSTIMER_INTR_ID) {
            MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_INT_CLR_REG) = MAIA_ESP32P4_SYSTIMER_TARGET0_INT_BIT;
            maia_display_refresh_dma_if_needed();
            (void)maia_sched_tick_isr();
            return maia_sched_select_next_sp(sp);
        }
        return sp;
    }

    /* Synchronous exception (e.g. ecall = 11, ebreak = 3) */
    uint32_t exc_code = mcause & 0xFFUL;
    if (exc_code == 11UL) {
        frame[28] = mepc + 4U;
        return maia_sched_select_next_sp(sp);
    }
    if (exc_code == 3UL) {
        frame[28] = mepc + 4U;
        return sp;
    }

    /* Advance PC on unhandled fault to avoid infinite fault loop during bring-up */
    frame[28] = mepc + 4U;
    return sp;
}
