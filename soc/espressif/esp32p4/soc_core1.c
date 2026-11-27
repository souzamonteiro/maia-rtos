/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register protocol adapted from ESP-IDF f5c3654 start_other_core and
 * cpu_utility_ll_unstall_cpu. Native bounded diagnostic; no vendor runtime.
 * See docs/hardware/EVIDENCE.md and LICENSES/Apache-2.0.txt.
 */
#include <maia/soc/esp32p4_core1.h>
#include <maia/soc/esp32p4_regs.h>
#include "core1_internal.h"

#define POLL_LIMIT 2000000U
#define STACK_GUARD 0x51ac0ffeU

maia_core1_mailbox_t maia_core1_mailbox __attribute__((aligned(16)));
uint32_t maia_core1_stack[MAIA_CORE1_STACK_BYTES / sizeof(uint32_t)]
    __attribute__((aligned(16)));
static unsigned attempted, online, ping_busy, launched;
static uint32_t last_command, pings;

#ifdef MAIA_CORE1_TEST
int maia_core1_test_context(void);
uint32_t maia_core1_test_read(uintptr_t address);
void maia_core1_test_write(uintptr_t address, uint32_t value);
void maia_core1_test_poll(void);
#define read32 maia_core1_test_read
#define write32 maia_core1_test_write
#else
static uint32_t read32(uintptr_t address) { return MAIA_MMIO32(address); }
static void write32(uintptr_t address, uint32_t value) {
    MAIA_MMIO32(address) = value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif

static int boot_context(void) {
#ifdef MAIA_CORE1_TEST
    return maia_core1_test_context();
#else
    uintptr_t hart, status;
    __asm__ volatile("csrr %0, mhartid\n\tcsrr %1, mstatus"
                     : "=r"(hart), "=r"(status));
    return hart == 0U && !(status & 8U);
#endif
}

static maia_core1_result_t wait_word(uint32_t *word, uint32_t expected) {
    for (unsigned i = 0U; i < POLL_LIMIT; ++i) {
#ifdef MAIA_CORE1_TEST
        maia_core1_test_poll();
#endif
        if (__atomic_load_n(&maia_core1_mailbox.state, __ATOMIC_ACQUIRE) == 3U) {
            return MAIA_CORE1_FAULT;
        }
        if (__atomic_load_n(word, __ATOMIC_ACQUIRE) == expected) {
            return MAIA_CORE1_OK;
        }
    }
    return MAIA_CORE1_TIMEOUT;
}

static maia_core1_result_t exchange(uint32_t command) {
    uint32_t payload = command * 0x10203U;
    maia_core1_mailbox.payload = payload;
    __atomic_store_n(&maia_core1_mailbox.command, command, __ATOMIC_RELEASE);
    maia_core1_result_t result = wait_word(&maia_core1_mailbox.ack, command);
    if (result != MAIA_CORE1_OK) return result;
    if (maia_core1_mailbox.result != ((payload ^ MAIA_CORE1_XOR) + command)) {
        return MAIA_CORE1_VERIFY;
    }
    return MAIA_CORE1_OK;
}

static maia_core1_result_t release_worker(maia_core1_report_t *report,
                                          uint32_t rounds) {
    report->clock_before = read32(MAIA_CORE1_CLOCK_REG);
    report->reset_before = read32(MAIA_CORE1_RESET_REG);
    report->stall_before = read32(MAIA_CORE1_STALL_REG);
    report->boot_before = read32(MAIA_CORE1_BOOT_REG);
    if (report->boot_before != 0U) return MAIA_CORE1_BUSY;

#ifdef MAIA_CORE1_TEST
    uintptr_t entry = 0x4ff00100U;
#else
    uintptr_t entry = (uintptr_t)maia_core1_entry;
    if (entry < MAIA_ESP32P4_L2MEM_LOW ||
        entry >= MAIA_ESP32P4_L2MEM_LOW + 0x28000U || (entry & 3U)) {
        return MAIA_CORE1_INVALID;
    }
#endif
    attempted = 1U;
    for (unsigned i = 0U; i < 4U; ++i) maia_core1_stack[i] = STACK_GUARD;
    uint32_t stall = (report->stall_before & ~MAIA_CORE1_STALL_MASK) |
                     MAIA_CORE1_STALL_MASK;
    write32(MAIA_CORE1_STALL_REG, stall);
    for (unsigned i = 0U;; ++i) {
        if (!(read32(MAIA_CORE1_STALLED_REG) & MAIA_CORE1_STALLED_BIT)) break;
        if (i == POLL_LIMIT - 1U) return MAIA_CORE1_TIMEOUT;
    }
    if (read32(MAIA_CORE1_STALL_REG) != stall) return MAIA_CORE1_VERIFY;
    report->completed = 1U;

    uint32_t clock = report->clock_before | MAIA_CORE1_CLOCK_BIT;
    write32(MAIA_CORE1_CLOCK_REG, clock);
    if (read32(MAIA_CORE1_CLOCK_REG) != clock) return MAIA_CORE1_VERIFY;
    uint32_t reset = report->reset_before & ~MAIA_CORE1_RESET_BIT;
    write32(MAIA_CORE1_RESET_REG, reset);
    if (read32(MAIA_CORE1_RESET_REG) != reset) return MAIA_CORE1_VERIFY;
    report->completed = 2U;

    __atomic_thread_fence(__ATOMIC_RELEASE);
    write32(MAIA_CORE1_BOOT_REG, (uint32_t)entry);
    maia_core1_result_t result = wait_word(&maia_core1_mailbox.state, 1U);
    if (result != MAIA_CORE1_OK) return result;
    report->hart = maia_core1_mailbox.hart;
    report->stack_pointer = maia_core1_mailbox.sp;
    if (report->hart != 1U ||
        report->stack_pointer != (uint32_t)((uintptr_t)maia_core1_stack +
                                            sizeof maia_core1_stack) ||
        read32(MAIA_CORE1_BOOT_REG) != 0U) return MAIA_CORE1_VERIFY;
    report->completed = 3U;

    for (uint32_t round = 1U; round <= rounds; ++round) {
        result = exchange(round);
        if (result != MAIA_CORE1_OK) return result;
        report->rounds = round;
    }
    last_command = rounds;
    report->completed = 4U;
    return MAIA_CORE1_OK;
}

static maia_core1_result_t run_probe(maia_core1_report_t *report) {
    maia_core1_result_t result = release_worker(report, MAIA_CORE1_PROBE_ROUNDS);
    if (result != MAIA_CORE1_OK) return result;
    __atomic_store_n(&maia_core1_mailbox.command, MAIA_CORE1_STOP, __ATOMIC_RELEASE);
    result = wait_word(&maia_core1_mailbox.ack, MAIA_CORE1_STOP);
    if (result != MAIA_CORE1_OK) return result;
    if (__atomic_load_n(&maia_core1_mailbox.state, __ATOMIC_ACQUIRE) != 2U) {
        return MAIA_CORE1_VERIFY;
    }
    for (unsigned i = 0U; i < 4U; ++i) {
        if (maia_core1_stack[i] != STACK_GUARD) return MAIA_CORE1_VERIFY;
    }
    report->completed = 5U;
    return MAIA_CORE1_OK;
}

static maia_core1_result_t finish(maia_core1_report_t *report,
                                  maia_core1_result_t result) {
    if (result == MAIA_CORE1_FAULT) {
        report->cause = maia_core1_mailbox.cause;
        report->pc = maia_core1_mailbox.pc;
    }
    return result;
}

maia_core1_result_t maia_esp32p4_core1_probe(maia_core1_report_t *report) {
    if (report == NULL) return MAIA_CORE1_INVALID;
    *report = (maia_core1_report_t){0};
    if (!boot_context()) return MAIA_CORE1_CONTEXT;
    if (attempted) return MAIA_CORE1_BUSY;
    return finish(report, run_probe(report));
}

maia_core1_result_t maia_esp32p4_core1_start(maia_core1_report_t *report) {
    if (report == NULL) return MAIA_CORE1_INVALID;
    *report = (maia_core1_report_t){0};
    if (!boot_context()) return MAIA_CORE1_CONTEXT;
    if (attempted) return MAIA_CORE1_BUSY;
    maia_core1_result_t result =
        finish(report, release_worker(report, MAIA_CORE1_START_ROUNDS));
    if (result == MAIA_CORE1_OK) {
        __atomic_store_n(&online, 1U, __ATOMIC_RELEASE);
    }
    return result;
}

maia_core1_result_t maia_esp32p4_core1_launch(uintptr_t entry, uintptr_t stack_top) {
    if (!entry || (entry & 1U) || !stack_top || (stack_top & 15U)) return MAIA_CORE1_INVALID;
    if (!boot_context()) return MAIA_CORE1_CONTEXT;
    if (!__atomic_load_n(&online, __ATOMIC_ACQUIRE) || launched) return MAIA_CORE1_BUSY;
    maia_core1_mailbox.payload = (uint32_t)entry;
    maia_core1_mailbox.result = (uint32_t)stack_top;
    __atomic_store_n(&maia_core1_mailbox.command, MAIA_CORE1_LAUNCH, __ATOMIC_RELEASE);
    maia_core1_result_t result = wait_word(&maia_core1_mailbox.ack, MAIA_CORE1_LAUNCH);
    if (result != MAIA_CORE1_OK) {
        __atomic_store_n(&online, 0U, __ATOMIC_RELEASE);
        return result;
    }
    if (__atomic_load_n(&maia_core1_mailbox.state, __ATOMIC_ACQUIRE) != MAIA_CORE1_STATE_LAUNCHED) {
        __atomic_store_n(&online, 0U, __ATOMIC_RELEASE);
        return MAIA_CORE1_VERIFY;
    }
    __atomic_store_n(&launched, 1U, __ATOMIC_RELEASE);
    return MAIA_CORE1_OK;
}

int maia_esp32p4_core1_launched(void) {
    return (int)__atomic_load_n(&launched, __ATOMIC_ACQUIRE);
}

maia_core1_result_t maia_esp32p4_core1_ping(maia_core1_status_t *status) {
    if (status == NULL) return MAIA_CORE1_INVALID;
    *status = (maia_core1_status_t){0};
    if (!__atomic_load_n(&online, __ATOMIC_ACQUIRE)) return MAIA_CORE1_CONTEXT;
    if (__atomic_load_n(&launched, __ATOMIC_ACQUIRE)) {
        /* The scheduler owns core 1: its IPI handler advances the heartbeat. */
        status->online = 1U;
        status->hart = 1U;
        status->heartbeat_before = __atomic_load_n(&maia_core1_mailbox.heartbeat, __ATOMIC_ACQUIRE);
        for (volatile unsigned spin = 0U; spin < 400000U; ++spin) {
        }
        status->heartbeat_after = __atomic_load_n(&maia_core1_mailbox.heartbeat, __ATOMIC_ACQUIRE);
        status->pings = ++pings;
        return status->heartbeat_after != status->heartbeat_before ? MAIA_CORE1_OK : MAIA_CORE1_TIMEOUT;
    }
    if (__atomic_exchange_n(&ping_busy, 1U, __ATOMIC_ACQUIRE)) {
        return MAIA_CORE1_BUSY;
    }
    status->online = 1U;
    status->heartbeat_before =
        __atomic_load_n(&maia_core1_mailbox.heartbeat, __ATOMIC_ACQUIRE);
    uint32_t command = last_command + 1U;
    if (command == 0U || command == MAIA_CORE1_STOP) command = 1U;
    maia_core1_result_t result = exchange(command);
    if (result == MAIA_CORE1_OK) {
        last_command = command;
        ++pings;
    } else {
        __atomic_store_n(&online, 0U, __ATOMIC_RELEASE);
        status->online = 0U;
        status->cause = maia_core1_mailbox.cause;
        status->pc = maia_core1_mailbox.pc;
    }
    status->heartbeat_after =
        __atomic_load_n(&maia_core1_mailbox.heartbeat, __ATOMIC_ACQUIRE);
    status->pings = pings;
    status->hart = maia_core1_mailbox.hart;
    __atomic_store_n(&ping_busy, 0U, __ATOMIC_RELEASE);
    return result;
}
