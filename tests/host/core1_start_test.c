#include <assert.h>
#include <string.h>

#include "../../soc/espressif/esp32p4/soc_core1.c"

static uint32_t clock_reg, reset_reg, stall_reg, boot_reg;
static unsigned context_ok, writes, launch, polls, drop, stuck, mode;

int maia_core1_test_context(void) { return (int)context_ok; }

uint32_t maia_core1_test_read(uintptr_t address) {
    switch (address) {
    case MAIA_CORE1_CLOCK_REG: return clock_reg;
    case MAIA_CORE1_RESET_REG: return reset_reg;
    case MAIA_CORE1_STALL_REG: return stall_reg;
    case MAIA_CORE1_BOOT_REG: return boot_reg;
    case MAIA_CORE1_STALLED_REG: return stuck ? MAIA_CORE1_STALLED_BIT : 0U;
    default: assert(0); return 0U;
    }
}

void maia_core1_test_write(uintptr_t address, uint32_t value) {
    ++writes;
    if (writes == drop) return;
    switch (address) {
    case MAIA_CORE1_STALL_REG:
        assert(writes == 1U);
        assert((value & ~MAIA_CORE1_STALL_MASK) ==
               (stall_reg & ~MAIA_CORE1_STALL_MASK));
        stall_reg = value;
        break;
    case MAIA_CORE1_CLOCK_REG:
        assert(writes == 2U && value == (clock_reg | MAIA_CORE1_CLOCK_BIT));
        clock_reg = value;
        break;
    case MAIA_CORE1_RESET_REG:
        assert(writes == 3U && value == (reset_reg & ~MAIA_CORE1_RESET_BIT));
        reset_reg = value;
        break;
    case MAIA_CORE1_BOOT_REG:
        assert(writes == 4U && value == 0x4ff00100U);
        assert(clock_reg & MAIA_CORE1_CLOCK_BIT);
        assert(!(reset_reg & MAIA_CORE1_RESET_BIT));
        assert(maia_core1_stack[0] == STACK_GUARD);
        boot_reg = value;
        launch = 1U;
        break;
    default: assert(0);
    }
}

void maia_core1_test_poll(void) {
    ++polls;
    if (!launch || mode == 1U) return;
    if (mode == 2U) {
        maia_core1_mailbox.cause = 2U;
        maia_core1_mailbox.pc = 0x4ff00120U;
        maia_core1_mailbox.state = 3U;
        return;
    }
    if (!maia_core1_mailbox.state) {
        maia_core1_mailbox.hart = mode == 3U ? 0U : 1U;
        maia_core1_mailbox.sp =
            (uint32_t)((uintptr_t)maia_core1_stack + sizeof maia_core1_stack);
        if (mode == 4U) maia_core1_mailbox.sp -= 16U;
        boot_reg = mode == 5U ? boot_reg : 0U;
        maia_core1_mailbox.state = 1U;
        return;
    }
    uint32_t command = maia_core1_mailbox.command;
    if (!command || command == maia_core1_mailbox.ack) return;
    if (mode == 6U && command == 7U) return;
    if (mode == 11U && command == MAIA_CORE1_START_ROUNDS + 2U) return;
    if (mode == 12U && command > MAIA_CORE1_START_ROUNDS) {
        maia_core1_mailbox.cause = 5U;
        maia_core1_mailbox.pc = 0x4ff00140U;
        maia_core1_mailbox.state = 3U;
        return;
    }
    ++maia_core1_mailbox.heartbeat;
    if (command == MAIA_CORE1_STOP) {
        if (mode == 7U) return;
        maia_core1_mailbox.state = mode == 8U ? 1U : 2U;
        if (mode == 9U) maia_core1_stack[0] ^= 1U;
    } else {
        maia_core1_mailbox.result =
            (maia_core1_mailbox.payload ^ MAIA_CORE1_XOR) + command;
        if (mode == 10U) ++maia_core1_mailbox.result;
    }
    maia_core1_mailbox.ack = command;
}

static void reset_test(void) {
    attempted = online = ping_busy = 0U;
    last_command = pings = 0U;
    memset(&maia_core1_mailbox, 0, sizeof maia_core1_mailbox);
    memset(maia_core1_stack, 0, sizeof maia_core1_stack);
    context_ok = 1U;
    writes = launch = polls = drop = stuck = mode = 0U;
    clock_reg = 0xa5a50000U;
    reset_reg = 0x5a5a0100U;
    stall_reg = 0xab1289cdU;
    boot_reg = 0U;
}

int main(void) {
    maia_core1_report_t report;
    reset_test();
    assert(maia_esp32p4_core1_probe(NULL) == MAIA_CORE1_INVALID && !writes);
    context_ok = 0U;
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_CONTEXT && !writes);

    reset_test(); boot_reg = 0x1234U;
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_BUSY && !writes);

    for (unsigned n = 1U; n <= 3U; ++n) {
        reset_test(); drop = n;
        assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_VERIFY);
        assert(writes == n && !launch);
        assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_BUSY && writes == n);
    }
    reset_test(); stuck = 1U;
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_TIMEOUT && writes == 1U);
    reset_test(); drop = 4U;
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_TIMEOUT && !launch);

    for (unsigned n = 1U; n <= 10U; ++n) {
        reset_test(); mode = n;
        maia_core1_result_t expected =
            n == 1U || n == 6U || n == 7U ? MAIA_CORE1_TIMEOUT :
            n == 2U ? MAIA_CORE1_FAULT : MAIA_CORE1_VERIFY;
        assert(maia_esp32p4_core1_probe(&report) == expected);
        assert(launch && polls <= POLL_LIMIT + MAIA_CORE1_PROBE_ROUNDS + 2U);
        if (n == 2U) assert(report.cause == 2U && report.pc == 0x4ff00120U);
        if (n == 6U) assert(report.rounds == 6U);
        assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_BUSY);
    }

    reset_test();
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_OK);
    assert(report.completed == 5U && report.hart == 1U);
    assert(report.rounds == 1024U && !report.cause && !report.pc);
    assert(maia_core1_mailbox.state == 2U && maia_core1_mailbox.ack == MAIA_CORE1_STOP);
    assert(writes == 4U && polls == 1026U);
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_BUSY && writes == 4U);

    reset_test(); clock_reg |= MAIA_CORE1_CLOCK_BIT; reset_reg &= ~MAIA_CORE1_RESET_BIT;
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_OK);
    assert(report.completed == 5U && report.rounds == 1024U);

    maia_core1_status_t status;
    reset_test();
    assert(maia_esp32p4_core1_ping(NULL) == MAIA_CORE1_INVALID);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_CONTEXT && !status.online);
    assert(maia_esp32p4_core1_start(NULL) == MAIA_CORE1_INVALID && !writes);
    context_ok = 0U;
    assert(maia_esp32p4_core1_start(&report) == MAIA_CORE1_CONTEXT && !writes);
    context_ok = 1U;
    assert(maia_esp32p4_core1_start(&report) == MAIA_CORE1_OK);
    assert(report.completed == 4U && report.rounds == MAIA_CORE1_START_ROUNDS);
    assert(maia_core1_mailbox.state == 1U && writes == 4U);
    assert(maia_esp32p4_core1_start(&report) == MAIA_CORE1_BUSY);
    assert(maia_esp32p4_core1_probe(&report) == MAIA_CORE1_BUSY && writes == 4U);
    for (uint32_t i = 1U; i <= 3U; ++i) {
        assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_OK);
        assert(status.online && status.hart == 1U && status.pings == i);
        assert(status.heartbeat_after > status.heartbeat_before);
        assert(maia_core1_mailbox.ack == MAIA_CORE1_START_ROUNDS + i);
    }
    ping_busy = 1U;
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_BUSY);
    ping_busy = 0U;

    reset_test(); mode = 11U;
    assert(maia_esp32p4_core1_start(&report) == MAIA_CORE1_OK);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_OK);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_TIMEOUT && !status.online);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_CONTEXT);

    reset_test(); mode = 12U;
    assert(maia_esp32p4_core1_start(&report) == MAIA_CORE1_OK);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_FAULT);
    assert(!status.online && status.cause == 5U && status.pc == 0x4ff00140U);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_CONTEXT);

    reset_test(); mode = 3U;
    assert(maia_esp32p4_core1_start(&report) == MAIA_CORE1_VERIFY);
    assert(maia_esp32p4_core1_ping(&status) == MAIA_CORE1_CONTEXT);
    return 0;
}
