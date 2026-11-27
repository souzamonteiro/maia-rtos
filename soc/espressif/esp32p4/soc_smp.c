/* ESP32-P4 SMP port: IPIs, core-1 scheduler entry, flash freeze, migration. */
#include <maia/arch/irq.h>
#include <maia/arch/riscv_stack.h>
#include <maia/scheduler_internal.h>
#include <maia/soc/esp32p4_core1.h>
#include <maia/soc/esp32p4_regs.h>
#include <maia/soc/esp32p4_smp.h>
#include "core1_internal.h"

#define CLIC_LINE_CTRL ((0x3FUL << 24U) | (0x3UL << 22U) | MAIA_ESP32P4_CLIC_INT_IE)
#define FREEZE_POLL_LIMIT 20000000U
#define IRAM __attribute__((section(".maia_flash_iram"), noinline))

extern void _maia_trap_entry(void);

static volatile uint32_t core1_online;
static volatile uint32_t freeze_req, freeze_ack;
static uint32_t freeze_seq;
static maia_esp32p4_smp_stats_t stats;

static void route_ipi_line(uintptr_t map_reg) {
    MAIA_MMIO32(map_reg) = MAIA_ESP32P4_CLIC_IPI_INTR_ID;
    MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_CTRL_REG(MAIA_ESP32P4_CLIC_IPI_INTR_ID)) = CLIC_LINE_CTRL;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}

void maia_esp32p4_smp_init_core0(void) {
    MAIA_MMIO32(MAIA_ESP32P4_CPU_INT_FROM_CPU_REG(1U)) = 0U;
    route_ipi_line(MAIA_ESP32P4_INTR_CORE0_MAP_REG(MAIA_ESP32P4_INTR_SRC_FROM_CPU1));
}

/* Raise the IPI towards `core` (FROM_CPU_n targets core 1-n here). */
static void raise_ipi(uint32_t core) {
    MAIA_MMIO32(MAIA_ESP32P4_CPU_INT_FROM_CPU_REG(core == 1U ? 0U : 1U)) = 1U;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}

void maia_arch_sched_kick(uint32_t core) {
    if (core == maia_arch_core_id()) return;
    if (core == 1U && !core1_online) return;
    raise_ipi(core);
}

/* SRAM-only: core 1 waits here while core 0 has the L2 cache suspended. */
static void IRAM freeze_service(void) {
    uint32_t seq = freeze_req;
    if (seq == 0U) return;
    __asm__ volatile("fence rw, rw" ::: "memory");
    freeze_ack = seq;
    while (freeze_req == seq) {
    }
    __asm__ volatile("fence rw, rw" ::: "memory");
}

int maia_esp32p4_smp_ipi_isr(void) {
    uint32_t core = maia_arch_core_id();
    if (core > 1U) return 0;
    /* Clear the level source first: a raise after this is taken again. */
    MAIA_MMIO32(MAIA_ESP32P4_CPU_INT_FROM_CPU_REG(core == 1U ? 0U : 1U)) = 0U;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    __atomic_add_fetch(&stats.ipi[core], 1U, __ATOMIC_RELAXED);
    if (core == 1U) {
        __atomic_add_fetch(&maia_core1_mailbox.heartbeat, 1U, __ATOMIC_RELAXED);
        freeze_service();
    }
    return 1;
}

__attribute__((noreturn)) static void secondary_main(void) {
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
    (void)maia_arch_trap_stack_init_secondary();
    uintptr_t vec = ((uintptr_t)&_maia_trap_entry) | 3UL;
    __asm__ volatile("csrw mtvec, %0" :: "r"(vec) : "memory");
    /* Core-local CLIC: same setup as core 0, plus the IPI line. */
    MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_CONFIG_REG) = (3UL << 1U);
    MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_THRESH_REG) = (0x1FUL << 24U);
    (void)MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_THRESH_REG);
    MAIA_MMIO32(MAIA_ESP32P4_CPU_INT_FROM_CPU_REG(0U)) = 0U;
    route_ipi_line(MAIA_ESP32P4_INTR_CORE1_MAP_REG(MAIA_ESP32P4_INTR_SRC_FROM_CPU0));
    __atomic_store_n(&core1_online, 1U, __ATOMIC_RELEASE);
    stats.online = 1U;
    maia_sched_start_secondary();
    for (;;) __asm__ volatile("wfi");
}

int maia_esp32p4_smp_launch_core1(void) {
    return (int)maia_esp32p4_core1_launch((uintptr_t)&secondary_main,
                                          maia_arch_trap_stack_secondary_top());
}

int maia_esp32p4_smp_core1_online(void) {
    return (int)__atomic_load_n(&core1_online, __ATOMIC_ACQUIRE);
}

void maia_esp32p4_smp_get_stats(maia_esp32p4_smp_stats_t *out) {
    if (!out) return;
    uintptr_t irq = maia_arch_irq_save();
    *out = stats;
    maia_arch_irq_restore(irq);
}

int maia_esp32p4_smp_freeze_core1(void) {
    if (maia_arch_core_id() != 0U) return -1;
    if (!core1_online) return 0;
    if (maia_arch_klock_depth() != 0U) return -1; /* Core 1 could be waiting on it. */
    uint32_t seq = ++freeze_seq;
    if (seq == 0U) seq = ++freeze_seq;
    freeze_req = seq;
    __asm__ volatile("fence rw, rw" ::: "memory");
    raise_ipi(1U);
    for (uint32_t i = 0U; i < FREEZE_POLL_LIMIT; ++i) {
        if (freeze_ack == seq) {
            ++stats.freezes;
            return 0;
        }
    }
    freeze_req = 0U;
    return -1;
}

void maia_esp32p4_smp_thaw_core1(void) {
    __asm__ volatile("fence rw, rw" ::: "memory");
    freeze_req = 0U;
}

int maia_esp32p4_smp_core0_begin(uint8_t *saved) {
    if (!saved) return -1;
    maia_task_id_t self = maia_task_current_id();
    maia_task_info_t info;
    if (self < 0 || maia_task_get_info((uint32_t)self, &info) != MAIA_OK) {
        if (maia_arch_core_id() != 0U) return -1;
        *saved = 0U;
        return 0;
    }
    *saved = info.affinity;
    if (maia_arch_core_id() == 0U && info.affinity == 1U) return 0;
    if (maia_task_set_affinity(self, 1U) != MAIA_OK) return -1;
    while (maia_arch_core_id() != 0U) maia_task_yield();
    uintptr_t irq = maia_arch_irq_save();
    ++stats.migrations;
    maia_arch_irq_restore(irq);
    return 0;
}

void maia_esp32p4_smp_core0_end(uint8_t saved) {
    if (saved == 0U || saved == 1U) return;
    maia_task_id_t self = maia_task_current_id();
    if (self < 0) return;
    (void)maia_task_set_affinity(self, saved);
    maia_task_yield();
}
